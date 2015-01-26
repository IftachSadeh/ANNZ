// ===========================================================================================================
// Copyright (C) 2015, Iftach Sadeh
// 
// This file is part of ANNZ.
// ANNZ is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
// ===========================================================================================================

#ifndef OutMngr_h
#define OutMngr_h

#include <commonInclude.hpp>
#include <OptMaps.hpp>
#include <Utils.hpp>

class TCanvas;
class TMultiGraph;
class TProfile2D;
class TString;
class TPad;
class TGraph;
class TGraphAsymmErrors;

// ===========================================================================================================
class OutMngr {
// ============

public:
		    
  OutMngr(TString aName = "MyOutMngr", Utils * aUtils = NULL, OptMaps * aMaps = NULL);
  ~OutMngr();  

  void     InitializeDir(TString outDirNameNow, TString outFileNameNow);
  void     SetOutDirName(TString outDirNameNow);
  void     SetOutFileName(TString outFileNameNow);
  TString  GetOutDirName();
  TString  GetOutFileName();

  void     SetMyStyle();
  void     WriteOutObjects(bool writePdfScripts = false, bool dontWriteHis = false);
  void     ResetObjects();
  void     optClear();

  void     SetHisStyle(TH1 * his);
  void     SetHisStyle(TGraph * grph, TMultiGraph * mGrph = NULL);
  void     PrepCanvas(vector <TPad*> * padV = NULL, int nPads = 0);

  void     normHisV(vector <TH1*> & hisV);
  void     setCommonHislimits(vector<TH1 *> & hisV);
  void     findMinMaxForPlotting(vector<TH1 *> & dataV, map <TString,double> * maxMinVal);
  void     buildLegend(map <TString,TString> & drawOptV, TPad * pad);
  void     DrawText();
  void     DrawLines();

  bool     drawHis1dV(TH1 * his1 = NULL, TPad * pad = NULL);
  bool     drawHis1dV(vector <TH1*> & hisVin, TPad * pad = NULL);

  void     drawHis1dMultiV(vector <TH1*> & hisMultiVin);
  void     drawHis1dMultiV(vector < vector <TH1*> > & hisMultiVin);

  void     drawMultiGraph(TMultiGraph * mGrph, TPad * pad = NULL);
  void     drawMultiGraphV(vector <TMultiGraph*> mGrphVin);
  void     drawMultiGraphV(TMultiGraph * mGrph = NULL);

  OptMaps    * glob, * draw;
  Utils      * utils;

  TString    CT;
  TString    coutDef, coutRed, coutGreen, coutBlue, coutLightBlue, coutYellow, coutPurple, coutCyan,
             coutUnderLine, coutWhiteOnBlack, coutWhiteOnRed, coutWhiteOnGreen, coutWhiteOnYellow;

  int        OutputRootFileIndex, OutputTreeFileIndex;
  TFile      * OutputRootFile;
  TDirectory * BaseDir;

  std::set <int>            excHisIndex;
  vector   <TString>        titleV;
  map < int , TString >     nameMap, titleMap;
  map < TString , double >  fitParMap;

  map < TString , TCanvas * > 		          CanvasMap;
	map < TString , TCanvas * >::iterator	    CanvasMapItr;
	map < TString , TH1 * > 		              HisMap1D;
	map < TString , TH1 * >::iterator        	HisMap1DItr;
	map < TString , TH2 * > 		              HisMap2D;
	map < TString , TH2 * >::iterator      	  HisMap2DItr;
	map < TString , TH3 * > 		              HisMap3D;
	map < TString , TH3 * >::iterator      	  HisMap3DItr;
	map < TString , TGraph * > 		            TGraphErrorsMap;
	map < TString , TGraph * >::iterator	    TGraphErrorsMapItr;
	map < TString , TProfile * > 		          HisMapProf;
	map < TString , TProfile * >::iterator	  HisMapProfItr;
	map < TString , TProfile2D * > 		        HisMapProf2D;
	map < TString , TProfile2D * >::iterator  HisMapProfItr2D;
	map < TString , TTree * > 		            TreeMap;
	map < TString , TTree * >::iterator	      TreeMapItr;

private:
  TString	 outputRootFileName, outDirName, outFileName;
  
};

#endif
