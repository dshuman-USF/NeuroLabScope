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



/*************************************************************
*** std -- source derived from xscopepb                    ***
***                                                        ***
*** started 7-25-98                                        ***
*** Peter R. Barnhill                                      ***
***                                                        ***
***    Version 1.0  24/Apr/02                              ***
***                                                        ***
*** Version History:                                       ***
***                                                        ***
***                                                        ***
***                                                        ***
***                                                        ***
*************************************************************/

/*** header files */
#include "std.h"		/* templates & definitions */
#include "filesup.h"		/* file info */
#include "blockList.h"		/* ????? */
#include "FilterTree.h"
#include "dispUnit64.h"
#include "options.h"
#include "WritePop.h"
#include "AddTree.h"
#include "color_table.h"
#include <Xm/Form.h>
#include <X11/Xmu/Editres.h>
#include <sys/time.h>
#include <X11/cursorfont.h>
#include <sys/utsname.h>

Widget app_shell;		/* ApplicationShell     */
Widget main_window;		/* MainWindow           */
Widget MessageBoxW;		/* Message Box */
Widget SideBarLabel;		/* Side Box Label */
Widget HSDChanW;		/* Message Box */
Widget bullBoard2W;		/*  BulletinBoard       */
Widget horiz_bar;		/*  Horizontal Scrollbar */
Widget ScreenLeft2W;
Widget DigitalChanW;
Widget AnalogChanW;
Widget SignalLabel2W[MAX_HDT_CHNL];
Display *Current_display;
GC WorkSpGC;
GC WorkSpBgGC;
GC InvertGC;
Window WorkSpWind;
Pixmap WorkSpPixmap;
int temppos;
int sliderpos;
int state2;
char textStr[256];
char *newname2;
int totalTime;
int l, m;
int channelcount;
Boolean FilterPopOpened;
Boolean MessageBoxUsed;
Boolean AnDiffWarn[MAX_ANLG_CHNL];
int scaleFactor, ticks_per_second;
int currFileType;

int WIN_HEIGHT;
int WIN_WIDTH;
int DAS_WIDTH;


static int mousePos, fileLoaded;
static int lastLeftTime;
static int lastPick, pickCount;
static int p = 10;              /* number of displayed channels */
static int lastInp, thisPick;
static int fpointer;
static XmString textString2;
static Display *display;
static Widget DrawingArea;
static Widget CursorPos2W;
static Widget psfileselect;
static Widget open_button;
static Widget fileselect;
static Widget FilenameW;
static Widget bullBoardW;		/*  BulletinBoard       */
static Widget ChannelBoxW;
static Widget ScrollBoxW;
static Widget ScreenWidthW;
static Widget form;			/*  MenuBar             */
static char countStr[256];
static char chanStr[25];
static char scrollStr[25];

static char *
sysname (void)
{
  static struct utsname u;
  if (uname (&u) == 0)
    return u.sysname;
  return "";
}

/*** pick block */
/* Callback routine for selecting edit blocks manually  */
void
pickBlock (Widget w, XtPointer client_data, XtPointer call_data)
{
  int ins;

  if (thisPick < lastInp)
  {
     int temp1 = lastInp;
     lastInp = thisPick;
     thisPick = temp1;
  }

  if (LookTog () != RESET)
    {
      ActuateTog ();
      lastInp = thisPick;
    }
  else
    {
      if (thisPick != lastInp)
	{
	  refreshBlocks ();
	  if ((ins = InsBlock (segmentList, lastInp, thisPick)) == NIL)
	    printf ("error inserting new block in block list\n");
	  refreshBlocks ();

	  ActuateTog ();
	}
    }
}

/*** motion ??? */
static void
motion (Widget w, XtPointer client_data, XEvent * event, Boolean * continu)
{
   static int lastx, lasty;
   static int cursorPos1[5];

   printf ("dx: %g (%d), dy: %d\n",
           (double)(event->xmotion.x - lastx) / scopeScreen_x * screenWidth / ticks_per_second,
           event->xmotion.x - lastx, event->xmotion.y - lasty);
   lastx = event->xmotion.x;
   lasty = event->xmotion.y;
   
   /* routines for selecting passing filter blocks */
   if (!fileLoaded || !FilterPopOpened)
      return;

   if (XmToggleButtonGetState (chunkUnselectButtonW) == 0)
   {
      cursorPos1[fpointer] = event->xmotion.x;
      if (fpointer == 0)
	 lastInp  = (long long)cursorPos1[0] * screenWidth / (int) scopeScreen_x + leftTime;
      else
	 thisPick = (long long)cursorPos1[1] * screenWidth / (int) scopeScreen_x + leftTime;
      fpointer = fpointer + 1;

      if (XmToggleButtonGetState (manualMethodButtonW) == 1 && fpointer == 2)
      {
	 pickBlock (w, event, event);
	 ResetTog ();
	 fpointer = 0;
      }
   }
   if (fpointer > 1 || XmToggleButtonGetState (manualMethodButtonW) == 0)
      fpointer = 0;
}

static void
draw_cursor (int x, int y)
{
  XSetFunction (Current_display, WorkSpGC, GXxor);
  XSetForeground (Current_display, WorkSpGC, -1);
  XDrawLine (Current_display, WorkSpWind, WorkSpGC, x, 0, x, scopeScreen_y);
  XDrawLine (Current_display, WorkSpWind, WorkSpGC, 0, y, scopeScreen_x, y);
  XSetForeground (Current_display, WorkSpGC, 0);
  XSetFunction (Current_display, WorkSpGC, GXcopy);
}


//0 = turn cursor off
//1 = turn off cursor at old location, turn on cursor at new location
//2 = turn off cursor at old location, draw vertical line at old location, turn on cursor at new location
//3 = set cursor_on indication to false without drawing anything
void
update_cursor (int new_x, int new_y, int func)
{
  static int cursor_x;
  static int cursor_y;
  static Boolean cursor_on;

  if (func == 3) {
     cursor_on = False;
     return;
  }
     
  if (cursor_on)
    draw_cursor (cursor_x, cursor_y);
  cursor_on = False;

  if (func == 0)
    return;

  if (func == 2)
     XDrawLine (Current_display, WorkSpWind, WorkSpGC, cursor_x, 0, cursor_x, scopeScreen_y);
  draw_cursor (cursor_x = new_x, cursor_y = new_y);
  cursor_on = True;
}

/*** I do not know ??? -- pb */
FILE *newEvents;


/***********************************************************************/
/*** pick event */
/* Callback routine for selecting new events manually  */
static void
pickEvent (int cursorPos2, int y)
{
  int thisPick;
  Cardinal i;
  Arg arglist[3];
  XmString countString;

  thisPick = cursorPos2 * screenWidth / (int) scopeScreen_x + leftTime;
  if (thisPick > lastPick)
    {
       //      XDrawLine (Current_display, WorkSpWind, WorkSpGC, cursorPos2, 0, cursorPos2, scopeScreen_y);
       update_cursor (cursorPos2, y, 2);
       
      fseek (newEvents, 0, SEEK_END);
      putw (thisPick, newEvents);
      newEventTally++;

      pickCount++;
      sprintf (countStr, "%d", pickCount);

      countString = XmStringCreateLtoR (countStr, XmSTRING_DEFAULT_CHARSET);
      i = 0;
      XtSetArg (arglist[i], XmNlabelString, countString); i++;
      XtSetValues (manCtOutStatW, arglist, i);

      lastPick = thisPick;
    }
}


/*** unselect block */
/* Callback routine for unselecting blocks individually  */
void
unselectBlock (int cursorPos2)
{
  int thisPick;
  int begin, end;

  thisPick = cursorPos2 * screenWidth / (int) scopeScreen_x + leftTime;

  if (FindTime (thisPick, segmentList, &begin, &end) != NIL)
    if (thisPick >= begin)
      {
	refreshBlocks ();
	if (DelBlock (segmentList, begin, end) == NIL)
	  printf ("ERROR  - unable to delete block\n\n");
	refreshBlocks ();
      }
}

/*** marker ??? */
static void
marker (Widget w, XtPointer client_data, XEvent * event, Boolean * continu)
{
   int cursorPos2;

   if (!fileLoaded)
      return;

   cursorPos2 = event->xmotion.x;
   if (manPop_up)
      pickEvent (cursorPos2, event->xmotion.y);

   /* routine for selecting filter blocks to be deleted */
   if (FilterPopOpened && XmToggleButtonGetState (chunkUnselectButtonW) == 1)
   {
      unselectBlock (cursorPos2);
      ResetTog ();
   }
}

/*** mouse ??? */
static void
mouse (Widget w, XtPointer client_data, XEvent * event, Boolean * continu)
{

  mousePos = event->xmotion.x;
  update_cursor (mousePos, event->xmotion.y, 1);
}


/*** clear message box */
void
clearMessageBox (void)
{
  Cardinal i;
  Arg args[3];
  XmString MBString;

  i = 0;
  MBString = XmStringCreateLtoR ("\0", XmSTRING_DEFAULT_CHARSET);
  XtSetArg (args[i], XmNlabelString, MBString); i++;
  XtSetValues (MessageBoxW, args, i);

  MessageBoxUsed = 0;
}


/*** newPos */
void
newPos (Widget w, XtPointer client_data, XEvent * event, Boolean * continu)
{
  /* Reset the cursor position widget */
  float realPos;
  Cardinal i;
  Arg args[3];
  XmString textString;

  if (fileLoaded == 1)
    {
      realPos = (mousePos / (float) DAS_WIDTH * screenWidth + leftTime) / (double)ticks_per_second;	/* was *22.66 */

      sprintf (textStr, "  %10.3f, %d", realPos, scopeScreen_y - event->xmotion.y);

      textString = XmStringCreateLtoR (textStr, XmSTRING_DEFAULT_CHARSET);

      i = 0;
      XtSetArg (args[i], XmNlabelString, textString); i++;
      XtSetValues (CursorPos2W, args, i);
    }
}

static int
round_width (int screenWidth)
{
  int sms, ticks_per_sm;

   if (scrollMargin == 0)
     scrollMargin = 8;
   if (scopeScreen_x == 0)
     scopeScreen_x = DAS_WIDTH;
   sms = (int) scopeScreen_x / scrollMargin;
   if (sms * scrollMargin != (int) scopeScreen_x)
   {
      printf("These numbers should be the same: %d and %d. Pick a different screen width.\n",
             sms*scrollMargin, scopeScreen_x);
      errno = EINVAL;
      DIE;
   }

  ticks_per_sm = floor ((double) screenWidth / sms + .5);
  if (screenWidth == fileTime)
     while (ticks_per_sm * sms < fileTime + 1)
	ticks_per_sm++;
  screenWidth = ticks_per_sm * sms;
  if (screenWidth < sms)
    screenWidth = sms;
  return screenWidth;
}


/*** get offset */
/* this routine converts a time (within the file limits)        */
/* to a corresponding record offset within the file             */
long
getOffset (int searchTime)
{
  long lowerRec, upperRec, searchRec, result = 0;
  int done, time;
  
  lowerRec = 4;
  upperRec = sf.recCount + 4;
  done = 0;

  fseek (fp, 8 * lowerRec, SEEK_SET);
  getw (fp); time = getw (fp);
  if (time >= searchTime) return 8 * lowerRec;
  
  while (done == 0)
    {
      searchRec = (upperRec + lowerRec) / 2;

      fseek (fp, 8 * searchRec, SEEK_SET);
      getw (fp);
      time = getw (fp);

      if (time >= searchTime)
	upperRec = searchRec;
      else
	lowerRec = searchRec;

      if ((upperRec - lowerRec) < 2) {
         result = 8 * upperRec;
         done = 1;
      }
    }
  return (result);
}

/*** setwidth */
void
setwidth (float realWidth)
{
   Cardinal i;
   Arg arglist[6];

   screenWidth = round_width ((double)ticks_per_second * realWidth);

   if (fileLoaded == 1)
   {
      /* Check for potential error conditions and correct them     */
      if (screenWidth > fileTime)
	 screenWidth = round_width (fileTime);

      marginTime = screenWidth * scrollMargin / (int) scopeScreen_x;

      if ((leftTime + screenWidth) > (firstTime + fileTime + 1))
	 leftTime = firstTime + fileTime + 1 - screenWidth;
      if (leftTime < firstTime)
	 leftTime = firstTime;
      if (fileTime == 0)
	 leftTime = firstTime;

      rightTime = leftTime + screenWidth;
      screenLeft = getOffset (leftTime);

      clearMessageBox ();
      XFillRectangle (Current_display, WorkSpPixmap, WorkSpBgGC, 0, 0,
		      scopeScreen_x, scopeScreen_y);
   }

   i = 0;
   XtSetArg(arglist[i], XmNmaximum, MAX (leftTime + screenWidth, firstTime + fileTime + 1)); i++;
   XtSetArg(arglist[i], XmNsliderSize, screenWidth); i++;
   XtSetArg(arglist[i], XmNvalue, leftTime); i++;
   XtSetArg(arglist[i], XmNminimum, firstTime); i++;
   XtSetArg(arglist[i], XmNincrement, screenWidth * scrollMargin / (int) scopeScreen_x); i++;
   XtSetArg(arglist[i], XmNpageIncrement, screenWidth); i++;
   XtSetValues(horiz_bar, arglist, i);

   realWidth = (double)screenWidth / ticks_per_second;
   sprintf (textStr, "Screen Width:%9.3f", realWidth);

   XFillRectangle (Current_display, WorkSpPixmap, WorkSpBgGC, 0, 0,
		   scopeScreen_x, scopeScreen_y);

   if (fileLoaded == 1)
      refresh ();

   XtUnmanageChild (ScreenWidthW);

   i = 0;
   XtSetArg (arglist[i], XmNy, WIN_HEIGHT-90); i++;
   XtSetArg (arglist[i], XmNx, 650); i++;
   ScreenWidthW = XmCreateLabel (form, textStr, arglist, i);
   XtManageChild (ScreenWidthW);
}

void newfwd (int pixels);
void newrev (int pixels);
void scrollselect (Widget w, XtPointer closure, XtPointer call_data);

/* the smallest number of pixels that is an integer number of ticks */
static int
round_pixels (void)
{
  int n, q;
  for (n = 1; n <= scrollMargin; n++)
    {
      q = (n * screenWidth) / (int) scopeScreen_x;
      if (q * scopeScreen_x == n * screenWidth)
	return n;
    }
  return 0;
}

void
newslide (Widget w, XtPointer closure, XtPointer call_data)
{
  Cardinal i;
  Arg args[3];
  int pixels, integral_pixels, ticks;
  XmScrollBarCallbackStruct *cbs = (XmScrollBarCallbackStruct *) call_data;

  i = 0;
  XtSetArg (args[i], XmNvalue, &leftTime); ++i;
  XtGetValues (horiz_bar, args, i);
  set_left_time ();

  ticks = abs (leftTime - lastLeftTime);
  pixels = floor (ticks * (int) scopeScreen_x / screenWidth + .5);

  if (cbs->reason == XmCR_DRAG)
    {
      XClearWindow (Current_display, WorkSpWind);
      realLeft = (double)leftTime / ticks_per_second;
      sprintf (textStr, "%7.3f", realLeft);
      textString2 = XmStringCreateLtoR (textStr, XmSTRING_DEFAULT_CHARSET);
      i = 0;
      XtSetArg (args[i], XmNlabelString, textString2); i++;
      XtSetValues (ScreenLeft2W, args, i);
      return;
    }

  if (cbs->reason == XmCR_DRAG && pixels < (int) scopeScreen_x)
    {
      int rp = round_pixels ();
      pixels = floor (pixels / rp + .5) * rp;
      ticks = (long long)pixels * screenWidth / (int) scopeScreen_x;
      ticks * scopeScreen_x == pixels * screenWidth || DIE;	/* debug */
      if (leftTime < lastLeftTime)
	leftTime = lastLeftTime - ticks;
      else
	leftTime = lastLeftTime + ticks;
    }
  integral_pixels = ticks * scopeScreen_x == pixels * screenWidth;
  if (!integral_pixels || pixels >= (int) scopeScreen_x)
    {
      if (leftTime != lastLeftTime)
	scrollselect (w, closure, call_data);
      return;
    }

  clearMessageBox ();
  if (fileTime > screenWidth)
    {
      rightTime += leftTime - lastLeftTime;

      if (leftTime != lastLeftTime)
	{
	   realLeft = (double)leftTime / ticks_per_second;
	  sprintf (textStr, "%7.3f", realLeft);
	  textString2 =
	    XmStringCreateLtoR (textStr, XmSTRING_DEFAULT_CHARSET);
	  i = 0;
	  XtSetArg (args[i], XmNlabelString, textString2); i++;
	  XtSetValues (ScreenLeft2W, args, i);

	  if (leftTime > lastLeftTime)
	    {
	      newfwd (pixels);
	      if (hdt == 1)
		hsforeward (pixels);
	    }
	  else
	    {
	      newrev (pixels);
	      if (hdt == 1)
		hsreverse (pixels);
	    }
	  lastLeftTime = leftTime;
	}
    }
}

/*** scrollup */
/*** girish added */
void
ScrollUp (Widget w, XtPointer closure, XtPointer call_data)
{
  Cardinal i;
  Arg args[3];
  XmString BoxString3;

  p = p + 1;
  if (p > 10)
    p = 10;
  sprintf (scrollStr, "%1d", p);

  i = 0;
  BoxString3 = XmStringCreateLtoR (scrollStr, XmSTRING_DEFAULT_CHARSET);
  XtSetArg (args[i], XmNlabelString, BoxString3); i++;
  XtSetValues (ScrollBoxW, args, i);
}


/*** scolldown */
/*** girish added */
void
ScrollDown (Widget w, XtPointer closure, XtPointer call_data)
{
  Cardinal i;
  Arg args[3];
  XmString BoxString4;

  p = p - 1;
  if (p < 1)
    p = 1;
  sprintf (scrollStr, "%1d", p);

  i = 0;
  BoxString4 = XmStringCreateLtoR (scrollStr, XmSTRING_DEFAULT_CHARSET);
  XtSetArg (args[i], XmNlabelString, BoxString4); i++;
  XtSetValues (ScrollBoxW, args, i);
}


/*** up arrow */
/* Callback routine for a slider movement */
void
UpArrow (Widget w, XtPointer closure, XtPointer call_data)
{
/* Code for scrolling vertically upward */
  clearMessageBox ();
  if (l > 0) {
      if ((l - p) < 0)
         l = 0;
      else
         l = l - p;
      partition = (int) scopeScreen_y / channelcount;	/* removed 2 */

      if (l == 0)
         m = 0;
      else
         m = m - p * partition;
      XFillRectangle (Current_display, WorkSpPixmap, WorkSpBgGC, 0, 0,
		      scopeScreen_x, scopeScreen_y);
      refresh ();
    }
}

static inline int
trace_count (int channelcount)
{
   int partition;
   
   partition = (int)scopeScreen_y / channelcount;
   if (partition == 0)
      partition = 1;
   return scopeScreen_y / partition;
}

/* Increase the number of channels in the drawing area */
void
ChannelUp (Widget w, XtPointer closure, XtPointer call_data)
{
  Cardinal i;
  Arg args[3];
  XmString BoxString;
  int trace_count_0;
  int max_trace_count = trace_count (scopeScreen_y);

  trace_count_0 = trace_count (channelcount);
  if (trace_count_0 == max_trace_count)
     return;
  l = 0;
  m = 0;
  while (trace_count (channelcount) == trace_count_0)
     channelcount++;
  trace_count_0 = trace_count (channelcount);
  if (trace_count_0 < max_trace_count)
     while (trace_count (channelcount + 1) == trace_count_0)
	channelcount++;
  sprintf (chanStr, "%d", channelcount);

  i = 0;
  BoxString = XmStringCreateLtoR (chanStr, XmSTRING_DEFAULT_CHARSET);
  XtSetArg (args[i], XmNlabelString, BoxString); i++;
  XtSetValues (ChannelBoxW, args, i);
}


/* Decrease the number of channels in the drawing area */
void
ChannelDown (Widget w, XtPointer closure, XtPointer call_data)
{
  Cardinal i;
  Arg args[3];
  XmString BoxString2;
  int trace_count_0;

  if (channelcount <= 1)
     return;
  l = 0;
  m = 0;
  trace_count_0 = trace_count (channelcount);
  while (trace_count (channelcount) == trace_count_0)
     channelcount--;
  channelcount >= 1 || DIE;
  i = 0;
  sprintf (chanStr, "%d", channelcount);
  BoxString2 = XmStringCreateLtoR (chanStr, XmSTRING_DEFAULT_CHARSET);
  XtSetArg (args[i], XmNlabelString, BoxString2); i++;
  XtSetValues (ChannelBoxW, args, i);
}


/* Code for scrolling vertically downward */
/*** updown scroll girish ********/
void
DownArrow (Widget w, XtPointer closure, XtPointer call_data)
{
  extern int hstotal;

  clearMessageBox ();
  partition = (int) scopeScreen_y / channelcount;	/* removed 2 */
  if ((l + p + channelcount) > (sf.achan + sf.dchan + hstotal)) {
     m = m + (sf.achan + sf.dchan + hstotal - channelcount - l) * partition;
     l = sf.achan + sf.dchan + hstotal - channelcount;
     if (l < 0) {
        l = 0;
        m = 0;
     }
  }
  else {
     l = l + p;
     m = m + p * partition;
  }
  XFillRectangle (Current_display, WorkSpPixmap, WorkSpBgGC, 0, 0,
		  scopeScreen_x, scopeScreen_y);
  refresh ();
}


/*** slide blocks */
/* Callback routine for a slider movement */
/* Scrolling routine to horizontally scroll filter blocks */
void
slideBlocks (Widget w, XtPointer closure, XtPointer call_data)
{
   int i;
   Arg args[3];

   /*** QQQ
	printf("slideBlocks.\n");
   ***/
   clearMessageBox ();

   i = 0;
   XtSetArg (args[i], XmNvalue, &sliderpos); i++;
   XtGetValues (horiz_bar, args, 1);

   if (sliderpos > temppos)
   {
      forewardBlocks ();
   }
   else if (sliderpos < temppos && leftTime >= 0)
   {
      reverseBlocks ();
   }
   /*** ???*/
   if (temppos <= 0)
      temppos = 1;
   temppos = sliderpos;
}


/*** scrollselect */
/* Callback routine for a scroll region selected */
/*   Code to move about the file by clicking mouse pointer in trough */
void
scrollselect (Widget w, XtPointer closure, XtPointer call_data)
{
   extern int hstotal;
   Cardinal i;
   Arg arglist[3];
   int debugprint = 0;

   /*** QQQ
	printf("scrollselect.\n");
   ***/
   clearMessageBox ();

   if (debugprint)
   {
      int val, width, pi;
      XtVaGetValues (horiz_bar, XmNvalue, &val, NULL);
      XtVaGetValues (horiz_bar, XmNsliderSize, &width, NULL);
      XtVaGetValues (horiz_bar, XmNpageIncrement, &pi, NULL);
      printf ("%s line %d: %d %d %d\n", __FILE__, __LINE__, val, width, pi);
      printf ("%ld\n", screenWidth * scrollMargin / (int) scopeScreen_x);
   }

   if (fileTime > screenWidth)
   {
      long pointeroff2;
      XtVaGetValues (horiz_bar, XmNvalue, &leftTime, NULL);
      set_left_time ();
      rightTime = leftTime + screenWidth;
      screenLeft = getOffset (leftTime);
      realLeft = (double)leftTime / ticks_per_second;
      if (hdt == 1)
      {
	 pointeroff2 = (double)realLeft * samplerate * hstotal * BYTES_PER_SAMPLE;
	 while ((pointeroff2 % (hstotal * BYTES_PER_SAMPLE)) != 0)
	    pointeroff2++;
	 fseek (fp9, pointeroff2, 0);
      }

      XFillRectangle (Current_display, WorkSpPixmap, WorkSpBgGC, 0, 0,
		      scopeScreen_x, scopeScreen_y);

      refresh ();

      if (leftTime != lastLeftTime)
      {
	 realLeft = (double)leftTime / ticks_per_second;
	 sprintf (textStr, "%7.3f", realLeft);
	 textString2 =
	    XmStringCreateLtoR (textStr, XmSTRING_DEFAULT_CHARSET);
	 i = 0;
	 XtSetArg (arglist[i], XmNlabelString, textString2); i++;
	 XtSetValues (ScreenLeft2W, arglist, i);
	 lastLeftTime = leftTime;
      }
      refreshBlocks ();
   }
}


/*** find cb */
void
FindCB (Widget w, XtPointer closure, XtPointer call_data)
{
  makeFindPop ();
}

/* This routine creates a signal label (button) for the given
   signal Id and offset.  The button is a toggle and its state
   can be used as a switch for filtering entire signals from
   an output file.                                              */

/* global variables */
Widget SignalLabelW[MAX_DIG_CHNL];
/*Widget SignalLabel2W[40];*/
Cardinal NextLabel, NextLabel2;
/***
int     buttonHeight;
*/


void
createSignalLabel (int ident, int loc)
{
  Cardinal i;
  Arg arglist[10];
  char sigLabel[15];

  /* digital signal case */
  if (ident < 1000)
    sprintf (sigLabel, "%4d", ident);
  /* analog signal case */
  else
    sprintf (sigLabel, "An %1d", ident - 1000);

  /* Arguments for push button widget */
  i = 0;

  XtSetArg (arglist[i], XmNlabelType, XmSTRING); i++;
  XtSetArg (arglist[i], XmNhighlightOnEnter, False); i++;
  XtSetArg (arglist[i], XmNborderWidth, 0); i++;
  XtSetArg (arglist[i], XmNmarginHeight, 0); i++;
  XtSetArg (arglist[i], XmNheight, 17); i++;
  XtSetArg (arglist[i], XmNy, loc - 13); i++;
  XtSetArg (arglist[i], XmNx, 5); i++;
  {
    XmString label_str = XmStringCreateLocalized (sigLabel);
    XtSetArg (arglist[i], XmNlabelString, label_str); i++;
    SignalLabelW[NextLabel] =
      XmCreateToggleButton (bullBoardW, sigLabel, arglist, i);
    XmStringFree (label_str);
  }
  XtManageChild (SignalLabelW[NextLabel]);

  NextLabel++;
}


void
createDummyLabel ()
{
  Cardinal i;
  Arg arglist[10];

  /* arguments for bulletin board parent */
  i = 0;
  XtSetArg (arglist[i], XmNvisibleWhenOff, False); i++;
  XtSetArg (arglist[i], XmNhighlightOnEnter, False); i++;
  XtSetArg (arglist[i], XmNborderWidth, 0); i++;
  XtSetArg (arglist[i], XmNmarginHeight, 0); i++;
  XtSetArg (arglist[i], XmNx, 5); i++;
  XtSetArg (arglist[i], XmNy, 2); i++;				/* 25 is the buttonHeight */
  SignalLabelW[NextLabel] =
    XmCreateToggleButton (bullBoardW, "    ", arglist, i);
  XtManageChild (SignalLabelW[NextLabel]);

  NextLabel++;
}


/*** create hs label */
void
createHSLabel (int ident2, int loc2)
{
  Cardinal i;
  Arg arglist[10];
  char sigLabel2[15];

  sprintf (sigLabel2, "HS %1d", ident2 + 1);
  i = 0;
  XtSetArg (arglist[i], XtNlabel, (XtArgVal) sigLabel2); i++;
  XtSetArg (arglist[i], XmNhighlightOnEnter, False); i++;
  XtSetArg (arglist[i], XmNborderWidth, 0); i++;
  XtSetArg (arglist[i], XmNmarginHeight, 0); i++;
  XtSetArg (arglist[i], XmNheight, 17); i++;
  XtSetArg (arglist[i], XmNy, loc2 - 13); i++;

  XtSetArg (arglist[i], XmNx, 5); i++;
  SignalLabel2W[NextLabel2] =
    XmCreateToggleButton (bullBoardW, sigLabel2, arglist, i);

  XtManageChild (SignalLabel2W[NextLabel2]);
  NextLabel2++;
}



/*** clear labels */
void
ClearLabels ()
/* This routine clears all signal labels from the scope, and sets a
   global variables used in other routines.                             */
{
  Cardinal x;

  if (NextLabel > 0)
    {
       for (x = 0; x < NextLabel; x++) {
          XtDestroyWidget (SignalLabelW[x]);
          SignalLabelW[x] = 0;
       }
      NextLabel = 0;
    }
}


/*** clear hs labels */
void
ClearHSLabels ()
{
  Cardinal i;

  if (NextLabel2 > 0)
    {
      for (i = 0; i < NextLabel2; i++)
	XtDestroyWidget (SignalLabel2W[i]);
      NextLabel2 = 0;
    }
}


/*** any label set */
Boolean
anyLabelSet ()
{
  Cardinal i, j;
  Arg arglist[5];
  Boolean getValue, setValue;

  setValue = False;

  i = 0;
  XtSetArg (arglist[i], XmNset, &getValue); i++;

  for (j = 0; j < NextLabel; j++)
    {
      XtGetValues (SignalLabelW[j], arglist, i);
      setValue = setValue || getValue;
    }
  return (setValue);
}


/*** d channel set */
Boolean
dchanSet (int index)
{
  Cardinal i;
  Arg arglist[3];
  Boolean getValue;


  getValue = False;

  i = 0;
  XtSetArg (arglist[i], XmNset, &getValue); i++;
  XtGetValues (SignalLabelW[index], arglist, i);

  return (getValue);
}


/*** a channel set */
Boolean
achanSet (int index)
{
  Cardinal i;
  Arg arglist[3];
  Boolean getValue;


  getValue = False;

  i = 0;
  XtSetArg (arglist[i], XmNset, &getValue); i++;
  XtGetValues (SignalLabelW[index + sf.dchan], arglist, i);

  return (getValue);
}


/* The following routines support the addition of new event trains
   to the display, and therefore the DA file.                           */

  /* int lastPick, pickCount; */

void
initNewEvents ()
{
  newEvents = tmpfile ();

  if (newEvents == NULL)
    {
      printf ("Unable to open newEvents temp file for reading & writing\n");
      exit (1);
    };
  lastPick = -1;
  pickCount = 0;
  newEventTally = 0;
  sprintf (countStr, "%d", pickCount);
}

void print (XtPointer client_data, XtIntervalId *id)
{
   String title;
   char *cmd, *dmpfile;
   void ExposeCB (Widget w, XtPointer client_data, XtPointer call_data);

   XWarpPointer(Current_display, 0, XtWindow (app_shell), 0, 0, 0, 0, 0, 0);
   ExposeCB (0, 0, 0);
   XSync (Current_display, False);
   XtVaGetValues (app_shell, XmNtitle, &title, NULL);
   strchr (title, '\'') && DIE;
   if (strcmp (sysname (), "Linux") == 0) {
      if (asprintf (&cmd, "import -silent -window 0x%x -density 78x78 -page '612x792+30+30' -rotate 90 temp.ps",
                    (unsigned int)XtWindow (app_shell)) == -1) exit (1);
      if (system(cmd)); free (cmd);
      if (asprintf (&cmd, "lpr temp.ps") == -1) exit (1);
      if (system(cmd)); free (cmd);
   }
   else if (strcmp (sysname (), "HP-UX") == 0) {
      if (asprintf (&cmd, "xwd -silent -multi -id \'0x%x\' -out \'%s.dmp\' -db", (unsigned int)XtWindow (app_shell), title) == -1) exit (1);
      if (system(cmd)); free (cmd);
      if (asprintf (&cmd, "/usr/contrib/bin/X11/xpr -density 600 -device ljet -rv \'%s.dmp\' | lp -oraw -onb", title) == -1) exit (1);
      if (system(cmd)); free (cmd);
   }
   else {
      makeWarningPop ("FYI", "Don't know how to print on this system.");
   }

   /*   system("xwd -silent -single -name STDX -out STDX.dmp -db");  // test overlaps */
   if (asprintf (&dmpfile, "%s.dmp", title) == -1) exit (1);
   remove(dmpfile); free (dmpfile);
   makeWarningPop ("FYI", "\"Print LaserJet\" done");
}

/* Callback routine for Done Push Button Release */
void printrs (XtPointer client_data, XtIntervalId *id)
{
   String title;
   char *cmd, *dmpfile;

   XtVaGetValues (app_shell, XmNtitle, &title, NULL);
   strchr (title, '\'') && DIE;
   if (asprintf (&cmd, "xwd -silent -multi -id \'0x%x\' -out \'%s.dmp\' -db", (unsigned int)XtWindow (app_shell), title) == -1) exit (1);
   if (system (cmd)); free (cmd);
   if (asprintf (&cmd, "/usr/contrib/bin/X11/xpr -density 600 -device ljet -rv \'%s.dmp\' | lp -dlaserjet_rs -oraw -onb", title) == -1) exit (1);
   if (system (cmd)); free (cmd);
   if (asprintf (&dmpfile, "%s.dmp", title) == -1) exit (1);
   remove(dmpfile); free (dmpfile);
   makeWarningPop ("FYI", "\"Print LaserJet RS\" done");
}

typedef struct 
{
   char *filename;
   Widget w;
} PsData;

void savePs (XtPointer client_data, XtIntervalId *id)
{
   PsData *pd = (PsData *) client_data;
   char *filename = pd->filename;
   Widget w = pd->w;

   /*
   String title;
   char *cmd;

     XtVaGetValues (app_shell, XmNtitle, &title, NULL);
     strchr (title, '\'') && DIE;
     strchr (filename, '\'') && DIE;
     asprintf (&cmd,
     "PATH=/usr/contrib/bin:$PATH xwd -frame -silent -id \'0x%x\' | xpr -density 600 -device ps -output \'%s\'",
     (unsigned int)XtWindow (app_shell), filename);
     system(cmd); free (cmd);
   */

   makePSPop (w, filename, 0);

   //   XtFree (filename);
   //   makeWarningPop ("FYI", "\"Save as PS\" done");
}

void
ExposeCB (Widget w, XtPointer client_data, XtPointer call_data)
{
   update_cursor (0, 0, 3);
   XCopyArea (Current_display, WorkSpPixmap, WorkSpWind, WorkSpGC, 0, 0,
	      scopeScreen_x, scopeScreen_y, 0, 0);
}

void
SetWatchCursor (Widget widget)
{
   static Cursor watch_cursor;

   if (watch_cursor == 0)
      watch_cursor = XCreateFontCursor (XtDisplay(widget), XC_watch);
   XDefineCursor (XtDisplay (widget), XtWindow (widget), watch_cursor);
   XmUpdateDisplay(widget);
}



void
FileAcceptCB (Widget w, XtPointer client_data, XtPointer call_data)
{
   extern int hstotal;

   Arg args[10];					/***  arg         ***/
   Cardinal r;					     /***  arg count   ***/

   int i, strlength, difflength;
   float rWidth;
   char widthStr[30], newbuffStr[256];
   char *namelength, c = '/', *s2 = ".hdt";
   char newname[256];
   XmString fileString, newWidth;
   XmFileSelectionBoxCallbackStruct
      * fcb = (XmFileSelectionBoxCallbackStruct *) call_data;

   char *filename = NULL, *strptr;
   char strfull[256];

   if (fileLoaded == 1)
   {
      hstotal = 0;
      ClearHSLabels ();
      clearMessageBox ();
      for (i = 0; i < MAX_ANLG_CHNL; i++)
	 AnDiffWarn[i] = 0;
   }

   strcpy (strfull, "Filename: ");

   /* get the filename from the file selection box */
   XmStringGetLtoR (fcb->value, XmSTRING_DEFAULT_CHARSET, &filename);

   sprintf (textStr, "Filename : %s", filename);
   if (asprintf (&spike_filename, "%s", filename) == -1) exit (1);

   for (i = 0; i < 100; i++)
      newbuffStr[i] = '\0';

   namelength = strrchr (textStr, c);

   strlength = strlen (filename);

   strncpy (newname, filename, strlength - 4);
   newname[strlength - 4] = '\0';
   strcat (newname, s2);
   newname2 = newname;

   difflength = strlength - 4;
   strncpy (newbuffStr, namelength, difflength);
   strcat (newbuffStr, s2);

   strptr = strrchr (filename, (int) '/');
   strptr++;

   strcat (strfull, strptr);

   r = 0;
   fileString = XmStringCreateLtoR (strfull, XmSTRING_DEFAULT_CHARSET);
   XtSetArg (args[r], XmNlabelString, fileString); r++;
   XtSetValues (FilenameW, args, r);

   SetWatchCursor (main_window);
   SetWatchCursor (fileselect);
   {
      int ok;
      ok = ProcFile (filename);
      XUndefineCursor (display, XtWindow (main_window));
      XUndefineCursor (display, XtWindow (fileselect));
      if (!ok) {
	 XtUnmanageChild (fileselect);
	 return;
      }
   }
   remoteReset ();

   InitScope ();
   lastLeftTime = leftTime;
   totalTime = fileTime;

   if (screenWidth > fileTime)
   {
      screenWidth = round_width (fileTime);
      rightTime = leftTime + screenWidth;
      XtVaSetValues (horiz_bar, XmNpageIncrement, screenWidth, NULL);
      XtVaSetValues (horiz_bar, XmNincrement, screenWidth * 8 / DAS_WIDTH,
		     NULL);
      rWidth = (double)screenWidth / ticks_per_second;
      sprintf (widthStr, "ScreenWidth : %9.3f", rWidth);
      newWidth = XmStringCreateLtoR (widthStr, XmSTRING_DEFAULT_CHARSET);

      r = 0;
      XtSetArg (args[r], XmNlabelString, newWidth); r++;
      XtSetValues (ScreenWidthW, args, r);
   }
   else
   {
      sprintf (widthStr, "ScreenWidth : %9.3f", (double)screenWidth / ticks_per_second);
      newWidth = XmStringCreateLtoR (widthStr, XmSTRING_DEFAULT_CHARSET);

      r = 0;
      XtSetArg (args[r], XmNlabelString, newWidth); r++;
      XtSetValues (ScreenWidthW, args, r);
   }

   r = 0;
   XtSetArg(args[r], XmNmaximum, MAX (leftTime + screenWidth, firstTime + fileTime + 1)); r++;
   XtSetArg(args[r], XmNsliderSize, screenWidth); r++;
   XtSetArg(args[r], XmNminimum, firstTime); r++;
   XtSetArg(args[r], XmNvalue, leftTime); r++;
   XtSetValues(horiz_bar, args, r);

   XFillRectangle (Current_display, WorkSpPixmap, WorkSpBgGC, 0, 0,
		   scopeScreen_x, scopeScreen_y);
   refresh ();

   fileLoaded = 1;
   nextStat = 0;

   /***
       XtDestroyWidget(fileselect);
   ***/

   XtUnmanageChild (fileselect);

}

void
FileCancelCB (Widget w, XtPointer client_data, XtPointer call_data)
{
/***
   XtDestroyWidget(fileselect);
***/

  XtUnmanageChild (fileselect);
}

void
PopShellCB (Widget w, XtPointer client_data, XtPointer call_data)
{
  Arg args[6];
  Cardinal i;
  Widget UnusedW;

  if (fileLoaded != 1)
    {
      i = 0;
      XtSetArg (args[i], XmNpattern, XmStringCreate ("*.?dt", XmSTRING_DEFAULT_CHARSET)); i++;
      XtSetArg (args[i], XmNresizePolicy, XmRESIZE_GROW); i++;
      XtSetArg (args[i], XmNwidth, 300); i++;

      fileselect =
	XmCreateFileSelectionDialog (open_button, "fileselect", args, i);

      XtAddCallback (fileselect, XmNokCallback, FileAcceptCB, NULL);
      XtAddCallback (fileselect, XmNcancelCallback, FileCancelCB, NULL);

      UnusedW = XmFileSelectionBoxGetChild (fileselect, XmDIALOG_HELP_BUTTON);
      XtUnmanageChild (UnusedW);

      XtManageChild (fileselect);
    }
  else
    {
      i = 0;
      XtSetArg (args[i], XmNpattern, XmStringCreate ("*.?dt", XmSTRING_DEFAULT_CHARSET)); i++;
      XtSetArg (args[i], XmNlistUpdated, TRUE); i++;
      XtSetValues (fileselect, args, i);

      XtManageChild (fileselect);
    }
}



/*** Clean up temp files and exit ***/
void
LastCB (Widget w, XtPointer client_data, XtPointer call_data)
{
  exit (0);
}


void
EditCB (Widget w, XtPointer client_data, XtPointer call_data)
{
  makeFilterRoot ();
  FilterPopOpened = 1;
  /*  makeAutoOptPop(); */
}

Boolean show_tally;

void
OptionsCB (Widget w, XtPointer client_data, XtPointer call_data)
{
   show_tally ^= 1;
   ClearSideBar ();
   if (show_tally)
      showTally ();
}


void
ScaleCB (Widget w, XtPointer client_data, XtPointer call_data)
{
  makeScalePop ();
}


void
WriteCB (Widget w, XtPointer client_data, XtPointer call_data)
{
  makeWritePop ();
}


void
HistogramCB (Widget w, XtPointer client_data, XtPointer call_data)
{
   //   makeHistogramPop ();
}


void
IntegrateCB (Widget w, XtPointer client_data, XtPointer call_data)
{
   makeIntegratePop ();
}

void
RedrawCB (Widget w, XtPointer client_data, XtPointer call_data)
{
  if (fp == NULL)
    return;

  clearMessageBox ();
  XFillRectangle (Current_display, WorkSpPixmap, WorkSpBgGC, 0, 0,
		  scopeScreen_x, scopeScreen_y);
  refresh ();
  refreshBlocks ();
}


void
AboutCB (Widget w, XtPointer client_data, XtPointer call_data)
{
  makeAboutPop ();
}


void
PrintCB (Widget w, XtPointer client_data, XtPointer call_data)
{
   XtAppAddTimeOut (app_context, 1000, print, 0);
}


void
PrintRSCB (Widget w, XtPointer client_data, XtPointer call_data)
{
   XtAppAddTimeOut (app_context, 1000, printrs, 0);
}

void
PsFileAcceptCB (Widget w, XtPointer client_data, XtPointer call_data)
{
   static PsData pd;
   
  XmFileSelectionBoxCallbackStruct *fcb =
    (XmFileSelectionBoxCallbackStruct *) call_data;
  static char *filename = NULL;

  XmStringGetLtoR (fcb->value, XmSTRING_DEFAULT_CHARSET, &filename);
  XtUnmanageChild (psfileselect);
  pd.filename = filename;
  pd.w = w;
  XtAppAddTimeOut (app_context, 1, savePs, &pd);
}

void
PsFileCancelCB (Widget w, XtPointer client_data, XtPointer call_data)
{
  XtUnmanageChild (psfileselect);
}

void
SavePsCB (Widget w, XtPointer client_data, XtPointer call_data)
{
  Arg args[6];
  Cardinal i;
  Widget UnusedW;

  if (!psfileselect)
    {
      i = 0;
      XtSetArg (args[i], XmNpattern, XmStringCreate ("*.ps", XmSTRING_DEFAULT_CHARSET)); i++;
      XtSetArg (args[i], XmNresizePolicy, XmRESIZE_GROW); i++;
      XtSetArg (args[i], XmNwidth, 300); i++;

      psfileselect =
	XmCreateFileSelectionDialog (open_button, "psfileselect", args, i);

      XtAddCallback (psfileselect, XmNokCallback, PsFileAcceptCB, NULL);
      XtAddCallback (psfileselect, XmNcancelCallback, PsFileCancelCB, NULL);

      UnusedW =
	XmFileSelectionBoxGetChild (psfileselect, XmDIALOG_HELP_BUTTON);
      XtUnmanageChild (UnusedW);
    }

  XtManageChild (psfileselect);

}


/***
void TestCB()
{
   showTally();
}
***/

static Boolean in_window;

static void
leave_window (Widget w, XtPointer client_data, XEvent * event, Boolean * continu)
{
   if (!in_window)
      return;

   in_window = False;
   update_cursor (0, 0, 0);
}

static void
enter_window (Widget w, XtPointer client_data, XEvent * event, Boolean * continu)
{
   if (in_window)
      return;
   
   in_window = True;
  update_cursor (event->xcrossing.x, event->xcrossing.y, 1);
}

/*-------------------------------------------------------------
**	CreateApplication	- create main window
*/
Widget
CreateApplication (Widget parent)
{
   static Widget ChannelDispLabelW;
   static Widget ScrollFactorLabel;
   static Widget menu_bar;		/*  MenuBar          */
   static Widget seprtr1;
   static Widget seprtr2;
   static Widget cascade;			/*  CascadeButton    */
   static Widget cascade1;
   static Widget frame;			/*  Frame            */
   static Widget file_pane;		/*  Menu Pane           */
   static Widget edit_button;		/*  */
   static Widget edit_button1;
   static Widget quit_button;		/*  */
   static Widget FileSeparator;
   static Widget saveas_button;		/*  */
   static Widget print_button;
   static Widget print_cascade;
   static Widget print_laser;
   static Widget print_laser_rs;
   static Widget options_button;
   static Widget findEventW;
   static Widget edit_pane;		/*  Edit Pane           */
   static Widget manual_button;
   static Widget periodic_button;
   static Widget offset_button;
   static Widget options_pane;		/*  Option Pane         */
   static Widget ScrollRowCol2W;
   static Widget CursorPosW;
   static Widget ScreenLeftW;
   static Widget RedrawW;
   static Widget UpArrowW;
   static Widget DownArrowW;
   static Widget ChannelRowColW;
   static Widget UpArrow2W;
   static Widget DownArrow2W;
   static Widget UpArrow3W;
   static Widget DownArrow3W;
   static Widget help_pane;
   static Widget help_button;
   int x0 = 17;
   Arg args[20];			/*  arg list            */
   Cardinal n;			/*  arg count              */
   int i;
   int doslide = 1;
   int debugprint = 0;


   /*      Create MainWindow.      */
   n = 0;
   XtSetArg (args[n], XmNresizable, FALSE); n++;
   XtSetArg (args[n], XmNheight, WIN_HEIGHT); n++;
//   added, this was not here XtSetArg (args[n], XmNwidth, 1400); n++;
   XtSetArg (args[n], XmNwidth, WIN_WIDTH); n++;
   main_window = XmCreateMainWindow (parent, "main_window", args, n);
   XtManageChild (main_window);

   /*  Create MenuBar in MainWindow.    */
   n = 0;
   menu_bar = XmCreateMenuBar (main_window, "menu_bar", args, n);
   XtManageChild (menu_bar);

   /*  Create File Pane    */
   n = 0;
   file_pane = XmCreatePulldownMenu (menu_bar, "file_pane", args, n);

   /*  Create Edit Pane    */
   n = 0;
   edit_pane = XmCreatePulldownMenu (menu_bar, "edit_pane", args, n);

   /*  Create Option Pane  */
   n = 0;
   options_pane = XmCreatePulldownMenu (menu_bar, "options_pane", args, n);

   /***  Create Help Pane  */
   n = 0;
   help_pane = XmCreatePulldownMenu (menu_bar, "help_pane", args, n);

   /* Create CascadeButton File */
   n = 0;
   XtSetArg (args[n], XmNsubMenuId, file_pane); n++;
   cascade = XmCreateCascadeButton (menu_bar, "File", args, n);
   XtManageChild (cascade);

   /* Create CascadeButton Edit  */
   n = 0;
   XtSetArg (args[n], XmNsubMenuId, edit_pane); n++;
   cascade = XmCreateCascadeButton (menu_bar, "Edit", args, n);
   XtManageChild (cascade);

   /* Create CascadeButton Options */
   n = 0;
   XtSetArg (args[n], XmNsubMenuId, options_pane); n++;
   cascade = XmCreateCascadeButton (menu_bar, "Options", args, n);
   XtManageChild (cascade);

   /*** Create CascadeButton Help */
   n = 0;
   XtSetArg (args[n], XmNsubMenuId, help_pane); n++;
   cascade = XmCreateCascadeButton (menu_bar, "Help", args, n);
   XtManageChild (cascade);

   n = 0;
   XtSetArg (args[n], XmNmenuHelpWidget, cascade); n++;
   XtSetValues (menu_bar, args, n);

   /* Create Pushbutton Open  */
   n = 0;
   open_button = XmCreatePushButton (file_pane, "Open", args, n);
   XtManageChild (open_button);


   XtAddCallback (open_button, XmNactivateCallback, PopShellCB, NULL);

   /* create separator */
   n = 0;
   XtSetArg (args[n], XmNshadowThickness, 4), n++;
   FileSeparator = XmCreateSeparator (file_pane, "space", args, n);
   XtManageChild (FileSeparator);

   /* Create Save As..  PushButton  */
   n = 0;
   saveas_button = XmCreatePushButton (file_pane, "Save as PS", args, n);
   XtManageChild (saveas_button);
   XtAddCallback (saveas_button, XmNactivateCallback, SavePsCB, NULL);

   /* Create Print PushButton  */
   n = 0;
   print_button = XmCreatePulldownMenu (file_pane, "Print", args, n);

   n = 0;
   XtSetArg (args[n], XmNsubMenuId, print_button); n++;
   print_cascade = XmCreateCascadeButton (file_pane, "Print", args, n);
   XtManageChild (print_cascade);

   print_laser = XmCreatePushButton (print_button, "LaserJet", args, n);
   XtManageChild (print_laser);
   XtAddCallback (print_laser, XmNactivateCallback, PrintCB, NULL);

   print_laser_rs = XmCreatePushButton (print_button, "LaserJet RS", args, n);
   XtManageChild (print_laser_rs);
   XtAddCallback (print_laser_rs, XmNactivateCallback, PrintRSCB, NULL);

   /* Create Quit Pushbutton  */
   n = 0;
   quit_button = XmCreatePushButton (file_pane, "Quit", args, n);
   XtManageChild (quit_button);
   XtAddCallback (quit_button, XmNactivateCallback, LastCB, NULL);

   /* Create Add option in Edit menu  */
   n = 0;
   edit_button1 = XmCreatePulldownMenu (edit_pane, "Add a Code", args, n);

   n = 0;
   XtSetArg (args[n], XmNsubMenuId, edit_button1); n++;
   cascade1 = XmCreateCascadeButton (edit_pane, "Add a code", args, n);
   XtManageChild (cascade1);

   manual_button = XmCreatePushButton (edit_button1, "Manual", args, n);
   XtManageChild (manual_button);
   XtAddCallback (manual_button, XmNactivateCallback, manual, manual_button);

   periodic_button = XmCreatePushButton (edit_button1, "Periodic", args, n);
   XtManageChild (periodic_button);
   XtAddCallback (periodic_button, XmNactivateCallback, makecycPop, NULL);

   offset_button = XmCreatePushButton (edit_button1, "Offset", args, n);
   XtManageChild (offset_button);
   XtAddCallback (offset_button, XmNactivateCallback, offset, NULL);

   /* Create Filter option in Edit menu  */
   n = 0;
   edit_button = XmCreatePushButton (edit_pane, "Set Filters", args, n);
   XtManageChild (edit_button);
   XtAddCallback (edit_button, XmNactivateCallback, EditCB, NULL);

   /* Create Delete option in Edit menu  */
   n = 0;
   edit_button = XmCreatePushButton (edit_pane, "Write", args, n);
   XtManageChild (edit_button);
   XtAddCallback (edit_button, XmNactivateCallback, WriteCB, NULL);

   /* Create Scale Button in Option menu  */
   n = 0;
   options_button = XmCreatePushButton (options_pane, "Scale", args, n);
   XtManageChild (options_button);
   XtAddCallback (options_button, XmNactivateCallback, ScaleCB, NULL);

   /*  Create Tally PushButton in Option menu  */
   n = 0;
   options_button = XmCreatePushButton (options_pane, "Tally", args, n);
   XtManageChild (options_button);
   XtAddCallback (options_button, XmNactivateCallback, OptionsCB, NULL);

   /*  Create Histogram PushButton in Option menu  */
   n = 0;
   options_button = XmCreatePushButton (options_pane, "Histogram", args, n);
   XtManageChild (options_button);
   XtAddCallback (options_button, XmNactivateCallback, makeHistogramPop, NULL);

   n = 0;
   options_button = XmCreatePushButton (options_pane, "Integrate", args, n);
   XtManageChild (options_button);
   XtAddCallback (options_button, XmNactivateCallback, IntegrateCB, NULL);

   n = 0;
   options_button = XmCreatePushButton (options_pane, "Bandpass", args, n);
   XtManageChild (options_button);
   XtAddCallback (options_button, XmNactivateCallback, makeBandpassPop, NULL);

   /*  Create Find Pushbutton in Option menu  */
   n = 0;
   findEventW = XmCreatePushButton (options_pane, "Find..", args, n);
   XtManageChild (findEventW);
   XtAddCallback (findEventW, XmNactivateCallback, FindCB, NULL);

   n = 0;
   options_button = XmCreatePushButton (options_pane, "Left Ticks", args, n);
   XtManageChild (options_button);
   XtAddCallback (options_button, XmNactivateCallback, makeLeftPop, NULL);

   /*  Create About Pushbutton in Help menu  */
   n = 0;
   help_button = XmCreatePushButton (help_pane, "About", args, n);
   XtManageChild (help_button);
   XtAddCallback (help_button, XmNactivateCallback, AboutCB, NULL);

   /***  Create test Pushbutton in Help menu  */
   /***
   {
      Widget test_button;

      n=0;
      test_button = XmCreatePushButton (help_pane, "Test", args, n);
      XtManageChild(test_button);
      XtAddCallback(test_button, XmNactivateCallback, TestCB, NULL);
   }
   ***/
   /*  Create Frame in MainWindow and ScrolledWindow in Frame. */
   n = 0;
   XtSetArg (args[n], XmNresizable, FALSE); n++;
   frame = (Widget) XmCreateFrame (main_window, "frame", args, n);
   XtManageChild (frame);

   /* added by girish */
   n = 0;
   XtSetArg (args[n], XmNmarginWidth, 3); n++;
   XtSetArg (args[n], XmNmarginHeight, 3); n++;
   XtSetArg (args[n], XmNresizable, FALSE); n++;
   form = (Widget) XmCreateForm (frame, "form ", args, n);
   XtManageChild (form);

   n = 0;
   XtSetArg (args[n], XmNheight, WIN_HEIGHT-100); n++;
   XtSetArg (args[n], XmNx, DAS_WIDTH + 58); n++;
   XtSetArg (args[n], XmNorientation, XmVERTICAL); n++;
   XtSetArg (args[n], XmNtopAttachment, XmATTACH_FORM); n++;
   XtSetArg (args[n], XmNseparatorType, XmNO_LINE); n++;
   seprtr1 = (Widget) XmCreateSeparator (form, "seprtr", args, n);
   XtManageChild (seprtr1);

   n = 0;
   XtSetArg (args[n], XmNorientation, XmHORIZONTAL); n++;
   XtSetArg (args[n], XmNtopWidget, seprtr1); n++;
   XtSetArg (args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
   XtSetArg (args[n], XmNleftAttachment, XmATTACH_FORM); n++;
   XtSetArg (args[n], XmNrightAttachment, XmATTACH_FORM); n++;
   XtSetArg (args[n], XmNseparatorType, XmNO_LINE); n++;
   seprtr2 = (Widget) XmCreateSeparator (form, "seprtr", args, n);
   XtManageChild (seprtr2);

   XmCreatePulldownMenu (menu_bar, "pulldown", args, 0);


   /* frame = form; */

   /* Filename ID */
   n = 0;
   XtSetArg (args[n], XmNmarginLeft, 150); n++;
   XtSetArg (args[n], XmNwidth, 375); n++;
   XtSetArg (args[n], XmNrecomputeSize, False); n++;
   FilenameW = XmCreateCascadeButton (menu_bar, "Filename : ", args, n);
   XtManageChild (FilenameW);

   /* Message Box */
   n = 0;
   XtSetArg (args[n], XmNwidth, 315); n++;
   XtSetArg (args[n], XmNrecomputeSize, False); n++;
   MessageBoxW = XmCreateCascadeButton (menu_bar, "", args, n);
   XtManageChild (MessageBoxW);

   /* Side Bar Label */
   n = 0;
   XtSetArg (args[n], XmNwidth, 100); n++;
   XtSetArg (args[n], XmNalignment, XmALIGNMENT_END); n++;
   XtSetArg (args[n], XmNrecomputeSize, False); n++;
   SideBarLabel = XmCreateCascadeButton (menu_bar, "", args, n);
   XtManageChild (SideBarLabel);

   n = 0;
   XtSetArg (args[n], XmNy, WIN_HEIGHT-90); n++;
   XtSetArg (args[n], XmNx, 650); n++;
   ScreenWidthW = XmCreateLabel (form, "Screen Width: 25.000", args, n);
   /*    ScreenWidthW = XmCreateLabel(form, "Screen Width: .05", args, n); */
   XtManageChild (ScreenWidthW);


   n = 0;
   XtSetArg (args[n], XmNy, WIN_HEIGHT-90); n++;
   XtSetArg (args[n], XmNx, 360); n++;
   CursorPosW = XmCreateLabel (form, "Cursor Position:", args, n);
   XtManageChild (CursorPosW);

   n = 0;
   XtSetArg (args[n], XmNy, WIN_HEIGHT-90); n++;
   XtSetArg (args[n], XmNleftWidget, CursorPosW); n++;
   XtSetArg (args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
   CursorPos2W = XmCreateLabel (form, "   000.000  ", args, n);
   XtManageChild (CursorPos2W);

   n = 0;
   XtSetArg (args[n], XmNy, WIN_HEIGHT-90); n++;
   XtSetArg (args[n], XmNx, 100); n++;
   ScreenLeftW = XmCreateLabel (form, "Screen Left:", args, n);
   XtManageChild (ScreenLeftW);

   n = 0;
   XtSetArg (args[n], XmNy, WIN_HEIGHT-90); n++;
   XtSetArg (args[n], XmNleftWidget, ScreenLeftW); n++;
   XtSetArg (args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
   ScreenLeft2W = XmCreateLabel (form, "  000.000 ", args, n);
   XtManageChild (ScreenLeft2W);

   n = 0;
   XtSetArg (args[n], XmNtopOffset, 20); n++;
   XtSetArg (args[n], XmNy, WIN_HEIGHT-66); n++;
   XtSetArg (args[n], XmNx, 100); n++;
   DigitalChanW = XmCreateLabel (form, "Digital Channel Total:", args, n);
   XtManageChild (DigitalChanW);

   n = 0;
   XtSetArg (args[n], XmNy, WIN_HEIGHT-66); n++;
   XtSetArg (args[n], XmNx, 360); n++;
   AnalogChanW = XmCreateLabel (form, "Analog Channel Total: ", args, n);
   XtManageChild (AnalogChanW);

   n = 0;
   XtSetArg (args[n], XmNy, WIN_HEIGHT-66); n++;
   XtSetArg (args[n], XmNx, 650); n++;
   HSDChanW = XmCreateLabel (form, "HSD Channel Total: ", args, n);
   XtManageChild (HSDChanW);


   /* Set up a bulletin board widget to hold signal Id buttons */
   /* Arguments for bulletin board widget */
   n = 0;
   XtSetArg (args[n], XmNchildren, SignalLabelW); n++;
   XtSetArg (args[n], XmNchildren, SignalLabel2W); n++;
   /*      XtSetArg(args[n], XmNwidth, 120); n++;  */
   XtSetArg (args[n], XmNtopAttachment, XmATTACH_FORM); n++;
   XtSetArg (args[n], XmNleftAttachment, XmATTACH_FORM); n++;
   /***
       XtSetArg(args[n], XmNmarginHeight, 1); n++;
       XtSetArg(args[n], XmNmarginWidth, 1); n++;
   */
   XtSetArg (args[n], XmNmarginHeight, 0); n++;
   XtSetArg (args[n], XmNmarginWidth, 0); n++;
   XtSetArg (args[n], XmNborderWidth, 0); n++;
   XtSetArg (args[n], XmNnoResize, True); n++;
   bullBoardW = XmCreateBulletinBoard (form, "bulletin", args, n);
   XtManageChild (bullBoardW);

   createDummyLabel ();

   n = 0;
   /* XtSetArg(args[n], XmNx, 910); n++; */
   XtSetArg (args[n], XmNtopAttachment, XmATTACH_FORM); n++;
   XtSetArg (args[n], XmNrightAttachment, XmATTACH_FORM); n++;
   XtSetArg (args[n], XmNleftWidget, seprtr1); n++;
   XtSetArg (args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
   XtSetArg (args[n], XmNbottomWidget, seprtr2); n++;
   XtSetArg (args[n], XmNbottomAttachment, XmATTACH_WIDGET); n++;
   XtSetArg (args[n], XmNmarginHeight, 1); n++;
   XtSetArg (args[n], XmNmarginWidth, 1); n++;
   XtSetArg (args[n], XmNnoResize, True); n++;
   bullBoard2W = XmCreateBulletinBoard (form, "bulletin", args, n);
   XtManageChild (bullBoard2W);

   n = 0;
   XtSetArg (args[n], XmNy, 50); n++;
   UpArrowW = XmCreateArrowButton (bullBoard2W, "Up Arrow", args, n);
   XtManageChild (UpArrowW);
   XtAddCallback (UpArrowW, XmNactivateCallback, UpArrow, NULL);

   n = 0;
   XtSetArg (args[n], XmNy, 50); n++;
   XtSetArg (args[n], XmNx, x0 + 99); n++;
   UpArrow3W = XmCreateArrowButton (bullBoard2W, "upArrow", args, n);
   XtManageChild (UpArrow3W);
   XtAddCallback (UpArrow3W, XmNactivateCallback, ScrollUp, NULL);

   n = 0;
   XtSetArg (args[n], XmNy, 75); n++;
   XtSetArg (args[n], XmNx, x0 + 99); n++;
   XtSetArg (args[n], XmNwidth, 20); n++;
   XtSetArg (args[n], XmNheight, 20); n++;
   ScrollRowCol2W = XmCreateRowColumn (bullBoard2W, "rowcol", args, n);
   XtManageChild (ScrollRowCol2W);

   n = 0;
   ScrollBoxW = XmCreateLabel (ScrollRowCol2W, "10", args, n);
   XtManageChild (ScrollBoxW);

   n = 0;
   XtSetArg (args[n], XmNy, 105); n++;
   XtSetArg (args[n], XmNx, x0 + 99); n++;
   XtSetArg (args[n], XmNarrowDirection, XmARROW_DOWN); n++;
   DownArrow3W = XmCreateArrowButton (bullBoard2W, "downArrow", args, n);
   XtManageChild (DownArrow3W);
   XtAddCallback (DownArrow3W, XmNactivateCallback, ScrollDown, NULL);

   n = 0;
   XtSetArg (args[n], XmNy, 48); n++;
   XtSetArg (args[n], XmNx, x0 + 75); n++;
   {
      XmString label_str =
	 XmStringCreateLtoR ("S F\nc a\nr c\no t\nl o\nl r",
			     XmSTRING_DEFAULT_CHARSET);
      XtSetArg (args[n], XmNlabelString, label_str); n++;
      ScrollFactorLabel =
	 XmCreateLabel (bullBoard2W, "S F\nc a\nr c\no t\nl o\nl r", args, n);
      XmStringFree (label_str);
   }
   XtManageChild (ScrollFactorLabel);

   n = 0;
   XtSetArg (args[n], XmNy, 271); n++;					   /*** was 260 */
   {
      XmString label_str =
	 XmStringCreateLtoR (" R \n e \n d \n r \n a \n w ",
			     XmSTRING_DEFAULT_CHARSET);
      XtSetArg (args[n], XmNlabelString, label_str); n++;
      RedrawW =
	 XmCreatePushButton (bullBoard2W, " R \n e \n d \n r \n a \n w ", args,
			     n);
      XmStringFree (label_str);
   }
   XtAddCallback (RedrawW, XmNactivateCallback, RedrawCB, NULL);	      /*** girish used 30 hs2 used NULL */
   XtManageChild (RedrawW);	/* 30 for actual redraw button */


   n = 0;
   XtSetArg (args[n], XmNy, 270); n++;
   XtSetArg (args[n], XmNx, x0 + 99); n++;
   XtSetArg (args[n], XmNinitialDelay, 100); n++;
   XtSetArg (args[n], XmNrepeatDelay, 33); n++;
   UpArrow2W = XmCreateArrowButton (bullBoard2W, "Up Arrow", args, n);
   XtManageChild (UpArrow2W);
   XtAddCallback (UpArrow2W, XmNactivateCallback, ChannelUp, NULL);

   n = 0;
   XtSetArg (args[n], XmNy, 303); n++;					   /*** was 300 */
   XtSetArg (args[n], XmNx, x0 + 99); n++;
   XtSetArg (args[n], XmNheight, 20); n++;
   XtSetArg (args[n], XmNwidth, 20); n++;
   XtSetArg (args[n], XmNborderWidth, 1); n++;
   ChannelRowColW = XmCreateRowColumn (bullBoard2W, "rowcol", args, n);
   XtManageChild (ChannelRowColW);

   n = 0;
   ChannelBoxW = XmCreateLabel (ChannelRowColW, "30", args, n);
   /* 	ChannelBoxW=XmCreateLabel(ChannelRowColW,"1",args, n); */
   XtManageChild (ChannelBoxW);

   n = 0;
   XtSetArg (args[n], XmNy, 339); n++;					   /*** was 340 */
   XtSetArg (args[n], XmNx, x0 + 99); n++;
   XtSetArg (args[n], XmNinitialDelay, 100); n++;
   XtSetArg (args[n], XmNrepeatDelay, 33); n++;
   XtSetArg (args[n], XmNarrowDirection, XmARROW_DOWN); n++;
   DownArrow2W = XmCreateArrowButton (bullBoard2W, "Down Arrow", args, n);
   XtManageChild (DownArrow2W);
   XtAddCallback (DownArrow2W, XmNactivateCallback, ChannelDown, NULL);

   n = 0;
   XtSetArg (args[n], XmNy, 560); n++;
   XtSetArg (args[n], XmNarrowDirection, XmARROW_DOWN); n++;
   DownArrowW = XmCreateArrowButton (bullBoard2W, "Down Arrow", args, n);
   XtManageChild (DownArrowW);
   XtAddCallback (DownArrowW, XmNactivateCallback, DownArrow, NULL);

   n = 0;
   XtSetArg (args[n], XmNy, 255); n++;
   XtSetArg (args[n], XmNx, x0 + 75); n++;
   {
      XmString label_str =
	 XmStringCreateLtoR ("# D\nC i\nh s\na p\nn l\nn a\ne y\nl e\ns d",
			     XmSTRING_DEFAULT_CHARSET);
      XtSetArg (args[n], XmNlabelString, label_str); n++;
      ChannelDispLabelW =
	 XmCreateLabel (bullBoard2W,
			"# D\nC i\nh s\na p\nn l\nn a\ne y\nl e\ns d", args, n);
      XmStringFree (label_str);
   }
   XtManageChild (ChannelDispLabelW);

   n = 0;

   XtSetArg (args[n], XmNrightWidget, seprtr1); n++;
   XtSetArg (args[n], XmNrightAttachment, XmATTACH_WIDGET); n++;
   XtSetArg (args[n], XmNleftWidget, bullBoardW); n++;
   XtSetArg (args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
   XtSetArg (args[n], XmNtopAttachment, XmATTACH_FORM); n++;
   XtSetArg (args[n], XmNwidth, DAS_WIDTH); n++;						      /*** added by pb */
   XtSetArg (args[n], XmNresizable, FALSE); n++;
   DrawingArea = XmCreateDrawingArea (form, "drawing area", args, n);
   XtManageChild (DrawingArea);

   n = 0;
   XtSetArg (args[n], XmNheight, 20); n++;
   XtSetArg (args[n], XmNrightWidget, seprtr1); n++;
   XtSetArg (args[n], XmNrightAttachment, XmATTACH_WIDGET); n++;
   XtSetArg (args[n], XmNbottomWidget, seprtr2); n++;
   XtSetArg (args[n], XmNbottomAttachment, XmATTACH_WIDGET); n++;
   XtSetArg (args[n], XmNleftAttachment, XmATTACH_FORM); n++;
   XtSetArg (args[n], XmNinitialDelay, 100); n++;						       /*** was 1 -- pb */
   XtSetArg (args[n], XmNrepeatDelay, 33); n++;						      /*** was 1 by girish -- hs 2 */
   XtSetArg (args[n], XmNorientation, XmHORIZONTAL); n++;
   XtSetArg (args[n], XmNresizable, FALSE); n++;
   XtSetArg (args[n], XmNscrollingPolicy, XmAUTOMATIC); n++;
   horiz_bar = XmCreateScrollBar (form, "horiz_bar", args, n); /*** was swindow */
   XtManageChild (horiz_bar);
   XtSetSensitive (horiz_bar, TRUE);/*** -- girish */

   /*** girish */

   n = 0;
   XtSetArg (args[n], XmNbottomWidget, horiz_bar); n++;
   XtSetArg (args[n], XmNbottomAttachment, XmATTACH_WIDGET); n++;
   XtSetValues (bullBoardW, args, n);

   n = 0;
   XtSetArg (args[n], XmNbottomWidget, horiz_bar); n++;
   XtSetArg (args[n], XmNbottomAttachment, XmATTACH_WIDGET); n++;
   XtSetValues (DrawingArea, args, n);

   XtAddCallback (horiz_bar, XmNvalueChangedCallback, scrollselect, NULL);
   XtAddCallback (horiz_bar, XmNdragCallback, newslide, NULL);
   XtAddCallback (horiz_bar, XmNpageIncrementCallback, scrollselect, NULL);
   XtAddCallback (horiz_bar, XmNpageDecrementCallback, scrollselect, NULL);

   if (doslide)
   {
      //XtAddCallback (horiz_bar, XmNdecrementCallback, slidePlus, NULL);
      //     XtAddCallback (horiz_bar, XmNincrementCallback, slidePlus, NULL); 
      XtAddCallback (horiz_bar, XmNdecrementCallback, newslide, NULL);
      XtAddCallback (horiz_bar, XmNincrementCallback, newslide, NULL);
      XtAddCallback (horiz_bar, XmNdecrementCallback, slideBlocks, NULL);
      XtAddCallback (horiz_bar, XmNincrementCallback, slideBlocks, NULL);
   }
   else
   {
      XtAddCallback (horiz_bar, XmNdecrementCallback, scrollselect, NULL);
      XtAddCallback (horiz_bar, XmNincrementCallback, scrollselect, NULL);
   }


   screenWidth = round_width (50000);
   /* 	screenWidth = round_width (100); */
   /* 	XtVaSetValues(horiz_bar, XmNsliderSize, screenWidth, NULL); */
   XtVaSetValues (horiz_bar, XmNpageIncrement, screenWidth, NULL);
   XtVaSetValues (horiz_bar, XmNincrement, screenWidth * 8 / DAS_WIDTH, NULL);

   if (debugprint)
   {
      int val, width, pi, max, min;
      XtVaGetValues (horiz_bar, XmNvalue, &val, NULL);
      XtVaGetValues (horiz_bar, XmNmaximum, &max, NULL);
      XtVaGetValues (horiz_bar, XmNminimum, &min, NULL);
      XtVaGetValues (horiz_bar, XmNsliderSize, &width, NULL);
      XtVaGetValues (horiz_bar, XmNpageIncrement, &pi, NULL);
      printf ("%s line %d: %d %d %d\n", __FILE__, __LINE__, val, width, pi);
      printf ("%s line %d: %d %d %d %d %d\n", __FILE__, __LINE__, val, width,
	      pi, min, max);

   }
   temppos = 0;
   fpointer = 0;
   hist_bintotal = 400;
   l = 0;
   m = 0;
   channelcount = 30;
   /* 	channelcount = 1; */
   scrollMargin = 8;
   for (i = 0; i < MAX_ANLG_CHNL; i++)
      AnDiffWarn[i] = 0;

   XtAddEventHandler (DrawingArea, ButtonPressMask, False, motion, 0);
   XtAddEventHandler (DrawingArea, ButtonPressMask, False, marker, 0);
   XtAddEventHandler (DrawingArea, PointerMotionMask, False, mouse, 0);
   XtAddEventHandler (DrawingArea, PointerMotionMask, False, newPos, 0);
   XtAddEventHandler (DrawingArea, LeaveWindowMask, False, leave_window, 0);
   XtAddEventHandler (DrawingArea, EnterWindowMask, False, enter_window, 0);

   return (main_window);
}

XColor black;
Pixmap palette_xpm[PALETTE_COUNT];

static void
no_cursor (Widget w)
{
  static char bm_null[] = { 0,0,0,0, 0,0,0,0 };
  Display *dpy = XtDisplay (w);
  Window win = XtWindow (w);

  Pixmap bm_off = XCreateBitmapFromData (dpy, win, bm_null, 8,8);
  Cursor ptr_off = XCreatePixmapCursor (dpy, bm_off, bm_off, &black, &black, 0, 0);
  XFreePixmap(dpy, bm_off);
  XDefineCursor(dpy, win, ptr_off);
}

static void
make_palettes_gray (void)
{
   int palette, n;
   for (palette = 0; palette < PALETTE_COUNT; palette++)
      for (n = 0; n < 256; n++) {
         unsigned short value = (n & 0xfe) * 65535 / 254;
         color[palette][n].red   = value;
         color[palette][n].green = value;
         color[palette][n].blue  = value;
      }
}

Colormap cmap;

int
main (int argc, char **argv)
{
   XColor yellow, red, white;
   int screen;
   XSetWindowAttributes *attributes;
   unsigned long valuemask;
   XGCValues *values;
   unsigned long valuemask_create;
   Arg args[5];
   Cardinal n;
   Dimension height;
   static String resource_list[] = {
      "Scope*background: white",
      "Scope*fontList: -Misc-Fixed-Medium-R-SemiCondensed--13-120-75-75-C-60*",
      "Scope*foreground: black",
      "Scope*min_bpm.value: 5",
      "Scope*max_bpm.value: 60",
      "Scope*tpb.value: 1",
      0
   };

   /*      Initialize toolkit and open the display. */
   if (argc > 2)
   {
       // if width is not evenly divisible by 8, inc until it is.
      char *ptr;
      WIN_HEIGHT = strtol(argv[1],&ptr,10);
      WIN_WIDTH = strtol(argv[2],&ptr,10);
      if (WIN_HEIGHT > 4010) // really huge numbers crashes X
         WIN_HEIGHT = 4010;
      if (WIN_WIDTH > 4010)
         WIN_WIDTH = 4010;
      while ((WIN_WIDTH-210) % 8 != 0) // scrollMargin == 8
         ++WIN_WIDTH;
      DAS_WIDTH = WIN_WIDTH-210;
   }
   else
   {
      WIN_HEIGHT = 750; // values for previous versions of scope
      WIN_WIDTH = 1010;
      DAS_WIDTH = 800;
   }

   app_shell = XtAppInitialize (&app_context, "Scope", 0, 0, &argc, argv, resource_list, 0, 0);
   cmap = XCopyColormapAndFree (XtDisplay (app_shell), DefaultColormapOfScreen (XtScreen (app_shell))) ;
   XtVaSetValues (app_shell, XmNcolormap, cmap, NULL);
   

   XtAddEventHandler (app_shell, (EventMask) 0, True,
		      (XtEventHandler) _XEditResCheckMessages, 0);

   display = XtDisplay (app_shell);

   main_window = CreateApplication (app_shell);
   XtRealizeWidget (app_shell);
   XtVaGetValues (DrawingArea, XmNheight, &height, NULL);
   scopeScreen_y = height;

   /*** girish added */
   n = 0;
//   XtSetArg (args[n], XmNgeometry, "1010x750"); n++;
//   XtSetArg (args[n], XmNgeometry, "1010x750"); n++;
   /*** Resize?   -- was FALSE */
   XtSetArg (args[n], XmNresizable, TRUE); n++;
   XtSetValues (app_shell, args, n);
   /*** end girish */

   /* Initialize Edit popup widget */
   makeFilterTree ();

   WorkSpWind = XtWindow (DrawingArea);
   Current_display = XtDisplay (DrawingArea);

   /*
   Cursor crosshair_cursor = XCreateFontCursor (Current_display, XC_crosshair);
   XDefineCursor(Current_display, WorkSpWind, crosshair_cursor);
   XmUpdateDisplay (DrawingArea);
   */   

   {
      typedef struct
      {
	 Pixel backgroundPixel;
      } ApplicationData, *ApplicationDataPtr;
      static XtResource resources[] = {
	 {XtNbackground, XtCBackground, XtRPixel, sizeof (Pixel),
	  XtOffset (ApplicationDataPtr, backgroundPixel), XtRString,
	  XtDefaultBackground}
      };
      ApplicationData appData;
      XWindowAttributes wa;
      XtResourceList r;
      Cardinal rescnt;

      XtGetResourceList (XtClass (app_shell), &r, &rescnt);
      XtGetApplicationResources (app_shell, &appData, resources,
				 XtNumber (resources), 0, 0);
      XGetWindowAttributes (Current_display, WorkSpWind, &wa);
      WorkSpPixmap =
	 XCreatePixmap (Current_display, WorkSpWind, wa.width, wa.height,
			wa.depth);
      WorkSpBgGC = XCreateGC (Current_display, WorkSpWind, NIL, NULL);
      XSetForeground (Current_display, WorkSpBgGC, appData.backgroundPixel);
      XFillRectangle (Current_display, WorkSpPixmap, WorkSpBgGC, 0, 0,
		      scopeScreen_x, scopeScreen_y);

      screen = XDefaultScreen (Current_display);
      {
         int palette, n;

         WorkSpGC = XCreateGC (Current_display, WorkSpWind, NIL, NULL);
         if (DefaultDepth (Current_display, screen) == 8)
            make_palettes_gray ();
         for (palette = 0; palette < PALETTE_COUNT; palette++) {
            for (n = 0; n < 256; n++)
               XAllocColor (Current_display, cmap, &color[palette][n]);
            palette_xpm[palette] = XCreatePixmap (Current_display, WorkSpWind, 128, 15, wa.depth);
            for (n = 0; n < 127; n++) {
               XSetForeground (Current_display, WorkSpGC, color[palette][n * 2].pixel);
               XDrawLine (Current_display, palette_xpm[palette], WorkSpGC, n, 0, n, 15);
            }
         }
      }
   }
   red.red = 65535;
   red.green = 65535;
   red.blue = 0;
   XAllocColor (Current_display, cmap, &red);
   yellow.red = 65535;
   yellow.green = 65535;
   yellow.blue = 0;
   XAllocColor (Current_display, cmap, &yellow);
   white.red = 65535;
   white.green = 65535;
   white.blue = 65535;
   XAllocColor (Current_display, cmap, &white);
   black.red = 0;
   black.green = 0;
   black.blue = 0;
   XAllocColor (Current_display, cmap, &black);
   no_cursor (DrawingArea);
   
   XSetForeground (Current_display, WorkSpGC, black.pixel);
   XSetBackground (Current_display, WorkSpGC, red.pixel);
   XSetLineAttributes (Current_display, WorkSpGC, 1, LineSolid, CapRound,
		       JoinRound);
   values = (XGCValues *) malloc (sizeof (XGCValues));
   values->function = GXinvert;
   valuemask_create = GCFunction;
   InvertGC =
      XCreateGC (Current_display, WorkSpWind, valuemask_create, values);
   XSync (Current_display, 0);

   /* Set Backing Store attribute of the wokspace window to when mapped */

   attributes =
      (XSetWindowAttributes *) malloc (sizeof (XSetWindowAttributes));
   valuemask = CWBackingStore;
   attributes->backing_store = WhenMapped;
   XChangeWindowAttributes (Current_display, WorkSpWind, valuemask,
			    attributes);
   free (attributes);
   /*   Get and dispatch events.        */

   leftTime = 0;
   fileLoaded = 0;
   scaleFactor = 1;
   ticks_per_second = 2000 * scaleFactor;
   currFileType = 0;
   XtAddCallback (DrawingArea, XmNexposeCallback, ExposeCB, NULL);
   XtAppMainLoop (app_context);
   return 0;
}


/* Local Variables: */
/* c-basic-offset: 3 */
/* c-file-offsets:  ((substatement-open . 0)) */
/* End: */
