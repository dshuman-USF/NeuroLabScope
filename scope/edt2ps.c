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

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>
#include <string.h>
#include "util.h"

#if !HAVE_GETLINE && HAVE_CONFIG_H
#include "getline.h"
#endif

#define MAX_DIG_CHNL 1000
#define MAX_ANLG_CHNL 99

enum Type {DIG, ANA};
  
typedef struct
{
  int time;
  int val;
  double dtime;
} Val;
  
typedef struct
{
  enum Type type;
  int first_analog;
  int id;
  unsigned alloc;
  unsigned count;
  Val *v;
} ValList;

static int mintime = INT_MIN;
static int maxtime = INT_MAX;
static ValList chn[MAX_DIG_CHNL + MAX_ANLG_CHNL];
static ValList *dig = chn;
static ValList *ana = chn + MAX_DIG_CHNL;
static ValList **vis;
static int legal, global, ticks_per_bin = 1;
static int ticks_per_second = 10000;
static int sps, ccnt;

static int
max_val (ValList *v)
{
  int n;
  int max = INT_MIN;
  for (n = 0; n < v->count; n++)
    if (v->v[n].val > max)
      max = v->v[n].val;
  return max;
}

static int
min_val (ValList *v)
{
  int n;
  int min = INT_MAX;
  for (n = 0; n < v->count; n++)
    if (v->v[n].val < min)
      min = v->v[n].val;
  return min;
}

static int
global_max_dig_val (void)
{
  int channel;
  int max = 0;
  int v;

  for (channel = 0; channel < MAX_DIG_CHNL; channel++)
    if ((v = max_val (&dig[channel])) > max)
      max = v;
  return max;
}

static void
lump (int ticks_per_bin)
{
  int vn;

  for (vn = 0; vn < ccnt; vn++) {
    ValList *v = vis[vn];
    int n, i = 0;
    v->v[i].dtime = v->v[i].time / ticks_per_bin;
    v->v[i].time /= ticks_per_bin;
    for (n = 1; n < v->count; n++) {
      int     tlump =          v->v[n].time / ticks_per_bin;
      double dtlump = (double) v->v[n].time / ticks_per_bin;
      if (v->type == DIG && tlump == v->v[i].time)
        v->v[i].val++;
      else {
        i++;
        v->v[i]. time =  tlump,
        v->v[i].dtime = dtlump;
      }
    }
    v->count = i + 1;
  }
  mintime /= ticks_per_bin;
  maxtime /= ticks_per_bin;
}

static inline int
tally (int chan)
{
  int n, sum = 0;
  for (n = 0; n < dig[chan].count; n++)
    sum += dig[chan].v[n].val;
  return sum;
}

static void
gen_ps (void)
{
  int n;
  double left_to_trace = .5;
  double left_to_num = .25;
  double right_to_trace = .6;
  double right_to_num = .25;
  double xscale = (8.5 - left_to_trace - right_to_trace) * 72.0 / (maxtime - mintime);
  int x0 = floor (left_to_trace * 72 / xscale + .5);
  double xnum = left_to_num * 72 / xscale;
  double xtally = (8.5 - right_to_num) * 72 / xscale;
  double dy, y0, max = 0;
  double seconds_per_bin = (double) ticks_per_bin / ticks_per_second;
  
  dy = (legal ? 13 : 10) * 72.0 / (ccnt + 1);        /* paper height */
  
  printf ("%%!\n/draw {add dup y moveto 0 3 -1 roll rlineto} bind def\n");
  printf ("/Helvetica findfont [%g 0 0 %g 0 0] makefont setfont\n", 6 / xscale, dy);
  printf ("/h %g def\n", dy * .7);
  printf ("%g 1 scale\n", xscale);
  printf ("0 setlinewidth\n");
  y0 = .5 * 72;
  printf ("%g %g moveto (%.3f s) show\n", xnum, y0, (double) mintime * ticks_per_bin / ticks_per_second);
  printf ("%g %g moveto (%.3f s) show\n", (xnum + xtally) / 2, y0, (double) (maxtime - mintime) * ticks_per_bin / ticks_per_second);
  printf ("%g %g moveto (%.3f s) dup stringwidth pop neg 0 rmoveto show\n",
          xtally, y0, (double) maxtime * ticks_per_bin / ticks_per_second);
  y0 += dy;
  
  for (n = 0; n < ccnt; n++) {
    double y = y0 + (ccnt - 1 - n) * dy;
    printf ("%g %g moveto (%d) show\n", xnum, y, vis[n]->id);
    if (vis[n]->type == DIG)
      printf ("%d %g moveto %d %g lineto stroke\n", x0, y, x0 + maxtime - mintime, y);
  }
  if (global)
    max = global_max_dig_val ();
  printf ("1 setlinewidth\n");
  for (n = 0; n < ccnt; n++) {
    ValList *v = vis[n];
    int segstart, min;
    if (v->first_analog) printf ("%%!\n/draw {add dup y 4 -1 roll add lineto} bind def 0 setlinewidth\n");
    max = (!global || v->type == ANA) ? max_val (v) : max;
    min = (           v->type == ANA) ? min_val (v) :   0;

    printf ("/y %g def\n", y0 + (ccnt - 1 - n) * dy);

    if (v->type == DIG)
      printf ("%g y moveto (%.*f) dup stringwidth pop neg 0 rmoveto show\n",
              xtally, sps ? 1 : 0, sps ? max / seconds_per_bin : tally (v->id));

    for (segstart = 0; segstart < v->count; segstart += 500) {
      int segend = MIN (segstart + 500, v->count);
      int i;
      
#     define Y ((v->v[i].val - min) / (max - min) * .7 * dy)
#     define ANALUMP 0
      for (i = segend - 1; i >= segstart; i--)
        if (ANALUMP || v->type == DIG)
          printf ("%g %d\n", Y, v->v[i]. time - (i ? v->v[i - 1]. time : mintime) );
        else {
          //          printf ("dbg: %d %g\n",  v->v[i - 1]. time, v->v[i - 1].dtime);
          printf ("%g %g\n", Y, v->v[i].dtime - (i ? v->v[i - 1].dtime : mintime) );
        }
      

      if (i < 0) i = 0;
      if (ANALUMP || v->type == DIG) {
        printf ("%d %g y add moveto\n", x0 + v->v[i].time - mintime, Y);
        printf ("%d\n", x0 + (segstart ? (v->v[i].time - mintime) : 0));
      } else {
        printf ("%g %g y add moveto\n", x0 + v->v[i].dtime - mintime , Y);
        printf ("%g\n", x0 + (segstart ? v->v[i].dtime - mintime : 0));
      }
        
      printf ("%d {draw} repeat stroke\n", segend - segstart);
    }
  }
  printf ("showpage\n");
}

void
get_vis (void)
{
  int i, n, do_first = 1;
  for (n = 0; n < MAX_DIG_CHNL + MAX_ANLG_CHNL; n++)
    if (chn[n].count)
      ccnt++;
  TMALLOC (vis, ccnt);
  for (i = n = 0; n < MAX_DIG_CHNL + MAX_ANLG_CHNL; n++)
    if (chn[n].count) {
      if (chn[n].type == ANA && do_first) {
        chn[n].first_analog = 1;
        do_first = 0;
      }
      else chn[n].first_analog = 0;
      vis[i++] = &chn[n];
    }
}

int
main (int argc, char **argv)
{
  static char *line;
  static size_t len;
  int time = 0, argn;

  for (argn = 1; argn + 1 < argc; argn += 2)
    if      (strcmp ("--start", argv[argn]) == 0)
      mintime = atoi (argv[argn + 1]);
    else if (strcmp ("--stop", argv[argn]) == 0)  
      maxtime = atoi (argv[argn + 1]);
    else if (strcmp ("--paper", argv[argn]) == 0)  
      legal = strcmp ("legal", argv[argn + 1]) == 0;
    else if (strcmp ("--norm", argv[argn]) == 0)  
      global = strcmp ("global", argv[argn + 1]) == 0;
    else if (strcmp ("--ticks_per_bin", argv[argn]) == 0 || strcmp ("-tpb", argv[argn]) == 0)
      ticks_per_bin = atoi (argv[argn + 1]);
    else if (strcmp ("--ticks_per_second", argv[argn]) == 0 || strcmp ("-tps", argv[argn]) == 0)  
      ticks_per_second = atoi (argv[argn + 1]);
    else if (strcmp ("--show_max_spks_per_sec", argv[argn]) == 0 || strcmp ("-sms", argv[argn]) == 0)  
      sps = strcmp ("yes", argv[argn + 1]) == 0;

  if (getline (&line, &len, stdin));
  if (getline (&line, &len, stdin));
  while (getline (&line, &len, stdin) > 0 && len > 5) {
    time = atoi (line + 5);
    int code;
    
    if (time > maxtime)
      break;
    if (time < mintime)
      continue;

    if (mintime == INT_MIN)
      mintime = time;

    line[5] = 0;
    code = atoi (line);
    code >= 0 || DIE;
    if (code < 1000) {
      ValList *d = &dig[code];
      if (d->count == d->alloc)
        TREALLOC (d->v, d->alloc += (1<<15));
      d->v[d->count].time = time;
      d->v[d->count].val = 1;
      d->id = code;
      d->type = DIG;
      d->count++;
    }
    else {
      int id = code / 4096;
      int val;
      ValList *a;
      (id >= 0 && id < MAX_ANLG_CHNL) || DIE;
      a = &ana[id];
      if((code & 0x800) == 0)
        val = code & 0x7FF;
      else
        val = code | 0xFFFFF000U;
      if (a->count == a->alloc)
        TREALLOC (a->v, a->alloc += (1<<21));
      a->v[a->count].time = time;
      a->v[a->count].val = val;
      a->type = ANA;
      a->id = id;
      a->count++;
    }
  }
  get_vis ();
  if (maxtime == INT_MAX)
    maxtime = time;
  lump (ticks_per_bin);
  gen_ps ();
  return 0;
}
