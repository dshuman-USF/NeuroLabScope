/*
Copyright 2004-2020 Kendall F. Morris

This file is part of the Scope software suite.

    The Scope software suite is free software: you can redistribute
    it and/or modify it under the terms of the GNU General Public
    License as published by the Free Software Foundation, either
    version 3 of the License, or (at your option) any later version.

    The suite is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with the suite.  If not, see <https://www.gnu.org/licenses/>.
*/

#define _GNU_SOURCE 1
#define _FILE_OFFSET_BITS 64
#define _LARGEFILE_SOURCE 1
#define _LARGEFILE64_SOURCE 1
#include <stdbool.h>
#include <fftw3.h>
#include <math.h>
#include <float.h>
#include <limits.h>
#include <stdbool.h>
#include <error.h>
#include <errno.h>
#include "std.h"
#include "bandpass.h"
#include "filesup.h"
#include "dispUnit64.h"
#include "defines.h"
#include "gen_control.h"
#include "gammafit_search.h"
#include "unpin.h"
#include "color_table.h"

typedef struct
{
  int fftstart;
  int fftlen;
  int binticks;
  unsigned screen_offset_bin;
  unsigned bins_on_screen;
  float **tdat;
  int **spiketimes;
  int *spikecount;
  float *min;
  float *max;
  float global_min;
  float global_max;
} DataArrays;


#define MAX_CHNL (MAX_DIG_CHNL + MAX_ANLG_CHNL)

char *spike_filename;
static int sfids[MAX_DIG_CHNL + MAX_ANLG_CHNL];

#ifndef isfinite
static bool
isfinite (double x)
{
  int c = fpclassify (x);
#ifdef FP_NAN
  return c != FP_NAN && c != FP_INFINITE;
#elif defined FP_SNAN
  return c != FP_SNAN || c != FP_QNAN || c != FP_PLUS_INF || c != FP_MINUS_INF;
#else
  return true;
#endif
}
#endif

static void
free_da (DataArrays *da)
{
  int n;
  for (n = 0; n < MAX_CHNL; n++) {
    free (da->tdat[n]);
    free (da->spiketimes[n]);
  }
  free (da->tdat);
  free (da->spiketimes);
  free (da->spikecount);
  free (da->min);
  free (da->max);
}

static void
goto_time (int target_time)
{
   int time, lo, mid, hi, recidx, hitime, lotime;

   if (sf.recCount == 0 || target_time <= sf.firstTime) {
      fseek (fp, 32, SEEK_SET);
      return;
   }
   if (target_time > sf.lastTime) {
      fseek (fp, -32, SEEK_END);
      return;
   }
   if (target_time == sf.lastTime) {
      for (recidx = sf.recCount - 2; recidx >= 0; recidx--) {
         fseeko (fp, 32 + recidx * 8, SEEK_SET);
         if (getw (fp), ((time = getw (fp)) < target_time))
            return;
      }
      fseek (fp, 32, SEEK_SET);
      return;
   }
   hi = sf.recCount - 1; hitime = sf.lastTime;
   lo = 0;               lotime = sf.firstTime;
   recidx = (ftello (fp) - 32) / 8;
   if (recidx > 0 && recidx < hi)
      mid = recidx;
   else
      mid = lo + (double)(target_time - lotime) / (hitime - lotime) * (hi - lo);
   if (mid <= lo) mid = lo + 1;
   if (mid >= hi) mid = hi - 1;

   while (hi - lo > 1) {
      fseeko (fp, 32 + mid * 8, SEEK_SET);
      getw (fp); time = getw (fp);
      if (time < target_time) {
         lo = mid;
         lotime = time;
      }
      else if (time > target_time) {
         hi = mid;
         hitime = time;
      }
      else
         for (recidx = mid - 1; recidx >= lo; recidx--) {
            fseeko (fp, 32 + recidx * 8, SEEK_SET);
            if (getw (fp), ((time = getw (fp)) < target_time))
               return;
         }
      mid = lo + (double)(target_time - lotime) / (hitime - lotime) * (hi - lo);
      if (mid <= lo) mid = lo + 1;
      if (mid >= hi) mid = hi - 1;
   }
   fseeko (fp, 32 + hi * 8, SEEK_SET);
   return;
}

static inline int
get_id_time_mag (int *idp, int *timep, int *magp)
{
   int code = getw (fp);
   int time = getw (fp);
   int mag, id;
   if (code == CODEFLAG)
      return 0;
   if (code < MAX_DIG_CHNL) {
     id = code;
     mag = 0;
   }
   else {
     id = code / 4096 + MAX_DIG_CHNL;
     mag = code % 4096; mag -= (mag > 2047) * 4096;
   }
   *idp = id;
   *timep = time;
   *magp = mag;
   return 1;
}

static
DataArrays
get_tdat (int fftstart, unsigned fftend, unsigned fftlen, unsigned binticks, BandpassParams *bp)
{
   int achan_idx, chan_idx;
   float **tdat;
   int *count;
   int **spiketimes;
   DataArrays da;
   float *min, *max;
   int **sample_count;

   memset (&da, 0, sizeof da);
   tdat = calloc (MAX_CHNL, sizeof *tdat);
   spiketimes = calloc (MAX_CHNL, sizeof *spiketimes);
   count = calloc (MAX_CHNL, sizeof *count);
   min = calloc (MAX_CHNL, sizeof *min);
   max = calloc (MAX_CHNL, sizeof *max);
   sample_count = calloc (MAX_CHNL, sizeof *sample_count);
   
   for (chan_idx = l; chan_idx < sf.dchan + sf.achan && chan_idx < l + channelcount; chan_idx++) {
      tdat[sfids[chan_idx]] = calloc (fftlen, sizeof **tdat);
      if (sfids[chan_idx] < MAX_DIG_CHNL)
        spiketimes[sfids[chan_idx]] = calloc (sf.tally[chan_idx], sizeof **spiketimes);
   }
   
   for (achan_idx = 0; achan_idx < sf.achan && sf.dchan + achan_idx < l + channelcount; achan_idx++)
      sample_count[sfids[sf.dchan+achan_idx]] = calloc (fftlen, sizeof **sample_count);
   goto_time (fftstart);
   int id, time, magnitude, bin;
   while (get_id_time_mag (&id, &time, &magnitude) && time < fftend)
     if (tdat[id]) {
       bin = (time - fftstart) / binticks;
       if (id < MAX_DIG_CHNL) {
         if (count[id] == 0 || time != spiketimes[id][count[id] - 1]) { /* nodups */
           spiketimes[id][count[id]++] = time;
           tdat[id][bin]++;
         }
       }
       else {
         tdat[id][bin] += magnitude;
         sample_count[id][bin]++;
       }
     }
   for (achan_idx = 0; achan_idx < sf.achan; achan_idx++) {
     int analogId = sfids[sf.dchan + achan_idx];
     if (tdat[analogId])
       for (bin = 0; bin < fftlen; bin++)
         if (sample_count[analogId][bin])
           tdat[analogId][bin] /= sample_count[analogId][bin];
   }

   for (achan_idx = 0; achan_idx < sf.achan && sf.dchan + achan_idx < l + channelcount; achan_idx++)
     free (sample_count[sfids[sf.dchan+achan_idx]]);
   free (sample_count);

   da.tdat = tdat;
   da.spiketimes = spiketimes;
   da.spikecount = count;
   da.min = min;
   da.max = max;
   da.fftstart = fftstart;
   da.fftlen = fftlen;
   da.binticks = binticks;
   return da;
}

static float *
get_binned_rate (int *spiketime, int spikecnt, int fftstart, unsigned fftend, unsigned binticks)
{
  struct {double rate; int next;} change[2];
  double rate;
  int next_change;
  int change_idx = 0, change_count = 0;
  unsigned fftlen = (fftend - fftstart) / binticks;
  float *binned_rate = calloc (fftlen, sizeof *binned_rate);

  //    printf ("%s line %d fftstart %d, fftend %d, spike 0 %d, spike1 %d\n", __FILE__, __LINE__, fftstart, fftend, spiketime[0], spiketime[1]);
  if (spikecnt == 1) {
    rate = 0;
    next_change = spiketime[0];
    change[0].rate = 1;
    change[0].next = spiketime[0] + 1;
    change[1].rate = 0;
    change[1].next = fftend;
    change_count = 2;
  }
  else if (spiketime[0] - fftstart < spiketime[1] - spiketime[0]) {
    rate = 1.0 / (spiketime[1] - spiketime[0]);
    next_change = spiketime[1];
  }
  else {
    rate = 0;
    next_change = spiketime[0] - (spiketime[1] - spiketime[0]);
    //    printf ("%s line %d rate %g, next_change %d\n", __FILE__, __LINE__, rate, next_change);
    change[0].rate = 1.0 / (spiketime[1] - spiketime[0]);
    change[0].next = spiketime[1];
    change_count = 1;
  }

  int spike = 0, tick;
  for (tick = fftstart; tick < (int)fftend; tick++) {
    if (tick == next_change) {
      if (change_idx < change_count) {
        rate = change[change_idx].rate;
        //        printf ("%s line %d: rate changed at %d to %g.  Next change %d\n", __FILE__, __LINE__, tick, rate,  change[change_idx].next);
        if (rate < 0 || !isfinite (rate)) {
            fprintf (stderr, "%s line %d: fatal error: rate = %g\n", __FILE__, __LINE__, rate);
            exit(1);
          }
        next_change = change[change_idx++].next;
      }
#     define T(n) spiketime[spike - 1 + n]
      else if (spike > 0 && spike + 2 < spikecnt
               && 10 * (T(1) - T(0)) < T(2) - T(1) && 10 * (T(3) - T(2)) < T(2) - T(1)) {
        static double dy = .5 - 1e-6;

        int dt1 = floor ((T(1) - T(0)) * dy + .5);
        int dt2 = floor ((T(3) - T(2)) * dy + .5);
        if (dt1 == 0) dt1 = 1;
        if (dt2 == 0) dt2 = 1;
        next_change = T(1) + dt1;
        rate = dy / dt1;
        change[0].rate = (1 - 2 * dy) / (T(2) - T(1) - dt1 - dt2);
        change[0].next = T(2) - dt2;
        change[1].rate = dy / dt2;
        change[1].next = T(2);


//        rate = 1.0 / (T(1) - T(0));
//        printf ("%s line %d: rate changed at %d to %g\n", __FILE__, __LINE__, tick, rate);

        if (rate < 0 || !isfinite(rate)) {fprintf (stderr, "%s line %d: fatal error: rate = %g\n", __FILE__, __LINE__, rate); exit(1);}
        //        printf ("%s line %d rate %g\n", __FILE__, __LINE__, rate);
//        next_change = floor (T(1) + (T(1) - T(0)) * dy + .5);
//        change[0].rate = (1 - 2 * dy) / (T(2) - T(1) - (T(3) - T(2) + T(1) - T(0)) * dy);
//        change[0].next = floor (T(2) - (T(3) - T(2)) * dy + .5);
//        change[1].rate = 1.0 / (T(3) - T(2));
//        change[1].next = T(2);
//        printf ("%s line %d: rate changed at %d to %g.  Next: %d %d\n", __FILE__, __LINE__, tick, rate, change[0].next, change[1].next);
//        printf ("%s line %d: T(2): %d, T(3): %d, dy: %g\n", __FILE__, __LINE__, T(2), T(3), dy);
        change_idx = 0;
        change_count = 2;
      }
      else if (spike + 1 < spikecnt) {
        rate = 1.0 / (T(2) - T(1));
        //        printf ("%s line %d: rate changed at %d to %g\n", __FILE__, __LINE__, tick, rate);
        if (rate < 0 || !isfinite(rate)) {fprintf (stderr, "%s line %d: fatal error: rate = %g\n", __FILE__, __LINE__, rate); exit(1);}
        next_change = T(2);
      }
      else {                    /* last spike */
        //        printf ("%s line %d: last spike\n", __FILE__, __LINE__);
        if (T(1) - T(0) < fftend - T(1)) {
          next_change = T(1) + T(1) - T(0);
          change[0].rate = 0;
          change[0].next = 0;
          change_count = 1;
          change_idx = 0;
        }
      }
    }
    int bin = (tick - fftstart) / binticks;
    //    printf ("%s line %d: bin %d, rate %g, tick %d\n", __FILE__, __LINE__, bin, rate, tick);
    binned_rate[bin] += rate;
    if (binned_rate[bin] < 0) {fprintf (stderr, "%s line %d: fatal error: rate = %g\n", __FILE__, __LINE__, rate); exit(1);}
    if (tick == spiketime[spike])
      spike++;
  }
  return binned_rate;
}

static void
fft (float *data, int fftlen, int direction)
{
#ifdef HAVE_LIBFFTW3F
  fftwf_plan plan = fftwf_plan_r2r_1d (fftlen, data, data, direction, FFTW_ESTIMATE);
  fftwf_execute (plan);
  fftwf_destroy_plan (plan);
#else
  int n;
  double *ddat = malloc (fftlen * sizeof *ddat);
  for (n = 0; n < fftlen; n++) ddat[n] = data[n];
  fftw_plan plan = fftw_plan_r2r_1d (fftlen, ddat, ddat, direction, FFTW_ESTIMATE);
  fftw_execute (plan);
  fftw_destroy_plan (plan);
  for (n = 0; n < fftlen; n++) data[n] = ddat[n];
  free (ddat);
#endif
}

static void
bandstop (float *tdat, unsigned fftlen, double freq, BandpassParams *bp)
{
  int n;

  fft (tdat, fftlen, FFTW_R2HC);

  for (n = 1; n < fftlen / 2.0; n++)
    if (n * freq * 60 >= bp->min_bpm && n * freq * 60 <= bp->max_bpm)
      tdat[n] = tdat[fftlen - n] = 0;

  fft (tdat, fftlen, FFTW_HC2R);
  for (n = 0; n < fftlen; n++) {
    tdat[n] /= fftlen;
    if (tdat[n] < 0)
      tdat[n] = 0;
  }
}

typedef struct
{
   int count;
   int index;
   int *val;
} IntArray;

typedef struct
{
   int count;
   int alloc;
   IntArray *array;
} IntArrayStack;

static IntArrayStack control_spiketrains;

static void
push_array (IntArrayStack *stack, int *val, int count)
{
   if (stack->count >= stack->alloc)
     stack->array = realloc (stack->array, ++stack->alloc * sizeof *stack->array);
   stack->array[stack->count].count = count;
   stack->array[stack->count].val = val;
   stack->array[stack->count].index = 0;
   stack->count++;
}

static void
clear_stack (IntArrayStack *stack)
{
   int n;
   for (n = 1; n < stack->count; n++)
      free (stack->array[n].val);
   stack->count = 0;
}

static float *
get_noise (int target_code, double g, DataArrays *da, BandpassParams *bp)
{
  int *spiketimes = da->spiketimes[target_code];
  int spikecount = da->spikecount[target_code];
  int fftstart = da->fftstart;
  int binticks = da->binticks;
  int fftlen = da->fftlen;
  unsigned fftend = fftstart + fftlen * binticks;

  if (0)
  {
    int n;
    FILE *x = fopen ("x", "w");
    FILE *y = fopen ("y", "w");
    printf ("%s line %d: FIXME\n", __FILE__, __LINE__);
    for (n = 0; n < spikecount; n++) {
      fprintf (x, "%d\n",  spiketimes[n]);
      fprintf (y, "%d\n",  n);
    }
    fclose (x);
    fclose (y);
  }

  //  printf ("%s line %d: spikecount %d\n", __FILE__, __LINE__, spikecount);
  float *binned_rate = get_binned_rate (spiketimes, spikecount, fftstart, fftend, binticks);
  double freq = 1.0 / (fftlen * binticks / ticks_per_second);

  if (0) {
    int spikesum = 0;
    int n;
    FILE *f = fopen ("br1", "w");
    printf ("%s line %d: FIXME\n", __FILE__, __LINE__);
    for (n = 0; n < fftlen; n++) {
      spikesum += binned_rate[n];
      fprintf (f, "%g\n",  binned_rate[n]);
      if (binned_rate[n] < 0) {
        printf ("%s line %d: binned_rate[%d] == %g\n", __FILE__, __LINE__, n, binned_rate[n]);
        exit (0);
      }
        
    }
    fclose (f);
    printf ("%s line %d: spikesum before %d\n", __FILE__, __LINE__, spikesum);
  }
    
  bandstop (binned_rate, fftlen, freq, bp);

  if (0)
  {
    int n;
    FILE *f = fopen ("br2", "w");
    printf ("%s line %d: FIXME\n", __FILE__, __LINE__);
    for (n = 0; n < fftlen; n++)
      fprintf (f, "%g\n",  binned_rate[n]);
    fclose (f);
  }

  double spikesum = 0;
  int n;
  for (n = 0; n < fftlen; n++)
    spikesum += binned_rate[n];
  double ratio = spikecount / spikesum;

  if (0) {
    printf ("%s line %d: FIXME\n", __FILE__, __LINE__);
    FILE *x = fopen ("x1", "w");
    FILE *y = fopen ("y1", "w");
    double sum = 0;
    for (n = 0; n < fftlen; n++) {
      fprintf (x, "%d\n", n * binticks);
      fprintf (y, "%g\n", sum);
      sum += binned_rate[n] * ratio;
    }
    fclose (x);
    fclose (y);
  }

  init_control ();
  double sum = 0;
  for (n = 0; n < fftlen; n++) {
    insert (sum, n * binticks);
    sum += binned_rate[n] * ratio;
  }
  free (binned_rate);

  insert (sum, fftlen * binticks);
  int control_spikecount = spikecount;
  //  printf ("%s line %d spikesum %g, ratio %g, sum %g\n", __FILE__, __LINE__, spikesum, ratio, sum);

  //  printf ("%s line %d: FIXME: using seed\n", __FILE__, __LINE__);
  int *control = gen_control_from_rate (sum, g, &control_spikecount, 0, false);
  //  int *control = gen_control_from_rate (0, sum, g, &control_spikecount, 1161873511, true);
  push_array (&control_spiketrains, control, control_spikecount);

  float *noise = calloc (fftlen, sizeof *noise);
  for (n = 0; n < control_spikecount; n++)
    if (control[n] / binticks < fftlen)
      noise[control[n] / binticks]++;
  
  if (0) {
    printf ("%s line %d: FIXME\n", __FILE__, __LINE__);
    FILE *x = fopen ("x2", "w");
    FILE *y = fopen ("y2", "w");
    sum = 0;
    for (n = 0; n < fftlen; n++) {
      insert (sum, n * binticks);
      fprintf (x, "%d\n", n * binticks);
      fprintf (y, "%g\n", sum);
      sum += noise[n];
    }
    fclose (x);
    fclose (y);
  }

  return noise;
}

typedef struct
{
  int count;
  int alloc;
  double *val;
} List;

static inline void
append (List *list, double val)
{
  if (list->count == list->alloc) {
    list->alloc = list->alloc * 2 + (list->alloc == 0);
    list->val = realloc (list->val, list->alloc * sizeof *list->val);
  }
  list->val[list->count++] = val;
}


static void
get_peaks (float *tdat, int len, List *peak_list)
{
  int n;
  int last_cross = 0;
  double max_amp = 0;

  if (0) {
    printf ("len: %d\n", len);
    for (n = 0; n < len; n++)
      printf ("%s line %d: %g\n", __FILE__, __LINE__, tdat[n]);
  }
  for (n = 1; n < len; n++) {
    if (tdat[n] < 0 && tdat[n - 1] >= 0) {
      if (last_cross)
        append (peak_list, max_amp);
      max_amp = fabs (tdat[n]);
      last_cross = n;
    }
    else if (fabs (tdat[n]) > max_amp)
      max_amp = fabs (tdat[n]);
  }
}

static double
get_mean_log (List *data)
{
  int n;
  double sum = 0;

  for (n = 0; n < data->count; n++)
    sum += log (data->val[n]);
  return sum / data->count;
}

static double
get_sd_log (List *data, double mean_log)
{
  int n;
  double sumsq = 0;

  for (n = 0; n < data->count; n++)
    sumsq += pow (log (data->val[n]) - mean_log, 2);
  return sqrt (sumsq / (data->count - 1));
}

static int
compare_doubles (const void *a, const void *b)
{
  const double *da = (const double *) a;
  const double *db = (const double *) b;

  return (*da > *db) - (*da < *db);
}

static double
get_quantile (List *data, double p)
{
  // Hyndman, R.J. and Fan, Y. (1996) "Sample quantiles in statistical
  // packages". The American Statistician, Vol. 50, No. 4. (Nov.,
  // 1996), pp. 361-365.
  // method number 8 (recommended)

  if (data->count == 0)
    return 0;
  qsort (data->val, data->count, sizeof (double), compare_doubles);
  double nppm = 1./3 + p * (data->count + 1./3);
  int j = floor (nppm);
  double delta = nppm - j;
  int i = j - 1;
  if (i < 0) i = j;
  if (j >= data->count) j = i;
  return (1 - delta) * data->val[i] + delta * data->val[j];
}

#define PSFILE "'scope_bandpass_histogram.ps'"
#define SHFILE  "scope_bandpass_histogram.sh"

char *matlab_strings[] = {"unset DISPLAY\nmatlab -nojvm", "", "print " PSFILE "\n",};

char *octave_strings[] = {
  "time octave",
  "__gnuplot_set__ term postscript\n__gnuplot_set__ output " PSFILE "\n",
  ""
};

static bool
write_hist_script (char *s[], List *data)
{
  FILE *hfile = fopen (SHFILE, "w");
  if (hfile == NULL)
    return false;
  fprintf (hfile, "%s &> /dev/null << EOF\n%sdata = [\n", s[0], s[1]);
  int n;
  for (n = 0; n < data->count; n++)
    fprintf (hfile, "%g\n", data->val[n]);
  fprintf (hfile, "];\n");
  fprintf (hfile, "hist (data)\n%sexit\nEOF\n", s[2]);
  fclose (hfile);
  return true;
}

static void
histogram (List *data)
{
  int status = 1;

  if (getenv ("USE_MATLAB")) {
    if (write_hist_script (matlab_strings, data) == false)
      return;
    unlink (PSFILE);
    status = system ("sh ./" SHFILE);
  }
  if (status != 0) {
    if (write_hist_script (octave_strings, data) == false)
      return;
    if (system ("sh ./" SHFILE));
  }
  if (system ("gv -scale=-5 " PSFILE "&"));
  /*
    Display *dpy = XtDisplay(app_shell);
    Window win = XtWindow(app_shell);
    Window client = XmuClientWindow(dpy,win);
    XRaiseWindow (dpy, client);
  */
}
        
static inline double
get_gamma (int *spiketimes, int spikecount)
{
  int *lodif = 0;
  double g;

  lodif = unpin (spiketimes, spikecount);
  g = gammafit_search (spiketimes, lodif, spikecount);
  free (lodif);
  return g;
}

static void
filter (float *tdat, unsigned fftlen, double freq, BandpassParams *bp)
{
  int n;
  fft (tdat, fftlen, FFTW_R2HC);
  tdat[0] = 0;
  for (n = 1; n < fftlen / 2.0; n++)
    if (n * freq * 60 < bp->min_bpm || n * freq * 60 > bp->max_bpm)
      tdat[n] = tdat[fftlen - n] = 0;
  if (n * 2 == fftlen)  
    tdat[fftlen / 2] = 0;
  fft (tdat, fftlen, FFTW_HC2R);
  for (n = 0; n < fftlen; n++)
    tdat[n] /= fftlen;
}

static void
write_control_edt (int target_code, int fftstart)
{
   bool done = false;
   IntArrayStack *s = &control_spiketrains;
   char *filename;
   char *typ = scaleFactor == 1 ? "bdt" : "edt";
   if (asprintf (&filename, "control_%d.%s", target_code, typ) == -1) exit (1);
   FILE *edtfile = NULL;
   edtfile = fopen (filename, "w");
   free (filename);
   if (edtfile) {
     char *hdr = scaleFactor == 1 ? "   11 1111111" : "   33   3333333";
     char *fmt = scaleFactor == 1 ? "%5d%8d\n" : "%5d%10d\n";
     fprintf (edtfile, "%s\n", hdr);
     fprintf (edtfile, "%s\n", hdr);
     while (!done) {
       done = true;
       int n;
       int min = INT_MAX, min_n = 0;
       for (n = 0; n < s->count; n++) {
         int i = s->array[n].index;
         if (i < s->array[n].count) {
           done = false;
           int val = s->array[n].val[i] + (n ? fftstart : 0);
           if (val < min) {
             min = val;
             min_n = n;
           }
         }
       }
       if (!done)
         fprintf (edtfile, fmt, 100 + min_n,
                  (min_n ? fftstart : 0) + s->array[min_n].val[s->array[min_n].index++]);
     }
     fclose (edtfile);
   }
}

static double
get_threshold (int target_code, DataArrays *da, BandpassParams *bp)
{
  int n;
  float *noise;
  List peak;

  memset (&peak, 0, sizeof peak);

  if ((!bp->threshold && !bp->log) || da->spikecount[target_code] == 0)
    return 0;

  double g = get_gamma (da->spiketimes[target_code], da->spikecount[target_code]);

  double freq = 1.0 / (da->fftlen * da->binticks / ticks_per_second);

  push_array (&control_spiketrains, da->spiketimes[target_code], da->spikecount[target_code]);
  for (n = 0; n < bp->surrogate_count; n++) {
    noise = get_noise (target_code, g, da, bp);
    if (0)
    {
      int i;
      FILE *f = fopen ("n1", "w");
      for (i = 0; i < da->fftlen; i++)
        fprintf (f, "%g\n", noise[i]);
      fclose (f);
    }
    filter (noise, da->fftlen, freq, bp);
    get_peaks (noise, da->fftlen, &peak);
    free (noise);
  }
  if (bp->save_on)
    write_control_edt (target_code, da->fftstart);
  clear_stack (&control_spiketrains);
  if (target_code == sfids[l] && bp->hist_on)
    histogram (&peak);
  double mean_log = get_mean_log (&peak);
  if (bp->gaussian) {
    double stdev_log = get_sd_log (&peak, mean_log);
    free (peak.val);
    if (bp->p01) return exp (mean_log - 2.3263 * stdev_log);
    if (bp->p05) return exp (mean_log - 1.6449 * stdev_log);
    if (bp->p50) return exp (mean_log);
    if (bp->p95) return exp (mean_log + 1.6449 * stdev_log);
    if (bp->p99) return exp (mean_log + 2.3263 * stdev_log);
  }
  else if (bp->empirical) {
    double q = 0;
    if (bp->p01) q = get_quantile (&peak, .01);
    if (bp->p05) q = get_quantile (&peak, .05);
    if (bp->p50) q = get_quantile (&peak, .50);
    if (bp->p95) q = get_quantile (&peak, .95);
    if (bp->p99) q = get_quantile (&peak, .99);
    free (peak.val);
    if (peak.count == 0)
      printf ("cell %d results invalid: don't have a complete noise cycle\n", target_code);
    return q;
  }
  return 0;
}

static void
filter_data (DataArrays *da, BandpassParams *bp)
{
  int chan;
  float *tdat;

  double freq = 1. / (da->fftlen * da->binticks / ticks_per_second);

  for (chan = 0; chan < MAX_CHNL; chan++)
    if ((tdat = da->tdat[chan]))
      filter (tdat, da->fftlen, freq, bp);
}

static void
get_min_max (DataArrays *da, BandpassParams *bp)
{
  int chan, n;
  float *tdat;

  da->global_min = FLT_MAX, da->global_max = -FLT_MAX;
  for (chan = 0; chan < MAX_CHNL; chan++)
    if ((tdat = da->tdat[chan])) {
      float *tdatp = tdat + da->screen_offset_bin;
      float tdat_min, tdat_max;
      tdat_min = tdat_max = tdatp[0];
      for (n = 1; n <= da->bins_on_screen; n++) {
        if (tdatp[n] < tdat_min)
          tdat_min = tdatp[n];
        if (tdatp[n] > tdat_max)
          tdat_max = tdatp[n];
      }
      if (tdat_min < da->global_min) da->global_min = tdat_min;
      if (tdat_max > da->global_max) da->global_max = tdat_max;
      da->min[chan] = tdat_min; 
      da->max[chan] = tdat_max;
   }
}

static void
envelope (float *tdat, int len, double thr, BandpassParams *bp)
{
  int n;
  int last_cross = 1;
  double max_amp = 0;
  
  if (bp->normal)
    return;
  
  tdat[0] = 0;
  for (n = 1; n < len; n++) {
    if ((tdat[n] < 0 && tdat[n - 1] >= 0) || n == len - 1) {
      double val = 0;
      int i;
      if (last_cross) {
        //        printf ("%s line %d: %g\n", __FILE__, __LINE__, max_amp);
        if (bp->threshold)
          val = max_amp > thr;
        else if (bp->log)
          val = thr ? MAX (log (max_amp / thr), 0) : 0;
        else if (bp->envelope)
          val = max_amp;
      }
      max_amp = fabs (tdat[n]);
      for (i = last_cross; i < n; i++)
        tdat[i] = val;
      if (n == len - 1)
        tdat[n] = val;
      if (bp->log || bp->envelope)
        tdat[last_cross] = 0;
      last_cross = n;
    }
    else if (fabs (tdat[n]) > max_amp)
      max_amp = fabs (tdat[n]);
  }
}

static inline int
reduce (int n)
{
  while (n % 2 == 0) n /= 2;
  while (n % 3 == 0) n /= 3;
  while (n % 5 == 0) n /= 5;
  while (n % 7 == 0) n /= 7;
  return n;
}

static inline int
good_length (int length)
{
  if (length <= 0)
    return length;
  while (reduce (length) > 1)
    length++;
  return length;
}

double
bandpass (BandpassParams *bp)
{
  double oversample_factor = 2;
  double nyquist_freq = bp->max_bpm / 60.0 * 2;
  double sample_freq = nyquist_freq * oversample_factor;
  double sample_period = 1 / sample_freq;
  unsigned binticks = sample_period * ticks_per_second;
  binticks = 1;
  unsigned fftlen = good_length ((screenWidth * 1.1) / binticks + 1);
  unsigned bins_on_screen = ceil (screenWidth / binticks);
  int fftstart = (int)leftTime - (fftlen - bins_on_screen) / 2 * binticks;
  unsigned fftend = fftstart + fftlen * binticks;
  int tdatp_len, chan_idx;

  int n;
  for (n = 0; n < sf.dchan; n++)
    sfids[n] = sf.ids[n];
  for (n = 0; n < sf.achan; n++)
    sfids[sf.dchan + n] = MAX_DIG_CHNL + sf.ids[MAX_DIG_CHNL + n];
  

  if (0)
  {
    printf ("%s line %d: FIXME\n", __FILE__, __LINE__);
    printf ("fftend %u, fftstart %d, fftlen %u, binticks %u\n", fftend, fftstart, fftlen, binticks);
    printf ("fftstart %d, leftTime %d, bins_on_screen: %u\n", fftstart, leftTime, bins_on_screen);
    printf ("screenWidth: %ld, sample_period %g, ticks_per_second %d\n", screenWidth, sample_period, ticks_per_second);
    printf ("sample_freq %g, nyquist_freq %g, oversample_factor %g\n", sample_freq, nyquist_freq, oversample_factor);
    printf ("bp->max_bpm %g\n", bp->max_bpm);
  }

  DataArrays da;
  if (!fp)
     return 0;
  da = get_tdat (fftstart, fftend, fftlen, binticks, bp); 
  da.screen_offset_bin = (leftTime - fftstart) / binticks;
  da.bins_on_screen = bins_on_screen;
  tdatp_len = fftlen -  da.screen_offset_bin;
  filter_data (&da, bp);
  for (chan_idx = l; chan_idx < sf.dchan + sf.achan && chan_idx < l + channelcount; chan_idx++) {
    int target_code = sfids[chan_idx];
    double thr = get_threshold (target_code, &da, bp);
    //    printf ("%s line %d: %d thr %g\n", __FILE__, __LINE__, target_code, thr);
    envelope (da.tdat[target_code], fftlen, thr, bp);
  }
  get_min_max (&da, bp);
  double tdat_min = da.global_min;
  double tdat_max = da.global_max;

  XFillRectangle (Current_display, WorkSpPixmap, WorkSpBgGC, 0, 0, scopeScreen_x, scopeScreen_y);

  char *filename = "threshold_counts.txt";
  FILE *f = NULL;
  int tcnt0[MAX_DIG_CHNL], tcnt1[MAX_DIG_CHNL];
  if (bp->threshold) {
    if ((f = fopen (filename, "w")) == NULL)
      //        error_at_line (0, errno, __FILE__, __LINE__, "Can't open %s for write", filename);
      fprintf (stderr, "Can't open %s for write: %s\n", filename, strerror (errno));
    memset (tcnt0, 0, sizeof tcnt0);
    memset (tcnt1, 0, sizeof tcnt1);
  }

  for (chan_idx = l; chan_idx < sf.dchan + sf.achan && chan_idx < l + channelcount; chan_idx++) {
    int target_code = sfids[chan_idx];
    
    float *tdatp = da.tdat[target_code] + da.screen_offset_bin;
    int lastx, lasty, miny, maxy, last_cycle_x, max_int_val, ymag, ylo = 0, yhi = 0;

    if (!bp->global) {
      tdat_min = da.min[target_code];
      tdat_max = da.max[target_code];
    }

    if (bp->log || bp->envelope)
      tdat_min = 0;
    last_cycle_x = lastx = lasty = miny = maxy = 0;
    //        printf ("code %d, min %g, max %g, tdatp[0] %g\n", target_code, tdat_min, tdat_max, tdatp[0]);
    int n;
    for (n = 0; n <= bins_on_screen; n++) {
      int x, y, draw_tic;
      x = (long long)n * binticks * (int)scopeScreen_x / screenWidth;

      if (f) {
        tcnt0[target_code] += tdatp[n] == 0;
        tcnt1[target_code] += tdatp[n] == 1;
      }

      draw_tic = 0;
      if ((bp->log || bp->envelope) && n < tdatp_len && tdatp[n] == 0) {
        if (n + 1 < tdatp_len)
          tdatp[n] = tdatp[n+1];
        if (bp->legs && x > last_cycle_x)
          draw_tic = 1;
        last_cycle_x = x;
      }
              
      ylo = signalBase[target_code];
      yhi = ylo - partition - 1;
      max_int_val = bp->palette ? 255 : partition * .7;

      ymag = (tdat_max == tdat_min
              ? (tdat_max > 0 ? max_int_val : 0)
              : ((double)tdatp[n] - tdat_min) / (tdat_max - tdat_min) * max_int_val);
      if (bp->palette) y = ymag;
      else             y = signalBase[target_code] - ymag;

      if (draw_tic)
        XDrawLine (Current_display, WorkSpPixmap, WorkSpGC, x, signalBase[target_code], x, y);
      if (n == 0)
        maxy = miny = y;
      if (x > lastx) {
        if (bp->palette) {
          int xt;
          int last_pxl_y = (maxy + miny) / 2;
          XSetForeground (Current_display, WorkSpGC, color[bp->palette - 1][last_pxl_y].pixel);
          XDrawLine (Current_display, WorkSpPixmap, WorkSpGC, lastx, ylo, lastx, yhi);
          for (xt = lastx + 1; xt < x; xt++) {
            int pxl_y = lasty + (double)(xt - lastx) / (x - lastx) * (y - lasty);
            XSetForeground (Current_display, WorkSpGC, color[bp->palette - 1][pxl_y].pixel);
            XDrawLine (Current_display, WorkSpPixmap, WorkSpGC, xt, ylo, xt, yhi);
          }
          XSetForeground (Current_display, WorkSpGC, black.pixel);
        }
        else {
          XPoint p[4];
          if (maxy > miny)
            XDrawLine (Current_display, WorkSpPixmap, WorkSpGC, lastx, miny, lastx, maxy);
          XDrawLine (Current_display, WorkSpPixmap, WorkSpGC, lastx, lasty, x, y);
          p[0].x = lastx; p[0].y = lasty;
          p[1].x = x;     p[1].y = y;
          p[2].x = x;     p[2].y = ylo + 1;
          p[3].x = lastx; p[3].y = ylo + 1;
          if (!bp->normal)
            XFillPolygon (Current_display, WorkSpPixmap, WorkSpGC, p, 4, Convex, CoordModeOrigin);
        }
        maxy = miny = y;
      }
      else {
        if (y > maxy)
          maxy = y;
        else if (y < miny)
          miny = y;
      }
      lastx = x;
      lasty = y;
    }
    update_cursor (0, 0, 3);
    XCopyArea (Current_display, WorkSpPixmap, WorkSpWind, WorkSpGC, 0,0, scopeScreen_x, scopeScreen_y, 0,0);                      
    XFlush(Current_display);
  }
  if (f) {
    fprintf (f, "%3s %6s %6s %s\n", " ID", " below", " above", "spike_filename");
    for (int i = 0; i < MAX_DIG_CHNL; i++)
      if (tcnt0[i] + tcnt1[i] > 0)
        fprintf (f, "%3d %6d %6d %s\n", i, tcnt0[i], tcnt1[i], spike_filename);
    printf ("threshold counts written to %s\n", filename);
    fclose (f);
  }
  free_da (&da);
  return tdat_max;
}

