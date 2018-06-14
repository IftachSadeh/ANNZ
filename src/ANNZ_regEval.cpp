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
/**
 * @brief             - Evaluate regression - direct loop over input tree.
 * 
 * @details           - If following optimization, then "postTrain" trees containing MLM results are used
 *                    [inChain != NULL]. Otherwise, first postTrain trees are created from the input dataset.
 *           
 * @param inChain     - A possible input chain to use, if MLM trees already exist. If [inChain==NULL], new trees are
 *                    created on the fly, directly evaluating MLMs using the proper TMVA::Reader objects.
 * @param outDirName  - A possible input path to an output directory in which the results are stored.
 * @param selctVarV   - If [selctVarV != NULL], the final list of selected variables in the output tree is stored,
 *                    in order to be used by another function, such as e.g., doMetricPlots().
 */
// ===========================================================================================================
void ANNZ::doEvalReg(TChain * inChain, TString outDirName, vector <TString> * selctVarV) {
// ===========================================================================================================
  aLOG(Log::INFO) <<coutPurple<<" - starting ANNZ::doEvalReg() ... "<<coutDef<<endl;

  aRegEval = new RegEval("aRegEval",utils,glob,outputs);
  aRegEval->inChain    = inChain;
  aRegEval->outDirName = outDirName;
  aRegEval->selctVarV  = selctVarV;

  evalRegSetup();
  
  evalRegLoop();

  DELNULL(aRegEval);
  
  return;
}

// ===========================================================================================================
/**
 * @brief  - setups the evaluation.
 */
// ===========================================================================================================
void ANNZ::evalRegSetup() {
// ===========================================================================================================
  aLOG(Log::DEBUG_1) <<coutWhiteOnBlack<<coutPurple<<" - starting ANNZ::evalRegSetup() ... "<<coutDef<<endl;

  TString MLMsToStore    = glob->GetOptC("MLMsToStore");
  TString outDirNameFull = glob->GetOptC("outDirNameFull");
  TString addOutputVars  = glob->GetOptC("addOutputVars");
  TString _typeANNZ      = glob->GetOptC("_typeANNZ");
  TString treeName       = glob->GetOptC("treeName");
  int     nMLMs          = glob->GetOptI("nMLMs");
  int     nPDFs          = glob->GetOptI("nPDFs");
  int     nPDFbins       = glob->GetOptI("nPDFbins");
  bool    isBinCls       = glob->GetOptB("doBinnedCls");
  bool    needBinClsErr  = glob->GetOptB("needBinClsErr");
  bool    doBiasCorPDF   = glob->GetOptB("doBiasCorPDF");
  bool    addMaxPDF      = glob->GetOptB("addMaxPDF");

  if(!aRegEval) {
    aRegEval = new RegEval("aRegEval",utils,glob,outputs);
  }

  if(aRegEval->outDirName != "") outDirNameFull = aRegEval->outDirName;
  TString hisName("");

  aRegEval->minWeight   = 0.001;
  aRegEval->hasErrKNN   = false;
  aRegEval->hasErrs     = false;
  aRegEval->hasMlmChain = (dynamic_cast<TChain*>(aRegEval->inChain));

  aRegEval->addMLMv      .clear();
  aRegEval->allMLMv      .clear();
  aRegEval->mlmSkip      .clear();
  aRegEval->mlmSkipDivded.clear();
  aRegEval->mlmSkipPdf   .clear();
  aRegEval->isErrKNNv    .resize(nMLMs,true);
  aRegEval->isErrINPv    .resize(nMLMs,false);
  aRegEval->pdfWeightV   .resize(nPDFs,vector<double>(nMLMs,0));
  aRegEval->mlmAvg_val   .resize(nPDFs,vector<double>(nMLMs,0));
  aRegEval->mlmAvg_err   .resize(nPDFs,vector<double>(nMLMs,0));
  aRegEval->mlmAvg_wgt   .resize(nPDFs,vector<double>(nMLMs,0));

  // -----------------------------------------------------------------------------------------------------------
  // extract the requested added variables check if any variables used
  // for cuts but not requested by the user need to be included in the tree
  // -----------------------------------------------------------------------------------------------------------
  aRegEval->addVarV = utils->splitStringByChar(addOutputVars,';');

  if(aRegEval->selctVarV) {
    for(int nVarsInNow=0; nVarsInNow<(int)aRegEval->selctVarV->size(); nVarsInNow++) {
      TString addVarName = aRegEval->selctVarV->at(nVarsInNow);

      if(find(aRegEval->addVarV.begin(),aRegEval->addVarV.end(),addVarName) == aRegEval->addVarV.end()) {
        aRegEval->addVarV.push_back(addVarName);
      }
    }
  }

  // -----------------------------------------------------------------------------------------------------------
  // 
  // -----------------------------------------------------------------------------------------------------------
  aRegEval->nPdfTypes = addMaxPDF ? 3 : 2;
  aRegEval->tagNameV.resize(aRegEval->nPdfTypes);
  aRegEval->tagNameV[0] = glob->GetOptC("baseTag_MLM_avg");
  aRegEval->tagNameV[1] = glob->GetOptC("baseTag_PDF_avg");
  if(aRegEval->nPdfTypes > 2) aRegEval->tagNameV[2] = glob->GetOptC("baseTag_PDF_max");

  // -----------------------------------------------------------------------------------------------------------
  // figure out which MLMs to generate an error for, using which method (KNN errors or propagation of user-defined parameter-errors)
  // -----------------------------------------------------------------------------------------------------------
  if(!isBinCls || needBinClsErr) {
    TString allErrsKNN(""), allErrsInp("");
    for(int nMLMnow=0; nMLMnow<nMLMs; nMLMnow++) {
      TString MLMname = getTagName(nMLMnow); if(mlmSkip[MLMname]) continue;

      // this check is safe, since (inNamesErr[nMLMnow].size() > 0) was confirmed in setNominalParams()
      VERIFY(LOCATION,(TString)"inNamesErr["+utils->intToStr(nMLMnow)+"] not initialized... "
                              +"something is horribly wrong ?!?",(inNamesErr[nMLMnow].size() > 0));
      
      if(aRegEval->isErrKNNv[nMLMnow] && (inNamesErr[nMLMnow][0] != "")) {
        aRegEval->isErrKNNv[nMLMnow] = false;
        aRegEval->isErrINPv[nMLMnow] = true;
      }
      if(aRegEval->isErrKNNv[nMLMnow])                                 aRegEval->hasErrKNN = true; // at least one is needed
      if(aRegEval->isErrKNNv[nMLMnow] || aRegEval->isErrINPv[nMLMnow]) aRegEval->hasErrs   = true; // at least one is needed

      if(aRegEval->isErrKNNv[nMLMnow]) allErrsKNN += coutBlue+getTagName(nMLMnow)+coutPurple+",";
      if(aRegEval->isErrINPv[nMLMnow]) allErrsInp += coutBlue+getTagName(nMLMnow)+coutPurple+",";
    }

    if(allErrsKNN != "") aLOG(Log::INFO)<<coutYellow<<" - Will gen. errors by KNN method for:   "<<allErrsKNN<<coutDef<<endl;
    if(allErrsInp != "") aLOG(Log::INFO)<<coutYellow<<" - Will gen. input-parameter errors for: "<<allErrsInp<<coutDef<<endl;
  }

  // -----------------------------------------------------------------------------------------------------------
  // define vectors and histograms for calculating the average MLM solution (hisPDF_e)
  // and the PDF solutions (aRegEval->hisPDF_w)
  // -----------------------------------------------------------------------------------------------------------
  aRegEval->hisPDF_w.resize(nPDFs);
  for(int nPDFnow=0; nPDFnow<nPDFs; nPDFnow++) {
    TString nPDFname  = TString::Format("_nPdf%d",nPDFnow);

    hisName = (TString)"Nz"+_typeANNZ+"_tmpHisPDF_fullPdf"+nPDFname;
    aRegEval->hisPDF_w[nPDFnow] = new TH1F(hisName,hisName,nPDFbins,&(zPDF_binE[0]));
  }

  // -----------------------------------------------------------------------------------------------------------
  // load histograms for bias-correction of PDFs and create local 1d projections
  // -----------------------------------------------------------------------------------------------------------
  aRegEval->hisBiasCorV.resize(nPDFs,vector<TH1*>(nPDFbins,NULL));
  if(doBiasCorPDF) {
    vector <TH2*> hisPdfBiasCorV(nPDFs,NULL);

    // load the histograms from the root file
    // ----------------------------------------------------------------------------------------------------------- 
    TString optimVerifName = (TString)(isBinCls ? "verifResults" : "optimResults");
    TString rootFileName   = getKeyWord("",optimVerifName,"rootSaveFileName");
    TString biasCorHisTag  = getKeyWord("",optimVerifName,"biasCorHisTag");

    aLOG(Log::INFO)<<coutYellow<<" - Reading bias-correction results from "<<coutGreen<<rootFileName<<coutYellow<<" ..."<<coutDef<<endl;

    TFile * rootSaveFile = new TFile(rootFileName,"READ");

    for(int nPDFnow=0; nPDFnow<nPDFs; nPDFnow++) {
      hisName = (TString)biasCorHisTag+utils->intToStr(nPDFnow);

      hisPdfBiasCorV[nPDFnow] = dynamic_cast<TH2*>(rootSaveFile->Get(hisName));
      VERIFY(LOCATION,(TString)"Could not find "+hisName+" in "+rootFileName+" Can try to run with [\"doBiasCorPDF\" = False]?!",
                      (dynamic_cast<TH2*>(hisPdfBiasCorV[nPDFnow])));

      hisPdfBiasCorV[nPDFnow] = (TH2*)hisPdfBiasCorV[nPDFnow]->Clone((TString)hisName+"_cln");
      hisPdfBiasCorV[nPDFnow]->SetDirectory(0);
    }

    rootSaveFile->Close();  DELNULL(rootSaveFile);

    // create the 1d projections - one histogram per zReg-bin
    // ----------------------------------------------------------------------------------------------------------- 
    for(int nPDFnow=0; nPDFnow<nPDFs; nPDFnow++) {
      for(int nBinXnow=1; nBinXnow<nPDFbins+1; nBinXnow++) {
        hisName = (TString)hisPdfBiasCorV[nPDFnow]->GetName()+"_"+utils->intToStr(nBinXnow);

        double intgr = hisPdfBiasCorV[nPDFnow]->Integral(nBinXnow,nBinXnow);  if(intgr < EPS) continue;

        TH1 * his1 = (TH1*)hisPdfBiasCorV[nPDFnow]->ProjectionY(hisName,nBinXnow,nBinXnow,"e");
        his1->Scale(1/intgr);

        for(int nBinYnow=1; nBinYnow<his1->GetNbinsX()+1; nBinYnow++) {
          double val = his1->GetBinContent(nBinYnow);
          if(val < aRegEval->minWeight) his1->SetBinContent(nBinYnow,0);
        }

        intgr = his1->Integral();
        if(intgr < EPS) {
          DELNULL(his1);
        }
        else {
          his1->Scale(1/intgr);
          aRegEval->hisBiasCorV[nPDFnow][nBinXnow-1] = his1;
        }
        // // may draw the 1d projections for debugging...
        // outputs->optClear(); outputs->draw->NewOptC("drawOpt","e1p");
        // outputs->drawHis1dV(aRegEval->hisBiasCorV[nPDFnow][nBinXnow-1]);
      }
      DELNULL(hisPdfBiasCorV[nPDFnow]);
    }
    hisPdfBiasCorV.clear();
  }

  // -----------------------------------------------------------------------------------------------------------
  // calculate the overlap factors between the classifications bins (one for eack MLM) and the pdf bins
  // -----------------------------------------------------------------------------------------------------------
  if(isBinCls) {
    setBinClsPdfBinWeights(aRegEval->pdfBinWgt,aRegEval->nClsBinsIn);
  }

  // -----------------------------------------------------------------------------------------------------------  
  // get the index of the best MLM and the pdf weights from file
  // ----------------------------------------------------------------------------------------------------------- 
  if(!isBinCls) {
    TString saveFileName = getKeyWord("","optimResults","configSaveFileName");
    aLOG(Log::INFO)<<coutYellow<<" - Getting optimization results from "<<coutGreen<<saveFileName<<coutYellow<<" ..."<<coutDef<<endl;

    OptMaps * optMap = new OptMaps("localOptMap");
    TString          saveName_best("");
    vector <TString> optNames, saveName_pdf(nPDFs);

    optNames.push_back(glob->versionTag()); optMap->NewOptC(glob->versionTag(), glob->GetOptC(glob->versionTag()));

    for(int nPDFnow=0; nPDFnow<nPDFs; nPDFnow++) {
      saveName_pdf[nPDFnow] = TString::Format("weightsPDF_%d",nPDFnow);

      optNames.push_back(saveName_pdf[nPDFnow]); optMap->NewOptC(saveName_pdf[nPDFnow],"");
    }
    saveName_best = "bestMLM"; optNames.push_back(saveName_best); optMap->NewOptI(saveName_best);

    utils->optToFromFile(&optNames,optMap,saveFileName,"READ","SILENT_KeepFile",inLOG(Log::DEBUG_2));

    for(int nPDFnow=0; nPDFnow<nPDFs; nPDFnow++) {
      TString saveNameNow   = TString::Format("weightsPDF_%d",nPDFnow);
      saveName_pdf[nPDFnow] = optMap->GetOptC(saveNameNow);

      VERIFY(LOCATION,(TString)"Did not find PDF weights for "+saveNameNow
                               +" - has this PDF been trained !?!?!", (saveName_pdf[nPDFnow] != ""));
    }

    aRegEval->bestANNZindex = optMap->GetOptI(saveName_best);

    optNames.clear(); DELNULL(optMap);

    // create a vector of the optimized MLMs from the list string, and check its consistancy
    // -----------------------------------------------------------------------------------------------------------
    vector < vector <TString> > optim_pdfV(nPDFs);
    for(int nPDFnow=0; nPDFnow<nPDFs; nPDFnow++) {
      optim_pdfV[nPDFnow] = utils->splitStringByChar(saveName_pdf[nPDFnow],';');

      bool goodLenWeightV = ((int)optim_pdfV[nPDFnow].size() == nMLMs);
      VERIFY(LOCATION,(TString)"Missmatch between stored MLM weights and value of nMLMs - "+saveName_pdf[nPDFnow],goodLenWeightV);
    }
    saveName_pdf.clear();

    // make sure that all the MLMs needed from the optimization are indeed accepted in the current run
    for(int nMLMnow=0; nMLMnow<nMLMs; nMLMnow++) {
      int hasWeight(0);
      for(int nPDFnow=0; nPDFnow<nPDFs; nPDFnow++) {
        aRegEval->pdfWeightV[nPDFnow][nMLMnow] = utils->strToDouble(optim_pdfV[nPDFnow][nMLMnow]);

        if(aRegEval->pdfWeightV[nPDFnow][nMLMnow] > EPS) hasWeight++;
      }  

      TString MLMname = getTagName(nMLMnow);
      aRegEval->mlmSkip[MLMname] = (hasWeight == 0 && nMLMnow != aRegEval->bestANNZindex);
      
      if(!aRegEval->mlmSkip[MLMname]) {
        VERIFY(LOCATION,(TString)"MLM ("+MLMname+") needed by PDF, but not found. Need to retrain ?!?",(!mlmSkip[MLMname]));
      }
    }

    // before adding user requested MLMs, keep track in aRegEval->mlmSkipPdf of those needed for the pdfs
    aRegEval->mlmSkipPdf = aRegEval->mlmSkip;

    // parse user request for specific MLMs to be added
    if(MLMsToStore != "") {
      vector <TString> allAcceptedMLMs(nMLMs);
      for(int nMLMnow=0; nMLMnow<nMLMs; nMLMnow++) allAcceptedMLMs[nMLMnow] = getTagName(nMLMnow);

      map <TString,bool> mlmSkipUser;
      selectUserMLMlist(allAcceptedMLMs,mlmSkipUser);

      TString addedMLMs("");
      for(int nMLMnow=0; nMLMnow<nMLMs; nMLMnow++) {
        TString MLMname = getTagName(nMLMnow); if(mlmSkipUser[MLMname]) continue;

        // all MLMs requested by the user
        aRegEval->addMLMv.push_back(nMLMnow);

        if(!aRegEval->mlmSkip[MLMname]) continue;

        // all MLMs requested by the user and not part of the original selection needed for the pdfs
        addedMLMs += coutGreen+MLMname+coutPurple+",";
        aRegEval->mlmSkip[MLMname] = false;
      }
      if(addedMLMs != "") {
        aLOG(Log::INFO)<<coutYellow<<" - Added user-requested MLMs which were not needed for the PDF: "<<addedMLMs<<coutDef<<endl;
      }

      allAcceptedMLMs.clear(); mlmSkipUser.clear();
    }

    optim_pdfV.clear();
  }

  // -----------------------------------------------------------------------------------------------------------  
  // 
  // -----------------------------------------------------------------------------------------------------------  
  TString allMLMs("");
  for(int nMLMnow=0; nMLMnow<nMLMs; nMLMnow++) {
    TString MLMname = getTagName(nMLMnow); if(aRegEval->mlmSkip[MLMname]) continue;

    allMLMs += coutPurple+MLMname+coutGreen+",";
    aRegEval->allMLMv.push_back(nMLMnow);
  }
  if(!isBinCls) {
    aLOG(Log::DEBUG) <<coutBlue<<" - The \"best\" MLM is: "<<coutYellow<<getTagName(aRegEval->bestANNZindex)<<coutDef<<endl;
  }
  aLOG(Log::DEBUG) <<coutBlue<<" - Will use the following MLMs: "<<allMLMs<<coutDef<<endl;

  return;
}

// ===========================================================================================================
/**
 * @brief    - perform regression evaluation loop.
 *
 * @details  - If following optimization, then "postTrain" trees containing MLM results are
 *           used [inChain != NULL]. Otherwise, first postTrain trees are created from the input
 *           dataset. The latter is done in nDivLoops steps, where in each step a subset of the MLMs
 *           is evaluated. The split into steps is done in order to avoid using too much memory, which
 *           may happen if multiple TMVA::Reader objects are initialized simultaneously.
 *           - Following the evaluation, both outputt trees and ascii output are created, which contain
 *           the "best" MLM (in case of regression), the registered PDFs, and any other individual MLM
 *           estimators which are requested by the user, using MLMsToStore. In addition, any variables from
 *           the original dataset which the user requests (using addOutputVars) are also included in
 *           the output catalog.
 */
// ===========================================================================================================
void ANNZ::evalRegLoop() {
// ===========================================================================================================
  aLOG(Log::DEBUG_1) <<coutWhiteOnBlack<<coutPurple<<" - starting ANNZ::evalRegLoop() ... "<<coutDef<<endl;

  TString outDirNameFull   = glob->GetOptC("outDirNameFull");
  TString addOutputVars    = glob->GetOptC("addOutputVars");
  TString userWgtPlots     = glob->GetOptC("userWeights_metricPlots");
  int     maxNobj          = glob->GetOptI("maxNobj");
  TString indexName        = glob->GetOptC("indexName");
  TString treeName         = glob->GetOptC("treeName");
  int     nMLMs            = glob->GetOptI("nMLMs");
  int     nDivLoops        = glob->GetOptI("nDivEvalLoops");
  TString zTrg             = glob->GetOptC("zTrg");
  int     nPDFs            = glob->GetOptI("nPDFs");
  int     nPDFbins         = glob->GetOptI("nPDFbins");
  bool    doStoreToAscii   = glob->GetOptB("doStoreToAscii");
  int     nSmearsRnd       = glob->GetOptI("nSmearsRnd");
  double  nSmearUnf        = glob->GetOptI("nSmearUnf"); // and cast to double, since we divide by this later
  TString _typeANNZ        = glob->GetOptC("_typeANNZ");
  TString baseTag_v        = glob->GetOptC("baseTag_v");
  TString baseTag_e        = glob->GetOptC("baseTag_e");
  TString baseTag_w        = glob->GetOptC("baseTag_w");
  bool    defErrBySigma68  = glob->GetOptB("defErrBySigma68");
  bool    isBinCls         = glob->GetOptB("doBinnedCls");
  bool    writePosNegErrs  = glob->GetOptB("writePosNegErrs");
  bool    doBiasCorPDF     = glob->GetOptB("doBiasCorPDF");
  bool    doStorePdfBins   = glob->GetOptB("doStorePdfBins");
  
  UInt_t  seed             = aRegEval->seed;
  TRandom * rnd            = aRegEval->rnd;
  TString regBestNameVal   = getTagBestMLMname(baseTag_v);
  TString regBestNameErr   = getTagBestMLMname(baseTag_e);
  TString regBestNameErrN  = getTagBestMLMname(baseTag_e+"N");
  TString regBestNameErrP  = getTagBestMLMname(baseTag_e+"P");
  TString regBestNameWgt   = getTagBestMLMname(baseTag_w);

  vector < vector<TString> > outTreeNameV(2,vector<TString>(nDivLoops,"")), outFileNameV(2,vector<TString>(nDivLoops,""));

  aRegEval->pdfWgtValV.resize(nPDFs,vector<double>(2,0));
  aRegEval->pdfWgtNumV.resize(nPDFs,vector<double>(2,0));
  aRegEval->regErrV   .resize(nMLMs,vector<double>(3,0));

  if(aRegEval->outDirName != "") outDirNameFull = aRegEval->outDirName;

  // -----------------------------------------------------------------------------------------------------------
  // create the chain for the loop, or assign the input chain
  // -----------------------------------------------------------------------------------------------------------
  if(aRegEval->hasMlmChain) {
    aRegEval->loopChain = aRegEval->inChain;
    
    aLOG(Log::DEBUG) <<coutRed<<" - using input chain "<<coutGreen<<aRegEval->loopChain->GetName()
                     <<"("<<aRegEval->loopChain->GetEntries()<<")"<<" from "
                     <<coutBlue<<aRegEval->loopChain->GetFile()->GetName()<<coutDef<<endl;
  }
  else {
    if(aRegEval->inTreeName == "") {
      aRegEval->inTreeName = (TString)treeName+glob->GetOptC("evalTreePostfix");
    }
    if(aRegEval->inFileName == "") {
      aRegEval->inFileName = (TString)outDirNameFull+aRegEval->inTreeName+"*.root";
    }

    aRegEval->loopChain = new TChain(aRegEval->inTreeName,aRegEval->inTreeName);
    aRegEval->loopChain->SetDirectory(0); aRegEval->loopChain->Add(aRegEval->inFileName);
    
    aLOG(Log::DEBUG) <<coutRed<<" - added chain "<<coutGreen<<aRegEval->inTreeName<<"("
                     <<aRegEval->loopChain->GetEntries()<<")"<<" from "<<coutBlue<<aRegEval->inFileName<<coutDef<<endl;
  }

  // -----------------------------------------------------------------------------------------------------------  
  // if using an input chain, make sure that all the required MLMs exist as branches
  // -----------------------------------------------------------------------------------------------------------  
  if(aRegEval->hasMlmChain) {
    vector <TString> inBranchNameV;
    utils->getTreeBranchNames(aRegEval->loopChain,inBranchNameV);

    for(int nMLMnow=0; nMLMnow<nMLMs; nMLMnow++) {
      TString MLMname   = getTagName(nMLMnow); if(aRegEval->mlmSkip[MLMname]) continue;

      bool    hasBranch = (find(inBranchNameV.begin(),inBranchNameV.end(), MLMname) != inBranchNameV.end());
      VERIFY(LOCATION,(TString)"MLM ("+MLMname+") needed by PDF, but not found. Something is horribly wrong ?!?",hasBranch);
    }
    inBranchNameV.clear();
  }

  // -----------------------------------------------------------------------------------------------------------  
  // loop twice:
  // -   (nLoopTypeNow == 0): create MLM trees from the input (skipped if aRegEval->hasMlmChain)
  //                          the trees are created in several steps (nDivLoops) in order to prevent
  //                          too many MLMs from being loaded into memory at once
  // -   (nLoopTypeNow == 1): use the MLM trees to calculate PDFs and store these and other MLMs requested
  //                          by the user to the final output tree (which is also written later to ascii)
  // -----------------------------------------------------------------------------------------------------------  
  for(int nLoopTypeNow=0; nLoopTypeNow<2; nLoopTypeNow++) {
    for(int nDivLoopNow=0; nDivLoopNow<nDivLoops; nDivLoopNow++) {
      if(nLoopTypeNow == 1 && nDivLoopNow > 0) break;

      // tree/file names
      if(nLoopTypeNow == 0) outTreeNameV[nLoopTypeNow][nDivLoopNow] = (TString)treeName+TString::Format("_MLM_nDiv%d",nDivLoopNow);
      else                  outTreeNameV[nLoopTypeNow][nDivLoopNow] = (TString)treeName+_typeANNZ;

      outFileNameV[nLoopTypeNow][nDivLoopNow] = (TString)outDirNameFull+outTreeNameV[nLoopTypeNow][nDivLoopNow]+"*.root";

      // if there is an input chain, then the first iteration of the loop is not needed
      if(nLoopTypeNow == 0 && aRegEval->hasMlmChain) continue;

      if(nLoopTypeNow == 0) {
        aLOG(Log::INFO)<<coutYellow<<" - creating MLM trees from input ..."                         <<coutDef<<endl;
      }
      else {
        aLOG(Log::INFO)<<coutYellow<<" - creating final MLM and PDFs trees from input MLM trees ..."<<coutDef<<endl;
      }
      
      // if there is no input chain, we need to update aRegEval->loopChain to point to the new trees we just created
      if(nLoopTypeNow == 1 &&  !aRegEval->hasMlmChain) {
        // delete the original chain
        DELNULL(aRegEval->loopChain);

        // create a new chain from the output of the previous iteration of the loop
        aRegEval->loopChain = new TChain(outTreeNameV[0][0],outTreeNameV[0][0]);
        aRegEval->loopChain->SetDirectory(0); aRegEval->loopChain->Add(outFileNameV[0][0]);
        
        aLOG(Log::DEBUG) <<coutRed<<" - added chain "<<coutGreen<<outTreeNameV[0][0]<<"("<<aRegEval->loopChain->GetEntries()<<")"
                         <<" from "<<coutBlue<<outFileNameV[0][0]<<coutDef<<endl;

        // in case of (nDivLoops > 1), add the other chains as friends
        for(int nDivLoopNow=1; nDivLoopNow<nDivLoops; nDivLoopNow++) {
          TChain  * aChainFriend = new TChain(outTreeNameV[0][nDivLoopNow],outTreeNameV[0][nDivLoopNow]);
          aChainFriend->SetDirectory(0); aChainFriend->Add(outFileNameV[0][nDivLoopNow]); 
        
          aLOG(Log::DEBUG) <<coutRed<<" - added chain friend "<<coutGreen<<outTreeNameV[0][nDivLoopNow]
                           <<" from "<<coutBlue<<outFileNameV[0][nDivLoopNow]<<coutDef<<endl;

          aRegEval->loopChain->AddFriend(aChainFriend,utils->nextTreeFriendName(aRegEval->loopChain));
        }
      }

      aRegEval->mlmSkipDivded = aRegEval->mlmSkip;

      if(nLoopTypeNow == 0) {
        // -----------------------------------------------------------------------------------------------------------  
        // create a local copy of the acceptance map of MLMs - select only a subsample of the MLMs for the
        // current run, based on the number of requested divisions (nDivLoops)
        // -----------------------------------------------------------------------------------------------------------  
        for(int nMLMnow=0; nMLMnow<nMLMs; nMLMnow++) {
          TString MLMname = getTagName(nMLMnow);
          aRegEval->mlmSkipDivded[MLMname] = true;
        }

        TString allMLMs   = "";
        int     nAcptMLMs = (int)aRegEval->allMLMv.size();
        int     nMLMsDiv  = static_cast<int>(ceil(nAcptMLMs/double(nDivLoops)));
        int     nAcptMin  = nMLMsDiv * nDivLoopNow;
        int     nAcptMax  = min(nMLMsDiv * (nDivLoopNow + 1) , nAcptMLMs);

        for(int nAcptMLMnow=nAcptMin; nAcptMLMnow<nAcptMax; nAcptMLMnow++) {
          int     nMLMnow = aRegEval->allMLMv[nAcptMLMnow];
          TString MLMname = getTagName(nMLMnow);
          
          aRegEval->mlmSkipDivded[MLMname] = false;
          allMLMs += coutGreen+MLMname+coutPurple+",";
        }

        // for(map <TString, bool>::iterator Itr=aRegEval->mlmSkip.begin(); Itr!=aRegEval->mlmSkip.end(); ++Itr) {
        //   cout <<"mlmSkip    "<<Itr->first<<CT<<Itr->second <<endl;
        // }
        // for(map <TString, bool>::iterator Itr=aRegEval->mlmSkipDivded.begin(); Itr!=aRegEval->mlmSkipDivded.end(); ++Itr) {
        //   cout <<"mlmSkipDivded "<<Itr->first<<CT<<Itr->second <<endl;
        // }

        aLOG(Log::INFO) <<coutBlue<<" - nDivLoopNow("<<coutPurple<<nDivLoopNow+1<<coutBlue<<"/"<<coutPurple<<nDivLoops
                        <<coutBlue<<") -> will use the following MLMs: "<<allMLMs<<coutDef<<endl;
      
        // -----------------------------------------------------------------------------------------------------------  
        // load the accepted readers
        // -----------------------------------------------------------------------------------------------------------  
        loadReaders(aRegEval->mlmSkipDivded);

        // -----------------------------------------------------------------------------------------------------------  
        // setup for the knn error estimation
        // -----------------------------------------------------------------------------------------------------------  
        evalRegErrSetup();
      }

      // -----------------------------------------------------------------------------------------------------------
      // create the vars to read/write trees
      // -----------------------------------------------------------------------------------------------------------
      VarMaps * var_0 = new VarMaps(glob,utils,"treeRegClsVar_0");
      VarMaps * var_1 = new VarMaps(glob,utils,"treeRegClsVar_1");

      // indexing varible
      var_1->NewVarI(indexName);

      // MLMs (requested by user or needed for pdf computation)
      for(int nMLMnow=0; nMLMnow<nMLMs; nMLMnow++) {
        TString MLMname    = getTagName(nMLMnow);      if(aRegEval->mlmSkipDivded[MLMname]) continue;
        TString MLMname_e  = getTagError(nMLMnow,"");  TString MLMname_w  = getTagWeight(nMLMnow);
        TString MLMname_eN = getTagError(nMLMnow,"N"); TString MLMname_eP = getTagError(nMLMnow,"P");

        // create MLM, MLM-eror and MLM-weight variables for the output vars
        var_1->NewVarF(MLMname); var_1->NewVarF(MLMname_w);
        if(aRegEval->hasErrs) { var_1->NewVarF(MLMname_e); var_1->NewVarF(MLMname_eN); var_1->NewVarF(MLMname_eP); }

        if(nLoopTypeNow == 0) {
          // create MLM-weight formulae for the input variables
          TString wgtStr = getRegularStrForm(userWgtsM[MLMname+"_valid"],var_0);
          var_0->NewForm(MLMname_w,wgtStr);

          // formulae for input-variable errors, to be used by getRegClsErrINP()
          if(aRegEval->isErrINPv[nMLMnow]) {
            int nInErrs = (int)inNamesErr[nMLMnow].size();
            for(int nInErrNow=0; nInErrNow<nInErrs; nInErrNow++) {
              TString inVarErr = getTagInVarErr(nMLMnow,nInErrNow);

              var_0->NewForm(inVarErr,inNamesErr[nMLMnow][nInErrNow]);
            }
          }
        }
      }

      if(nLoopTypeNow == 1) {
        // best MLM solution - add the correct tag
        if(!isBinCls) {
          var_1->NewVarF(regBestNameVal); var_1->NewVarF(regBestNameWgt);
          var_1->NewVarF(regBestNameErr); var_1->NewVarF(regBestNameErrN); var_1->NewVarF(regBestNameErrP);
        }

        // pdf branches (pdf bins and pdf average)
        for(int nPDFnow=0; nPDFnow<nPDFs; nPDFnow++) {
          // pdf value in each pdf-bin
          if(doStorePdfBins) {
            for(int nPdfBinNow=0; nPdfBinNow<nPDFbins; nPdfBinNow++) {
              TString pdfBinName = getTagPdfBinName(nPDFnow,nPdfBinNow);
              var_1->NewVarF(pdfBinName);
            }
          }

          // average unweighted and weighted pdf values and corresponding errors
          for(int nPdfTypeNow=0; nPdfTypeNow<aRegEval->nPdfTypes; nPdfTypeNow++) {
            if(isBinCls && nPdfTypeNow == 0) continue;

            TString pdfAvgName    = getTagPdfAvgName(nPDFnow,(TString)baseTag_v+aRegEval->tagNameV[nPdfTypeNow]);
            TString pdfAvgErrName = getTagPdfAvgName(nPDFnow,(TString)baseTag_e+aRegEval->tagNameV[nPdfTypeNow]);
            TString pdfAvgWgtName = getTagPdfAvgName(nPDFnow,(TString)baseTag_w+aRegEval->tagNameV[nPdfTypeNow]);

            var_1->NewVarF(pdfAvgName);
            if(nPdfTypeNow < 2) { var_1->NewVarF(pdfAvgErrName); var_1->NewVarF(pdfAvgWgtName); }
          }
        }
      }

      // connect the input vars to the tree before looping
      // -----------------------------------------------------------------------------------------------------------
      if(nLoopTypeNow == 0) var_0->connectTreeBranchesForm(aRegEval->loopChain,&readerInptV);
      else                  var_0->connectTreeBranches(aRegEval->loopChain);

      // make sure the target variable is included and check that all elements of aRegEval->addVarV exist in the input tree
      // -----------------------------------------------------------------------------------------------------------
      if(var_0->HasVar(zTrg) && find(aRegEval->addVarV.begin(),aRegEval->addVarV.end(),zTrg) == aRegEval->addVarV.end()) {
        aRegEval->addVarV.insert(aRegEval->addVarV.begin(),zTrg);
      }

      // check if any user-requested weight variables for plotting (used in doMetricPlots()) are needed, but not already included
      if(userWgtPlots != "" && userWgtPlots != "1") {
        vector <TString> inBranchNameV;
        utils->getTreeBranchNames(aRegEval->loopChain,inBranchNameV);

        for(int nVarNow=0; nVarNow<(int)inBranchNameV.size(); nVarNow++) {
          TString varNameNow = inBranchNameV[nVarNow];

          if( userWgtPlots.Contains(varNameNow) 
              && (find(aRegEval->addVarV.begin(),aRegEval->addVarV.end(),varNameNow) == aRegEval->addVarV.end()) )
          {
            aRegEval->addVarV.push_back(varNameNow);
          }
        }
        inBranchNameV.clear();
      }

      for(int nVarsInNow=0; nVarsInNow<(int)aRegEval->addVarV.size(); nVarsInNow++) {
        TString addVarName = aRegEval->addVarV[nVarsInNow];
        VERIFY(LOCATION,(TString)"from addOutputVars - trying to use undefined "
                                +"variable (\""+addVarName+"\") ...",var_0->HasVar(addVarName));
      }

      // possible additional variables added to the output (do once after connectTreeBranchesForm
      // of the input tree), create the output tree, and connects it to the vars
      // -----------------------------------------------------------------------------------------------------------
      var_1->varStruct(var_0,&aRegEval->addVarV);
      
      // best MLM solution - replaced the original variables and with the correct tag - no need to
      // store this solution, unless needed by the pdf
      if(!isBinCls && nLoopTypeNow == 1) {
        TString MLMname = getTagName(aRegEval->bestANNZindex);
        if(aRegEval->mlmSkipPdf[MLMname]) {
          TString MLMname_e  = getTagError(aRegEval->bestANNZindex,"");  TString MLMname_w  = getTagWeight(aRegEval->bestANNZindex);
          TString MLMname_eN = getTagError(aRegEval->bestANNZindex,"N"); TString MLMname_eP = getTagError(aRegEval->bestANNZindex,"P");

          var_1->DelVarF(MLMname); var_1->DelVarF(MLMname_w);
          if(aRegEval->hasErrs) { var_1->DelVarF(MLMname_e); var_1->DelVarF(MLMname_eN); var_1->DelVarF(MLMname_eP); } 
        }
      }

      TTree * treeOut = new TTree(outTreeNameV[nLoopTypeNow][nDivLoopNow],outTreeNameV[nLoopTypeNow][nDivLoopNow]);
      treeOut->SetDirectory(0); outputs->TreeMap[outTreeNameV[nLoopTypeNow][nDivLoopNow]] = treeOut;

      var_1->createTreeBranches(treeOut); 

      vector < pair<TString,TString> > varTypeNameV_com, varTypeNameV_all;
      // get the full list of variables common to both var_0 and var_1
      var_1->varStruct(var_0,NULL,NULL,&varTypeNameV_com,false);
      // get the full list of variables and variable-types in var_1
      // var_1->GetAllVarNameTypes(varTypeNameV_all);

      // -----------------------------------------------------------------------------------------------------------
      // loop on the tree
      // -----------------------------------------------------------------------------------------------------------
      bool breakLoop(false), mayWriteObjects(false);
      int  nObjectsToWrite(glob->GetOptI("nObjectsToWrite")), nObjectsToPrint(glob->GetOptI("nObjectsToPrint"));
      int  nHasNoErr(0), nHasZeroErr(0);
      TString aChainName(aRegEval->loopChain->GetName());
      var_0->clearCntr();
      for(Long64_t loopEntry=0; true; loopEntry++) {
        if(!var_0->getTreeEntry(loopEntry)) breakLoop = true;

        if((var_0->GetCntr("nObj") % nObjectsToPrint == 0 && var_0->GetCntr("nObj") > 0)) { var_0->printCntr(aChainName,Log::DEBUG); }
        if((mayWriteObjects && var_0->GetCntr("nObj") % nObjectsToWrite == 0) || breakLoop) {
          var_0->printCntr(aChainName); outputs->WriteOutObjects(false,true); outputs->ResetObjects();
          mayWriteObjects = false;
        }
        if(breakLoop) break;
        
        // set to default before anything else
        var_1->setDefaultVals(&varTypeNameV_all);
        // copy current content of all common variables (index + content of aRegEval->addVarV)
        var_1->copyVarData(var_0,&varTypeNameV_com);

        if(nLoopTypeNow == 1) {
          for(int nPDFnow=0; nPDFnow<nPDFs; nPDFnow++) {
            aRegEval->hisPDF_w  [nPDFnow]->Reset();
            aRegEval->mlmAvg_val[nPDFnow].resize(nMLMs,0);
            aRegEval->mlmAvg_err[nPDFnow].resize(nMLMs,0);
            aRegEval->mlmAvg_wgt[nPDFnow].resize(nMLMs,0);

            aRegEval->pdfWgtValV[nPDFnow][0] = aRegEval->pdfWgtValV[nPDFnow][1] = 0;
            aRegEval->pdfWgtNumV[nPDFnow][0] = aRegEval->pdfWgtNumV[nPDFnow][1] = 0;
          }
        }

        // -----------------------------------------------------------------------------------------------------------
        // calculate the KNN errors if needed, for each variation of aRegEval->knnErrModule
        // -----------------------------------------------------------------------------------------------------------
        if(aRegEval->hasErrKNN && nLoopTypeNow == 0) {
          for(map < TMVA::kNN::ModulekNN*,vector<int> >::iterator Itr=aRegEval->getErrKNN.begin(); Itr!=aRegEval->getErrKNN.end(); ++Itr) {
            getRegClsErrKNN(var_0,Itr->first,aRegEval->trgIndexV,Itr->second,!isBinCls,aRegEval->regErrV);
          }
        }

        // -----------------------------------------------------------------------------------------------------------
        // binned classification
        // -----------------------------------------------------------------------------------------------------------
        if(isBinCls) {
          // -----------------------------------------------------------------------------------------------------------
          // just generate MLM trees
          // -----------------------------------------------------------------------------------------------------------
          if(nLoopTypeNow == 0) {
            for(int nMLMnow=0; nMLMnow<nMLMs; nMLMnow++) {
              TString MLMname   = getTagName(nMLMnow);  if(aRegEval->mlmSkipDivded[MLMname]) continue;
              TString MLMname_e = getTagError(nMLMnow); TString MLMname_w = getTagWeight(nMLMnow);

              double clsPrb     = getReader(var_0,ANNZ_readType::PRB,true,nMLMnow);
              double clsWgt     = var_0->GetForm(MLMname_w);

              // sanity check that weights are properly defined
              if(clsWgt < 0) { var_0->printVars(); VERIFY(LOCATION,(TString)"Weights can only be >= 0 ... Something is horribly wrong ?!?",false); }

              var_1->SetVarF(MLMname,clsPrb); var_1->SetVarF(MLMname_w,clsWgt);

              if(aRegEval->hasErrs) {
                double  clsErr  = -1; 
                if     (aRegEval->isErrKNNv[nMLMnow]) clsErr = aRegEval->regErrV[nMLMnow][1];
                else if(aRegEval->isErrINPv[nMLMnow]) clsErr = getRegClsErrINP(var_0,false,nMLMnow,&seed);
                
                var_1->SetVarF(MLMname_e,clsErr);
              }
            }
          }
          // -----------------------------------------------------------------------------------------------------------
          // full solution, pdf etc.
          // -----------------------------------------------------------------------------------------------------------
          else {
            // go over all pdfs
            for(int nPDFnow=0; nPDFnow<nPDFs; nPDFnow++) {
              // go over all pdf bins
              for(int nPdfBinNow=0; nPdfBinNow<nPDFbins; nPdfBinNow++) {
                // in each pdf-bin, use the overlapping cls-bins
                for(int nClsBinNow=0; nClsBinNow<aRegEval->nClsBinsIn[nPdfBinNow]; nClsBinNow++) {
                  int    clsIndex   = aRegEval->pdfBinWgt[nPdfBinNow][nClsBinNow].first;
                  double binWgt     = aRegEval->pdfBinWgt[nPdfBinNow][nClsBinNow].second;

                  TString MLMname   = getTagName(clsIndex);
                  TString MLMname_e = getTagError(clsIndex); TString MLMname_w = getTagWeight(clsIndex);

                  double  binVal    = max(min(var_0->GetVarF(MLMname),1.),0.);
                  double  clsWgt    = var_0->GetVarF(MLMname_w);
                  double  totWgt    = binVal * binWgt * clsWgt;

                  aRegEval->hisPDF_w[nPDFnow]->Fill(zPDF_binC[nPdfBinNow],totWgt);
                 
                  aRegEval->pdfWgtValV[nPDFnow][1] += totWgt;
                  aRegEval->pdfWgtNumV[nPDFnow][1] += binVal * binWgt;

                  // generate random smearing factors for one of the PDFs
                  // -----------------------------------------------------------------------------------------------------------
                  if(nPDFnow == 1) {
                    double clsErr = var_0->GetVarF(MLMname_e);

                    if(clsErr > EPS) {
                      for(int nSmearRndNow=0; nSmearRndNow<nSmearsRnd; nSmearRndNow++) {
                        double sfNow     = fabs(rnd->Gaus(0,clsErr));        if(nSmearRndNow%2 == 0) sfNow *= -1;
                        double binSmr    = max(min((binVal + sfNow),1.),0.);
                        double totWgtSmr = binSmr * binWgt * clsWgt;

                        aRegEval->hisPDF_w[nPDFnow]->Fill(zPDF_binC[nPdfBinNow],totWgtSmr);
                        
                        aRegEval->pdfWgtValV[nPDFnow][1] += totWgtSmr;
                        aRegEval->pdfWgtNumV[nPDFnow][1] += binSmr * binWgt;
                      }
                    }
                  }
                }
              }
            }
          }
        }
        // -----------------------------------------------------------------------------------------------------------
        // randomized regression
        // -----------------------------------------------------------------------------------------------------------
        else {
          for(int nMLMnow=0; nMLMnow<nMLMs; nMLMnow++) {
            TString MLMname    = getTagName(nMLMnow);      if(aRegEval->mlmSkipDivded[MLMname]) continue;
            TString MLMname_e  = getTagError(nMLMnow,"");  TString MLMname_w  = getTagWeight(nMLMnow);
            TString MLMname_eN = getTagError(nMLMnow,"N"); TString MLMname_eP = getTagError(nMLMnow,"P");

            double regVal(0), regErr(0), regErrN(0), regErrP(0), regWgt(0);
            if(nLoopTypeNow == 0) {
              regVal = getReader(var_0,ANNZ_readType::REG,true,nMLMnow);
              regWgt = var_0->GetForm(MLMname_w);

              // sanity check that weights are properly defined
              if(regWgt < 0) {
                var_0->printVars();
                VERIFY(LOCATION,(TString)"Weights can only be >= 0 ... Something is horribly wrong ?!?",false);
              }

              if(aRegEval->isErrINPv[nMLMnow]) getRegClsErrINP(var_0,true,nMLMnow,&seed,&(aRegEval->regErrV[nMLMnow]));

              regErrN = aRegEval->regErrV[nMLMnow][0];
              regErr  = aRegEval->regErrV[nMLMnow][1];
              regErrP = aRegEval->regErrV[nMLMnow][2];
            }
            else {
              regVal  = var_0->GetVarF(MLMname);    regWgt = var_0->GetVarF(MLMname_w);
              regErrN = var_0->GetVarF(MLMname_eN); regErr = var_0->GetVarF(MLMname_e); regErrP = var_0->GetVarF(MLMname_eP);
            }

            bool hasNoErrNow = ((regErrN < 0) || (regErr < 0) || (regErrP < 0));

            // in the (hopefully unlikely) event that the error calculation failed for a valid object
            if(hasNoErrNow && (regWgt > EPS)) {
              nHasNoErr++;
              if(inLOG(Log::DEBUG_2)) {
                aLOG(Log::DEBUG_2)<<coutYellow<<" - Got an undefined error calculation for:"<<coutDef<<endl;
                var_0->printVars();
              }
            }

            // objects with undefined errors can not be used...
            if(hasNoErrNow) regWgt = 0;

            // the "best" MLM solution
            if(nLoopTypeNow == 1 && nMLMnow == aRegEval->bestANNZindex) {
              var_1->SetVarF(regBestNameVal, regVal);  var_1->SetVarF(regBestNameWgt,regWgt);
              var_1->SetVarF(regBestNameErrN,regErrN); var_1->SetVarF(regBestNameErr,regErr); var_1->SetVarF(regBestNameErrP,regErrP); 
            }
            else {
              var_1->SetVarF(MLMname,regVal);     var_1->SetVarF(MLMname_w,regWgt);
              var_1->SetVarF(MLMname_eN,regErrN); var_1->SetVarF(MLMname_e,regErr); var_1->SetVarF(MLMname_eP,regErrP); 
            }

            if(nLoopTypeNow == 0) continue;      // in the 0-iteration, we only compute the MLM quantites and store a tree
            if(regWgt < EPS)      continue;      // if the weight is zero, no sense in continuing the loop
            if(regErr < EPS)      nHasZeroErr++; // to prompt a warning message later on

            if(aRegEval->mlmSkipPdf[MLMname]) continue; // some MLMs may be requested by the user, but not needed for the pdfs

            for(int nPDFnow=0; nPDFnow<nPDFs; nPDFnow++) {
              double pdfWgt = aRegEval->pdfWeightV[nPDFnow][nMLMnow] * regWgt;  if(pdfWgt < EPS) continue;

              aRegEval->pdfWgtValV[nPDFnow][0] += regWgt;
              aRegEval->pdfWgtNumV[nPDFnow][0] += 1;
              aRegEval->pdfWgtValV[nPDFnow][1] += pdfWgt;
              aRegEval->pdfWgtNumV[nPDFnow][1] += aRegEval->pdfWeightV[nPDFnow][nMLMnow];

              // input original value into the pdf before smearing
              aRegEval->hisPDF_w[nPDFnow]->Fill(regVal,pdfWgt);

              aRegEval->mlmAvg_val[nPDFnow][nMLMnow] = regVal;
              aRegEval->mlmAvg_err[nPDFnow][nMLMnow] = regErr;
              aRegEval->mlmAvg_wgt[nPDFnow][nMLMnow] = regWgt;

              // generate random smearing factors for this MLM
              for(int nSmearRndNow=0; nSmearRndNow<nSmearsRnd; nSmearRndNow++) {
                int     signNow(-1);
                double  errNow(regErrN);
                if(nSmearRndNow%2 == 0) { errNow = regErrP; signNow = 1; }

                double sfNow  = signNow * fabs(rnd->Gaus(0,errNow));
                double regSmr = regVal + sfNow;
                aRegEval->hisPDF_w[nPDFnow]->Fill(regSmr,pdfWgt);
              }
            }
          }
        }

        // -----------------------------------------------------------------------------------------------------------
        // fill the pdf tree branches
        // -----------------------------------------------------------------------------------------------------------
        if(nLoopTypeNow == 1) {
          for(int nPDFnow=0; nPDFnow<nPDFs; nPDFnow++) {
            double intgrPDF_w = aRegEval->hisPDF_w[nPDFnow]->Integral();

            if(intgrPDF_w > EPS) {
              // rescale the weighted probability distribution
              aRegEval->hisPDF_w[nPDFnow]->Scale(1/intgrPDF_w);

              // apply the bias-correction to the pdf
              // -----------------------------------------------------------------------------------------------------------
              if(doBiasCorPDF) {
                TString clnName        = (TString)aRegEval->hisPDF_w[nPDFnow]->GetName()+"_TMP";
                TH1     * hisPDF_w_TMP = (TH1*)aRegEval->hisPDF_w[nPDFnow]->Clone(clnName);

                for(int nBinXnow=1; nBinXnow<nPDFbins+1; nBinXnow++) {
                  double val = hisPDF_w_TMP->GetBinContent(nBinXnow);

                  if(val < aRegEval->minWeight)                                     continue;
                  if(!aRegEval->hisBiasCorV[nPDFnow][nBinXnow-1])                   continue;
                  if( aRegEval->hisBiasCorV[nPDFnow][nBinXnow-1]->Integral() < EPS) continue;

                  val /= nSmearUnf;
                  for(int nSmearUnfNow=0; nSmearUnfNow<nSmearUnf; nSmearUnfNow++) {
                    double rndVal = aRegEval->hisBiasCorV[nPDFnow][nBinXnow-1]->GetRandom();
                    aRegEval->hisPDF_w[nPDFnow]->Fill(rndVal,val);
                  }
                }

                intgrPDF_w = aRegEval->hisPDF_w[nPDFnow]->Integral();
                if(intgrPDF_w > EPS) aRegEval->hisPDF_w[nPDFnow]->Scale(1/intgrPDF_w);
                
                DELNULL(hisPDF_w_TMP);
              }
            }

            // if the objects was skipped (zero weight), the average value will have the
            // default (std::numeric_limits<float>::max()), but to avoid very big meaningless output, set the pdf-bins to zero
            // -----------------------------------------------------------------------------------------------------------
            if(intgrPDF_w < EPS) {
              if(doStorePdfBins) {
                for(int nPdfBinNow=0; nPdfBinNow<nPDFbins; nPdfBinNow++) {
                  TString pdfBinName = getTagPdfBinName(nPDFnow,nPdfBinNow);

                  var_1->SetVarF(pdfBinName,0);
                }
              }
              for(int nPdfTypeNow=0; nPdfTypeNow<aRegEval->nPdfTypes; nPdfTypeNow++) {
                if((isBinCls && nPdfTypeNow == 0) || nPdfTypeNow == 2) continue;

                TString pdfAvgErrName = getTagPdfAvgName(nPDFnow,(TString)baseTag_e+aRegEval->tagNameV[nPdfTypeNow]);
                TString pdfAvgWgtName = getTagPdfAvgName(nPDFnow,(TString)baseTag_w+aRegEval->tagNameV[nPdfTypeNow]);

                var_1->SetVarF(pdfAvgErrName,-1); var_1->SetVarF(pdfAvgWgtName,0);
              }
              continue;
            }

            // the value of the pdf in the different bins
            // -----------------------------------------------------------------------------------------------------------
            if(doStorePdfBins) {
              for(int nPdfBinNow=0; nPdfBinNow<nPDFbins; nPdfBinNow++) {
                TString pdfBinName = getTagPdfBinName(nPDFnow,nPdfBinNow);
                double  pdfValNow  = aRegEval->hisPDF_w[nPDFnow]->GetBinContent(nPdfBinNow+1);

                var_1->SetVarF(pdfBinName,pdfValNow);
              }
            }

            // the average value and the width of the pdf distribution
            for(int nPdfTypeNow=0; nPdfTypeNow<aRegEval->nPdfTypes; nPdfTypeNow++) {
              if(isBinCls && nPdfTypeNow == 0) continue;

              TString pdfAvgName    = getTagPdfAvgName(nPDFnow,(TString)baseTag_v+aRegEval->tagNameV[nPdfTypeNow]);
              TString pdfAvgErrName = getTagPdfAvgName(nPDFnow,(TString)baseTag_e+aRegEval->tagNameV[nPdfTypeNow]);
              TString pdfAvgWgtName = getTagPdfAvgName(nPDFnow,(TString)baseTag_w+aRegEval->tagNameV[nPdfTypeNow]);

              if(nPdfTypeNow == 0) {
                double avg_val(0), avg_err(0), sum_wgt(0);
                for(int nMLMnow=0; nMLMnow<nMLMs; nMLMnow++) {
                  double regWgt = aRegEval->mlmAvg_wgt[nPDFnow][nMLMnow];  if(regWgt < EPS) continue;
                  double regVal = aRegEval->mlmAvg_val[nPDFnow][nMLMnow];
                  double regErr = aRegEval->mlmAvg_err[nPDFnow][nMLMnow];

                  sum_wgt += regWgt; avg_val += regWgt*regVal; avg_err += regWgt*regErr;
                }
                if(sum_wgt > EPS) {
                  var_1->SetVarF(pdfAvgName,   avg_val/sum_wgt);
                  var_1->SetVarF(pdfAvgErrName,avg_err/sum_wgt);
                }
              }
              else if(nPdfTypeNow == 1) {
                utils->param->clearAll();
                utils->param->NewOptF("meanWithoutOutliers",5);
                if(utils->getInterQuantileStats(aRegEval->hisPDF_w[nPDFnow])) {
                  double  regAvgPdfVal  = utils->param->GetOptF("quant_mean_Nsig68");
                  double  regAvgPdfErr  = defErrBySigma68 ? utils->param->GetOptF("quant_sigma_68") : utils->param->GetOptF("quant_sigma");

                  var_1->SetVarF(pdfAvgName,   regAvgPdfVal);
                  var_1->SetVarF(pdfAvgErrName,regAvgPdfErr);
                  // if(var_0->GetCntr("nObj") < 14) cout << "xx "<<nPDFnow<<CT<<pdfAvgName<<CT<<regAvgPdfVal<<endl;
                }
              }
              else if(nPdfTypeNow == 2) {
                int maxBin = aRegEval->hisPDF_w[nPDFnow]->GetMaximumBin() - 1; // histogram bins start at 1, not at 0

                var_1->SetVarF(pdfAvgName,zPDF_binC[maxBin]);
              }

              if(nPdfTypeNow < 2) {
                VERIFY(LOCATION,(TString)"If intgrPDF_w>0 then there is no way that pdfWgtNumV==0 ... "
                                        +"something is horribly wrong ?!?!",(aRegEval->pdfWgtNumV[nPDFnow][nPdfTypeNow] > 0));

                aRegEval->pdfWgtValV[nPDFnow][nPdfTypeNow] /= aRegEval->pdfWgtNumV[nPDFnow][nPdfTypeNow];
                
                var_1->SetVarF(pdfAvgWgtName,aRegEval->pdfWgtValV[nPDFnow][nPdfTypeNow]);
              }
            }
          }
        }

        var_1->fillTree();

        mayWriteObjects = true; var_0->IncCntr("nObj"); if(var_0->GetCntr("nObj") == maxNobj) breakLoop = true;
      }
      if(!breakLoop) { var_0->printCntr(aChainName); outputs->WriteOutObjects(false,true); outputs->ResetObjects(); }
    
      if(nHasZeroErr > 0 || nHasNoErr > 0) {
        aLOG(Log::WARNING) <<coutWhiteOnRed<<" - Found "<<nHasZeroErr<<" error estimates equal to 0 and "<<nHasNoErr
                           <<" undefined errors... Please check that this makes sense !"<<coutDef<<endl;
      }

      //cleanup
      if(nLoopTypeNow == 0) {
        evalRegErrCleanup();
        clearReaders();
      }

      DELNULL(var_0); DELNULL(var_1); varTypeNameV_com.clear(); varTypeNameV_all.clear();
      DELNULL(treeOut); outputs->TreeMap.erase(outTreeNameV[nLoopTypeNow][nDivLoopNow]);
    } // ENDOF for(int nDivLoopNow=0; nDivLoopNow<nDivLoops; nDivLoopNow++) {}
  } // ENDOF for(int nLoopTypeNow=0; nLoopTypeNow<2; nLoopTypeNow++) {}
  // -----------------------------------------------------------------------------------------------------------  



  // -----------------------------------------------------------------------------------------------------------
  // write the combined output to an ascii file
  // -----------------------------------------------------------------------------------------------------------
  if(doStoreToAscii) {
    TChain  * aChainReg = new TChain(outTreeNameV[1][0],outTreeNameV[1][0]);
    aChainReg->SetDirectory(0); aChainReg->Add(outFileNameV[1][0]); 
    
    int nEntriesChain   = aChainReg->GetEntries();
    aLOG(Log::DEBUG) <<coutRed<<" - added chain "<<coutGreen<<outTreeNameV[1][0]<<"("<<nEntriesChain
                     <<")"<<" from "<<coutBlue<<outFileNameV[1][0]<<coutDef<<endl;

    VarMaps * var_2 = new VarMaps(glob,utils,"treeRegClsVar_2");

    // include MLMs requested by the user
    if(!isBinCls) {
      for(int nAddMLMnow=0; nAddMLMnow<(int)aRegEval->addMLMv.size(); nAddMLMnow++) {
        int     nMLMnow    = aRegEval->addMLMv[nAddMLMnow];      TString MLMname    = getTagName(nMLMnow);
        TString MLMname_e  = getTagError(nMLMnow);     TString MLMname_w  = getTagWeight(nMLMnow);
        TString MLMname_eN = getTagError(nMLMnow,"N"); TString MLMname_eP = getTagError(nMLMnow,"P");

        // create MLM, MLM-eror and MLM-weight variables for the output vars
        aRegEval->addVarV.push_back(MLMname); aRegEval->addVarV.push_back(MLMname_w);
        if(aRegEval->hasErrs) {
          aRegEval->addVarV.push_back(MLMname_e);
          if(writePosNegErrs) { aRegEval->addVarV.push_back(MLMname_eN); aRegEval->addVarV.push_back(MLMname_eP); }
        }
      }

      // the best MLM solution
      aRegEval->addVarV.push_back(regBestNameVal); aRegEval->addVarV.push_back(regBestNameWgt);
      if(aRegEval->hasErrs) {
        aRegEval->addVarV.push_back(regBestNameErr); if(writePosNegErrs) { aRegEval->addVarV.push_back(regBestNameErrN); aRegEval->addVarV.push_back(regBestNameErrP); }
      }
    }

    for(int nPDFnow=0; nPDFnow<nPDFs; nPDFnow++) {
      // average unweighted and weighted pdf values and corresponding errors
      for(int nPdfTypeNow=0; nPdfTypeNow<aRegEval->nPdfTypes; nPdfTypeNow++) {
        if(isBinCls && nPdfTypeNow == 0) continue;

        TString pdfAvgName    = getTagPdfAvgName(nPDFnow,(TString)baseTag_v+aRegEval->tagNameV[nPdfTypeNow]);
        TString pdfAvgErrName = getTagPdfAvgName(nPDFnow,(TString)baseTag_e+aRegEval->tagNameV[nPdfTypeNow]);
        TString pdfAvgWgtName = getTagPdfAvgName(nPDFnow,(TString)baseTag_w+aRegEval->tagNameV[nPdfTypeNow]);

        aRegEval->addVarV.push_back(pdfAvgName);
        if(nPdfTypeNow < 2) { aRegEval->addVarV.push_back(pdfAvgErrName); aRegEval->addVarV.push_back(pdfAvgWgtName); }
      }

      // pdf value in each pdf-bin
      if(doStorePdfBins) {
        for(int nPdfBinNow=0; nPdfBinNow<nPDFbins; nPdfBinNow++) {
          TString pdfBinName = getTagPdfBinName(nPDFnow,nPdfBinNow);
          aRegEval->addVarV.push_back(pdfBinName);
        }
      }
    }
    var_2->connectTreeBranches(aChainReg);

    var_2->storeTreeToAscii("ANNZ"+_typeANNZ,outDirNameFull,0,glob->GetOptI("nObjectsToWrite"),"",&aRegEval->addVarV,NULL);

    DELNULL(var_2); DELNULL(aChainReg);
  }

  // -----------------------------------------------------------------------------------------------------------
  // if needed, store the list of selected MLMs for use outside this function
  // -----------------------------------------------------------------------------------------------------------
  if(aRegEval->selctVarV) {
    aRegEval->selctVarV->clear();
    for(int nVarsInNow=0; nVarsInNow<(int)aRegEval->addVarV.size(); nVarsInNow++) {
      aRegEval->selctVarV->push_back(aRegEval->addVarV[nVarsInNow]);
    }
  }

  // -----------------------------------------------------------------------------------------------------------
  // cleanup
  // -----------------------------------------------------------------------------------------------------------
  if(!glob->GetOptB("keepEvalTrees") && !aRegEval->hasMlmChain) {
    for(int nDivLoopNow=0; nDivLoopNow<nDivLoops; nDivLoopNow++) {
      utils->safeRM(outFileNameV[0][nDivLoopNow],inLOG(Log::DEBUG));
    }
  }
  
  outTreeNameV.clear(); outFileNameV.clear(); 

  return;
}

// ===========================================================================================================
/**
 * @brief    - setup for the knn error estimation.
 *
 * @details  - inputComboNow: check for each MLM if the combination of variables,
 *           weights and cuts has been used before (avoid creating multiple identicle kd-trees)
 */
// ===========================================================================================================
void ANNZ::evalRegErrSetup() {
// ===========================================================================================================
  VERIFY(LOCATION,(TString)"Calling evalRegErrSetup() with no aRegEval defined ..."
                          +" something is horribly wrong ?!?",(dynamic_cast<RegEval*>(aRegEval)));

  if(!aRegEval->hasErrKNN) return;
  
  aLOG(Log::DEBUG_1) <<coutGreen<<" - starting ANNZ::evalRegErrSetup() ... "<<coutDef<<endl;
  
  int     nMLMs            = glob->GetOptI("nMLMs");
  TString postTrainDirName = glob->GetOptC("postTrainDirNameFull");

  evalRegErrCleanup();

  map <TString,bool> mlmSkipNow = aRegEval->mlmSkip;
  if(aRegEval->mlmSkipDivded.size() > 0) mlmSkipNow = aRegEval->mlmSkipDivded;

  aRegEval->aChainKnn    .resize(2,NULL);
  aRegEval->knnErrFactory.resize(nMLMs,NULL);
  aRegEval->knnErrOutFile.resize(nMLMs,NULL);
  aRegEval->knnErrModule .resize(nMLMs,NULL); 
  aRegEval->knnErrDataLdr.resize(nMLMs,NULL);

  TString inTreeNameKnn = getKeyWord("","treeErrKNN","treeErrKNNname");
  TString inFileNameKnn = postTrainDirName+inTreeNameKnn+"*.root";

  aRegEval->aChainKnn[0] = new TChain(inTreeNameKnn,inTreeNameKnn);
  aRegEval->aChainKnn[0]->SetDirectory(0); aRegEval->aChainKnn[0]->Add(inFileNameKnn);

  TString inTreeKnnFrnd = (TString)glob->GetOptC("treeName")+"_train";
  TString inFileKnnFrnd = (TString)glob->GetOptC("inputTreeDirName")+inTreeKnnFrnd+"*.root";
  aRegEval->aChainKnn[1] = new TChain(inTreeKnnFrnd,inTreeKnnFrnd);
  aRegEval->aChainKnn[1]->SetDirectory(0); aRegEval->aChainKnn[1]->Add(inFileKnnFrnd);

  aRegEval->aChainKnn[0]->AddFriend(aRegEval->aChainKnn[1],utils->nextTreeFriendName(aRegEval->aChainKnn[0]));

  int nEntriesChainKnn = aRegEval->aChainKnn[0]->GetEntries();
  aLOG(Log::INFO) <<coutRed<<" - Created KnnErr chain  "<<coutGreen<<inTreeNameKnn<<coutRed
                  <<"+"<<coutBlue<<inTreeKnnFrnd<<"("<<nEntriesChainKnn<<")"<<" from "<<coutGreen
                  <<inFileNameKnn<<coutRed<<"+"<<coutBlue<<inFileKnnFrnd<<coutDef<<endl;

  aRegEval->varKNN = new VarMaps(glob,utils,"varKNN");
  aRegEval->varKNN->connectTreeBranches(aRegEval->aChainKnn[0]);  // connect the tree so as to allocate memory for cut variables

  for(int nMLMnow=0; nMLMnow<nMLMs; nMLMnow++) {
    TString MLMname = getTagName(nMLMnow); if(mlmSkipNow[MLMname]) continue;
    
    if(!aRegEval->isErrKNNv[nMLMnow]) continue;
    
    setMethodCuts(aRegEval->varKNN,nMLMnow,false);

    TCut    cutsNow = aRegEval->varKNN->getTreeCuts("_comn") + aRegEval->varKNN->getTreeCuts(MLMname+"_valid");
    TString wgtReg  = getRegularStrForm(userWgtsM[MLMname+"_valid"],aRegEval->varKNN);

    TString inputComboNow = (TString)"[__ANNZ_VAR__]"+inputVariableV[nMLMnow]+"[__ANNZ_WGT__]"+wgtReg+"[__ANNZ_CUT__]"+(TString)cutsNow;
    inputComboNow.ReplaceAll(" ","").ReplaceAll("[__"," [__").ReplaceAll("__]","__] ");

    // if this is a new combination of variables/weights/cuts, create a new kd-tree
    if((aRegEval->allInputCombos.find(inputComboNow) == aRegEval->allInputCombos.end())) {
      aRegEval->allInputCombos[inputComboNow] = nMLMnow;

      aLOG(Log::DEBUG_1) <<coutBlue<<" - registering a new cmbination of input-variables and cuts ["
                         <<coutYellow<<inputComboNow<<coutBlue<<"] - in "<<coutGreen<<MLMname<<coutDef<<endl;

      setupKdTreeKNN( aRegEval->aChainKnn[0], aRegEval->knnErrOutFile[nMLMnow],
                      aRegEval->knnErrFactory[nMLMnow], aRegEval->knnErrDataLdr[nMLMnow],
                      aRegEval->knnErrModule[nMLMnow], aRegEval->trgIndexV,nMLMnow,cutsNow,wgtReg );
    }
    // if existing combination of variables and cuts, assign to the correct index
    else {
      int nMLMprev = aRegEval->allInputCombos[inputComboNow];

      aRegEval->knnErrOutFile[nMLMnow] = aRegEval->knnErrOutFile[nMLMprev];
      aRegEval->knnErrFactory[nMLMnow] = aRegEval->knnErrFactory[nMLMprev];
      aRegEval->knnErrDataLdr[nMLMnow] = aRegEval->knnErrDataLdr[nMLMprev];
      aRegEval->knnErrModule [nMLMnow] = aRegEval->knnErrModule [nMLMprev];

      aLOG(Log::DEBUG_1) <<coutPurple<<" - For "<<coutYellow<<MLMname<<coutPurple<<" found existing combination of "
                         <<"variables/cuts for kd-tree from "<<coutGreen<<getTagName(nMLMprev)<<coutDef<<endl;
      aLOG(Log::DEBUG_2) <<coutPurple<<"   --> ["<<coutYellow<<inputComboNow<<coutPurple<<"] ..."<<coutDef<<endl;
    }
  }

  // get the MLMs which are associated with each unique aRegEval->knnErrModule
  // -----------------------------------------------------------------------------------------------------------
  for(int nMLMnow=0; nMLMnow<nMLMs; nMLMnow++) {
    TString MLMname = getTagName(nMLMnow);
    if(mlmSkipNow[MLMname])           continue;
    if(!aRegEval->isErrKNNv[nMLMnow]) continue;

    aRegEval->getErrKNN[ aRegEval->knnErrModule[nMLMnow] ].push_back(nMLMnow);
  }

  for(int nMLMnow=0; nMLMnow<nMLMs; nMLMnow++) {
    TString MLMname = getTagName(nMLMnow); if(mlmSkipNow[MLMname]) continue;

    // formulae for input-variable errors, to be used by getRegClsErrINP()
    if(aRegEval->isErrINPv[nMLMnow]) {
      int nInErrs = (int)inNamesErr[nMLMnow].size();
      for(int nInErrNow=0; nInErrNow<nInErrs; nInErrNow++) {
        TString inVarErr = getTagInVarErr(nMLMnow,nInErrNow);

        aRegEval->varWrapper->NewForm(inVarErr,inNamesErr[nMLMnow][nInErrNow]);
        // cout <<getTagName(nMLMnow)<<" - inVarErr: "<<inVarErr<<CT<<inNamesErr[nMLMnow][nInErrNow]<<endl;
      }
    }
  }

  mlmSkipNow.clear();

  return;
}

// ===========================================================================================================
/**
 * @brief  - cleanup for the knn error estimation.
 */
// ===========================================================================================================
void ANNZ::evalRegErrCleanup() {
// ===========================================================================================================
  if(!aRegEval) return;
  
  aLOG(Log::DEBUG_1) <<coutRed<<" - starting ANNZ::evalRegErrCleanup() ... "<<coutDef<<endl;
  
  for(map <TString,int>::iterator Itr=aRegEval->allInputCombos.begin(); Itr!=aRegEval->allInputCombos.end(); ++Itr) {
    int nMLMnow = Itr->second; TString MLMname = getTagName(nMLMnow);

    cleanupKdTreeKNN(aRegEval->knnErrOutFile[nMLMnow],aRegEval->knnErrFactory[nMLMnow],aRegEval->knnErrDataLdr[nMLMnow]);

    utils->safeRM(getKeyWord(MLMname,"knnErrXML","outFileDirKnnErr"), inLOG(Log::DEBUG_1));
    utils->safeRM(getKeyWord(MLMname,"knnErrXML","outFileNameKnnErr"),inLOG(Log::DEBUG_1));
  }
  DELNULL(aRegEval->varKNN);
  
  if(aRegEval->aChainKnn.size() > 0) {
    if(aRegEval->aChainKnn.size() > 1) {
      if(dynamic_cast<TChain*>(aRegEval->aChainKnn[0]) && dynamic_cast<TChain*>(aRegEval->aChainKnn[1])) {
        aRegEval->aChainKnn[0]->RemoveFriend(aRegEval->aChainKnn[1]);
      }
      DELNULL(aRegEval->aChainKnn[1]);
    }
    DELNULL(aRegEval->aChainKnn[0]);
  }

  aRegEval->knnErrOutFile.clear(); aRegEval->knnErrFactory.clear();
  aRegEval->knnErrDataLdr.clear(); aRegEval->knnErrModule.clear();
  aRegEval->trgIndexV.clear();     aRegEval->aChainKnn.clear();
  aRegEval->getErrKNN.clear();     aRegEval->allInputCombos.clear();

  return;
}

// ===========================================================================================================
/**
 * @brief    - setup of evaluate regression - wrapper interface.
 * 
 * @details  - setup evaluation, used by the Wrapper class, to be called from e.g. python.
 *           this allowes individual events to be estimated from seperate python calls, where the
 *           corresponding resources are only loaded once upon setup
 */
// ===========================================================================================================
void ANNZ::evalRegWrapperSetup() {
// ===========================================================================================================
  aLOG(Log::INFO) <<coutWhiteOnBlack<<coutPurple<<" - starting ANNZ::evalRegWrapperSetup() ... "<<coutDef<<endl;
  
  evalRegSetup();

  loadReaders(aRegEval->mlmSkip);
  
  evalRegErrSetup();

  // weight formulae setup
  int nMLMs = glob->GetOptI("nMLMs");

  for(int nMLMnow=0; nMLMnow<nMLMs; nMLMnow++) {
    TString MLMname    = getTagName(nMLMnow);      if(aRegEval->mlmSkip[MLMname]) continue;
    TString MLMname_w  = getTagWeight(nMLMnow);

    // create MLM-weight formulae for the input variables
    TString wgtStr = getRegularStrForm(userWgtsM[MLMname+"_valid"],aRegEval->varWrapper);
    // cout <<" -- NewForm() ++ MLMname_w,wgtStr: "<<MLMname_w<<CT<<wgtStr<<endl;
    aRegEval->varWrapper->NewForm(MLMname_w,wgtStr);
    // cout <<" ++ NewForm() ++ MLMname_w,wgtStr: "<<MLMname_w<<CT<<wgtStr<<endl;
  }
  
  return;
}

// ===========================================================================================================
/**
 * @brief    - single event estimation of evaluate regression - wrapper interface.
 */
// ===========================================================================================================
TString ANNZ::evalRegWrapperLoop() {
// ===========================================================================================================
  aLOG(Log::DEBUG_2) <<coutWhiteOnBlack<<coutPurple<<" - starting ANNZ::evalRegWrapperLoop() ... "<<coutDef<<endl;
  // aRegEval->varWrapper->printVars();

  TString output("");

  VarMaps * var            = aRegEval->varWrapper;
  int     nMLMs            = glob->GetOptI("nMLMs");
  int     nPDFs            = glob->GetOptI("nPDFs");
  int     nPDFbins         = glob->GetOptI("nPDFbins");
  TString baseTag_v        = glob->GetOptC("baseTag_v");
  TString baseTag_e        = glob->GetOptC("baseTag_e");
  TString baseTag_w        = glob->GetOptC("baseTag_w");
  bool    defErrBySigma68  = glob->GetOptB("defErrBySigma68");
  bool    isBinCls         = glob->GetOptB("doBinnedCls");
  bool    doBiasCorPDF     = glob->GetOptB("doBiasCorPDF");
  bool    doStorePdfBins   = glob->GetOptB("doStorePdfBins");
  int     nSmearsRnd       = glob->GetOptI("nSmearsRnd");
  double  nSmearUnf        = glob->GetOptI("nSmearUnf"); // and cast to double, since we divide by this later
  TString regBestNameVal   = getTagBestMLMname(baseTag_v);
  TString regBestNameErr   = getTagBestMLMname(baseTag_e);
  TString regBestNameErrN  = getTagBestMLMname(baseTag_e+"N");
  TString regBestNameErrP  = getTagBestMLMname(baseTag_e+"P");
  TString regBestNameWgt   = getTagBestMLMname(baseTag_w);
  TRandom * rnd            = aRegEval->rnd;

  vector < double > binClsVal(nMLMs,0), binClsErr(nMLMs,0), binClsWgt(nMLMs,0);
  
  aRegEval->pdfWgtValV.resize(nPDFs,vector<double>(2,0));
  aRegEval->pdfWgtNumV.resize(nPDFs,vector<double>(2,0));
  aRegEval->regErrV   .resize(nMLMs,vector<double>(3,0));

  for(int nPDFnow=0; nPDFnow<nPDFs; nPDFnow++) {
    aRegEval->hisPDF_w  [nPDFnow]->Reset();
    aRegEval->mlmAvg_val[nPDFnow].resize(nMLMs,0);
    aRegEval->mlmAvg_err[nPDFnow].resize(nMLMs,0);
    aRegEval->mlmAvg_wgt[nPDFnow].resize(nMLMs,0);

    aRegEval->pdfWgtValV[nPDFnow][0] = aRegEval->pdfWgtValV[nPDFnow][1] = 0;
    aRegEval->pdfWgtNumV[nPDFnow][0] = aRegEval->pdfWgtNumV[nPDFnow][1] = 0;
  }

  // -----------------------------------------------------------------------------------------------------------
  // calculate the KNN errors if needed, for each variation of aRegEval->knnErrModule
  // -----------------------------------------------------------------------------------------------------------
  if(aRegEval->hasErrKNN) {
    for(map < TMVA::kNN::ModulekNN*,vector<int> >::iterator Itr=aRegEval->getErrKNN.begin(); Itr!=aRegEval->getErrKNN.end(); ++Itr) {
      getRegClsErrKNN(var,Itr->first,aRegEval->trgIndexV,Itr->second,!isBinCls,aRegEval->regErrV);
    }
  }
  if(aRegEval->hasErrs) {
    for(int nMLMnow=0; nMLMnow<nMLMs; nMLMnow++) {
      TString MLMname = getTagName(nMLMnow);
      if(!aRegEval->isErrINPv[nMLMnow] || aRegEval->mlmSkip[MLMname]) continue;

      getRegClsErrINP(var,true,nMLMnow,&(aRegEval->seed),&(aRegEval->regErrV[nMLMnow]));
    }
  }

  // -----------------------------------------------------------------------------------------------------------
  // binned classification
  // -----------------------------------------------------------------------------------------------------------
  if(isBinCls) {
    // -----------------------------------------------------------------------------------------------------------
    // 
    // -----------------------------------------------------------------------------------------------------------
    for(int nMLMnow=0; nMLMnow<nMLMs; nMLMnow++) {
      TString MLMname   = getTagName(nMLMnow);  if(aRegEval->mlmSkip[MLMname]) continue;
      TString MLMname_e = getTagError(nMLMnow); TString MLMname_w = getTagWeight(nMLMnow);

      binClsVal[nMLMnow] = getReader(var,ANNZ_readType::PRB,true,nMLMnow);
      binClsWgt[nMLMnow] = var->GetForm(MLMname_w);
      binClsErr[nMLMnow] = aRegEval->regErrV[nMLMnow][1];

      // cout << MLMname<<CT<< binClsVal[nMLMnow]<<CT<<binClsWgt[nMLMnow]<<CT<<binClsErr[nMLMnow]<<endl;
    }

    // -----------------------------------------------------------------------------------------------------------
    // setup the pdf clculation
    // -----------------------------------------------------------------------------------------------------------
    for(int nPDFnow=0; nPDFnow<nPDFs; nPDFnow++) {
      // go over all pdf bins
      for(int nPdfBinNow=0; nPdfBinNow<nPDFbins; nPdfBinNow++) {
        // in each pdf-bin, use the overlapping cls-bins
        for(int nClsBinNow=0; nClsBinNow<aRegEval->nClsBinsIn[nPdfBinNow]; nClsBinNow++) {
          int    clsIndex   = aRegEval->pdfBinWgt[nPdfBinNow][nClsBinNow].first;
          double binWgt     = aRegEval->pdfBinWgt[nPdfBinNow][nClsBinNow].second;

          double  binVal    = max(min(binClsVal[clsIndex],1.),0.);
          double  clsWgt    = binClsWgt[clsIndex];
          double  totWgt    = binVal * binWgt * clsWgt;

          aRegEval->hisPDF_w[nPDFnow]->Fill(zPDF_binC[nPdfBinNow],totWgt);
         
          aRegEval->pdfWgtValV[nPDFnow][1] += totWgt;
          aRegEval->pdfWgtNumV[nPDFnow][1] += binVal * binWgt;

          // generate random smearing factors for one of the PDFs
          // -----------------------------------------------------------------------------------------------------------
          if(nPDFnow == 1) {
            double clsErr = binClsErr[clsIndex];

            if(clsErr > EPS) {
              for(int nSmearRndNow=0; nSmearRndNow<nSmearsRnd; nSmearRndNow++) {
                double sfNow     = fabs(rnd->Gaus(0,clsErr));        if(nSmearRndNow%2 == 0) sfNow *= -1;
                double binSmr    = max(min((binVal + sfNow),1.),0.);
                double totWgtSmr = binSmr * binWgt * clsWgt;

                aRegEval->hisPDF_w[nPDFnow]->Fill(zPDF_binC[nPdfBinNow],totWgtSmr);
                
                aRegEval->pdfWgtValV[nPDFnow][1] += totWgtSmr;
                aRegEval->pdfWgtNumV[nPDFnow][1] += binSmr * binWgt;
              }
            }
          }
        }
      }
    }
  }
  // -----------------------------------------------------------------------------------------------------------
  // randomized regression
  // -----------------------------------------------------------------------------------------------------------
  else {
    // -----------------------------------------------------------------------------------------------------------
    // 
    // -----------------------------------------------------------------------------------------------------------
    for(int nMLMnow=0; nMLMnow<nMLMs; nMLMnow++) {
      TString MLMname    = getTagName(nMLMnow);      if(aRegEval->mlmSkip[MLMname]) continue;
      TString MLMname_e  = getTagError(nMLMnow,"");  TString MLMname_w  = getTagWeight(nMLMnow);
      TString MLMname_eN = getTagError(nMLMnow,"N"); TString MLMname_eP = getTagError(nMLMnow,"P");

      double regVal(0), regErr(0), regErrN(0), regErrP(0), regWgt(0);

      regVal = getReader(var,ANNZ_readType::REG,true,nMLMnow);
      regWgt = var->GetForm(MLMname_w);

      // sanity check that weights are properly defined
      if(regWgt < 0) {
        var->printVars();
        VERIFY(LOCATION,(TString)"Weights("+MLMname_w+") can only be >= 0 ... Something is horribly wrong ?!?",false);
      }

      regErrN = aRegEval->regErrV[nMLMnow][0];
      regErr  = aRegEval->regErrV[nMLMnow][1];
      regErrP = aRegEval->regErrV[nMLMnow][2];

      output += (TString)"\""+MLMname   +"\":"+utils->floatToStr(regVal) +",";
      output += (TString)"\""+MLMname_w +"\":"+utils->floatToStr(regWgt) +",";
      output += (TString)"\""+MLMname_eN+"\":"+utils->floatToStr(regErrN)+",";
      output += (TString)"\""+MLMname_e +"\":"+utils->floatToStr(regErr) +",";
      output += (TString)"\""+MLMname_eP+"\":"+utils->floatToStr(regErrP)+",";


      // the "best" MLM solution
      if(nMLMnow == aRegEval->bestANNZindex) {
        output += (TString)"\""+regBestNameVal +"\":"+utils->floatToStr(regVal) +",";
        output += (TString)"\""+regBestNameWgt +"\":"+utils->floatToStr(regWgt) +",";
        output += (TString)"\""+regBestNameErrN+"\":"+utils->floatToStr(regErrN)+",";
        output += (TString)"\""+regBestNameErr +"\":"+utils->floatToStr(regErr) +",";
        output += (TString)"\""+regBestNameErrP+"\":"+utils->floatToStr(regErrP)+",";
      }

      for(int nPDFnow=0; nPDFnow<nPDFs; nPDFnow++) {
        double pdfWgt = aRegEval->pdfWeightV[nPDFnow][nMLMnow] * regWgt;  if(pdfWgt < EPS) continue;

        aRegEval->pdfWgtValV[nPDFnow][0] += regWgt;
        aRegEval->pdfWgtValV[nPDFnow][1] += pdfWgt;
        aRegEval->pdfWgtNumV[nPDFnow][0] += 1;
        aRegEval->pdfWgtNumV[nPDFnow][1] += aRegEval->pdfWeightV[nPDFnow][nMLMnow];

        // input original value into the pdf before smearing
        aRegEval->hisPDF_w[nPDFnow]->Fill(regVal,pdfWgt);

        aRegEval->mlmAvg_val[nPDFnow][nMLMnow] = regVal;
        aRegEval->mlmAvg_err[nPDFnow][nMLMnow] = regErr;
        aRegEval->mlmAvg_wgt[nPDFnow][nMLMnow] = regWgt;

        // generate random smearing factors for this MLM
        for(int nSmearRndNow=0; nSmearRndNow<nSmearsRnd; nSmearRndNow++) {
          int     signNow(-1);
          double  errNow(regErrN);
          if(nSmearRndNow%2 == 0) { errNow = regErrP; signNow = 1; }

          double sfNow  = signNow * fabs(rnd->Gaus(0,errNow));
          double regSmr = regVal + sfNow;
          aRegEval->hisPDF_w[nPDFnow]->Fill(regSmr,pdfWgt);
        }
      }
    }
  }

  // -----------------------------------------------------------------------------------------------------------
  // calculate the pdf
  // -----------------------------------------------------------------------------------------------------------
  for(int nPDFnow=0; nPDFnow<nPDFs; nPDFnow++) {
    double intgrPDF_w = aRegEval->hisPDF_w[nPDFnow]->Integral();

    if(intgrPDF_w > EPS) {
      // rescale the weighted probability distribution
      aRegEval->hisPDF_w[nPDFnow]->Scale(1/intgrPDF_w);

      // apply the bias-correction to the pdf
      // -----------------------------------------------------------------------------------------------------------
      if(doBiasCorPDF) {
        TString clnName        = (TString)aRegEval->hisPDF_w[nPDFnow]->GetName()+"_TMP";
        TH1     * hisPDF_w_TMP = (TH1*)aRegEval->hisPDF_w[nPDFnow]->Clone(clnName);

        for(int nBinXnow=1; nBinXnow<nPDFbins+1; nBinXnow++) {
          double val = hisPDF_w_TMP->GetBinContent(nBinXnow);

          if(val < aRegEval->minWeight)                                     continue;
          if(!aRegEval->hisBiasCorV[nPDFnow][nBinXnow-1])                   continue;
          if( aRegEval->hisBiasCorV[nPDFnow][nBinXnow-1]->Integral() < EPS) continue;

          val /= nSmearUnf;
          for(int nSmearUnfNow=0; nSmearUnfNow<nSmearUnf; nSmearUnfNow++) {
            double rndVal = aRegEval->hisBiasCorV[nPDFnow][nBinXnow-1]->GetRandom();
            aRegEval->hisPDF_w[nPDFnow]->Fill(rndVal,val);
          }
        }

        intgrPDF_w = aRegEval->hisPDF_w[nPDFnow]->Integral();
        if(intgrPDF_w > EPS) aRegEval->hisPDF_w[nPDFnow]->Scale(1/intgrPDF_w);

        DELNULL(hisPDF_w_TMP);
      }
    }

    // if the objects was skipped (zero weight), the average value will have the
    // default (std::numeric_limits<float>::max()), but to avoid very big meaningless output, set the pdf-bins to zero
    // -----------------------------------------------------------------------------------------------------------
    if(intgrPDF_w < EPS) {
      if(doStorePdfBins) {
        for(int nPdfBinNow=0; nPdfBinNow<nPDFbins; nPdfBinNow++) {
          TString pdfBinName = getTagPdfBinName(nPDFnow,nPdfBinNow);

          output += (TString)"\""+pdfBinName+"\": 0,";
        }
      }
      for(int nPdfTypeNow=0; nPdfTypeNow<aRegEval->nPdfTypes; nPdfTypeNow++) {
        if((isBinCls && nPdfTypeNow == 0) || nPdfTypeNow == 2) continue;

        TString pdfAvgErrName = getTagPdfAvgName(nPDFnow,(TString)baseTag_e+aRegEval->tagNameV[nPdfTypeNow]);
        TString pdfAvgWgtName = getTagPdfAvgName(nPDFnow,(TString)baseTag_w+aRegEval->tagNameV[nPdfTypeNow]);

        output += (TString)"\""+pdfAvgErrName+"\": -1,";
        output += (TString)"\""+pdfAvgWgtName+"\": 0,";
      }
      continue;
    }

    // the value of the pdf in the different bins
    // -----------------------------------------------------------------------------------------------------------
    if(doStorePdfBins) {
      for(int nPdfBinNow=0; nPdfBinNow<nPDFbins; nPdfBinNow++) {
        TString pdfBinName = getTagPdfBinName(nPDFnow,nPdfBinNow);
        double  pdfValNow  = aRegEval->hisPDF_w[nPDFnow]->GetBinContent(nPdfBinNow+1);

        output += (TString)"\""+pdfBinName+"\":"+utils->floatToStr(pdfValNow) +",";
      }
    }

    // the average value and the width of the pdf distribution
    for(int nPdfTypeNow=0; nPdfTypeNow<aRegEval->nPdfTypes; nPdfTypeNow++) {
      if(isBinCls && nPdfTypeNow == 0) continue;

      TString pdfAvgName    = getTagPdfAvgName(nPDFnow,(TString)baseTag_v+aRegEval->tagNameV[nPdfTypeNow]);
      TString pdfAvgErrName = getTagPdfAvgName(nPDFnow,(TString)baseTag_e+aRegEval->tagNameV[nPdfTypeNow]);
      TString pdfAvgWgtName = getTagPdfAvgName(nPDFnow,(TString)baseTag_w+aRegEval->tagNameV[nPdfTypeNow]);

      if(nPdfTypeNow == 0) {
        double avg_val(0), avg_err(0), sum_wgt(0);
        for(int nMLMnow=0; nMLMnow<nMLMs; nMLMnow++) {
          double regWgt = aRegEval->mlmAvg_wgt[nPDFnow][nMLMnow];  if(regWgt < EPS) continue;
          double regVal = aRegEval->mlmAvg_val[nPDFnow][nMLMnow];
          double regErr = aRegEval->mlmAvg_err[nPDFnow][nMLMnow];

          sum_wgt += regWgt; avg_val += regWgt*regVal; avg_err += regWgt*regErr;
        }
        if(sum_wgt > EPS) {
          output += (TString)"\""+pdfAvgName   +"\":"+utils->floatToStr(avg_val/sum_wgt) +",";
          output += (TString)"\""+pdfAvgErrName+"\":"+utils->floatToStr(avg_err/sum_wgt) +",";
        }
      }
      else if(nPdfTypeNow == 1) {
        utils->param->clearAll();
        utils->param->NewOptF("meanWithoutOutliers",5);
        if(utils->getInterQuantileStats(aRegEval->hisPDF_w[nPDFnow])) {
          double  regAvgPdfVal  = utils->param->GetOptF("quant_mean_Nsig68");
          double  regAvgPdfErr  = defErrBySigma68 ? utils->param->GetOptF("quant_sigma_68") : utils->param->GetOptF("quant_sigma");

          output += (TString)"\""+pdfAvgName   +"\":"+utils->floatToStr(regAvgPdfVal) +",";
          output += (TString)"\""+pdfAvgErrName+"\":"+utils->floatToStr(regAvgPdfErr) +",";
          // cout << "xx "<<nPDFnow<<CT<<pdfAvgName<<CT<<regAvgPdfVal<<endl;
        }
      }
      else if(nPdfTypeNow == 2) {
        int maxBin = aRegEval->hisPDF_w[nPDFnow]->GetMaximumBin() - 1; // histogram bins start at 1, not at 0

        output += (TString)"\""+pdfAvgName+"\":"+utils->floatToStr(zPDF_binC[maxBin]) +",";
      }

      if(nPdfTypeNow < 2) {
        VERIFY(LOCATION,(TString)"If intgrPDF_w>0 then there is no way that pdfWgtNumV==0 ... "
                                +"something is horribly wrong ?!?!",(aRegEval->pdfWgtNumV[nPDFnow][nPdfTypeNow] > 0));

        aRegEval->pdfWgtValV[nPDFnow][nPdfTypeNow] /= aRegEval->pdfWgtNumV[nPDFnow][nPdfTypeNow];
        
        output += (TString)"\""+pdfAvgWgtName+"\":"+utils->floatToStr(aRegEval->pdfWgtValV[nPDFnow][nPdfTypeNow]) +",";
      }
    }
  }

  binClsVal.clear();  binClsErr.clear();  binClsWgt.clear();

  output = (TString)"{"+output+"}";
  // aLOG(Log::DEBUG_2) <<coutWhiteOnBlack<<coutGreen<<" - output: "<<coutBlue<<output<<coutDef<<endl;

  return output;
}

// ===========================================================================================================
/**
 * @brief    - cleanup of evaluate regression - wrapper interface.
 */
// ===========================================================================================================
void ANNZ::evalRegWrapperCleanup() {
// ===========================================================================================================
  aLOG(Log::DEBUG_1) <<coutWhiteOnBlack<<coutPurple<<" - starting ANNZ::evalRegWrapperCleanup() ... "<<coutDef<<endl;

  evalRegErrCleanup();
  
  clearReaders();
  
  DELNULL(aRegEval->loopChain);

  return;
}
