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


void showTally (void);
void makeScalePop (void);
void makeHistogramPop (Widget w, XtPointer client_data, XtPointer call_data);
//void makeHistogramPop (void);
void makeIntegratePop (void);
void makeAboutPop (void);
void makeFindPop (void);
void ClearSideBar (void);
void showSPS (int n, double sps);
void createTallyStat (int loc);
void destroySideBar (void);
void makeBandpassPop (Widget w, XtPointer client_data, XtPointer call_data);
void makePSPop (Widget w, XtPointer client_data, XtPointer call_data);
void makeLeftPop (Widget w, XtPointer client_data, XtPointer call_data);
void set_left_time (void);

extern int hist_bintotal;
extern int nextStat;
extern int  binwidth;
extern float scalefactor;


