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



#define _FILE_OFFSET_BITS 64
#define _LARGEFILE_SOURCE 1
#define _LARGEFILE64_SOURCE 1
#include "std.h"
#include <sys/time.h>
#include "filesup.h"
#include "options.h"
#include "blockList.h"
#include "dispUnit64.h"
#include "AddTree.h"
#include "FilterTree.h"
#include "color_table.h"
#include <fftw3.h>
#include <float.h>
#include <limits.h>
#include <stdbool.h>
#include "gammafit_search.h"
#include "unpin.h"
#include "gen_control.h"

#if HAVE_ASM_I486_MSR_H
#include <asm-i486/msr.h>
#else
#define rdtscll(x)
#endif /* HAVE_ASM_I486_MSR_H */

#define ahead 1
#define back (-1)
#define undefined 0

#define HALFMS_PER_SECOND 2000

static unsigned long swaptest = 1;

/*** forward Dec */
void createSignalLabel();
void ClearLabels();
int should_the_block_be_displayed();
void createHSLabel();
void ClearHSLabels();
void display_a_hdt_block();
void hssignals();
char *ltoa ();
void makeWarningPop();


/********       Global variables for scrolling and redraws      **********/
Widget          bullBoardW;            /*  BulletinBoard       */
int     first_offset_forhdt = 0;
int     analog_left_x [MAX_ANLG_CHNL];            /* leftmost analog draw  */
int     analog_left_y [MAX_ANLG_CHNL];            /* leftmost analog draw  */
int     analog_right_x[MAX_ANLG_CHNL];           /* rightmost analog draw */
int     analog_right_y[MAX_ANLG_CHNL];           /* rightmost analog draw */

long    screenWidth;                    /* in half mil time units */
long    screenLeft;                         /* file position ptr         */
long    screenRight;                     /* file position ptr    */
unsigned int scroll_width;            /* width of scroll shift */
int     scrollMargin;                    /* in pixels redrawn    */
int     strokeLength;                    /* digital (in pixels)  */
int     signalBase[MAX_DIG_CHNL + MAX_ANLG_CHNL];                /* y_offset for trains  */
Dimension  scopeScreen_x;                  /* scope x-dimension  */
Dimension  scopeScreen_y;                  /* scope y-dimension  */
int     anaMap[MAX_ANLG_CHNL];                      /* maps code --> min/max */
int     partition;                          /* y-space for each train */
int     leftTime,rightTime, firstTime;
int     fileTime;                           /* total time spanned        */
int     marginTime;                         /* time of scroll margin */
int     last_direction;               /* a scroll direction flag */
int     leftoff; 
int     totalchans;

static short num, num1,num2, num3, num4, num5, num6, num7;
static short   num8, num9, num10, num11, num12,num13,num14,num15;
short   peak1,peak2, peak3, peak4;
int     sum1,sum2,sum3, sum4,sum5,sum6,sum7,sum8, hsId;
int  yoffset2, lastx2[MAX_HDT_CHNL], lasty2[MAX_HDT_CHNL];
static short int HdtValue, SizeOfHdtValue;
long realpointeroff, rp2;
float rightoff, rightoff2,  divide, realWidth2, start,leftTime2, totalWidth, samplerate;
extern  int  hstotal;
int    togglestate[MAX_HDT_CHNL];
int   xoffset2;
int   scroll;
int  basehdt[MAX_HDT_CHNL], positionhdt;
int     i,j, yoffset, newwidth, temp;
unsigned int   lastx[MAX_ANLG_CHNL],lasty[MAX_ANLG_CHNL];
int     analogId, magnitude;
int     lastOffset[MAX_DIG_CHNL];
int     tempTally;
extern int bins;
float   realLeft;
static  float startoffset[MAX_BLOCKS], stopoffset[MAX_BLOCKS];
long long startbyte[MAX_BLOCKS+1];
int hdtblkstart[MAX_BLOCKS], hdtblkend[MAX_BLOCKS];
int     byteshift;
long    hdtendtime;
int     y2old[1000], hsmax[MAX_HDT_CHNL], hsmin[MAX_HDT_CHNL];   /*** was [100] */
int     state2;
int     Hist[1000]; /*** was 100 */
float   integDown[1000], integUp[1000];  /*** was [100] */
float   integFactor[1000], integMax[1000];         /*** was [100] */
float   realRight;
extern int hdt;
int hdt_block_count;
extern  Widget SignalLabel2W[MAX_HDT_CHNL];

FILE    *fp, *fp9;        /* direct access file ptr */

extern Window WorkSpWind;
extern Pixmap WorkSpPixmap;
extern GC WorkSpGC, InvertGC;
extern GC WorkSpBgGC;
extern Display *Current_display;
extern Widget horiz_bar;
extern int sliderpos;

long long bytes_in_the_block[MAX_BLOCKS]; /* for 200 hdt signals! */
int file_header_offset_in_bytes;
float screen_rightside_time, screen_leftside_time; 
/* these are in secs */

float begin_offset_in_block, end_offset_in_block;

Cardinal NextLabel, NextLabel2;

/*************************************************************************/
void InitScope()
{
   extern Widget DigitalChanW, AnalogChanW, HSDChanW, ScreenLeft2W;
   Arg    args[20];
   int    i, code, time = 0;
   float  realLeft4;
   char analogStr[30], hsdStr[30], digitalStr[30], textStr4[80]; 
   XmString HSDString, DigitalString, AnalogString, textString4;

   XFillRectangle (Current_display, WorkSpPixmap, WorkSpBgGC, 0, 0, scopeScreen_x, scopeScreen_y);
   /*   XClearWindow(Current_display,WorkSpWind); */

   scrollMargin = 8;    /* default scrolling margin */
   l = 0;

   /* set scope window  width and height        */

   scopeScreen_x = DAS_WIDTH;

   sprintf(digitalStr,"Digital Channel Total: %d",sf.dchan);
   DigitalString=XmStringCreateLtoR(digitalStr,XmSTRING_DEFAULT_CHARSET);

   i = 0;
   XtSetArg(args[i],XmNlabelString,DigitalString); i++;
   XtSetValues(DigitalChanW,args,i);

   sprintf(analogStr,"Analog Channel Total: %d",sf.achan);
   AnalogString=XmStringCreateLtoR(analogStr,XmSTRING_DEFAULT_CHARSET);

   i = 0;
   XtSetArg(args[i],XmNlabelString,AnalogString); i++;
   XtSetValues(AnalogChanW,args,i);

   sprintf(hsdStr,"HDT Channel Total: %d",hstotal);
   HSDString=XmStringCreateLtoR(hsdStr,XmSTRING_DEFAULT_CHARSET);

   i = 0;
   XtSetArg(args[i],XmNlabelString,HSDString); i++;
   XtSetValues(HSDChanW,args,i);

   scroll_width = scopeScreen_x - scrollMargin;

   partition = (int)scopeScreen_y / channelcount;
   strokeLength = partition / 3;

   /* Set the digital & analog signal bases     */
   for(i = 0; i < sf.dchan; i++) 
      signalBase[sf.ids[i]] = (i + 1) * partition - 1 - partition / 7;

   for(i = 0; i < sf.achan; i++)
      signalBase[MAX_DIG_CHNL + sf.ids[i + MAX_DIG_CHNL]] = (sf.dchan + i + 1) * partition - 1 - partition / 7;

   /* Create signal labels and tallies for each signal present */
   ClearLabels ();

   for(i = 0; i < sf.achan;i++)
   {
      createSignalLabel(1000 + sf.ids[i + MAX_DIG_CHNL], signalBase[1000 + sf.ids[i + MAX_DIG_CHNL]]);
   }

   /*   XtVaSetValues(bullBoardW, XmNnumChildren, NextLabel, NULL); */

   /* Set the analog code maps                  */
   for(i = 0; i < sf.achan; i++)
      anaMap[sf.ids[MAX_DIG_CHNL + i]] = i;

   /* Open the direct access file               */
   rewind (fp);

   /* set the left & right screen edge time variables   */
   screenLeft = 32;
   fseek(fp, screenLeft, 0);

   if((code = getw(fp)) != EOF && (time = getw(fp)) != EOF)
      leftoff = time; 

   leftTime = time; 
   realLeft4 = (double)leftTime / ticks_per_second;
   firstTime = leftTime;

   if(time < 0 ) {
      makeWarningPop ("PLEASE NOTE", "negative event time in file?");
      time = leftTime = realLeft4 = 0;
   }

   rightTime = leftTime + screenWidth;

   sprintf(textStr4,"%7.3f", realLeft4);
   textString4 = XmStringCreateLtoR(textStr4, XmSTRING_DEFAULT_CHARSET);

   i = 0;
   XtSetArg(args[i], XmNlabelString, textString4); i++;
   XtSetValues(ScreenLeft2W, args, i);

   marginTime = screenWidth * scrollMargin / (int)scopeScreen_x;

   fileTime = sf.lastTime - leftTime;

   totalchans = sf.achan + sf.dchan;

   fseek(fp, screenLeft, 0);
   fflush(fp);

   /*   Header information for hdt signals */
   if(hdt == 1)
   {
      long long byte, file_bytes;
      if ( (fp9 = fopen64(newname2,"rb")) == (FILE *) NULL)
	 return;
      fseeko64 (fp9, 0, SEEK_END);
      file_bytes = ftello64 (fp9);
      fseeko64(fp9, 0, 0);
      num = fgetc(fp9);
      num1 = fgetc(fp9);
      num2 = fgetc(fp9);
      num3 = fgetc(fp9);

      samplerate = sum1 =  256 * num +  num1; /* Sampling Rate      */
      sum2 =  256 * num2 + num3;              /* Number of Channels */

      for(j=0; j < hstotal; ++j)
      {
	 peak1 = fgetc(fp9);
	 peak2 = fgetc(fp9);
	 peak3 = fgetc(fp9);
	 peak4 = fgetc(fp9);

	 /* Minimum Peaks for each channel */    
	 hsmin[j] = 256 * peak1 + peak2;
	 if(hsmin[j] > 32767)
	    hsmin[j] = hsmin[j] - 65536;

	 /* Maximum Peaks for each channel */    
	 hsmax[j] = (256 * peak3 + peak4);
	 if(hsmax[j] > 32767)
	    hsmax[j] = hsmax[j] - 65536;
      }
      num4 = fgetc(fp9);
      num5 = fgetc(fp9);
      num6 = fgetc(fp9);
      num7 = fgetc(fp9);

      /* Number of Blocks */
      sum3 = (256 * num4 + num5) * 32768;
      sum4 = (256 * num6 + num7) + sum3;

      hdt_block_count = sum4;

      byte = file_header_offset_in_bytes = ftello(fp9) + hdt_block_count * 8;

      for(j=0; j < hdt_block_count; ++j)
      { 
	 num8 = fgetc(fp9);
	 num9 = fgetc(fp9);
	 num10 = fgetc(fp9);
	 num11 = fgetc(fp9);
	 num12 = fgetc(fp9);
	 num13 = fgetc(fp9);
	 num14 = fgetc(fp9);
	 num15 = fgetc(fp9);

	 sum5 = (256 * num8 + num9) * 32768;
	 sum6 = (256 * num10 + num11) + sum5;
	 if(sum6 < 0) sum6 += 256;
	 hdtblkstart[j] = sum6;

	 sum7 = (256 * num12 + num13) * 32768;
	 sum8 = (256 * num14 + num15) + sum7;
	 if(sum8 < 0) sum8 += 256;
	 hdtendtime = sum8;
	 hdtblkend[j] = sum8;

	 bytes_in_the_block[j] = (halfms2samp (sum8) - halfms2samp (sum6)) * BYTES_PER_SAMPLE * hstotal;
	 if (byte + bytes_in_the_block[j] > file_bytes) {
	    long long bytes_left = file_bytes - byte;
	    long long samp0 = halfms2samp (hdtblkstart[j]);
	    makeWarningPop ("BAD HDT FILE", "too few bytes in hdt file, using it anyway");
	    hdt_block_count = j + 1;
	    hdtblkend[j] = hdtblkstart[j];
	    while ((halfms2samp (hdtblkend[j] + 1) - samp0) * BYTES_PER_SAMPLE * hstotal <= bytes_left)
	       hdtblkend[j]++;
	    bytes_in_the_block[j] = (halfms2samp (hdtblkend[j] + 1) - samp0) * BYTES_PER_SAMPLE * hstotal;
	    if (bytes_in_the_block[j] == 0)
	       hdt_block_count--;
	    if (hdt_block_count == 0) {
	       makeWarningPop ("BAD HDT FILE", "way too few bytes in hdt file, not using it after all");
	       hdt = 0;
	    }
	 }
	 startbyte[j] = byte;
	 byte += bytes_in_the_block[j];

	 startoffset[j] = (double)hdtblkstart[j] / HALFMS_PER_SECOND;
	 stopoffset[j] = (double)hdtblkend[j] / HALFMS_PER_SECOND;
      }
      if (byte < file_bytes)
	 makeWarningPop ("BAD HDT FILE", "too many bytes in hdt file, using it anyway");
   } /* if hdt == 1 */

   temppos = 0;
   for(i=0; i < MAX_HDT_CHNL; ++i)
      togglestate[i] = 0;
}

static XSegment *segment;
static int segment_alloc;


static void
draw_newEvents (void)
{
  int time;

  if (newEvents == NULL)
     return;
  rewind (newEvents);
  while ((time = getw (newEvents)) != EOF) {
     if (time > leftTime && (time - leftTime) <= screenWidth) {
        unsigned long xoffset = (long long)(time - leftTime) * scopeScreen_x / screenWidth;
        XDrawLine (Current_display, WorkSpWind, WorkSpGC, xoffset, 0, xoffset, scopeScreen_y);
     }
  }
}

void refresh ()
{
   extern         Widget SignalLabelW[MAX_DIG_CHNL]; /*** was 100 */
   unsigned long int   xoffset,code, time;
   int            i,yoffset, inc, temp;
   int            lastx[MAX_ANLG_CHNL],lasty[MAX_ANLG_CHNL];/*** was 50 */
   int            analogId, magnitude;
   int            lastOffset[1000];                   /*** XYZ was 1000 changed 10-17-02 to 2000 pb */
   char           msg[50];
   static int maxpoints;
   static XPoint *points[MAX_ANLG_CHNL];
   static int point_count[MAX_ANLG_CHNL];
   int segment_count;
   static int spikestate[1000];

   if (maxpoints == 0)
      maxpoints = (XMaxRequestSize (Current_display) - 3) / 2;

   /* XVision dies if maxpoints >= 31750 , even though
      (XMaxRequestSize (Current_display) - 3) / 2 == 32766.
      It works if maxpoints == 31749 */
   maxpoints = MIN (30000, maxpoints);          /* workaround for XVision bug */

   partition = (int)scopeScreen_y / channelcount;
   strokeLength = partition / 3;

   /* Initialize the lastx & lasty arrays               */
   for(i = 0; i < 50; i++)
      lastx[i] = lasty[i] = 0;

   for(i = 0; i + l < sf.dchan; i++)
      signalBase[sf.ids[i + l]] = (i + 1) * partition - 1 - partition / 7;

   for(i = 0; i < sf.achan; ++i)
      signalBase[1000+sf.ids[i+MAX_DIG_CHNL]] = (sf.dchan+1+i) * partition - 1 - m - partition / 7;

   for(inc = 0; inc < (sf.dchan+sf.achan); ++inc)
      spikestate[inc] = SignalLabelW[inc] ? XmToggleButtonGetState(SignalLabelW[inc]) : 0;

   ClearLabels();

   destroySideBar ();
   for (i = 0; i < sf.dchan; i++)
   {
      Hist[i] = sf.ids[i];
      createSignalLabel(sf.ids[i],signalBase[sf.ids[i]] );
      if (i >= l && i < l + channelcount)
         createTallyStat (signalBase[sf.ids[i]]);
   }
   if (show_tally)
      showTally ();
   else
      ClearSideBar ();

   for (i = 0;i < sf.achan;i++)
      createSignalLabel(1000+sf.ids[i+MAX_DIG_CHNL], signalBase[1000+sf.ids[i+MAX_DIG_CHNL]]);

   /*   XtVaSetValues(bullBoardW, XmNnumChildren, NextLabel, NULL); */

   {
      char *msg = "Please wait for redraw";
      XDrawImageString (Current_display, WorkSpWind, WorkSpGC, 350, 380, msg, strlen (msg));
   }

   /* Zero the last offset array                */
   /* Draw the digital signal baselines         */
   for(i = l; i < sf.dchan; i++) /*** QQQ l was 0 */
   {
      lastOffset[sf.ids[i]] = -1;
      XDrawLine(Current_display, WorkSpPixmap,WorkSpGC, 0, signalBase[sf.ids[i]], 
		scopeScreen_x, signalBase[sf.ids[i]]);
   }

   /* Set the file position pointer     */
   fseek(fp,screenLeft,0);

   /* Read and draw signals until EOF or screen is full */
   code = getw(fp);
   time = getw(fp);
   memset (point_count, 0, sizeof (point_count));
   if (segment_alloc < 10000)
      (segment = realloc (segment, (segment_alloc = 10000) * sizeof (XSegment))) || DIE;
   segment_count = 0;
   while(code != CODEFLAG && time < rightTime)
   {
      /*** PPP scaleFactor */
      xoffset = (long long)(time - leftTime) * scopeScreen_x / screenWidth;

      if(code < 1000 && code >= sf.ids[l] && l < sf.dchan)   /* a digital signal        */
	 if(xoffset != lastOffset[code])
	 {
	    if (segment_count == segment_alloc)
	    {
	       XDrawSegments (Current_display, WorkSpPixmap, WorkSpGC, segment, segment_count);
	       segment_count = 0;
	    }
	    segment[segment_count].x1 = xoffset;
	    segment[segment_count].y1 = signalBase[code];
	    segment[segment_count].x2 = xoffset;
	    segment[segment_count].y2 = signalBase[code] - strokeLength;
	    segment_count++;
	    lastOffset[code] = xoffset;
	 }
	 else ;
      else
      {
	 analogId = code / 4096;
	 if(analogId > MAX_ANLG_CHNL || analogId < 0)
	 {
	    printf("the analog id is %d, not > 0 and < %d \n", analogId, MAX_ANLG_CHNL);
	    exit (1);
	 }

	 if((code & 0x800) == 0)
	    magnitude = code & 0x7FF;
	 else
	    magnitude = code | 0xFFFFF000U;

	 temp = sf.amin[anaMap[analogId]];
	 if(abs(sf.amax[anaMap[analogId]] - temp ) < 1)
	 {
	    if (!AnDiffWarn[analogId])
	    {
	       strcpy(msg, "Analog ID ");
	       strcat(msg, ltoa(analogId));
	       strcat(msg, " differance to small!");
	       makeWarningPop("Analog Error", /*** QQQ "Analog differance to small!" */msg);
	       AnDiffWarn[analogId] = 1;
	    }
	    yoffset = signalBase[1000+analogId];
	 }
	 else
	    yoffset = signalBase[1000+analogId] - (magnitude - temp)*partition * 2 / (3 * (sf.amax[anaMap[analogId]] - temp));

	 if (points[analogId] == 0)
	    (points[analogId] = malloc (maxpoints * sizeof (XPoint))) || DIE;

	 {
	    int n = point_count[analogId];

	    if (n >= maxpoints) {
	       XDrawLines (Current_display,WorkSpPixmap,WorkSpGC, points[analogId], point_count[analogId], CoordModeOrigin);
	       n = point_count[analogId] = 0;
	    }
	    if(n < maxpoints && lasty[analogId] != 0 && lastx[analogId] <= xoffset && xoffset != 0) {
	       if (n == 0) {
		  points[analogId][n].x = lastx[analogId];
		  points[analogId][n].y = lasty[analogId];
		  n++;
	       }
	       points[analogId][n].x = xoffset;
	       points[analogId][n].y = yoffset;
	       n++;
	       point_count[analogId] = n;
	    }
	 }

	 lastx[analogId] = xoffset;
	 lasty[analogId] = yoffset;
      }

      code = getw(fp);
      time = getw(fp);

   }

   {
      int n;
      for (n = 0; n < MAX_ANLG_CHNL; n++)
	 if (point_count[n] > 1)
	    XDrawLines (Current_display,WorkSpPixmap,WorkSpGC, points[n], point_count[n], CoordModeOrigin);
   }
   if (segment_count)
      XDrawSegments (Current_display,WorkSpPixmap,WorkSpGC, segment, segment_count);
   update_cursor (0, 0, 3);
   XCopyArea (Current_display, WorkSpPixmap, WorkSpWind, WorkSpGC, 0,0, scopeScreen_x, scopeScreen_y, 0,0);                     

   screenRight = ftell(fp) - 8;

   last_direction = undefined;
   fflush(fp);

   for(inc = 0; inc < sf.dchan+sf.achan; ++inc)
   {
      if(spikestate[inc] == 1)
	 XmToggleButtonSetState(SignalLabelW[inc],True,False);
   }
   draw_newEvents ();

   /*Call routine for displaying hdt signals if present*/
   if(hdt == 1)
      hssignals();
}


void hssignals()
{
   int k;
   partition = (int)scopeScreen_y / channelcount ;

   if (NextLabel2 > 0)
      for(i = 0; i < hstotal; ++i)
      {
	 state2 = XmToggleButtonGetState(SignalLabel2W[i]);
	 if(state2 !=0)
	    togglestate[i] = 1;
	 else
	    togglestate[i] = 0;
      }

   ClearHSLabels();
   for(hsId = 0; hsId < hstotal; ++hsId)
   {
      basehdt[hsId] = (sf.dchan + sf.achan + hsId + 1 - l) * partition - 1 - partition / 7;
      createHSLabel(hsId, basehdt[hsId]);
      if(togglestate[hsId] == 1)
	 XmToggleButtonSetState(SignalLabel2W[hsId], True, False);
   } 

   HdtValue = 0;
   SizeOfHdtValue = sizeof(HdtValue);
   for(k=0; k < hstotal; ++k)
   {
      lastx2[k] = 0;
      lasty2[k] = basehdt[k];
   }

   realWidth2 = (double)screenWidth / ticks_per_second;
   leftTime2 = (double)leftTime  / ticks_per_second;
   realRight = realWidth2 + leftTime2;

   realWidth2 = (double)screenWidth / ticks_per_second;
   divide = realWidth2 * samplerate / (float)DAS_WIDTH;
   rightoff = realWidth2 * samplerate;
   leftTime2 = (double)leftTime / ticks_per_second;

   scroll = 1;

   screen_leftside_time = (double)leftTime / ticks_per_second;
   screen_rightside_time = (double)rightTime / ticks_per_second;

   /* first writing simple case , then extend it */
   {

      for (k = 0; k < hdt_block_count; k++)
	  if (should_the_block_be_displayed(k)) 
	     display_a_hdt_block(k);
       XFlush(Current_display);
   }
}


int
should_the_block_be_displayed (int k)
{
   if (startoffset[k] <= screen_rightside_time && (stopoffset[k] >= screen_leftside_time))
   {
      begin_offset_in_block =  MAX (startoffset[k] , screen_leftside_time) ;
      end_offset_in_block   = MIN (stopoffset[k] , screen_rightside_time);
      return(TRUE);
   }
   else return (FALSE);
}


void display_a_hdt_block(block_num)
     int block_num;
     /****
	  xoffset and yoffset denote the starting point from where this block starts
	  showing up, the first two are related to what block and how much to be displayed 
     ****/
{
   long j;
   long temp, temp1, temp2, BlockOffset;
   double ptsPerInc;
   int segment_count;
   int lastseg[MAX_HDT_CHNL];

   {
      char *msg = "Please wait for redraw";
      XDrawImageString (Current_display, WorkSpWind, WorkSpGC, 350, 380, msg, strlen (msg));
   }

   temp = startbyte[block_num];

   j = floor ((begin_offset_in_block - startoffset[block_num]) * samplerate + .5) * hstotal * BYTES_PER_SAMPLE;
   temp += j;
   fseeko64(fp9, temp, SEEK_SET); /* file is set to point the correct byte of the first hs signal */

   temp2 = (bytes_in_the_block[block_num] - j) / ((double)hstotal * BYTES_PER_SAMPLE);

   temp1 = realWidth2 * samplerate ; /* this many points can be accomodated on the screen */

   if (temp1 <= temp2)
      temp2 = temp1; /* taking the smaller one to fill in */

   /*** set max value */
   temp1 = temp2;

   ptsPerInc = (realWidth2 / (float)scopeScreen_x) * samplerate;

   xoffset2 = MAX ((startoffset[block_num] - screen_leftside_time), 0) * scopeScreen_x / (float)realWidth2;
   BlockOffset = xoffset2;

   if(xoffset2 < 0)
   {
      printf("something wrong with offset x \n");
      xoffset2 = 0;
      return;
   }

   if (segment_alloc < 10000)
      (segment = realloc (segment, (segment_alloc = 10000) * sizeof (XSegment))) || DIE;
   segment_count = 0;
   memset (lastseg, 0xff, sizeof (lastseg));
   /* that many points are already skipped */
   while(temp2 > 0 && ! feof(fp9))
   {
      for(hsId = 0; hsId < hstotal; ++hsId)
      {
	 if (fread(&HdtValue, SizeOfHdtValue, 1, fp9) != 1) exit (1);
	 SWAB (HdtValue);

	 (HdtValue >= hsmin[hsId] && HdtValue <= hsmax[hsId]) || DIE;

         yoffset2 = basehdt[hsId] - (((double)HdtValue - hsmin[hsId]) * (partition - 1)) / (hsmax[hsId] - hsmin[hsId]);

         /*** used to blank between blocks */
         if (xoffset2 == BlockOffset)
            lastx2[hsId] = xoffset2;
         
         if (segment_count == segment_alloc) {
	    XDrawSegments (Current_display, WorkSpPixmap, WorkSpGC, segment, segment_count);
	    segment_count = 0;
	    memset (lastseg, 0xff, sizeof (lastseg));
         }
         {
	    int segn = lastseg[hsId];
	    if (segn >= 0 && segment[segn].x1 == xoffset2 && segment[segn].x2 == xoffset2 && lastx2[hsId] == xoffset2) {
	       segment[segn].y1 = MIN (segment[segn].y1, yoffset2);
	       segment[segn].y2 = MAX (segment[segn].y2, yoffset2);
	    }
	    else if (lastx2[hsId] == xoffset2) {
	       segment[segment_count].x1 = segment[segment_count].x2 = xoffset2;
	       segment[segment_count].y1 = MIN (lasty2[hsId], yoffset2);
	       segment[segment_count].y2 = MAX (lasty2[hsId], yoffset2);
	       lastseg[hsId] = segment_count++;
	    }
	    else {
	       segment[segment_count].x1 = lastx2[hsId];
	       segment[segment_count].y1 = lasty2[hsId];
	       segment[segment_count].x2 = xoffset2;
	       segment[segment_count].y2 = yoffset2;
	       lastseg[hsId] = segment_count++;
	    }
         }

         lasty2[hsId] = yoffset2;
	 lastx2[hsId] = xoffset2;
      } /* end of for loop */
      
      xoffset2 = (int)((float)(temp1 - temp2) / ptsPerInc) + BlockOffset;
      temp2 --;
   }
   XDrawSegments (Current_display, WorkSpPixmap, WorkSpGC, segment, segment_count);
   update_cursor (0, 0, 3);
   XCopyArea (Current_display, WorkSpPixmap, WorkSpWind, WorkSpGC, 0,0, scopeScreen_x, scopeScreen_y, 0,0);                     
}

void newrev(int pixels)
{
   unsigned int         xoffset,code, time;
   int                  i,yoffset, temp;
   static unsigned int  lastx[30],lasty[30];
   int                  analogId, magnitude;
   int                  lastOffset[1000];

   /* shift the leftmost scopeScreen_x - scrollMargin columns  */
   /* of pixels scrollMargin pixels to the right              */

   XCopyArea( Current_display,           /* display variable     */
	      WorkSpPixmap,              /* source drawable      */
	      WorkSpPixmap,              /* destination drawable */
	      WorkSpGC,                  /* graphics context     */
	      0,0,                       /* source of shift      */
	      scopeScreen_x - pixels,    /* width of shift area  */
	      scopeScreen_y,             /* height of shift area */
	      pixels,0);                 /* destination of shift */         
   XFillRectangle (Current_display,              /* display variable     */
		   WorkSpPixmap,         /* drawable             */
		   WorkSpBgGC,
		   0,                    /* x value of region    */
		   0,                    /* y value of region    */
		   pixels,
		   (unsigned int)scopeScreen_y
		   );

   /* zero the lastx array                    */
   if(last_direction != back) {
      for(i = 0; i < 30; i++) 
	 lastx[i] = lasty[i] = 0;
      last_direction = back;
   }

   /* Zero the last offset array         */
   for(i = 0; i < sf.dchan; i++)
      lastOffset[sf.ids[i]] = -1;

   /* redraw the signal baselines in the margins              */
   for (i=0;i<sf.dchan;i++)
      if(l < sf.dchan)
	 XDrawLine(Current_display,WorkSpPixmap,WorkSpGC,
		   0,      signalBase[sf.ids[i]],
		   pixels, signalBase[sf.ids[i]]);

   /* draw the signals until left margin is reached           */

   /* Set the file position pointer      */
   fseek(fp, screenLeft, 0);

   code = getw(fp);
   time = getw(fp);

   while(code != CODEFLAG && time >= leftTime )
   {
      /*** PPP scaleFactor */
      xoffset = (long long)(time - leftTime) * scopeScreen_x / screenWidth;

      if(code < 1000 && code >= sf.ids[l] && l < sf.dchan) /* a digital signal */
	 if(xoffset != lastOffset[code])
	 {
            XDrawLine(Current_display,WorkSpPixmap,WorkSpGC,xoffset,signalBase[code],xoffset,signalBase[code]-strokeLength);
            lastOffset[code] = xoffset;
	 }
	 else
	    ;
      else
      {
	 analogId = code / 4096;

	 if((code & 0x800) == 0)
            magnitude = code & 0x7FF;
	 else
            magnitude = code | 0xFFFFF000U;

	 temp = sf.amin[anaMap[analogId]];
	 if (AnDiffWarn[analogId])
            yoffset = signalBase[1000 + analogId];
	 else
            yoffset = signalBase[1000+analogId] - (magnitude - temp)*partition * 2 / (3 * (sf.amax[anaMap[analogId]] - temp));


	 if(lasty[analogId] != 0){
            int  from_x;
              
            from_x = (lastx[analogId]-leftTime) * scopeScreen_x /screenWidth;
            XDrawLine(Current_display,WorkSpPixmap,WorkSpGC,
                      from_x,lasty[analogId],xoffset,yoffset);
	 }

	 lastx[analogId] = time;
	 lasty[analogId] = yoffset;
      }
      fseek(fp,-16L,1);

      code = getw(fp);
      time = getw(fp);
   }
   update_cursor (0, 0, 3);
   XCopyArea (Current_display, WorkSpPixmap, WorkSpWind, WorkSpGC, 0,0, scopeScreen_x, scopeScreen_y, 0,0);                      

   screenLeft = ftell(fp) + 16;

   /* re-justify the next_right_offset pointer   */

   fseek(fp,screenRight,0);
   code=getw(fp);
   time=getw(fp);

   if(time <= rightTime)
      fseek(fp,screenRight,0);
   else {

      while(time > rightTime) {
	 code=getw(fp);
	 time=getw(fp);
	 fseek(fp,-16L,1);
      }
      screenRight = ftell(fp) - 8;
   }
   fflush(fp);
}

void newfwd(int pixels)
     /* a subroutine to complete one step of a foreward scroll  */
{
   unsigned int         xoffset, code, time;
   int                  i, yoffset, temp;
   static unsigned int  lastx[30],lasty[30];
   int                  analogId, magnitude;
   int                  lastOffset[1000];

   /* shift the rightmost scopeScreen_x - pixels columns  */
   /* of pixels pixels pixels to the left                     */
   /* and clear the rightmost pixels columns of pixels        */

   XCopyArea( Current_display,           /* display variable     */
	      WorkSpPixmap,              /* source drawable      */
	      WorkSpPixmap,              /* destination drawable */
	      WorkSpGC,          /* graphics context     */
	      pixels,0,          /* source of shift      */
	      scopeScreen_x - pixels,    /* width of shift area  */
	      scopeScreen_y,             /* height of shift area */
	      0,0);                      /* destination of shift */
   XFillRectangle (Current_display,      /* display variable     */
		   WorkSpPixmap,         /* drawable             */
		   WorkSpBgGC,
		   scopeScreen_x - pixels,       /* x value of region    */
		   0,                    /* y value of region    */
		   pixels,
		   (unsigned int)scopeScreen_y
		   );

   /* adjust the lastx array to reflect the shift             */

   if(last_direction != ahead) 
   {
      for(i = 0; i < 30; i++) 
	 lastx[i] = lasty[i] = 0;
      last_direction = ahead;
   }

   /* Zero the last offset array         */
   for(i = 0; i < sf.dchan; i++)
      lastOffset[sf.ids[i]] = 0;

   /* redraw the signal baselines in the margins              */
   for(i = 0; i < sf.dchan; i++)
      if(l < sf.dchan)
	 XDrawLine(Current_display, WorkSpPixmap, WorkSpGC,
		   scopeScreen_x - pixels, signalBase[sf.ids[i]],
		   scopeScreen_x,          signalBase[sf.ids[i]]);

   /* draw the signals until right margin is reached          */

   /* Set the file position pointer      */
   fseek(fp,screenRight,0);

   /* Read and draw signals until EOF or screen is full  */
   code = getw(fp);
   time = getw(fp);

   while(code != CODEFLAG && time < rightTime) 
   {
      xoffset = (long long)(time - leftTime) * scopeScreen_x / screenWidth;

      if(code < 1000 && code >= sf.ids[l] && l < sf.dchan)      /* a digital signal     */
	 if(xoffset != lastOffset[code]) 
	 {
            XDrawLine(Current_display, WorkSpPixmap, WorkSpGC, xoffset, signalBase[code],
                      xoffset, signalBase[code] - strokeLength);
            lastOffset[code] = xoffset;
	 }
	 else ;

      else 
      {
	 analogId = code / 4096;

	 if((code & 0x800) == 0)
            magnitude = code & 0x7FF;
	 else
            magnitude = code | 0xFFFFF000U;

	 temp = sf.amin[anaMap[analogId]];
	 if (AnDiffWarn[analogId])
            yoffset = signalBase[1000 + analogId];
	 else
            yoffset = signalBase[1000+analogId] - (magnitude - temp) * partition * 2 
	       / (3 * (sf.amax[anaMap[analogId]] - temp));


	 if(lasty[analogId] != 0)
	 {
	    int  from_x;
              
	    from_x = (lastx[analogId] - leftTime) * scopeScreen_x / screenWidth;
	    XDrawLine(Current_display, WorkSpPixmap, WorkSpGC, from_x, lasty[analogId], xoffset, yoffset);
	 }
	 lastx[analogId] = time;
	 lasty[analogId] = yoffset;
      }
      code = getw(fp);
      time = getw(fp);
   }
   update_cursor (0, 0, 3);
   XCopyArea (Current_display, WorkSpPixmap, WorkSpWind, WorkSpGC, 0,0, scopeScreen_x, scopeScreen_y, 0,0);                      

  
   screenRight = ftell(fp) - 24;

   /* re-justify the left_offset pointer */

   fseek(fp,screenLeft,0);
   code=getw(fp);
   time=getw(fp);

   if(time >= leftTime)
      fseek(fp, screenLeft, 0);
   else
   {
      while( time < leftTime) 
      {
	 code = getw(fp);
	 time = getw(fp);
      }

      screenLeft = ftell(fp) + 16;
   }
   fflush(fp);
}

void 
hsforeward (int pixels)
{
   int fromTime = rightTime - pixels * screenWidth / (int)scopeScreen_x;
   int block, start, blk_byte_start, samples, n;
   int xoffset, yoffset, time, have_last_val;
   int lastseg[MAX_HDT_CHNL];
   int segment_count = 0;

   for (block = 0; block < hdt_block_count; block++)
      if (fromTime < hdtblkend[block] * scaleFactor && rightTime > hdtblkstart[block] * scaleFactor)
	 break;
   if (block < hdt_block_count) {
      start = MAX (fromTime, hdtblkstart[block] * scaleFactor);
      blk_byte_start = (hstotal * BYTES_PER_SAMPLE
			* ceil ((start - hdtblkstart[block] * scaleFactor) * (double)samplerate / ticks_per_second));
      samples = ceil ((rightTime - fromTime) * (double)samplerate / ticks_per_second) + 1;
      fseeko64 (fp9, startbyte[block] + blk_byte_start, SEEK_SET);
      have_last_val = 0;
      if (segment_alloc < 10000)
	 (segment = realloc (segment, (segment_alloc = 10000) * sizeof (XSegment))) || DIE;
      segment_count = 0;
      memset (lastseg, 0xff, sizeof (lastseg));
      for (n = 0; n < samples; n++) {
	 xoffset = scopeScreen_x - pixels + n * pixels / samples;
	 time = floor (fromTime + n * (double)ticks_per_second / samplerate);
	 if (block < hdt_block_count && time >= hdtblkend[block] * scaleFactor)
	    block++;
	 if (block < hdt_block_count
	     && time >= hdtblkstart[block] * scaleFactor
	     && time <  hdtblkend[block]   * scaleFactor)
	 {
	    for(hsId=0; hsId < hstotal; ++hsId)
	    {
	       if (fread(&HdtValue, SizeOfHdtValue, 1, fp9) != 1) exit (1);
	       SWAB (HdtValue);
               yoffset = basehdt[hsId] - (((double)HdtValue - hsmin[hsId]) * (partition - 1)) / (hsmax[hsId] - hsmin[hsId]);

	       if (have_last_val) {
		  int segn;
		  if (segment_count == segment_alloc) {
		     XDrawSegments (Current_display, WorkSpPixmap, WorkSpGC, segment, segment_count);
		     segment_count = 0;
		     memset (lastseg, 0xff, sizeof (lastseg));
		  }
		  segn = lastseg[hsId];
		  if (segn >= 0 && segment[segn].x1 == xoffset && segment[segn].x2 == xoffset && lastx2[hsId] == xoffset) {
		     segment[segn].y1 = MIN (segment[segn].y1, yoffset);
		     segment[segn].y2 = MAX (segment[segn].y2, yoffset);
		  }
		  else if (lastx2[hsId] == xoffset) {
		     segment[segment_count].x1 = segment[segment_count].x2 = xoffset;
		     segment[segment_count].y1 = MIN (lasty2[hsId], yoffset);
		     segment[segment_count].y2 = MAX (lasty2[hsId], yoffset);
		     lastseg[hsId] = segment_count++;
		  }
		  else {
		     segment[segment_count].x1 = lastx2[hsId];
		     segment[segment_count].y1 = lasty2[hsId];
		     segment[segment_count].x2 = xoffset;
		     segment[segment_count].y2 = yoffset;
		     lastseg[hsId] = segment_count++;
		  }
	       }
	       lasty2[hsId] = yoffset;
	       lastx2[hsId] = xoffset;
	       have_last_val = 1;
	    }
	 }
	 else have_last_val = 0;
      }
   }
   XDrawSegments (Current_display, WorkSpPixmap, WorkSpGC, segment, segment_count);
   update_cursor (0, 0, 3);
   XCopyArea (Current_display, WorkSpPixmap, WorkSpWind, WorkSpGC, 0,0, scopeScreen_x, scopeScreen_y, 0,0);                      
}

void hsreverse(int pixels)
{
   int tilTime = leftTime + pixels * screenWidth / (int)scopeScreen_x;
   int block, start, blk_byte_start, samples, n;
   int xoffset, yoffset, time, have_last_val;
   int lastseg[MAX_HDT_CHNL];
   int segment_count = 0;

   for (block = 0; block < hdt_block_count; block++)
      if (leftTime < hdtblkend[block] * scaleFactor && tilTime > hdtblkstart[block] * scaleFactor)
	 break;
   if (block < hdt_block_count) {
      start = MAX (leftTime, hdtblkstart[block] * scaleFactor);
      blk_byte_start = (hstotal * BYTES_PER_SAMPLE
			* ceil ((start - hdtblkstart[block] * scaleFactor) * (double)samplerate / ticks_per_second));
      samples = ceil ((tilTime - leftTime) * (double)samplerate / ticks_per_second) + 1;
      fseeko64 (fp9, startbyte[block] + blk_byte_start, SEEK_SET);
      have_last_val = 0;
      if (segment_alloc < 10000)
	 (segment = realloc (segment, (segment_alloc = 10000) * sizeof (XSegment))) || DIE;
      segment_count = 0;
      memset (lastseg, 0xff, sizeof (lastseg));
      for (n = 0; n < samples; n++) {
	 xoffset = n * pixels / samples;
	 time = floor (leftTime + n * (double)ticks_per_second / samplerate);
	 if (block < hdt_block_count && time >= hdtblkend[block] * scaleFactor)
	    block++;
	 if (block < hdt_block_count
	     && time >= hdtblkstart[block] * scaleFactor
	     && time <  hdtblkend[block]   * scaleFactor)
	 {
	    for(hsId=0; hsId < hstotal; ++hsId)
	    {
	       if (fread(&HdtValue, SizeOfHdtValue, 1, fp9) != 1) exit (1);
	       SWAB (HdtValue);
               yoffset = basehdt[hsId] - (((double)HdtValue - hsmin[hsId]) * (partition - 1)) / (hsmax[hsId] - hsmin[hsId]);

	       if (have_last_val) {
		  int segn;
		  if (segment_count == segment_alloc) {
		     XDrawSegments (Current_display, WorkSpPixmap, WorkSpGC, segment, segment_count);
		     segment_count = 0;
		     memset (lastseg, 0xff, sizeof (lastseg));
		  }
		  segn = lastseg[hsId];
		  if (segn >= 0 && segment[segn].x1 == xoffset && segment[segn].x2 == xoffset && lastx2[hsId] == xoffset) {
		     segment[segn].y1 = MIN (segment[segn].y1, yoffset);
		     segment[segn].y2 = MAX (segment[segn].y2, yoffset);
		  }
		  else if (lastx2[hsId] == xoffset) {
		     segment[segment_count].x1 = segment[segment_count].x2 = xoffset;
		     segment[segment_count].y1 = MIN (lasty2[hsId], yoffset);
		     segment[segment_count].y2 = MAX (lasty2[hsId], yoffset);
		     lastseg[hsId] = segment_count++;
		  }
		  else {
		     segment[segment_count].x1 = lastx2[hsId];
		     segment[segment_count].y1 = lasty2[hsId];
		     segment[segment_count].x2 = xoffset;
		     segment[segment_count].y2 = yoffset;
		     lastseg[hsId] = segment_count++;
		  }
	       }
	       lasty2[hsId] = yoffset;
	       lastx2[hsId] = xoffset;
	       have_last_val = 1;
	    }
	 }
	 else have_last_val = 0;
      }
   }
   XDrawSegments (Current_display, WorkSpPixmap, WorkSpGC, segment, segment_count);
   update_cursor (0, 0, 3);
   XCopyArea (Current_display, WorkSpPixmap, WorkSpWind, WorkSpGC, 0,0, scopeScreen_x, scopeScreen_y, 0,0);                      
}

/*************************************************************************/
void reInitScope()
{
   int i;

   XFillRectangle (Current_display, WorkSpPixmap, WorkSpBgGC, 0, 0, scopeScreen_x, scopeScreen_y);

   partition = (int)scopeScreen_y / (sf.achan + sf.dchan);
   strokeLength = partition / 3;

   /* Set the digital & analog signal bases        */
   for(i = 0; i < sf.dchan; i++)
      signalBase[sf.ids[i]] = (i+1) * partition - 1 - partition / 7;

   for(i = 0; i < sf.achan; i++)
      signalBase[1000+sf.ids[i+MAX_DIG_CHNL]] = (sf.dchan+1+i) * partition - 1 - m - partition / 7;


   /* Create signal labels for each signal present */
   ClearLabels();
   for(i = 0; i < sf.dchan; i++)
   {
      Hist[i] = sf.ids[i];
      createSignalLabel(sf.ids[i],signalBase[sf.ids[i]]);
   }

   for(i=0;i<sf.achan;i++)
   {
      createSignalLabel(1000+sf.ids[i+MAX_DIG_CHNL],signalBase[1000+sf.ids[i+MAX_DIG_CHNL]]);
   }

   /*    XtVaSetValues(bullBoardW, XmNnumChildren, NextLabel, NULL); */
            
   rewind (fp);
}

void refreshBlocks()
{
   int found, begin, end;
   int done, left, right, pixLeft, pixRight;

   /*printf("refreshblocks.\n"); */
   /* find the first marked block which appears on the screen */
   if((found = FindTime(leftTime,segmentList,&begin,&end)) != NIL){
        
      /* while the current block does not extend past the right
	 of the screen, mark the current blocks on the screen    */
 
      done=False;
      while(done == False)
	 if(begin >  rightTime)
	    done = True;
	 else {
	    left = MAX(leftTime,begin);
	    right = MIN(rightTime,end);

	    pixLeft =(long long)(left-leftTime)* (int)scopeScreen_x/screenWidth;
	    pixRight = (long long)(right-leftTime)*(int)scopeScreen_x/screenWidth;
                
	    XFillRectangle(Current_display,WorkSpPixmap,InvertGC, pixLeft,0,pixRight-pixLeft+1,scopeScreen_y);
            update_cursor (0, 0, 3);
	    XCopyArea (Current_display, WorkSpPixmap, WorkSpWind, WorkSpGC, 0,0, scopeScreen_x, scopeScreen_y, 0,0);


	    if((found = NextBlock(segmentList,&begin,&end)) == NIL)
	       done = True;
	 }
   }

}

void forewardBlocks()
{
   int found, begin, end;
   int done, left, right, pixLeft, pixRight;
   int marginLeft;


   /* find the first marked block which appears on the screen */

   marginLeft = rightTime - marginTime;

   if((found = FindTime(marginLeft,segmentList,&begin,&end)) != NIL){

      /* while the current block does not extend past the right
	 of the screen, mark the current blocks on the screen    */

      done = False;
      while(done == False)
	 if(begin > rightTime)
	    done = True;
	 else {
	    left = MAX(marginLeft,begin);
	    right = MIN(rightTime,end);

	    pixLeft =(left-leftTime)* (int)scopeScreen_x/screenWidth;

	    pixRight = (right-leftTime)*(int)scopeScreen_x/screenWidth;

	    XFillRectangle(Current_display,WorkSpPixmap,InvertGC, pixLeft,0,(pixRight-pixLeft+1),scopeScreen_y);
            update_cursor (0, 0, 3);
	    XCopyArea (Current_display, WorkSpPixmap, WorkSpWind, WorkSpGC, 0,0, scopeScreen_x, scopeScreen_y, 0,0);

	    if((found = NextBlock(segmentList,&begin,&end)) == NIL)
	       done = True;
	 }
   }                                             
}


void reverseBlocks()
{
   int found, begin, end;
   int done, left, right, pixLeft, pixRight;
   int marginRight;

   /* find the first marked block which appears on the screen */

   marginRight = leftTime + marginTime;

   if((found = FindTime(leftTime,segmentList,&begin,&end)) != NIL){

      /* while the current block does not extend past the right
	 of the screen, mark the current blocks on the screen    */

      done = False;
      while(done == False)
	 if(begin > marginRight)
	    done = True;
	 else {
	    left = MAX(leftTime,begin);
	    right = MIN(marginRight,end);

	    pixLeft =(left-leftTime)* (int)scopeScreen_x/screenWidth;

	    pixRight = (right-leftTime)*(int)scopeScreen_x/screenWidth;

	    XFillRectangle(Current_display,WorkSpWind,InvertGC, pixLeft,0,(pixRight-pixLeft),scopeScreen_y);
            update_cursor (0, 0, 3);
	    XCopyArea (Current_display, WorkSpPixmap, WorkSpWind, WorkSpGC, 0,0, scopeScreen_x, scopeScreen_y, 0,0);

	    if((found = NextBlock(segmentList,&begin,&end)) == NIL)
	       done = True;
	 }
   }

}

#define BINCNT 1000
static int     bintotal[BINCNT][MAX_DIG_CHNL];
static int     max[MAX_DIG_CHNL];

void
histogram (HistogramParams *hp)
{
   unsigned int code, time;
   int lastx[30], lasty[30];
   int globalmax;
   Boolean seen[MAX_DIG_CHNL];

   memset (seen, False, sizeof seen);
   memset (bintotal, 0, sizeof bintotal);
   memset (max, 0, sizeof max);

   fseek(fp, screenLeft, 0);

   code = getw(fp);
   time = getw(fp);

   while (code != CODEFLAG && time < rightTime) {
      if (code < MAX_DIG_CHNL && code >= sf.ids[l] && l < sf.dchan) { /* a digital signal */
         int bin = (long long)(time - leftTime) * hp->bins / (rightTime - leftTime);
         bintotal[bin][code]++;
         seen[code]= True;
      }
      code = getw(fp);
      time = getw(fp);
   }
   screenRight = ftell(fp) - 8;

   last_direction = undefined;
   fflush(fp);

   {
      int bin;
      for (bin = 0; bin < hp->bins; bin++)
         for (code = 0; code < MAX_DIG_CHNL; code++)
            if (bintotal[bin][code] > max[code])
               max[code] = bintotal[bin][code];
      globalmax = 0;
      for (code = 0; code < MAX_DIG_CHNL; code++)
         if(max[code] > globalmax)
            globalmax = max[code];
   }

   if (hp->palette) {
      XSetForeground (Current_display, WorkSpGC, color[hp->palette - 1][0].pixel);
      XFillRectangle (Current_display, WorkSpPixmap, WorkSpGC, 0, 0, scopeScreen_x, scopeScreen_y);
      XSetForeground (Current_display, WorkSpGC, black.pixel);
   }
   else
      XFillRectangle (Current_display, WorkSpPixmap, WorkSpBgGC, 0, 0, scopeScreen_x, scopeScreen_y);


   int chan_idx;
   for(chan_idx = l; chan_idx < sf.dchan; chan_idx++)
      XDrawLine(Current_display, WorkSpPixmap, WorkSpGC, 0, signalBase[sf.ids[chan_idx]], scopeScreen_x, signalBase[sf.ids[chan_idx]]);

   if (l < sf.dchan) {
      int pixels_per_bin = DAS_WIDTH / hp->bins;
      double maxy = hp->palette ? 255 : partition * 0.8;
      for (code = sf.ids[l]; code < MAX_DIG_CHNL; code++)
         if (seen[code]) {
            int xoffset;
            for (xoffset = 0; xoffset < DAS_WIDTH; xoffset++) {
               int binscale, y1, y2;
               int bin = xoffset / pixels_per_bin;
         
               if (hp->local)
                  binscale = max[code] ? maxy *  bintotal[bin][code] / max[code] : 0;
               else
                  binscale = globalmax ? maxy / globalmax * bintotal[bin][code] : 0;
                
               y1 = signalBase[code];
               y2 = signalBase[code] - binscale;

               if (hp->palette) {
                  XSetForeground (Current_display, WorkSpGC, color[hp->palette - 1][binscale].pixel);
                  XDrawLine (Current_display, WorkSpPixmap, WorkSpGC, xoffset, y1, xoffset, y1 - partition + 1);
                  XSetForeground (Current_display, WorkSpGC, black.pixel);
               }
               else XDrawLine (Current_display, WorkSpPixmap, WorkSpGC, xoffset, y1, xoffset, y2);
            }
         }
   }

   fseek(fp,screenLeft,0);
   code = getw(fp);
   time = getw(fp);
   memset (lastx, 0, sizeof lastx);
   memset (lasty, 0, sizeof lasty);

   while(code != CODEFLAG && time < rightTime) {
      int xoffset = (long long)(time - leftTime) * (int)scopeScreen_x / screenWidth;
      if(code >= MAX_DIG_CHNL) {
         int magnitude, yoffset, temp;
         int analogId = code / 4096;

	 if((code & 0x800) == 0)
	    magnitude = code & 0x7FF;
	 else
	    magnitude = code | 0xFFFFF000U;

	 temp = sf.amin[anaMap[analogId]];

         yoffset = signalBase[MAX_DIG_CHNL+analogId] - (magnitude - temp)*partition * 2.0 / 
            (3 * (sf.amax[anaMap[analogId]] - temp));

	 if(lasty[analogId] != 0) 
	    XDrawLine (Current_display,WorkSpPixmap,WorkSpGC, lastx[analogId], lasty[analogId],xoffset,yoffset);

	 lastx[analogId] = xoffset;
	 lasty[analogId] = yoffset;
      }

      code = getw(fp);
      time = getw(fp);
   }

   screenRight = ftell(fp) - 8;

   last_direction = undefined;
   fflush(fp);

   double realscWidth = (double)screenWidth / ticks_per_second;
   double bintime = realscWidth / hp->bins;
   
   update_cursor (0, 0, 3);
   XCopyArea (Current_display, WorkSpPixmap, WorkSpWind, WorkSpGC, 0,0, scopeScreen_x, scopeScreen_y, 0, 0);                       

   if(hp->show_max_spk) {
      ClearSideBar ();
      for(chan_idx = 0; chan_idx < sf.dchan; chan_idx++)
         if(chan_idx >= l && chan_idx < channelcount + l) {
            double spikes_per_second;
            if(hp->local) spikes_per_second = max[sf.ids[chan_idx]] / bintime;
            else          spikes_per_second = globalmax             / bintime ;
            showSPS (chan_idx - l, spikes_per_second);
         }  
   }

   if(hdt == 1)
      hssignals();
}

static int globalmax;
static void integrate2();

void
integrate ()                     /* find the max values of the integrated digital signals */
{
   unsigned int code, time;
   int leftmark, binnum;
   int spcount[MAX_DIG_CHNL];
   static int maxtotal[BINCNT][MAX_DIG_CHNL];

   memset (bintotal, 0, sizeof bintotal);
   memset (maxtotal, 0, sizeof maxtotal);
   memset (max,      0, sizeof max);
   memset (spcount,  0, sizeof spcount);

   fseek (fp, screenLeft, SEEK_SET);

   /* Read and draw signals until EOF or screen is full */
   code = getw(fp);
   time = getw(fp);
   leftmark = 0;
   binnum = 0;
   while(code != CODEFLAG && time < rightTime) {
      if(code < MAX_DIG_CHNL) {             /* a digital signal     */
	 if(time > leftmark + binwidth) {
	    int cd;
	    binnum++;
	    leftmark = time;
	    for (cd = 0; cd < MAX_DIG_CHNL; cd++)
	       spcount[cd] = 1;
	 }
	 maxtotal[binnum][code] = spcount[code] + 1;
	 bintotal[binnum][code] = spcount[code]++;
      }
      code = getw(fp);
      time = getw(fp);
   }
   screenRight = ftell(fp) - 8;
   last_direction = undefined;
   for (j = 0; j < MAX_DIG_CHNL; ++j)
      for (i = 0; i < BINCNT; i++)
         if (maxtotal[i][j] >  max[j])
	    max[j] = maxtotal[i][j];
   globalmax = 0;
   for (j = 1; j < MAX_DIG_CHNL; ++j)
      if (max[j] > globalmax)
	 globalmax = max[j];
   integrate2();
}

static void showintegrate();

void integrate2()               /* calculate integMax[code] */
{
   int leftmark, binnum, code;
   unsigned int time, xoffset;

   memset (integDown,   0, sizeof integDown);
   memset (integUp,     0, sizeof integUp);
   memset (integMax,    0, sizeof integMax);
   memset (lastOffset,  0, sizeof lastOffset);
                       
   fseek(fp,screenLeft,0);

   /* Read and draw signals until EOF or screen is full    */
   code = getw(fp);
   time = getw(fp);
   leftmark = 0;
   binnum = 0;
   while(code != CODEFLAG && time < rightTime) {
      xoffset = (long long)(time - leftTime) * (int)scopeScreen_x / screenWidth;
      if(code < MAX_DIG_CHNL ) {
         int binscale;
	 if (time > leftmark + binwidth) {
	    binnum++;
	    leftmark = time;
	 }
	 if (0)                 /* not implemented */
	    binscale = (partition * .8) / globalmax * bintotal[binnum][code];
	 else {
	    if(max[code] < 1)
	       max[code] = 1;
	    binscale = (partition * .8) / (max[code] - 1) *  bintotal[binnum][code];
	 }
	 while(lastOffset[code] < xoffset) {
	    lastOffset[code] = lastOffset[code] + 1; 
	    integDown[code] = integDown[code] * scalefactor;
	    if(integDown[code] < 0.0)
	       integDown[code] = 1.0;
	    integUp[code] = integDown[code];
	 }
	 integUp[code] = (integUp[code] * scalefactor) + (binscale * .15);
	 if(integUp[code] > integMax[code])
	    integMax[code] = integUp[code]; 
	 integDown[code] = integUp[code];
	 lastOffset[code] = xoffset;
      }
      code = getw(fp);
      time = getw(fp);
   }
   screenRight = ftell(fp) - 8;
   last_direction = undefined;
   showintegrate();
}

void showintegrate()
{
   unsigned int time, xoffset;
   int leftmark, binnum, code;
   int lastOffset2[MAX_DIG_CHNL];
   float intvalue, intvalue2;
   int flags[MAX_DIG_CHNL];

   memset (flags,       0, sizeof flags);
   memset (y2old,       0, sizeof y2old);
   memset (integDown,   0, sizeof integDown);
   memset (integUp,     0, sizeof integUp);
   memset (lastOffset2, 0, sizeof lastOffset2);
   memset (lastOffset,  0, sizeof lastOffset);
   memset (lastx,       0, sizeof lastx);
   memset (lasty,       0, sizeof lasty);
            
   XFillRectangle (Current_display, WorkSpPixmap, WorkSpBgGC, 0, 0, scopeScreen_x, scopeScreen_y);

   fseek (fp, screenLeft, 0);

   /* Read and draw signals until EOF or screen is full    */
   code = getw(fp);
   time = getw(fp);
   leftmark = 0;
   binnum = 0;
   while (code != CODEFLAG && time <= rightTime) {
      xoffset = (long long)(time - leftTime ) * (int)scopeScreen_x / screenWidth ;
      if (code < MAX_DIG_CHNL) {
         int y2, binscale;
         integFactor[code] = partition / integMax[code];
	 if (time > leftmark + binwidth) {
	    binnum++;
	    leftmark = time;
	 }
	 if (0)                 /* not implemented */
	    binscale = (partition * .7) / globalmax * bintotal[binnum][code];
	 else {
	    if(max[code] < 1)
	       max[code] = 1;
	    binscale = (partition * .7) / (max[code]-1) * bintotal[binnum][code];
	 }
	 while (lastOffset[code] < xoffset) {
	    lastOffset[code] = lastOffset[code] + 1; 
	    integDown[code] = (integDown[code] * scalefactor);
	    intvalue = (integFactor[code] * integUp[code]) * .7;
	    y2 = signalBase[code] - intvalue;
	    if(code >= sf.ids[l] && xoffset > lastOffset[code] && lastOffset2[code] > 0 )
	       XDrawLine(Current_display, WorkSpPixmap, WorkSpGC, lastOffset2[code], y2old[code], lastOffset[code], y2);
	    lastOffset2[code] = lastOffset[code];
	    y2old[code] = y2;
	    integUp[code] = integDown[code];
	 }
	 integUp[code] = (integUp[code] * scalefactor) + (binscale * .15);
	 intvalue2 = (integUp[code] * integFactor[code]) * .7;
	 y2 = signalBase[code] - intvalue2; 
	 if(code >= sf.ids[l] && lastOffset2[code] >  0 && xoffset < 800 )
	    XDrawLine (Current_display, WorkSpPixmap, WorkSpGC, lastOffset2[code], y2old[code], xoffset, y2);
	 y2old[code] = y2;
	 integDown[code] = integUp[code];
	 lastOffset2[code] = lastOffset[code];
	 lastOffset[code] = xoffset;
	 flags[code] = lastOffset[code];
      }
      else {
	 analogId = code / 4096;
	 if((code & 0x800) == 0)
	    magnitude = code & 0x7FF;
	 else
	    magnitude = code | 0xFFFFF000U;
	 temp = sf.amin[anaMap[analogId]];
	 yoffset = signalBase[MAX_DIG_CHNL + analogId] - (magnitude - temp) * partition * 2.0 / (3 * (sf.amax[anaMap[analogId]] - temp));
         if(lasty[analogId] != 0) 
	    XDrawLine (Current_display, WorkSpPixmap, WorkSpGC, lastx[analogId], lasty[analogId], xoffset, yoffset);
	 lastx[analogId] = xoffset;
	 lasty[analogId] = yoffset;
      }
      code = getw(fp);
      time = getw(fp);
   }
   screenRight = ftell(fp) - 8;
   last_direction = undefined;

   for (code = 0; code < MAX_DIG_CHNL; code++)
      if (flags[code] !=0  && flags[code] < 800) {
         xoffset = 800;
         integFactor[code] = partition / integMax[code];
	 while(lastOffset[code] < xoffset  ) /** addded a 5 here  **/ { 
            int y2;
	    lastOffset[code] = lastOffset[code] + 1; 
	    integDown[code] = (integDown[code] * scalefactor);
	    intvalue = (integFactor[code] * integUp[code]) * .7;
	    y2 = signalBase[code] - intvalue; 
	    if (code >= sf.ids[l])
               XDrawLine (Current_display,WorkSpPixmap,WorkSpGC,lastOffset2[code],y2old[code],lastOffset[code],y2);
	    lastOffset2[code] = lastOffset[code];
	    y2old[code] = y2;
	    flags[code] = lastOffset[code];
	    integUp[code] = integDown[code];
	 }
      }
   update_cursor (0, 0, 3);
   XCopyArea (Current_display, WorkSpPixmap, WorkSpWind, WorkSpGC, 0,0, scopeScreen_x, scopeScreen_y, 0,0);                      
   XFlush(Current_display);

   if(hdt==1)
      hssignals();
}

void
ps (PSParams *p)
{
   char *cmd;
   
   if (asprintf (&cmd, "edt2ps --start %d --stop %d --paper %s --norm %s -tpb %d -tps %d -sms %s < %s > %s",
                 leftTime, rightTime,
                 p->legal  ? "legal"  : "letter",
                 p->global ? "global" : "local",
                 p->ticks_per_bin,
                 ticks_per_second,
                 p->tally ? "no" : "yes",
                 sf.Ifname,
                 p->filename) == -1) exit (1);
   if (system (cmd));
   free (cmd);
   if (asprintf (&cmd, "gv %s &", p->filename) == -1) exit (1);
   if (system (cmd));
   free (cmd);
}


/* Local Variables: */
/* c-basic-offset: 3 */
/* c-file-offsets:  ((substatement-open . 0)) */
/* End: */
