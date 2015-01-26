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
void OutMngr::SetHisStyle(TH1 * his) {
// ===================================

  double titleSizeX(0.05), titleSizeY(0.05), titleSizeZ(0.05), labelSize(0.05), titleOffsetX(1.2), titleOffsetY(1.5), titleOffsetZ(1.2);
  if(draw->OptOrNullB("wideCnvs")) titleOffsetY = 1.2;

  if(draw->HasOptF("titleOffsetX")) titleOffsetX = draw->GetOptF("titleOffsetX"); if(draw->HasOptF("titleSizeX")) titleSizeX = draw->GetOptF("titleSizeX");
  if(draw->HasOptF("titleOffsetY")) titleOffsetY = draw->GetOptF("titleOffsetY"); if(draw->HasOptF("titleSizeY")) titleSizeY = draw->GetOptF("titleSizeY");
  if(draw->HasOptF("titleOffsetZ")) titleOffsetZ = draw->GetOptF("titleOffsetZ"); if(draw->HasOptF("titleSizeZ")) titleSizeZ = draw->GetOptF("titleSizeZ");

  his->GetXaxis()->SetTitle(draw->OptOrNullC("axisTitleX"));
  his->GetYaxis()->SetTitle(draw->OptOrNullC("axisTitleY"));
  his->GetZaxis()->SetTitle(draw->OptOrNullC("axisTitleZ"));

  his->GetXaxis()->SetTitleSize(titleSizeX);  his->GetXaxis()->SetTitleOffset(titleOffsetX); his->GetXaxis()->SetLabelSize(labelSize);
  his->GetYaxis()->SetTitleSize(titleSizeY);  his->GetYaxis()->SetTitleOffset(titleOffsetY); his->GetYaxis()->SetLabelSize(labelSize);
  his->GetZaxis()->SetTitleSize(titleSizeZ);  his->GetZaxis()->SetTitleOffset(titleOffsetZ); his->GetZaxis()->SetLabelSize(labelSize);
  
  if(draw->HasOptF("LabelsOffsetX")) his->GetXaxis()->SetLabelOffset(draw->GetOptF("LabelsOffsetX"));
  if(draw->HasOptF("LabelsOffsetY")) his->GetYaxis()->SetLabelOffset(draw->GetOptF("LabelsOffsetY"));
  if(draw->HasOptF("LabelsOffsetZ")) his->GetZaxis()->SetLabelOffset(draw->GetOptF("LabelsOffsetZ"));

  if(draw->HasOptF("LabelsSizeX")) his->GetXaxis()->SetLabelSize(draw->GetOptF("LabelsSizeX"));
  if(draw->HasOptF("LabelsSizeY")) his->GetYaxis()->SetLabelSize(draw->GetOptF("LabelsSizeY"));
  if(draw->HasOptF("LabelsSizeZ")) his->GetZaxis()->SetLabelSize(draw->GetOptF("LabelsSizeZ"));

  his->SetLineStyle(1); his->SetLineWidth(1); his->SetFillStyle(0); his->SetMarkerSize(1);

  if(draw->OptOrNullB("moreLogX")) { his->GetXaxis()->SetMoreLogLabels(); }
  if(draw->OptOrNullB("moreLogY")) { his->GetYaxis()->SetMoreLogLabels(); }
  if(draw->OptOrNullB("moreLogZ")) { his->GetZaxis()->SetMoreLogLabels(); }

  return;
}

// ===========================================================================================================
void OutMngr::SetHisStyle(TGraph * grph, TMultiGraph * mGrph) {
// ============================================================

  double titleSizeX(0.05), titleSizeY(0.05), labelSize(0.05), titleOffsetX(1.2), titleOffsetY(1.5);
  if(draw->OptOrNullB("wideCnvs")) titleOffsetY = 1.2;

  if(draw->HasOptF("titleOffsetX")) titleOffsetX = draw->GetOptF("titleOffsetX"); if(draw->HasOptF("titleSizeX") > 0) titleSizeX = draw->GetOptF("titleSizeX");
  if(draw->HasOptF("titleOffsetY")) titleOffsetY = draw->GetOptF("titleOffsetY"); if(draw->HasOptF("titleSizeY") > 0) titleSizeY = draw->GetOptF("titleSizeY");

  grph->GetXaxis()->SetTitle(draw->OptOrNullC("axisTitleX"));
  grph->GetYaxis()->SetTitle(draw->OptOrNullC("axisTitleY"));

  grph->GetXaxis()->SetTitleSize(titleSizeX);  grph->GetXaxis()->SetTitleOffset(titleOffsetX); grph->GetXaxis()->SetLabelSize(labelSize);
  grph->GetYaxis()->SetTitleSize(titleSizeY);  grph->GetYaxis()->SetTitleOffset(titleOffsetY); grph->GetYaxis()->SetLabelSize(labelSize);

  if(draw->HasOptF("LabelsOffsetX")) grph->GetXaxis()->SetLabelOffset(draw->GetOptF("LabelsOffsetX"));
  if(draw->HasOptF("LabelsOffsetY")) grph->GetYaxis()->SetLabelOffset(draw->GetOptF("LabelsOffsetY"));
  
  if(draw->HasOptF("LabelsSizeX")) grph->GetXaxis()->SetLabelSize(draw->GetOptF("LabelsSizeX"));
  if(draw->HasOptF("LabelsSizeY")) grph->GetYaxis()->SetLabelSize(draw->GetOptF("LabelsSizeY"));

  grph->SetLineStyle(1); grph->SetLineWidth(1); grph->SetFillStyle(0); grph->SetMarkerSize(1);

  if(draw->OptOrNullB("moreLogX")) { grph->GetXaxis()->SetMoreLogLabels(); }
  if(draw->OptOrNullB("moreLogY")) { grph->GetYaxis()->SetMoreLogLabels(); }

  if(dynamic_cast<TMultiGraph*>(mGrph)) {
    grph->GetXaxis()->Copy(*mGrph->GetXaxis()); grph->GetYaxis()->Copy(*mGrph->GetYaxis()); // must come after: mGrph->Draw("a")
  }

  return;
}

// ===========================================================================================================
void OutMngr::PrepCanvas(vector <TPad*> * padV, int nPads) {
// =========================================================

  TCanvas * cnvs(NULL); TPad * pad0(NULL);
  
  int     nPadsRow((draw->OptOrNullI("nPadsRow")>0)?draw->GetOptI("nPadsRow"):3), nBinsSqrt(0);
  int     cnvsWidth(550), cnvsHight(500), cnvsPosX(0), cnvsPosY(0);
  double  topMarginC(0.15), bottomMarginC(0.15), leftMarginC(0.18), rightMarginC(0.17);
  double  topMarginP(0.15), bottomMarginP(0.15), leftMarginP(0.18), rightMarginP(0.17);

  if(draw->OptOrNullB("wideCnvs")) {
  ////assert(nPads == 0);
    cnvsWidth = 730; leftMarginP = 0.137; rightMarginP = 0.377;
  }

  if(nPads) {
    double scaleW(.9 - 0.05*int(nPads/double(nPadsRow))), scaleH(.65);
    nBinsSqrt = int(sqrt(double(nPads)))+1;

    if     (nPads <= nPadsRow)    cnvsWidth = int(cnvsWidth*nPads    *scaleW);
    else if(nPads <= nPadsRow*3)  cnvsWidth = int(cnvsWidth*nPadsRow *scaleW);
    else                          cnvsWidth = int(cnvsWidth*nBinsSqrt*scaleW);
    
    if(nPads  > nPadsRow   && nPads <= nPadsRow*2)  cnvsHight = int(cnvsHight*2        *scaleH);
    if(nPads  > nPadsRow*2 && nPads <= nPadsRow*3)  cnvsHight = int(cnvsHight*3        *scaleH);
    if(nPads  > nPadsRow*3)                         cnvsHight = int(cnvsHight*nBinsSqrt*scaleH);
  }
  
  while(CanvasMap[draw->GetOptC("cnvsName")] != NULL) draw->SetOptC("cnvsName",draw->GetOptC("cnvsName")+(TString)"_");  
  cnvs = new TCanvas(draw->GetOptC("cnvsName"),draw->GetOptC("cnvsName"),cnvsPosX,cnvsPosY,cnvsWidth,cnvsHight);  
  CanvasMap[draw->GetOptC("cnvsName")] = cnvs;  if(padV) padV->push_back(cnvs);

  cnvs->SetHighLightColor(2);       cnvs->SetFillColor(0);              cnvs->SetFillStyle(4000);       cnvs->SetBorderMode(0);
  cnvs->SetBorderSize(0);           cnvs->SetTickx(1);                  cnvs->SetTicky(1);
  cnvs->SetLeftMargin(leftMarginC); cnvs->SetRightMargin(rightMarginC); cnvs->SetTopMargin(topMarginC); cnvs->SetBottomMargin(bottomMarginC);
  cnvs->SetFrameFillColor(0);       cnvs->SetFrameLineWidth(0);         cnvs->SetFrameBorderMode(0);

  if(nPads) {
    if(nPads <= nPadsRow)                           cnvs->Divide(nPads,            1,0.0001,0.0001);
    if(nPads  > nPadsRow   && nPads <= nPadsRow*2)  cnvs->Divide(nPadsRow,         2,0.0001,0.0001);
    if(nPads  > nPadsRow*2 && nPads <= nPadsRow*3)  cnvs->Divide(nPadsRow,         3,0.0001,0.0001);
    if(nPads  > nPadsRow*3)                         cnvs->Divide(nBinsSqrt,nBinsSqrt,0.0001,0.0001);
  }
  else {
    pad0 = new TPad(draw->GetOptC("pad0Name"),draw->GetOptC("pad0Name"),0,0,1,1);  if(padV) padV->push_back(pad0);
    pad0->Draw();  pad0->cd();
  }

  for(int nPadNow=-1; nPadNow<nPads; nPadNow++) {
    TPad * padNow(NULL);
    if(nPadNow == -1) { if(pad0) padNow = pad0; }
    else              { padNow = (TPad*)cnvs->GetPad(nPadNow+1); }
    if(!dynamic_cast<TPad*>(padNow)) continue;

    padNow->SetLeftMargin(leftMarginP);  padNow->SetRightMargin(rightMarginP);
    padNow->SetTopMargin(topMarginP);    padNow->SetBottomMargin(bottomMarginP);  //pad0->SetTopMargin(0.148578);     pad0->SetBottomMargin(0.1507561);

    if(draw->OptOrNullB("setLogX"))  padNow->SetLogx();  if(draw->OptOrNullB("setLogY"))  padNow->SetLogy();  if(draw->OptOrNullB("setLogZ"))  padNow->SetLogz();
    if(draw->OptOrNullB("setGridX")) padNow->SetGridx(); if(draw->OptOrNullB("setGridY")) padNow->SetGridy();
  }

  return;
}

// ===========================================================================================================
void OutMngr::buildLegend(map <TString,TString> & drawOptV, TPad * pad) {
// ======================================================================
  if(draw->OptOrNullB("noLegend")) return;

  TLegend * leg(NULL); leg = pad->BuildLegend(); if(!dynamic_cast<TLegend*>(leg)) return;
  
  double  textSize = glob->GetOptF("txtSize")-0.01;
  //double  posX1(0.65), posY1(0.55), posX2(0.90), posY2(0.80);
  double  posX1(0.22), posY1(0.65), posX2(0.47), posY2(0.81);

  if(draw->OptOrNullB("wideCnvs")) {
  //posX1 = 0.657; posY1 = 0.019; posX2 = 0.908; posY2 = 0.930;
    posX1 = 0.685; posY1 = 0.019; posX2 = 0.908; posY2 = 0.930;
  }

  leg->SetTextSize(textSize); leg->SetFillColor(0); leg->SetFillStyle(0); leg->SetBorderSize(0);
  leg->SetX1(posX1); leg->SetX2(posX2); leg->SetY1(posY1); leg->SetY2(posY2);

  TObjLink * lnk = leg->GetListOfPrimitives()->FirstLink();
  while(lnk) {
    TString hisName = ((TH1*)(((TLegendEntry*) lnk->GetObject())->GetObject()))->GetName();
    TString lableDrawOpt = "";

    TString drawOptNow = drawOptV[hisName];  drawOptNow.ReplaceAll("same",""); drawOptNow.ReplaceAll("SAME","");
    
    if(     drawOptNow.Contains("e",TString::kIgnoreCase)
        ||  drawOptNow.Contains("p",TString::kIgnoreCase))  lableDrawOpt += "p";

    if(drawOptNow.Contains("HIST",TString::kIgnoreCase) || drawOptNow.Contains("l")) lableDrawOpt += draw->OptOrNullB("legendHistOpt")?"f":"l";
    if(drawOptNow.Contains("BOX",TString::kIgnoreCase))  lableDrawOpt += "f";
    if(drawOptNow.Contains("SCAT",TString::kIgnoreCase)) lableDrawOpt += "p";

    if(lableDrawOpt == "") lableDrawOpt = "l";

    if(draw->HasOptC("forceLegendDrawOpt")) lableDrawOpt = draw->GetOptC("forceLegendDrawOpt");

    ((TLegendEntry*) lnk->GetObject())->SetOption(lableDrawOpt);
    lnk = lnk->Next();
  }

  if(draw->HasOptC("legHeader")) leg->SetHeader(draw->GetOptC("legHeader"));

  return;
}

// ===========================================================================================================
void  OutMngr::setCommonHislimits(vector<TH1 *> & hisV) {
// ======================================================

  draw->NewOptF("yPlotRangeL" , (draw->HasOptC("yRangeL"))  ? draw->GetOptC("yRangeL").Atof() : 0);
  draw->NewOptF("yPlotRangeH" , (draw->HasOptC("yRangeH"))  ? draw->GetOptC("yRangeH").Atof() : 0);

  if(draw->GetOptF("yPlotRangeL") >= draw->GetOptF("yPlotRangeH")) {
    map <TString,double> maxMinVal;
    findMinMaxForPlotting(hisV,&maxMinVal);

    if(maxMinVal["max"] > maxMinVal["min"]) {
      if(maxMinVal["max"] > 0) maxMinVal["max"] *= 1.2;  else maxMinVal["max"] *= .8;
      if(maxMinVal["min"] < 0) maxMinVal["min"] *= 1.2;  else maxMinVal["min"] *= .8;

      draw->SetOptF("yPlotRangeL" , maxMinVal["min"]);
      draw->SetOptF("yPlotRangeH" , maxMinVal["max"]);
    }
  }

  if(draw->GetOptF("yPlotRangeL") < draw->GetOptF("yPlotRangeH")) {
    for(int nHisNow=0; nHisNow<int(hisV.size()); nHisNow++) {
      hisV[nHisNow]->GetYaxis()->SetRangeUser(draw->GetOptF("yPlotRangeL"),draw->GetOptF("yPlotRangeH"));
      hisV[nHisNow]            ->SetMinimum  (draw->GetOptF("yPlotRangeL"));
      hisV[nHisNow]            ->SetMaximum  (draw->GetOptF("yPlotRangeH"));
    }
  }

  return;
}

// ===========================================================================================================
void  OutMngr::findMinMaxForPlotting(vector<TH1 *> & dataV, map <TString,double> * maxMinVal) {
// ============================================================================================

  vector <double> allVals;

  int nHis = (int)dataV.size();
  for(int nHisNow=0; nHisNow<nHis; nHisNow++) {
    if((int)excHisIndex.size() > 0 && excHisIndex.find(nHisNow) != excHisIndex.end()) continue;

    TH1 * his = dataV[nHisNow];

    int     nBinsX    = his->GetNbinsX();
    double  lowEdgeX  = his->GetXaxis()->GetBinLowEdge( his->GetXaxis()->GetFirst() );
    double  highEdgeX = his->GetXaxis()->GetBinUpEdge ( his->GetXaxis()->GetLast()  );

    for(int nBinNowX=-5; nBinNowX<nBinsX+5; nBinNowX++) {
      double binCenterX     = his->GetXaxis()->GetBinLowEdge(nBinNowX);
      if(binCenterX < lowEdgeX || binCenterX > highEdgeX) continue;
      double binContentNow  = his->GetBinContent(nBinNowX);
      if(fabs(binContentNow) < EPS) continue;
      allVals.push_back(binContentNow);
    }
  }

  (*maxMinVal)["max"] = -1; (*maxMinVal)["min"] = 1;
  if(int(allVals.size()) > 0){
    (*maxMinVal)["max"] = *max_element(allVals.begin(), allVals.end());
    (*maxMinVal)["min"] = *min_element(allVals.begin(), allVals.end());
  }

  return;
}

// ===========================================================================================================
void  OutMngr::normHisV(vector<TH1 *> & hisV) {
// ============================================
 
  for(int nHisNow=0; nHisNow<(int)hisV.size(); nHisNow++) {
    if(draw->OptOrNullB("doNormEntriesWidth")) {double normFactor = hisV[nHisNow]->GetEntries(); if(normFactor>0) hisV[nHisNow]->Scale(1/normFactor,"width");}
    if(draw->OptOrNullB("doNormIntegralWidth")){double normFactor = hisV[nHisNow]->Integral();   if(normFactor>0) hisV[nHisNow]->Scale(1/normFactor,"width");}
    if(draw->OptOrNullB("doNormIntegral"))     {double normFactor = hisV[nHisNow]->Integral();   if(normFactor>0) hisV[nHisNow]->Scale(1/normFactor);        }
    if(draw->OptOrNullB("doNormEntries"))      {double normFactor = hisV[nHisNow]->GetEntries(); if(normFactor>0) hisV[nHisNow]->Scale(1/normFactor);        }
    if(draw->OptOrNullB("doNormWidth"))        {double normFactor = 1.;                          if(normFactor>0) hisV[nHisNow]->Scale(1/normFactor,"width");}
  }

  return;
}

// ===========================================================================================================
void OutMngr::DrawText() {
// =======================

  TPaveText * title(NULL);
  if(draw->HasOptC("generalHeader")) {
    if(draw->GetOptC("generalHeader") != ""){
      title = new TPaveText(0.015,0.93,0.86,0.98,"blNDC"); //title = new TPaveText(.64,.93,.98,.98,"blNDC");
      title->SetTextSize(glob->GetOptF("txtSize")); title->SetBorderSize(0); title->SetFillStyle(0); title->SetLineColor(0); title->SetTextAlign(12);
      title->AddText(draw->GetOptC("generalHeader")); title->Draw();
    }
  }

  int nTitles = (int)titleV.size();
  if(nTitles > 0) {
    if(!draw->OptOrNullB("wideCnvs"))  title = new TPaveText(0.1831502,0.8544304,0.8278388,0.9978903,"blNDC");
    else                               title = new TPaveText(0.137741,0.8523207,0.6212121,0.9957806,"blNDC");
    title->SetTextSize(glob->GetOptF("txtSize")); title->SetBorderSize(0); title->SetFillStyle(0); title->SetLineColor(0);
   
    for(int nTitleNow=0; nTitleNow<nTitles; nTitleNow++) title->AddText(titleV[nTitleNow]);
    
    title->Draw();
  }

  return;
}

// ===========================================================================================================
void OutMngr::DrawLines() {
// ========================

  vector <TString> optNames;
  draw->GetAllOptNames(optNames,"C");

  for(int nOptNow=0; nOptNow<(int)optNames.size(); nOptNow++) {
    if(!(optNames[nOptNow].Contains("drawLine_"))) continue;

    if(optNames[nOptNow].Contains("_vert") && draw->GetOptC(optNames[nOptNow]) != "") {
      gPad->Update();
    //double x0 = gPad->GetFrame()->GetX1();
    //double x1 = gPad->GetFrame()->GetX2();
      double y0 = gPad->GetFrame()->GetY1();
      double y1 = gPad->GetFrame()->GetY2();
      double x0 = (draw->GetOptC(optNames[nOptNow])).Atof();
 
      TLine * line = new TLine(x0,y0,x0,y1);
      if(draw->HasOptI("drawLine_col")) line->SetLineColor(draw->GetOptI("drawLine_col"));
      if(draw->HasOptI("drawLine_sty")) line->SetLineStyle(draw->GetOptI("drawLine_sty"));
      line->SetLineWidth(2);
      line->Draw();
    }
  }
  optNames.clear();

  gPad->Update(); gPad->Modified();

  return;
}

// ===========================================================================================================
bool OutMngr::drawHis1dV(vector <TH1*> & hisVin, TPad * pad) {
// =============================== ===========================

  TString hisName("");
  int nHis = (int)hisVin.size();  if(nHis == 0) return false;
  double  totEntries(0); int nHisAccept(0);
  for(int nHisNow=0; nHisNow<nHis; nHisNow++) {
    double  entryNow = hisVin[nHisNow]->GetEntries();
    
    if(entryNow < EPS)                                                                continue;
    if((int)excHisIndex.size() > 0 && excHisIndex.find(nHisNow) != excHisIndex.end()) continue;
    
    nHisAccept++;  totEntries += entryNow;
  }
  if(totEntries < EPS) return false;

  if(draw->OptOrNullC("doHisRatio") != "" && nHisAccept < 2) {
    cout <<coutYellow<<"Cant run doHisRatio with only one his in hisV... "<<coutDef<<endl; return false;
  }

  if(nHis > 1 && draw->HasOptC("addSumHis")) {
    TH1 * hisSum = (TH1*)hisVin[0]->Clone((TString)hisVin[0]->GetName()+"_incl");
    for(int nHisNow=1; nHisNow<nHis; nHisNow++) hisSum->Add(hisVin[nHisNow]);
    hisSum->SetTitle(draw->GetOptC("addSumHis"));
    hisVin.insert(hisVin.begin(),hisSum);   nHis++;
  }

  // clone before anything else
  // -----------------------------------------------------------------------------------------------------------
  vector <TH1*> hisV(nHis);
  for(int nHisNow=0; nHisNow<nHis; nHisNow++) 
    hisV[nHisNow] = (TH1*)hisVin[nHisNow]->Clone((TString)hisVin[nHisNow]->GetName()+"_");

  if(!dynamic_cast<TPad*>(pad)) {
    vector <TPad*> padV;
    draw->NewOptC("cnvsName" , (TString)hisV[0]->GetName()+"_cnvs");
    draw->NewOptC("pad0Name" , (TString)hisV[0]->GetName()+"_pad0");
    PrepCanvas(&padV);  pad = padV[1];
    padV.clear();       assert(dynamic_cast<TPad*>(pad));
  } 
  pad->cd();

  int colOffset   = (draw->OptOrNullC("colOffset") == "") ? 0 : draw->OptOrNullC("colOffset").Atoi();
  int maxDrawMark = (!draw->HasOptC("maxDrawMark"))       ? 9 : draw->GetOptC("maxDrawMark").Atoi();
  
  // rebin all his (before normalizations etc.)
  if(draw->OptOrNullC("rebinX") != "") {
    for(int nHisNow=0; nHisNow<nHis; nHisNow++)
      hisV[nHisNow]->Rebin(draw->OptOrNullC("rebinX").Atoi());
  }

  // normalization before anything else....
  normHisV(hisV);

  if(draw->OptOrNullC("doHisRatio") != "") {
    int indexBaseHis = draw->GetOptC("doHisRatio").Atoi();  assert(indexBaseHis<nHis);
    TH1 * hisBase    = hisV[indexBaseHis];                  excHisIndex.insert(indexBaseHis);

    for(int nHisNow=0; nHisNow<nHis; nHisNow++) {
    //if(nHisNow == indexBaseHis)                                                       continue;
      if(hisV[nHisNow]->GetEntries() < EPS)                                             continue;
      if((int)excHisIndex.size() > 0 && excHisIndex.find(nHisNow) != excHisIndex.end()) continue;
      if(draw->OptOrNullC("onlyEverySuchBin") != "") 
        if(nHisNow%(draw->OptOrNullC("onlyEverySuchBin").Atoi()) != 0 && nHisNow > 0)   continue;

      if(draw->OptOrNullB("doRelHisRatio")) hisV[nHisNow]->Add(hisBase,-1);
      hisV[nHisNow]->Divide(hisBase);

      hisV[nHisNow]->GetXaxis()->UnZoom(); hisV[nHisNow]->GetYaxis()->UnZoom(); hisV[nHisNow]->GetZaxis()->UnZoom();
    }
  }

  // set common limits on the Y-axis (may be overriden by the parameters draw->optC["yRangeL"], draw->optC["yRangeH"])
  setCommonHislimits(hisV);

  map <TString,TString> drawOptV;

  int   nHisDrawn(0);
  bool  hasDrawn(0);
  for(int nHisNow=0; nHisNow<nHis; nHisNow++) {
    if(hisV[nHisNow]->GetEntries() < EPS)                                                                                         continue;
    if((int)excHisIndex.size() > 0 && excHisIndex.find(nHisNow) != excHisIndex.end())                                             continue;
    if(draw->OptOrNullC("onlyEverySuchBin") != "") if(nHisNow%(draw->OptOrNullC("onlyEverySuchBin").Atoi()) != 0 && nHisNow > 0)  continue;

    SetHisStyle(hisV[nHisNow]);
    int nColIndex = (draw->OptOrNullB("keepOrigColOrder")) ? nHisNow+colOffset : nHisDrawn+colOffset;

    hisV[nHisNow]->SetMarkerStyle(utils->markers[nColIndex]);
    hisV[nHisNow]->SetMarkerColor(utils->colours[nColIndex]); 
    hisV[nHisNow]->SetLineColor  (utils->colours[nColIndex]);
    hisV[nHisNow]->SetFillColor  (utils->colours[nColIndex]);

    TList * funcList = hisV[nHisNow]->GetListOfFunctions();
    int   nElements  = funcList->GetEntries();
    for(int nFuncNow=0; nFuncNow<nElements; nFuncNow++) {
      TF1 * funcNow = dynamic_cast<TF1*>(funcList->At(nFuncNow));  if(!funcNow) continue;

      funcNow->SetMarkerStyle(utils->markers[nColIndex]);
      funcNow->SetMarkerColor(utils->colours[nColIndex]); 
      funcNow->SetLineColor(utils->colours[nColIndex]);
    }
    if(nElements > 0 && draw->OptOrNullB("hideMarkers")) {
      hisV[nHisNow]->SetMarkerStyle(11);
      hisV[nHisNow]->SetMarkerColor(kWhite);
    }
    // consider also doing: for(int nBinXNow=1; nBinXNow<his1->GetNbinsX()+1; nBinXNow++) his1->SetBinError(nBinXNow,EPS);
    
    TString drawOpt = (draw->OptOrNullC("drawOpt") == "") ? "e1p" : draw->OptOrNullC("drawOpt");
    if(nHis > maxDrawMark) {
      if(nHisDrawn%2 == 1) { if(drawOpt.Contains("HIST")) drawOpt = "e1p"; else drawOpt = "HIST"; }
    } else {
      if(draw->HasOptC(TString::Format("drawOpt_%d",nHisNow))) drawOpt = draw->GetOptC(TString::Format("drawOpt_%d",nHisNow));
    }
    if(drawOpt == "x") {
      drawOpt = "p"; hisV[nHisNow]->SetMarkerStyle(5);
    }
    if(drawOpt == ".") {
      drawOpt = "p"; hisV[nHisNow]->SetMarkerStyle(7);
    }
    if(drawOpt == "o") {
      drawOpt = "p"; hisV[nHisNow]->SetMarkerStyle(24);
    }
    if(drawOpt == "t") {
      drawOpt = "p"; hisV[nHisNow]->SetMarkerStyle(26);
    }
    if(drawOpt == "s") {
      drawOpt = "p"; hisV[nHisNow]->SetMarkerStyle(25);
    }
    if(drawOpt.Contains("FILL_")) {
      drawOpt.ReplaceAll("FILL_",""); hisV[nHisNow]->SetFillStyle(drawOpt.Atoi());
      drawOpt = "HISTBOX";
    }
    if(hasDrawn) drawOpt += "same";
    drawOptV[hisV[nHisNow]->GetName()] = drawOpt;

    hisV[nHisNow]->Draw(drawOpt);
    hasDrawn = true; nHisDrawn++;

  //if(draw->HasOptC(TString::Format("drawOpt_%d",nHisNow))) cout <<nHisNow<<" "<<hisV[nHisNow]->GetName()<<CT<< draw->GetOptC(TString::Format("drawOpt_%d",nHisNow))<<endl;
  }

  if(nHisAccept > 1) buildLegend(drawOptV,pad);

  DrawText();
  DrawLines();

  drawOptV.clear();
  hisV.clear();
  return true;
}

// -----------------------------------------------------------------------------------------------------------
bool OutMngr::drawHis1dV(TH1 * his1, TPad * pad) {
// ===============================================
  if(!dynamic_cast<TH1*>(his1)) return false;

  vector <TH1*> hisV; hisV.push_back(his1);
 
  bool hasDrawn = drawHis1dV(hisV,pad);
  
  hisV.clear();
  return hasDrawn;
}

// ===========================================================================================================
void OutMngr::drawHis1dMultiV(vector <TH1*> & hisMultiVin) {
// =========================================================
  int nHisV = (int)hisMultiVin.size(); if(nHisV == 0) return;

  vector <TH1*>             hisMultiVNow(1);
  vector < vector <TH1*> >  hisMultiV(nHisV);
  for(int nHisVnow=0; nHisVnow<nHisV; nHisVnow++) {
    hisMultiVNow[0]     = hisMultiVin[nHisVnow];
    hisMultiV[nHisVnow] = hisMultiVNow;
  }

  drawHis1dMultiV(hisMultiV);

  for(int nHisVnow=0; nHisVnow<nHisV; nHisVnow++) hisMultiV[nHisVnow].clear();
  hisMultiV.clear(); hisMultiVNow.clear();

  return;
}

// ===========================================================================================================
void OutMngr::drawHis1dMultiV(vector < vector <TH1*> > & hisMultiVin) {
// ====================================================================
  int nHisV = (int)hisMultiVin.size(); if(nHisV == 0) return;

  vector < vector <TH1*> > hisMultiV;

  // only work with vectors of his which actually have filled histograms
  for(int nHisVnow=0; nHisVnow<nHisV; nHisVnow++) {
    vector <TH1*> hisV = hisMultiVin[nHisVnow];

    int nEntries = 0;
    int nHis     = (int)hisV.size(); if(nHis == 0) continue;
    for(int nHisNow=0; nHisNow<nHis; nHisNow++) {
      if(!dynamic_cast<TH1*>(hisV[nHisNow])) continue;
      nEntries += (int)hisV[nHisNow]->GetEntries();
    }
    hisV.clear();
    if(nEntries == 0) continue;

    hisMultiV.push_back(hisMultiVin[nHisVnow]);
  }
  nHisV = (int)hisMultiV.size(); if(nHisV == 0) return;
  
  // use the first graph to define the shared pad
  vector <TPad*> padV;
  draw->NewOptC("cnvsName" , (TString)hisMultiV[0][0]->GetName()+"_MGcnvs");
  draw->NewOptC("pad0Name" , (TString)hisMultiV[0][0]->GetName()+"_pad0");
  PrepCanvas(&padV,nHisV);

  bool    noLegend_orig   = draw->OptOrNullB("noLegend");
  TString axisTitleX_orig = draw->OptOrNullC("axisTitleX");
  TString axisTitleY_orig = draw->OptOrNullC("axisTitleY");

  if(!draw->HasOptB("noLegend"))   draw->NewOptB("noLegend",false);
  if(!draw->HasOptC("axisTitleX")) draw->NewOptC("axisTitleX","");
  if(!draw->HasOptC("axisTitleY")) draw->NewOptC("axisTitleY","");
  
  TString drawOptOrig = (TString)((draw->HasOptC("drawOpt")) ? draw->GetOptC("drawOpt") : "NULL");

  for(int nHisVnow=0; nHisVnow<nHisV; nHisVnow++) {
    TPad * padNow = (TPad*)padV[0]->GetPad(nHisVnow+1);
    
    if(draw->HasOptC("onlyThisPadLegend")) draw->SetOptB("noLegend" , (nHisVnow != draw->GetOptC("onlyThisPadLegend").Atoi()) );
    draw->SetOptC("axisTitleX" , hisMultiV[nHisVnow][0]->GetXaxis()->GetTitle());
    draw->SetOptC("axisTitleY" , hisMultiV[nHisVnow][0]->GetYaxis()->GetTitle());

    TString generalHeader_orig = draw->OptOrNullC("generalHeader");
    TString generalHeaderName  = TString::Format("generalHeader_%d",nHisVnow);
    if(draw->HasOptC(generalHeaderName)) draw->NewOptC("generalHeader",draw->GetOptC(generalHeaderName));

    TString drawOptPad = TString::Format("drawOpt_pad%d",nHisVnow);
    if(draw->HasOptC(drawOptPad)) {
      draw->NewOptC("drawOpt",draw->GetOptC(drawOptPad));
    }
    else if(drawOptOrig != "NULL") draw->SetOptC("drawOpt",drawOptOrig);

    drawHis1dV(hisMultiV[nHisVnow],padNow);

    if(draw->OptOrNullB("doIndividualPlots")) {
      vector <TPad*> padV;
      draw->NewOptC("cnvsName" , (TString)hisMultiV[nHisVnow][0]->GetName()+"_singlePad_cnvs");
      draw->NewOptC("pad0Name" , (TString)hisMultiV[nHisVnow][0]->GetName()+"_singlePad_pad0");
      PrepCanvas(&padV);

      bool noLegend = draw->GetOptB("noLegend");
      draw->SetOptB("noLegend",false);
      
      drawHis1dV(hisMultiV[nHisVnow], padV[1]);
      
      draw->SetOptB("noLegend",noLegend);
    }

    if(draw->HasOptC("generalHeader"))  draw->SetOptC("generalHeader",generalHeader_orig);
  }

  hisMultiV.clear();

  draw->NewOptB("noLegend",noLegend_orig);
  draw->NewOptC("axisTitleX",axisTitleX_orig);
  draw->NewOptC("axisTitleY",axisTitleY_orig);

  if(drawOptOrig != "NULL") draw->SetOptC("drawOpt",drawOptOrig);

  return;
}

// ===========================================================================================================
void OutMngr::drawMultiGraph(TMultiGraph * mGrph, TPad * pad) {
// ============================================================

  if(!dynamic_cast<TMultiGraph*>(mGrph)) return;

  TList * mGrphList(NULL); mGrphList = mGrph->GetListOfGraphs();
  int     nElements(0);    if(mGrphList) nElements = mGrphList->GetEntries();
  if(nElements == 0) return;

  int colOffset   = (draw->OptOrNullC("colOffset") == "") ? 0 : draw->OptOrNullC("colOffset").Atoi();

  map <TString,TString> drawOptV;
  for(int nGrapNow=0; nGrapNow<nElements; nGrapNow++){
    TGraph * grph = (TGraph*)mGrph->GetListOfGraphs()->At(nGrapNow); if(grph == NULL) continue; if(grph->GetN() < 1) continue;
    if(draw->OptOrNullC("onlyEverySuchBin") != "" && nGrapNow%(draw->OptOrNullC("onlyEverySuchBin").Atoi()) != 0 && nGrapNow > 0) continue;

    if(!dynamic_cast<TPad*>(pad)) {
      vector <TPad*> padV;
      draw->NewOptC("cnvsName" , (TString)grph->GetName()+"_MGcnvs");
      draw->NewOptC("pad0Name" , (TString)grph->GetName()+"_pad0");
      PrepCanvas(&padV);  pad = padV[1];
      padV.clear();       assert(dynamic_cast<TPad*>(pad));
    } 
    pad->cd();

    SetHisStyle(grph);
 
    grph->SetMarkerStyle(utils->markers[nGrapNow+colOffset]);
    grph->SetMarkerColor(utils->colours[nGrapNow+colOffset]); 
    grph->SetLineColor(utils->colours[nGrapNow+colOffset]);

    drawOptV[grph->GetName()] = draw->OptOrNullC("drawOpt");
    
    if(draw->OptOrNullC("fitFunName") != "") {
      TF1* fitFunc = (TF1*)grph->GetListOfFunctions()->FindObject(draw->OptOrNullC("fitFunName"));
      if(dynamic_cast<TF1*>(fitFunc)) fitFunc->SetLineColor(utils->colours[nGrapNow+colOffset]);
    }
  }

  TString drawOpt = (draw->OptOrNullC("drawOpt") != "") ? draw->OptOrNullC("drawOpt") : "ap" ;  if(!drawOpt.Contains("a")) drawOpt += (TString)"a";
  mGrph->Draw(drawOpt); // other options included when adding graphs to the mGraph

  for(int nGrapNow=0; nGrapNow<nElements; nGrapNow++){
    TGraph * grph = (TGraph*)mGrph->GetListOfGraphs()->At(nGrapNow); if(grph == NULL) continue;
    SetHisStyle(grph,mGrph);
    
    if(draw->OptOrNullB("hasLablesAxisX") || draw->OptOrNullB("hasLablesAxisY")) {
      for(int nAxisNow=0; nAxisNow<2; nAxisNow++) {
        TString titleXY("");
        if(nAxisNow == 0) { if(draw->OptOrNullB("hasLablesAxisX")) titleXY = "X"; else continue; }
        if(nAxisNow == 1) { if(draw->OptOrNullB("hasLablesAxisY")) titleXY = "Y"; else continue; }
        
        int       nPoints   = grph->GetN();
        Double_t  * pointsV = (nAxisNow == 0) ? grph->GetX()      : grph->GetY();
        TAxis     * axis    = (nAxisNow == 0) ? mGrph->GetXaxis() : mGrph->GetYaxis();
        for(int nPointNow=0; nPointNow<nPoints; nPointNow++) {
          int     nBin      = axis->FindBin(pointsV[nPointNow]);
          TString binTitle  = draw->OptOrNullC(TString::Format("label"+titleXY+"_%d",nPointNow));
          
          axis->SetBinLabel(nBin,binTitle);
        }
        if(draw->OptOrNullB((TString)"noTitleAxis"+titleXY))        axis->SetTitle("");
        if(draw->OptOrNullC((TString)"LabelsOption"+titleXY) != "") 
          mGrph->GetHistogram()->LabelsOption(draw->GetOptC((TString)"LabelsOption"+titleXY),titleXY);  // does not work with axis, must use the histogram
      }
    }
    
    break;
  }

  if(nElements > 1) buildLegend(drawOptV,pad);
  
  DrawText();
  DrawLines();

  drawOptV.clear();

  return;
}

// ===========================================================================================================
void OutMngr::drawMultiGraphV(vector <TMultiGraph*> mGrphVin) {
// ============================================================

  int nMgraphs = (int)mGrphVin.size();  if(nMgraphs == 0) return;

  vector <TMultiGraph*> mGrphV;
  vector <TGraph *> grphV;
  
  // trim the vector and only keep the TMultiGraphs which have elements. also store a pointer to the first graph
  for(int nMgrpahNow=0; nMgrpahNow<nMgraphs; nMgrpahNow++) {
    TList * mGrphList(NULL); mGrphList = mGrphVin[nMgrpahNow]->GetListOfGraphs();
    int     nElements(0);    if(mGrphList) nElements = mGrphList->GetEntries();
    
    if(nElements == 0) continue;
    mGrphV.push_back(mGrphVin[nMgrpahNow]);
    
    TGraph * grph = (TGraph*)mGrphVin[nMgrpahNow]->GetListOfGraphs()->At(0);
    assert(dynamic_cast<TGraph*>(grph));  grphV.push_back(grph);
  }
  nMgraphs = (int)mGrphV.size();  if(nMgraphs == 0) return;
  
  // use the first graph to define the shared pad
  vector <TPad*> padV;
  draw->NewOptC("cnvsName" , (TString)grphV[0]->GetName()+"_MGcnvs");
  draw->NewOptC("pad0Name" , (TString)grphV[0]->GetName()+"_pad0");
  PrepCanvas(&padV,nMgraphs);

  bool    noLegend_orig   = draw->OptOrNullB("noLegend");
  TString axisTitleX_orig = draw->OptOrNullC("axisTitleX");
  TString axisTitleY_orig = draw->OptOrNullC("axisTitleY");

  if(!draw->HasOptB("noLegend"))   draw->NewOptB("noLegend",false);
  if(!draw->HasOptC("axisTitleX")) draw->NewOptC("axisTitleX","");
  if(!draw->HasOptC("axisTitleY")) draw->NewOptC("axisTitleY","");

  for(int nMgrpahNow=0; nMgrpahNow<nMgraphs; nMgrpahNow++) {
    TPad * padNow = (TPad*)padV[0]->GetPad(nMgrpahNow+1);
    
    if(draw->HasOptC("onlyThisPadLegend")) draw->SetOptB("noLegend" , (nMgrpahNow != draw->GetOptC("onlyThisPadLegend").Atoi()) );
    draw->SetOptC("axisTitleX" , grphV[nMgrpahNow]->GetXaxis()->GetTitle());
    draw->SetOptC("axisTitleY" , grphV[nMgrpahNow]->GetYaxis()->GetTitle());

    TString generalHeader_orig = draw->OptOrNullC("generalHeader");
    TString generalHeaderName  = TString::Format("generalHeader_%d",nMgrpahNow);
    if(draw->HasOptC(generalHeaderName)) draw->NewOptC("generalHeader",draw->GetOptC(generalHeaderName));

    // in case of different axis lables of graphs in the multigraph vector, set the current lables
    bool hasDifferentLablesAxisX = draw->OptOrNullB(TString::Format("hasDifferentLablesAxisX_%d",nMgrpahNow)) || draw->OptOrNullB("hasDifferentLablesAxisX");
    bool hasDifferentLablesAxisY = draw->OptOrNullB(TString::Format("hasDifferentLablesAxisY_%d",nMgrpahNow)) || draw->OptOrNullB("hasDifferentLablesAxisY");
    
    if(hasDifferentLablesAxisX || hasDifferentLablesAxisY) {
      for(int nAxisNow=0; nAxisNow<2; nAxisNow++) {
        TString titleXY("");
        if(nAxisNow == 0) { if(hasDifferentLablesAxisX) titleXY = "X"; else continue; }
        if(nAxisNow == 1) { if(hasDifferentLablesAxisY) titleXY = "Y"; else continue; }

        draw->NewOptB((TString)"hasLablesAxis"+titleXY,true);
        
        TGraph    * grph    = (TGraph*)mGrphV[nMgrpahNow]->GetListOfGraphs()->At(0);
        int       nPoints   = grph->GetN();
        for(int nPointNow=0; nPointNow<nPoints; nPointNow++) {
          TString binTitle   = draw->OptOrNullC(TString::Format("labelV_%d_"+titleXY+"_%d",nMgrpahNow,nPointNow));
          TString newOptName = TString::Format("label"+titleXY+"_%d",nPointNow);
          
          draw->NewOptC(newOptName,binTitle);
        }
      }
    }
    else {
      draw->DelOptB((TString)"hasLablesAxis"+"X");
      draw->DelOptB((TString)"hasLablesAxis"+"Y");
    }

    drawMultiGraph(mGrphV[nMgrpahNow],padNow);

    if(draw->OptOrNullB("doIndividualPlots")) {
      vector <TPad*> padV;
      
      TList * mGrphList(NULL); mGrphList = mGrphV[nMgrpahNow]->GetListOfGraphs();
      int     nElements(0);    if(mGrphList) nElements = mGrphList->GetEntries();
      for(int nGrapNow=0; nGrapNow<nElements; nGrapNow++) {
        TGraph * grph = (TGraph*)mGrphV[nMgrpahNow]->GetListOfGraphs()->At(nGrapNow); if(grph == NULL) continue; if(grph->GetN() < 1) continue;
        if(draw->OptOrNullC("onlyEverySuchBin") != "" && nGrapNow%(draw->OptOrNullC("onlyEverySuchBin").Atoi()) != 0 && nGrapNow > 0) continue;

        draw->NewOptC("cnvsName" , (TString)grph->GetName()+"_singlePad_MGcnvs");
        draw->NewOptC("pad0Name" , (TString)grph->GetName()+"_singlePad_pad0");
        PrepCanvas(&padV);
      }
      
      bool noLegend = draw->GetOptB("noLegend");
      draw->SetOptB("noLegend",false);
      
      drawMultiGraph(mGrphV[nMgrpahNow], padV[1]);
      
      draw->SetOptB("noLegend",noLegend);
    }

    if(draw->HasOptC("generalHeader"))  draw->SetOptC("generalHeader",generalHeader_orig);
  }

  draw->NewOptB("noLegend",noLegend_orig);
  draw->NewOptC("axisTitleX",axisTitleX_orig);
  draw->NewOptC("axisTitleY",axisTitleY_orig);

  return;
}

// ===========================================================================================================
void OutMngr::drawMultiGraphV(TMultiGraph * mGrph) {
// =================================================
  vector <TMultiGraph*> mGrphV;
  mGrphV.push_back(mGrph);

  drawMultiGraphV(mGrphV);
  
  mGrphV.clear();
  return;
}

// ===========================================================================================================
void OutMngr::SetMyStyle() {
// =========================
  TStyle * sadehStyle = new  TStyle("sadehStyle", "sadehStyle");

  gStyle->SetHistTopMargin(0.5);  // default is 0.05

  //set the background color to white
  sadehStyle->SetFillColor(10);
  sadehStyle->SetFrameFillColor(10);
  sadehStyle->SetCanvasColor(10);
  sadehStyle->SetPadColor(10);
  sadehStyle->SetTitleFillColor(10);
  sadehStyle->SetStatColor(10);

  //dont put a colored frame around the plots
  sadehStyle->SetFrameBorderMode(0);
  sadehStyle->SetCanvasBorderMode(0);
  sadehStyle->SetPadBorderMode(0);
  sadehStyle->SetLegendBorderSize(0);

  //set the color palette
  sadehStyle->SetPalette(1,0);

  //set the default line color for a histogram to be black
  sadehStyle->SetHistLineColor(kBlack);

  //set the default line color/style for a fit function
  sadehStyle->SetFuncColor(kRed-7);
  sadehStyle->SetFuncStyle(1);

  //make the axis labels black
  sadehStyle->SetLabelColor(kBlack,"xyz");

  //set the default title color to be black
  sadehStyle->SetTitleColor(kBlack);
 
  //set the margins
  sadehStyle->SetPadBottomMargin(0.15);
  sadehStyle->SetPadTopMargin(0.15);
  sadehStyle->SetPadRightMargin(0.15);
  sadehStyle->SetPadLeftMargin(0.15);

  //set axis label and title text sizes
  sadehStyle->SetTitleOffset(1.4,"yz");
  sadehStyle->SetTitleOffset(1.2,"x");
  sadehStyle->SetTitleSize(0.05,"x");
  sadehStyle->SetTitleSize(0.05,"yz");

  //set line widths
  sadehStyle->SetFrameLineWidth(2);
  sadehStyle->SetFuncWidth(2);
  sadehStyle->SetHistLineWidth(2);

  //set the number of divisions to show
  sadehStyle->SetNdivisions(508, "x");
  sadehStyle->SetNdivisions(508, "y");
  sadehStyle->SetNdivisions(508, "z");

  //turn off xy grids
  sadehStyle->SetPadGridX(0);
  sadehStyle->SetPadGridY(0);

  //set the tick mark style
  sadehStyle->SetPadTickX(1);
  sadehStyle->SetPadTickY(1);

  //turn on/off stats
  sadehStyle->SetOptStat(0);
  sadehStyle->SetOptTitle(0);
  ////sadehStyle->SetOptFit(0111);
  sadehStyle->SetOptFit(0);
  sadehStyle->SetFitFormat("1.1e");

  //marker settings
  sadehStyle->SetMarkerStyle(20);
  sadehStyle->SetMarkerSize(1);
  sadehStyle->SetMarkerColor(kBlack);

  //surrounding box
  sadehStyle->SetLineWidth(1); 

  // unmber of axis digits
  TGaxis::SetMaxDigits(3);

  // fix pallet colours
  const Int_t NRGBs = 5;
  const Int_t NCont = 99;

  Double_t stops[NRGBs] = { 0.00, 0.34, 0.61, 0.84, 1.00 };
  Double_t red[NRGBs]   = { 0.00, 0.00, 0.87, 1.00, 0.51 };
  Double_t green[NRGBs] = { 0.00, 0.81, 1.00, 0.20, 0.00 };
  Double_t blue[NRGBs]  = { 0.51, 1.00, 0.12, 0.00, 0.00 };
  TColor::CreateGradientColorTable(NRGBs, stops, red, green, blue, NCont);
  gStyle->SetNumberContours(NCont);

  sadehStyle->cd(); gROOT->ForceStyle();

  glob->NewOptF("txtSize" , 0.045);  // if set to zero than auto text size applies according to the dimensions of the object

  // prevent creating canvases during plotting
  gROOT->SetBatch(true);  

  // set verbosity level
  gErrorIgnoreLevel = inLOG(Log::DEBUG_2) ? kInfo : kWarning; //kPrint, kInfo, kWarning, kError, kBreak, kSysError, kFatal;

  return;
}