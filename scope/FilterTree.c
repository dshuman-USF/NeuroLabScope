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
#include <X11/IntrinsicP.h>
#include "blockList.h"
#include "FilterTree.h"
#include "dispUnit64.h"

#define PASS 0
#define DELETE 1

extern void refreshBlocks(void);
extern void refresh(void);

/* global variables (widgets,etc....) */
static	Widget filterPop;

extern Widget app_shell;

/* global variables for the manOptPop & autoOptPop popups */
Widget autoOptPop;
Widget triggerCodeEditW;
Widget timespanEditW;
Widget rangeBeginEditW;
Widget rangeEndEditW;

/* List for holding selected manual or automatic filter blocks */
int segmentList;

/* routines to pop up and down the filter option popup */

Widget chunkUnselectButtonW;
Widget manualMethodButtonW;
Widget automaticMethodButtonW;
Widget markToggleW;
Widget filterModeStatW;
Widget filterModeLabelW;
Widget filterform;

void makeAutoOptPop();

void showFilterPop()
{
   XtPopup(filterPop,XtGrabNone);
}

void hideFilterPop()
{
   XtPopdown(filterPop);
}

void escapeForm(w,closure,call_data)
     Widget w;
     XtPointer closure;
     XtPointer call_data;
{
   XtUnmanageChild(filterform);
}

/* These routines return the state of some switches which effect
   the nature of the write function.					               */
int filterMode;

Boolean markCodes()
{
   Cardinal i;
   Arg arglist[4];
   Boolean value;

   i = 0;
   XtSetArg(arglist[i], XmNset, &value); i++;
   XtGetValues(markToggleW, arglist, i);

   return(value == True);
}

Boolean passBlocks()
{
   return(filterMode == PASS);
}

/* callback routines for widget events */
void manualSelect(w,closure,call_data)
   Widget w;
   XtPointer closure;
   XtPointer call_data;
{
   XtSetSensitive(automaticMethodButtonW,False);
}

void automaticSelect(w,closure,call_data)
   Widget w;
   XtPointer closure;
   XtPointer call_data;
{
   XtSetSensitive(manualMethodButtonW,False);
   makeAutoOptPop();
}

void filterModeSelect(w,closure,call_data)
   Widget w;
   XtPointer closure;
   XtPointer call_data;
{
   Cardinal i;
   Arg arglist[4];
   char numStr3[10];
   XmString textString5;
      
   if(filterMode == PASS)
   {
      sprintf(numStr3," Delete %s", "");
      textString5 = XmStringCreateLtoR(numStr3,XmSTRING_DEFAULT_CHARSET);
      i = 0;
      XtSetArg(arglist[i],XmNlabelString,textString5);i++;
      XtSetValues(filterModeStatW,arglist,i);
      filterMode = DELETE;
   }
   else
   {
      sprintf(numStr3," Pass %s", "");
      textString5 = XmStringCreateLtoR(numStr3,XmSTRING_DEFAULT_CHARSET);
      i = 0;
      XtSetArg(arglist[i],XmNlabelString,textString5);i++;
      XtSetValues(filterModeStatW,arglist,i);
      filterMode = PASS;
   }
}

extern void RedrawCB (Widget w, XtPointer client_data, XtPointer call_data);

void resetSelect(w,closure,call_data)
   Widget w;
   XtPointer closure;
   XtPointer call_data;
{
   ClearList(segmentList);
   RedrawCB (w, closure, call_data);
   refresh();
   refreshBlocks(); 
}

void autoreset(w,closure,call_data)
   Widget w;
   XtPointer closure;
   XtPointer call_data;
{
  /*   XClearWindow; */
}

void remoteReset(void)
{
   ClearList(segmentList);
}


/********************************************************** 
 *          The root popup of the filter tree     	 *
 *            initializations & definitions		 *
 *********************************************************/
void makeFilterRoot(void)
{
   /* local variable declarations */
   Arg arglist[20];
   Cardinal i;


   Widget filterPopRowColW;
   Widget filterModeRowColW;
   Widget filterModeButtonW;
   Widget resetButtonW;
   Widget escapeButtonW;

   /* This section creates a popup for the filter option.      */
   /* Initialize toolkit */
   i = 0;
   XtSetArg(arglist[i], XmNx, 810); i++;
   XtSetArg(arglist[i], XmNy, 70); i++;
   filterPop = XmCreateDialogShell(app_shell, "filterPop", arglist, i);

   i = 0;
   XtSetArg(arglist[i], XmNheight, 300); i++;
   XtSetArg(arglist[i], XmNwidth, 320); i++;
   filterform = XmCreateForm(filterPop, "filterPop", arglist, i);
   XtManageChild(filterform);

   i = 0;
   XtSetArg(arglist[i], XmNx,2); i++;
   XtSetArg(arglist[i], XmNwidth, 300); i++;
   XtSetArg(arglist[i], XmNborderWidth, 1); i++;
   escapeButtonW = XmCreatePushButton(filterform, "Escape", arglist, i);
   XtManageChild(escapeButtonW);
   XtAddCallback(escapeButtonW, XmNactivateCallback, escapeForm, NULL);

   /* static text arguments */
   i = 0;
   XtSetArg(arglist[i], XmNy, 50); i++;
   XtSetArg(arglist[i], XtNborderWidth, 0); i++;
   filterModeLabelW = XmCreateLabel(filterform, "                    Selection Method", arglist, i);
   XtManageChild(filterModeLabelW);

   /* Set up Row column widget */
   i = 0;
   XtSetArg(arglist[i], XmNy, 70); i++;
   XtSetArg(arglist[i], XmNx, 10); i++;
   XtSetArg(arglist[i], XmNnumColumns, 2); i++;
   XtSetArg(arglist[i], XmNpacking, XmPACK_COLUMN); i++;
   XtSetArg(arglist[i], XmNspacing, 60); i++;
   XtSetArg(arglist[i], XtNborderWidth, 1); i++;
   filterPopRowColW = XmCreateRowColumn(filterform, "filterRowCol", arglist, i);
   XtManageChild(filterPopRowColW);
    
   /* Set up Push button widget inside row column widget */
   i = 0;
   XtSetArg(arglist[i], XmNx, 2); i++;
   XtSetArg(arglist[i], XtNlabel, " Manual "); i++;
   manualMethodButtonW = XmCreateToggleButton(filterPopRowColW, "Manual", arglist, i);
   XtManageChild(manualMethodButtonW);
   XtAddCallback(manualMethodButtonW, XmNarmCallback, manualSelect, NULL);
           
   /* Set up Push button widget inside row column widget */
   /* Argument for push button widget */
   i = 0;
   automaticMethodButtonW = XmCreatePushButton(filterPopRowColW, "    Automatic", arglist, i);
   XtManageChild(automaticMethodButtonW);
   XtAddCallback(automaticMethodButtonW, XmNactivateCallback, automaticSelect, NULL);

   /* Set up Push button widget inside (outer) row column widget */
   i = 0;
   XtSetArg(arglist[i], XmNy, 120); i++;
   XtSetArg(arglist[i], XmNx, 10); i++;
   XtSetArg(arglist[i], XmNwidth, 300); i++;
   XtSetArg(arglist[i], XtNborderWidth, 1); i++;
   chunkUnselectButtonW = XmCreateToggleButton(filterform, "Unselect Time-segments",arglist,i);
   XtManageChild(chunkUnselectButtonW);

   /* Set up Row column widget */
   i = 0;
   XtSetArg(arglist[i], XmNy, 170); i++;
   XtSetArg(arglist[i], XmNx, 10); i++;
   XtSetArg(arglist[i], XmNwidth, 330); i++;
   XtSetArg(arglist[i], XmNnumColumns, 2); i++;
   XtSetArg(arglist[i], XmNpacking, XmPACK_COLUMN); i++;
   XtSetArg(arglist[i], XmNspacing, 116); i++;
   XtSetArg(arglist[i], XtNborderWidth, 1); i++;
   filterModeRowColW = XmCreateRowColumn(filterform, "filterModeRowCol", arglist, i);
   XtManageChild(filterModeRowColW);
            
   /* Set up Push button widget inside row column widget */
   i = 0;
   filterModeButtonW = XmCreatePushButton(filterModeRowColW, "FilterMode", arglist, i);
   XtManageChild(filterModeButtonW);
   XtAddCallback (filterModeButtonW, XmNactivateCallback, filterModeSelect, NULL);
            
   /* Set up static text widget inside row-col */

   filterMode = PASS;	/* initialize mode variable */

   i = 0;
   filterModeStatW=XmCreateLabel(filterModeRowColW, "Pass", arglist, i);
   XtManageChild(filterModeStatW);

   /* Set up toggle widget inside row-col */
   i = 0;
   XtSetArg(arglist[i], XmNy , 220); i++;
   XtSetArg(arglist[i], XmNx , 10); i++;
   XtSetArg(arglist[i], XmNwidth , 300); i++;
   XtSetArg(arglist[i], XtNborderWidth, 1); i++;
   markToggleW=XmCreateToggleButton(filterform, "Add markB & markE codes", arglist, i);
   XtManageChild(markToggleW);
           
   /* Reset Push button widget inside row-col */
   /* Argument for push button widget */
   i = 0;
   XtSetArg(arglist[i], XmNy , 260); i++;
   XtSetArg(arglist[i], XmNx , 10); i++;
   XtSetArg(arglist[i], XmNwidth, 300); i++;
   XtSetArg(arglist[i], XmNborderWidth, 1); i++;
   resetButtonW=XmCreatePushButton(filterform, "Reset", arglist, i);
   XtManageChild(resetButtonW);
   XtAddCallback (resetButtonW,XmNactivateCallback,resetSelect,NULL); 
   XtMoveWidget(filterPop, 810, 70);
}


/*********************************************************
 *      The automatic option popup of the filter tree    *
 *            initializations & definitions              *
 *********************************************************/
/* global variables for autoOptPop */

Widget setAutoW;

/* callback routines for the autoOptPop popup widgets */
void setAuto(w,closure,call_data)
   Widget w;
   XtPointer closure;
   XtPointer call_data;
{
   char *buff1, *buff2, *buff3, *buff4;
   float timespan;
   int trigger,firstTrig, lastTrig, ins;
   int trigIndex, done, code, time = 0, endTime, intTime;

   buff1 = XmTextGetString(triggerCodeEditW);
   buff2 = XmTextGetString(timespanEditW);
   buff3 = XmTextGetString(rangeBeginEditW);
   buff4 = XmTextGetString(rangeEndEditW);

   sscanf(buff1,"%d",&trigger);
   sscanf(buff2,"%f",&timespan);
   sscanf(buff3,"%d",&firstTrig);
   sscanf(buff4,"%d",&lastTrig);

   timespan *= ticks_per_second;
   intTime = timespan;
   refresh(); /** added by girish */
   refreshBlocks();
   rewind(fp);

   /* skip codes before the first trigger */

   done = False;
   trigIndex = 0;
   while(trigIndex < firstTrig && done == False)
      if((code=getw(fp))==EOF || (time=getw(fp))==EOF) 
	 done = True;
      else
	 if(code == trigger)
	    trigIndex++;

   /* mark the first block */
   if(done == False)
   {
      endTime = time + intTime;
      if((ins = InsBlock(segmentList,time,endTime)) == NIL)
         printf("failed attempt at block insert\n");
      while(time <= endTime && done == False)
         if((code=getw(fp))==EOF || (time=getw(fp))==EOF) 
	    done = True;
      if(done == False)
         fseek(fp,-8,1);
   }

   /* mark blocks until trigIndex > lastTrig */
   if(done == False)
   {
      while(done == False && trigIndex < lastTrig)
         if((code=getw(fp))==EOF || (time=getw(fp))==EOF) 
	    done = True;
         else
            if(code == trigger)
            {
               trigIndex++;
               endTime = time + intTime;
               if((ins = InsBlock(segmentList,time,endTime)) == NIL)
                  printf("failed attempt at block insert\n");
               while(time <= endTime && done == False)
                  if((code=getw(fp))==EOF || (time=getw(fp))==EOF) 
                     done = True;
               if(done == False)
                  fseek(fp,-8,1);
            }
   }
   refresh();
   refreshBlocks();
}


void makeAutoOptPop()
{
   /* local variable declarations */
   Arg arglist[20];
   Cardinal i;

   Widget autoOptFormW;
   Widget autoOptTitleW;
   Widget autoOptTitle;
   Widget autoOptTime;
   Widget autoOptTrigger;
   Widget autoOptRowCol;
   Widget ClearW;
   Widget rangeBeginStaticW;
   Widget rangeEndStaticW;

   /* Initialize toolkit */
   i = 0;
   autoOptPop=XmCreateDialogShell(app_shell, "autoOptPop", arglist, i);

   /* Set up a form widget for the manual option popup */
   i = 0;
   XtSetArg(arglist[i], XmNheight, 350); i++;
   XtSetArg(arglist[i], XmNwidth, 400); i++;
   autoOptFormW=XmCreateForm(autoOptPop, "autoOptForm", arglist, i);
   XtManageChild(autoOptFormW);
                    
   /* Set up Title Bar widget */
   /* Arguments for parent form widget */
   i = 0;
   XtSetArg(arglist[i], XmNnumColumns, 2); i++;
   XtSetArg(arglist[i], XmNpacking, XmPACK_COLUMN); i++;
   XtSetArg(arglist[i], XmNspacing, 110); i++;
   XtSetArg(arglist[i], XmNborderWidth, 2); i++;
   autoOptRowCol=XmCreateRowColumn(autoOptFormW, "autoOptRowCol", arglist, i);
   XtManageChild(autoOptRowCol);
          
   /* Arguments for parent form widget */
   i = 0;
   XtSetArg(arglist[i], XmNhighlightThickness, 1); i++;
   autoOptTitleW=XmCreateLabel(autoOptRowCol, "Automatic Filtering", arglist, i);
   XtManageChild(autoOptTitleW);
           
   /* Set Push button widget inside title bar*/
   /* Argument for push button widget */
   i = 0;
   setAutoW=XmCreatePushButton(autoOptRowCol, "            Set", arglist, i);
   XtManageChild(setAutoW);
   XtAddCallback (setAutoW, XmNactivateCallback, setAuto, NULL);

   i = 0;
   XtSetArg(arglist[i], XmNhighlightThickness, 1); i++;
   XtSetArg(arglist[i], XmNy, 50); i++;
   XtSetArg(arglist[i], XmNx, 2); i++;
   autoOptTitle=XmCreateLabel(autoOptFormW, "Trigger Code :", arglist, i);
   XtManageChild(autoOptTitle);

   /* Set up static text widget */
   i = 0;
   XtSetArg(arglist[i], XmNy, 50); i++;
   XtSetArg(arglist[i], XmNx, 170); i++;
   triggerCodeEditW=XmCreateText(autoOptFormW, "triggerCodeStatic", arglist, i);
   XtManageChild(triggerCodeEditW);
          
   i = 0;
   XtSetArg(arglist[i], XmNhighlightThickness, 1); i++;
   XtSetArg(arglist[i], XmNy, 100); i++;
   XtSetArg(arglist[i], XmNx, 2); i++;
   autoOptTime=XmCreateLabel(autoOptFormW, "Timespan :", arglist, i);
   XtManageChild(autoOptTime);

   /* Set up text edit widget for code input */
   /* Argument for text edit widget */

   i = 0;
   XtSetArg(arglist[i], XmNy, 100); i++;
   XtSetArg(arglist[i], XtNx, 170); i++;
   timespanEditW=XmCreateText(autoOptFormW, "triggerCodeEdit", arglist, i);
   XtManageChild(timespanEditW);
              
   i = 0;
   XtSetArg(arglist[i], XmNhighlightThickness, 1); i++;
   XtSetArg(arglist[i], XmNy, 150); i++;
   XtSetArg(arglist[i], XmNx, 2); i++;
   autoOptTrigger=XmCreateLabel(autoOptFormW, "Trigger Code\nRange", arglist, i);
   XtManageChild(autoOptTrigger);

   /* Set up static text widget */
   i = 0;
   XtSetArg(arglist[i], XmNx, 100); i++;
   XtSetArg(arglist[i], XmNy, 200); i++;
   rangeBeginStaticW=XmCreateLabel(autoOptFormW, "Begin :", arglist, i);
   XtManageChild(rangeBeginStaticW);
          
   /* Set up text edit widget for code input */
   /* Argument for text edit widget */
   i = 0;
   XtSetArg(arglist[i], XmNx, 170); i++;
   XtSetArg(arglist[i], XmNy, 200); i++;
   rangeBeginEditW=XmCreateText(autoOptFormW, "rangeBeginEdit", arglist, i);
   XtManageChild(rangeBeginEditW);
           
   /* Set up static text widget */
   i = 0;
   XtSetArg(arglist[i], XtNborderWidth, 0); i++;
   XtSetArg(arglist[i], XmNx, 100); i++;
   XtSetArg(arglist[i], XmNy, 250); i++;
   rangeEndStaticW=XmCreateLabel(autoOptFormW, "End :", arglist, i);
   XtManageChild(rangeEndStaticW);
        
   /* Set up text edit widget for code input */
   /* Argument for text edit widget */
   i = 0;
   XtSetArg(arglist[i], XmNx, 170); i++;
   XtSetArg(arglist[i], XmNy, 250); i++;
   rangeEndEditW=XmCreateText(autoOptFormW, "rangeEndEdit", arglist, i);
   XtManageChild(rangeEndEditW);
                
   /* Set up text edit widget for code input */
   /* Argument for text edit widget */
   i = 0;
   XtSetArg(arglist[i], XmNx, 170); i++;
   XtSetArg(arglist[i], XmNy, 300); i++;
   ClearW=XmCreatePushButton(autoOptFormW, "Clear", arglist, i);
   XtManageChild(ClearW);
   XtAddCallback (ClearW, XmNactivateCallback,autoreset,NULL);
           
   XtRealizeWidget(autoOptPop);
}


/* The following variable and routines constitute a toggle.  Although
   it may seem unnecessary to isolate toggle access to these routines,
   I am compelled to do so by an irresistable force.			*/
static int begin_end_toggle;

void ResetTog(void)
{
   begin_end_toggle = RESET;
}

int LookTog(void)
{
   return(begin_end_toggle);
}

void ActuateTog(void)
{
   if(begin_end_toggle == RESET)
      begin_end_toggle = SET;
   else
      begin_end_toggle = RESET;
}



/* This procedure initializes all popups which comprise the filter tree */
void makeFilterTree(void)
{
   ResetTog();
   InitListTools();
   segmentList = InitList();
}

/* Local Variables: */
/* c-basic-offset: 3 */
/* c-file-offsets:  ((substatement-open . 0)) */
/* End: */
