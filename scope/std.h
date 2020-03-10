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


#define _ISOC99_SOURCE 1
#define _SVID_SOURCE 1
#define _GNU_SOURCE 1

#if HAVE_CONFIG_H
#  include <config.h>
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include <X11/Intrinsic.h>
#include <X11/Shell.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <Xm/Xm.h>
#include <Xm/MainW.h>
#include <Xm/DialogS.h>
#include <Xm/ArrowB.h> 
#include <Xm/BulletinB.h>
#include <Xm/PushB.h>
#include <Xm/PushBG.h> 
#include <Xm/RowColumn.h>
#include <Xm/CascadeB.h>
#include <Xm/CascadeBG.h>
#include <Xm/ScrolledW.h>
#include <Xm/DrawingA.h>
#include <Xm/ScrollBar.h>
#include <Xm/ScrolledW.h>
#include <Xm/Separator.h>
#include <Xm/Frame.h>
#include <Xm/MessageB.h>
#include <Xm/FileSB.h>
#include <Xm/Text.h>
#include <Xm/Label.h>
#include <Xm/ToggleB.h>

#include "defines.h"

#define RESET 0
#define SET 1

#define MAX_HDT_CHNL 24         /* analog ID's can range from 1 to 23 inclusive */
#define MAX_BLOCKS 5000

extern int WIN_HEIGHT;
extern int WIN_WIDTH;
extern int DAS_WIDTH;

#define DDT_FACTOR 5
#define BYTES_PER_SAMPLE 2

#undef MAX
#undef MIN
#define MAX(x,y)  (((x)>(y))?(x):(y))
#define MIN(x,y)  (((x)<(y))?(x):(y))

#include <errno.h>
#define DIE (fprintf (stderr, "fatal error in %s at line %d \n", __FILE__, __LINE__), errno ? (perror ("last system error"), 0) : 0, exit (1), 0)

#define SWAB(x) if (*(char *) &swaptest) x = ((((x) >> 8) & 0xff) | (((x) & 0xff) << 8))

#ifdef P_tmpdir
#undef P_tmpdir
#endif
#define P_tmpdir "/home/tmp"

extern void makeWarningPop(char *, char *);

extern int scaleFactor, ticks_per_second; /* ticks_per_second = 2000 * scaleFactor */
extern int hdt_block_count;
extern int hdtblkstart[MAX_BLOCKS], hdtblkend[MAX_BLOCKS];

static inline long long
halfms2samp (int halfms)
{
   extern float samplerate;
   long long tmp = (long long)halfms * (long long)samplerate;
   long long quot = tmp / 2000;
   int rem = tmp % 2000;

   return quot + (rem != 0);
}

#if !HAVE_ASPRINTF && HAVE_CONFIG_H
int asprintf(char **buffer, char *fmt, ...);
#endif

#if !defined(_LFS64_LARGEFILE) || !defined(_LFS64_STDIO)
#    define fopen64 fopen
#    define tmpfile64 tmpfile
#    define fseeko64 fseeko
#    define ftello64 ftello
#endif

extern long long startbyte[MAX_BLOCKS+1];
extern Widget app_shell;		/* ApplicationShell     */
extern Widget main_window;		/* MainWindow           */
extern Widget MessageBoxW;		/* Message Box */
extern Widget SideBarLabel;		/* Side Box Label */
extern Widget HSDChanW;		/* Message Box */
extern Widget bullBoard2W;		/*  BulletinBoard       */
extern Widget horiz_bar;		/*  Horizontal Scrollbar */
extern Widget ScreenLeft2W;
extern Widget DigitalChanW;
extern Widget AnalogChanW;
extern Widget SignalLabel2W[MAX_HDT_CHNL];
extern Display *Current_display;
extern GC WorkSpGC;
extern GC WorkSpBgGC;
extern GC InvertGC;
extern Window WorkSpWind;
extern Pixmap WorkSpPixmap;
extern int sliderpos;
extern int state2;
extern char textStr[256];
extern int temppos;
extern char *newname2;
extern int totalTime;
extern int l;                   /* index in sf.ids of the first displayed channel */
extern int m;                   /* the number of pixels scrolled off the top of the display */
extern int channelcount;
extern Boolean FilterPopOpened;
extern Boolean MessageBoxUsed;
extern Boolean AnDiffWarn[MAX_ANLG_CHNL];
extern int currFileType;
extern FILE *newEvents;
extern XColor black;
extern Colormap cmap;
extern Boolean show_tally;

XtAppContext app_context;
void SetWatchCursor (Widget widget);

extern Pixmap palette_xpm[];

void update_cursor (int new_x, int new_y, int func);

extern char *spike_filename;
