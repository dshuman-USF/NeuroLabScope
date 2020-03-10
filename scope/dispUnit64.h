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


void InitScope (void);
void refresh (void);
void forewardBlocks (void);
void reverseBlocks (void);
void refreshBlocks (void);
void hsforeward (int pixels);
void hsreverse (int pixels);
void integrate (void);
void makeBandpassPop (Widget w, XtPointer client_data, XtPointer call_data);
void map_dialog (Widget dialog, XtPointer client_data, XtPointer call_data);

typedef struct 
{
	char     *label;
	void    (*callback)();
	XtPointer data;
} ActionAreaItem;
Widget CreateActionArea (Widget parent, ActionAreaItem *actions, int num_actions);

extern long screenWidth;        /* in ticks */
extern int leftTime, rightTime; /* in ticks */
extern int leftoff, partition;
extern Dimension scopeScreen_x, scopeScreen_y;
extern int rightTime;
extern int scrollMargin, marginTime, fileTime, firstTime;
extern FILE *fp, *fp9;
extern long screenLeft;
extern float samplerate;
extern float realLeft;
extern int signalBase[MAX_DIG_CHNL + MAX_ANLG_CHNL]; /* y_offset for trains  */

typedef struct
{
  unsigned bins;
  unsigned char palette; 
  unsigned local : 1;
  unsigned show_max_spk : 1;
} HistogramParams;

typedef struct
{
  char *filename;
  int ticks_per_bin;
  unsigned legal : 1;
  unsigned global : 1;
  unsigned tally : 1;
} PSParams;
void ps (PSParams *);
