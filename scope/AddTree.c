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


#include "std.h"
#include <Xm/Form.h>
#include <Xm/MwmUtil.h>
#include "filesup.h"
#include "AddTree.h"
#include "dispUnit64.h"

long getOffset();
void reInitScope();
void mergeNewEvents();
void initNewEvents();
int nextNewCodeId();

/* global variables (widgets,etc....) */

char codeStr[10];
int newCodeId;

/*** ZZZ
Display *Current_display;
***/
Widget codePop; 
Widget codePopFormW;
Widget manualPop;
Widget cyclePop;
Widget offsetPop;
Widget periodEditW;
Widget cycOffsetEditW;
Widget offsetIdEditW;
Widget offsetOffsetEditW;
Widget newOffsetValueW;
Widget codePop1W;
Widget codePop2W;
Widget codePop3W;
Widget AddACodeW;
Widget manPopFormW;
Widget cycPopFormW;
Widget offsetPopFormW;


Widget manIdOutStatW;
Widget manIdOutCodeW;
Widget manCtOutStatW;

Widget peW;



int newEventTally;
Boolean manPop_up;

void makeWarningPop();
void makemanPop();
void makecycPop();
void makeoffsetPop();

extern void refresh();

extern Widget main_window;

extern int leftTime, marginTime;
extern long screenLeft;
extern int firstTime;


/* callback routines for widget events */
void codeEsc(w, closure, call_data)
   Widget w;
   XtPointer closure;
   XtPointer call_data;
{
   XtUnmanageChild(codePopFormW);
}


/* Callback routine for "Manual" Push Button */
void manual(w, client_data, call_data)
   Widget w;
   XtPointer client_data;
   XtPointer call_data;
{
   Widget parent = client_data;
   
   newCodeId = nextNewCodeId();
   sprintf(codeStr,"%d",newCodeId);

   initNewEvents();

   makemanPop(parent);

   XmString s = XmStringCreateLocalized (codeStr);
   XtVaSetValues (manIdOutCodeW, XmNlabelString, s, NULL);
   XmStringFree (s);

   s = XmStringCreateLocalized ("0");
   XtVaSetValues (manCtOutStatW, XmNlabelString, s, NULL);
   XmStringFree (s);
}


/* Callback routine for "Offset" Push Button */
void offset(w, closure, call_data)
   Widget w;
   XtPointer closure;
   XtPointer call_data;
{
   newCodeId = nextNewCodeId();
   sprintf(codeStr,"%d",newCodeId);

   makeoffsetPop();
}



/* Callback routine for Esc Push Button Release */
static void
manualEsc(w, client_data, call_data)
   Widget w;
   XtPointer client_data;
   XtPointer call_data;
{
   Widget dialog = (Widget) client_data;

   XtUnmanageChild (dialog);
   fclose(newEvents);
   newEvents = 0;
}

Widget progress;

Boolean
mergeEventsWorkProc (XtPointer client_data)
{
   progress = XmCreateWorkingDialog (client_data, "working", NULL, 0);
   XtVaSetValues (progress, XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL, NULL);
   XtUnmanageChild (XtNameToWidget (progress, "OK"));
   XtUnmanageChild (XtNameToWidget (progress, "Cancel"));
   XtUnmanageChild (XtNameToWidget (progress, "Help"));
   XtManageChild (progress);
   SetWatchCursor (progress);
   SetWatchCursor (app_shell);
   mergeNewEvents();
   reInitScope();
   screenLeft = getOffset(leftTime/marginTime*marginTime);
   refresh();
   XUndefineCursor (XtDisplay (app_shell), XtWindow (app_shell));
   XUndefineCursor (XtDisplay (progress), XtWindow (progress));
   XtDestroyWidget (progress);
   progress = NULL;
   return True;
}

/* Callback routine for Done Push Button Release */
void manualDone(Widget w, XtPointer client_data, XtPointer call_data)
{
   XtAppAddWorkProc (app_context, mergeEventsWorkProc, client_data);
   XtUnmanageChild (client_data);
}

/* Callback routine for Esc Push Button Release */
void cyclicEsc(w, closure, call_data)
   Widget w;
   XtPointer closure;
   XtPointer call_data;
{
   XtPopdownIDRec PopdownId;

   PopdownId.shell_widget = cyclePop;
   PopdownId.enable_widget = codePop2W;
   XtCallbackPopdown(w,(XtPointer)&PopdownId,NULL);
}


/* a routine to create a periodic event train */
void createPeriodicEvents(periodStr,offsetStr)
     double periodStr;
     double offsetStr;
{
   int  event, offset, period;

   newCodeId = nextNewCodeId();

   newEventTally = 0;

   newEvents = tmpfile ();

   if(newEvents == NULL)
   {
      printf("Unable to open newEvents.bin for reading & writing\n");
      DIE;
   };

   offset = (firstTime + offsetStr * ticks_per_second);
   period = (periodStr * ticks_per_second);
	 
   event = offset;

   while(event < sf.lastTime)
   {
      putw(event,newEvents);
      newEventTally++;
      event = event + period;
   }
}
	

/* a routine to create an offset event train */
void createOffsetEvents(codeStr,offsetStr)
   int codeStr;
   double offsetStr;
{
   int offset, code, time, event;
   Boolean valid;
   int i;
   valid = False;

   for(i = 0; i < sf.dchan; i++)
      if(codeStr == sf.ids[i])
	 valid = True;

   if(valid == True)
   {
      newEvents = tmpfile ();

      if(newEvents == NULL)
      {
         printf("Unable to open newEvents.bin for reading & writing\n");
         DIE;
      }

      offset = (offsetStr * ticks_per_second);

      newEventTally = 0;

      fseek(fp,32L,0);

      time = 0;
      while(time < ENDFLAG)
      {
         code = getw(fp);
	 time = getw(fp);
	
         if(code == codeStr)
         {
	    event = time + offset;
	    if(event > 0 && event < sf.lastTime)
            {
	       newEventTally++;
	       putw(event,newEvents);
	    }
	 }
      }
   }
}

void cyclicDone(w, closure, call_data)
   Widget w;
   XtPointer closure;
   XtPointer call_data;
{
   XtPopdownIDRec PopdownId;
   char *buff5, *buff6;
   float offsetStr, periodStr;

   buff5 = XmTextGetString(cycOffsetEditW);
   buff6 = XmTextGetString(periodEditW);

   offsetStr = 0;
   sscanf(buff5,"%f",&offsetStr);
   if (sscanf(buff6,"%f",&periodStr) != 1 || periodStr <= 0) {
     makeWarningPop ("Value Error", "Bad value for period");
     return;
   }

   createPeriodicEvents(periodStr,offsetStr);
   mergeNewEvents();
   reInitScope();
   screenLeft = getOffset(leftTime/marginTime*marginTime);
   refresh(); 

   PopdownId.shell_widget = cyclePop;
   PopdownId.enable_widget = codePop2W;
   XtCallbackPopdown(w,(XtPointer)&PopdownId,NULL);
}


/* Callback routine for Esc Push Button Release */
void offsetEsc(w, closure, call_data)
     Widget w;
     XtPointer closure;
     XtPointer call_data;
{
   XtPopdownIDRec PopdownId;

   PopdownId.shell_widget = offsetPop;
   PopdownId.enable_widget = codePop3W;
   XtCallbackPopdown(w,(XtPointer)&PopdownId,NULL);
}

void offsetDone(w, closure, call_data)
   Widget w;
   XtPointer closure;
   XtPointer call_data;
{
   XtPopdownIDRec PopdownId;
   char *buff1, *buff2;
   float offsetStr;
   int codeStr;

   buff1 = XmTextGetString(offsetOffsetEditW);
   buff2 = XmTextGetString(offsetIdEditW);

   sscanf(buff1,"%f",&offsetStr);
   sscanf(buff2,"%d",&codeStr);

   createOffsetEvents(codeStr,offsetStr);
   mergeNewEvents();
   reInitScope();
   screenLeft = getOffset(leftTime/marginTime*marginTime);

   refresh(); 
   PopdownId.shell_widget = offsetPop;
   PopdownId.enable_widget = codePop3W;
   XtCallbackPopdown(w,(XtPointer)&PopdownId,NULL);
}

static void
manPop_popdown_CB (Widget dialog, XtPointer client_data, XtPointer call_data)
{
   manPop_up = 0;
}

static void
manPop_popup_CB (Widget dialog, XtPointer client_data, XtPointer call_data)
{
   manPop_up = 1;
}

void makemanPop(Widget parent)
{
  Arg arglist[20];
  Cardinal i;

  static Widget manPopFormW;

  if (fp == NULL) {
    makeWarningPop ("Warning", "A file must be opened first");
    return;
  }

  /* This section creates a popup for the manual code option */
	
  if (manualPop == 0) {
     Widget manPopTitleW;
     Widget manPopStatW;
     Widget manPopDoneW;
     Widget manPopEscW;
     Widget manCtTitleStatW;
     i = 0;
     XtSetArg(arglist[i], XmNmwmDecorations, MWM_DECOR_ALL); i++;
     XtSetArg (arglist[i], XmNdeleteResponse, XmUNMAP); i++;
     manualPop=XmCreateDialogShell(parent,"manPop",arglist,i);
     XtAddCallback (manualPop, XmNpopdownCallback, manPop_popdown_CB, NULL);
     XtAddCallback (manualPop, XmNpopupCallback, manPop_popup_CB, NULL);
     XtAddCallback (manualPop, XmNpopupCallback, map_dialog, NULL);

     /* Set up Forms widget */
     i = 0;
     manPopFormW = (Widget)XmCreateForm(manualPop, "manPopForm", arglist, i);

     i = 0;
     XtSetArg(arglist[i], XmNwidth, 310); i++;
     XtSetArg(arglist[i], XmNheight,150); i++;
     XtSetArg(arglist[i], XmNy,220);i++;
     XtSetArg(arglist[i], XmNx,840);i++;
     XtSetValues(XtParent(manPopFormW),arglist,i);

     /* Set up Row Column widget */
     i = 0;
     /* Argument for form parent */
     XtSetArg(arglist[i], XmNborderWidth, 1); i++;
     XtSetArg(arglist[i], XmNnumColumns, 3); i++;
     XtSetArg(arglist[i], XmNpacking, XmPACK_COLUMN); i++;
     XtSetArg(arglist[i], XmNspacing, 2); i++;
     manPopTitleW=XmCreateRowColumn(manPopFormW, "manTitle", arglist, i);
     XtManageChild(manPopTitleW);
                       
     /* Set up Push button widget inside title bar */
     i = 0;
     manPopEscW=XmCreatePushButton(manPopTitleW,"Esc",arglist,i);
     XtManageChild(manPopEscW);
     XtAddCallback(manPopEscW,XmNactivateCallback,manualEsc,(XtPointer)manPopFormW);

     /* Set up Push button widget inside title bar */
     i = 0;
     manPopStatW=XmCreateLabel(manPopTitleW,"Manual Code",arglist,i);
     XtManageChild(manPopStatW);

     /* Set up Push button widget inside title bar */
     i = 0;
     manPopDoneW=XmCreatePushButton(manPopTitleW,"Done",arglist,i);
     XtManageChild(manPopDoneW);
     XtAddCallback(manPopDoneW,XmNactivateCallback,manualDone,manPopFormW);

     /* Set up static text widget inside row column widget */
     i = 0;
     XtSetArg(arglist[i], XmNy,50); i++;
     XtSetArg(arglist[i], XmNx,10); i++;
     manIdOutStatW=XmCreateLabel(manPopFormW,"Code Id :",arglist,i);
     XtManageChild(manIdOutStatW);

     /* Set up static text widget inside row column widget */
     i = 0;
     XtSetArg(arglist[i], XmNy,50); i++;
     XtSetArg(arglist[i], XmNx,80); i++;
     manIdOutCodeW=XmCreateLabel(manPopFormW,codeStr,arglist,i);
     XtManageChild(manIdOutCodeW);

     /* Set up static text widget inside row column widget */
     i = 0;
     /* static text arguments */
     XtSetArg(arglist[i], XmNy,80); i++;
     XtSetArg(arglist[i], XmNx,10); i++;
     manCtTitleStatW=XmCreateLabel(manPopFormW,"Events  :",arglist,i);
     XtManageChild(manCtTitleStatW);

     /* Set up static text widget inside row column widget */
     i = 0;
     XtSetArg(arglist[i], XmNy,80); i++;
     XtSetArg(arglist[i], XmNx,80); i++;
     manCtOutStatW=XmCreateLabel(manPopFormW,"0",arglist,i);
     XtManageChild(manCtOutStatW);
  }
  else XtRemoveCallback (manualPop, XmNpopupCallback, map_dialog, NULL);

  XtManageChild(manPopFormW);
}

/* This section creates a popup for the periodic code option */
void makecycPop(Widget w, XtPointer closure, XtPointer call_data)
{
   Widget cycPopFormW;
   Widget cycPopTitleW;
   Widget cycPopStatW;
   Widget cycPopDoneW;
   Widget cycPopEscW;

   Arg arglist[20];
   Cardinal i;

   if (fp == NULL) {
      makeWarningPop ("Warning", "A file must be opened first");
      return;
   }

   i = 0;
   codePop = XmCreateDialogShell(main_window,"codePop",arglist,i);

   i = 0;
   cyclePop=XmCreateDialogShell(codePop,"cycPop",arglist,i);
        
   /* Set up Forms widget */
   i = 0;
   XtSetArg(arglist[i], XmNwidth, 360); i++;
   XtSetArg(arglist[i], XmNheight, 150); i++;
   cycPopFormW = (Widget)XmCreateForm(cyclePop, "cycPopForm", arglist, i);
   XtManageChild(cycPopFormW);

   /* Set up Title bar widget */
   i = 0;
   XtSetArg(arglist[i], XmNborderWidth, 1); i++;
   XtSetArg(arglist[i], XmNnumColumns, 3); i++;
   XtSetArg(arglist[i], XmNpacking, XmPACK_COLUMN); i++;
   XtSetArg(arglist[i], XmNspacing, 13); i++;
   cycPopTitleW=XmCreateRowColumn(cycPopFormW, "cycTitle", arglist, i);
   XtManageChild(cycPopTitleW);
                  
   /* Set up Push button widget inside title bar */
   i = 0;
   /* Argument for push button widget */
   cycPopEscW=XmCreatePushButton(cycPopTitleW,"Esc",arglist,i);
   XtManageChild(cycPopEscW);
   XtAddCallback(cycPopEscW,XmNactivateCallback,cyclicEsc,NULL);

   /* Set up static text widget inside title bar */
   i = 0;
   cycPopStatW=XmCreateLabel(cycPopTitleW,"Periodic Code",arglist,i);
   XtManageChild(cycPopStatW);

   /* Set up Push button widget inside title bar */
   i = 0;
   /* Argument for push button widget */
   cycPopDoneW=XmCreatePushButton(cycPopTitleW,"Done",arglist,i);
   XtManageChild(cycPopDoneW);
   XtAddCallback(cycPopDoneW,XmNactivateCallback,cyclicDone,NULL);

   /* Set up static text widget inside title bar */
   i = 0;
   /* static text arguments */
   XtSetArg(arglist[i], XmNx, 2); i++;
   XtSetArg(arglist[i], XmNy, 50); i++;
   cycPopStatW=XmCreateLabel(cycPopFormW,"Period :",arglist,i);
   XtManageChild(cycPopStatW);

   /* Set up static text widget inside row column widget */
   i = 0;
   /* static text arguments */
   XtSetArg(arglist[i], XmNx, 100); i++;
   XtSetArg(arglist[i], XmNy, 50); i++;
   periodEditW=XmCreateText(cycPopFormW,"Period",arglist,i);
   XtManageChild(periodEditW);

   /* Period input text edit widget */

   /* Argument for text edit widget */
   i = 0;
   XtSetArg(arglist[i], XmNx, 2); i++;
   XtSetArg(arglist[i], XmNy, 100); i++;
   peW=XmCreateLabel(cycPopFormW,"Offset :",arglist,i);
   XtManageChild(peW);

   /* Set up static text widget inside row column widget */
   i = 0;
   /* static text arguments */
   XtSetArg(arglist[i], XmNx, 100); i++;
   XtSetArg(arglist[i], XmNy, 100); i++;
   cycOffsetEditW=XmCreateText(cycPopFormW,"Offset",arglist,i);
   XtManageChild(cycOffsetEditW);

   /* XtRealizeWidget(cyclePop);*/
}


/* This section creates a popup for the offset code option */
void makeoffsetPop()
{
   Widget offsetPopFormW;
   Widget offsetPopTitleW;
   Widget offsetPopStatW;
   Widget offsetPopDoneW;
   Widget offsetPopEscW;
   Widget offsetIdStatW;
   Widget offsetOffsetStatW;
   Widget newOffsetStatW;

   Arg arglist[20];
   Cardinal i;

   i = 0;
   codePop = XmCreateDialogShell(main_window,"codePop",arglist,i);

   if (fp == NULL) {
      makeWarningPop ("Warning", "A file must be opened first");
      return;
   }

   i = 0;
   offsetPop=XmCreateDialogShell(codePop,"offsetPop",arglist,i);

   /* Set up Forms widget */
   i = 0;
   XtSetArg(arglist[i], XmNwidth, 350); i++;
   XtSetArg(arglist[i], XmNheight, 250); i++;
   offsetPopFormW = (Widget)XmCreateForm(offsetPop, "offsetPopForm", arglist, i);
   XtManageChild(offsetPopFormW);

   /* Set up Title bar widget */
   i = 0;
   XtSetArg(arglist[i], XmNborderWidth, 1); i++;
   XtSetArg(arglist[i], XmNnumColumns, 3); i++;
   XtSetArg(arglist[i], XmNpacking, XmPACK_COLUMN); i++;
   XtSetArg(arglist[i], XmNspacing, 36); i++;
   offsetPopTitleW=XmCreateRowColumn(offsetPopFormW, "offsetTitle", arglist, i);
   XtManageChild(offsetPopTitleW);

   /* Set up Push button widget inside title bar */
   i = 0;
   /* Argument for push button widget */
   offsetPopEscW=XmCreatePushButton(offsetPopTitleW, "Esc", arglist, i);
   XtManageChild(offsetPopEscW);
   XtAddCallback(offsetPopEscW,XmNactivateCallback,offsetEsc,NULL);

   /* Set up static text widget inside title bar */
   i = 0;
   /* Arguments for parent title bar widget */
   offsetPopStatW=XmCreateLabel(offsetPopTitleW, "Offset Code",	arglist, i);
   XtManageChild(offsetPopStatW);

   /* Set up Push button widget inside title bar */
   i = 0;
   /* Argument for push button widget */
   offsetPopDoneW=XmCreatePushButton(offsetPopTitleW, "Done", arglist, i);
   XtManageChild(offsetPopDoneW);
   XtAddCallback(offsetPopDoneW,XmNactivateCallback,offsetDone,NULL);

   /* Set up static text widget inside row column widget */
   i = 0;
   XtSetArg(arglist[i], XmNy, 50); i++;
   XtSetArg(arglist[i], XmNx, 2); i++;
   offsetIdStatW=XmCreateLabel(offsetPopFormW, "Code to offset :", arglist, i);
   XtManageChild(offsetIdStatW);

   /* Period input text edit widget */

   /* Argument for text edit widget */
   i = 0;
   XtSetArg(arglist[i], XmNy, 50); i++;
   XtSetArg(arglist[i], XmNx, 150); i++;
   offsetIdEditW=XmCreateText(offsetPopFormW, "offsetIdEdit", arglist, i);
   XtManageChild(offsetIdEditW);

   /* Set up static text widget inside row column widget */
   i = 0;
   XtSetArg(arglist[i], XmNy, 100); i++;
   XtSetArg(arglist[i], XmNx, 2); i++;
   offsetOffsetStatW=XmCreateLabel(offsetPopFormW, "Offset amount  :", arglist, i);
   XtManageChild(offsetOffsetStatW);

   /* Set up offset text edit widget */

   /* Argument for text edit widget */
   i=0;
   XtSetArg(arglist[i], XmNy, 100); i++;
   XtSetArg(arglist[i], XmNx, 150); i++;
   offsetOffsetEditW=XmCreateText(offsetPopFormW, "offsetOffsetEdit",	arglist, i);
   XtManageChild(offsetOffsetEditW);

   /* Set up static text widget inside row column widget */
   i = 0;
   XtSetArg(arglist[i], XmNy, 150); i++;
   XtSetArg(arglist[i], XmNx, 2); i++;
   newOffsetStatW=XmCreateLabel(offsetPopFormW, "Offset code Id :", arglist, i);
   XtManageChild(newOffsetStatW);

   /* Set up static text widget inside row column widget */
   i = 0;
   XtSetArg(arglist[i], XmNy, 150); i++;
   XtSetArg(arglist[i], XmNx, 150); i++;
   newOffsetValueW=XmCreateLabel(offsetPopFormW, codeStr, arglist, i);
   XtManageChild(newOffsetValueW);
}

/* Local Variables: */
/* c-basic-offset: 3 */
/* c-file-offsets:  ((substatement-open . 0)) */
/* End: */
