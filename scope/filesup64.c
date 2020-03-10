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


#define _LARGEFILE64_SOURCE 1
#include "std.h"
#include "filesup.h"
#include "dispUnit64.h"
#include "AddTree.h"

extern Widget     horiz_bar;
extern int        leftTime, firstTime, fileTime;

SpikeFile         sf;
extern int        newCodeId;
extern int        newEventTally;	   /* gally of new code events	    */

int               hstotal, hdt;
long              hscounter;


void
SortCodes (void)
{
  int i, j;
  int tempMin, tempMax, tempId, tempTally;

  /* This section of code sorts the codes within the Spikefile
     record using bubblesort.                                       */

  /* digital codes */
  for (i = sf.dchan; i > 0; i--) /* has messed with i>=0 also dchan -1 */
    for (j = 0; j < i; j++)
      if (sf.ids[j] > sf.ids[j+1])
	{
	  tempId        = sf.ids[j];
	  sf.ids[j]     = sf.ids[j+1];
	  sf.ids[j+1]   = tempId;
	  tempTally     = sf.tally[j];
	  sf.tally[j]   = sf.tally[j+1];
	  sf.tally[j+1] = tempTally;
	}

  /* actually doing nothing but pulling up the bug in the bubble sort filter */
  if( sf.dchan > 0)
    {
      while (sf.ids[0] ==  0  ) 
	{
	  for(j =1; j< sf.dchan +1; j++) 
	    {
	      sf.ids[j-1]     = sf.ids[j];
	      sf.tally[j-1]   = sf.tally[j];
	    }
	}

      for(i=sf.dchan-1;i<sf.dchan+3;i++)
	{
	  if( sf.ids[i] == sf.ids[i+1] )
	    {
	      sf.ids[i+1] = 0;
	    }
	}
    }
  /* moving till all the -1s are gone **/

  /* sf.dchan;  major hacking */
  /* analog codes */
  /*        sf.achan = sf.achan + hstotal;*/
  for(i=sf.achan-1;i>0;i--)
    for(j=0;j<i;j++)
      if(sf.ids[j+MAX_DIG_CHNL] > sf.ids[j+MAX_DIG_CHNL+1])
	{
	  tempId = sf.ids[j+MAX_DIG_CHNL];
	  tempMax = sf.amax[j];
	  tempMin = sf.amin[j];
	  sf.ids[j+MAX_DIG_CHNL ] = sf.ids[j+ MAX_DIG_CHNL+1];
	  sf.amax[j] = sf.amax[j+1];
	  sf.amin[j] = sf.amin[j+1];
	  sf.ids[j+MAX_DIG_CHNL+1] = tempId;
	  sf.amax[j+1] = tempMax;
	  sf.amin[j+1] = tempMin;
	}

  /* set the index array which maps code id to index in the SpikeFile */
  for(j=0;j<2000;j++)
    sf.indexMap[j] = 0; 

  for (j = 0; j < sf.dchan; j++)
     sf.indexMap[sf.ids[j]] = j;

  for (j = 0; j < sf.achan; j++)
     sf.indexMap[sf.ids[MAX_DIG_CHNL+j]+1000] = MAX_DIG_CHNL + j;
}

void
alert (char *msg)
{
   extern Widget app_shell;
   static Widget w;
   Widget c;
   XmString s = XmStringCreateLocalized (msg);
   if (w == 0) {
      w = XmCreateErrorDialog (app_shell, "alert", 0, 0);
      c = XtNameToWidget (w, "Cancel");
      XtUnmanageChild (c);
      c = XtNameToWidget (w, "Help");
      XtUnmanageChild (c);
   }
   XtVaSetValues (w, XmNmessageString, s, NULL);
   XmStringFree (s);
   XtManageChild (w);
}

int ProcFile(char fname[])
{
   int            code = 0, time = 0, len;
   FILE           *fp1, *f2, *fp6;
   char           pair[20], buffer[30];
   int            analogId, magnitude, i, j;
   short          numb2, numb3;

   hdt=0;
   hscounter = 0;

   /* Initialize data file parameters	*/

   strcpy(sf.Ifname,fname);
   sf.recCount = 0;
   sf.lastTime = 0;
   sf.achan = 0;
   sf.dchan = 0;
   m=0;
   for (i = 0; i < MAX_DIG_CHNL + MAX_ANLG_CHNL; i++)
   {
      sf.ids[i] = 0;
      sf.tally[i] = 0;
   }

   for(i=0;i<99;++i)
   {
      sf.amax[i] = -2048;
      sf.amin[i] =  2047;
   }

   /* Open the input file for reading	*/
   fp1 = fopen (sf.Ifname, "r");
   if(fp1 == NULL)
   {
      printf("\nunable to open %s for reading.\n\n",sf.Ifname);
      DIE;
   }

   /* Open the direct access file for output	*/
   f2 = tmpfile64 ();
   if(f2 == NULL)
   {
      printf("\nunable to open temporary direct access file\n\n");
      DIE;
   }

   /* open high speed file to get total # of channel */
   if ( (fp6 = fopen64(newname2,"rb")) != (FILE *) NULL)
   {
      /*      printf("hdt...\n");*/
      fseek(fp6,0, SEEK_SET);
      fgetc(fp6);
      fgetc(fp6);
      numb2 = fgetc(fp6);
      numb3 = fgetc(fp6);

      /* Number of Channels */
      hstotal   =  256 * numb2 + numb3;
      fclose(fp6);

      hdt=1;

      /*      printf("end hdt...\n");*/
   }
   
   /* Write dummy records to beginning of output file	*/
   for(i=0;i<4;i++) 
   {
      putw(CODEFLAG,f2);
      putw(STARTFLAG,f2);
   }

   /* Find the first data pair in the file */
   fgets (pair, 20, fp1) || DIE;
   len = strlen (pair);
   while(len <= 10)
   {
      if (fgets (pair, 20, fp1) == NULL) exit (1);
      len = strlen (pair);
   }
	
   /* Get the file type and reset the file pointer		*/
   scaleFactor = 1;
   if (len == 11)
   {
      fseek (fp1, (long)-len, SEEK_CUR); /* no header, go back */
      currFileType = 0;
   }
   else if (len == 14)
   {
      fseek (fp1, (long)len, SEEK_CUR); /* skip second header line */
      if(strcmp(fname + (strlen (fname) - 3), "ddt") == 0) {
         scaleFactor = 5;
         currFileType = 2;
      }
      else
         currFileType = 1;

   }
   else if (len == 16)
   {
      fseek (fp1, (long)len, SEEK_CUR); /* skip second header line */
      scaleFactor = 5;
      currFileType = 3;
   }
   ticks_per_second = 2000 * scaleFactor;

   /* The file may now be read and processed	*/
   while (fgets (buffer, 20, fp1) != NULL)
   {
      int codelen;
      if(len == 11)
         codelen = 2;
      else if (len == 14 || len == 16)
         codelen = 5;
      else { 
	 printf("dt line len = %d\n", len);
	 DIE;
      }

      time = atoi (buffer + codelen);
      buffer[codelen] = 0;
      code = atoi (buffer);
      putw (code, f2);
      putw (time, f2);

      /*                                   */
      /* Set the spike file parameters		*/
      /*                                   */
	
      if (sf.recCount++ == 0)
         sf.firstTime = time;
      
      if (code < 1000) /* a digital signal	*/
      {
         j = 0;
         while(sf.ids[j] != 0 && sf.ids[j] != code)
            j++;
         if(sf.ids[j] == 0) {
            sf.ids[j] = code;
            sf.dchan++;
         }
         sf.tally[j]++;
      }
      else if (code > 4095)     /* an analog signal  */
      {
         analogId = code / 4096;
         
         if((code & 0x800) == 0)
            magnitude = code & 0x7FF;
         else
            magnitude = code | 0xFFFFF000U;

         j = MAX_DIG_CHNL;
         while (sf.ids[j] != 0 && sf.ids[j] != analogId)
            j++;

         if(sf.ids[j] == 0) {
            sf.ids[j] = analogId;
            sf.achan++;
         }

         if(sf.amax[j-MAX_DIG_CHNL] < magnitude)
            sf.amax[j-MAX_DIG_CHNL] = magnitude;

         if(sf.amin[j-MAX_DIG_CHNL] > magnitude)
            sf.amin[j-MAX_DIG_CHNL] = magnitude;
      }
      else {
	 char *msg;
	 if (asprintf (&msg, "ILLEGAL CODE %d IN\n\n%s\n\nCANNOT LOAD FILE", code, fname) == -1) exit (1);
	 alert (msg);
	 free (msg);
	 return 0;
      }
   }
   sf.dchan <= 1000 || DIE;
   
   sf.lastTime = time;

   /* Write dummy records to end of output file	*/

   for(i = 0; i < 4; i++) 
   {
      putw(CODEFLAG,f2);
      putw(ENDFLAG,f2);
   }

   /* -------------------------CUTY--------------------------------------*/
   rewind (f2);
   fp = f2;
   fclose(fp1);

   SortCodes ();

   return(1);
}


/* This procedure checks the spikefile record and returns an unused
   code # for use in adding codes the scope.  The "new" codes start
   at Id = 50.                                                       */
int nextNewCodeId(void)
{
   int i;
   Boolean present;
   int newId = 50;

   present = True;
   while(present == True)
   {
      newId++;
      if (newId == 97)
         newId = 100;
      present = False;
      for(i = 0; i < 999; i++)
         if(sf.ids[i] == newId)
            present = True;
   }
   return(newId);
}

static inline void
showProgressMsg (char *msg)
{
   XmString s = XmStringCreateLocalized (msg);
   XtVaSetValues (progress, XmNmessageString, s, NULL);
   XmStringFree (s);
   XFlush (XtDisplay (app_shell));
   XmUpdateDisplay (app_shell);
}

static inline void
showProgress (int eventTime)
{
   if (progress)
   {
      static int last_pct;
      int pct;
      char *pct_s;

      pct = (eventTime - sf.firstTime) * 100LL / (sf.lastTime - sf.firstTime);
      if (pct == last_pct)
         return;
      //      printf ("%d %d %d: %d\n", sf.firstTime, eventTime, sf.lastTime, pct);
      if (asprintf (&pct_s, "%d%%", pct) == -1) exit (1);
      showProgressMsg (pct_s);
      free (pct_s);
      last_pct = pct;
   }
}

void mergeNewEvents(void)
{
   FILE *target;
   Cardinal i;
   int newTime, eventId = 0, eventTime = 0;
   int lastTime = 0, counter;

   counter = -1;

   if (fp)
     fseek(fp,32L, SEEK_SET);
   rewind(newEvents);

   /* open DA file, to serve as a target for the merge */
   target = tmpfile ();
   if(target == NULL)
   {
      printf("\nunable to open (target file) for writing\n\n");
      DIE;
   }
   
   /* Write dummy records to beginning of output file      */
   for(i=0;i<4;i++)
   {
      putw(CODEFLAG,target);
      putw(STARTFLAG,target);
   }

   /* get the first items from each file to be merged */
   if (fp) {
     eventId   = getw(fp);
     eventTime = getw(fp);
   }
   newTime   = getw(newEvents);


   /* merge the files until the end of a file is reached */
   while((fp == NULL || eventId != CODEFLAG) && newTime != EOF)
   {
      if(fp == NULL || newTime < eventTime)
      {

         /* write the new ID and time and get the next newTime  */
         putw(newCodeId,target);
         putw(newTime,target);

         newTime = getw(newEvents);
      }
      else
      {
      
         /* write the event ID and time to target and get
            	the next event ID and time.                      */
         putw(eventId,target);
         putw(eventTime,target);
         showProgress (eventTime);
         eventId = getw(fp);
         eventTime = getw(fp);
      }
      counter++;
   }

   /* write the remaining file to the target */
   if(fp == NULL || eventId == CODEFLAG)
      /* write the remaining new event file */
      while(newTime != EOF)
      {
         putw(newCodeId,target);
         putw(newTime,target);
         counter++;
         lastTime = newTime;
         newTime = getw(newEvents);
      }
   else
      /* write remaining event file */
      while(eventId != CODEFLAG)
      {
         putw(eventId,target);
         putw(eventTime,target);
         showProgress (eventTime);
         lastTime = eventTime;
         counter++;
         eventId = getw(fp);
         eventTime = getw(fp);
      }

   /* Write dummy records to end of output file    */
   for(i=0;i<4;i++)
   {
      putw(CODEFLAG,target);
      putw(ENDFLAG,target);
   }
   /* close the event and target files */
   if (fp)
     fclose(fp);
   fp = target;
   rewind (fp);

   /* debug */
   if (0)
   {
     int time, last_time = 0;
     getw (fp);
     time = getw (fp);
     while (!feof (fp)) {
       time >= last_time || DIE;
       showProgress (time);
       last_time = time;
       getw (fp);
       time = getw (fp);
     }
     rewind (fp);
   }

   fclose(newEvents);
   newEvents = 0;

   /* update the spikefile record to reflect addition of the new event. */
   sf.ids[sf.dchan] = newCodeId;
   sf.tally[sf.dchan] = newEventTally;
   sf.indexMap[newCodeId] = sf.dchan;
   sf.dchan++;
   sf.recCount = counter;
   sf.lastTime = lastTime;
   SortCodes ();

   {
     Cardinal i;
     Arg args[2];

     i=0;
     XtSetArg(args[i], XmNmaximum, MAX (leftTime + screenWidth, firstTime + fileTime + 1)); i++;
     XtSetValues(horiz_bar, args, i);
   }
}


/*** ZXC  removed 10-29-02 */
/*** rm pb 5-03
int tally(int code)
{
   return(0);
}
***/
/***/

/* These routines are DA file access routines which serve
   the find-event function.                                     */
static long eventPtr;
static int eventIndex;
static int activeCode;


void resetEvents(int codeId)
{
   if(codeId != activeCode)
   {
      eventPtr = 32;
      eventIndex = 0;
      activeCode = codeId;
   }
}


int getNextEvent(void)
{
   int code, time;

/*	if(code == 0)
	  return(-1);*/

   fseek(fp,eventPtr, SEEK_SET);

   code = getw(fp);
   time = getw(fp);

   while((code != activeCode) && (code != CODEFLAG))
   {
      code = getw(fp);
      time = getw(fp);
   }

   if(code == activeCode)
   {
      eventIndex++;
      eventPtr = ftell(fp);
      return(time);
   }
   else
      return(-1);
}


int getEventNum(int num)
{
   int code;
   static int time;

   if (num == eventIndex)
      return time;
   fseek(fp,eventPtr, SEEK_SET);
   code = 0;
			  
   if(num > eventIndex)
      while((num > eventIndex) && (code != CODEFLAG))
      {
         code = getw(fp);
         time = getw(fp);
         if(code == activeCode)
            eventIndex++;
      }
   else
      while((num < eventIndex) && (code != CODEFLAG))
      {
         fseek(fp,-16L, SEEK_CUR);
         code = getw(fp);
         time = getw(fp);
         if(code == activeCode)
            eventIndex--;
      }

   if(code == activeCode)
   {
      eventPtr = ftell(fp);
      return(time);
   }
   else
      return(-1);
}

int eventIndexVal(void)
{
   return(eventIndex);
}

/* Local Variables: */
/* c-basic-offset: 3 */
/* c-file-offsets:  ((substatement-open . 0)) */
/* End: */
