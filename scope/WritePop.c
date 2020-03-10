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



/*** */
#define _SVID_SOURCE 1
#define _LARGEFILE64_SOURCE 1
#include "std.h"
#include <ctype.h>
#include <limits.h>
#include "filesup.h"
#include "blockList.h"
#include "WritePop.h"
#include "FilterTree.h"
#include "dispUnit64.h"

#define MARKB  21
#define MARKE  22

static unsigned long swaptest = 1;

Boolean anyLabelSet();
Boolean passBlocks();
Boolean markCodes();
void makeWarningPop();

/* global variables (widgets,etc....) */

extern Widget SignalLabel2W[MAX_HDT_CHNL];
extern int NextLabel2;
extern int hstotal;

extern float samplerate;
extern int hsmax[MAX_HDT_CHNL], hsmin[MAX_HDT_CHNL];
extern int hdt;

extern Widget main_window;

Widget writePop;
Widget outFileEditW;
Widget omitToggleW, includeToggleW, eventMarkToggleW, integrateToggleW;
Widget adtToggleW, bdtToggleW, ddtToggleW, edtToggleW;
Widget truncationToggleW;
Widget eventEditW;

/*** WriteInfoPop */
/* global variables */

Widget   writeInfoPop,
         filetypeValW,
         outfileValW,
         truncateValW,
         truncateEventStatW,
         editblockStatW,
         passCodeStatW,
         selectedChannelW,
         digitalOutputW,
         filterModeInfoStatW,
         markCodeStatW,
         overwriteStateW;

int  FileType, PassSelCh, IntegrateOut;
char mystring[80];

int   state, hdtchannels, hdtselect[MAX_HDT_CHNL];

char  overwrite[80],
      stringtype[80],
      truncval[80],   /*changed girish */
      eventStr[35],
      editBlockdef[80],
      editBlockout[80],
      markCodeadd[80],
      passCodeOut[80],
      selectedChannel[80],
      digitalOutput[80];


/* procedures for setting the write information popup */
void setTruncEvents();
void writeOutputFile();
void writeIntegrateFile();


void makeWriteInfoPop();

extern Boolean dchanSet();
extern Boolean achanSet();

static int truncationToggleVal;
Boolean truncateSet()
{
   Cardinal i;
   Arg arglist[4];
   Boolean value;

   i=0;
   XtSetArg(arglist[i],XmNset,&value);i++;
   XtGetValues(truncationToggleW,arglist,i);
   truncationToggleVal = value;
   return(value);
}

void truncateSetCB (Widget w, XtPointer client_data, XtPointer call_data)
{
   truncateSet();
}

void omitChannels(Widget w, XtPointer client_data, XtPointer call_data)
{
   int  i;
   Arg arglist[4];

   i=0;
   XtSetArg(arglist[i],XmNset,True);i++;
   XtSetValues(omitToggleW,arglist,i); 
   i=0;
   XtSetArg(arglist[i],XmNset,False);i++;
   XtSetValues(includeToggleW,arglist,i); 

   PassSelCh = 0;
   XFlush(XtDisplay(omitToggleW));

}

void includeChannels(Widget w, XtPointer client_data, XtPointer call_data)
{
   int  i;
   Arg arglist[4];

   i=0;
   XtSetArg(arglist[i],XmNset,False);i++;
   XtSetValues(omitToggleW,arglist,i); 
   i=0;
   XtSetArg(arglist[i],XmNset,True);i++;
   XtSetValues(includeToggleW,arglist,i); 

   PassSelCh = 1; 
   XFlush(XtDisplay(includeToggleW));

}


void eventMark(Widget w, XtPointer client_data, XtPointer call_data)
{
   int  i;
   Arg arglist[4];

   i=0;
   XtSetArg(arglist[i],XmNset,True);i++;
   XtSetValues(eventMarkToggleW,arglist,i); 
   i=0;
   XtSetArg(arglist[i],XmNset,False);i++;
   XtSetValues(integrateToggleW,arglist,i); 

   IntegrateOut = 0;
   XFlush(XtDisplay(omitToggleW));

}


void integrateOut(Widget w, XtPointer client_data, XtPointer call_data)
{
   int  i;
   Arg arglist[4];

   i=0;
   XtSetArg(arglist[i],XmNset,False);i++;
   XtSetValues(eventMarkToggleW,arglist,i); 
   i=0;
   XtSetArg(arglist[i],XmNset,True);i++;
   XtSetValues(integrateToggleW,arglist,i); 

   IntegrateOut = 1; 
   XFlush(XtDisplay(includeToggleW));

}


void edtFileType(Widget w, XtPointer client_data, XtPointer call_data)
{
   int  i;
   Arg arglist[4];

   i=0;
   XtSetArg(arglist[i],XmNset,False);i++;
   XtSetValues(adtToggleW,arglist,i); 
   i=0;
   XtSetArg(arglist[i],XmNset,False);i++;
   XtSetValues(bdtToggleW,arglist,i); 
   i=0;
   XtSetArg(arglist[i],XmNset,False);i++;
   XtSetValues(ddtToggleW,arglist,i); 
   i=0;
   XtSetArg(arglist[i],XmNset,True);i++;
   XtSetValues(edtToggleW,arglist,i); 

   FileType = 3; 
   XFlush(XtDisplay(edtToggleW));

   if (scaleFactor == 1)
      makeWarningPop("Scale Error","File types have different resolution.\nIncreased resoulution will be simulated.");

}


void ddtFileType(Widget w, XtPointer client_data, XtPointer call_data)
{
   int  i;
   Arg arglist[4];

   i=0;
   XtSetArg(arglist[i],XmNset,False);i++;
   XtSetValues(adtToggleW,arglist,i); 
   i=0;
   XtSetArg(arglist[i],XmNset,False);i++;
   XtSetValues(bdtToggleW,arglist,i); 
   i=0;
   XtSetArg(arglist[i],XmNset,True);i++;
   XtSetValues(ddtToggleW,arglist,i); 
   i=0;
   XtSetArg(arglist[i],XmNset,False);i++;
   XtSetValues(edtToggleW,arglist,i); 

   FileType = 2; 
   XFlush(XtDisplay(ddtToggleW));

   if (scaleFactor == 1)
      makeWarningPop("Scale Error","File types have different resolution.\nIncreased resoulution will be simulated.");

}


void bdtFileType(Widget w, XtPointer client_data, XtPointer call_data)
{
   int  i;
   Arg arglist[4];

   i=0;
   XtSetArg(arglist[i],XmNset,False);i++;
   XtSetValues(adtToggleW,arglist,i); 
   i=0;
   XtSetArg(arglist[i],XmNset,True);i++;
   XtSetValues(bdtToggleW,arglist,i); 
   i=0;
   XtSetArg(arglist[i],XmNset,False);i++;
   XtSetValues(ddtToggleW,arglist,i); 
   i=0;
   XtSetArg(arglist[i],XmNset,False);i++;
   XtSetValues(edtToggleW,arglist,i); 

   FileType = 1; 
   XFlush(XtDisplay(bdtToggleW));

   if (scaleFactor == 5)
      makeWarningPop("Scale Error","File types have different resolution.\nResoulution will be decreased.");

}


void adtFileType(Widget w, XtPointer client_data, XtPointer call_data)
{
   int  i;
   Arg arglist[4];

   i=0;
   XtSetArg(arglist[i],XmNset,True);i++;
   XtSetValues(adtToggleW,arglist,i);
   i=0;
   XtSetArg(arglist[i],XmNset,False);i++;
   XtSetValues(bdtToggleW,arglist,i);
   i=0;
   XtSetArg(arglist[i],XmNset,False);i++;
   XtSetValues(ddtToggleW,arglist,i);
   i=0;
   XtSetArg(arglist[i],XmNset,False);i++;
   XtSetValues(edtToggleW,arglist,i);
	
   FileType = 0; 
   XFlush(XtDisplay(adtToggleW));

   if (scaleFactor == 5)
      makeWarningPop("Scale Error","File types have different resolution.\nResoulution will be decreased.");
}


int truncEvents()
{
   char *buff;
   int value;

   buff = XmTextGetString(eventEditW);
   sscanf(buff,"%d",&value);

   return(value);
}


void processWrite(w,closure,call_data)
   Widget w;
   XtPointer closure;
   XtPointer call_data;
{
   FILE *fpOut;

   /* get the list of codes to be passed */
   switch (FileType)
   {
   case 0:
      strcpy(stringtype, "ADT");
      break;
   case 1:
      strcpy(stringtype, "BDT");
      break;
   case 2:
      strcpy(stringtype, "DDT");
      break;
   case 3:
      strcpy(stringtype, "EDT");
      break;
   default:
      strcpy(stringtype, "DDT");
      break;
   }

   /*** XZY
	if(FileType == 1)
	strcpy(stringtype, "BDT");
	else 
	strcpy(stringtype, "ADT");
   ***/

   /* set the write Information fields of the last write popup */
   {
      char *str = XmTextGetString (outFileEditW);
      int n, len;

      strlen (str) < 80 || DIE;
      strcpy (mystring, str);
      XtFree (str);
      len = strlen (mystring);
      if (len < 5 || mystring[len-4] != '.') {
	 len < 76 || DIE;
	 strcat (mystring, ".");
	 strcat (mystring, stringtype);
	 for (n = len + 1; n < len + 4; n++)
	    mystring[n] = tolower (mystring[n]);
      }
   }

   fpOut = fopen (mystring, "r");
   if (fpOut == NULL)
      strcpy(overwrite, "New output file");
   else {
      strcpy(overwrite, "*** WARNING FILE OVERWRITE ***  ");
      fclose(fpOut);
   }
   /* get truncation switch state and event limit */
   if(truncateSet() == True)
   {
      strcpy(truncval, "ON");
      setTruncEvents(truncEvents());
   }
   else
   {
      strcpy(truncval, "OFF");
      setTruncEvents(0);
   }

   if(BlockCount(segmentList) > 0) 
   {
      strcpy(editBlockdef,"Edit Blocks are defined");
      if (FilterPopOpened)
	 if(markCodes() == True)
	    strcpy(markCodeadd,"MarkE & MarkB codes added");
	 else
	    strcpy(markCodeadd,"\0");
      else
	 strcpy(markCodeadd,"\0");

      if(passBlocks() == True)
	 strcpy(editBlockout, "Edit blocks will be output");
      else
	 strcpy(editBlockout,"Edit blocks will be\nomitted from output");
   }
   else
   {
      strcpy(editBlockdef,"No Edit Blocks are defined");
      strcpy(markCodeadd, "\0");
      strcpy(editBlockout, "\0");
   }

   if(anyLabelSet() == True) {
      if (PassSelCh)
	 strcpy(selectedChannel, "Selected channels will be the output");
      else
	 strcpy(selectedChannel, "Selected channels will be omitted from output");
   }
   else {
      if (PassSelCh)
	 strcpy(selectedChannel, "All codes will be omitted from output");
      else
	 strcpy(selectedChannel, "All codes will be output");
   }


   if (IntegrateOut)
      strcpy(digitalOutput, "Digital channels will be output as integrated unit activity");
   else
      strcpy(digitalOutput, "Digital channels will be output as event marks");

   /* destroy popup */
   XtDestroyWidget(writePop);
   
   /* popup up final description of write state */
   makeWriteInfoPop();
}


/*********************************************************
 *          The root popup of the filter tree            *
 *            initializations & definitions              *
 *********************************************************/

void writeEscUp(w, closure, call_data)
   Widget   w;
   XtPointer  closure;
   XtPointer  call_data;

{
   XtDestroyWidget(writePop);
}


void makeWritePop(void)
{
   /* callback lists */
   /* Callback list for button selections */
   static XtCallbackRec writeCallbacks[] = 
      {
	 { processWrite, NULL },
	 { NULL, NULL },
      };

   static XtCallbackRec cancelCallbacks[] =
      {
	 { writeEscUp, NULL },
	 { NULL, NULL }
      };

   /* local variable declarations */
   Arg arglist[6];
   Cardinal i;

   Widget outFileStatW;
   Widget eventStatW;
   Widget writeButtonW;
   Widget cancelButtonW;
   Widget labelW, label1W, label2W;

   /*** This section creates an information popup */
   FileType = currFileType;
   PassSelCh = 0;
   IntegrateOut = 0;
   /*** PPP */
   i = 0;
   XtSetArg(arglist[i], XmNwidth, 300); i++;
   //   XtSetArg(arglist[i], XmNheight, 300); i++;
   XtSetArg(arglist[i], XmNheight, 355); i++;
   XtSetArg(arglist[i], XmNmarginHeight, 1); i++;
   XtSetArg(arglist[i], XmNautoUnmanage, False); i++;
   XtSetArg(arglist[i], XmNallowOverlap, False); i++;

   writePop = XmCreateBulletinBoardDialog(main_window, "Write Info Popup", arglist, i);

   
   i = 0;
   XtSetArg(arglist[i], XmNactivateCallback, writeCallbacks); i++;
   XtSetArg(arglist[i], XmNshowAsDefault, 1); i++;
   //   XtSetArg(arglist[i], XmNy, 250); i++;
   XtSetArg(arglist[i], XmNy, 305); i++;
   XtSetArg(arglist[i], XmNx, 10); i++;
   XtSetArg(arglist[i], XmNwidth, 60); i++;

   writeButtonW = XmCreatePushButton(writePop, "   OK   ", arglist, i);
   XtManageChild(writeButtonW);

   
   i = 0;
   XtSetArg(arglist[i], XmNactivateCallback, cancelCallbacks); i++;
   //   XtSetArg(arglist[i], XmNy, 250); i++;
   XtSetArg(arglist[i], XmNy, 305); i++;
   XtSetArg(arglist[i], XmNx, 219); i++;
   XtSetArg(arglist[i], XmNwidth, 60); i++;

   cancelButtonW = XmCreatePushButton(writePop, " Cancel ", arglist, i);
   XtManageChild(cancelButtonW);


   i = 0;
   XtSetArg(arglist[i], XmNdefaultButton, writeButtonW); i++;
   XtSetArg(arglist[i], XmNcancelButton, cancelButtonW); i++;
   XtSetValues(writePop, arglist, i);


   /* Set up static text widget */
   i = 0;
   XtSetArg(arglist[i], XmNx, 10); i++;
   XtSetArg(arglist[i], XmNy, 20); i++;
   {
      char *str = "Output\nFilename :";
      XmString label_str = XmStringCreateLtoR (str, XmSTRING_DEFAULT_CHARSET);
      XtSetArg (arglist[i], XmNlabelString, label_str);  i++;
      outFileStatW=XmCreateLabel(writePop, str, arglist, i);
      XmStringFree (label_str);
   }

   XtManageChild(outFileStatW);

   /* Set up text edit widget for code input */
   i = 0;
   XtSetArg(arglist[i], XmNx, 80); i++;
   XtSetArg(arglist[i], XmNy, 20); i++;
   XtSetArg(arglist[i], XmNcolumns, 31); i++;
   outFileEditW=XmCreateText(writePop,"outFileEdit", arglist,i);
   XtManageChild(outFileEditW); 

   i = 0;
   XtSetArg(arglist[i], XmNy, 65); i++;
   XtSetArg(arglist[i], XmNx, 10); i++;
   label1W = XmCreateLabel(writePop, "Selected channels will be", arglist, i);
   XtManageChild(label1W);

   /* Set up omit toggle widget    */
   i = 0;
   XtSetArg(arglist[i], XmNy, 85); i++;
   XtSetArg(arglist[i], XmNx, 10); i++;
   XtSetArg(arglist[i], XmNset, True); i++;
   XtSetArg(arglist[i], XmNindicatorOn, True);i++;
   XtSetArg(arglist[i], XmNfillOnSelect, True);i++;
   omitToggleW = XmCreateToggleButton(writePop, "omitted from output", arglist,i);
   XtManageChild(omitToggleW);
   XtAddCallback(omitToggleW, XmNarmCallback, omitChannels, NULL);
   XtAddCallback(omitToggleW, XmNdisarmCallback, omitChannels, NULL);

   /* Set up include toggle widget    */
   i = 0;
   XtSetArg(arglist[i], XmNy, 85); i++;
   XtSetArg(arglist[i], XmNx, 150); i++;
   XtSetArg(arglist[i], XmNindicatorOn, True);i++;
   XtSetArg(arglist[i], XmNfillOnSelect, True);i++;
   includeToggleW = XmCreateToggleButton(writePop, "the output", arglist,i);
   XtManageChild(includeToggleW);
   XtAddCallback(includeToggleW, XmNarmCallback, includeChannels, NULL);
   XtAddCallback(includeToggleW, XmNdisarmCallback, includeChannels, NULL);

   i = 0;
   XtSetArg(arglist[i], XmNy, 120); i++;
   XtSetArg(arglist[i], XmNx, 10); i++;
   label2W = XmCreateLabel(writePop, "Digital channels will be output as", arglist, i);
   XtManageChild(label2W);

   /* Set up omit toggle widget    */
   i = 0;
   XtSetArg(arglist[i], XmNy, 140); i++;
   XtSetArg(arglist[i], XmNx, 10); i++;
   XtSetArg(arglist[i], XmNset, True); i++;
   XtSetArg(arglist[i], XmNindicatorOn, True);i++;
   XtSetArg(arglist[i], XmNfillOnSelect, True);i++;
   eventMarkToggleW = XmCreateToggleButton(writePop, "event marks", arglist,i);
   XtManageChild(eventMarkToggleW);
   XtAddCallback(eventMarkToggleW, XmNarmCallback, eventMark, NULL);
   XtAddCallback(eventMarkToggleW, XmNdisarmCallback, eventMark, NULL);

   /* Set up include toggle widget    */
   i = 0;
   XtSetArg(arglist[i], XmNy, 140); i++;
   XtSetArg(arglist[i], XmNx, 105); i++;
   XtSetArg(arglist[i], XmNindicatorOn, True);i++;
   XtSetArg(arglist[i], XmNfillOnSelect, True);i++;
   integrateToggleW = XmCreateToggleButton(writePop, "integrated unit activity", arglist,i);
   XtManageChild(integrateToggleW);
   XtAddCallback(integrateToggleW, XmNarmCallback, integrateOut, NULL);
   XtAddCallback(integrateToggleW, XmNdisarmCallback, integrateOut, NULL);

   i = 0;
   //   XtSetArg(arglist[i], XmNy, 120); i++;
   XtSetArg(arglist[i], XmNy, 175); i++;
   XtSetArg(arglist[i], XmNx, 10); i++;
   labelW = XmCreateLabel(writePop, "File Format", arglist, i);
   XtManageChild(labelW);

   /* Set up ADT toggle widget    */
   i = 0;
   //   XtSetArg(arglist[i], XmNy, 140); i++;
   XtSetArg(arglist[i], XmNy, 195); i++;
   XtSetArg(arglist[i], XmNx, 10); i++;
   if (FileType == 0)
   {
      XtSetArg(arglist[i], XmNset, True); i++;
   }
   XtSetArg(arglist[i], XmNindicatorOn, True);i++;
   XtSetArg(arglist[i], XmNfillOnSelect, True);i++;
   adtToggleW = XmCreateToggleButton(writePop, "ADT", arglist,i);
   XtManageChild(adtToggleW);
   XtAddCallback(adtToggleW, XmNarmCallback, adtFileType, NULL);
   XtAddCallback(adtToggleW, XmNdisarmCallback, adtFileType, NULL);

   /* Set up BDT toggle widget     */
   i = 0;
   //   XtSetArg(arglist[i], XmNy, 140); i++;
   XtSetArg(arglist[i], XmNy, 195); i++;
   XtSetArg(arglist[i], XmNx, 80); i++;
   if (FileType == 1)
   {
      XtSetArg(arglist[i], XmNset, True); i++;
   }
   XtSetArg(arglist[i], XmNindicatorOn, True); i++;
   XtSetArg(arglist[i], XmNfillOnSelect, True); i++;
   bdtToggleW=XmCreateToggleButton(writePop,"BDT", arglist,i);
   XtManageChild(bdtToggleW);
   XtAddCallback(bdtToggleW, XmNarmCallback, bdtFileType, NULL);
   XtAddCallback(bdtToggleW, XmNdisarmCallback, bdtFileType, NULL);

   /* Set up DDT toggle widget     */
   i = 0;
   //   XtSetArg(arglist[i], XmNy, 140); i++;
   XtSetArg(arglist[i], XmNy, 195); i++;
   XtSetArg(arglist[i], XmNx, 150); i++;
   if (FileType == 2)
   {
      XtSetArg(arglist[i], XmNset, True); i++;
   }
   XtSetArg(arglist[i], XmNindicatorOn, True); i++;
   XtSetArg(arglist[i], XmNfillOnSelect, True); i++;
   ddtToggleW=XmCreateToggleButton(writePop,"DDT", arglist,i);
   XtManageChild(ddtToggleW);
   XtAddCallback(ddtToggleW, XmNarmCallback, ddtFileType, NULL);
   XtAddCallback(ddtToggleW, XmNdisarmCallback, ddtFileType, NULL);

   /* Set up EDT toggle widget     */
   i = 0;
   //   XtSetArg(arglist[i], XmNy, 140); i++;
   XtSetArg(arglist[i], XmNy, 195); i++;
   XtSetArg(arglist[i], XmNx, 220); i++;
   if (FileType == 3)
   {
      XtSetArg(arglist[i], XmNset, True); i++;
   }
   XtSetArg(arglist[i], XmNindicatorOn, True); i++;
   XtSetArg(arglist[i], XmNfillOnSelect, True); i++;
   edtToggleW=XmCreateToggleButton(writePop,"EDT", arglist,i);
   XtManageChild(edtToggleW);
   XtAddCallback(edtToggleW, XmNarmCallback, edtFileType, NULL);
   XtAddCallback(edtToggleW, XmNdisarmCallback, edtFileType, NULL);

   /* Set up Truncation toggle widget */
   i = 0;
   //   XtSetArg(arglist[i], XmNy, 170 ); i++;
   XtSetArg(arglist[i], XmNy, 225); i++;
   XtSetArg(arglist[i], XmNx, 10); i++;
   truncationToggleW = XmCreateToggleButton(writePop,"Truncation", arglist, i);
   XtManageChild(truncationToggleW);
   XtAddCallback(truncationToggleW, XmNarmCallback, truncateSetCB, NULL);
         
   /* Set up static text widget */
   i = 0;
   //   XtSetArg(arglist[i], XmNy, 205); i++;
   XtSetArg(arglist[i], XmNy, 260); i++;
   XtSetArg(arglist[i], XmNx, 3); i++;
   eventStatW=XmCreateLabel(writePop, "Events :", arglist,i);
   XtManageChild(eventStatW);
          
   /* Set up text edit widget for event count input */
   i = 0;
   //   XtSetArg(arglist[i], XmNy, 205); i++;
   XtSetArg(arglist[i], XmNy, 260); i++;
   XtSetArg(arglist[i], XmNx, 80); i++;
   XtSetArg(arglist[i], XmNcolumns, 31); i++;
   eventEditW=XmCreateText(writePop,"eventEdit", arglist, i);
   XtManageChild(eventEditW);

   XtManageChild(writePop);

   /* ZXC */
   /*printf("sF = %d, FT = %d, cFT = %d\n", scaleFactor, FileType, currFileType); */
   if (((scaleFactor == 5) && ((FileType == 0) || (FileType == 1))) ||
       ((scaleFactor == 1) && ((FileType == 2) || (FileType == 3))))
      makeWarningPop("Scale Error","File types have different resolution.\nResoulution will be decreased.");
}


/*********************************************************
 *          The write information popup
 *********************************************************/


void setTruncEvents(count)
     int count;
{
   if(count > 0)
      sprintf(eventStr,"Truncation after %d events",count);
   else
      sprintf(eventStr, "%s", "");
}


/* Callback routine for a write button  selected */
void doWrite(w, closure, call_data)
   Widget w;
   XtPointer closure;
   XtPointer call_data;
{
   /*** PPP */
   if (IntegrateOut)
      writeIntegrateFile();
   else
      writeOutputFile();
   XtDestroyWidget(writeInfoPop);
}


/* Callback routine for a write button  selected */
void abortWrite(w, closure, call_data)
   Widget w;
   XtPointer closure;
   XtPointer call_data;
{
   XtDestroyWidget(writeInfoPop);
}


void makeWriteInfoPop()
{
   /* callback lists */

   /* Callback list for button selections */

   static XtCallbackRec writeCallbacks[] = 
      { 
	 { doWrite, NULL },
	 { NULL, NULL },
      };

   static XtCallbackRec abortCallbacks[] = 
      {
	 { abortWrite, NULL },
	 { NULL, NULL },
      };

   /* local variable declarations */
   Arg arglist[6];
   Cardinal i;

   Widget outfileStatW;
   Widget filetypeStatW;
   Widget truncateStatW;
   Widget writeW;
   Widget abortW;

   /* This section creates the popup  with info pertaining to writing a file */
   i = 0;
   XtSetArg(arglist[i], XmNheight, 430); i++;
   XtSetArg(arglist[i], XmNwidth, 300); i++;
   XtSetArg(arglist[i], XmNmarginHeight, 1); i++;
   XtSetArg(arglist[i], XmNautoUnmanage, False); i++;
   XtSetArg(arglist[i], XmNallowOverlap, False); i++;

   writeInfoPop = XmCreateBulletinBoardDialog(main_window, "Write Info Pop", arglist,i);

   /* Set up Push button widget inside row col */
   i = 0;
   XtSetArg(arglist[i], XmNactivateCallback, writeCallbacks); i++;
   XtSetArg(arglist[i], XmNshowAsDefault, 1); i++;
   XtSetArg(arglist[i], XmNy, 360); i++;
   XtSetArg(arglist[i], XmNx, 10); i++;
   XtSetArg(arglist[i], XmNwidth, 60); i++;
   writeW=XmCreatePushButton(writeInfoPop, " Write ", arglist, i);

   XtManageChild(writeW);

   /* Argument for push button widget */
   i = 0;
   XtSetArg(arglist[i], XmNactivateCallback, abortCallbacks); i++;
   XtSetArg(arglist[i], XmNy, 360); i++;
   XtSetArg(arglist[i], XmNx, 218); i++;
   XtSetArg(arglist[i], XmNwidth, 60); i++;

   abortW = XmCreatePushButton(writeInfoPop, " Abort ", arglist, i);
   XtManageChild(abortW);
              
   i = 0;
   XtSetArg(arglist[i], XmNdefaultButton, writeW); i++;
   XtSetArg(arglist[i], XmNcancelButton, abortW); i++;
   XtSetValues(writeInfoPop, arglist, i);

   /* Set up static text widget */
   i = 0;
   XtSetArg(arglist[i], XmNy, 20); i++;
   XtSetArg(arglist[i], XmNx, 10); i++;
   outfileStatW=XmCreateLabel(writeInfoPop, "Output File :", arglist,i);
   XtManageChild(outfileStatW);
       
   /* Set up static text widget */
   i = 0;
   XtSetArg(arglist[i], XmNy, 20); i++;
   XtSetArg(arglist[i], XmNx, 100); i++;
   outfileValW=XmCreateLabel(writeInfoPop, mystring, arglist,i);
   XtManageChild(outfileValW);          
             
   /* Set up static text widget */
   i = 0;
   XtSetArg(arglist[i], XmNy, 50); i++;
   XtSetArg(arglist[i], XmNx, 10); i++;
   overwriteStateW=XmCreateLabel(writeInfoPop, overwrite,arglist,i);
   XtManageChild(overwriteStateW);          
      
   /* Set up static text widget */
   i = 0;
   XtSetArg(arglist[i], XmNy, 80); i++;
   XtSetArg(arglist[i], XmNx, 10); i++;
   filetypeStatW=XmCreateLabel(writeInfoPop, "File Type  :", arglist,i);
   XtManageChild(filetypeStatW);
              
   /* Set up static text widget */
   i = 0;
   XtSetArg(arglist[i], XmNy, 80); i++;
   XtSetArg(arglist[i], XmNx, 100); i++;
   filetypeValW=XmCreateLabel(writeInfoPop, stringtype, arglist,i);
   XtManageChild(filetypeValW);
             
   /* Set up static text widget */
   i = 0;
   XtSetArg(arglist[i], XmNy, 110); i++;
   XtSetArg(arglist[i], XmNx, 10); i++;
   truncateStatW=XmCreateLabel(writeInfoPop, "Truncation switch :", arglist,i);
   XtManageChild(truncateStatW);
        
   /* Set up static text widget */
   i = 0;
   XtSetArg(arglist[i], XmNy, 110); i++;
   XtSetArg(arglist[i], XmNx, 130); i++;
   truncateValW=XmCreateLabel(writeInfoPop, truncval, arglist,i);
   XtManageChild(truncateValW);

   /* Set up static text widget */
   i = 0;
   XtSetArg(arglist[i], XmNy, 140); i++;
   XtSetArg(arglist[i], XmNx, 10); i++;
   truncateEventStatW=XmCreateLabel(writeInfoPop, eventStr, arglist, i);
   XtManageChild(truncateEventStatW);
                     
   /* Set up static text widget */
   i = 0;
   XtSetArg(arglist[i], XmNy, 170); i++;
   XtSetArg(arglist[i], XmNx, 10); i++;
   editblockStatW=XmCreateLabel(writeInfoPop, editBlockdef, arglist, i);
   XtManageChild(editblockStatW);
           
   /* Set up static text widget */
   i = 0;
   XtSetArg(arglist[i], XmNy, 200); i++;
   XtSetArg(arglist[i], XmNx, 10); i++;
   filterModeInfoStatW=XmCreateLabel(writeInfoPop, editBlockout, arglist, i);
   XtManageChild(filterModeInfoStatW);     

   /* Set up static text widget */
   i = 0;
   XtSetArg(arglist[i], XmNy, 230); i++;
   XtSetArg(arglist[i], XmNx, 10); i++;
   markCodeStatW=XmCreateLabel(writeInfoPop, markCodeadd, arglist, i);
   XtManageChild(markCodeStatW);

   /* Set up static text widget */
   i = 0;
   XtSetArg(arglist[i], XmNy, 260); i++;
   XtSetArg(arglist[i], XmNx, 10); i++;
   passCodeStatW=XmCreateLabel(writeInfoPop, passCodeOut, arglist, i);
   XtManageChild(passCodeStatW);
            
   /* Set up static text widget */
   i = 0;
   XtSetArg(arglist[i], XmNy, 290); i++;
   XtSetArg(arglist[i], XmNx, 10); i++;
   selectedChannelW=XmCreateLabel(writeInfoPop, selectedChannel, arglist, i);
   XtManageChild(selectedChannelW);
            
   XtManageChild(writeInfoPop);       

   /* Set up static text widget */
   i = 0;
   XtSetArg(arglist[i], XmNy, 320); i++;
   XtSetArg(arglist[i], XmNx, 10); i++;
   digitalOutputW=XmCreateLabel(writeInfoPop, digitalOutput, arglist, i);
   XtManageChild(digitalOutputW);
            
   XtManageChild(writeInfoPop);       
}


/* function to write integrated unit activity to a file */
/*** PPP */
void writeIntegrateFile()
/***  write out whole file, selected digital channels channels?, no analog, no high speed
      channels.  Simply replaces current file extension and replaces it with ".int".
***/
{
   FILE     *fpOut;
   char     *format;
   char     dummy1[80];
   Boolean  truncation, markBnE, trunc;
   int      events, eventCount[MAX_DIG_CHNL + MAX_ANLG_CHNL], outputList, i;
   int      code, time, writeList[MAX_DIG_CHNL+MAX_ANLG_CHNL], done, begin, end, start, stop;
   int      truncBegin, truncEnd;


   /* open output file for writing */
   strcpy(sf.Ofname, sf.Ifname);
   strcpy(sf.Ofname + (strlen(sf.Ofname) - 3), "int");
   //printf("%s\n", sf.Ofname);
   //exit(0);
   //   fpOut=fopen(mystring,"w");
   fpOut=fopen(sf.Ofname,"w");
   strcpy(dummy1,"chmod 666");
   strcat(dummy1, "  ");
   strcat(dummy1,mystring);
   if (system(dummy1));

   if(fpOut == NULL)
      printf("Unable to open output file for writing\n");

   /* integrated output will be written "ch, amplitude" */
   format = "%3d,%8d\n";

   /* get truncation switch setting */
   truncation = truncationToggleVal;

   /* get truncation event count */
   if(truncation == True)
      events = truncEvents();
   else
      events = INT_MAX;		/* largest possible integer */

   /* create the write list array */
   for(i = 0; i < (MAX_DIG_CHNL + MAX_ANLG_CHNL); i++)
      writeList[i] = False;
   
   for(i = 0; i < sf.dchan; i++)
      if (PassSelCh)
         writeList[i] = dchanSet(i);
      else
         writeList[i] = !dchanSet(i);
   
   markBnE = 0;

   /* initialize new (output) block list */
   outputList = InitList();
   if(outputList == NIL)
      printf("unable to initialize output block List\n");
            
   /* check if blocks are currently defined */
   if(BlockCount(segmentList) > 0) 
   {
      if(passBlocks() == False)
      {
	 /* invert old block list => new list */
         done = FirstBlock(segmentList,&start, &stop);
	 begin = -1;

	 while( done != NIL)
         {
            if((begin + 1) < (start - 1))
            {
               done = InsBlock(outputList,begin + 1,start - 1);
               if(done == NIL)
                  printf("error inserting into outputList\n");
               else
		  if (0)
		     printf("inserted %d -> %d into outputList\n",begin+1,start-1);
	    }
	    begin = stop;
	    done = NextBlock(segmentList,&start,&stop);
	 }
	 if((begin + 1) < sf.lastTime)
	    InsBlock(outputList,begin + 1,sf.lastTime);
      }
      else /* else, filter mode = pass */
      {
	 /* copy old block list => new list */
         done = FirstBlock(segmentList,&start, &stop);
         while( done != NIL)
         {
            InsBlock(outputList,start, stop);
            done = NextBlock(segmentList,&start,&stop);
         }
      }
      if(truncation == True) /* if truncation is on, modify the list to end on a "full block" boundary. */
      {
         for(i = 0; i < (MAX_DIG_CHNL + MAX_ANLG_CHNL);i++)
            eventCount[i] = 0;

	 /* reset the new (output) list */
         fseek(fp,32,0);

	 /* get the first block boundaries */
         done = FirstBlock(outputList,&begin,&end);

	 /* while the block is non-null */
         trunc = False;
         while((done != NIL) && (trunc != True)) 
         {
	    /* advance the file pointer to within the block boundaries */
            code = getw(fp);
            time = getw(fp);
            while(time < begin)
            {
               code = getw(fp);
               time = getw(fp);
            }
	    /* while the code is within the boundaries ... */
            while((time <= end) && (trunc != True) && (done != NIL))
            {
               if(code < 1000) 
                  if(writeList[sf.indexMap[code]] == True)
                     if(++eventCount[sf.indexMap[code]] > events)
                     {
                        trunc = True;
                        done = CurrentBlock(outputList,&truncBegin,&truncEnd);
                     }
               /* get the next code & time */
               if(trunc != True)
               {
                  code = getw(fp);
                  time = getw(fp);
               }
               if(code == CODEFLAG && time == ENDFLAG)
                  done  = NIL;
            }
	    /* get the next block in the new list */
            if(trunc != True)
               done = NextBlock(outputList,&begin,&end);
         }
         if(trunc == True)
            TruncList(outputList,truncBegin,truncEnd);
      };
   }
   else
   {
      InsBlock(outputList,0,sf.lastTime);
   }
   for(i = 0; i < (MAX_DIG_CHNL + MAX_ANLG_CHNL);i++)
      eventCount[i] = 0;

   /* reset the new (output) list */
   fseek(fp,32,0);

   /* get the first block boundaries */
   done = FirstBlock(outputList,&begin,&end);

   /* while the block is non-null */
   trunc = False;
   while((done != NIL) && (trunc != True))
   {
      /* advance the file pointer to within the block boundaries */
      code = getw(fp);
      time = getw(fp);
      while(time < begin)
      {
         code = getw(fp);
         time = getw(fp);
      }

      /* write markB code if required */
      if((markBnE == True) && (BlockCount(segmentList) > 0))
      {
         if ((scaleFactor == 1) && ((FileType == 2) || (FileType == 3)))
            time = time * 5;
         else if ((scaleFactor == 5) && (FileType == 1))
            time = time / 5;

         fprintf(fpOut,format,MARKB,begin);
      }

      /* while the code is within the boundaries ... */
      while((time <= end) && (trunc != True))
      {
	 /* write codes if set in the write-list and not past trunc limit */
         if(code > 1000) 
         {
            if((FileType == 1) || (FileType == 2) || (FileType == 3))
	       if(writeList[sf.indexMap[code / 4096 + 1000]] == True)
	       {
		  if ((scaleFactor == 1) && ((FileType == 2) || (FileType == 3)))
		     time = time * 5;
		  else if ((scaleFactor == 5) && (FileType == 1))
		     time = time / 5;
		  fprintf(fpOut,format,code,time);
	       }
         }
         else
            if(writeList[sf.indexMap[code]] == True)
            {
               if(++eventCount[sf.indexMap[code]] > events)
                  trunc = True;
               else
               {
                  if ((scaleFactor == 1) && ((FileType == 2) || (FileType == 3)))
                     time = time * 5;
                  else if ((scaleFactor == 5) && (FileType == 1))
                     time = time / 5;
                  
                  fprintf(fpOut,format,code,time);
               }
            }
                 
	 /* get the next code & time */
	 if(trunc != True)
	 {
	    code = getw(fp);
	    time = getw(fp);
	 }
	 if(code == CODEFLAG && time == ENDFLAG)
	 {
	    trunc = True;
	    done  = NIL;
	 }
      }
      /* write markE code if required */
      if((markBnE == True) && (BlockCount(segmentList) > 0))
      {
         if ((scaleFactor == 1) && ((FileType == 2) || (FileType == 3)))
            time = time * 5;
         else if ((scaleFactor == 5) && (FileType == 1))
            time = time / 5;
         
         fprintf(fpOut,format,MARKE,end);
      }

      /* get the next block in the new list */
      if(trunc != True)
         done = NextBlock(outputList,&begin,&end);
   }

   /* close the output file */
   fclose(fpOut);
   DestroyMyList (outputList);
}

static void
intersect (int outputList, int newList, int trunc_time)
{
   int edit_start, edit_end;
   int intersect_start, intersect_end;
   long long intersect_offset;
   int n;

   CurrentBlock (outputList, &edit_start, &edit_end) || DIE;
   if (trunc_time < edit_end)
      edit_end = trunc_time;
   for (n = 0; n < hdt_block_count; n++) {
      if (hdtblkend[n] * scaleFactor <= edit_start || hdtblkstart[n] * scaleFactor > edit_end)
	 continue;
      if (hdtblkstart[n] * scaleFactor >= edit_start)
      {
	 intersect_start = hdtblkstart[n];
	 intersect_offset = startbyte[n];
      }
      else 
      {
	 intersect_start =  edit_start / scaleFactor;
	 intersect_offset = startbyte[n] + (  halfms2samp (intersect_start)
					    - halfms2samp (hdtblkstart[n] )) * BYTES_PER_SAMPLE * hstotal;
      }
      intersect_end = (hdtblkend[n] * scaleFactor <= edit_end
		       ? hdtblkend[n]
		       : edit_end / scaleFactor + 1);
      InsHdtBlock (newList, intersect_start, intersect_end, intersect_offset);
   }
}

/* function to write to a file */
void writeOutputFile()
{
   extern FILE *fp9;
   FILE *fpOut, *fpOuthdt;
   char *format;
   char dummy1[80];
   Boolean truncation, markBnE, trunc;
   int events, eventCount[MAX_DIG_CHNL + MAX_ANLG_CHNL], outputList, i;
   int code, time, writeList[MAX_DIG_CHNL+MAX_ANLG_CHNL], done, begin, end, start, stop;
   int truncBegin, truncEnd;
   Arg arglist[4];
   int i1, dohdt, numblocks, newList = 0, j;
   long long n, sample_count, seekbyte;
   char  buff1, buff2;
   static short int hdtvalue, newMin[MAX_HDT_CHNL], newMax[MAX_HDT_CHNL];

   /* open output file for writing */
	
   fpOut=fopen(mystring,"w");
   strcpy(dummy1,"chmod 666");
   strcat(dummy1, "  ");
   strcat(dummy1,mystring);
   if (system(dummy1));

   if(fpOut == NULL)
      printf("Unable to open output file for writing\n");

   /* get output file type (ADT or BDT, DDT, EDT), set format and write header */
   if(FileType == 1)
   {
      format = "%5d%8d\n";
      fprintf(fpOut,format,11,1111111);
      fprintf(fpOut,format,11,1111111);
   }
   else if(FileType == 2)
   {
      format = "%5d%8d\n";
      fprintf(fpOut,format,22,2222222);
      fprintf(fpOut,format,22,2222222);
   }
   else if(FileType == 3)
   {
      format = "%5d%10d\n";
      fprintf(fpOut,format,33,3333333);
      fprintf(fpOut,format,33,3333333);
   }
   else
      format = "%2d%8d\n";

   /* get truncation switch setting */
   truncation = truncationToggleVal;

   /* get truncation event count */
   if(truncation == True)
      events = truncEvents();
   else
      events = INT_MAX;		/* largest possible integer */

   /* create the write list array */
   for(i = 0; i < (MAX_DIG_CHNL + MAX_ANLG_CHNL); i++)
      writeList[i] = False;

   for(i = 0; i < sf.dchan; i++)
      if (PassSelCh)
	 writeList[i] = dchanSet(i);
      else
	 writeList[i] = !dchanSet(i);

   if((FileType == 1) || (FileType == 2) || (FileType == 3))
      for(i = 0; i < sf.achan; i++) {
	 if (PassSelCh)
	    writeList[MAX_DIG_CHNL + i] = achanSet(i);
	 else
	    writeList[MAX_DIG_CHNL + i] = !achanSet(i);
      }

   /* get markE & markB switch setting */
   if (FilterPopOpened)
      markBnE = markCodes();
   else
      markBnE = 0;

   /* initialize new (output) block list */
   outputList = InitList();
   if(outputList == NIL)
      printf("unable to initialize output block List\n");
            
   /* check if blocks are currently defined */
   if(BlockCount(segmentList) > 0) 
   {
      if(passBlocks() == False)
      {
	 /* invert old block list => new list */
	 done = FirstBlock(segmentList,&start, &stop);
	 begin = -1;

	 while( done != NIL)
	 {
	    if((begin + 1) < (start - 1))
	    {
	       done = InsBlock(outputList,begin + 1,start - 1);
	       if(done == NIL)
		  printf("error inserting into outputList\n");
	       else
		  if (0)
		     printf("inserted %d -> %d into outputList\n",begin+1,start-1);
	    }
	    begin = stop;
	    done = NextBlock(segmentList,&start,&stop);
	 }
	 if((begin + 1) < sf.lastTime)
	    InsBlock(outputList,begin + 1,sf.lastTime);
      }

      /* else, filter mode = pass */
      else
      {
	 /* copy old block list => new list */
	 done = FirstBlock(segmentList,&start, &stop);
	 while( done != NIL)
	 {
	    InsBlock(outputList,start, stop);
	    done = NextBlock(segmentList,&start,&stop);
	 }
      }
      /* if truncation is on, modify the list to end on a "full block" boundary. */
      if(truncation == True)
      {  
	 for(i = 0; i < (MAX_DIG_CHNL + MAX_ANLG_CHNL);i++)
	    eventCount[i] = 0;

	 /* reset the new (output) list */
	 fseek(fp,32,0);

	 /* get the first block boundaries */
	 done = FirstBlock(outputList,&begin,&end);

	 /* while the block is non-null */
	 trunc = False;
	 while((done != NIL) && (trunc != True)) 
	 {
	    /* advance the file pointer to within the block boundaries */
	    code = getw(fp);
	    time = getw(fp);
	    while(time < begin)
	    {
	       code = getw(fp);
	       time = getw(fp);
	    }
	    /* while the code is within the boundaries ... */
	    while((time <= end) && (trunc != True) && (done != NIL))
	    {
	       if(code < 1000) 
		  if(writeList[sf.indexMap[code]] == True)
		     if(++eventCount[sf.indexMap[code]] > events)
		     {
			trunc = True;
			done = CurrentBlock(outputList,&truncBegin,&truncEnd);
		     }
	       /* get the next code & time */
	       if(trunc != True)
	       {
		  code = getw(fp);
		  time = getw(fp);
	       }
	       if(code == CODEFLAG && time == ENDFLAG)
		  done  = NIL;
	    }
	    /* get the next block in the new list */
	    if(trunc != True)
	       done = NextBlock(outputList,&begin,&end);
	 }
	 if(trunc == True)
	    TruncList(outputList,truncBegin,truncEnd);
      };
   }
   else
   {
      InsBlock(outputList,0,sf.lastTime);
   }
   for(i = 0; i < (MAX_DIG_CHNL + MAX_ANLG_CHNL);i++)
      eventCount[i] = 0;

   /* reset the new (output) list */
   fseek(fp,32,0);

   /* get the first block boundaries */
   done = FirstBlock(outputList,&begin,&end);

   /* while the block is non-null */
   trunc = False;
   dohdt = 0;
   if (hdt)
   {
      hdtchannels = 0;
      for(j = 0; j < NextLabel2; ++j)
      {
	 i = 0;
	 XtSetArg(arglist[i], XmNset, &state); i++;
	 XtGetValues(SignalLabel2W[j], arglist, i);
	 if (PassSelCh)
	    hdtselect[j] = state;
	 else
	    hdtselect[j] = !state;
	 if (hdtselect[j])
	    hdtchannels++;
      }
      if (hdtchannels) {
	 newList = InitList ();
	 dohdt = 1;
      }
   }

   code = getw(fp);
   time = getw(fp);
   while((done != NIL) && (trunc != True)) 
   {
      int trunc_time = INT_MAX;
      /* advance the file pointer to within the block boundaries */
      while(time < begin)
      {
	 code = getw(fp);
	 time = getw(fp);
      }

      /* write markB code if required */
      if((markBnE == True) && (BlockCount(segmentList) > 0))
      {
	 if ((scaleFactor == 1) && ((FileType == 2) || (FileType == 3)))
            time = time * 5;
	 else if ((scaleFactor == 5) && (FileType == 1))
            time = time / 5;

	 fprintf(fpOut,format,MARKB,begin);
      }

      /* while the code is within the boundaries ... */
      while((time <= end) && (trunc != True))
      {
	 /* write codes if set in the write-list and not past trunc limit */
	 if(code > 1000) 
	 {
	    if((FileType == 1) || (FileType == 2) || (FileType == 3))
	       if(writeList[sf.indexMap[code / 4096 + 1000]] == True)
	       {
		  if ((scaleFactor == 1) && ((FileType == 2) || (FileType == 3)))
		     time = time * 5;
		  else if ((scaleFactor == 5) && (FileType == 1))
		     time = time / 5;
		  fprintf(fpOut,format,code,time);
	       }
	 }
	 else 
	    if(writeList[sf.indexMap[code]] == True)
	    {
	       if(++eventCount[sf.indexMap[code]] > events)
	       {
		  trunc = True;
		  trunc_time = time;
	       }
	       else
	       {
		  if ((scaleFactor == 1) && ((FileType == 2) || (FileType == 3)))
		     time = time * 5;
		  else if ((scaleFactor == 5) && (FileType == 1))
		     time = time / 5;

		  fprintf(fpOut,format,code,time);
	       }
	    }
                 
	 /* get the next code & time */
	 if(trunc != True)
	 {
	    code = getw(fp);
	    time = getw(fp);
	 }
	 if(code == CODEFLAG && time == ENDFLAG)
	 {
	    trunc = True;
	    done  = NIL;
	 }
      }
      /* write markE code if required */
      if((markBnE == True) && (BlockCount(segmentList) > 0))
      {
	 if ((scaleFactor == 1) && ((FileType == 2) || (FileType == 3)))
            time = time * 5;
	 else if ((scaleFactor == 5) && (FileType == 1))
            time = time / 5;

	 fprintf(fpOut,format,MARKE,end);
      }

      if (dohdt) intersect (outputList, newList, trunc_time);

      /* get the next block in the new list */
      if(trunc != True)
	 done = NextBlock(outputList,&begin,&end);
   }

   /* close the output file */
   fclose(fpOut);
   
   if (!dohdt)
      return;

   {
      char *hdt_file_name = strdup (mystring);
      int len = strlen (mystring);
      len >= 4 || DIE;
      hdt_file_name[len - 4] = 0;
      strcat(hdt_file_name, ".hdt");
      fpOuthdt = fopen(hdt_file_name,"w+b");
      free (hdt_file_name);
   }
   /* write out hdt header information */

   buff1 = (int)samplerate / 256;
   buff2 = (int)samplerate % 256;
   fputc(buff1, fpOuthdt);
   fputc(buff2, fpOuthdt);

   buff1 = hdtchannels / 256;
   buff2 = hdtchannels % 256;
   fputc(buff1, fpOuthdt);
   fputc(buff2, fpOuthdt);

   for(j = 0; j < hstotal; ++j)
      if(hdtselect[j] == 1)
	 fseeko64 (fpOuthdt, 4, SEEK_CUR);

   numblocks = BlockCount (newList);

   /* write the number of hdt blocks */
   {
      char  shortchar1, shortchar2, shortchar3, shortchar4;
      short int shortblock1, shortblock2;

      shortblock1 = numblocks / 32768;
      shortchar1 = shortblock1 /256;
      shortchar2 = shortblock1 -256*shortchar1; 
      shortblock2 = numblocks - shortblock1 * 32768;
      shortchar3 = shortblock2 /256;
      shortchar4 = shortblock2 -256*shortchar3; 
      fputc(shortchar1, fpOuthdt);
      fputc(shortchar2, fpOuthdt);
      fputc(shortchar3, fpOuthdt);
      fputc(shortchar4, fpOuthdt);
   }


   /* write the locations of each blocks start and stop time to be passed*/ 

   for (done = FirstBlock (newList, &start, &end);
	done != NIL;
	done = NextBlock  (newList, &start, &end))
   {
      char charbdt1, charbdt2, charbdt3, charbdt4, charbdt5, charbdt6, charbdt7, charbdt8;
      short int shortbdt1, shortbdt2, shortbdt3, shortbdt4;

      shortbdt1 = start / 32768;
      charbdt1 = shortbdt1 / 256; 
      charbdt2 = shortbdt1 - 256* charbdt1;
      shortbdt2 = start - shortbdt1 * 32768;
      charbdt3 = shortbdt2 / 256;
      charbdt4 = shortbdt2 - 256*charbdt3;

      shortbdt3 = end / 32768;
      charbdt5 = shortbdt3 / 256; 
      charbdt6 = shortbdt3 - 256 * charbdt5;
      shortbdt4 = end - shortbdt3 * 32768;
      charbdt7 = shortbdt4 / 256;
      charbdt8 = shortbdt4 - 256 * charbdt7;

      fputc(charbdt1, fpOuthdt);
      fputc(charbdt2, fpOuthdt);
      fputc(charbdt3, fpOuthdt);
      fputc(charbdt4, fpOuthdt);
      fputc(charbdt5, fpOuthdt);
      fputc(charbdt6, fpOuthdt);
      fputc(charbdt7, fpOuthdt);
      fputc(charbdt8, fpOuthdt);
   }
   for (n = 0; n < MAX_HDT_CHNL; n++) newMin[n] = SHRT_MAX;
   for (n = 0; n < MAX_HDT_CHNL; n++) newMax[n] = SHRT_MIN;
   for (done = FirstHdtBlock (newList, &start, &end, &seekbyte);
	done != NIL;
	done = NextHdtBlock  (newList, &start, &end, &seekbyte))
   {
      fseeko64 (fp9, seekbyte, SEEK_SET) == 0 || DIE;
      sample_count = halfms2samp (end) - halfms2samp (start);
      for (n = 0; n < sample_count; n++)
	 for (i1 = 0; i1 < hstotal; i1++)
	 {
	    fread (&hdtvalue, sizeof hdtvalue, 1, fp9) == 1 || DIE;
	    if (hdtselect[i1] == 1)
	    {
	       short tmp = hdtvalue;
	       SWAB (tmp);
	       if(tmp < newMin[i1])
		  newMin[i1] = tmp;
	       if(tmp > newMax[i1])
		  newMax[i1] = tmp;
	       fwrite (&hdtvalue, sizeof hdtvalue, 1, fpOuthdt);
	    }
	 }
   }
   DestroyMyList (newList);
   DestroyMyList (outputList);

   fflush(fpOuthdt);
   fseeko64(fpOuthdt, 4, SEEK_SET);
   for(i1 = 0; i1 < hstotal; i1++)
      if(hdtselect[i1] == 1)
      {
	 SWAB (newMin[i1]);
	 fwrite(&newMin[i1], sizeof(newMin[i1]), 1, fpOuthdt);
	 SWAB (newMax[i1]);
	 fwrite(&newMax[i1], sizeof(newMax[i1]), 1, fpOuthdt);
      }
   fflush(fpOuthdt);
   fclose(fpOuthdt);
} 

/* Local Variables: */
/* c-basic-offset: 3 */
/* c-file-offsets:  ((substatement-open . 0)) */
/* End: */
