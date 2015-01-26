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

// ===========================================================================================================
void OutMngr::SetOutDirName(TString outDirNameNow) { 
  if(!outDirNameNow.EndsWith("/")) outDirNameNow += "/";

  outDirName = outDirNameNow;
  utils->checkPathPrefix(outDirName);
  utils->validDirExists(outDirName);

  return;
}
// ===========================================================================================================
void OutMngr::SetOutFileName(TString outFileNameNow) {
  outFileName = outFileNameNow;
  return;
}
// ===========================================================================================================
TString OutMngr::GetOutDirName()  {
  return outDirName;
}
// ===========================================================================================================
TString OutMngr::GetOutFileName() {
  return outFileName;
}
// ===========================================================================================================
void OutMngr::InitializeDir(TString outDirNameNow , TString outFileNameNow) {
// ==========================================================================
  if(!outDirNameNow.EndsWith("/")) outDirNameNow += "/";

  aLOG(Log::DEBUG) << coutBlue<<" - InitializeDir("<<outDirNameNow<<" , "<<outFileNameNow<<") ..."<<coutDef<<endl;

  OutputRootFileIndex = OutputTreeFileIndex = -1;
  
  utils->resetDirectory(outDirNameNow);
  SetOutDirName(outDirNameNow);
  SetOutFileName(outFileNameNow);

  return;
}
// ===========================================================================================================


// ===========================================================================================================
void OutMngr::WriteOutObjects(bool writePdfScripts, bool dontWriteHis) {
// =====================================================================
  aLOG(Log::DEBUG_1) <<coutWhiteOnBlack<<coutBlue<<" - starting OutMngr::WriteOutObjects() ... "<<coutDef<<endl;

  int numHis;

  // tree map
  // -----------------------------------------------------------------------------------------------------------
  numHis            = TreeMap.size();
  TreeMapItr        = TreeMap.begin();
  for(int hisNow = 0; hisNow < numHis; hisNow++ , ++TreeMapItr) {
    TString hisName = (TString)(*TreeMapItr).first;

    if(!dynamic_cast<TTree*>(TreeMap[hisName])) continue;
    if(TreeMap[hisName]->GetEntries() < 1)      continue;

    OutputTreeFileIndex++;
    outputRootFileName = (TString)outDirName+hisName+"_"+TString::Format("%1.5d",OutputTreeFileIndex)+".root";
    OutputRootFile = new TFile(outputRootFileName,"RECREATE");

    if(dynamic_cast<TChain*>(TreeMap[hisName])) ((TChain*)TreeMap[hisName])->Merge(outputRootFileName);
    else                                                  TreeMap[hisName] ->Write();
    
    OutputRootFile->Close();  DELNULL(OutputRootFile);
    aLOG(Log::DEBUG) << coutCyan<<" - Wrote tree to: "<<coutPurple<<outputRootFileName<<coutDef<<endl;
  }

  if(writePdfScripts) {
    TString outDirNamePlot = (TString)outDirName+"plots/";
    utils->exeShellCmndOutput((TString)"mkdir -p "+outDirNamePlot,false,true);
    aLOG(Log::INFO) << coutCyan<<" - Writing to plotting directory "<<coutPurple<<outDirNamePlot<<coutDef<<endl;

    outputRootFileName = (TString)outDirNamePlot+outFileName+"_TMP.root";
    OutputRootFile     = new TFile(outputRootFileName,"RECREATE");

    TString printPlotExtension(glob->GetOptC("printPlotExtension"));
    vector <TString> eraseKeys;

    numHis            = CanvasMap.size();
    CanvasMapItr      = CanvasMap.begin();
    for(int hisNow = 0; hisNow < numHis; hisNow++ , ++CanvasMapItr) {
      TString hisName = (TString)(*CanvasMapItr).first;

      // the order matters for some reason...  cnvs->Write() , cnvs->Print(name,"pdf") , cnvs->SaveAs(name+".C")
      if(dynamic_cast<TCanvas*>(CanvasMap[hisName])) {
        CanvasMap[hisName]->Write();
        CanvasMap[hisName]->Print((TString)outDirNamePlot+hisName+".C");
        if(printPlotExtension != "")
          CanvasMap[hisName]->SaveAs((TString)outDirNamePlot+hisName+"."+printPlotExtension);
    
        eraseKeys.push_back(hisName);
      }
    }

    for(int eraseKeyNow=0; eraseKeyNow<int(eraseKeys.size()); eraseKeyNow++) CanvasMap.erase(eraseKeys[eraseKeyNow]);
    eraseKeys.clear();

    OutputRootFile->Close();  DELNULL(OutputRootFile);
    utils->exeShellCmndOutput((TString)"rm -rf "+outputRootFileName);
  }

  if(dontWriteHis) return;

  OutputRootFileIndex++;
  outputRootFileName = (TString)outDirName+outFileName+"_"+TString::Format("%1.5d",OutputRootFileIndex)+".root";
  OutputRootFile = new TFile(outputRootFileName,"RECREATE");
  //cout<<coutRed<<"\toutputRootFileName = "<<outputRootFileName<<coutDef<<endl;

  // 1D histogram map
  // -----------------------------------------------------------------------------------------------------------
  numHis            = HisMap1D.size();
  HisMap1DItr       = HisMap1D.begin();
  for(int hisNow = 0; hisNow < numHis; hisNow++ , HisMap1DItr++) {
    TString hisName = (TString)(*HisMap1DItr).first;

    if(dynamic_cast<TH1*>((*HisMap1DItr).second)) HisMap1D[hisName]->Write();
  }

  // 2D histogram map
  // -----------------------------------------------------------------------------------------------------------
  numHis            = HisMap2D.size();
  HisMap2DItr       = HisMap2D.begin();
  for(int hisNow = 0; hisNow < numHis; hisNow++ , HisMap2DItr++) {
    TString hisName = (TString)(*HisMap2DItr).first;

    if(dynamic_cast<TH2*>((*HisMap2DItr).second)) HisMap2D[hisName]->Write();
  }

  // 3D histogram map
  // -----------------------------------------------------------------------------------------------------------
  numHis            = HisMap3D.size();
  HisMap3DItr       = HisMap3D.begin();
  for(int hisNow = 0; hisNow < numHis; hisNow++ , HisMap3DItr++) {
    TString hisName = (TString)(*HisMap3DItr).first;

    if(dynamic_cast<TH3*>((*HisMap3DItr).second)) HisMap3D[hisName]->Write();
  }

  // profile histogram
  // -----------------------------------------------------------------------------------------------------------
  numHis             = HisMapProf.size();
  HisMapProfItr      = HisMapProf.begin();
  for(int hisNow = 0; hisNow < numHis; hisNow++ , HisMapProfItr++) {
    TString hisName = (TString)(*HisMapProfItr).first;

    if(dynamic_cast<TProfile*>((*HisMapProfItr).second)) HisMapProf[hisName]->Write();
  }

  // profile2D histogram
  // -----------------------------------------------------------------------------------------------------------
  numHis               = HisMapProf2D.size();
  HisMapProfItr2D = HisMapProf2D.begin();
  for(int hisNow = 0; hisNow < numHis; hisNow++ , HisMapProfItr2D++) {
    TString hisName = (TString)(*HisMapProfItr2D).first;

    if(dynamic_cast<TProfile2D*>((*HisMapProfItr2D).second)) HisMapProf2D[hisName]->Write();
  }

  // TGraphErrorsMap
  // -----------------------------------------------------------------------------------------------------------
  numHis                   = TGraphErrorsMap.size();
  TGraphErrorsMapItr       = TGraphErrorsMap.begin();
  for(int hisNow = 0; hisNow < numHis; hisNow++ , TGraphErrorsMapItr++) {
    TString hisName = (TString)(*TGraphErrorsMapItr).first;

    if(dynamic_cast<TGraph*>((*TGraphErrorsMapItr).second)) TGraphErrorsMap[hisName]->Write();
  }

  OutputRootFile->Close();  DELNULL(OutputRootFile);
  aLOG(Log::DEBUG) << coutCyan<<" - Wrote HIS/GRPH/CNVS to output file: "<<coutPurple<<outputRootFileName<<coutDef<<endl;
  return;
}

// ===========================================================================================================
void OutMngr::ResetObjects() {
// ===========================
  aLOG(Log::DEBUG_1) <<coutWhiteOnBlack<<coutBlue<<" - starting OutMngr::ResetObjects() ... "<<coutDef<<endl;

  int numHis;

  // 1D histogram map
  numHis            = HisMap1D.size();
  HisMap1DItr       = HisMap1D.begin();
  for(int hisNow = 0; hisNow < numHis; hisNow++ , HisMap1DItr++) {
    TString hisName = (TString)(*HisMap1DItr).first;
    if(glob->OptOrNullB((TString)"NoReset_"+hisName)) continue;
    if((*HisMap1DItr).second != NULL) HisMap1D[hisName]->Reset();
  }

  // 2D histogram map
  numHis            = HisMap2D.size();
  HisMap2DItr       = HisMap2D.begin();
  for(int hisNow = 0; hisNow < numHis; hisNow++ , HisMap2DItr++) {
    TString hisName = (TString)(*HisMap2DItr).first;
    if(glob->OptOrNullB((TString)"NoReset_"+hisName)) continue;
    if((*HisMap2DItr).second != NULL) HisMap2D[hisName]->Reset();
  }

  // 3D histogram map
  numHis            = HisMap3D.size();
  HisMap3DItr       = HisMap3D.begin();
  for(int hisNow = 0; hisNow < numHis; hisNow++ , HisMap3DItr++) {
    TString hisName = (TString)(*HisMap3DItr).first;
    if(glob->OptOrNullB((TString)"NoReset_"+hisName)) continue;
    if((*HisMap3DItr).second != NULL) HisMap3D[hisName]->Reset();
  }

  // profile histogram
  numHis             = HisMapProf.size();
  HisMapProfItr      = HisMapProf.begin();
  for(int hisNow = 0; hisNow < numHis; hisNow++ , HisMapProfItr++) {
    TString hisName = (TString)(*HisMapProfItr).first;
    if(glob->OptOrNullB((TString)"NoReset_"+hisName)) continue;
    if((*HisMapProfItr).second != NULL) HisMapProf[hisName]->Reset();
  }

  // profile2D histogram
  numHis               = HisMapProf2D.size();
  HisMapProfItr2D = HisMapProf2D.begin();
  for(int hisNow = 0; hisNow < numHis; hisNow++ , HisMapProfItr2D++) {
    TString hisName = (TString)(*HisMapProfItr2D).first;
    if(glob->OptOrNullB((TString)"NoReset_"+hisName)) continue;
    if((*HisMapProfItr2D).second != NULL) HisMapProf2D[hisName]->Reset();
  }

  // TGraphErrorsMap
  numHis                   = TGraphErrorsMap.size();
  TGraphErrorsMapItr       = TGraphErrorsMap.begin();
  for(int hisNow = 0; hisNow < numHis; hisNow++ , TGraphErrorsMapItr++) {
    TString hisName = (TString)(*TGraphErrorsMapItr).first;
    if(glob->OptOrNullB((TString)"NoReset_"+hisName)) continue;

    if(dynamic_cast<TGraph*>((*TGraphErrorsMapItr).second)) {
      TString grphName = TGraphErrorsMap[hisName]->GetName();
      if     (dynamic_cast<TGraphAsymmErrors*>(TGraphErrorsMap[hisName])) {
        DELNULL(TGraphErrorsMap[hisName]); TGraphErrorsMap[hisName] = new TGraphAsymmErrors();
      }
      else if(dynamic_cast<TGraphErrors*>(TGraphErrorsMap[hisName])) {
        DELNULL(TGraphErrorsMap[hisName]); TGraphErrorsMap[hisName] = new TGraphErrors();
      }
      else if(dynamic_cast<TGraph*>(TGraphErrorsMap[hisName])) {
        DELNULL(TGraphErrorsMap[hisName]); TGraphErrorsMap[hisName] = new TGraph();
      }
      TGraphErrorsMap[hisName]->SetName(grphName);
    }
  }

  // tree map
  numHis            = TreeMap.size();
  TreeMapItr        = TreeMap.begin();
  for(int hisNow = 0; hisNow < numHis; hisNow++ , ++TreeMapItr) {
    TString hisName = (TString)(*TreeMapItr).first;
    if(glob->OptOrNullB((TString)"NoReset_"+hisName)) continue;
    if((*TreeMapItr).second != NULL) TreeMap[hisName]->Reset();
  }  
  
  //cout<<coutGreen<<"Done with: OutMngr::ResetObjects ..."<<coutDef<<endl;
  return;
}

// ===========================================================================================================
void OutMngr::optClear() {
// =======================
  draw->clearAll(); titleV.clear(); excHisIndex.clear();
  return;
}




