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


#define ADT 0
#define BDT 1
#define DDT 2
#define EDT 3

#define CODEFLAG -2
#define STARTFLAG 0
#define ENDFLAG 0x7FFFFFFF

#include "defines.h"

typedef struct
{
   char Ifname[256];	/* Input file name (sequential access)	*/
   char Ofname[256];	/* Output file name (direct access)	*/
   int  recCount;		/* Output file record count		*/
   int  firstTime;		/* First time tic in file		*/
   int  lastTime;		/* Last time tic in file		*/
   int  achan, dchan;	/* Analog & digital signal counts	*/
   int  ids[MAX_DIG_CHNL + MAX_ANLG_CHNL];		/* Signal IDs in file(was 60)		*/
   int  tally[MAX_DIG_CHNL + MAX_ANLG_CHNL]; 	/* Tally of events for digital codes	*/
   int  indexMap[2000];	/* Maps IDs to corresponding index	*/
   int  amax[99], amin[99];/* Max & min of analog signals found	*/
} SpikeFile;

/*** ZZZ future ????	
struct l_list
{
   int                  CH;
   unsigned long int    TS;

   struct l_list        *prev;
   struct l_list        *next;

//   l_list() { prev = NULL; next = NULL; }
};
ZZZ ***/

typedef char tempFile[256];

int ProcFile (char *fname);

extern SpikeFile sf;
extern int hdt;

void count_95s (char *file, int line);
