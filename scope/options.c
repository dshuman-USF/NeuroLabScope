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



/*** Program Constants */
#define WIDTH_ERROR_TEXT "Warning!\nScreen width can not be less than 0.05.\nScreen width set to 0.05."
#define MAX1 21600000

/*** Include files */
#include "std.h"
#include "filesup.h"
#include "dispUnit64.h"
#include "blockList.h"
#include "options.h"
#include <stdlib.h>
#include <Xm/SelectioB.h>
#include <Xm/Form.h>
#include <stdarg.h>
#include "color_table.h"
#include "bandpass.h"

extern long getOffset(int searchTime);
/*** rm RO 
void slidePlus();
***/
char *ltoa ();
void histogram();
void gofind();

Dimension  scopeScreen_x;		   /* scope x-dimension	 */
Dimension  scopeScreen_y;		   /* scope y-dimension	 */

/*** External Variables */
extern   Widget   app_shell;		/* ApplicationShell     */
extern   Widget   main_window;
extern   Widget   MessageBoxW;
extern   Widget   bullBoard2W;
extern   Widget   SideBarLabel;
extern   int      SideBarSet;
/* extern            makeWarningPop();*/
extern int setwidth(float);
extern   Widget   main_window;
extern void scrollselect();


/*** Global Variables -- to options */
extern GC WorkSpBgGC;
Widget   SideBar[1000];        // was 100
int      SideBarShown;
int      binwidth;
int      hist_bintotal;
int      bins;
int SideBarSet;
int nextStat;

/*** Forward Declarations */
void ClearSideBar();
void makeWidthErrorPop();
void makeWarningPop(char *, char *);



/******************************************************************************************
                              Scale
******************************************************************************************/
Widget   scalePop;


/* Callback routine for Esc Push Button Release */
void scaleEscUp(Widget w, XtPointer closure, XtPointer call_data)
{
   XtUnmanageChild(scalePop);
}


/* Callback routine for Done Push Button Release */
void scaleDoneUp(Widget w, XtPointer closure, XtPointer call_data)
{
  char *buff = "\0";
  double buffStr;
  Widget tempW;
	
  tempW = XmSelectionBoxGetChild(scalePop, XmDIALOG_TEXT);
  buff = XmTextGetString(tempW);
  buffStr = atof(buff);
  XtUnmanageChild(scalePop);

  if (buffStr > 0.0)
    {
      if (buffStr < 0.05)
	{
	  buffStr = 0.05;
	  makeWidthErrorPop();
	}
      /*** PPP adding scaleFactor */
      /*      setwidth(buffStr * scaleFactor); */
      setwidth(buffStr);
    }
}


/*** make Scale PopUp window */
void makeScalePop(void)
{
   /* create callback list - esc callback */
   static XtCallbackRec EscCallbacks[] = 
   {
      {scaleEscUp, NULL},
      {NULL, NULL},
   };

   /* create callback list - done callback */
   static XtCallbackRec DoneCallbacks[] = 
   {
      {scaleDoneUp, NULL},
      {NULL, NULL},
   };

	Arg arglist[10];
	Cardinal i;

	Widget UnusedW;
   XmString TitleString = NULL;
   XmString MessageLabel = NULL;
   XmString ButtonString = NULL;

   /* This section creates the compound strings */
   TitleString = XmStringCreateLtoR("Scale Pop", XmSTRING_DEFAULT_CHARSET);
   MessageLabel = XmStringCreateLtoR("Screen Width:", XmSTRING_DEFAULT_CHARSET);
   ButtonString = XmStringCreateLtoR(" OK ", XmSTRING_DEFAULT_CHARSET);

   /* this section creates an information popup */
	i = 0;
   XtSetArg(arglist[i], XmNdialogTitle, TitleString); i++;
   XtSetArg(arglist[i], XmNselectionLabelString, MessageLabel); i++;
   XtSetArg(arglist[i], XmNokLabelString, ButtonString); i++;
   XtSetArg(arglist[i], XmNwidth, 400); i++;
   XtSetArg(arglist[i], XmNheight, 100); i++;
   XtSetArg(arglist[i], XmNokCallback, DoneCallbacks); i++;
   XtSetArg(arglist[i], XmNcancelCallback, EscCallbacks); i++;
   XtSetArg(arglist[i], XmNautoUnmanage, False); i++;

   scalePop = XmCreatePromptDialog(main_window, "scalePop", arglist,i);
   
   UnusedW = XmSelectionBoxGetChild(scalePop, XmDIALOG_HELP_BUTTON);
   XtUnmanageChild(UnusedW);
   UnusedW = XmSelectionBoxGetChild(scalePop, XmDIALOG_SEPARATOR);
   XtUnmanageChild(UnusedW);

   XtManageChild(scalePop);
}


/******************************************************************************************
                              Integrate
******************************************************************************************/
Widget   integratePop;

float scalefactor, expval;

/* Callback routine for Esc Push Button Release */
void integrateEscUp(w, closure, call_data)
	Widget w;
	XtPointer closure;
	XtPointer call_data;
{
   XtUnmanageChild(integratePop);
}


void integrateDoneUp(w, closure, call_data)
	Widget w;
	XtPointer closure;
	XtPointer call_data;
{
	Arg args[2];
	Cardinal i;

   XmString MBString;
   char mbStr[50];
   int timeconstant;
   float deltat;

   char buff[5];
   Widget tempW;
	
   tempW = XmSelectionBoxGetChild(integratePop, XmDIALOG_TEXT);
   strcpy(buff, XmTextGetString(tempW));
   timeconstant = atoi(buff);

   XtUnmanageChild(integratePop);
   deltat= ((screenWidth / (double)ticks_per_second) / DAS_WIDTH) * 1000.0;
   expval = - (deltat / timeconstant);
   scalefactor = exp( expval);
   binwidth = screenWidth / DAS_WIDTH;
   bins = hist_bintotal;

   integrate();
   
   hist_bintotal = DAS_WIDTH;

   strcpy(mbStr, "Integrated with ");
   strcat(mbStr, buff);
   strcat(mbStr, " ms time constant");

   i = 0;
   MBString = XmStringCreateLtoR(mbStr, XmSTRING_DEFAULT_CHARSET);;
   XtSetArg(args[i], XmNlabelString, MBString); i++;
   XtSetValues(MessageBoxW,args,i);

   MessageBoxUsed = 1;
}


/*** make Integrate PopUp window */
void makeIntegratePop(void)
{
   /* create callback list - esc callback */
   static XtCallbackRec EscCallbacks[] = 
   {
      {integrateEscUp, NULL},
      {NULL, NULL},
   };

   /* create callback list - done callback */
   static XtCallbackRec DoneCallbacks[] = 
   {
      {integrateDoneUp, NULL},
      {NULL, NULL},
   };

	Arg arglist[10];
	Cardinal i;

	Widget UnusedW;
   XmString TitleString = NULL;
   XmString MessageString = NULL;
   XmString MessageLabel = NULL;
   XmString ButtonString = NULL;

   /* This section creates the compound strings */
   TitleString = XmStringCreateLtoR("Integrate Pop", XmSTRING_DEFAULT_CHARSET);
   MessageLabel = XmStringCreateLtoR("Time Constant: (ms)", XmSTRING_DEFAULT_CHARSET);
   MessageString = XmStringCreateLtoR("100", XmSTRING_DEFAULT_CHARSET);
   ButtonString = XmStringCreateLtoR(" OK ", XmSTRING_DEFAULT_CHARSET);

   /* this section creates an information popup */
	i = 0;
   XtSetArg(arglist[i], XmNdialogTitle, TitleString); i++;
   XtSetArg(arglist[i], XmNselectionLabelString, MessageLabel); i++;
   XtSetArg(arglist[i], XmNtextString, MessageString); i++;
   XtSetArg(arglist[i], XmNokLabelString, ButtonString); i++;
   XtSetArg(arglist[i], XmNwidth, 400); i++;
   XtSetArg(arglist[i], XmNheight, 100); i++;
   XtSetArg(arglist[i], XmNokCallback, DoneCallbacks); i++;
   XtSetArg(arglist[i], XmNcancelCallback, EscCallbacks); i++;
   XtSetArg(arglist[i], XmNautoUnmanage, False); i++;

   integratePop = XmCreatePromptDialog(main_window, "Integrate Pop", arglist,i);
   
   UnusedW = XmSelectionBoxGetChild(integratePop, XmDIALOG_HELP_BUTTON);
   XtUnmanageChild(UnusedW);
   UnusedW = XmSelectionBoxGetChild(integratePop, XmDIALOG_SEPARATOR);
   XtUnmanageChild(UnusedW);

   XtManageChild(integratePop);
}

/******************************************************************************************
                              Tally & Histogram Stat
******************************************************************************************/
static int TallyCount;

void
createTallyStat (int loc)
{
  Cardinal i;
  Arg arglist[8];
  i = 0;

  XtSetArg(arglist[i], XmNrecomputeSize, False); i++;
  XtSetArg(arglist[i], XmNborderWidth, 0); i++;
  XtSetArg(arglist[i], XmNwidth, 52); i++;
  XtSetArg(arglist[i], XmNx, 35); i++;
  XtSetArg(arglist[i], XmNy, (loc - 10)); i++;
  XtSetArg(arglist[i], XmNalignment, XmALIGNMENT_END); i++;
  SideBar[TallyCount] = XmCreateLabel(bullBoard2W, "tally",arglist,i);
  XtManageChild (SideBar[TallyCount]);

  TallyCount++;
}

void
destroySideBar (void)
{
  int n;
  
  for (n = 0; n < TallyCount; n++)
    XtDestroyWidget (SideBar[n]);
  TallyCount = 0;
}

void
ClearSideBar (void)
{
  XmString blank;
  int n;

  blank = XmStringCreateLtoR ("", XmSTRING_DEFAULT_CHARSET);
  
  for (n = 0; n < TallyCount; n++)
    XtVaSetValues (SideBar[n], XmNlabelString, blank, NULL);
  XtVaSetValues (SideBarLabel, XmNlabelString, blank, NULL);
  XmStringFree (blank);
}

void
showTally (void)
{
  XmString xms;
  int n;

  xms = XmStringCreateLtoR ("Tally", XmSTRING_DEFAULT_CHARSET);
  XtVaSetValues (SideBarLabel, XmNlabelString, xms, XmNmarginLeft, 50, NULL);
  XmStringFree (xms);
  
  for (n = 0; n < TallyCount && n < channelcount && l + n < sf.dchan; n++) {
    int i = l + n;
    char *s;
    if (asprintf (&s, "%d", sf.tally[i]) == -1) exit (1);
    xms = XmStringCreateLtoR (s, XmSTRING_DEFAULT_CHARSET);
    XtVaSetValues (SideBar[n], XmNlabelString, xms, NULL);
    XmStringFree (xms);
    free (s);
  }
}

void
showSPS (int n, double sps)
{
  XmString xms;

  if (n == 0) {
    xms = XmStringCreateLtoR ("Max SpS", XmSTRING_DEFAULT_CHARSET);
    XtVaSetValues (SideBarLabel, XmNlabelString, xms, XmNmarginLeft, 50, NULL);
    XmStringFree (xms);
  }
  
  if (n < TallyCount && n < channelcount && l + n < sf.dchan) {
    char *s;
    if (asprintf (&s, "%3.1f", sps) == -1) exit (1);
    xms = XmStringCreateLtoR (s, XmSTRING_DEFAULT_CHARSET);
    XtVaSetValues (SideBar[n], XmNlabelString, xms, NULL);
    XmStringFree (xms);
    free (s);
  }
}

/******************************************************************************************
                              About popup
******************************************************************************************/

Widget aboutPop;
Widget widthErrorPop;
Widget warningPop;

void makeAboutPop(void)
{
	Arg arglist[5];
	Cardinal i;

	Widget UnusedButtonW;
   XmString TitleString = NULL;
   XmString MessageString = NULL;
   XmString ButtonString = NULL;

   /* This section creates the compound strings */
   TitleString = XmStringCreateLtoR("About Xscopepb", XmSTRING_DEFAULT_CHARSET);
   MessageString
      = XmStringCreateLtoR("     " PACKAGE_NAME "\n\n  Version  " VERSION "\n    " __DATE__,
			   XmSTRING_DEFAULT_CHARSET);
   ButtonString = XmStringCreateLtoR(" OK ", XmSTRING_DEFAULT_CHARSET);

   /* this section creates an information popup */
	
	i = 0;
   XtSetArg(arglist[i], XmNdialogTitle, TitleString); i++;
   XtSetArg(arglist[i], XmNmessageString, MessageString); i++;
   XtSetArg(arglist[i], XmNokLabelString, ButtonString); i++;
   XtSetArg(arglist[i], XmNwidth, 200); i++;
   
   aboutPop = XmCreateInformationDialog(main_window, "about", arglist,i);
   
   UnusedButtonW = XmMessageBoxGetChild(aboutPop, XmDIALOG_CANCEL_BUTTON);
   XtUnmanageChild(UnusedButtonW);
   UnusedButtonW = XmMessageBoxGetChild(aboutPop, XmDIALOG_HELP_BUTTON);
   XtUnmanageChild(UnusedButtonW);

   XtManageChild(aboutPop);
}


/*****************************************************************************
   Width Error popup
*****************************************************************************/
void makeWidthErrorPop()
{
	Arg arglist[6];
	Cardinal i;

	Widget UnusedButtonW;
   XmString TitleString = NULL;
   XmString MessageString = NULL;
   XmString ButtonString = NULL;

/* This section creates the compound strings */
   TitleString = XmStringCreateLtoR("Width Error", XmSTRING_DEFAULT_CHARSET);
   MessageString = XmStringCreateLtoR(WIDTH_ERROR_TEXT, XmSTRING_DEFAULT_CHARSET);
   ButtonString = XmStringCreateLtoR(" OK ", XmSTRING_DEFAULT_CHARSET);

/* this section creates an information popup */
	
	i = 0;
   XtSetArg(arglist[i], XmNdialogTitle, TitleString); i++;
   XtSetArg(arglist[i], XmNmessageString, MessageString); i++;
   XtSetArg(arglist[i], XmNmessageAlignment, XmALIGNMENT_CENTER); i++;
   XtSetArg(arglist[i], XmNokLabelString, ButtonString); i++;
   XtSetArg(arglist[i], XmNwidth, 400); i++;
   
   widthErrorPop = XmCreateWarningDialog(main_window, "Error", arglist,i);
   
   UnusedButtonW = XmMessageBoxGetChild(widthErrorPop, XmDIALOG_CANCEL_BUTTON);
   XtUnmanageChild(UnusedButtonW);
   UnusedButtonW = XmMessageBoxGetChild(widthErrorPop, XmDIALOG_HELP_BUTTON);
   XtUnmanageChild(UnusedButtonW);

   XtManageChild(widthErrorPop);
}


/*****************************************************************************
   Print popup
*****************************************************************************/

/***
void printDoneUp(w, closure, call_data)
	Widget w;
	XtPointer closure;
	XtPointer call_data;
{
   printPage = 1;
}

void printEscUp(w, closure, call_data)
	Widget w;
	XtPointer closure;
	XtPointer call_data;
{
   printPage = 0;
}
***/

/*** make Test PopUp window */
/***
int makePrintPop()
{
   * create callback list - done callback *
   static XtCallbackRec DoneCallbacks[] = 
   {
      {printDoneUp, NULL},
      {NULL, NULL},
   };

   static XtCallbackRec EscCallbacks[] = 
   {
      {printEscUp, NULL},
      {NULL, NULL},
   };

	Arg arglist[15];
	Cardinal i;

	Widget UnusedW;
   XmString TitleString = NULL;
   XmString MessageString = NULL;
   XmString ButtonString = NULL;

   * This section creates the compound strings *
   TitleString = XmStringCreateLtoR("Print Pop", XmSTRING_DEFAULT_CHARSET);
   MessageString = XmStringCreateLtoR("Print Window on rt11 local printer", XmSTRING_DEFAULT_CHARSET);
   ButtonString = XmStringCreateLtoR("Print", XmSTRING_DEFAULT_CHARSET);

   * this section creates an information popup *
	i = 0;
   XtSetArg(arglist[i], XmNdialogTitle, TitleString); i++;
   XtSetArg(arglist[i], XmNmessageString, MessageString); i++;
   XtSetArg(arglist[i], XmNmessageAlignment, XmALIGNMENT_CENTER); i++;
   XtSetArg(arglist[i], XmNokLabelString, ButtonString); i++;
   XtSetArg(arglist[i], XmNwidth, 400); i++;
   XtSetArg(arglist[i], XmNheight, 100); i++;
   XtSetArg(arglist[i], XmNokCallback, DoneCallbacks); i++;
   XtSetArg(arglist[i], XmNcancelCallback, EscCallbacks); i++;

   printPop = XmCreateMessageDialog(main_window, "Print Pop", arglist,i);
   
   UnusedW = XmMessageBoxGetChild(printPop, XmDIALOG_HELP_BUTTON);
   XtUnmanageChild(UnusedW);
   UnusedW = XmMessageBoxGetChild(printPop, XmDIALOG_SEPARATOR);
   XtUnmanageChild(UnusedW);

   XtManageChild(printPop);
}
***/



/*****************************************************************************
   Warning popup
*****************************************************************************/
void makeWarningPop(title, message)
   char *title, *message;
{
	Arg arglist[6];
	Cardinal i;

	Widget UnusedButtonW;
   XmString TitleString = NULL;
   XmString MessageString = NULL;
   XmString ButtonString = NULL;

/* This section creates the compound strings */
   TitleString = XmStringCreateLtoR(title , XmSTRING_DEFAULT_CHARSET);
   MessageString = XmStringCreateLtoR(message, XmSTRING_DEFAULT_CHARSET);
   ButtonString = XmStringCreateLtoR(" OK ", XmSTRING_DEFAULT_CHARSET);

/* this section creates an information popup */
	
	i = 0;
   XtSetArg(arglist[i], XmNdialogTitle, TitleString); i++;
   XtSetArg(arglist[i], XmNmessageString, MessageString); i++;
   XtSetArg(arglist[i], XmNmessageAlignment, XmALIGNMENT_CENTER); i++;
   XtSetArg(arglist[i], XmNokLabelString, ButtonString); i++;
   XtSetArg(arglist[i], XmNwidth, 400); i++;
   
   warningPop = XmCreateWarningDialog(main_window, title, arglist,i);
   
   UnusedButtonW = XmMessageBoxGetChild(warningPop, XmDIALOG_CANCEL_BUTTON);
   XtUnmanageChild(UnusedButtonW);
   UnusedButtonW = XmMessageBoxGetChild(warningPop, XmDIALOG_HELP_BUTTON);
   XtUnmanageChild(UnusedButtonW);

   XtManageChild(warningPop);
}


/******************************************************************************************
                              Find Pop
******************************************************************************************/



/*** external vars */
extern Display *Current_display;
extern Window WorkSpWind;
extern Pixmap WorkSpPixmap;
extern Widget horiz_bar, ScreenLeft2W;
extern int marginTime, firstTime, leftTime, rightTime;

/* external procedures for finding events */
extern int eventIndexVal();
extern int getLastEvent();
extern int getEventNum();
extern int getNextEvent();
extern int resetEvents();

static void setLocationVal();

Widget findPop;
Widget findform;
Widget instanceEditW;
Widget codeIdEditW;
Widget locationValW;


void escapeFind(w, closure, call_data)
   Widget w;
   XtPointer closure;
   XtPointer call_data;
{
   XtUnmanageChild(findform);
}


/* widget data access procedures */
int getCodeId()
{
	char *numStr;
	int num;

   numStr =  XmTextGetString(codeIdEditW);
	sscanf(numStr,"%d",&num);
	free(numStr);
	return(num);
}


int getInstanceNum()
{
	char *numStr;
	int num;

   numStr = XmTextGetString(instanceEditW);
	sscanf(numStr,"%d",&num);
	free(numStr);
	return(num);
}


/* widget modification procedures */
void setInstanceNum(num)
	int num;
{
	Cardinal i;
	Arg arglist[2];
	char numStr[11];

	sprintf(numStr,"%d",num);

	i=0;
	XtSetArg(arglist[i],XtNvalue,numStr);i++;
	XtSetValues(instanceEditW,arglist,i);
}


static void setLocationVal(num)
	int num;
{
  Cardinal i;
  Arg arglist[2];
  char numStr[11];
  XmString textString3;
  sprintf(numStr,"%.3f",num/(double)ticks_per_second);
  textString3 = XmStringCreateLtoR(numStr,XmSTRING_DEFAULT_CHARSET);

  i=0;
  XtSetArg(arglist[i],XmNlabelString,textString3);i++;
  XtSetValues(locationValW,arglist,i);
  XtVaSetValues(horiz_bar, XmNvalue, num, NULL);
  scrollselect ();
  return;
}


/* callback routines for widget events */
void processNext(w,closure,call_data)
   Widget w;
   XtPointer closure;
   XtPointer call_data;
{
  int eventTime, position/*, now*/;
  extern int fileTime;

  resetEvents(getCodeId());
  eventTime = getNextEvent();
  if(eventTime != -1)
    {
      setInstanceNum(eventIndexVal());

      if(eventTime < (fileTime+firstTime-screenWidth))
	position = (eventTime - firstTime)/marginTime - 1;
      else
	position = (fileTime-screenWidth)/marginTime + 1;

      if(position < 0) 
	position = 0;

      leftTime = eventTime; /* test */ 
      XtVaSetValues(horiz_bar, XmNvalue, leftTime, NULL);
      screenLeft = getOffset(leftTime);
      XFillRectangle (Current_display, WorkSpPixmap, WorkSpBgGC, 0, 0, scopeScreen_x, scopeScreen_y);
      setLocationVal(eventTime);
    }
}


void processInstance(w,closure,call_data)
   Widget w;
   XtPointer closure;
   XtPointer call_data;
{
   int instance, position, eventTime;

   resetEvents(getCodeId());
   instance = getInstanceNum();
   if(instance > 0)
   {
      eventTime = getEventNum(instance);
      if(eventTime != -1)
      {
	 if(eventTime < (totalTime - screenWidth))
	    position = eventTime;
	 else
	    position = totalTime-screenWidth;

	 if(position < 0) 
	    position = 0;

	 leftTime = position;
	 XtVaSetValues(horiz_bar, XmNvalue, leftTime, NULL);
	 rightTime = leftTime + screenWidth;
	 XFillRectangle (Current_display, WorkSpPixmap, WorkSpBgGC, 0, 0, scopeScreen_x, scopeScreen_y);
	 setLocationVal(eventTime);
      }
   }
}


/*********************************************************
 *          The root popup of the find event feature     *
 *            initializations & definitions              *
 *********************************************************/
void makeFindPop(void)
{
   /* local variable declarations */
	Widget goButtonW;
	Widget escButtonW;
	Widget buttonRowCol2W;
	Widget buttonRowCol3W;
	Widget codeIdStatW;
	Widget instanceStatW;
	Widget nextButtonW;
	Widget locationStatW;

   Arg arglist[7];
   Cardinal i;

   /* This section creates the popup  with info pertaining to writing a file */
   i = 0;
   findPop = XmCreateDialogShell(app_shell, "findpop", arglist, i);

   i=0;
	XtSetArg(arglist[i], XmNwidth, 320); i++;
	XtSetArg(arglist[i], XmNheight, 400); i++;
	findform = XmCreateForm(findPop, "filterPop", arglist, i);
	XtManageChild(findform);

   i = 0;
   XtSetArg(arglist[i], XmNy, 5); i++;
   XtSetArg(arglist[i], XmNx, 5); i++;
   XtSetArg(arglist[i], XmNnumColumns, 2); i++;
   XtSetArg(arglist[i], XmNpacking, XmPACK_COLUMN); i++;
   XtSetArg(arglist[i], XmNspacing, 175); i++;
   XtSetArg(arglist[i], XtNborderWidth, 1); i++;
   buttonRowCol2W = XmCreateRowColumn(findform, "buttonRowCol", arglist, i);
   XtManageChild(buttonRowCol2W);

   /* Set up Push button widget inside row column widget */
   i = 0;
   escButtonW = XmCreatePushButton(buttonRowCol2W, "Escape", arglist, i);
   XtManageChild(escButtonW);
   XtAddCallback (escButtonW, XmNactivateCallback,escapeFind,NULL);	

   /* Set up Push button widget inside row column widget */
   i = 0;
   goButtonW = XmCreatePushButton(buttonRowCol2W, "Go", arglist, i);
   XtManageChild(goButtonW);
   XtAddCallback(goButtonW, XmNactivateCallback, processInstance, NULL);	

   /* Set up static text widget */
   i = 0;
   XtSetArg(arglist[i], XmNx, 7); i++;
	XtSetArg(arglist[i], XmNy, 57); i++;
   codeIdStatW=XmCreateLabel(findform, "Code Id", arglist, i);
   XtManageChild(codeIdStatW);

   /* Set up text edit widget for code input */
   i = 0;
   XtSetArg(arglist[i], XmNy, 60); i++;
   XtSetArg(arglist[i], XmNx, 117); i++;
   codeIdEditW=XmCreateText(findform, "codeIdEdit", arglist, i);
   XtManageChild(codeIdEditW);

   /* Set up static text widget */
   i = 0;
	XtSetArg(arglist[i], XmNy, 102); i++;
	XtSetArg(arglist[i], XmNx, 5); i++;
   instanceStatW=XmCreateLabel(findform, "Instance #", arglist, i);
   XtManageChild(instanceStatW);

   /* Set up text edit widget for instance input */
   i = 0;
   XtSetArg(arglist[i], XmNx, 117); i++;
   XtSetArg(arglist[i], XmNy, 100); i++;
   instanceEditW=XmCreateText(findform, "instanceEdit", arglist, i);
   XtManageChild(instanceEditW);
   XtAddCallback(instanceEditW, XmNactivateCallback, processInstance, NULL);	

   i = 0;
   XtSetArg(arglist[i], XmNy, 170); i++;
   XtSetArg(arglist[i], XmNx, 70); i++;
   XtSetArg(arglist[i], XmNnumColumns, 1); i++;
   XtSetArg(arglist[i], XtNborderWidth, 1); i++;
   buttonRowCol3W=XmCreateRowColumn(findform, "buttonRowCol", arglist, i);
   XtManageChild(buttonRowCol3W);

   /* Set up Push button widget inside row column widget */
   i = 0;
   nextButtonW=XmCreatePushButton(buttonRowCol3W, "Next", arglist,i);
   XtManageChild(nextButtonW);
   XtAddCallback (nextButtonW, XmNactivateCallback, processNext, NULL);	

   /* Set up static text widget */
   i = 0;
	XtSetArg(arglist[i], XmNy, 230); i++;
	XtSetArg(arglist[i], XmNx, 5); i++;
   locationStatW=XmCreateLabel(findform,"Location", arglist,i);
   XtManageChild(locationStatW);

/* Set up static text widget */
   i = 0;
	XtSetArg(arglist[i], XmNy, 230); i++;
	XtSetArg(arglist[i], XmNx, 150); i++;
   locationValW=XmCreateLabel(findform,"*******", arglist, i);
   XtManageChild(locationValW);
}

#include <Xm/Form.h>

#define TIGHTNESS 20

Widget
CreateActionArea (Widget parent, ActionAreaItem *actions, int num_actions)
{
   Widget action_area, widget;
   int    i;
	
   action_area = XmCreateForm (parent, "action_area", NULL, 0);
	
   XtVaSetValues (action_area, XmNfractionBase, TIGHTNESS*num_actions - 1,
                  XmNleftOffset, 10,
                  XmNrightOffset, 10,
                  NULL);

   for (i = 0; i < num_actions; i++) {
      widget = XmCreatePushButton (action_area, actions[i].label, NULL, 0);
		
      XtVaSetValues (widget, XmNleftAttachment, i? XmATTACH_POSITION: XmATTACH_FORM, 
                     XmNleftPosition, TIGHTNESS*i,
                     XmNtopAttachment, XmATTACH_FORM,
                     XmNbottomAttachment, XmATTACH_FORM,
                     XmNrightAttachment, 
                     i != num_actions - 1 ? XmATTACH_POSITION : XmATTACH_FORM,
                     XmNrightPosition, TIGHTNESS * i + (TIGHTNESS - 1),
                     XmNshowAsDefault, i == 0, 
                     XmNdefaultButtonShadowThickness, 1, 
                     NULL); 

      if (actions[i].callback)
         XtAddCallback (widget, XmNactivateCallback, 
                        actions[i].callback, (XtPointer) actions[i].data);
			
      XtManageChild (widget);
		
      if (i == 0) {
         /* Set the action_area's default button to the first widget 
         ** created (or, make the index a parameter to the function
         ** or have it be part of the data structure). Also, set the
         ** pane window constraint for max and min heights so this
         ** particular pane in the PanedWindow is not resizable.
         */
			
         Dimension height, h;
			
         XtVaGetValues (action_area, XmNmarginHeight, &h, NULL);
         XtVaGetValues (widget, XmNheight, &height, NULL);
			
         height += 2 * h; 
			
         XtVaSetValues (action_area, XmNdefaultButton, widget, 
                        XmNpaneMaximum, height, 
                        XmNpaneMinimum, height,
                        NULL);
      }
   }
	
   XtManageChild (action_area);
	
   return action_area;
}


static void
close_dialog (Widget w, XtPointer client_data, XtPointer call_data)
{
   Widget pane = *(Widget *) client_data;
   XtUnmanageChild (pane);
}

#include <Xm/TextF.h>

static int
get_state (Widget w, char *button, char *label)
{
   char *text;
   XmString str;
   Widget wtmp;
   unsigned char set_state;

   wtmp = XtNameToWidget (w, button);
   XtVaGetValues (wtmp, XmNlabelString, &str, XmNset, &set_state, NULL);
#if XmVERSION == 1
   XmStringGetLtoR (str, XmFONTLIST_DEFAULT_TAG, &text);
#define XmSET 1
#else
   text = (char *)XmStringUnparse (str, NULL, XmCHARSET_TEXT, XmCHARSET_TEXT, NULL, 0, XmOUTPUT_ALL);
#endif
   if (strcmp (text, label) != 0) {
      fprintf (stderr, "text is %s, label is %s\n", text, label);
      DIE;
   }
   
   XtFree (text);
   return set_state == XmSET;
}

#include <Xm/PanedW.h>
#include <Xm/LabelG.h>

void
map_dialog (Widget dialog, XtPointer client_data, XtPointer call_data)
{
   static Position x, y, px, py;
   Dimension w, h, pw, ph, bw;
   Widget app, tmp;
   app = XtParent (dialog);
   while ((tmp = XtParent (app)))
      app = tmp;

   XtRealizeWidget (dialog);
   XtVaGetValues (dialog, XmNwidth, &w, XmNheight, &h, XmNhighlightThickness, &bw, NULL);
   XtVaGetValues (app, XmNwidth, &pw, XmNheight, &ph, XmNx, &px, XmNy, &py, NULL);

   {
      XWindowAttributes xwa;
      XGetWindowAttributes (XtDisplay(app), XtWindow(app), &xwa);
      bw = xwa.x;
   }
   if (px + pw + w < WidthOfScreen (XtScreen (dialog)))
      x = px + pw;
   else
      x = px - w - 2 * bw;
   y = py;
   
   if ((x + w) >= WidthOfScreen (XtScreen (dialog)))
      x = WidthOfScreen (XtScreen (dialog)) - w - 1;
   if (x < 0)
      x = 0;
   if ((y + h) >= HeightOfScreen (XtScreen (dialog)))
      y = HeightOfScreen (XtScreen (dialog)) - h - 1;
   if (y < 0)
      y = 0;
   XtVaSetValues (dialog, XmNx, x, XmNy, y, NULL);
}

XmString
S (char *s)
{
  static int alloc;
  static int count;
  static XmString *xms;
  
  if (s == 0) {
    int n;
    for (n = 0; n < count; n++)
      XmStringFree (xms[n]);
    count = 0;
    return 0;
  }
  if (count == alloc)
    xms = realloc (xms, ++alloc * sizeof *xms);
  return (xms[count++] = XmStringCreateLocalized (s));
}

static void
set_args (Arg **argp, int *np, ...)
{
  static ArgList args;
  static int count, alloc;
  va_list ap;
  String name;
  XtArgVal value;

  va_start (ap, np);
  count = 0;
  while (1) {
    name = va_arg (ap, String);
    if (name == NULL) {
      *argp = args;
      *np = count;
      va_end (ap);
      return;
    }
    value = va_arg (ap, XtArgVal);
    if (count == alloc)
       args = realloc (args, ++alloc * sizeof *args);
    XtSetArg (args[count], name, value); count++;
    fflush (stdout);
  }
}

static void
flatten (Widget w, int count)
{
  int n;
  for (n = 0; n < count; n++) {
    char *b;
    if (asprintf (&b, "button_%d", n) == -1) exit (1);
    XtVaSetValues (XtNameToWidget (w, b), XmNmarginHeight, 0, XmNmarginTop, 0, XmNmarginBottom, 0, NULL);
    free (b);
  }
}

Widget
make_radiobox (Widget parent, String name, int button_set, ...)
{
  Arg *argp;
  int n, count;
  Widget w;
  static int alloc;
  static XmStringTable labels;
  static XmButtonTypeTable types;
  va_list ap;
  
  set_args (&argp, &n, XmNmarginHeight, 0, XmNmarginTop, 10, XmNmarginBottom, 0, NULL);
  XtManageChild (XmCreateLabel (parent, name, argp, n));

  va_start (ap, button_set);
  count = 0;
  while (1) {
    String label = va_arg (ap, String);
    if (label == NULL) {
      va_end (ap);
      break;
    }
    if (count == alloc)
      labels = realloc (labels, ++alloc * sizeof *labels);
    labels[count++] = S (label);
  }
  types = realloc (types, alloc * sizeof *types);
  for (n = 0; n < count; n++)
    types[n] = XmRADIOBUTTON;

  set_args (&argp, &n,
            XmNbuttonSet, button_set,
            XmNbuttonCount, count,
            XmNbuttons, labels,
            XmNorientation, XmHORIZONTAL,
            NULL);

  XtManageChild (w = XmCreateSimpleRadioBox (parent, name, argp, n));
  flatten (w, count); S (0);
  return w;
}

Widget
spacer (Widget parent, int height)
{
   Arg *argp;
   int n;
   Widget w;

   set_args (&argp, &n, XmNheight, height, NULL);
   XtManageChild (w = XmCreateDrawingArea (parent, "", argp, n));
   return w;
}

Widget
make_palette_sel (Widget parent, void (*callback)())
{
  Widget w;

  XtManageChild (w = XmVaCreateSimpleOptionMenu (parent, "palette", S ("Color"), 'l', 0, callback,
                                             XmVaPUSHBUTTON, S ("Off"),               'O', NULL, NULL,
                                             XmVaPUSHBUTTON, S ("BGL favorite"),      'B', NULL, NULL,
                                             XmVaPUSHBUTTON, S ("Printable on gray"), 'P', NULL, NULL,
                                             XmVaPUSHBUTTON, S ("Traditional pm3d"),  'T', NULL, NULL,
                                             XmVaPUSHBUTTON, S ("Hot"),               'H', NULL, NULL,
                                             XmVaPUSHBUTTON, S ("AFM hot"),           'A', NULL, NULL,
                                             XmVaPUSHBUTTON, S ("Rainbow"),           'R', NULL, NULL,
                                             XmVaPUSHBUTTON, S ("Gray scale"),        'G', NULL, NULL,
                                             XmNtearOffModel,XmTEAR_OFF_ENABLED,
                                             XmNcolormap,    cmap,
                                             NULL));
  S (0);
  return w;
}

Widget
make_entry (Widget parent, char *label, char *name)
{
   Arg      *argp;
   int n;
   Widget w;

   set_args (&argp, &n, XmNlabelString, S(label), NULL);
   XtManageChild (XmCreateLabelGadget (parent, label, argp, n));
   XtManageChild (w = XmCreateTextField (parent, name, NULL, 0));
   S(0);
   return w;
}

void
start_dialog (Widget parent, char *name, Widget *shell, Widget *pane, Widget *rc)
{
   Arg      *argp;
   int      n;

   set_args (&argp, &n, XmNtitle, name, XmNdeleteResponse, XmUNMAP, XmNallowShellResize, FALSE, NULL);
   *shell = XmCreateDialogShell (XtParent (parent), "dialog", argp, n);    
	
   set_args (&argp, &n, XmNsashWidth, 1, XmNsashHeight, 1, NULL);
   *pane  = XmCreatePanedWindow (*shell, "pane", argp, n);
	
   set_args (&argp, &n, XmNsashWidth, 1, NULL);
   *rc    = XmCreateRowColumn (*pane, "control_area", argp, n);
}

static int
get_palette (Widget w, unsigned widget_n, Widget *pushbutton)
{
  int n;
  int widget_count = PALETTE_COUNT + 1;
  for (n = 1; n < widget_count; n++)
    if (pushbutton[n])
      XtVaSetValues(pushbutton[n], XmNlabelType, XmSTRING, NULL);
  pushbutton[widget_n] = w;
  if (widget_n != 0)
    XtVaSetValues(w, XmNlabelType, XmPIXMAP, XmNlabelPixmap, palette_xpm[widget_n - 1], NULL);
  return widget_n;
}

static inline double
get_double (Widget text_field)
{
   double d;
   char *text = XmTextFieldGetString (text_field);
   d = atof (text);
   XtFree (text);
   return d;
}

static inline int
get_int (Widget text_field)
{
   int i;
   char *text = XmTextFieldGetString (text_field);
   i = atoi (text);
   XtFree (text);
   return i;
}

typedef struct
{
   Widget shell;
   Widget pane;
   Widget rc;
   Widget global_local;
   Widget format;
   Widget min_bpm_w;
   Widget max_bpm_w;
   Widget surr_count;
   Widget distribution;
   Widget histogram;
   Widget savesurr;
   Widget level;
   Widget palette;
} BandpassWidgets;

static void
bandpass_ok (Widget w, XtPointer client_data, XtPointer call_data)
{
   static BandpassWidgets *bw;
   static BandpassParams bandpass_params;
   static Widget pushbutton[PALETTE_COUNT + 1];

   if ((long) client_data <= PALETTE_COUNT)
     bandpass_params.palette = get_palette (w, (unsigned long)client_data, pushbutton);
   else
      bw = (BandpassWidgets *) client_data;

   if (!bw)
       return;
   
   bandpass_params.min_bpm = get_double (bw->min_bpm_w);
   bandpass_params.max_bpm = get_double (bw->max_bpm_w);

   bandpass_params.global = get_state (bw->global_local, "button_1", "Global");

   int n = bandpass_params.normal    = get_state (bw->format, "button_0", "Norm");
   int l = bandpass_params.log       = get_state (bw->format, "button_1", "Log" );
   int e = bandpass_params.envelope  = get_state (bw->format, "button_2", "Env" );
   int t = bandpass_params.threshold = get_state (bw->format, "button_3", "Thr" );

   bandpass_params.surrogate_count =
   get_state (bw->surr_count, "button_0", "1" )
   + get_state (bw->surr_count, "button_1", "20" ) * 20
   + get_state (bw->surr_count, "button_2", "100" ) * 100;

   bandpass_params.gaussian  = get_state (bw->distribution, "button_0", "Normal" );
   bandpass_params.empirical = get_state (bw->distribution, "button_1", "Empirical" );

   int p01 = bandpass_params.p01       = get_state (bw->level, "button_0", "01" );
   int p05 = bandpass_params.p05       = get_state (bw->level, "button_1", "05" );
   int p50 = bandpass_params.p50       = get_state (bw->level, "button_2", "50" );
   int p95 = bandpass_params.p95       = get_state (bw->level, "button_3", "95" );
   int p99 = bandpass_params.p99       = get_state (bw->level, "button_4", "99" );

   bandpass_params.hist_on   = get_state (bw->histogram, "button_0", "On" );
   bandpass_params.save_on   = get_state (bw->savesurr , "button_0", "On" );

   double fs = bandpass (&bandpass_params);

   char *t1;
   if (asprintf (&t1, "Bandpass %g-%g bpm %s %s",
                 bandpass_params.min_bpm, bandpass_params.max_bpm,
                 (bandpass_params.global ? "Gbl" : "Lcl"),
                 n ? "Norm" : l ? "Log" : e ? "Env" : t ? "Thr" : "") == -1) exit (1);
   char *t2;
   if (bandpass_params.log || bandpass_params.threshold)
   {if (asprintf (&t2, "%s %d Sur %s %s%%", t1,
                  bandpass_params.surrogate_count,
                  bandpass_params.gaussian ? "Gau" : "Emp",
                  p01 ? "1" : p05 ? "5" : p50 ? "50" : p95 ? "95" : p99 ? "99" : "?") == -1) exit (1);}
   else {t2 = t1; t1 = 0;}
   free (t1);
   if (bandpass_params.log && bandpass_params.global)
   {if (asprintf (&t1, "%s %.1f FS", t2, fs) == -1) exit (1);}
   else {t1 = t2; t2 = 0;}
   free (t2);

   XmString s = XmStringCreateLocalized (t1);
   XtVaSetValues (MessageBoxW, XmNlabelString, s, NULL);
   XmStringFree (s);
   free (t1);
}

void
makeBandpassPop (Widget w, XtPointer client_data, XtPointer call_data)
{
   static BandpassWidgets bw;
   static ActionAreaItem action_items[] = { 
      { "Apply", bandpass_ok,    &bw },
      { "Close", close_dialog, &bw.pane },
   };

   if (bw.shell == 0) {
      start_dialog (w, "Bandpass", &bw.shell, &bw.pane, &bw.rc);
      bw.min_bpm_w    = make_entry       (bw.rc, "min breaths per minute:", "min_bpm");
      bw.max_bpm_w    = make_entry       (bw.rc, "max breaths per minute:", "max_bpm");
      bw.global_local = make_radiobox    (bw.rc, "Normalization:",   0, "Local", "Global", NULL);
      bw.format       = make_radiobox    (bw.rc, "Display Format:",  0, "Norm", "Log", "Env", "Thr", NULL);
      bw.surr_count   = make_radiobox    (bw.rc, "Surrogate Count:", 0, "1", "20", "100", NULL);
      bw.distribution = make_radiobox    (bw.rc, "Threshold Distribution:", 0, "Normal", "Empirical", NULL);
      bw.level        = make_radiobox    (bw.rc, "Threshold Level:", 0, "01", "05", "50", "95", "99", NULL);
      bw.histogram    = make_radiobox    (bw.rc, "Empirical Distribution Histogram:", 1, "On", "Off", NULL);
      bw.savesurr     = make_radiobox    (bw.rc, "Save Surrogates:"                 , 1, "On", "Off", NULL);

      spacer (bw.rc, 15);
      bw.palette      = make_palette_sel (bw.rc, bandpass_ok);
      
      XtManageChild (bw.rc);
                                     
      CreateActionArea (bw.pane, action_items, XtNumber (action_items));

      XtAddCallback (bw.shell, XmNpopupCallback, map_dialog, NULL);
      S (0);
   }
   else {
      //the dialog will remember where it was when you pop it back up, so
      //there is no further need for map_dialog - remove it.

      XtRemoveCallback (bw.shell, XmNpopupCallback, map_dialog, NULL);
      XtUnmanageChild (bw.pane);
   }

   XtManageChild (bw.pane);
}

typedef struct
{
   Widget shell;
   Widget pane;
   Widget rc;
   Widget local_global;
   Widget bins;
   Widget show_max_spk;
   Widget palette;
} HistogramWidgets;

static void
histogram_ok (Widget w, XtPointer client_data, XtPointer call_data)
{
   static HistogramWidgets *hw;
   static HistogramParams histogram_params;
   static Widget pushbutton[PALETTE_COUNT + 1];

   if ((long) client_data <= PALETTE_COUNT)
     histogram_params.palette = get_palette (w, (unsigned long)client_data, pushbutton);
   else
     hw = (HistogramWidgets *) client_data;

   if (!hw)
       return;

   histogram_params.bins = (get_state (hw->bins, "button_0", "100") ? 100 :
                            (get_state (hw->bins, "button_1", "200") ? 200 :
                             (get_state (hw->bins, "button_2", "400") ? 400 :
                              (get_state (hw->bins, "button_3", "800") ? 800 : DIE))));

   histogram_params.local = get_state (hw->local_global, "button_0", "Local");
   histogram_params.show_max_spk = get_state (hw->show_max_spk, "button_1", "On");

   char *t;
   if (asprintf (&t, "Histogram with %d bins.   %s",
                 histogram_params.bins,
                 (histogram_params.local ? "Local" : "Global")) == -1) exit (1);
   XmString s = XmStringCreateLocalized (t);
   XtVaSetValues (MessageBoxW, XmNlabelString, s, NULL);
   XmStringFree (s);
   free (t);

   histogram (&histogram_params);
}

void
makeHistogramPop (Widget w, XtPointer client_data, XtPointer call_data)
{
   static HistogramWidgets hw;
   static ActionAreaItem action_items[] = { 
      { "Apply", histogram_ok,    &hw },
      { "Close", close_dialog, &hw.pane },
   };

   if (hw.shell == 0) {
      start_dialog (w, "Histogram", &hw.shell, &hw.pane, &hw.rc);
      hw.bins         = make_radiobox (hw.rc, "Number of bins:", 0, "100", "200", "400", "800", NULL);
      hw.local_global = make_radiobox (hw.rc, "Scaling:", 0, "Local", "Global", NULL);
      hw.show_max_spk = make_radiobox (hw.rc, "Show Max Spikes per Second:", 0, "Off", "On", NULL);
      spacer (hw.rc, 15);
      hw.palette      = make_palette_sel (hw.rc, histogram_ok);

      XtManageChild (hw.rc);
                                     
      CreateActionArea (hw.pane, action_items, XtNumber (action_items));

      XtAddCallback (hw.shell, XmNpopupCallback, map_dialog, NULL);
   }
   else {
      //the dialog will remember where it was when you pop it back up, so
      //there is no further need for map_dialog - remove it.

      XtRemoveCallback (hw.shell, XmNpopupCallback, map_dialog, NULL);
      XtUnmanageChild (hw.pane);
   }

   XtManageChild (hw.pane);
}

typedef struct
{
   Widget shell;
   Widget pane;
   Widget rc;

   Widget ticks_per_bin;
   Widget paper;
   Widget norm;
   Widget count;
   char *filename;
} PSWidgets;

static void
ps_ok (Widget w, XtPointer client_data, XtPointer call_data)
{
   static PSWidgets *pw;
   static PSParams ps_params;

   pw = (PSWidgets *) client_data;

   ps_params.ticks_per_bin = get_int (pw->ticks_per_bin);

   ps_params.global = get_state (pw->norm,  "button_1", "Global");
   ps_params.legal  = get_state (pw->paper, "button_1", "Legal");
   ps_params.tally  = get_state (pw->count, "button_0", "Tally");
   ps_params.filename = pw->filename;
   
   ps (&ps_params);
   XtUnmanageChild (pw->pane);
}

void
makePSPop (Widget w, XtPointer client_data, XtPointer call_data)
{
   static PSWidgets pw;
   static ActionAreaItem action_items[] = { 
      { "OK", ps_ok,    &pw },
      { "Close", close_dialog, &pw.pane },
   };

   XtFree (pw.filename);
   pw.filename = (char *) client_data;

   if (pw.shell == 0) {
      start_dialog (w, "Save as PS", &pw.shell, &pw.pane, &pw.rc);
      pw.ticks_per_bin = make_entry (pw.rc, "ticks per bin:", "tpb");
      pw.paper = make_radiobox (pw.rc, "Paper Size:", 0, "Letter", "Legal", NULL);
      pw.norm  = make_radiobox (pw.rc, "Scaling:", 0, "Local", "Global", NULL);
      pw.count = make_radiobox (pw.rc, "Count Field:", 0, "Tally", "Max Spike/s", NULL);
      
      XtManageChild (pw.rc);
                                     
      CreateActionArea (pw.pane, action_items, XtNumber (action_items));

      //      XtAddCallback (pw.shell, XmNpopupCallback, map_dialog, NULL);
   }
   else {
      //the dialog will remember where it was when you pop it back up, so
      //there is no further need for map_dialog - remove it.

      //      XtRemoveCallback (pw.shell, XmNpopupCallback, map_dialog, NULL);
      XtUnmanageChild (pw.pane);
   }

   XtManageChild (pw.pane);
}

typedef struct
{
   Widget shell;
   Widget pane;
   Widget rc;
   Widget left_time;
} LeftWidgets;

static Widget left_time_w;

void
set_left_time (void)
{
   if (left_time_w)
   {
      char *tmp;
      if (asprintf (&tmp, "%d", leftTime) == -1) exit (1);
      XtVaSetValues (left_time_w, XmNvalue, tmp, NULL);
      free(tmp);
   }
}

void goto_time (int time)
{
   leftTime = time;
   XtVaSetValues (horiz_bar, XmNvalue, leftTime, NULL);
   screenLeft = getOffset (leftTime);
   XFillRectangle (Current_display, WorkSpPixmap, WorkSpBgGC, 0, 0, scopeScreen_x, scopeScreen_y);
   scrollselect ();
}

static void
left_ok (Widget w, XtPointer client_data, XtPointer call_data)
{
   static LeftWidgets *lw;

   lw = (LeftWidgets *) client_data;

   if (!lw || !fp)
       return;

   goto_time (get_int (lw->left_time));
}

void
makeLeftPop (Widget w, XtPointer client_data, XtPointer call_data)
{
   static LeftWidgets lw;
   static ActionAreaItem action_items[] = { 
      { "Apply", left_ok,    &lw },
      { "Close", close_dialog, &lw.pane },
   };

   if (lw.shell == 0) {
      start_dialog (w, "Left", &lw.shell, &lw.pane, &lw.rc);
      left_time_w = lw.left_time = make_entry (lw.rc, "left ticks:", "left_ticks");
      XtAddCallback (lw.left_time, XmNactivateCallback, left_ok, &lw);
      XtManageChild (lw.rc);
                                     
      CreateActionArea (lw.pane, action_items, XtNumber (action_items));

      XtAddCallback (lw.shell, XmNpopupCallback, map_dialog, NULL);
      S (0);
   }
   else {
      //the dialog will remember where it was when you pop it back up, so
      //there is no further need for map_dialog - remove it.

      XtRemoveCallback (lw.shell, XmNpopupCallback, map_dialog, NULL);
      XtUnmanageChild (lw.pane);
   }
   char *tmp;
   if (asprintf (&tmp, "%d", leftTime) == -1) exit (1);
   XtVaSetValues(lw.left_time, XmNvalue, tmp, NULL);
   free(tmp);

   XtManageChild (lw.pane);
}

/* Local Variables: */
/* c-basic-offset: 3 */
/* c-file-offsets:  ((substatement-open . 0)) */
/* End: */
