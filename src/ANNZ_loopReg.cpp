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
 * @brief    - Perform optimization of regression results: find best MLM, derive PDF weights and produce performance plots.
 * 
 * @details  - Randomized regression:
 *           - Training dataset:
 *             1. fillColosureV():       Calculate the "nominal metrics" (bias, scatter and combined 2,3sigma outlier-fraction) in bins
 *                                       of the regression target (zTgr), and as averaged over the entire sample.
 *             2. getBestANNZ():         Derive the "best" MLM solution, as the one which minimizes the combination
 *                                       of nominal metrics.
 *             3. getRndMethodBestPDF(): Generate candidate PDF weighting schemes and select the "best" solution.
 *           - Validation dataset:
 *             1. doEvalReg():           Evaluate the results (produce PDFs etc.)
 *             2. doMetricPlots():       create performance plots.
 *           - Binned classifiction:
 *             - Perform steps 1,2 of the validation phase.
 */
// ===========================================================================================================
void ANNZ::optimReg() {
// ====================
  aLOG(Log::INFO) <<coutWhiteOnBlack<<coutCyan<<" - starting ANNZ::optimReg() ... "<<coutDef<<endl;

  int     nMLMs             = glob->GetOptI("nMLMs");
  int     nPDFs             = glob->GetOptI("nPDFs");
  // bool    separateTestValid = glob->GetOptB("separateTestValid"); // deprecated
  TString addOutputVars     = glob->GetOptC("addOutputVars");
  TString indexName         = glob->GetOptC("indexName");
  TString zTrg              = glob->GetOptC("zTrg");
  bool    isBinCls          = glob->GetOptB("doBinnedCls");
  bool    doBiasCorPDF      = glob->GetOptB("doBiasCorPDF");

  TString outDirNameOrig(outputs->GetOutDirName()), outDirName(""), inTreeName(""), inFileName("");

  for(int nTrainValidNow=0; nTrainValidNow<2; nTrainValidNow++) {
    // deprecated
    // TCut    aCut("");
    // TString treeNamePostfix("");
    // if(separateTestValid) {
    //   aCut            = (TCut)   ((nTrainValidNow == 0) ? getTrainTestCuts("_train") : getTrainTestCuts("_valid"));
    //   treeNamePostfix = (TString)((nTrainValidNow == 0) ? "_valid"                   : "_valid");
    // }
    // else {
    //   treeNamePostfix = (TString)((nTrainValidNow == 0) ? "_train"                   : "_valid");
    // }
    TString treeNamePostfix = (TString)((nTrainValidNow == 0) ? "_train" : "_valid");

    if(isBinCls && nTrainValidNow == 0 && !doBiasCorPDF) continue;

    // create the chain for the loop
    // -----------------------------------------------------------------------------------------------------------
    inTreeName = (TString)glob->GetOptC("treeName")+treeNamePostfix;
    inFileName = (TString)glob->GetOptC("inputTreeDirName")+inTreeName+"*.root";

    TChain * aChain_0 = new TChain(inTreeName,inTreeName); aChain_0->SetDirectory(0); aChain_0->Add(inFileName);
    int nEntriesChain_0 = aChain_0->GetEntries();
    aLOG(Log::DEBUG) <<coutRed<<" - added chain "<<coutGreen<<inTreeName<<"("<<nEntriesChain_0<<")"<<" from "<<coutBlue<<inFileName<<coutDef<<endl;

    inTreeName = (TString)glob->GetOptC("treeName")+treeNamePostfix;
    inFileName = (TString)glob->GetOptC("postTrainDirNameFull")+inTreeName+"*.root";

    TChain * aChain_1 = new TChain(inTreeName,inTreeName); aChain_1->SetDirectory(0); aChain_1->Add(inFileName);
    int nEntriesChain_1 = aChain_1->GetEntries(); 
    VERIFY(LOCATION,(TString)"Main and friend chains have different numbers of entries ... Something is horribly wrong !!!"
                   ,(nEntriesChain_0 == nEntriesChain_1));

    // deprecated
    // // define a temporary VarMaps so as to regularize the weight expression
    // for(int nChainNow=0; nChainNow<2; nChainNow++) {
    //   aCut = (TCut)getRegularStrForm((TString)aCut,NULL,((nChainNow == 0) ? aChain_0 : aChain_1));
    // }

    // get the list of branch names from the friend-chain (all ANNZ_* branches), and add the target name
    // -----------------------------------------------------------------------------------------------------------
    vector <TString> acceptV;
    utils->getTreeBranchNames(aChain_1,acceptV);
    acceptV.push_back(indexName);
    acceptV.push_back(zTrg);

    // extract the requested added variables and add them to acceptV (needed for inclusion in the output of doEvalReg())
    vector <TString> addVarV = utils->splitStringByChar(addOutputVars,';');
    for(int nVarsInNow=0; nVarsInNow<(int)addVarV.size(); nVarsInNow++) {
      TString addVarName = addVarV[nVarsInNow];

      if(find(acceptV.begin(),acceptV.end(),addVarName) == acceptV.end()) acceptV.push_back(addVarName);
    }

    // check if any variables used for cuts but not requested by the user need to be included in the tree
    TString cutStr = (TString)getTrainTestCuts((TString)"_comn"+";"+getTagName(0)+"_train"+";"+getTagName(0)+"_valid");

    vector <TString> chain0_nameV, addPlotVarV;
    utils->getTreeBranchNames(aChain_0,chain0_nameV);

    for(int nVarsInNow=0; nVarsInNow<(int)chain0_nameV.size(); nVarsInNow++) {
      TString addVarName = chain0_nameV[nVarsInNow];

      if(cutStr.Contains(addVarName) && find(acceptV.begin(),acceptV.end(),addVarName) == acceptV.end()) {
        acceptV    .push_back(addVarName);
        addPlotVarV.push_back(addVarName);
      }
    }

    // create subdirectory for the output trees, as the names will be the same in case of separateTestValid
    outDirName = (TString)outDirNameOrig+((nTrainValidNow == 0) ? "train" : "valid")+"/";
    outputs->InitializeDir(outDirName,glob->GetOptC("baseName"));

    // create the merged chain
    // TChain * aChainMerged = mergeTreeFriends(aChain_0,aChain_1,NULL,&acceptV,NULL,aCut); // deprecated
    TChain * aChainMerged = mergeTreeFriends(aChain_0,aChain_1,NULL,&acceptV);

    verifyIndicesMLM(aChainMerged);

    outputs->SetOutDirName(outDirNameOrig); // redirect outputs back to the current directory

    acceptV.clear(); chain0_nameV.clear(); addVarV.clear();
    DELNULL(aChain_0); DELNULL(aChain_1);

    // ----------------------------------------------------------------------------------------------------------- 
    // _train 
    // ----------------------------------------------------------------------------------------------------------- 
    if(nTrainValidNow == 0) {
      vector <TH2*> hisPdfBiasCorV;

      if(isBinCls) {
        getBinClsBiasCorPDF(aChainMerged,hisPdfBiasCorV);
      } 
      else {
        vector <int>               bestMLMsV;
        map < int,vector<int> >    zRegQnt_nANNZ;
        map < int,vector<double> > zRegQnt_bias, zRegQnt_sigma68, zRegQnt_fracSig68;

        // ----------------------------------------------------------------------------------------------------------- 
        // calculate the optimization metrics from the training trees 
        // ----------------------------------------------------------------------------------------------------------- 
        fillColosureV(zRegQnt_nANNZ, zRegQnt_bias, zRegQnt_sigma68, zRegQnt_fracSig68, aChainMerged);

        // ----------------------------------------------------------------------------------------------------------- 
        // find the "best" MLMs, given the three metrics: bias, sig68 and fracSig68,
        // where for optimCondReg (one of the three) the best several methods within the top fracLim
        // percentile are chosen, so long as for the other two metrics, the MLMs are within
        // the top (fracLim + 1_sigma) of the distribution of MLMs ---> This selects the solution
        // which gives the "best" of the selected metric, which is also not the "worst" of the other two.
        // If not enough methods are chosen (at least minNoptimMLMs), try again with a higher percentage of the dist
        // ----------------------------------------------------------------------------------------------------------- 
        getBestANNZ(zRegQnt_nANNZ, zRegQnt_bias, zRegQnt_sigma68, zRegQnt_fracSig68, bestMLMsV);

        VERIFY(LOCATION,(TString)"Could not find any accepted MLMs which pass minimal metric-cuts ... Something is horribly wrong !!!",
                                 ((int)bestMLMsV.size() > 0));

        // -----------------------------------------------------------------------------------------------------------
        // find the collection of methods with optimal error and store it in bestWeightsV
        // -----------------------------------------------------------------------------------------------------------
        int bestANNZindex = bestMLMsV[0];
        aLOG(Log::INFO) <<coutBlue<<" - The \"best\" MLM is: "<<coutRed<<getTagName(bestANNZindex)<<coutDef<<endl;

        vector < vector<double> > bestWeightsV;
        getRndMethodBestPDF(aChainMerged, bestANNZindex, zRegQnt_nANNZ[-1], zRegQnt_bias[-1], zRegQnt_sigma68[-1],
                            zRegQnt_fracSig68[-1], bestWeightsV, hisPdfBiasCorV);

        VERIFY(LOCATION,(TString)"After getRndMethodBestPDF(), found [bestWeightsV.size() = "+utils->intToStr((int)bestWeightsV.size())
                                 +"] - but expected "+"this to be [= "+utils->intToStr(nPDFs)+"] ... Something is horribly wrong !!!"
                                 , ((int)bestWeightsV.size() == nPDFs));

        for(int nPDFnow=0; nPDFnow<nPDFs; nPDFnow++) {
          VERIFY(LOCATION,(TString)"After getRndMethodBestPDF(), found [bestWeightsV[0,1].size() = "
                                   +utils->intToStr((int)bestWeightsV.size())
                                   +"] - but expected this to be [= nMLMs = "+utils->intToStr(nMLMs)
                                   +"] ... Something is horribly wrong !!!",((int)bestWeightsV[nPDFnow].size() == nMLMs));
        }

        // parse the weight vectors of the two pdfs into strings
        vector <TString> pdfWeightList(nPDFs,"");
        for(int nPDFnow=0; nPDFnow<nPDFs; nPDFnow++) {
          TString pdfWeightListPrint("");
          for(int nMLMnow=0; nMLMnow<nMLMs; nMLMnow++) {
            pdfWeightList[nPDFnow] += (TString)utils->floatToStr(bestWeightsV[nPDFnow][nMLMnow])+";";

            pdfWeightListPrint     += (TString)coutGreen+getTagName(nMLMnow)+":"+coutPurple
                                              +utils->floatToStr(bestWeightsV[nPDFnow][nMLMnow])+coutGreen+", ";
          }
          // remove trailing ";"
          pdfWeightList[nPDFnow] = pdfWeightList[nPDFnow](0,pdfWeightList[nPDFnow].Length()-1);

          aLOG(Log::INFO) <<coutBlue<<" - Derived the following PDF("<<nPDFnow<<") weights: "<<pdfWeightListPrint<<coutDef<<endl;
        }

        // save the optimization results to file
        // -----------------------------------------------------------------------------------------------------------
        TString saveFileName = getKeyWord("","optimResults","configSaveFileName");
        aLOG(Log::INFO)<<coutYellow<<" - Saving optimization results in "<<coutGreen<<saveFileName<<coutYellow<<" ..."<<coutDef<<endl;

        OptMaps * optMap = new OptMaps("localOptMap");
        TString          saveName("");
        vector <TString> optNames;
        
        saveName = glob->versionTag();  optNames.push_back(saveName); optMap->NewOptC(saveName, glob->GetOptC(glob->versionTag()));
        saveName = "bestMLM";           optNames.push_back(saveName); optMap->NewOptI(saveName, bestANNZindex);
        saveName = "userPdfBins";       optNames.push_back(saveName); optMap->NewOptC(saveName, glob->GetOptC("userPdfBins"));
        
        for(int nPDFnow=0; nPDFnow<nPDFs; nPDFnow++) {
          saveName = TString::Format("weightsPDF_%d",nPDFnow); optNames.push_back(saveName); optMap->NewOptC(saveName, pdfWeightList[nPDFnow]);
        }
        
        utils->optToFromFile(&optNames,optMap,saveFileName,"WRITE");

        optNames.clear(); DELNULL(optMap);

        bestMLMsV.clear(); zRegQnt_nANNZ.clear(); zRegQnt_bias.clear();
        zRegQnt_sigma68.clear(); zRegQnt_fracSig68.clear(); pdfWeightList.clear();
        bestWeightsV.clear();
      }

      // save the histograms of the pdf bias-correction in a root file
      // ----------------------------------------------------------------------------------------------------------- 
      if(doBiasCorPDF) {
        TString optimVerifName = (TString)(isBinCls ? "verifResults" : "optimResults");
        TString rootFileName   = getKeyWord("",optimVerifName,"rootSaveFileName");
        TString biasCorHisTag  = getKeyWord("",optimVerifName,"biasCorHisTag");

        aLOG(Log::INFO)<<coutYellow<<" - Saving bias-correction results in "<<coutGreen<<rootFileName<<coutYellow<<" ..."<<coutDef<<endl;

        TFile * rootSaveFile = new TFile(rootFileName,"RECREATE");

        for(int nPDFnow=0; nPDFnow<nPDFs; nPDFnow++) { hisPdfBiasCorV[nPDFnow]->Write((TString)biasCorHisTag+utils->intToStr(nPDFnow)); }

        rootSaveFile->Close();  DELNULL(rootSaveFile);
      }
      hisPdfBiasCorV.clear();

    }

    // ----------------------------------------------------------------------------------------------------------- 
    // _valid
    // run on the _valid chain, evaluate the pdf and other solutions and produce performance plots
    // ----------------------------------------------------------------------------------------------------------- 
    if(nTrainValidNow == 1) {
      // create subdirectory for the output trees, as the names will be the same in case of separateTestValid
      outDirName = (TString)outDirNameOrig+"eval"+"/";
      outputs->InitializeDir(outDirName,glob->GetOptC("baseName"));
      
      // -----------------------------------------------------------------------------------------------------------
      // evaluate the _valid chain
      // -----------------------------------------------------------------------------------------------------------
      doEvalReg(aChainMerged,outDirName,&addPlotVarV);

      // create the chain for the plot-loop from the evaluated _valid chain
      // -----------------------------------------------------------------------------------------------------------
      inTreeName = (TString)glob->GetOptC("treeName")+glob->GetOptC("_typeANNZ");
      inFileName = (TString)outDirName+inTreeName+"*.root";

      TChain * aChain_plots = new TChain(inTreeName,inTreeName); aChain_plots->SetDirectory(0); aChain_plots->Add(inFileName);
      int nEntriesChain_plots = aChain_plots->GetEntries();
      aLOG(Log::DEBUG) <<coutRed<<"Created chain for plotting "<<coutGreen<<inTreeName<<"("<<nEntriesChain_plots
                       <<")"<<" from "<<coutBlue<<inFileName<<coutDef<<endl;

      doMetricPlots(aChain_plots,&addPlotVarV);

      outputs->SetOutDirName(outDirNameOrig); // redirect outputs back to the current directory

      DELNULL(aChain_plots);
    }

    addPlotVarV.clear();
    DELNULL(aChainMerged);
  }

  // cleanup the transient trees we created during optimization
  if(!glob->GetOptB("keepOptimTrees_randReg")) {
    for(int nTrainValidNow=0; nTrainValidNow<2; nTrainValidNow++) {
      if(isBinCls && nTrainValidNow == 0) continue;

      outDirName = (TString)outDirNameOrig+((nTrainValidNow == 0) ? "train" : "valid")+"/";
      utils->safeRM(outDirName,inLOG(Log::DEBUG));
    }
  }

  return;
}


// ===========================================================================================================
/**
 * @brief                    - Calculate the "nominal metrics", the bias, scatter and combined 2,3sigma outlier fractions
 *                           for the different MLMs.
 * 
 * @param zRegQnt_nANNZ      - Map of vector, which is filled with the index of MLMs, corresponding to the order of the
 *                           entries of the metrics in the other vectors (zRegQnt_bias, zRegQnt_sigma68 and zRegQnt_fracSig68).
 * @param zRegQnt_bias       - Map of vector, which is filled with the claculated bias metric.
 * @param zRegQnt_sigma68    - Map of vector, which is filled with the claculated sigma68 metric.
 * @param zRegQnt_fracSig68  - Map of vector, which is filled with the claculated combined outlier-fraction metric.
 * @param aChain             - Input chain, for which the metrics are calculated.
 */
// ===========================================================================================================
void  ANNZ::fillColosureV(
  map < int,vector<int> >    & zRegQnt_nANNZ,   map < int,vector<double> > & zRegQnt_bias,
  map < int,vector<double> > & zRegQnt_sigma68, map < int,vector<double> > & zRegQnt_fracSig68,
  TChain * aChain
) {
// ===========================================================================================================
  VERIFY(LOCATION,(TString)"Memory leak ?! ",(dynamic_cast<TChain*>(aChain)));

  aLOG(Log::INFO) <<coutWhiteOnBlack<<coutPurple<<" - starting ANNZ::fillColosureV() ... "<<coutDef<<endl;

  TString hisName(""), drawExprs(""), wgtCut(""), zAxisTitle("");
  bool    breakLoop(false);

  int     nMLMs             = glob->GetOptI("nMLMs");
  int     maxNobj           = glob->GetOptI("maxNobj");
  double  minValZ           = glob->GetOptF("minValZ");
  double  maxValZ           = glob->GetOptF("maxValZ");
  int     nObjectsToWrite   = glob->GetOptI("nObjectsToWrite");
  TString zTrgName          = glob->GetOptC("zTrg");
  TString aChainName        = (TString)aChain->GetName();

  int     nZclosBins        = glob->GetOptI("nZclosBins");
  if(nZclosBins <= 0) nZclosBins = 10;

  bool    optimWithMAD      = glob->GetOptB("optimWithMAD");
  TString quantScatterName  = optimWithMAD ? (TString)"quant_MAD" : (TString)"quant_sigma_68";
  TString sig68Title        = optimWithMAD ? (TString)"MAD"       : (TString)"sig68";

  bool    optimWithSclBias  = glob->GetOptB("optimWithScaledBias");
  TString biasTitle         = optimWithSclBias ? (TString)"bias/(1+"+glob->GetOptC("zTrgTitle")+")" : (TString)"bias";

  // -----------------------------------------------------------------------------------------------------------
  // if not defined, derive the closure bins from the quantile distribution of zTrg
  // -----------------------------------------------------------------------------------------------------------
  int nBinsZ = (int)zClos_binC.size();
  if(nBinsZ == 0) {
    nBinsZ =  nZclosBins;
    
    vector <double> fracV(nBinsZ-1), quantV(nBinsZ-1,-1);
    for(int nBinNow=0; nBinNow<nBinsZ-1; nBinNow++) {
      fracV[nBinNow] = (nBinNow + 1)/double(nBinsZ);
    }

    hisName   = (TString)aChain->GetName()+"_hisQuantTMP";
    drawExprs = (TString)zTrgName+">>"+hisName;
    wgtCut    = (TString)zTrgName+">="+utils->floatToStr(minValZ)+"&&"+zTrgName+"<="+utils->floatToStr(maxValZ);

    utils->drawTree(aChain,drawExprs,wgtCut);
    
    TH1 * hisQuantTMP = (TH1F*)gDirectory->Get(hisName);
    hisQuantTMP->SetDirectory(0); hisQuantTMP->BufferEmpty();

    utils->param->clearAll();
    bool gotQuants = utils->getQuantileV(fracV,quantV,hisQuantTMP);
    VERIFY(LOCATION,(TString)"Could not derive quantiles from "+aChain->GetName(),gotQuants);

    zClos_binC.resize(nBinsZ,0); zClos_binE.resize(nBinsZ+1,0);
    for(int nBinNow=1; nBinNow<nBinsZ+1; nBinNow++) {
      zClos_binE[nBinNow] = quantV[nBinNow-1];
    }
    zClos_binE[0]      = minValZ;
    zClos_binE[nBinsZ] = maxValZ;
    
    for(int nBinNow=0; nBinNow<nBinsZ; nBinNow++) {
      zClos_binC[nBinNow] = zClos_binE[nBinNow] + 0.5 * (zClos_binE[nBinNow+1] - zClos_binE[nBinNow]);
    }

    if(inLOG(Log::DEBUG)) {
      TString quantBins("");
      for(int nBinNow=0; nBinNow<nBinsZ+1; nBinNow++) {
        quantBins += utils->floatToStr(zClos_binE[nBinNow]) + TString((nBinNow < nBinsZ) ? "," : "");
      }
      aLOG(Log::DEBUG) <<coutWhiteOnBlack<<coutGreen<<" - setting "<<coutPurple<<nBinsZ<<coutGreen
                       <<" closure bins by quantiles: "<<coutYellow<<"["<<quantBins<<"]"<<coutDef<<endl;
    }

    DELNULL(hisQuantTMP);
    fracV.clear(); quantV.clear();
  }


  // -----------------------------------------------------------------------------------------------------------
  // 
  // -----------------------------------------------------------------------------------------------------------
  vector < double >        sumWeightsBin(nBinsZ,0);
  vector < vector <TH1*> > closH(nMLMs);

  int closHisN(glob->GetOptI("closHisN")), hisBufSize(glob->GetOptI("hisBufSize"));
  for(int nMLMnow=0; nMLMnow<nMLMs; nMLMnow++) {
    TString MLMname = getTagName(nMLMnow); if(mlmSkip[MLMname]) continue;

    closH[nMLMnow].resize(nBinsZ);
    for(int nBinNow=0; nBinNow<nBinsZ; nBinNow++) {
      hisName = TString::Format("TMPclosureHis_%d_"+MLMname,nBinNow);
      closH[nMLMnow][nBinNow] = new TH1F(hisName,hisName,closHisN,1,-1);
      closH[nMLMnow][nBinNow]->SetDefaultBufferSize(hisBufSize);
    }
  }

  VarMaps * var = new VarMaps(glob,utils,"treeRegVar");
  var->connectTreeBranches(aChain);

  // -----------------------------------------------------------------------------------------------------------
  // loop on the tree
  // -----------------------------------------------------------------------------------------------------------
  var->clearCntr(); breakLoop = false;
  for(Long64_t loopEntry=0; true; loopEntry++) {
    if(!var->getTreeEntry(loopEntry)) breakLoop = true;

    if((var->GetCntr("nObj")+1 % nObjectsToWrite == 0) || breakLoop) var->printCntr(aChainName,Log::DEBUG);
    if(breakLoop) break;
    
    double zTrg = var->GetVarF(zTrgName);
    for(int nMLMnow=0; nMLMnow<nMLMs; nMLMnow++) {
      TString MLMname   = getTagName(nMLMnow);           if(mlmSkip[MLMname]) continue;
      TString MLMname_w = getTagWeight(nMLMnow);

      double weightNow = var->GetVarF(MLMname_w);        if(weightNow < EPS)  continue;
      double regValNow = var->GetVarF(MLMname);
      int    zRegBinN  = getBinZ(regValNow,zClos_binE);  if(zRegBinN < 0)     continue;

      double sclBias(regValNow-zTrg);
      if(optimWithSclBias) {
        double denom = 1 + zTrg;
        if(fabs(denom) < EPS) sclBias  = DefOpts::DefF;
        else                  sclBias /= denom;
      }

      sumWeightsBin[zRegBinN] += weightNow;
      closH[nMLMnow][zRegBinN]->Fill(sclBias,weightNow);

      if(nMLMnow == 0) var->IncCntr("nObj with [weight > 0]");
    }

    var->IncCntr("nObj"); if(var->GetCntr("nObj") == maxNobj) breakLoop = true;
  }
  if(!breakLoop) { var->printCntr(aChainName,Log::DEBUG); }

  // ----------------------------------------------------------------------------------------------------------- 
  // calculate average metrics over MLMs for each of the Z-bins (0 <= nBinNow < nBinsZ) and over all bins of
  // for a given method (nBinNow == -1)
  // ----------------------------------------------------------------------------------------------------------- 
  zRegQnt_nANNZ.clear(); zRegQnt_bias.clear(); zRegQnt_sigma68.clear(); zRegQnt_fracSig68.clear();
  for(int nBinNow=-1; nBinNow<nBinsZ; nBinNow++) {
    zRegQnt_nANNZ  [nBinNow].reserve(nMLMs); zRegQnt_bias     [nBinNow].reserve(nMLMs);
    zRegQnt_sigma68[nBinNow].reserve(nMLMs); zRegQnt_fracSig68[nBinNow].reserve(nMLMs);
  }

  aLOG(Log::INFO) <<coutCyan<<" ---------------------------------------------------------------------------------- "<<coutDef<<endl;
  aLOG(Log::INFO) <<coutBlue<<" - Got the following average properties: "<<coutDef<<endl;
  aLOG(Log::INFO) <<coutCyan<<" --------------------------------------- "<<coutDef<<endl;
  for(int nMLMnow=0; nMLMnow<nMLMs; nMLMnow++) {
    TString MLMname = getTagName(nMLMnow); if(mlmSkip[MLMname]) continue;

    double  mean_bias(0), mean_sigma68(0), mean_fracSig68_2(0), mean_fracSig68_3(0), sumWeights(0), avgFracSig68(0);
    for(int nBinNow=0; nBinNow<nBinsZ; nBinNow++) {
      double  quant_mean(-1), quant_sigma_68(-1), quant_fracSig68_2(-1), quant_fracSig68_3(-1), quant_fracSig68_23(-1);

      utils->param->clearAll();
      utils->param->NewOptB("doFracLargerSigma" , true);
      utils->param->NewOptB("getMAD"            , optimWithMAD);
      int hasQnt = utils->getInterQuantileStats(closH[nMLMnow][nBinNow]);

      // if the calculation disnt go through (no entries) then the currect metrics are all negative
      if(hasQnt) {
        quant_mean          = fabs(utils->param->GetOptF("quant_mean"));  // all values must be positive for the optimization to work
        quant_sigma_68      = utils->param->GetOptF(quantScatterName);
        quant_fracSig68_2   = utils->param->GetOptF("quant_fracSig68_2");
        quant_fracSig68_3   = utils->param->GetOptF("quant_fracSig68_3");
        quant_fracSig68_23  = 0.5 * (quant_fracSig68_2 + quant_fracSig68_3);

        // update values for the average calculation
        sumWeights       += sumWeightsBin[nBinNow];
        mean_bias        += sumWeightsBin[nBinNow] * quant_mean;
        mean_sigma68     += sumWeightsBin[nBinNow] * quant_sigma_68;
        mean_fracSig68_2 += sumWeightsBin[nBinNow] * quant_fracSig68_2;  
        mean_fracSig68_3 += sumWeightsBin[nBinNow] * quant_fracSig68_3;

        // cout << closH[nMLMnow][nBinNow]->GetName()<<CT<< optimWithMAD <<CT<<quantScatterName <<CT
        //      <<utils->param->GetOptF(quantScatterName) <<CT<<utils->param->GetOptF("quant_sigma_68")<<endl;
      }
      // store the metrics in each Zbin
      zRegQnt_nANNZ    [nBinNow].push_back(nMLMnow);
      zRegQnt_bias     [nBinNow].push_back(quant_mean);
      zRegQnt_sigma68  [nBinNow].push_back(quant_sigma_68);
      zRegQnt_fracSig68[nBinNow].push_back(quant_fracSig68_23);
    }
    // store the average of the metrics
    if(sumWeights > 0) {
      mean_bias /= sumWeights; mean_sigma68 /= sumWeights; avgFracSig68 = 0.5 * (mean_fracSig68_2 + mean_fracSig68_3) / sumWeights;
    }
    else mean_bias = mean_sigma68 = avgFracSig68 = -1;

    zRegQnt_nANNZ    [-1].push_back(nMLMnow);
    zRegQnt_bias     [-1].push_back(mean_bias);
    zRegQnt_sigma68  [-1].push_back(mean_sigma68);
    zRegQnt_fracSig68[-1].push_back(avgFracSig68);

    aLOG(Log::INFO) <<coutCyan<<" - type/name,<"<<biasTitle<<">,<"<<sig68Title<<">,<fracSig68_2,3>:  "
                    <<coutYellow<<typeToNameMLM[typeMLM[nMLMnow]]<<coutCyan<<"/"<<coutYellow<<getTagName(nMLMnow)
                    <<CT<<coutPurple<<mean_bias<<CT<<coutGreen<<mean_sigma68<<coutBlue<<CT<<avgFracSig68<<coutDef<<endl;
  }
  aLOG(Log::INFO) <<coutCyan<<" ------------------------------------------------------------------------------------------------- "<<coutDef<<endl;

  // cleanup
  for(int nMLMnow=0; nMLMnow<nMLMs; nMLMnow++) {
    TString MLMname = getTagName(nMLMnow); if(mlmSkip[MLMname]) continue;
   
    for(int nBinNow=0; nBinNow<nBinsZ; nBinNow++) DELNULL(closH[nMLMnow][nBinNow]);
  }
  closH.clear(); sumWeightsBin.clear();
  DELNULL(var);

  return;
}


// ===========================================================================================================
/**
 * @brief                    - Find the "best" MLM solution in randomized regression.
 * 
 * @details                  - Find the "best" MLMs, given the three metrics: bias, sig68 and fracSig68,
 *                           where for optimCondReg (one of the three) the best several methods within the top fracLimNow0
 *                           percentile are chosen, so long as for the other two metrics, the MLMs are within
 *                           the top fracLimNow1 (fracLimNow1 ~> fracLimNow1+0.2) of the distribution of MLMs. This selects the solution
 *                           which gives the "best" of the selected metric, which is also not the "worst" of the other two.
 *             
 * @param zRegQnt_nANNZ      - Map of vector, which is filled with the index of MLMs, corresponding to the order of the
 *                           entries of the metrics in the other vectors (zRegQnt_bias, zRegQnt_sigma68 and zRegQnt_fracSig68).
 * @param zRegQnt_bias       - Map of vector, which is filled with the claculated bias metric.
 * @param zRegQnt_sigma68    - Map of vector, which is filled with the claculated sigma68 metric.
 * @param zRegQnt_fracSig68  - Map of vector, which is filled with the claculated combined outlier-fraction metric.
 * @param bestMLMsV          - Vector which is filled with the results of the search (ordered collection of "best" MLM solutions).
 * @param onlyInclusiveBin   - Flag to decide if to use the metrics which are claculated in bins of the target variable (zTrg),
 *                           or alternatively, to use the average metrics (over all zTrg bins).
*/
// ===========================================================================================================
void  ANNZ::getBestANNZ(
  map < int,vector<int> >    & zRegQnt_nANNZ,   map < int,vector<double> > & zRegQnt_bias,
  map < int,vector<double> > & zRegQnt_sigma68, map < int,vector<double> > & zRegQnt_fracSig68,
  vector < int >             & bestMLMsV,       bool                        onlyInclusiveBin
) {
// ===========================================================================================================
  aLOG(Log::INFO) <<coutWhiteOnBlack<<coutCyan<<" - starting ANNZ::getBestANNZ(optimCondReg=\""
                  <<coutYellow<<glob->GetOptC("optimCondReg")<<coutCyan<<"\") ... "<<coutDef<<endl;

  bestMLMsV.clear(); // before anything, clear the ouput vector

  int     minAcptANNZs       = glob->GetOptI("minAcptMLMsForPDFs");
  TString optimCondReg       = glob->GetOptC("optimCondReg");
  bool    optimWithFracSig68 = glob->GetOptB("optimWithFracSig68");
  TString zTrgTitle          = glob->GetOptC("zTrgTitle");
  TString optimCondTitle     = glob->GetOptB("optimWithMAD")        ? (TString)"MAD"                    : (TString)optimCondReg;
  TString biasTitle          = glob->GetOptB("optimWithScaledBias") ? (TString)"bias/(1+"+zTrgTitle+")" : (TString)"bias";

  // if not choosing to optimize by the outlier fraction, don't use it at all (nMetrics set to 2)
  int nMetrics = (optimCondReg == "fracSig68" || optimWithFracSig68) ? 3 : 2;
  if(nMetrics == 2) {
    aLOG(Log::INFO) <<coutBlue<<" - Will optimize based on bias and sig68 only (priority on "
                    <<coutGreen<<optimCondReg<<coutBlue<<")"<<coutDef<<endl;
  }

  // -----------------------------------------------------------------------------------------------------------
  // if too few MLMs are available, do a simple ordering by the nominal metric
  // -----------------------------------------------------------------------------------------------------------
  vector< pair<int,double> > nMLMmetricPairV;

  for(int nAcptANNZnow=0; nAcptANNZnow<(int)zRegQnt_nANNZ[-1].size(); nAcptANNZnow++) {
    if(zRegQnt_sigma68[-1][nAcptANNZnow] < EPS) continue; // has been initialized, such that no entries in this bin

    int    nMLMnow   = zRegQnt_nANNZ [-1][nAcptANNZnow];
    double metricNow = 0;
    if     (optimCondReg == "bias")      metricNow = zRegQnt_bias     [-1][nAcptANNZnow];
    else if(optimCondReg == "sig68")     metricNow = zRegQnt_sigma68  [-1][nAcptANNZnow];
    else if(optimCondReg == "fracSig68") metricNow = zRegQnt_fracSig68[-1][nAcptANNZnow];
    else VERIFY(LOCATION,(TString)"Configuration problem... \"optimCondReg\" should have one of the "+
                                  "following values: \"sig68\", \"bias\" or \"fracSig68\" options ...",false);

    nMLMmetricPairV.push_back( pair<int,double>(nMLMnow,metricNow) );
  }
  if((int)nMLMmetricPairV.size() < minAcptANNZs) {
    // sort so that the smallest element is first
    sort(nMLMmetricPairV.begin(),nMLMmetricPairV.end(),sortFunc::pairID::lowToHighBy1);

    for(int nAcptANNZnow=0; nAcptANNZnow<(int)nMLMmetricPairV.size(); nAcptANNZnow++) {
      bestMLMsV.push_back(nMLMmetricPairV[nAcptANNZnow].first);
    }
  }
  nMLMmetricPairV.clear();

  if((int)bestMLMsV.size() > 0) return;


  // -----------------------------------------------------------------------------------------------------------
  // find the "best" MLM, which has an "optimal" combination of good metrics
  // -----------------------------------------------------------------------------------------------------------
  int nAcptBestMLMs(0), nfracLims0(30), nfracLims1(6), minNoptimMLMs(1), nBinsZ(-1);
  for(map < int,vector<int> >::iterator Itr=zRegQnt_nANNZ.begin(); Itr!=zRegQnt_nANNZ.end(); ++Itr) { nBinsZ++; }
    
  map < int,vector<int> >                               nANNZv;
  map < int,vector<double> >                            biasV, sigma68V, fracSig68V;
  map < int,map< TString,vector<int> > >                bestMLMs;
  map < int,map<TString,vector< pair<int,double> > > >  bestMLMsPairs;

  bool foundBest(false);
  for(int nfracLimNow1=0; nfracLimNow1<nfracLims1; nfracLimNow1++) {
    for(int nfracLimNow0=0; nfracLimNow0<nfracLims0; nfracLimNow0++) {
      if(foundBest) break;

      double fracLimNow0 = 0.1         + 0.025 *      nfracLimNow0 ; fracLimNow0 = min(fracLimNow0,0.8);
      double fracLimNow1 = fracLimNow0 + 0.1   * (1 + nfracLimNow1); fracLimNow1 = min(fracLimNow1,0.8);

      aLOG(Log::INFO) <<coutRed<<" - Now trying to find best methods with a limit of "<<coutYellow<<fracLimNow0*100<<coutRed
                      <<"\% of the ["<<optimCondTitle<<"] dist, and "<<coutYellow<<fracLimNow1*100<<coutRed<<"\% of the other(s)"
                      <<" - iteration "<<coutYellow<<(nfracLimNow0+nfracLimNow1*nfracLims0)<<coutRed<<" ..."<<coutDef<<endl;

      nANNZv.clear(); biasV.clear(); sigma68V.clear(); fracSig68V.clear();

      for(int nBinNow=-1; nBinNow<nBinsZ; nBinNow++) {
        for(int nAcptANNZnow=0; nAcptANNZnow<(int)zRegQnt_nANNZ[nBinNow].size(); nAcptANNZnow++) {
          if(zRegQnt_sigma68[nBinNow][nAcptANNZnow] < EPS) continue; // has been initialized, such that no entries in this bin

          nANNZv    [nBinNow].push_back(zRegQnt_nANNZ    [nBinNow][nAcptANNZnow]);
          biasV     [nBinNow].push_back(zRegQnt_bias     [nBinNow][nAcptANNZnow]);
          sigma68V  [nBinNow].push_back(zRegQnt_sigma68  [nBinNow][nAcptANNZnow]);
          fracSig68V[nBinNow].push_back(zRegQnt_fracSig68[nBinNow][nAcptANNZnow]);
        }
      }

      bestMLMsPairs.clear();
      nAcptBestMLMs = 0;
      for(int nBinNow=-1; nBinNow<nBinsZ; nBinNow++) {
        if(onlyInclusiveBin && nBinNow == 0) break;

        map < TString, vector<double> > metricLowQuants;
        for(int nMetricNow=0; nMetricNow<nMetrics; nMetricNow++) {
          TString         metricName("");
          vector <double> fracV, quantV(2,-1);
          fracV.push_back(fracLimNow0); fracV.push_back(fracLimNow1);

          int hasQnt(0);
          utils->param->clearAll();
          if     (nMetricNow == 0) { hasQnt = utils->getQuantileV(fracV,quantV,biasV     [nBinNow]); metricName = "bias";      }
          else if(nMetricNow == 1) { hasQnt = utils->getQuantileV(fracV,quantV,sigma68V  [nBinNow]); metricName = "sig68";     }
          else if(nMetricNow == 2) { hasQnt = utils->getQuantileV(fracV,quantV,fracSig68V[nBinNow]); metricName = "fracSig68"; }
          
          double  quantLow = hasQnt ? quantV[0] : -1;
          double  quantDif = hasQnt ? quantV[1] : -1;
          // double  quantDif = hasQnt ? quantLow + (quantV[2] - quantV[1])/2. : -1;
          
          metricLowQuants[metricName].push_back(quantLow);
          metricLowQuants[metricName].push_back(quantDif);

          aLOG(Log::DEBUG_2)<<coutYellow<<" - getQuantileV("<<nBinNow<<","<<metricName<<"): "
                            <<CT<<coutGreen <<"["<<fracV[0]<<","<<quantLow<<","<<quantDif<<"]"<<coutDef<<endl;

          fracV.clear(); fracV.clear();
        }

        int nAcptANNZs = (int)nANNZv[nBinNow].size();
        for(int nAcptANNZnow=0; nAcptANNZnow<nAcptANNZs; nAcptANNZnow++) {
          int     nMLMnow        = nANNZv    [nBinNow][nAcptANNZnow];
          double  mean_bias      = biasV     [nBinNow][nAcptANNZnow];
          double  mean_sigma68   = sigma68V  [nBinNow][nAcptANNZnow];
          double  mean_fracSig68 = fracSig68V[nBinNow][nAcptANNZnow];

          // make sure all the quantile calculations for this bin which are OK
          bool skip(false);
          for(int nEle=0; nEle<2; nEle++) { 
            for(int nMetricNow=0; nMetricNow<nMetrics; nMetricNow++) {
              TString metricName("");
              if     (nMetricNow == 0) metricName = "bias";
              else if(nMetricNow == 1) metricName = "sig68";
              else if(nMetricNow == 2) metricName = "fracSig68";
              
              if(metricLowQuants[metricName][nEle] < 0) skip = true;
            }
          }
          if(skip) continue;

          // -----------------------------------------------------------------------------------------------------------
          // check which MLMs passed the required thresholds
          // -----------------------------------------------------------------------------------------------------------
          double              frac_bias(0), frac_sig68(0), frac_fracSig68(0);
          map <TString,bool>  hasMetric;

          // check if "bias" is within the top fracLimNow0 percentile, as well as that "sig68","fracSig68" are within fracLimNow1
          // -----------------------------------------------------------------------------------------------------------
          frac_bias  = metricLowQuants["bias"] [0];
          frac_sig68 = metricLowQuants["sig68"][1];
          if(nMetrics == 3) frac_fracSig68 = metricLowQuants["fracSig68"][1];
          else              frac_fracSig68 = mean_fracSig68+1;
          
          if(mean_bias < frac_bias && mean_sigma68 < frac_sig68 && mean_fracSig68 < frac_fracSig68) {
            hasMetric["bias"] = true;
            bestMLMsPairs[nBinNow]["bias"].push_back(pair<int,double>(nMLMnow,mean_bias));
          }
          
          // check if "sig68" is within the top fracLimNow0 percentile, as well as that "bias","fracSig68" are within fracLimNow1
          // -----------------------------------------------------------------------------------------------------------
          frac_bias  = metricLowQuants["bias"] [1];
          frac_sig68 = metricLowQuants["sig68"][0];
          if(nMetrics == 3) frac_fracSig68 = metricLowQuants["fracSig68"][1];
          else              frac_fracSig68 = mean_fracSig68+1;

          if(mean_bias < frac_bias && mean_sigma68 < frac_sig68 && mean_fracSig68 < frac_fracSig68) {
            hasMetric["sig68"] = true;
            bestMLMsPairs[nBinNow]["sig68"].push_back(pair<int,double>(nMLMnow,mean_sigma68));
          }
          
          // check if "fracSig68" is within the top fracLimNow0 percentile, as well as that "bias","sig68" are within fracLimNow1
          // -----------------------------------------------------------------------------------------------------------
          if(nMetrics == 3) {
            frac_bias      = metricLowQuants["bias"]     [1];
            frac_sig68     = metricLowQuants["sig68"]    [1];
            frac_fracSig68 = metricLowQuants["fracSig68"][0];
            if(mean_bias < frac_bias && mean_sigma68 < frac_sig68 && mean_fracSig68 < frac_fracSig68) {
              hasMetric["fracSig68"] = true;
              bestMLMsPairs[nBinNow]["fracSig68"].push_back(pair<int,double>(nMLMnow,mean_fracSig68));
            }
          }

          if(nBinNow == -1) { if(hasMetric[optimCondReg]) nAcptBestMLMs++; }

          TString hasAll = TString(hasMetric["bias"]?(TString)biasTitle+" ":"")+TString(hasMetric["sig68"]?(TString)optimCondTitle+" ":"")
                          +TString(hasMetric["fracSig68"]?"fracSig68 ":"");
          if(hasAll != "") aLOG(Log::DEBUG) <<coutGreen<<" - Success for nBinZ,nMLMnow = "<<nBinNow<<","
                                            <<nMLMnow<<" -> "<<biasTitle<<","<<optimCondTitle<<",fracSig68 = "<<coutPurple
                                            <<mean_bias<<CT<<mean_sigma68<<CT<<mean_fracSig68<<coutYellow<<"\t -> "<<hasAll<<coutDef<<endl;
          hasMetric.clear();
        }
        metricLowQuants.clear();
      }

      // -----------------------------------------------------------------------------------------------------------
      // now sort the results be smallest metric value and store in bestMLMs
      // -----------------------------------------------------------------------------------------------------------
      bestMLMs.clear();
      for(int nBinNow=-1; nBinNow<nBinsZ; nBinNow++) {
        for(int nMetricNow=0; nMetricNow<nMetrics; nMetricNow++) {
          TString metricName("");
          if     (nMetricNow == 0) metricName = "bias";
          else if(nMetricNow == 1) metricName = "sig68";
          else if(nMetricNow == 2) metricName = "fracSig68";
        
          int nAcptANNZs = (int)bestMLMsPairs[nBinNow][metricName].size();
          if(nAcptANNZs == 0) continue;

          // sort so that the smallest element is first
          sort(bestMLMsPairs[nBinNow][metricName].begin(),bestMLMsPairs[nBinNow][metricName].end(),sortFunc::pairID::lowToHighBy1); 

          for(int nAcptANNZnow=0; nAcptANNZnow<nAcptANNZs; nAcptANNZnow++) {
            bestMLMs[nBinNow][metricName].push_back(bestMLMsPairs[nBinNow][metricName][nAcptANNZnow].first);

            aLOG(Log::DEBUG_2)<<coutPurple<<" - accepted list("<<nBinNow<<","<<nAcptANNZnow<<","<<metricName<<"): "
                              <<CT<<coutGreen<<bestMLMsPairs[nBinNow][metricName][nAcptANNZnow].first
                              <<CT<<coutRed<<bestMLMsPairs[nBinNow][metricName][nAcptANNZnow].second<<coutDef<<endl;
          }
          
        }
      }

      // check the number of chosen 'best' methods and stop if there are enough
      if(nAcptBestMLMs >= minNoptimMLMs || (nfracLimNow0 == nfracLims0-1 && nfracLimNow1 == nfracLims1-1)) {
        aLOG(Log::INFO) <<coutGreen<<" - Found "<<nAcptBestMLMs<<" \"best\" methods (min-threshold was "
                        <<minNoptimMLMs<<") from the bottom "<<fracLimNow0*100<<"\% of the ["<<optimCondTitle<<"] dist."<<coutDef<<endl;
        foundBest = true;
      }
    }
  }
  VERIFY(LOCATION,(TString)"Could not find any accepted MLMs which pass minimal metric-cuts ... Something is horribly wrong !!!",(nAcptBestMLMs > 0));
  
  // store the result in the vector which is accessed from outside the function
  bestMLMsV = bestMLMs[-1][optimCondReg];

  // cleanup
  nANNZv.clear(); biasV.clear(); sigma68V.clear(); fracSig68V.clear(); bestMLMsPairs.clear(); bestMLMs.clear();

  return;
}


// ===========================================================================================================
/**
 * @brief                    - Generate a PDF weighting scheme for randomized regression using a simple random walk alg.
 * 
 * @details                  - Candidate PDFs are created by randomely generating weighting schemes, which represent different
 *                           supression powers af MLM, based on metric ranking: MLMs with better (smaller values of) metrics, have
 *                           larger relative weights.
 *                           - PDFs are selected by choosing the weighting scheme which is "most compatible" with the true value.
 *                           This is determined in two ways (generating two alternative PDFs), using cumulative distributions; for one
 *                           PDF, the cumulative distribution is based on the "truth" (the target variable, zTrg). For the other
 *                           PDF, the cumulative distribution is based on the "best" MLM.
 *                           For the former, a set of templates, derived from zTrg is used to fit the dataset. For the later,
 *                           a flat distribution of the cumulator serves as the baseline.
 *                           
 * @param aChain             - Input chain, for which to do the PDF derivation.
 * @param bestANNZindex      - Index of the "best" MLM.
 * @param zRegQnt_nANNZ      - Map of vector, which is filled with the index of MLMs, corresponding to the order of the
 *                           entries of the metrics in the other vectors (zRegQnt_sigma68 and zRegQnt_bias).
 * @param zRegQnt_bias       - Map of vector, which is filled with the claculated bias metric.
 * @param zRegQnt_sigma68    - Map of vector, which is filled with the claculated sigma68 metric.
 * @param zRegQnt_fracSig68  - Map of vector, which is filled with the claculated combined outlier-fraction metric.
 * @param bestWeightsV       - Vector into which the results of the PDF derivation are stored (as weights for
 *                           each MLM).
 * @param hisPdfBiasCorV     - Vector for bias correction histograms for pdfs.
 */
// ===========================================================================================================
void  ANNZ::getRndMethodBestPDF(
  TTree                     * aChain,          int            bestANNZindex,
  vector<int>               & zRegQnt_nANNZ,   vector<double> & zRegQnt_bias,
  vector<double>            & zRegQnt_sigma68, vector<double> & zRegQnt_fracSig68, 
  vector < vector<double> > & bestWeightsV,    vector <TH2*>  & hisPdfBiasCorV
) {
// ===========================================================================================================
  aLOG(Log::INFO) <<coutWhiteOnBlack<<coutYellow<<" - starting ANNZ::getRndMethodBestPDF() ... "<<coutDef<<endl;
  VERIFY(LOCATION,(TString)"Memory leak ?! ",(dynamic_cast<TChain*>(aChain)));

  TString zTrgName           = glob->GetOptC("zTrg");
  int     maxNobj            = glob->GetOptI("maxNobj");
  int     nSmearsRnd         = glob->GetOptI("nSmearsRnd");
  int     nMLMs              = glob->GetOptI("nMLMs");
  int     nPDFbins           = glob->GetOptI("nPDFbins");
  TString _typeANNZ          = glob->GetOptC("_typeANNZ");
  UInt_t  seed               = glob->GetOptI("initSeedRnd"); if(seed > 0) seed += 98187;
  int     minAcptMLMsForPDFs = glob->GetOptI("minAcptMLMsForPDFs");
  int     nPDFs              = glob->GetOptI("nPDFs");
  double  max_sigma68_PDF    = glob->GetOptF("max_sigma68_PDF");
  double  max_bias_PDF       = glob->GetOptF("max_bias_PDF");
  double  max_frac68_PDF     = glob->GetOptF("max_frac68_PDF");
  TString zTrgTitle          = glob->GetOptC("zTrgTitle");
  bool    doBiasCorPDF       = glob->GetOptB("doBiasCorPDF");
  TString intgrZtreeName     = (TString)glob->GetOptC("treeName")+"_intgrZmlm";
  
  // parameters for the random walk alg
  double  max_optimObj_PDF   = (double)glob->GetOptI("max_optimObj_PDF");
  int     nOptimLoops        = max(100, glob->GetOptI("nOptimLoops"));
  int     nIntgrZsmears      = 15; // sould be odd number >= 3
  int     maxOptimTries      = glob->GetOptI("max_staticOptimTries_PDF");
  int     nSameWeightsMax    = int(floor(0.5 * maxOptimTries));
  int     nNoUpdateMax       = 50;
  double  fracWeightUpdate   = 0.15;
  bool    saveProfileHis     = inLOG(Log::DEBUG_1);

  vector < double > excRange(2);
  excRange[0] = glob->GetOptF("excludeRangePdfModelFit");
  excRange[1] = 1 - excRange[0];

  int     nBinsZ        = (int)zClos_binC.size();
  TString MLMnameBest   = getTagName(bestANNZindex);
  TString MLMnameBest_e = getTagError(bestANNZindex);

  if(nPDFs == 0) {
    aLOG(Log::INFO) <<coutGreen<<" - no PDFs requested - nothing to do here... "<<coutDef<<endl;
    return;
  }
  VERIFY(LOCATION,(TString)"zClos_binC not properly initialized ?!? ",(nBinsZ > 0)); //sanity check

  // -----------------------------------------------------------------------------------------------------------
  // define bias-correction histograms for the pdf
  // -----------------------------------------------------------------------------------------------------------
  TH2F * hisPdfTryBiasCorV(NULL);
  if(doBiasCorPDF) {
    TString hisName   = (TString)"pdfBias"+_typeANNZ+"_nominal";
    hisPdfTryBiasCorV = new TH2F(hisName,hisName,nPDFbins,&(zPDF_binE[0]),nPDFbins,&(zPDF_binE[0]));
  }
  
  TRandom                     * rnd(NULL);
  vector < int >              sigmToBiasV(nMLMs,0);
  vector < double >           weightV(nMLMs,0);
  vector < pair<int,double> > sigm68V, biasV;

  // -----------------------------------------------------------------------------------------------------------
  // sort all accepted MLMs by their sigm68 and bias metrics.
  // -----------------------------------------------------------------------------------------------------------
  int nAcptMLMs = (int)zRegQnt_nANNZ.size();
  for(int nAcptMLMnow=0; nAcptMLMnow<nAcptMLMs; nAcptMLMnow++) {
    if(zRegQnt_sigma68[nAcptMLMnow] < 0) continue; // has been initialized, such that no entries in this bin

    // check if the metrics are not above threshold (if defined by the user)
    if(max_sigma68_PDF > 0 && zRegQnt_sigma68[nAcptMLMnow] > max_sigma68_PDF) {
     aLOG(Log::INFO) <<coutRed<<" - "<<coutYellow<<getTagName(zRegQnt_nANNZ[nAcptMLMnow])<<coutRed<<" has sigma68 = "
                     <<coutYellow<<zRegQnt_sigma68[nAcptMLMnow]<<coutRed<<" which is above threshold ("<<coutBlue<<max_sigma68_PDF
                     <<coutRed<<") -> it will be rejected from the PDF ..." <<coutDef<<endl;
     continue;
    }
    if(max_bias_PDF > 0 && zRegQnt_bias[nAcptMLMnow] > max_bias_PDF) {
      aLOG(Log::INFO) <<coutRed<<" - "<<coutYellow<<getTagName(zRegQnt_nANNZ[nAcptMLMnow])<<coutRed<<" has bias = "
                      <<coutYellow<<zRegQnt_bias[nAcptMLMnow]<<coutRed<<" which is above threshold ("<<coutBlue<<max_bias_PDF
                      <<coutRed<<") -> it will be rejected from the PDF ..." <<coutDef<<endl;
      continue;
    }
    if(max_frac68_PDF > 0 && zRegQnt_fracSig68[nAcptMLMnow] > max_frac68_PDF) {
      aLOG(Log::INFO) <<coutRed<<" - "<<coutYellow<<getTagName(zRegQnt_nANNZ[nAcptMLMnow])<<coutRed<<" has combined outlier-fraction = "
                      <<coutYellow<<zRegQnt_fracSig68[nAcptMLMnow]<<coutRed<<" which is above threshold ("<<coutBlue<<max_frac68_PDF
                      <<coutRed<<") -> it will be rejected from the PDF ..." <<coutDef<<endl;
      continue;
    }

    biasV  .push_back(pair<int,double>(zRegQnt_nANNZ[nAcptMLMnow],zRegQnt_bias   [nAcptMLMnow]));
    sigm68V.push_back(pair<int,double>(zRegQnt_nANNZ[nAcptMLMnow],zRegQnt_sigma68[nAcptMLMnow]));
  }
  nAcptMLMs = (int)sigm68V.size();
  
  VERIFY(LOCATION,(TString)" - found only "+utils->intToStr(nAcptMLMs)+" accepted MLMs, but requested minAcptMLMsForPDFs = "
                          +utils->intToStr(minAcptMLMsForPDFs)+" ...",(nAcptMLMs >= minAcptMLMsForPDFs));

  // sort so that the smallest element is first
  sort(biasV  .begin(),biasV  .end(),sortFunc::pairID::lowToHighBy1);
  sort(sigm68V.begin(),sigm68V.end(),sortFunc::pairID::lowToHighBy1);

  // get the index conversion between the two vectors after their independent sorts
  for(int nAcptMLMnow=0; nAcptMLMnow<nAcptMLMs; nAcptMLMnow++) {
    int nMLMnow = biasV[nAcptMLMnow].first;
    sigmToBiasV[nMLMnow] = nAcptMLMnow;
  }
  // sanity check (should never be triggered)
  for(int nAcptMLMnow=0; nAcptMLMnow<nAcptMLMs; nAcptMLMnow++) {
    bool isSameMLM = (biasV[sigmToBiasV[sigm68V[nAcptMLMnow].first]].first == sigm68V[nAcptMLMnow].first);
    VERIFY(LOCATION,(TString)" - mismatch between MLM indices ... ", isSameMLM);
  }


  // -----------------------------------------------------------------------------------------------------------
  // make sure we have enough MLMs for a pdf:
  // - reject worst ??% out of hand, demanding that enough MLMs have both good sigm68 and good bias metrics
  // - make sure what remains is at least minAcptMLMsForPDFs methods
  // -----------------------------------------------------------------------------------------------------------
  if(seed > 0) { seed++; } rnd = new TRandom3(seed);
  
  int             nNonZeroWeights(0);
  bool            foundGoodSetup(false);
  map <int,bool>  acceptV_sigm, acceptV_both;

  for(int nRelaxReject=0; nRelaxReject<100; nRelaxReject++) {
    int     minRejConst_sigm(1), maxRejConst_sigm(6), minRejConst_bias(0), maxRejConst_bias(3);
    int     maxAcptMLMs(0);
    double  rndNow(0), rejectionFrac(0);

    if(nRelaxReject > 0) {
      maxRejConst_sigm -= 0.5 * nRelaxReject;
      maxRejConst_bias -= 0.5 * nRelaxReject;
      if(maxRejConst_sigm <= minRejConst_sigm) break;
      if     (maxRejConst_bias <= minRejConst_bias && maxRejConst_sigm <= minRejConst_sigm) break;
      else if(maxRejConst_bias <= minRejConst_bias                                        ) maxRejConst_bias = minRejConst_bias + 1;

      aLOG(Log::DEBUG_2) <<coutLightBlue<<"("<<nRelaxReject
                         <<") Reduced rejectionFrac for sigma,bias to: "<<maxRejConst_sigm<<" , "<<maxRejConst_bias<<coutDef<<endl;
    }
    
    // initialize the selection lists
    for(int nMLMnow=0; nMLMnow<nMLMs; nMLMnow++) {
      acceptV_sigm[nMLMnow] = false; acceptV_both[nMLMnow] = false;
    }


    // first check accepted methods by minimal sigma_68
    // -----------------------------------------------------------------------------------------------------------
    for(int nRjctFarcNow=0; nRjctFarcNow<100; nRjctFarcNow++) {
      rndNow        = rnd->Rndm();
      rejectionFrac = 0.1 * (minRejConst_sigm + int(ceil(rndNow * (maxRejConst_sigm-minRejConst_sigm))));
      maxAcptMLMs   = static_cast<int>(floor(nAcptMLMs * (1-rejectionFrac)));
      
      // set the flag for accepted/rejected methods
      nNonZeroWeights = 0;
      for(int nAcptMLMnow=0; nAcptMLMnow<nAcptMLMs; nAcptMLMnow++) {
        int nMLMnow = sigm68V[nAcptMLMnow].first;
        
        if(nAcptMLMnow > maxAcptMLMs) {
          acceptV_sigm[nMLMnow] = false;
          aLOG(Log::DEBUG_3) <<coutRed<<"Reject SIGM for "<<nAcptMLMnow<<CT<<nMLMnow<<coutDef<<endl;
        }
        else {
          acceptV_sigm[nMLMnow] = true;
          nNonZeroWeights++;
          aLOG(Log::DEBUG_3) <<coutBlue<<"Accept SIGM for "<<nAcptMLMnow<<CT<<nMLMnow<<coutDef<<endl;
        }
      }
      // check the number of accepted methods after the rejection cuts
      aLOG(Log::DEBUG_2) <<coutYellow<<"SIGM - nRjctFarcNow,rejectionFrac,maxAcptMLMs,nNonZeroWeights - "<<nRjctFarcNow<<CT
                         <<rejectionFrac<<CT<<maxAcptMLMs<<CT<<nNonZeroWeights<<coutDef<<endl;
      
      if(nNonZeroWeights >= minAcptMLMsForPDFs) {
        foundGoodSetup = true; break;
      }
    }

    // check the accepted methods by minimal bias (requiring to already also be accepted by sigma_68)
    // -----------------------------------------------------------------------------------------------------------
    if(foundGoodSetup) {
      foundGoodSetup = false;

      for(int nRjctFarcNow=0; nRjctFarcNow<100; nRjctFarcNow++) {
        rndNow        = rnd->Rndm();
        rejectionFrac = 0.1 * (minRejConst_bias + int(ceil(rndNow * (maxRejConst_bias-minRejConst_bias))));
        maxAcptMLMs   = static_cast<int>(floor(nAcptMLMs * (1-rejectionFrac)));
        
        // set the flag for accepted/rejected methods
        nNonZeroWeights = 0;
        for(int nAcptMLMnow=0; nAcptMLMnow<nAcptMLMs; nAcptMLMnow++) {
          int nMLMnow = biasV[nAcptMLMnow].first;
          
          if(nAcptMLMnow > maxAcptMLMs) {
            acceptV_both[nMLMnow] = false;
            aLOG(Log::DEBUG_3) <<coutRed<<"Reject BIAS for "<<nAcptMLMnow<<CT<<nMLMnow<<coutDef<<endl;
          }
          else {
            if(acceptV_sigm[nMLMnow]) {
              acceptV_both[nMLMnow] = true;
              nNonZeroWeights++;
              aLOG(Log::DEBUG_3) <<coutBlue<<"Accept BIAS for "<<nAcptMLMnow<<CT<<nMLMnow<<coutDef<<endl;
            }
            else {
              acceptV_both[nMLMnow] = false;
              aLOG(Log::DEBUG_3) <<coutPurple<<"Reject SIGM for "<<nAcptMLMnow<<CT<<nMLMnow<<coutDef<<endl;
            }
          }
        }
        // check the number of accepted methods after the rejection cuts
        aLOG(Log::DEBUG_2) <<coutYellow<<"BIAS - nRjctFarcNow,rejectionFrac,maxAcptMLMs,nNonZeroWeights - "<<nRjctFarcNow<<CT
                           <<rejectionFrac<<CT<<maxAcptMLMs<<CT<<nNonZeroWeights<<coutDef<<endl;
        
        if(nNonZeroWeights >= minAcptMLMsForPDFs) {
          foundGoodSetup = true;
          break;
        }
      }
    }

    if(foundGoodSetup) break;
    
    // if didn't break, than could not find a combination of MLMs with good metrics under the current restrictions
    aLOG(Log::DEBUG_1) <<coutRed<<"Cant find a rejectionFrac which leavs "<<minAcptMLMsForPDFs
                       <<" accepted methods... Will try to relax fraction-cuts."<<coutDef<<endl;
  }
  // shouldn't happen...
  VERIFY(LOCATION,(TString)" --- Cant find a rejectionFrac which leavs "+utils->intToStr(minAcptMLMsForPDFs)
                           +" accepted methods ... Something is horribly wrong !!! Try setting a lower value"
                           +" of minAcptMLMsForPDFs or increasing the number of trained MLMs ...",foundGoodSetup);


  map < TString,bool > mlmSkipPdf;
  for(int nMLMnow=0; nMLMnow<nMLMs; nMLMnow++) {
    TString MLMname = getTagName(nMLMnow);
    if(mlmSkip[MLMname]) {
      mlmSkipPdf[MLMname] = true;
      
      VERIFY(LOCATION,(TString)" - got inconsistent MLM skip criteria ... Something is horribly wrong !!! ",(!acceptV_both[nMLMnow]));
    }
    else {
      mlmSkipPdf[MLMname] = !acceptV_both[nMLMnow];
    }
  }

  DELNULL(rnd);
  acceptV_sigm.clear(); acceptV_both.clear();


  // -----------------------------------------------------------------------------------------------------------
  // finally, set the initial weights based on the accepted methods by both sigma and bias
  // -----------------------------------------------------------------------------------------------------------
  double norm_bias(0), norm_sigm68(0);
  for(int nAcptMLMnow=0; nAcptMLMnow<nAcptMLMs; nAcptMLMnow++) {
    int     nMLMnow = sigm68V[nAcptMLMnow].first;
    TString MLMname = getTagName(nMLMnow);  if(mlmSkipPdf[MLMname]) continue;

    double bias   = biasV[sigmToBiasV[nMLMnow]].second;
    double sigm68 = sigm68V[nAcptMLMnow].second;

    norm_bias   += pow(bias  , 2);
    norm_sigm68 += pow(sigm68, 2);
  }

  // -----------------------------------------------------------------------------------------------------------
  // derive weights based on a combination of the bias and scatter
  // -----------------------------------------------------------------------------------------------------------
  double sumWeights(0);
  weightV.resize(nMLMs,0);
  for(int nAcptMLMnow=0; nAcptMLMnow<nAcptMLMs; nAcptMLMnow++) {
    int     nMLMnow = sigm68V[nAcptMLMnow].first;
    TString MLMname = getTagName(nMLMnow);  if(mlmSkipPdf[MLMname]) continue;

    double bias      = biasV[sigmToBiasV[nMLMnow]].second;
    double sigm68    = sigm68V[nAcptMLMnow].second;
    double weightNow = 0.5 * (pow(bias, 2)/norm_bias + pow(sigm68, 2)/norm_sigm68);

    weightV[nMLMnow] = weightNow;
    sumWeights += weightV[nMLMnow];
  }

  // -----------------------------------------------------------------------------------------------------------
  // normalize the weights, where higher bias/scatter get smaller weights
  // -----------------------------------------------------------------------------------------------------------
  vector < pair<int,double> > tmpWeightV;
  for(int nAcptMLMnow=0; nAcptMLMnow<nAcptMLMs; nAcptMLMnow++) {
    int     nMLMnow = sigm68V[nAcptMLMnow].first;
    TString MLMname = getTagName(nMLMnow);  if(mlmSkipPdf[MLMname]) continue;

    double weightNow = 1 - weightV[nMLMnow]/sumWeights;
    tmpWeightV.push_back(pair<int,double>(nMLMnow,weightNow));
  }
  int nAcptWgts = (int)tmpWeightV.size();

  // sort so that the largest element is first
  sort(tmpWeightV.begin(),tmpWeightV.end(),sortFunc::pairID::highToLowBy1);

  // now that we have the right ranking of MLM weiights, set the values to by fixed scale
  sumWeights = 0;
  weightV.resize(nMLMs,0);
  for(int nAcptMLMnow=0; nAcptMLMnow<nAcptWgts; nAcptMLMnow++) {
    int nMLMnow = tmpWeightV[nAcptMLMnow].first;
    
    weightV[nMLMnow] = (nAcptWgts - nAcptMLMnow);
    sumWeights += weightV[nMLMnow];
  }

  for(int nMLMnow=0; nMLMnow<nMLMs; nMLMnow++) { weightV[nMLMnow] /= sumWeights; }

  // -----------------------------------------------------------------------------------------------------------
  // some output for the user, detailing the accepted MLMs and their initial weights
  // -----------------------------------------------------------------------------------------------------------
  TString acceptedMLMs(""), rejectedMLMs("");
  for(int nMLMnow=0; nMLMnow<nMLMs; nMLMnow++) {
    if(weightV[nMLMnow] > EPS) {
      acceptedMLMs += (TString)coutPurple+getTagName(nMLMnow)+coutGreen+", ";
    }
    else {
      rejectedMLMs += (TString)coutYellow+getTagName(nMLMnow)+coutGreen+", ";
    }
  }
  aLOG(Log::DEBUG) <<coutGreen<<" - accepted MLMs: "<<acceptedMLMs<<coutDef<<endl;
  aLOG(Log::DEBUG) <<coutGreen<<" - rejected MLMs: "<<rejectedMLMs<<coutDef<<endl;

  tmpWeightV.clear();
  for(int nMLMnow=0; nMLMnow<nMLMs; nMLMnow++) {
    if(weightV[nMLMnow] < EPS) continue;
    tmpWeightV.push_back(pair<int,double>(nMLMnow,weightV[nMLMnow]));
  }
  
  // sort so that the largest element is first
  sort(tmpWeightV.begin(),tmpWeightV.end(),sortFunc::pairID::highToLowBy1);

  TString weightMsg("");
  for(int nAcptMLMnow=0; nAcptMLMnow<(int)tmpWeightV.size(); nAcptMLMnow++) {
    TString MLMname = getTagName(tmpWeightV[nAcptMLMnow].first);
    weightMsg += (TString)coutGreen+MLMname+":"+coutYellow+utils->floatToStr(tmpWeightV[nAcptMLMnow].second,"%1.3f")+" ";
  }
  aLOG(Log::INFO) <<coutPurple<<" - initial PDF weights: " <<coutPurple<<weightMsg<<coutDef<<endl;
  aLOG(Log::DEBUG) <<coutGreen<<" ----------------------------------------------------------------------------- "<<coutDef<<endl;


  // -----------------------------------------------------------------------------------------------------------
  // loop on the input trees, claculate the integral-metric and store it in a new tree for quick access
  // -----------------------------------------------------------------------------------------------------------
  aLOG(Log::INFO) <<coutGreen<<" - will loop on the input trees ..."<<coutDef<<endl;

  if(seed > 0) { seed++; } rnd = new TRandom3(seed);

  // define a VarMaps to loop over the chain
  TString aChainName = aChain->GetName();
  VarMaps * var_0    = new VarMaps(glob,utils,(TString)"inputTreeVars_"+aChainName);
  var_0->connectTreeBranches(aChain);

  double pdfObjFrac(-1);
  if(max_optimObj_PDF > 0) {
    double nEntriesChain = aChain->GetEntries();
    pdfObjFrac = min((max_optimObj_PDF/nEntriesChain), 1.);
  }

  for(int nMLMnow=0; nMLMnow<nMLMs; nMLMnow++) {
    TString MLMname = getTagName(nMLMnow); 
    if(!mlmSkip[MLMname]) aChain->SetBranchStatus(getTagIndex(nMLMnow),0);
  }

  VarMaps * var_1 = new VarMaps(glob,utils,(TString)"vars_"+intgrZtreeName);
  for(int nMLMnow=0; nMLMnow<nMLMs; nMLMnow++) {
    TString MLMname   = getTagName(nMLMnow);   if(mlmSkipPdf[MLMname]) continue;
    TString MLMname_w = getTagWeight(nMLMnow);
    var_1->NewVarF(MLMname);
  }
  var_1->NewVarF("eventWeight");
  var_1->NewVarF(zTrgName);

  TTree * outTree = new TTree(intgrZtreeName,intgrZtreeName);
  outTree->SetDirectory(0); outputs->TreeMap[intgrZtreeName] = outTree;
  
  var_1->createTreeBranches(outTree);

  // -----------------------------------------------------------------------------------------------------------
  // loop on the tree
  // -----------------------------------------------------------------------------------------------------------
  bool  breakLoop(false);
  int   nObjectsToWrite(glob->GetOptI("nObjectsToWrite"));
  var_0->clearCntr();
  for(Long64_t loopEntry=0; true; loopEntry++) {
    if(!var_0->getTreeEntry(loopEntry)) breakLoop = true;

    if((var_0->GetCntr("nObj")+1 % nObjectsToWrite == 0) || breakLoop) { var_0->printCntr(aChainName,Log::DEBUG_1); }
    if(breakLoop) break;

    if(pdfObjFrac > 0) {
      if(rnd->Rndm() > pdfObjFrac) continue;
    }
    var_0->IncCntr("nObj"); if(var_0->GetCntr("nObj") == maxNobj) breakLoop = true;

    double zTrg = var_0->GetVarF(zTrgName);

    // -----------------------------------------------------------------------------------------------------------
    // loop over all MLMs
    // -----------------------------------------------------------------------------------------------------------
    double eventWeight(0);
    for(int nMLMnow=0; nMLMnow<nMLMs; nMLMnow++) {
      TString MLMname    = getTagName(nMLMnow);      if(mlmSkipPdf[MLMname]) continue;
      TString MLMname_w  = getTagWeight(nMLMnow);
      TString MLMname_eN = getTagError(nMLMnow,"N"); TString MLMname_eP = getTagError(nMLMnow,"P");

      double  regNow    = var_0->GetVarF(MLMname);
      double  errN      = var_0->GetVarF(MLMname_eN); if(errN      < EPS) continue;
      double  errP      = var_0->GetVarF(MLMname_eP); if(errP      < EPS) continue;
      double  weightNow = var_0->GetVarF(MLMname_w);  if(weightNow < EPS) continue;
      
      eventWeight += weightNow;

      double intgrZmlm(0);
      for(int nSmearNow=0; nSmearNow<nIntgrZsmears; nSmearNow++) {
        double sfNow(0);
        if(nSmearNow > 0) {
          int    signNow = (nSmearNow % 2 == 0) ? 1    : -1;
          double errNow  = (nSmearNow % 2 == 0) ? errP : errN;
          sfNow          = signNow * fabs(rnd->Gaus(0,errNow));
        }
        double zReg = regNow + sfNow;
        if(zReg < zTrg) intgrZmlm += 1;
      }
      intgrZmlm /= double(nIntgrZsmears);
      var_1->SetVarF(MLMname, intgrZmlm);
    }
    eventWeight /= double(nMLMs);
    var_1->SetVarF("eventWeight",eventWeight);
    var_1->SetVarF(zTrgName,zTrg);
    
    var_1->fillTree();
  }
  if(!breakLoop) { var_0->printCntr(aChainName,Log::DEBUG_1); }

  DELNULL(rnd);

  outputs->WriteOutObjects(false,true); outputs->ResetObjects();

  DELNULL(var_1);
  DELNULL(outTree); outputs->TreeMap.erase(intgrZtreeName);

  
  // -----------------------------------------------------------------------------------------------------------
  // use the integral-metric with various MLM combinations to compare pdf candidates
  // -----------------------------------------------------------------------------------------------------------
  TString intgrZfileName = (TString)glob->GetOptC("outDirNameFull")+intgrZtreeName+"*.root";
  TChain  * intgrZchain  = new TChain(intgrZtreeName,intgrZtreeName); intgrZchain->SetDirectory(0); intgrZchain->Add(intgrZfileName); 
  aLOG(Log::DEBUG) <<coutRed<<" - added chain "<<coutGreen<<intgrZtreeName<<"("<<intgrZchain->GetEntries()<<")"
                   <<" from "<<coutBlue<<intgrZfileName<<coutDef<<endl;

  var_1 = new VarMaps(glob,utils,(TString)"vars_"+intgrZtreeName);
  var_1->connectTreeBranches(intgrZchain);

  TFile    * rootSaveFile(NULL);
  TProfile * hisIntgrZmlm(NULL);
  if(saveProfileHis) {
    TString hisName("his_intgrZmlmOptim");
    hisIntgrZmlm = new TProfile(hisName,hisName,nPDFbins,&(zPDF_binE[0]));
    hisIntgrZmlm->GetXaxis()->SetTitle((TString)"C("+zTrgTitle+")");
    hisIntgrZmlm->GetYaxis()->SetTitle((TString)"1/N dN/dC("+zTrgTitle+")");

    // make sure the name does not fall under the patter for intgrZfileName (sice it could be deleted...)
    TString rootFileName = (TString)glob->GetOptC("outDirNameFull")+"intgrZmlmOptimProfileHis.root";
    rootSaveFile = new TFile(rootFileName,"RECREATE");
  }

  if(seed > 0) { seed++; } rnd = new TRandom3(seed);

  // -----------------------------------------------------------------------------------------------------------
  // perform the random walk alg
  // -----------------------------------------------------------------------------------------------------------
  double            avgIntgrZ(0.5);
  int               nNoUpdate(0), nSameWeights(0);
  double            varIntgrBest(std::numeric_limits<double>::max()), varIntgrPrev(varIntgrBest);
  vector < double > weightsNow(weightV), weightsPrev(weightV),
                    weightsClipped, weightsBest, intgrZ_valV, intgrZmlm_nEvtV;

  vector < int > updateIndexV;
  for(int nMLMnow=0; nMLMnow<nMLMs; nMLMnow++) {
    TString MLMname = getTagName(nMLMnow);
    if(!mlmSkipPdf[MLMname]) updateIndexV.push_back(nMLMnow);
  }
  vector < int >::iterator indexItr = updateIndexV.begin();
  int nWgtsIn = updateIndexV.size();
  int nRnds0(0);
  if     (nWgtsIn < 10) nRnds0 = 1;
  else if(nWgtsIn < 20) nRnds0 = 5;
  else if(nWgtsIn < 40) nRnds0 = 10;
  else                  nRnds0 = floor(nWgtsIn * 0.2);

  for(int nLoopNow=0; nLoopNow<nOptimLoops; nLoopNow++) {
    bool canPrint(false);
    if     (nLoopNow < 10)                        canPrint = true;
    else if(nLoopNow < 100  && nLoopNow%10  == 0) canPrint = true;
    else if(nLoopNow < 500  && nLoopNow%20  == 0) canPrint = true;
    else if(nLoopNow < 1000 && nLoopNow%50  == 0) canPrint = true;
    else if(                   nLoopNow%100 == 0) canPrint = true;

    intgrZ_valV.resize(nPDFbins,0); intgrZmlm_nEvtV.resize(nPDFbins,0);

    if(canPrint && saveProfileHis) hisIntgrZmlm->Reset();

    // -----------------------------------------------------------------------------------------------------------
    // clip any weights smaller than minPdfWeight, so long as at least minAcptMLMsForPDFs MLMs remain
    // -----------------------------------------------------------------------------------------------------------
    weightsClipped = clipWeightsPDF(weightsNow, Log::DEBUG_2);

    // -----------------------------------------------------------------------------------------------------------
    // calculate the optimization metric for this set of PDF weights from the entire dataset
    // -----------------------------------------------------------------------------------------------------------
    for(Long64_t loopEntry=0; true; loopEntry++) {
      if(!var_1->getTreeEntry(loopEntry)) break;

      double zTrg        = var_1->GetVarF(zTrgName);
      double eventWeight = var_1->GetVarF("eventWeight");
      int    nPdfBinNow  = getBinZ(zTrg, zPDF_binE);  if(nPdfBinNow < 0) continue;

      double intgrZ(0);
      for(int nMLMnow=0; nMLMnow<nMLMs; nMLMnow++) {
        TString MLMname = getTagName(nMLMnow); if(mlmSkipPdf[MLMname]) continue;
        
        double intgrZmlm = var_1->GetVarF(MLMname);
        intgrZ += intgrZmlm * weightsClipped[nMLMnow];
      }

      if(intgrZ >= excRange[0] && intgrZ <= excRange[1]) {
        if(canPrint && saveProfileHis) hisIntgrZmlm->Fill(zTrg,intgrZ,eventWeight);

        intgrZ_valV    [nPdfBinNow] += intgrZ * eventWeight;
        intgrZmlm_nEvtV[nPdfBinNow] += eventWeight;
      }
    }

    // -----------------------------------------------------------------------------------------------------------
    // derive the final metric as the RMS around avgIntgrZ
    // -----------------------------------------------------------------------------------------------------------
    double varIntgrNow(0), nBinsIn(0);
    for(int nPdfBinNow=0; nPdfBinNow<nPDFbins; nPdfBinNow++) {
      if(intgrZmlm_nEvtV[nPdfBinNow] < EPS) continue;

      double intgrZ = intgrZ_valV[nPdfBinNow]/intgrZmlm_nEvtV[nPdfBinNow];
      varIntgrNow += pow((intgrZ - avgIntgrZ), 2);

      nBinsIn++;
    }
    varIntgrNow = (varIntgrNow > EPS) ? sqrt(varIntgrNow/nBinsIn) : 0;
    
    // check if the current solution is better then the previous one (or the best so far)
    bool isBest   = varIntgrNow < varIntgrBest;
    bool isBetter = varIntgrNow < varIntgrPrev;
    
    bool canPrintNew(false);
    if(isBest) {
      if(varIntgrBest < EPS) canPrintNew = true;
      else                   canPrintNew = ((varIntgrBest - varIntgrNow)/varIntgrBest > 0.01);
    }
    
    // -----------------------------------------------------------------------------------------------------------
    // output for the user to validate that things are going well
    // -----------------------------------------------------------------------------------------------------------
    if(canPrint || canPrintNew) {
      weightMsg = "";
      for(int nMLMnow=0; nMLMnow<nMLMs; nMLMnow++) { 
        TString MLMname = getTagName(nMLMnow); if(mlmSkipPdf[MLMname]) continue;
        weightMsg += coutBlue+MLMname+":"+coutPurple+utils->floatToStr(weightsClipped[nMLMnow],"%1.3f")+" ";
      }

      TString msgHead = (TString)(isBest ? " - NEW:  " : " - nTry: ");
      TString msgCol  = (TString)(isBest ? coutGreen : coutYellow);
      aLOG(Log::INFO) <<msgCol<<msgHead<<coutPurple<<nLoopNow
                      <<msgCol<<" - min-param best/prev/now: "<<coutBlue<< utils->floatToStr(varIntgrBest,"%1.5e")
                      <<msgCol<<" / "<<coutRed<< utils->floatToStr(varIntgrPrev,"%1.5e")
                      <<msgCol<<" / "<<coutBlue<< utils->floatToStr(varIntgrNow,"%1.5e")
                      <<msgCol<<(TString)(inLOG(Log::DEBUG_2)?" , PDF: ":"")
                      <<(TString)(inLOG(Log::DEBUG_2)?weightMsg:"")<<coutDef<<endl;

      if(canPrint && saveProfileHis) {      
        hisIntgrZmlm->SetTitle((TString)(TString)"RMS[C("+zTrgTitle+")] = "+utils->floatToStr(varIntgrNow,"%1.5e"));
        hisIntgrZmlm->Write((TString)hisIntgrZmlm->GetName()+"_"+utils->intToStr(nLoopNow));
      }
    }

    // -----------------------------------------------------------------------------------------------------------
    // update the best weights if needed
    // -----------------------------------------------------------------------------------------------------------
    if(isBest) {
      varIntgrBest = varIntgrNow;
      weightsBest  = weightsNow;
      nSameWeights = 0;
    }
    else nSameWeights++;
    
    // -----------------------------------------------------------------------------------------------------------
    // go back to the best solution if there has been no improvement for a while
    // finish if there has been no improvement for a long time
    // -----------------------------------------------------------------------------------------------------------
    if(nSameWeights % nSameWeightsMax == 0) weightsPrev = weightsBest;
    if(nSameWeights >= maxOptimTries) break;

    // -----------------------------------------------------------------------------------------------------------
    // continue with this set of weights if the result is better, or if forceUpdate
    // -----------------------------------------------------------------------------------------------------------
    bool forceUpdate = (nNoUpdate >= nNoUpdateMax) || (rnd->Rndm() < fracWeightUpdate);
    if(isBetter || forceUpdate) {
      nNoUpdate    = 0;
      weightsPrev  = weightsNow;
      varIntgrPrev = varIntgrNow;
    }
    else {
      nNoUpdate++;
      weightsNow = weightsPrev;
    }

    // -----------------------------------------------------------------------------------------------------------
    // change one or more of the weights for the next iteration
    // -----------------------------------------------------------------------------------------------------------
    int nRnds(nRnds0);
    if(rnd->Rndm() < 0.5) {
      nRnds = max(nWgtsIn, nRnds + int(ceil(0.2 * rnd->Rndm() * nWgtsIn)));
    }
    for(int nRndNow=0; nRndNow<nRnds; nRndNow++) {
      while(true) {
        weightsNow[*indexItr] += 0.05 * (2*rnd->Rndm() - 1);
        if(weightsNow[*indexItr] >= 0) break;
      }
      
      indexItr++;
      if(indexItr == updateIndexV.end()) { indexItr = updateIndexV.begin(); }
    }

    // normalize the new weights
    double sumWeights(0);
    for(int nMLMnow=0; nMLMnow<nMLMs; nMLMnow++) { sumWeights += weightsNow[nMLMnow]; }
    for(int nMLMnow=0; nMLMnow<nMLMs; nMLMnow++) { weightsNow[nMLMnow] /= sumWeights; }
  }


  // -----------------------------------------------------------------------------------------------------------
  // clip any weights smaller than minPdfWeight, so long as at least minAcptMLMsForPDFs MLMs remain
  // -----------------------------------------------------------------------------------------------------------
  weightsBest = clipWeightsPDF(weightsBest, Log::DEBUG_1);

  // -----------------------------------------------------------------------------------------------------------
  // final message with the results
  // -----------------------------------------------------------------------------------------------------------
  tmpWeightV.clear();
  for(int nMLMnow=0; nMLMnow<nMLMs; nMLMnow++) {
    if(weightsBest[nMLMnow] < EPS) continue;
    tmpWeightV.push_back(pair<int,double>(nMLMnow,weightsBest[nMLMnow]));
  }

  // sort so that the largest element is first
  sort(tmpWeightV.begin(),tmpWeightV.end(),sortFunc::pairID::highToLowBy1);

  weightMsg = "";
  for(int nAcptMLMnow=0; nAcptMLMnow<(int)tmpWeightV.size(); nAcptMLMnow++) {
    TString MLMname = getTagName(tmpWeightV[nAcptMLMnow].first);
    weightMsg += (TString)coutGreen+MLMname+":"+coutYellow+utils->floatToStr(tmpWeightV[nAcptMLMnow].second,"%1.3f")+" ";
  }

  aLOG(Log::INFO)  <<coutPurple<<" - finished PDF optimization! --> final minimization parameter: "
                   <<coutRed<<utils->floatToStr(varIntgrBest,"%1.5e")<<coutDef<<endl;
  aLOG(Log::INFO)  <<coutPurple<<"   final PDF weights: "<<weightMsg<<coutDef<<endl;
  aLOG(Log::DEBUG) <<coutGreen<<" ----------------------------------------------------------------------------- "
                   <<coutDef<<endl;

  // cleanup
  DELNULL(var_1); DELNULL(intgrZchain);
  DELNULL(rnd);

  // -----------------------------------------------------------------------------------------------------------
  // loop over the original input trees to calculate the bias correction for the final pdf
  // -----------------------------------------------------------------------------------------------------------
  if(doBiasCorPDF) {
    aLOG(Log::INFO) <<coutBlue<<" - will derive the bias correction for the pdf ... "<<coutDef<<endl;

    if(seed > 0) { seed++; } rnd = new TRandom3(seed);
    
    breakLoop = false; var_0->clearCntr();
    for(Long64_t loopEntry=0; true; loopEntry++) {
      if(!var_0->getTreeEntry(loopEntry)) breakLoop = true;

      if((var_0->GetCntr("nObj")+1 % nObjectsToWrite == 0) || breakLoop) { var_0->printCntr(aChainName,Log::DEBUG_1); }
      if(breakLoop) break;
      
      var_0->IncCntr("nObj"); if(var_0->GetCntr("nObj") == maxNobj) breakLoop = true;

      // get the target value and smear the reg-value of the best MLM
      double zTrg = var_0->GetVarF(zTrgName);

      // loop over all MLMs
      for(int nMLMnow=0; nMLMnow<nMLMs; nMLMnow++) {
        TString MLMname    = getTagName(nMLMnow);      if(mlmSkipPdf[MLMname]) continue;
        TString MLMname_w  = getTagWeight(nMLMnow);
        TString MLMname_eN = getTagError(nMLMnow,"N"); TString MLMname_eP = getTagError(nMLMnow,"P");

        double  regNow    = var_0->GetVarF(MLMname);
        double  errN      = var_0->GetVarF(MLMname_eN); if(errN      < EPS) continue;
        double  errP      = var_0->GetVarF(MLMname_eP); if(errP      < EPS) continue;
        double  weightNow = var_0->GetVarF(MLMname_w);  if(weightNow < EPS) continue;

        // generate random smearing factors for this MLM
        for(int nSmearRndNow=0; nSmearRndNow<nSmearsRnd; nSmearRndNow++) {
          int    signNow    = (nSmearRndNow % 2 == 0) ? 1    : -1;
          double errNow     = (nSmearRndNow % 2 == 0) ? errP : errN;
          double sfNow      = signNow * fabs(rnd->Gaus(0,errNow));
          double zReg       = regNow + sfNow;
          double weightFull = weightsBest[nMLMnow] * weightNow;

          hisPdfTryBiasCorV->Fill(zReg,zTrg,weightFull);
        }
      }
    }
    if(!breakLoop) { var_0->printCntr(aChainName,Log::DEBUG_1); }

    DELNULL(rnd);
  }
  DELNULL(var_0);

  // -----------------------------------------------------------------------------------------------------------
  // store the results in the vector accosible by the outside world
  // -----------------------------------------------------------------------------------------------------------
  bestWeightsV.clear();                hisPdfBiasCorV.clear();
  bestWeightsV.push_back(weightsBest); hisPdfBiasCorV.push_back(hisPdfTryBiasCorV);
  
  // -----------------------------------------------------------------------------------------------------------
  // cleanup
  // -----------------------------------------------------------------------------------------------------------
  if(saveProfileHis) {
    aLOG(Log::INFO) <<coutYellow<<" - closing file with profile histograms: "<<coutGreen<<rootSaveFile->GetName()<<coutDef<<endl;
    rootSaveFile->Close();  DELNULL(rootSaveFile);
  }

  // cleanup the transient trees we created during optimization
  if(!glob->GetOptB("keepOptimTrees_randReg")) {
    utils->safeRM(intgrZfileName,inLOG(Log::DEBUG));
  }

  sigm68V.clear();    biasV.clear();       sigmToBiasV.clear();
  weightV.clear();    mlmSkipPdf.clear();  updateIndexV.clear();
  weightsNow.clear(); weightsPrev.clear(); weightsBest.clear();
  excRange.clear();   intgrZ_valV.clear(); intgrZmlm_nEvtV.clear();
  tmpWeightV.clear(); weightsClipped.clear();

  // -----------------------------------------------------------------------------------------------------------
  // call the old-style pdf function if needed
  // -----------------------------------------------------------------------------------------------------------
  if(nPDFs > 1) {
    getOldStyleRndMethodBestPDF(
      aChain, bestANNZindex, zRegQnt_nANNZ, zRegQnt_bias,
      zRegQnt_sigma68, zRegQnt_fracSig68, bestWeightsV, hisPdfBiasCorV
    );
  }

  return;
}


// ===========================================================================================================
/**
 * @brief                    - clip a vector of weights.
 * 
 * @details                  - clip a vector of weights, keeping weights above minPdfWeight, with the constraint that
 *                           the minimal number of elements in the vector remains no smaller than minAcptMLMsForPDFs
 *                           
 * @param weightsIn          - vector of input weights (is not modified within the function)
 * @param logLevel           - new vector of output weights, after clipping
 */
// ===========================================================================================================
vector < double >  ANNZ::clipWeightsPDF(vector < double > & weightsIn, Log::LOGtypes logLevel) {
// ===========================================================================================================
  int     nMLMs              = glob->GetOptI("nMLMs");
  double  minPdfWeight       = glob->GetOptF("minPdfWeight");
  int     minAcptMLMsForPDFs = glob->GetOptI("minAcptMLMsForPDFs");
  bool    printFullWeightV   = false; // debugging flag

  vector < pair<int,double> > tmpWeightV;
  vector < double >           weightsClipped(weightsIn);
  
  // -----------------------------------------------------------------------------------------------------------
  // only proceed if the minPdfWeight threshold is defined
  // -----------------------------------------------------------------------------------------------------------
  if(minPdfWeight < EPS || minPdfWeight >= 1) return weightsClipped;

  // -----------------------------------------------------------------------------------------------------------
  // just in case, normalize the original weights + do a sanity chekc, before getting started
  // -----------------------------------------------------------------------------------------------------------
  double sumWeights(0);
  for(int nMLMnow=0; nMLMnow<nMLMs; nMLMnow++) { sumWeights += weightsClipped[nMLMnow]; }
  if(sumWeights < EPS) {
    aLOG(Log::WARNING) <<coutRed<<"Clipping attempted with sumWeights=0 !!! Something is horribly "
                       <<"wrong !!! - ANNZ::clipWeightsPDF() aborted ..."<<coutDef<<endl;
    
    return weightsClipped;
  }
  for(int nMLMnow=0; nMLMnow<nMLMs; nMLMnow++) { weightsClipped[nMLMnow] /= sumWeights; }

  // sort the weights
  for(int nMLMnow=0; nMLMnow<nMLMs; nMLMnow++) {
    if(weightsClipped[nMLMnow] < EPS) continue;
    tmpWeightV.push_back(pair<int,double>(nMLMnow,weightsClipped[nMLMnow]));
  }

  // sort so that the largest element is first
  sort(tmpWeightV.begin(),tmpWeightV.end(),sortFunc::pairID::highToLowBy1);

  // -----------------------------------------------------------------------------------------------------------
  // consecutively clip weights smaller than minPdfWeight, so long as at least minAcptMLMsForPDFs MLMs remain
  // -----------------------------------------------------------------------------------------------------------
  while(true) {
    bool canClip(false);
    for(int nAcptMLMnow=0; nAcptMLMnow<(int)tmpWeightV.size(); nAcptMLMnow++) {
      if(tmpWeightV[nAcptMLMnow].second < minPdfWeight) {
        canClip = true;
        break;
      }
    }
    if(!canClip) break;

    if((int)tmpWeightV.size() <= minAcptMLMsForPDFs) {
      aLOG(logLevel) <<coutRed<<" - can not clip the next least-significant MLMs from "
                     <<"PDF ... need at least "<<coutYellow<<minAcptMLMsForPDFs<<coutRed
                     <<" to survive ..."<<coutDef<<endl;
      break;
    }

    int nMLMminWgt = tmpWeightV.back().first;
    aLOG(logLevel) <<coutYellow<<" - clipping least-significant MLMs from PDF: " <<coutGreen
                   <<getTagName(nMLMminWgt)<<coutYellow<<" with weight "<<coutRed
                   <<utils->floatToStr(tmpWeightV.back().second,"%1.5e")<<coutYellow
                   <<" has been removed ... "<<coutDef<<endl;

    weightsClipped[nMLMminWgt] = 0;
    tmpWeightV.pop_back();
    
    sumWeights = 0;
    for(int nAcptMLMnow=0; nAcptMLMnow<(int)tmpWeightV.size(); nAcptMLMnow++) {
      sumWeights += tmpWeightV[nAcptMLMnow].second;
    }
    if(sumWeights < EPS) {
      weightsClipped = weightsIn;

      aLOG(Log::WARNING) <<coutRed<<"Clipping resulted in sumWeights=0 !!! try adjusting the values "
                         <<"of \"minPdfWeight\" and/or \"minAcptMLMsForPDFs\". "
                         <<"ANNZ::clipWeightsPDF() aborted ..."<<coutDef<<endl;
      break;
    }

    for(int nAcptMLMnow=0; nAcptMLMnow<(int)tmpWeightV.size(); nAcptMLMnow++) {
      tmpWeightV[nAcptMLMnow].second /= sumWeights;
      weightsClipped[tmpWeightV[nAcptMLMnow].first] = tmpWeightV[nAcptMLMnow].second;
    }

    if(inLOG(logLevel) && printFullWeightV) {
      for(int nMLMnow=0; nMLMnow<nMLMs; nMLMnow++) {
        aLOG(logLevel) <<coutGreen<<" - clipWeightsPDF:  "<<coutBlue
                       <<nMLMnow<<CT<<coutGreen<<TString::Format("%1.4f",weightsIn[nMLMnow])<<" -> "
                       <<coutYellow<<TString::Format("%1.4f",weightsClipped[nMLMnow])
                       <<coutRed<<(TString)(nMLMnow == nMLMminWgt ? " CLIPPED" : "")<<coutDef<<endl;
      }
    }
  }
  if(printFullWeightV) aLOG(logLevel) << endl;

  tmpWeightV.clear();

  return weightsClipped;
}


// ===========================================================================================================
/**
 * @brief                    - Generate PDF weighting schemes for randomized regression.
 * 
 * @details                  - Candidate PDFs are created by randomely generating weighting schemes, which represent different
 *                           supression powers af MLM, based on metric ranking: MLMs with better (smaller values of) metrics, have
 *                           larger relative weights.
 *                           - PDFs are selected by choosing the weighting scheme which is "most compatible" with the true value.
 *                           This is determined in two ways (generating two alternative PDFs), using cumulative distributions; for one
 *                           PDF, the cumulative distribution is based on the "truth" (the target variable, zTrg). For the other
 *                           PDF, the cumulative distribution is based on the "best" MLM.
 *                           For the former, a set of templates, derived from zTrg is used to fit the dataset. For the later,
 *                           a flat distribution of the cumulator serves as the baseline.
 *                           
 * @param aChain             - Input chain, for which to do the PDF derivation.
 * @param bestANNZindex      - Index of the "best" MLM.
 * @param zRegQnt_nANNZ      - Map of vector, which is filled with the index of MLMs, corresponding to the order of the
 *                           entries of the metrics in the other vectors (zRegQnt_sigma68 and zRegQnt_bias).
 * @param zRegQnt_bias       - Map of vector, which is filled with the claculated bias metric.
 * @param zRegQnt_sigma68    - Map of vector, which is filled with the claculated sigma68 metric.
 * @param zRegQnt_fracSig68  - Map of vector, which is filled with the claculated combined outlier-fraction metric.
 * @param bestWeightsV       - Vector into which the results of the PDF derivation are stored (as weights for
 *                           each MLM).
 * @param hisPdfBiasCorV     - Vector for bias correction histograms for pdfs.
 */
// ===========================================================================================================
void  ANNZ::getOldStyleRndMethodBestPDF(
  TTree                     * aChain,          int            bestANNZindex,
  vector<int>               & zRegQnt_nANNZ,   vector<double> & zRegQnt_bias,
  vector<double>            & zRegQnt_sigma68, vector<double> & zRegQnt_fracSig68, 
  vector < vector<double> > & bestWeightsV,    vector <TH2*>  & hisPdfBiasCorV
) {
// ===========================================================================================================
  aLOG(Log::INFO) <<coutWhiteOnBlack<<coutYellow<<" - starting ANNZ::getOldStyleRndMethodBestPDF() ... "<<coutDef<<endl;
  VERIFY(LOCATION,(TString)"Memory leak ?! ",(dynamic_cast<TChain*>(aChain)));

  double  minValZ            = glob->GetOptF("minValZ");
  double  maxValZ            = glob->GetOptF("maxValZ");
  TString zTrgName           = glob->GetOptC("zTrg");
  int     maxNobj            = glob->GetOptI("maxNobj");
  int     nTryPDFs           = glob->GetOptI("nRndPdfWeightTries"); // number of variations of pdf to test
  int     nSmearsRnd         = glob->GetOptI("nSmearsRnd");
  int     nMLMs              = glob->GetOptI("nMLMs");
  int     nPDFbins           = glob->GetOptI("nPDFbins");
  TString zTrgTitle          = glob->GetOptC("zTrgTitle");
  TString _typeANNZ          = glob->GetOptC("_typeANNZ");
  TString zRegTitle          = glob->GetOptC("zRegTitle");
  UInt_t  seed               = glob->GetOptI("initSeedRnd"); if(seed > 0) seed += 98187;
  int     closHisN           = glob->GetOptI("closHisN");
  double  closHisL           = glob->GetOptF("closHisL");
  double  closHisH           = glob->GetOptF("closHisH");
  int     minAcptMLMsForPDFs = glob->GetOptI("minAcptMLMsForPDFs");
  double  minPdfWeight       = glob->GetOptF("minPdfWeight");
  int     nPDFs              = glob->GetOptI("nPDFs");
  double  max_sigma68_PDF    = glob->GetOptF("max_sigma68_PDF");
  double  max_bias_PDF       = glob->GetOptF("max_bias_PDF");
  double  max_frac68_PDF     = glob->GetOptF("max_frac68_PDF");
  bool    doBiasCorPDF       = glob->GetOptB("doBiasCorPDF");
  bool    doPlots            = glob->GetOptB("doPlots");

  bool    optimWithMAD       = glob->GetOptB("optimWithMAD");
  TString quantScatterName   = optimWithMAD ? (TString)"quant_MAD" : (TString)"quant_sigma_68";
  TString sig68Title         = optimWithMAD ? (TString)"MAD"       : (TString)"#sigma_{68}";
  
  bool    optimWithSclBias   = glob->GetOptB("optimWithScaledBias");
  TString biasTitle          = optimWithSclBias ? (TString)"#delta/(1+"+zTrgTitle+")" : (TString)"#delta";

  TString MLMnameBest        = getTagName(bestANNZindex);
  TString MLMnameBest_e      = getTagError(bestANNZindex);
  int     nModels            = 7;
  int     nSimObjs           = (int)1e4;
  int     plotBinFreq        = static_cast<int>(floor(0.01 + 6/double(nTryPDFs)))+1;
  int     nSmearsRndHalf     = static_cast<int>(floor(0.01 + nSmearsRnd/2.));
  int     nBinsZ             = (int)zClos_binC.size();

  if(nPDFs == 0) { aLOG(Log::INFO) <<coutGreen<<" - no PDFs requested - nothing to do here... "<<coutDef<<endl; return; }

  VERIFY(LOCATION,(TString)"zClos_binC not properly initialized ?!? ",(nBinsZ > 0)); //sanity check

  TRandom                           * rnd(NULL);
  vector < TString >                funcNameV, fitTitleV;
  vector < TH1* >                   hisIntgrZtrgV(nTryPDFs,NULL), hisIntgrZregV(nTryPDFs,NULL);
  vector < TGraphErrors* >          weightGrphV;
  vector < double >                 fitMetric(nTryPDFs);
  vector < pair<int,double> >       sigm68V, biasV, fitMetric_nPDF(nTryPDFs);
  vector < vector<double> >         weightsM(nTryPDFs,vector<double>(nMLMs,0));
  map    < int,vector < double > >  scaleFactorModel;
  vector < double >                 bestWeightsV_Ztrg, bestWeightsV_Zreg;

  // -----------------------------------------------------------------------------------------------------------
  // setup histograms
  // -----------------------------------------------------------------------------------------------------------
  vector < TH2* >         his2_N(nTryPDFs,NULL);
  vector < TH1* >         his1_d(nTryPDFs,NULL), his1_s(nTryPDFs,NULL), hisModelAll(nTryPDFs,NULL);
  vector < vector<TH1*> > hisIntgrZtrgSimV(nTryPDFs,vector<TH1*>(nModels,NULL));

  int    nBins0(21);
  double binEdgeL0(-0.025), binEdgeH0(1.025);
  for(int nPDFnow=0; nPDFnow<nTryPDFs; nPDFnow++) {
    TString nPdfName(TString::Format("_nPdf%d",nPDFnow)), hisName("");

    hisName = (TString)"pdfIngrWeightHis"+"_zTrg"+nPdfName+_typeANNZ;
    hisIntgrZtrgV[nPDFnow] = new TH1F(hisName,hisName,nBins0,binEdgeL0,binEdgeH0);
    hisIntgrZtrgV[nPDFnow]->GetXaxis()->SetTitle((TString)"C("+zTrgTitle+")");
    hisIntgrZtrgV[nPDFnow]->GetYaxis()->SetTitle((TString)"1/N dN/dC("+zTrgTitle+")");

    hisName = (TString)"pdfIngrWeightHis"+"_zReg"+nPdfName+_typeANNZ;
    hisIntgrZregV[nPDFnow] = (TH1*)hisIntgrZtrgV[nPDFnow]->Clone(hisName);
    hisIntgrZregV[nPDFnow]->GetXaxis()->SetTitle((TString)"C("+zTrgTitle+"(best))");
    hisIntgrZregV[nPDFnow]->GetYaxis()->SetTitle((TString)"1/N dN/dC("+zTrgTitle+"(best)})");
  
    hisName = (TString)"modelPdf_Nz"+nPdfName+_typeANNZ;
    his2_N[nPDFnow] = new TH2F(hisName,hisName,closHisN,closHisL,closHisH,nBinsZ,&(zClos_binE[0]));
    his2_N[nPDFnow]->GetXaxis()->SetTitle((TString)zRegTitle+"-"+zTrgTitle);
    his2_N[nPDFnow]->GetYaxis()->SetTitle((TString)zTrgTitle);
    
    hisName = (TString)"modelPdf_DELTA"+nPdfName+_typeANNZ;     his1_d[nPDFnow] = new TH1F(hisName,hisName,nBinsZ,&(zClos_binE[0]));
    hisName = (TString)"modelPdf_SIGMA"+nPdfName+_typeANNZ;     his1_s[nPDFnow] = new TH1F(hisName,hisName,nBinsZ,&(zClos_binE[0]));
    his1_d[nPDFnow]->GetXaxis()->SetTitle((TString)zTrgTitle);  his1_d[nPDFnow]->GetYaxis()->SetTitle((TString)biasTitle);
    his1_s[nPDFnow]->GetXaxis()->SetTitle((TString)zTrgTitle);  his1_s[nPDFnow]->GetYaxis()->SetTitle((TString)sig68Title);  
    
    for(int nModelNow=0; nModelNow<nModels; nModelNow++) {
      hisName = TString::Format((TString)"pdfIngrWeightHis"+nPdfName+"_nModel_%d"+_typeANNZ,nModelNow);
      hisIntgrZtrgSimV[nPDFnow][nModelNow] = new TH1F(hisName,hisName,nBins0,binEdgeL0,binEdgeH0);
      hisIntgrZtrgSimV[nPDFnow][nModelNow]->GetXaxis()->SetTitle((TString)"C("+zTrgTitle+")");
      hisIntgrZtrgSimV[nPDFnow][nModelNow]->GetYaxis()->SetTitle((TString)"1/N dN/dC("+zTrgTitle+")");
      hisIntgrZtrgSimV[nPDFnow][nModelNow]->SetTitle("");
    }
  }

  // -----------------------------------------------------------------------------------------------------------
  // define bias-correction histograms for the pdfs
  // -----------------------------------------------------------------------------------------------------------
  TH2F           * hisPdfBiasV_Ztrg(NULL), * hisPdfBiasV_Zreg(NULL);
  vector <TH2F*> hisPdfTryBiasCorV(nTryPDFs,NULL);

  if(doBiasCorPDF) {
    for(int nPDFnow=0; nPDFnow<nTryPDFs; nPDFnow++) {
      TString nPDFname           = TString::Format("_nPdf%d",nPDFnow);
      TString hisName            = (TString)"pdfBias"+_typeANNZ+nPDFname;
      hisPdfTryBiasCorV[nPDFnow] = new TH2F(hisName,hisName,nPDFbins,&(zPDF_binE[0]),nPDFbins,&(zPDF_binE[0]));
    }
  }

  // -----------------------------------------------------------------------------------------------------------
  // sort all accepted MLMs by their sigm68 and bias metrics
  // -----------------------------------------------------------------------------------------------------------
  int nAcptMLMs = (int)zRegQnt_nANNZ.size();
  for(int nAcptMLMnow=0; nAcptMLMnow<nAcptMLMs; nAcptMLMnow++) {
    if(zRegQnt_sigma68[nAcptMLMnow] < 0) continue; // has been initialized, such that no entries in this bin

    // check if the metrics are not above threshold (if defined by the user)
    if(max_sigma68_PDF > 0 && zRegQnt_sigma68[nAcptMLMnow] > max_sigma68_PDF) {
     aLOG(Log::INFO) <<coutRed<<" - "<<coutYellow<<getTagName(zRegQnt_nANNZ[nAcptMLMnow])<<coutRed<<" has sigma68 = "
                     <<coutYellow<<zRegQnt_sigma68[nAcptMLMnow]<<coutRed<<" which is above threshold ("<<coutBlue<<max_sigma68_PDF
                     <<coutRed<<") -> it will be rejected from the PDF ..." <<coutDef<<endl;
     continue;
    }
    if(max_bias_PDF > 0 && zRegQnt_bias[nAcptMLMnow] > max_bias_PDF) {
      aLOG(Log::INFO) <<coutRed<<" - "<<coutYellow<<getTagName(zRegQnt_nANNZ[nAcptMLMnow])<<coutRed<<" has bias = "
                      <<coutYellow<<zRegQnt_bias[nAcptMLMnow]<<coutRed<<" which is above threshold ("<<coutBlue<<max_bias_PDF
                      <<coutRed<<") -> it will be rejected from the PDF ..." <<coutDef<<endl;
      continue;
    }
    if(max_frac68_PDF > 0 && zRegQnt_fracSig68[nAcptMLMnow] > max_frac68_PDF) {
      aLOG(Log::INFO) <<coutRed<<" - "<<coutYellow<<getTagName(zRegQnt_nANNZ[nAcptMLMnow])<<coutRed<<" has combined outlier-fraction = "
                      <<coutYellow<<zRegQnt_fracSig68[nAcptMLMnow]<<coutRed<<" which is above threshold ("<<coutBlue<<max_frac68_PDF
                      <<coutRed<<") -> it will be rejected from the PDF ..." <<coutDef<<endl;
      continue;
    }

    sigm68V.push_back(pair<int,double>(zRegQnt_nANNZ[nAcptMLMnow],zRegQnt_sigma68[nAcptMLMnow]));
    biasV  .push_back(pair<int,double>(zRegQnt_nANNZ[nAcptMLMnow],zRegQnt_bias   [nAcptMLMnow]));
  }
  nAcptMLMs = (int)sigm68V.size();
  
  VERIFY(LOCATION,(TString)" - found only "+utils->intToStr(nAcptMLMs)+" accepted MLMs, but requested minAcptMLMsForPDFs = "
                          +utils->intToStr(minAcptMLMsForPDFs)+" ...",(nAcptMLMs >= minAcptMLMsForPDFs));

  // sort so that the smallest element is first
  sort(sigm68V.begin(),sigm68V.end(),sortFunc::pairID::lowToHighBy1);
  sort(biasV.begin(),  biasV.end(),  sortFunc::pairID::lowToHighBy1);


  // -----------------------------------------------------------------------------------------------------------
  // go over all PDF variations and derive the PDF weights
  // -----------------------------------------------------------------------------------------------------------
  for(int nPDFnow=0; nPDFnow<nTryPDFs; nPDFnow++) {
    if(seed > 0) { seed++; } rnd = new TRandom3(seed);

    // -----------------------------------------------------------------------------------------------------------
    // reject worst ?% out of hand, demanding that enough MLMs have both good sigm68 and good bias metrics
    // - make sure what remains is at least minAcptMLMsForPDFs methods
    // -----------------------------------------------------------------------------------------------------------
    int             nNonZeroWeights(0);
    bool            foundGoodSetup(false);
    map <int,bool>  acceptV_sigm, acceptV_both;

    for(int nRelaxReject=0; nRelaxReject<100; nRelaxReject++) {
      int     minRejConst_sigm(1), maxRejConst_sigm(6), minRejConst_bias(0), maxRejConst_bias(3);
      int     maxAcptMLMs(0);
      double  rndNow(0), rejectionFrac(0);

      if(nRelaxReject > 0) {
        maxRejConst_sigm -= 0.5 * nRelaxReject;
        maxRejConst_bias -= 0.5 * nRelaxReject;
        if(maxRejConst_sigm <= minRejConst_sigm) break;
        if     (maxRejConst_bias <= minRejConst_bias && maxRejConst_sigm <= minRejConst_sigm) break;
        else if(maxRejConst_bias <= minRejConst_bias                                        ) maxRejConst_bias = minRejConst_bias + 1;

        aLOG(Log::DEBUG_2) <<coutLightBlue<<nPDFnow<<"("<<nRelaxReject
                           <<") Reduced rejectionFrac for sigma,bias to: "<<maxRejConst_sigm<<" , "<<maxRejConst_bias<<coutDef<<endl;
      }
      acceptV_sigm.clear(); acceptV_both.clear();

      // first check accepted methods by minimal sigma_68
      // -----------------------------------------------------------------------------------------------------------
      for(int nRjctFarcNow=0; nRjctFarcNow<100; nRjctFarcNow++) {
        rndNow        = rnd->Rndm();
        rejectionFrac = 0.1 * (minRejConst_sigm + int(ceil(rndNow * (maxRejConst_sigm-minRejConst_sigm))));
        maxAcptMLMs   = static_cast<int>(floor(nAcptMLMs * (1-rejectionFrac)));
        
        // set the flag for accepted/rejected methods
        nNonZeroWeights = 0;
        for(int nAcptMLMnow=0; nAcptMLMnow<nAcptMLMs; nAcptMLMnow++) {
          int nANNZ_sigm = sigm68V[nAcptMLMnow].first;
          
          if(nAcptMLMnow > maxAcptMLMs) {
            acceptV_sigm[nANNZ_sigm] = false;
            aLOG(Log::DEBUG_3) <<coutRed<<"Reject SIGM for "<<nAcptMLMnow<<CT<<nANNZ_sigm<<coutDef<<endl;
          }
          else {
            acceptV_sigm[nANNZ_sigm] = true;
            nNonZeroWeights++;
            aLOG(Log::DEBUG_3) <<coutBlue<<"Accept SIGM for "<<nAcptMLMnow<<CT<<nANNZ_sigm<<coutDef<<endl;
          }
        }
        // check the number of accepted methods after the rejection cuts
        aLOG(Log::DEBUG_2) <<coutYellow<<"SIGM - nRjctFarcNow,rejectionFrac,maxAcptMLMs,nNonZeroWeights - "<<nRjctFarcNow<<CT
                           <<rejectionFrac<<CT<<maxAcptMLMs<<CT<<nNonZeroWeights<<coutDef<<endl;
        
        if(nNonZeroWeights >= minAcptMLMsForPDFs) {
          foundGoodSetup = true; break;
        }
      }

      // check the accepted methods by minimal bias (requiring to already also be accepted by sigma_68)
      // -----------------------------------------------------------------------------------------------------------
      if(foundGoodSetup) {
        foundGoodSetup = false;

        for(int nRjctFarcNow=0; nRjctFarcNow<100; nRjctFarcNow++) {
          rndNow        = rnd->Rndm();
          rejectionFrac = 0.1 * (minRejConst_bias + int(ceil(rndNow * (maxRejConst_bias-minRejConst_bias))));
          maxAcptMLMs   = static_cast<int>(floor(nAcptMLMs * (1-rejectionFrac)));
          
          // set the flag for accepted/rejected methods
          nNonZeroWeights = 0;
          for(int nAcptMLMnow=0; nAcptMLMnow<nAcptMLMs; nAcptMLMnow++) {
            int nANNZ_bias = biasV[nAcptMLMnow].first;
            
            if(nAcptMLMnow > maxAcptMLMs) {
              acceptV_both[nANNZ_bias] = false;
              aLOG(Log::DEBUG_3) <<coutRed<<"Reject BIAS for "<<nAcptMLMnow<<CT<<nANNZ_bias<<coutDef<<endl;
            }
            else {
              if(acceptV_sigm[nANNZ_bias]) {
                acceptV_both[nANNZ_bias] = true;
                nNonZeroWeights++;
                aLOG(Log::DEBUG_3) <<coutBlue<<"Accept BIAS for "<<nAcptMLMnow<<CT<<nANNZ_bias<<coutDef<<endl;
              }
              else {
                acceptV_both[nANNZ_bias] = false;
                aLOG(Log::DEBUG_3) <<coutPurple<<"Reject SIGM for "<<nAcptMLMnow<<CT<<nANNZ_bias<<coutDef<<endl;
              }
            }
          }
          // check the number of accepted methods after the rejection cuts
          aLOG(Log::DEBUG_2) <<coutYellow<<"BIAS - nRjctFarcNow,rejectionFrac,maxAcptMLMs,nNonZeroWeights - "<<nRjctFarcNow<<CT
                             <<rejectionFrac<<CT<<maxAcptMLMs<<CT<<nNonZeroWeights<<coutDef<<endl;
          
          if(nNonZeroWeights >= minAcptMLMsForPDFs) {
            foundGoodSetup = true; break;
          }
        }
      }

      if(foundGoodSetup) break;
      
      // if didn't break, than could not find a combination of MLMs with good metrics under the current restrictions
      aLOG(Log::DEBUG_1) <<coutRed<<"Cant find a rejectionFrac which leavs "<<minAcptMLMsForPDFs
                         <<" accepted methods... Will try to relax fraction-cuts."<<coutDef<<endl;
    }
    // shouldn't happen...
    VERIFY(LOCATION,(TString)" --- Cant find a rejectionFrac which leavs "+utils->intToStr(minAcptMLMsForPDFs)
                             +" accepted methods ... Something is horribly wrong !!! Try setting a lower value"
                             +" of minAcptMLMsForPDFs or increasing the number of trained MLMs ...",foundGoodSetup);


    // finally, set the initial weights to zero/one based on the accepted methods by both sigma and bias
    // -----------------------------------------------------------------------------------------------------------
    nNonZeroWeights = 0;
    TString acceptedMLMs(""), rejectedMLMs("");
    for(int nAcptMLMnow=0; nAcptMLMnow<nAcptMLMs; nAcptMLMnow++) {
      int nANNZ_sigm = sigm68V[nAcptMLMnow].first;
      int baseWeight = (acceptV_both[nANNZ_sigm]) ? 1 : 0;

      if(baseWeight) acceptedMLMs   += (TString)coutPurple+getTagName(nANNZ_sigm)+coutGreen+",";
      else           rejectedMLMs   += (TString)coutYellow+getTagName(nANNZ_sigm)+coutGreen+",";

      nNonZeroWeights               += baseWeight;
      weightsM[nPDFnow][nANNZ_sigm]  = baseWeight;
    }
    aLOG(Log::DEBUG) <<coutGreen<<" - accepted MLMs for PDF: "<<acceptedMLMs<<coutDef<<endl;
    aLOG(Log::DEBUG) <<coutGreen<<" - rejected MLMs for PDF: "<<rejectedMLMs<<coutDef<<endl;
    aLOG(Log::DEBUG) <<coutGreen<<" ----------------------------------------------------------------------------- "<<coutDef<<endl;

    acceptV_sigm.clear(); acceptV_both.clear();


    // -----------------------------------------------------------------------------------------------------------
    // define functions between one and zero zero for the weights (first method gets weight one)
    // -----------------------------------------------------------------------------------------------------------
    double  funcSlope = rnd->Rndm() / double(nAcptMLMs);
    int     funcPower = 1 + int(ceil(rnd->Rndm() * 3));
    TString funcName  = TString::Format("TMath::Power(1 - x*%1.6g , %d)",funcSlope,funcPower);
    
    TF1 * pdfWeight_sigm  = new TF1("pdfWeight_sigm",funcName,0,nAcptMLMs);
    TF1 * pdfWeight_bias  = new TF1("pdfWeight_bias",funcName,0,nAcptMLMs);
 
    aLOG(Log::DEBUG_1) <<coutPurple<<" - Optim-stats: #,nNonZeroWeights,func: "<<coutYellow<<nPDFnow<<CT<<nNonZeroWeights<<CT<<funcName<<coutDef<<endl;

    // for each method derive the weight by the sigma68 and the fracSig68
    // -----------------------------------------------------------------------------------------------------------
    for(int nAcptMLMnow=0; nAcptMLMnow<nAcptMLMs; nAcptMLMnow++) {
      int     nANNZ_sigm  = sigm68V[nAcptMLMnow].first;
      int     nANNZ_bias  = biasV[nAcptMLMnow].first;

      // derive weights for the methods according to the current PDF variation
      double  weight_sigm = max(pdfWeight_sigm->Eval(nAcptMLMnow),0.);
      double  weight_bias = max(pdfWeight_bias->Eval(nAcptMLMnow),0.);

      if(weight_sigm > EPS) weight_sigm = sqrt(weight_sigm);
      if(weight_bias > EPS) weight_bias = sqrt(weight_bias);
      
      weightsM[nPDFnow][nANNZ_sigm] *= weight_sigm;
      weightsM[nPDFnow][nANNZ_bias] *= weight_bias;
    }

    funcNameV.push_back((TString)" = "+funcName);

    // -----------------------------------------------------------------------------------------------------------
    // normalize the weights and create a plot
    // -----------------------------------------------------------------------------------------------------------
    double weightSum(0);
    for(int nAcptMLMnow=0; nAcptMLMnow<nAcptMLMs; nAcptMLMnow++) {
      int nANNZ_sigm  = sigm68V[nAcptMLMnow].first;
      weightSum      += weightsM[nPDFnow][nANNZ_sigm];
    }

    if(weightSum > 0) {
      // -----------------------------------------------------------------------------------------------------------
      // remove weights which are smaller than minPdfWeight after normalization
      // -----------------------------------------------------------------------------------------------------------
      int             nActiveWeights(0), nWhiles(0);
      vector <double> weightsV(nAcptMLMs,0);

      while(nActiveWeights < nAcptMLMs) {
        // normalize the weights
        for(int nAcptMLMnow=0; nAcptMLMnow<nAcptMLMs; nAcptMLMnow++) {
          int nANNZ_sigm = sigm68V[nAcptMLMnow].first;
          weightsM[nPDFnow][nANNZ_sigm] /= weightSum;
          weightsV[nAcptMLMnow]          = weightsM[nPDFnow][nANNZ_sigm];
        }

        // remove weights which are too small
        TString                     weightList("");
        vector < pair<int,double> > tmpV;
        nActiveWeights = 0; weightSum = 0;
        for(int nAcptMLMnow=0; nAcptMLMnow<nAcptMLMs; nAcptMLMnow++) {
          int nANNZ_sigm = sigm68V[nAcptMLMnow].first;
          if(weightsM[nPDFnow][nANNZ_sigm] < EPS) continue;

          if(weightsM[nPDFnow][nANNZ_sigm] < minPdfWeight) {
            tmpV.push_back( pair<int,double>(nANNZ_sigm,weightsM[nPDFnow][nANNZ_sigm]) );
          }

          nActiveWeights += 1;
          weightSum      += weightsM[nPDFnow][nANNZ_sigm];
          weightList     += (TString)utils->floatToStr(weightsM[nPDFnow][nANNZ_sigm])+",";          
        }
        // now remove the smallest weight before trying again...
        if((int)tmpV.size() > 0) {
          // sort so that the smallest element is first
          sort(tmpV.begin(),tmpV.end(),sortFunc::pairID::lowToHighBy1);

          int    nANNZ_min  = tmpV[0].first;
          double weight_min = tmpV[0].second;

          weightsM[nPDFnow][nANNZ_min] = 0; weightSum -= weight_min; nActiveWeights--;
        }
        else break;

        // TString all(""); double sum(0);
        // for(int nAcptMLMnow=0; nAcptMLMnow<nAcptMLMs; nAcptMLMnow++) {
        //   int nANNZ_sigm = sigm68V[nAcptMLMnow].first;
        //   if(weightsM[nPDFnow][nANNZ_sigm] < EPS) continue;
        //   all += (TString)utils->floatToStr(weightsM[nPDFnow][nANNZ_sigm])+", ";
        //   sum += weightsM[nPDFnow][nANNZ_sigm];
        // }
        // cout <<nWhiles<<CT<<sum<<CT<<all<<endl;
        // if((int)tmpV.size() < EPS) break;

        nWhiles++;
        tmpV.clear();

        // check that we did not remove all weights somewho
        VERIFY(LOCATION,(TString)"All derived weights ("+weightList+") truned out to be too small (\"minPdfWeight\" threshold = "
                                 +utils->floatToStr(minPdfWeight)+"). Can set \"minPdfWeight\" to lower value "+
                                 "(value smaller than zero will avoid small-weight removal altogether) ...", (weightSum > 0));
        VERIFY(LOCATION,(TString)"Somehow developed an infinite loop ... Please set \"minPdfWeight\" to "
                                 +"smaller than zero to avoid small-weight removal all together...", (nWhiles < 1e4));
      }

      // sort the vector used for plotting, and create a graph with the current weights
      // -----------------------------------------------------------------------------------------------------------
      sort(weightsV.begin(),weightsV.end(),sortFunc::highToLowD);

      vector <double> graph_X, graph_Y, graph_Xerr, graph_Yerr;

      for(int nAcptMLMnow=0; nAcptMLMnow<nAcptMLMs; nAcptMLMnow++) {
        graph_X   .push_back(nAcptMLMnow); graph_Y   .push_back(weightsV[nAcptMLMnow]);
        graph_Xerr.push_back(EPS);         graph_Yerr.push_back(EPS);
      }

      TGraphErrors * grph = new TGraphErrors(int(graph_X.size()),&graph_X[0], &graph_Y[0],&graph_Xerr[0], &graph_Yerr[0]);
      grph->SetName(TString::Format("pdfWeightGrph"+glob->GetOptC("_typeANNZ")+"_nPdf_%d",nPDFnow));
      grph->GetXaxis()->SetTitle("MLM index");
      grph->GetYaxis()->SetTitle("Weight");
      weightGrphV.push_back(grph);

      weightsV.clear(); graph_X.clear(); graph_Y.clear(); graph_Xerr.clear(); graph_Yerr.clear();
    }

    // cleanup
    DELNULL(pdfWeight_sigm); DELNULL(pdfWeight_bias); DELNULL(rnd);
  }


  // -----------------------------------------------------------------------------------------------------------
  // loop on the trees and fill the histograms with PDF results
  // -----------------------------------------------------------------------------------------------------------
  aLOG(Log::INFO) <<coutGreen<<" - will loop on the input trees ..."<<coutDef<<endl;

  if(seed > 0) { seed++; } rnd = new TRandom3(seed);

  // define a VarMaps to loop over the chain
  TString aChainName = aChain->GetName();
  VarMaps * var      = new VarMaps(glob,utils,(TString)"inputTreeVars_"+aChainName);
  var->connectTreeBranches(aChain);

  // loop on the tree
  // -----------------------------------------------------------------------------------------------------------
  bool  breakLoop(false);
  int   nObjectsToWrite(glob->GetOptI("nObjectsToWrite"));
  var->clearCntr();
  for(Long64_t loopEntry=0; true; loopEntry++) {
    if(!var->getTreeEntry(loopEntry)) breakLoop = true;

    if((var->GetCntr("nObj")+1 % nObjectsToWrite == 0) || breakLoop) { var->printCntr(aChainName,Log::DEBUG_1); }
    if(breakLoop) break;
    
    var->IncCntr("nObj"); if(var->GetCntr("nObj") == maxNobj) breakLoop = true;

    // get the target value and smear the reg-value of the best MLM
    double  zTrg     = var->GetVarF(zTrgName);
    double  zBest    = var->GetVarF(MLMnameBest);
    double  zBestErr = var->GetVarF(MLMnameBest_e);  if(zBestErr < EPS) continue;
    double  zBestSF  = rnd->Gaus(0,zBestErr);
    double  zBestNow = zBest + zBestSF;

    // loop over all PDFs
    for(int nPDFnow=0; nPDFnow<nTryPDFs; nPDFnow++) {
      double  intgrZtrg(0), intgrZreg(0), zPdfAvg(0), zPdfWgt(0);

      // loop over all MLMs
      for(int nMLMnow=0; nMLMnow<nMLMs; nMLMnow++) {
        if(weightsM[nPDFnow][nMLMnow] < EPS) continue;

        TString MLMname    = getTagName(nMLMnow);      if(mlmSkip[MLMname]) continue;
        TString MLMname_w  = getTagWeight(nMLMnow);
        TString MLMname_eN = getTagError(nMLMnow,"N"); TString MLMname_eP = getTagError(nMLMnow,"P");

        double  regNow    = var->GetVarF(MLMname);
        double  errN      = var->GetVarF(MLMname_eN); if(errN      < EPS) continue;
        double  errP      = var->GetVarF(MLMname_eP); if(errP      < EPS) continue;
        double  weightNow = var->GetVarF(MLMname_w);  if(weightNow < EPS) continue;

        // generate random smearing factors for this MLM
        for(int nSmearRndNow=0; nSmearRndNow<nSmearsRnd; nSmearRndNow++) {
          int     signNow(-1);
          double  errNow(errN);
          if(nSmearRndNow < nSmearsRndHalf) { errNow = errP; signNow = 1; }

          double  sfNow      = signNow * fabs(rnd->Gaus(0,errNow));
          double  zReg       = regNow + sfNow;
          double  weightFull = weightsM[nPDFnow][nMLMnow] * weightNow;

          if(zReg < zTrg)     intgrZtrg += weightFull;
          if(zReg < zBestNow) intgrZreg += weightFull;

          zPdfAvg += weightFull * zReg;
          zPdfWgt += weightFull;

          if(doBiasCorPDF) hisPdfTryBiasCorV[nPDFnow]->Fill(zReg,zTrg,weightFull);
        }
      }
      if(zPdfWgt < EPS) break; // no need to go on with all PDFs - this object must has zero weights

      // normalize everything
      intgrZtrg /= zPdfWgt; intgrZreg /= zPdfWgt; zPdfAvg /= zPdfWgt;
      
      // fill the histograms
      hisIntgrZtrgV[nPDFnow]->Fill(min(max(intgrZtrg,EPS),1-EPS));
      hisIntgrZregV[nPDFnow]->Fill(min(max(intgrZreg,EPS),1-EPS));
      
      double sclBias(zPdfAvg-zTrg);
      if(optimWithSclBias) {
        double denom = 1 + zTrg;
        if(fabs(denom) < EPS) sclBias  = DefOpts::DefF;
        else                  sclBias /= denom;
      }

      his2_N[nPDFnow]->Fill(sclBias,zTrg);      
    }

  }
  if(!breakLoop) { var->printCntr(aChainName,Log::DEBUG_1); }

  DELNULL(var); DELNULL(rnd);

  // normalize the histograms
  for(int nPDFnow=0; nPDFnow<nTryPDFs; nPDFnow++) {
    double intgrS = hisIntgrZtrgV[nPDFnow]->Integral(); if(intgrS > 0) hisIntgrZtrgV[nPDFnow]->Scale(1/intgrS);
    double intgrP = hisIntgrZregV[nPDFnow]->Integral(); if(intgrP > 0) hisIntgrZregV[nPDFnow]->Scale(1/intgrP);
  }

  // -----------------------------------------------------------------------------------------------------------
  // generate MC pdfs from the average bias/sigma metrics as a function of zTrg
  // -----------------------------------------------------------------------------------------------------------
  vector <TH1*> his1_N(nTryPDFs,NULL);

  for(int nPDFnow=0; nPDFnow<nTryPDFs; nPDFnow++) {
    aLOG(Log::INFO) <<coutPurple<<" - generating MC ("<<coutGreen<<nModels<<" models"<<coutPurple<<") for nPDFnow = "
                    <<coutBlue<<nPDFnow+1<<coutPurple<<"/"<<coutBlue<<nTryPDFs<<coutPurple<<" ..."<<coutDef<<endl;

    his1_N[nPDFnow] = ((TH2*)his2_N[nPDFnow])->ProjectionY((TString)his2_N[nPDFnow]->GetName()+"_proj");

    // sanity check
    VERIFY(LOCATION,(TString)"For ["+(TString)his1_N[nPDFnow]->GetName()+"], found no entries ..."
                            +"something is horribly wrong ?!?",(his1_N[nPDFnow]->Integral() > EPS));

    vector <TH1*> his1_NzV;
    utils->his2d_to_his1dV(NULL,his2_N[nPDFnow],his1_NzV);

    // create splines for generating the values of the metrics
    his1_d[nPDFnow]->Reset(); his1_s[nPDFnow]->Reset();
    for(int nHisNow=0; nHisNow<(int)his1_NzV.size(); nHisNow++) {
      utils->param->clearAll();
      utils->param->NewOptB("getMAD",optimWithMAD);
      if(!utils->getInterQuantileStats(his1_NzV[nHisNow])) continue;

      double  delta = utils->param->GetOptF("quant_mean");
      double  sigma = utils->param->GetOptF(quantScatterName);

      // cout << his1_NzV[nHisNow]->GetName()<<CT<< optimWithMAD <<CT<<quantScatterName <<CT<<sigma
      //      <<CT<<utils->param->GetOptF("quant_sigma_68")<<endl;

      his1_d[nPDFnow]->SetBinContent(nHisNow+1,delta); his1_d[nPDFnow]->SetBinError(nHisNow+1,EPS);
      his1_s[nPDFnow]->SetBinContent(nHisNow+1,sigma); his1_s[nPDFnow]->SetBinError(nHisNow+1,EPS);
    }

    TSpline3 * spline_delta = new TSpline3(his1_d[nPDFnow]);
    TSpline3 * spline_sigma = new TSpline3(his1_s[nPDFnow]);

    if(seed > 0) { seed++; } rnd = new TRandom3(seed);

    // define functions for the different models as combination of Gaussian functions
    vector <TF1*> funcModelV(nModels);
    for(int nModelNow=0; nModelNow<nModels; nModelNow++) {
      TString funcName = TString::Format("funcModel_%d",nModelNow);

      // 4 functions of a single Gaussian
      if(nModelNow  < 4) {
        funcModelV[nModelNow] = new TF1(funcName,"gaus");
        funcModelV[nModelNow]->FixParameter(0,1);
      }
      // (nModels-4) functions of a convolution of two Gaussians
      // [ gaus(0) is a substitute for [0]*exp(-0.5*((x-[1])/[2])**2) and (0) means start numbering parameters at 0 ]
      else {
        funcModelV[nModelNow] = new TF1(funcName,"gaus(0)+gaus(3)");
      }

      funcModelV[nModelNow]->SetRange(minValZ,maxValZ);
    }

    // setup the parameters of the Gaussian functions using fractions of the metrics
    int nContinue(0);
    vector <double> sigFracForTitle(nModels);
    for(int nSimObjNow=0; nSimObjNow<nSimObjs; nSimObjNow++) {
      // sanity check indicating infinite loop
      if(nContinue > 10*nSimObjs) {
        aLOG(Log::WARNING) <<coutWhiteOnRed<<"Could not generate models ... This can happen if the initial evaluation of the "
                           <<"scatter metric is empty. This type of PDF can not be generated !!!"<<coutDef<<endl;
        
        for(int nModelNow=0; nModelNow<nModels; nModelNow++) hisIntgrZtrgSimV[nPDFnow][nModelNow]->Reset();
        break;
      }

      // generate random values and fill the simulated PDF objects in hisIntgrZtrgSimV
      double  zTrg  = his1_N[nPDFnow]->GetRandom();
      double  delta = spline_delta->Eval(zTrg);
      double  sigma = spline_sigma->Eval(zTrg);  if(sigma < EPS) { nContinue++; nSimObjNow--; continue; }

      for(int nModelNow=0; nModelNow<nModels; nModelNow++) {
        // single Gaussian functions
        if(nModelNow  < 4) {
          double  sigFrac = 0.95 - (nModelNow * 0.25);
          double  sigmNow = sigma * sigFrac;
          double  deltNow = rnd->Gaus(delta,(sigma - sigmNow));
          
          sigFracForTitle[nModelNow] = sigFrac;

          funcModelV[nModelNow]->FixParameter(1, zTrg+deltNow);
          funcModelV[nModelNow]->FixParameter(2, sigmNow      );
        }
        // convolution of two Gaussian functions
        else {
          double gausNorm = rnd->Rndm();
          double sigFrac0 = 0.8 - 0.25*(nModelNow-4);            double sigFrac1 = sigFrac0 * rnd->Rndm();
          double sigmNow0 = sigma * sigFrac0;                    double sigmNow1 = sigma * sigFrac1;
          double deltNow0 = rnd->Gaus(delta,(sigma - sigmNow0)); double deltNow1 = rnd->Gaus(delta,(sigma - sigmNow1));
          
          sigFracForTitle[nModelNow] = sigFrac0;

          funcModelV[nModelNow]->FixParameter(0, gausNorm      );
          funcModelV[nModelNow]->FixParameter(1, zTrg+deltNow0);
          funcModelV[nModelNow]->FixParameter(2, sigmNow0      );
          funcModelV[nModelNow]->FixParameter(3, 1-gausNorm    );
          funcModelV[nModelNow]->FixParameter(4, zTrg+deltNow1);
          funcModelV[nModelNow]->FixParameter(5, sigmNow1      );
        }

        double intgrZtrg = funcModelV[nModelNow]->Integral(minValZ,zTrg);
        double intgrNorm = funcModelV[nModelNow]->Integral(minValZ,maxValZ);
        
        double intgrRelZs(0);
        if(intgrNorm > 0) intgrRelZs = intgrZtrg / intgrNorm;
        else              intgrRelZs = (delta > 0) ? 1 - EPS : EPS;

        hisIntgrZtrgSimV[nPDFnow][nModelNow]->Fill(intgrRelZs);
      }
    }

    // normalization of the hisIntgrZtrgSimV histograms
    for(int nModelNow=0; nModelNow<nModels; nModelNow++) {
      double intgr =     hisIntgrZtrgSimV[nPDFnow][nModelNow]->Integral();
      if(intgr > 0)      hisIntgrZtrgSimV[nPDFnow][nModelNow]->Scale(1/intgr);

      if(nModelNow < 4) {
        hisIntgrZtrgSimV[nPDFnow][nModelNow]->SetTitle(TString::Format("G(%2.2f #sigma_{68})"         ,sigFracForTitle[nModelNow]));
      }
      else {
        hisIntgrZtrgSimV[nPDFnow][nModelNow]->SetTitle(TString::Format("G(%2.2f #sigma_{68}) #oplus G",sigFracForTitle[nModelNow]));
      }
    }

    sigFracForTitle.clear();
    
    for(int nFuncNow=0; nFuncNow<(int)funcModelV.size(); nFuncNow++) {
      DELNULL(funcModelV[nFuncNow]);
    }
    funcModelV.clear();

    DELNULL(spline_delta); DELNULL(spline_sigma); DELNULL(rnd);
    
    for(int nHisNow=0; nHisNow<(int)his1_NzV.size(); nHisNow++) DELNULL(his1_NzV[nHisNow]);
    his1_NzV.clear();
  }


  // -----------------------------------------------------------------------------------------------------------
  // fit a constant to the zReg integrated-weights histogram
  // -----------------------------------------------------------------------------------------------------------
  fitMetric_nPDF.clear(); fitMetric_nPDF.resize(nTryPDFs); fitMetric.clear(); fitMetric.resize(nTryPDFs); scaleFactorModel.clear();

  // fit limits for the histograms
  double fitBottom = hisIntgrZregV[0]->GetXaxis()->GetBinCenter(hisIntgrZregV[0]->GetXaxis()->FindBin(0+glob->GetOptF("excludeRangePdfModelFit")));
  double fitTop    = hisIntgrZregV[0]->GetXaxis()->GetBinCenter(hisIntgrZregV[0]->GetXaxis()->FindBin(1-glob->GetOptF("excludeRangePdfModelFit")));

  int doConstFit(0), doMeanFit(1);
  VERIFY(LOCATION,(TString)"can only set one of the two options",(doConstFit+doMeanFit == 1)); // consistency check

  aLOG(Log::DEBUG) <<coutLightBlue<<"----------------------------------------------------------------------------------------"<<coutDef<<endl;
  aLOG(Log::DEBUG) <<coutYellow   <<" - Fitting a constant to the zReg integrated-weights histogram (optimal result is 0.5)"  <<coutDef<<endl;
  aLOG(Log::DEBUG) <<coutLightBlue<<"----------------------------------------------------------------------------------------"<<coutDef<<endl;

  for(int nPDFnow=0; nPDFnow<nTryPDFs; nPDFnow++) {
    map < TString,double >  fitParMap;
    fitParMap["fitBottom"] = fitBottom; fitParMap["fitTop"] = fitTop;

    if(doConstFit == 1) {
      TString theFunc("pol0");
      utils->doPolyFit(hisIntgrZregV[nPDFnow],&fitParMap,theFunc);
      double  fitMetricNow = fitParMap["fitChiSquare"]/fitParMap["fitNDF"];

      fitMetric     [nPDFnow] = fitMetricNow;
      fitMetric_nPDF[nPDFnow] = pair<int,double>(nPDFnow,fabs(fitMetricNow - 1));

      aLOG(Log::DEBUG) <<coutRed<<" - Fit const ("<<nPDFnow<<") chi^2/ndf = "<<coutBlue<<fitMetricNow<<coutRed
                       <<" , fit result = "<<coutPurple<<fitParMap["fitPar_0"]<<coutDef<<endl;
    }
    else {
      double mean = hisIntgrZregV[nPDFnow]->GetMean();
 
      fitMetric     [nPDFnow] = mean;
      fitMetric_nPDF[nPDFnow] = pair<int,double>(nPDFnow,fabs(mean - 0.5));
    
      aLOG(Log::DEBUG) <<coutRed<<" - Fit const ("<<nPDFnow<<") mean = "<<coutBlue<<mean<<coutDef<<endl;
    }

    fitParMap.clear();
  }

  // sort so that the smallest element is first
  sort(fitMetric_nPDF.begin(),fitMetric_nPDF.end(),sortFunc::pairID::lowToHighBy1);

  vector < TH1* > hisIntgrZregFirstFewV, hisVtmp;
  hisVtmp = hisIntgrZregV; hisIntgrZregV.clear();

  for(int nSortedPdfNow=0; nSortedPdfNow<nTryPDFs; nSortedPdfNow++) {
    int     nPDFnow      = fitMetric_nPDF[nSortedPdfNow].first;
    double  fitMetricNow = fitMetric[nPDFnow];

    TString funcName     = TString::Format((TString)"W_{%d}"+funcNameV[nPDFnow],nSortedPdfNow);
    
    vector <TString> titleV = utils->splitStringByChar(funcName,'=');
    TString titleNow("");
    if(doConstFit)  titleNow = TString::Format(titleV[0]+", #chi^{2}/NDF = %1.3f",fitMetricNow);
    else            titleNow = TString::Format(titleV[0]+", Mean = %1.3f",fitMetricNow);
    hisVtmp[nPDFnow]->SetTitle(titleNow);
    titleV.clear();

    hisIntgrZregV.push_back(hisVtmp[nPDFnow]);

    if(nSortedPdfNow < 5) {
      hisIntgrZregFirstFewV. push_back( (TH1*)hisVtmp[nPDFnow]->Clone((TString)hisVtmp[nPDFnow]->GetName()+"_firstFew") );
    }

    if(nSortedPdfNow == 0) {
      bestWeightsV_Zreg = weightsM[nPDFnow];
      hisPdfBiasV_Zreg  = hisPdfTryBiasCorV[nPDFnow];
    }

    aLOG(Log::DEBUG) <<coutGreen<<" -- Sorted minimization-metric of PDF int' weights ["
                     <<coutBlue<<funcName<<coutGreen<<"] -> "<<coutYellow<< fitMetricNow <<coutDef<<endl;
  }  
  aLOG(Log::DEBUG) <<coutLightBlue<<"... -------------------------------------------------------------------------------- ..."<<coutDef<<endl;
  aLOG(Log::DEBUG) <<coutPurple<<" - Best minimization-metric "<<coutBlue<<"(index = "<<fitMetric_nPDF[0].first<<")"
                   <<coutPurple<<" -> "<<coutYellow<<fitMetric[fitMetric_nPDF[0].first]<<coutDef<<endl;

  hisVtmp.clear();

  // -----------------------------------------------------------------------------------------------------------
  // fit a combination of models to the zTrg integrated-weights histogram
  // -----------------------------------------------------------------------------------------------------------
  fitMetric_nPDF.clear(); fitMetric_nPDF.resize(nTryPDFs); fitMetric.clear(); fitMetric.resize(nTryPDFs); scaleFactorModel.clear();

  aLOG(Log::DEBUG) <<coutLightBlue<<"------------------------------------------------------------------------------------------------------"<<coutDef<<endl;
  aLOG(Log::DEBUG) <<coutYellow   <<" - Fitting a combination of models to the zTrg integrated-weights histogram (optimal is chi^2/ndf=1)"  <<coutDef<<endl;
  aLOG(Log::DEBUG) <<coutLightBlue<<"------------------------------------------------------------------------------------------------------"<<coutDef<<endl;

  for(int nPDFnow=0; nPDFnow<nTryPDFs; nPDFnow++) {
    OptMaps * fitOpts = new OptMaps("fitOpts");

    fitOpts->NewOptF("fitBottom",fitBottom);
    fitOpts->NewOptF("fitTop",fitTop);
    for(int nModelNow=0; nModelNow<nModels; nModelNow++) {
      fitOpts->NewOptC(TString::Format("parLimits_%d",nModelNow),"0:10");
    }
    utils->doFitFuncByHisContent(fitOpts,hisIntgrZtrgV[nPDFnow],hisIntgrZtrgSimV[nPDFnow]);

    double fitMetricNow = (fitOpts->GetOptI("fitNDF") < EPS) ? 1e10 : fitOpts->GetOptF("fitChiSquare") / double(fitOpts->GetOptI("fitNDF"));

    for(int nModelNow=0; nModelNow<nModels; nModelNow++) {
      scaleFactorModel[nPDFnow].push_back(fitOpts->GetOptF(TString::Format("fitParVal_%d",nModelNow)));
    }

    fitMetric     [nPDFnow] = fitMetricNow;
    fitMetric_nPDF[nPDFnow] = pair<int,double>(nPDFnow,fabs(fitMetricNow -1));

    TString fitParsStr("");
    for(int nModelNow=0; nModelNow<nModels; nModelNow++) {
      fitParsStr += (TString)coutPurple+" , "+coutYellow+utils->floatToStr(fitOpts->GetOptF(TString::Format("fitParVal_%d",nModelNow)));
    }
    aLOG(Log::DEBUG) <<coutRed<<" - Fit model ("<<nPDFnow<<") chi^2/ndf = "<<coutBlue<<fitMetricNow<<coutRed<<" , fit results:"<< fitParsStr <<coutDef<<endl;

    DELNULL(fitOpts);
  }

  // sort so that the smallest element is first
  sort(fitMetric_nPDF.begin(),fitMetric_nPDF.end(),sortFunc::pairID::lowToHighBy1);
  
  hisVtmp.clear(); hisVtmp = hisIntgrZtrgV;
  hisIntgrZtrgV.clear();

  vector < TH1* >         hisIntgrZtrgFirstFewV, his1PlotV;
  vector < vector<TH1*> > his1PlotVV(3);
  TMultiGraph             * weightsMgrph         = new TMultiGraph();
  TMultiGraph             * weightsMgrphFirstFew = new TMultiGraph();

  int nPdfBestModel(0);
  for(int nSortedPdfNow=0; nSortedPdfNow<nTryPDFs; nSortedPdfNow++) {
    int     nPDFnow      = fitMetric_nPDF[nSortedPdfNow].first;
    double  fitMetricNow = fitMetric[nPDFnow];

    funcNameV[nPDFnow] = TString::Format((TString)"W_{%d}"+funcNameV[nPDFnow],nSortedPdfNow);
    
    vector <TString> titleV = utils->splitStringByChar(funcNameV[nPDFnow],'=');
    TString titleNow = TString::Format(titleV[0]+", #chi^{2}/NDF = %1.3f",fitMetricNow);
    fitTitleV.push_back(titleNow);
    hisVtmp[nPDFnow]->SetTitle(titleNow);
    titleV.clear();

    hisIntgrZtrgV.push_back(hisVtmp[nPDFnow]);

    weightGrphV[nPDFnow]->SetTitle(funcNameV[nPDFnow]); 
    weightsMgrph->Add(weightGrphV[nPDFnow]);

    if(nSortedPdfNow < 5) {
      hisIntgrZtrgFirstFewV. push_back( (TH1*)         hisVtmp    [nPDFnow]->Clone((TString)hisVtmp    [nPDFnow]->GetName()+"_firstFew") );
      weightsMgrphFirstFew  ->Add     ( (TGraphErrors*)weightGrphV[nPDFnow]->Clone((TString)weightGrphV[nPDFnow]->GetName()+"_firstFew") );
    }

    if(nSortedPdfNow % plotBinFreq == 0) {
      if(nSortedPdfNow == 0) his1PlotVV[0].push_back(his1_N[nPDFnow]);
      his1_d[nPDFnow]->SetTitle(titleNow); his1PlotVV[1].push_back(his1_d[nPDFnow]);
      his1_s[nPDFnow]->SetTitle(titleNow); his1PlotVV[2].push_back(his1_s[nPDFnow]);
    }

    if(nSortedPdfNow == 0) {
      nPdfBestModel     = nPDFnow;
      bestWeightsV_Ztrg = weightsM[nPDFnow];
      hisPdfBiasV_Ztrg  = hisPdfTryBiasCorV[nPDFnow];
    }

    aLOG(Log::DEBUG) <<coutGreen<<" -- Sorted minimization-metric of PDF int' weights ["
                     <<coutBlue<<funcNameV[nPDFnow]<<coutGreen<<"] -> "<<coutYellow<< fitMetricNow <<coutDef<<endl;
  }
  aLOG(Log::DEBUG) <<coutLightBlue<<"... -------------------------------------------------------------------------------- ..."<<coutDef<<endl;
  aLOG(Log::DEBUG) <<coutPurple<<" - Best minimization-metric "<<coutBlue<<"(index = "<<fitMetric_nPDF[0].first<<")"
                   <<coutPurple<<" -> "<<coutYellow<<fitMetric[fitMetric_nPDF[0].first]<<coutDef<<endl;
  aLOG(Log::DEBUG) <<coutLightBlue<<"... ------------------------------------------------------------------------------------"<<coutDef<<endl;

  hisVtmp.clear();


  // -----------------------------------------------------------------------------------------------------------
  // plots
  // -----------------------------------------------------------------------------------------------------------
  if(doPlots) {
    aLOG(Log::INFO) <<coutGreen<<" - Creating some plots ..."<<coutDef<<endl;
    
    // draw the integrated zTrg,zReg histograms
    // -----------------------------------------------------------------------------------------------------------
    for(int nHisType=0; nHisType<100; nHisType++) {
      if(nHisType == 0 || nHisType == 2 || nHisType == 3) continue; // don't really need these plots...

      outputs->optClear();
      outputs->draw->NewOptB("multiCnvs"   , true);
      outputs->draw->NewOptB("setLogY"     , (nHisType < 2 || nHisType == 3));
      outputs->draw->NewOptC("drawOpt"     , "ex0");
      outputs->draw->NewOptB("wideCnvs"    , true);
      outputs->draw->NewOptC("maxDrawMark" , "100");
      outputs->draw->NewOptC("axisTitleX"  , (TString)(((int)hisIntgrZtrgV.size() > 0) ? hisIntgrZtrgV[0]->GetXaxis()->GetTitle() : ""));
      outputs->draw->NewOptC("axisTitleY"  , (TString)(((int)hisIntgrZtrgV.size() > 0) ? hisIntgrZtrgV[0]->GetYaxis()->GetTitle() : ""));
      if(nHisType == 1                 ) outputs->draw->NewOptC("colOffset"                     , "1");
      if(nHisType == 1                 ) outputs->draw->NewOptC("generalHeader"                 , (TString)(((int)fitTitleV.size() > 0) ? fitTitleV[0] : ""));
      if(nHisType == 0 || nHisType == 3) outputs->draw->NewOptC("generalHeader"                 , (TString)"Model-fit by "+glob->GetOptC("zTrgTitle"));
      if(nHisType == 2 || nHisType == 4) outputs->draw->NewOptC("generalHeader"                 , (TString)"Constant-fit by "+glob->GetOptC("zRegTitle"));
      if(nHisType == 0 || nHisType == 3) outputs->draw->NewOptC(TString::Format("drawOpt_%d",0) , "HISTex0");
      if(nHisType == 3 || nHisType == 4) outputs->draw->NewOptC("onlyEverySuchBin"              , TString::Format("%d",plotBinFreq));

      if     (nHisType == 0) outputs->drawHis1dV(hisIntgrZtrgFirstFewV);
      else if(nHisType == 1) outputs->drawHis1dV(hisIntgrZtrgSimV[nPdfBestModel]); // all models - each normalized to unity
      else if(nHisType == 2) outputs->drawHis1dV(hisIntgrZregFirstFewV);
      else if(nHisType == 3) outputs->drawHis1dV(hisIntgrZtrgV);
      else if(nHisType == 4) outputs->drawHis1dV(hisIntgrZregV);
      else break;
    }

    // draw the average bias/scatter histograms
    // -----------------------------------------------------------------------------------------------------------
    outputs->optClear();
    outputs->draw->NewOptB("doIndividualPlots", glob->OptOrNullB("doIndividualPlots"));
    outputs->draw->NewOptC("generalHeader_0"  , (TString)"N(z) distribution");
    outputs->draw->NewOptC("generalHeader_1"  , (TString)"Bias ("+zTrgTitle+" bins)");
    outputs->draw->NewOptC("generalHeader_2"  , (TString)"Scatter ("+zTrgTitle+" bins)");
    outputs->draw->NewOptI("nPadsRow"         , 3);
    outputs->draw->NewOptB("multiCnvs"        , true);
    outputs->draw->NewOptC("drawOpt"          , "e1p");
    outputs->draw->NewOptC("maxDrawMark"      , "100");   
    outputs->drawHis1dMultiV(his1PlotVV);

    // draw the combined models and each individual component of the combination (scaled according
    // to its contribution in the pdf) for several of the proposed pdfs
    // -----------------------------------------------------------------------------------------------------------
    his1PlotVV.clear();
    for(int nSortedPdfNow=0; nSortedPdfNow<nTryPDFs; nSortedPdfNow++) {
      int nPDFnow = fitMetric_nPDF[nSortedPdfNow].first;
      
      his1PlotV.clear();
      his1PlotV.push_back(hisIntgrZtrgV[nSortedPdfNow]);
      
      // turn all very low-scalled contributions to something small (but reasonable for plotting), and compute normalization
      double  scaleNorm(0);
      for(int nModelNow=0; nModelNow<nModels; nModelNow++) {
        scaleFactorModel[nPDFnow][nModelNow] = max(scaleFactorModel[nPDFnow][nModelNow],1e-4);
        scaleNorm += scaleFactorModel[nPDFnow][nModelNow];
      }
      if(scaleNorm < EPS) continue; //sanity check

      for(int nModelNow=0; nModelNow<nModels+1; nModelNow++) {
        if(nModelNow<nModels) {
          double  scaleFactorModelNow = scaleFactorModel[nPDFnow][nModelNow];
          TString hisNameNow          = (TString)hisIntgrZtrgSimV[nPDFnow][nModelNow]->GetName()+"_normByFit";
          
          TH1 * hisModelNow = (TH1*)hisIntgrZtrgSimV[nPDFnow][nModelNow]->Clone(hisNameNow);
          his1PlotV.push_back(hisModelNow);
          
          hisModelNow->Scale(scaleFactorModelNow);
          if(scaleFactorModelNow < 1.0001e-4) hisModelNow->SetTitle((TString)"(< 1e-4) #times "+hisModelNow->GetTitle());
          else                                hisModelNow->SetTitle(TString::Format((TString)"%1.3f #times "+hisModelNow->GetTitle(),scaleFactorModelNow/scaleNorm));

          if(!hisModelAll[nPDFnow]) hisModelAll[nPDFnow] = (TH1*)hisModelNow->Clone((TString)hisNameNow+"_all");
          else                      hisModelAll[nPDFnow]->Add(hisModelNow);
        }
        else {
          hisModelAll[nPDFnow]->SetTitle("Combined models");
          his1PlotV.push_back(hisModelAll[nPDFnow]);
        }
      }
      
      // the best PDF has a plot for itself
      if(nSortedPdfNow == 0) {
        outputs->optClear();
        outputs->draw->NewOptC("generalHeader" , fitTitleV[0]);
        outputs->draw->NewOptB("multiCnvs"     , true);
        outputs->draw->NewOptB("wideCnvs"      , true);
        outputs->draw->NewOptC("drawOpt"       , "e1p");
        outputs->draw->NewOptB("setLogY"       , true);
        outputs->draw->NewOptC("maxDrawMark"   , "100");
        outputs->draw->NewOptC("axisTitleX"    , his1PlotV[0]->GetXaxis()->GetTitle());
        outputs->draw->NewOptC("axisTitleY"    , his1PlotV[0]->GetYaxis()->GetTitle());
        for(int nModelNow=0; nModelNow<nModels+1; nModelNow++) outputs->draw->NewOptC(TString::Format("drawOpt_%d",nModelNow+1) , "HIST");
        outputs->drawHis1dV(his1PlotV);
      }

      if(nSortedPdfNow % plotBinFreq == 0) his1PlotVV.push_back(his1PlotV);
    }

    outputs->optClear();
    outputs->draw->NewOptB("multiCnvs"   , true);
    outputs->draw->NewOptB("wideCnvs"    , true);
    outputs->draw->NewOptC("drawOpt"     , "e1p");
    outputs->draw->NewOptB("setLogY"     , true);
    outputs->draw->NewOptC("maxDrawMark" , "100");
    for(int nModelNow=0; nModelNow<nModels+1; nModelNow++) {
      outputs->draw->NewOptC(TString::Format("drawOpt_%d",nModelNow+1) , "HIST");
    }
    for(int nSortedPdfNow=0; nSortedPdfNow<nTryPDFs; nSortedPdfNow++) {
      if(nSortedPdfNow % plotBinFreq != 0) continue;
      outputs->draw->NewOptC(TString::Format("generalHeader_%d",static_cast<int>(floor(0.001+nSortedPdfNow/double(plotBinFreq)))), fitTitleV[nSortedPdfNow]);
    }
    outputs->drawHis1dMultiV(his1PlotVV);


    // draw the weights for the different PDF variations
    // -----------------------------------------------------------------------------------------------------------
    for(int nGrphType=0; nGrphType<2; nGrphType++) {
      TMultiGraph * mGrph = (nGrphType == 0) ? weightsMgrph : weightsMgrphFirstFew;

      if(nGrphType == 0) continue; // don't really need the full plot

      outputs->optClear();
      outputs->draw->NewOptB("wideCnvs" , true);
      outputs->draw->NewOptC("drawOpt" , "alp");
      outputs->draw->NewOptC("axisTitleX", "MLM index");
      outputs->draw->NewOptC("axisTitleY", "Weight");
      outputs->drawMultiGraph(mGrph);
    }

    outputs->WriteOutObjects(true,true); outputs->ResetObjects();
  }

  // -----------------------------------------------------------------------------------------------------------
  // store the results in the vector accosible by the outside world
  // -----------------------------------------------------------------------------------------------------------
  VERIFY(LOCATION,(TString)" - mismatch in ordering of PDFs ... Something is horribly wrong ?!?! ",(bestWeightsV.size() == 1 && hisPdfBiasCorV.size() == 1));
  
  // combination of models -> PDF_0
  bestWeightsV.push_back(bestWeightsV_Ztrg); hisPdfBiasCorV.push_back(hisPdfBiasV_Ztrg);
  // constant fit -> PDF_1
  if(nPDFs > 2) {
    bestWeightsV.push_back(bestWeightsV_Zreg); hisPdfBiasCorV.push_back(hisPdfBiasV_Zreg);
  }

  // -----------------------------------------------------------------------------------------------------------
  // cleanup
  // -----------------------------------------------------------------------------------------------------------
  for(int nPDFnow=0; nPDFnow<nTryPDFs; nPDFnow++) {
    DELNULL(hisIntgrZtrgV[nPDFnow]); DELNULL(hisIntgrZregV[nPDFnow]);

    if(nPDFnow < (int)hisIntgrZtrgFirstFewV.size()) DELNULL(hisIntgrZtrgFirstFewV[nPDFnow]);
    if(nPDFnow < (int)hisIntgrZregFirstFewV.size()) DELNULL(hisIntgrZregFirstFewV[nPDFnow]);
    
    DELNULL(his1_N[nPDFnow]); DELNULL(his2_N[nPDFnow]); DELNULL(his1_d[nPDFnow]); DELNULL(his1_s[nPDFnow]); DELNULL(hisModelAll[nPDFnow]);
 
    for(int nModelNow=0; nModelNow<nModels; nModelNow++) DELNULL(hisIntgrZtrgSimV[nPDFnow][nModelNow]);
  }
  hisIntgrZtrgV.clear(); hisIntgrZregV.clear(); hisIntgrZtrgFirstFewV.clear();
  his1_N.clear(); his2_N.clear(); his1_d.clear(); his1_s.clear(); hisIntgrZtrgSimV.clear();
  weightGrphV.clear(); sigm68V.clear(); biasV.clear(); weightsM.clear(); funcNameV.clear();
  his1PlotV.clear(); his1PlotVV.clear(); bestWeightsV_Ztrg.clear(); bestWeightsV_Zreg.clear();
  fitMetric_nPDF.clear(); fitMetric.clear(); scaleFactorModel.clear(); fitTitleV.clear();

  for(int nPDFnow=0; nPDFnow<nTryPDFs; nPDFnow++) {
    if(hisPdfTryBiasCorV[nPDFnow] == hisPdfBiasV_Ztrg || hisPdfTryBiasCorV[nPDFnow] == hisPdfBiasV_Zreg) continue;
    DELNULL(hisPdfTryBiasCorV[nPDFnow]);
  }
  hisPdfTryBiasCorV.clear();

  return;
}


// ===========================================================================================================
/**
 * @brief             - Derive bin-weights, based on the difference betwen the width of
 *                    classification- and pdf-bins
 * 
 * @details           - We define dp as the probability density of a given classifier: For each classification bin, we compute dp as the 
 *                    classification-probability of the corresponding MLM, divided by the width of the classification bin. Then, 
 *                    binWgt is the weight required to compute the average of dp for a given PDF bin. the average is defined
 *                    over all of the classification bins which overlap the PDF bin.
 *                    in actually:
 *                      double areaOfOverlap = (min(clsBinE_1,pdfBinE_1) - max(clsBinE_0,pdfBinE_0)) / (clsBinE_1 - clsBinE_0);
 *                      double avgWeight     = (min(clsBinE_1,pdfBinE_1) - max(clsBinE_0,pdfBinE_0)) / (pdfBinE_1 - pdfBinE_0);
 *                      double binWgt        = areaOfOverlap * avgWeight;
 *                    so we can just compute the epxression:
 * 
 * @param pdfBinWgt   - Will hold the weights
 * @param nClsBinsIn  - Will hold the number of derived weights
 */
// ===========================================================================================================
void ANNZ::setBinClsPdfBinWeights(
  vector < vector < pair<int,double> > > & pdfBinWgt, vector <int> & nClsBinsIn
) {
// ===========================================================================================================

  int nPDFbins = glob->GetOptI("nPDFbins");
  int nClsBins = (int)zBinCls_binC.size();

  pdfBinWgt.resize(nPDFbins); nClsBinsIn.resize(nPDFbins,0);

  aLOG(Log::DEBUG) <<coutCyan<<LINE_FILL('-',112)<<coutDef<<endl;
  aLOG(Log::DEBUG) <<coutGreen<< " - pdf bin, pdfBin-edges, "<<coutBlue
                   <<"classification-bin, clsBin-edges, "<<coutYellow<<" clsBin-weights -"<<coutDef<<endl;
  aLOG(Log::DEBUG) <<coutCyan<<LINE_FILL('-',112)<<coutDef<<endl;

  for(int nPdfBinNow=0; nPdfBinNow<nPDFbins; nPdfBinNow++) {
    double  pdfBinE_0 = zPDF_binE[nPdfBinNow];
    double  pdfBinE_1 = zPDF_binE[nPdfBinNow+1];

    for(int nClsBinNow=0; nClsBinNow<nClsBins; nClsBinNow++) {
      double  clsBinE_0 = zBinCls_binE[nClsBinNow];
      double  clsBinE_1 = zBinCls_binE[nClsBinNow+1];

      if(clsBinE_0 > pdfBinE_1 || clsBinE_1 < pdfBinE_0) continue;

      double binWgt = pow((min(clsBinE_1,pdfBinE_1) - max(clsBinE_0,pdfBinE_0)),2) / ((clsBinE_1 - clsBinE_0) * (pdfBinE_1 - pdfBinE_0));

      if(binWgt < EPS) continue;

      pdfBinWgt[nPdfBinNow].push_back( pair<int,double>(nClsBinNow,binWgt) );

      aLOG(Log::DEBUG) <<std::left<<coutGreen<<" - "<<std::setw(6)<<nPdfBinNow<<" "<<std::setw(14)<<pdfBinE_0<<" "<<std::setw(14)<<pdfBinE_1
                       <<" "<<coutBlue<<std::setw(6)<<nClsBinNow<<" "<<std::setw(14)<<clsBinE_0<<" "<<std::setw(14)<<clsBinE_1<<" "
                       <<coutYellow<<std::setw(14)<<binWgt<<coutDef<<endl;
    }
    nClsBinsIn[nPdfBinNow] = (int)pdfBinWgt[nPdfBinNow].size();
  }

  aLOG(Log::DEBUG) <<coutCyan<<LINE_FILL('-',112)<<coutDef<<endl;

  return;
}


// ===========================================================================================================
/**
 * @brief                 - Derive the pdf bias-correction histograms for binned classification pdfs
 * 
 * @param aChain          - Input chain, with titch to do the claculation.
 * @param hisPdfBiasCorV  - Will hold the derived histograms
 */
// ===========================================================================================================
void  ANNZ::getBinClsBiasCorPDF(TChain * aChain, vector <TH2*>  & hisPdfBiasCorV) {
// ===========================================================================================================
  aLOG(Log::INFO) <<coutWhiteOnBlack<<coutPurple<<" - starting ANNZ::getBinClsBiasCorPDF() ... "<<coutDef<<endl;

  int     maxNobj         = glob->GetOptI("maxNobj");
  int     nObjectsToWrite = glob->GetOptI("nObjectsToWrite");
  TString zTrgName        = glob->GetOptC("zTrg");
  TString aChainName      = (TString)aChain->GetName();
  int     nPDFs           = glob->GetOptI("nPDFs");
  int     nPDFbins        = glob->GetOptI("nPDFbins");
  TString _typeANNZ       = glob->GetOptC("_typeANNZ");
  int     nSmearsRnd      = glob->GetOptI("nSmearsRnd");
  int     nSmearsRndHalf  = static_cast<int>(floor(0.01 + nSmearsRnd/2.));
  UInt_t  seed            = glob->GetOptI("initSeedRnd"); if(seed > 0) seed += 15229;
  TRandom * rnd           = new TRandom(seed);

  vector < vector < pair<int,double> > > pdfBinWgt;
  vector < int >                         nClsBinsIn;
  setBinClsPdfBinWeights(pdfBinWgt,nClsBinsIn);

  hisPdfBiasCorV.resize(nPDFs,NULL);
  for(int nPDFnow=0; nPDFnow<nPDFs; nPDFnow++) {
    TString nPDFname        = TString::Format("_nPdf%d",nPDFnow);
    TString hisName         = (TString)"pdfBias"+_typeANNZ+nPDFname;
    hisPdfBiasCorV[nPDFnow] = new TH2F(hisName,hisName,nPDFbins,&(zPDF_binE[0]),nPDFbins,&(zPDF_binE[0]));
  }

  VarMaps * var = new VarMaps(glob,utils,"treeRegVar");
  var->connectTreeBranches(aChain);

  // -----------------------------------------------------------------------------------------------------------
  // loop on the tree
  // -----------------------------------------------------------------------------------------------------------
  bool breakLoop(false);
  var->clearCntr();
  for(Long64_t loopEntry=0; true; loopEntry++) {
    if(!var->getTreeEntry(loopEntry)) breakLoop = true;

    if((var->GetCntr("nObj")+1 % nObjectsToWrite == 0) || breakLoop) var->printCntr(aChainName,Log::DEBUG);
    if(breakLoop) break;

    double zTrg = var->GetVarF(zTrgName);

    for(int nPDFnow=0; nPDFnow<nPDFs; nPDFnow++) {
      // go over all pdf bins
      for(int nPdfBinNow=0; nPdfBinNow<nPDFbins; nPdfBinNow++) {
        // in each pdf-bin, use the overlapping cls-bins
        for(int nClsBinNow=0; nClsBinNow<nClsBinsIn[nPdfBinNow]; nClsBinNow++) {
          int    clsIndex   = pdfBinWgt[nPdfBinNow][nClsBinNow].first;
          double binWgt     = pdfBinWgt[nPdfBinNow][nClsBinNow].second;

          TString MLMname   = getTagName(clsIndex);
          TString MLMname_e = getTagError(clsIndex); TString MLMname_w = getTagWeight(clsIndex);

          double  binVal    = max(min(var->GetVarF(MLMname),1.),0.);
          double  clsWgt    = var->GetVarF(MLMname_w);
          double  totWgt    = binVal * binWgt * clsWgt;

          hisPdfBiasCorV[nPDFnow]->Fill(zPDF_binC[nPdfBinNow],zTrg,totWgt);

          // generate random smearing factors for one of the PDFs
          // -----------------------------------------------------------------------------------------------------------
          if(nPDFnow == 1) {
            double clsErr = var->GetVarF(MLMname_e);

            if(clsErr > EPS) {
              for(int nSmearRndNow=0; nSmearRndNow<nSmearsRnd; nSmearRndNow++) {
                double sfNow     = fabs(rnd->Gaus(0,clsErr));        if(nSmearRndNow < nSmearsRndHalf) sfNow *= -1;
                double binSmr    = max(min((binVal + sfNow),1.),0.);
                double totWgtSmr = binSmr * binWgt * clsWgt;

                hisPdfBiasCorV[nPDFnow]->Fill(zPDF_binC[nPdfBinNow],zTrg,totWgtSmr);
              }
            }
          }

        }
      }
    }

    var->IncCntr("nObj"); if(var->GetCntr("nObj") == maxNobj) breakLoop = true;
  }
  if(!breakLoop) { var->printCntr(aChainName,Log::DEBUG); }

  DELNULL(var); DELNULL(rnd);
  pdfBinWgt.clear(); nClsBinsIn.clear();

  return;
}


// ===========================================================================================================
/**
 * @brief                  - Create performance plots for regression.
 *
 * @param aChain           - Input chain, the result of doEvalReg().
 * @param addPlotVarV      - Possible vector of MLM or variable names, which will be added to the list of solutions
 *                         for which plots are generated.
 * @param addOutputVarsIn  - additional variables that would get plots
 */
// ===========================================================================================================
void  ANNZ::doMetricPlots(TChain * aChain, vector <TString> * addPlotVarV, TString addOutputVarsIn) {
// ===========================================================================================================
  if(!glob->GetOptB("doPlots")) {
    aLOG(Log::DEBUG) <<coutWhiteOnBlack<<coutPurple<<" - skipping ANNZ::doMetricPlots() ... "<<coutDef<<endl;
    return;
  }

  VERIFY(LOCATION,(TString)"Memory leak ?! ",(dynamic_cast<TChain*>(aChain)));

  aLOG(Log::INFO) <<coutWhiteOnBlack<<coutPurple<<" - starting ANNZ::doMetricPlots() ... "<<coutDef<<endl;

  TString outDirNameFull      = glob->GetOptC("outDirNameFull");
  TString plotExt             = glob->GetOptC("printPlotExtension");
  TString basePrefix          = glob->GetOptC("basePrefix");
  TString addOutputVars       = glob->GetOptC("addOutputVars");
  TString alwaysPlotVars      = glob->GetOptC("alwaysPlotVars");
  TString baseName_regMLM_avg = glob->GetOptC("baseName_regMLM_avg");
  TString baseName_regPDF_max = glob->GetOptC("baseName_regPDF_max");
  TString baseName_regPDF_avg = glob->GetOptC("baseName_regPDF_avg");
  TString baseName_nPDF       = glob->GetOptC("baseName_nPDF");
  TString baseName_regBest    = glob->GetOptC("baseName_regBest");
  TString zTrgName            = glob->GetOptC("zTrg");
  TString zTrgTitle           = glob->GetOptC("zTrgTitle");
  TString zRegTitle           = glob->GetOptC("zRegTitle");
  int     maxNobj             = glob->GetOptI("maxNobj");
  TString baseTag_v           = glob->GetOptC("baseTag_v");
  TString baseTag_e           = glob->GetOptC("baseTag_e");
  TString baseTag_w           = glob->GetOptC("baseTag_w");
  TString userWgtPlots        = glob->GetOptC("userWeights_metricPlots");
  TString noQuantileBinsStr   = glob->GetOptC("noQuantileBinsPlots");
  bool    defErrBySigma68     = glob->GetOptB("defErrBySigma68");
  bool    doGausSigmaRelErr   = glob->GetOptB("doGausSigmaRelErr");
  int     nPDFs               = glob->GetOptI("nPDFs");
  int     nPDFbins            = glob->GetOptI("nPDFbins");
  double  minValZ             = glob->GetOptF("minValZ");
  double  maxValZ             = glob->GetOptF("maxValZ");
  bool    isBinCls            = glob->GetOptB("doBinnedCls");
  bool    addMaxPDF           = glob->GetOptB("addMaxPDF");
  bool    doStorePdfBins      = glob->GetOptB("doStorePdfBins");
  bool    doKnnErrPlots       = glob->GetOptB("doKnnErrPlots");
  int     closHisN            = glob->GetOptI("closHisN");
  int     hisBufSize          = glob->GetOptI("hisBufSize");

  bool    plotWithSclBias     = glob->GetOptB("plotWithScaledBias");
  TString biasTitle           = plotWithSclBias ? (TString)"#delta/(1+"+glob->GetOptC("zTrgTitle")+")" : (TString)"#delta";
  int     errTrgHisN          = 40;

  if(!doStorePdfBins) {
    aLOG(Log::INFO) <<coutRed<<" - Found \"doStorePdfBins\" = true ---> Will skip ANNZ::doMetricPlots()"<<coutDef
                    <<coutYellow<<" (It is allowed set \"doStorePdfBins\" to false for optimization, and then "
                    <<"to true for evaluation, in order to properly derive the optimization metric plots ...)"<<coutDef<<endl;
    return;
  }

  TString aChainName = (TString)aChain->GetName();

  // added variables are plotted with equal-quintile binning instead of equal-width
  // bins, if they are NOT included in this list
  vector <TString> noQuantileBinsV = utils->splitStringByChar(noQuantileBinsStr, ';');

  // combine a possible list of addOutputVars with that defined in the global variable
  if(addOutputVarsIn != "") {
    addOutputVarsIn.ReplaceAll(" ","").ReplaceAll(";;",";");

    if(addOutputVars != "") addOutputVars += ";";
    addOutputVars += addOutputVarsIn;
  }
  if(alwaysPlotVars != "") {
    alwaysPlotVars.ReplaceAll(" ","").ReplaceAll(";;",";");

    if(addOutputVars != "") addOutputVars += ";";
    addOutputVars += alwaysPlotVars;
  }
  addOutputVars.ReplaceAll(" ","").ReplaceAll(";;",";");

  // used for formula of user defined weights for the plots
  bool    hasUserWgt(userWgtPlots != "" && userWgtPlots != "1");
  TString userWgtFrm("userWgtFrm"); // some internal unique name

  TString hisName("");
  int     nBinsZ((int)zPlot_binC.size()), nBinsZtrg((int)zTrgPlot_binC.size()), maxSigmaRelErrToPlot(10);

  // single-estimator
  // -----------------------------------------------------------------------------------------------------------
  TString regBestNameVal  = getTagBestMLMname(baseTag_v);
  TString regBestNameErr  = getTagBestMLMname(baseTag_e);
  TString regBestNameErrN = getTagBestMLMname(baseTag_e+"N");
  TString regBestNameErrP = getTagBestMLMname(baseTag_e+"P");
  TString regBestNameWgt  = getTagBestMLMname(baseTag_w);

  // if we're only computing knn uncertanties, the single-estimator variable names are different
  // -----------------------------------------------------------------------------------------------------------
  if(glob->GetOptB("doOnlyKnnErr")) {
    int     nMLMnow  = glob->GetOptI("nMLMnow");
    TString zRegName = glob->GetOptC("zReg_onlyKnnErr");
    VERIFY(LOCATION,(TString)"For doOnlyKnnErr, found empty value of \"zReg_onlyKnnErr\" ?!?!",(zRegName != ""));

    regBestNameVal  = zRegName;
    regBestNameErr  = (getTagError(nMLMnow,"")) .ReplaceAll(getTagName(nMLMnow), zRegName);
    regBestNameErrN = (getTagError(nMLMnow,"N")).ReplaceAll(getTagName(nMLMnow), zRegName);
    regBestNameErrP = (getTagError(nMLMnow,"P")).ReplaceAll(getTagName(nMLMnow), zRegName);
    regBestNameWgt  = (glob->GetOptC("weights_onlyKnnErr") != "") ? glob->GetOptC("weights_onlyKnnErr") : glob->GetOptC("baseName_wgtKNN");
  }

  // pdfs
  // -----------------------------------------------------------------------------------------------------------
  int              nPdfTypes(addMaxPDF ? 3 : 2);
  vector <TString> tagNameV(nPdfTypes);
  tagNameV[0] = glob->GetOptC("baseTag_MLM_avg"); tagNameV[1] = glob->GetOptC("baseTag_PDF_avg");
  if(nPdfTypes > 2) tagNameV[2] = glob->GetOptC("baseTag_PDF_max");

  vector < TString > pdfTagWgtV(nPDFs), pdfTagErrV(nPDFs);
  for(int nPDFnow=0; nPDFnow<nPDFs; nPDFnow++) {
    TString pdfTagWgt   = (TString)glob->GetOptC("baseTag_w")+tagNameV[1];
    TString pdfTagErr   = (TString)glob->GetOptC("baseTag_e")+tagNameV[1];

    pdfTagWgtV[nPDFnow] = getTagPdfAvgName(nPDFnow,pdfTagWgt);
    pdfTagErrV[nPDFnow] = getTagPdfAvgName(nPDFnow,pdfTagErr);
  }

  VERIFY(LOCATION,(TString)"Somehow nBinsZ == 0 ... Something is horribly wrong ?!?! ",(nBinsZ > 0));

  // -----------------------------------------------------------------------------------------------------------
  // derive the list of MLMs and pdfs from the input chain
  // -----------------------------------------------------------------------------------------------------------
  vector <int>     nMLMsChain, nPDFsChain;
  vector <TString> branchNameV;
  int              nTagBestMLM(0);
  TString          allMLMsIn(""), allPDFsIn("");
  
  utils->getTreeBranchNames(aChain,branchNameV);

  // go over all branch names in the chain and identify MLM/PDF patterns
  for(int nBranchNow=0; nBranchNow<(int)branchNameV.size(); nBranchNow++) {
    TString branchName = branchNameV[nBranchNow];

    // search for format like "ANNZ_0":
    if(branchName.BeginsWith(basePrefix)) {
      TString nMLMstr(branchName); nMLMstr.ReplaceAll(basePrefix,"");

      if(nMLMstr.IsDigit()) {
        // if a selection vector is inputed, only accept MLMs which are listed in it
        if(addPlotVarV) {
          if(find(addPlotVarV->begin(),addPlotVarV->end(), branchName) == addPlotVarV->end()) continue;
        }

        int nMLMnow  = utils->strToInt(nMLMstr);
        allMLMsIn   += (TString)coutGreen+branchName+coutPurple+",";
        nMLMsChain.push_back(nMLMnow);
      }
    }

    // the best MLM, its error and its weight
    if(branchName == regBestNameVal || branchName == regBestNameErr || branchName == regBestNameWgt) nTagBestMLM++;

    // search for format like "ANNZ_PDF_avg_0"
    if(branchName.BeginsWith(baseName_regPDF_avg)) {
      TString nPDFstr(branchName); nPDFstr.ReplaceAll(baseName_regPDF_avg,"");

      if(nPDFstr.IsDigit()) {
        int nPDFnow  = utils->strToInt(nPDFstr);
        allPDFsIn   += (TString)coutPurple+"PDF_"+nPDFstr+coutGreen+",";
        nPDFsChain.push_back(nPDFnow);
      }
    }
  }
  // sort so that the smallest element is first
  sort(nMLMsChain.begin(),nMLMsChain.end(),sortFunc::lowToHighI);
  sort(nPDFsChain.begin(),nPDFsChain.end(),sortFunc::lowToHighI);


  // -----------------------------------------------------------------------------------------------------------
  // store the list of all MLMs, average PDF solutions and PDF bins, together with corresponding errors,
  // weights and titles. Perform sanity checks that all branches exist along the way
  // -----------------------------------------------------------------------------------------------------------
  vector < TString >          nameV_MLM_v, nameV_MLM_w, titleV_MLM, titleV_PDF, nameV_PDF, nameV_MLM_e, nameV_MLM_eN, nameV_MLM_eP;
  vector < vector <TString> > nameV_PDF_b;

  if(nTagBestMLM > 0) {
    nameV_MLM_v.push_back(regBestNameVal); nameV_MLM_e.push_back(regBestNameErr);
    nameV_MLM_w.push_back(regBestNameWgt); titleV_MLM.push_back(baseName_regBest);

    allMLMsIn = (TString)coutGreen+regBestNameVal+coutPurple+","+allMLMsIn;

    VERIFY(LOCATION,(TString)"Found only "+utils->intToStr(nTagBestMLM)+" of "+regBestNameVal+", "+regBestNameErr+", "+regBestNameWgt
                            +" in the input tree... something is horribly wrong ?!?",(nTagBestMLM == 3));
    
    if(doKnnErrPlots) {
      bool found_eN = (find(branchNameV.begin(),branchNameV.end(), regBestNameErrN) != branchNameV.end());
      bool found_eP = (find(branchNameV.begin(),branchNameV.end(), regBestNameErrP) != branchNameV.end());

      TString errN(regBestNameErrN), errP(regBestNameErrP);
      if(!found_eN || !found_eP) {
        errN = ""; errP = "";
      }
      nameV_MLM_eN.push_back(errN); nameV_MLM_eP.push_back(errP);
    }
  }

  // go over PDFs
  // -----------------------------------------------------------------------------------------------------------
  for(int nPDFinNow=0; nPDFinNow<(int)nPDFsChain.size(); nPDFinNow++) {
    int     nPDFnow    = nPDFsChain[nPDFinNow];   
    int     nPdfsSoFar = (int)nameV_PDF_b.size();
    TString titlePDF   = (TString)baseName_nPDF+utils->intToStr(nPDFnow);

    nameV_PDF_b.push_back(vector<TString>(nPDFbins,""));
    nameV_PDF.push_back(titlePDF); titleV_PDF.push_back(titlePDF);

    // go over all PDF bins
    // -----------------------------------------------------------------------------------------------------------
    for(int nPdfBinNow=0; nPdfBinNow<nPDFbins; nPdfBinNow++) {
      TString pdfBinName = getTagPdfBinName(nPDFnow,nPdfBinNow);

      nameV_PDF_b[nPdfsSoFar][nPdfBinNow] = pdfBinName;
      
      bool found_allPDFbin = (find(branchNameV.begin(),branchNameV.end(), pdfBinName) != branchNameV.end());

      VERIFY(LOCATION,(TString)"Did not find the expected "+utils->intToStr(nPDFbins)+" bins of (nPDFnow = "
                               +utils->intToStr(nPDFnow)+") ... Something is horribly wrong ?!?!",found_allPDFbin);
    }

    // search for format like "ANNZ_MLM_avg_0", "ANNZ_MLM_avg_0_err" and "ANNZ_MLM_avg_0_wgt"
    for(int nPdfTypeNow=0; nPdfTypeNow<nPdfTypes; nPdfTypeNow++) {
      if(isBinCls && nPdfTypeNow == 0) continue;

      // since the baseTag_PDF_max does not have its own branches for the error and weight, we use
      // the corresponding values for baseTag_PDF_avg for pdfAvgErrName and pdfAvgWgtName
      TString pdfAvgName    = getTagPdfAvgName(nPDFnow,(TString)baseTag_v+tagNameV[                        nPdfTypeNow ]);
      TString pdfAvgErrName = getTagPdfAvgName(nPDFnow,(TString)baseTag_e+tagNameV[ (nPdfTypeNow==2) ? 1 : nPdfTypeNow ]);
      TString pdfAvgWgtName = getTagPdfAvgName(nPDFnow,(TString)baseTag_w+tagNameV[ (nPdfTypeNow==2) ? 1 : nPdfTypeNow ]);

      nameV_MLM_v.push_back(pdfAvgName);    nameV_MLM_e.push_back(pdfAvgErrName);
      nameV_MLM_w.push_back(pdfAvgWgtName); titleV_MLM.push_back((TString)titlePDF+" ("+tagNameV[nPdfTypeNow]+" avg.)"); 

      bool found_MLMavg = (   (find(branchNameV.begin(),branchNameV.end(), pdfAvgName)    != branchNameV.end())
                           && (find(branchNameV.begin(),branchNameV.end(), pdfAvgErrName) != branchNameV.end())
                           && (find(branchNameV.begin(),branchNameV.end(), pdfAvgWgtName) != branchNameV.end()) );

      VERIFY(LOCATION,(TString)"Did not find "+pdfAvgName+" of (nPDFnow = "
                               +utils->intToStr(nPDFnow)+") ... Something is horribly wrong ?!?!",found_MLMavg);
    
      if(doKnnErrPlots) {
        nameV_MLM_eN.push_back(""); nameV_MLM_eP.push_back("");
      }
    }
  }

  // go over MLMs
  // -----------------------------------------------------------------------------------------------------------
  for(int nMLMinNow=0; nMLMinNow<(int)nMLMsChain.size(); nMLMinNow++) {
    int nMLMnow       = nMLMsChain[nMLMinNow]; TString MLMname   = getTagName(nMLMnow);
    TString MLMname_e = getTagError(nMLMnow);  TString MLMname_w = getTagWeight(nMLMnow);

    nameV_MLM_v.push_back(MLMname);   nameV_MLM_e.push_back(MLMname_e);
    nameV_MLM_w.push_back(MLMname_w); titleV_MLM.push_back(MLMname);

    bool found_e = (find(branchNameV.begin(),branchNameV.end(), MLMname_e) != branchNameV.end());
    bool found_w = (find(branchNameV.begin(),branchNameV.end(), MLMname_w) != branchNameV.end());

    VERIFY(LOCATION,(TString)"Did not find "+MLMname_e+" , "+MLMname_w+" ... Something is horribly wrong ?!?!",(found_w && found_e));

    if(doKnnErrPlots) {
      TString MLMname_eN = getTagError(nMLMnow,"N"); TString MLMname_eP = getTagError(nMLMnow,"P");

      bool found_eN = (find(branchNameV.begin(),branchNameV.end(), MLMname_eN) != branchNameV.end());
      bool found_eP = (find(branchNameV.begin(),branchNameV.end(), MLMname_eP) != branchNameV.end());

      if(!found_eN || !found_eP) {
        MLMname_eN = ""; MLMname_eP = "";
      }
      nameV_MLM_eN.push_back(MLMname_eN); nameV_MLM_eP.push_back(MLMname_eP);
    }
  }

  branchNameV.clear(); nMLMsChain.clear(); nPDFsChain.clear();

  int nMLMsIn((int)nameV_MLM_v.size()), nPDFsIn((int)nameV_PDF.size());

  if(allMLMsIn != "") aLOG(Log::INFO)<<coutYellow<<" - Will use ("<<nMLMsIn-2*nPDFsIn<<") MLMs from the input chain: "<<allMLMsIn<<coutDef<<endl;
  if(allPDFsIn != "") aLOG(Log::INFO)<<coutYellow<<" - Will use ("<<nPDFsIn          <<") PDFs from the input chain: "<<allPDFsIn<<coutDef<<endl;

  
  // -----------------------------------------------------------------------------------------------------------
  // create the vars to read chain and validate the requested added plotting variables
  // -----------------------------------------------------------------------------------------------------------
  VarMaps * var = new VarMaps(glob,utils,"treePlotVar");
  var->connectTreeBranches(aChain);

  vector <TString> plotVars, plotVarForms, plotVarNames, addVarInV, alwaysPlotV;
  plotVarNames = utils->splitStringByChar(addOutputVars,  ';');
  addVarInV    = utils->splitStringByChar(addOutputVarsIn,';');
  alwaysPlotV  = utils->splitStringByChar(alwaysPlotVars, ';');
  
  for(int nNameNow=0; nNameNow<(int)plotVarNames.size(); nNameNow++) {
    TString plotVarNameNow     = (TString)plotVarNames[nNameNow];
    TString plotVarFormNameNow = (TString)"form_"+plotVarNameNow;

    // avoid empties
    if(plotVarNameNow == "") continue;
    // avoid double counting
    if(find(plotVars.begin(),plotVars.end(),plotVarNameNow) != plotVars.end()) continue;

    bool doPlotVar =    (find(addVarInV  .begin(),addVarInV  .end(),plotVarNameNow) != addVarInV  .end())
                     || (find(alwaysPlotV.begin(),alwaysPlotV.end(),plotVarNameNow) != alwaysPlotV.end());

    if(var->HasVarF(plotVarNameNow) || var->HasVarI(plotVarNameNow)) {
      plotVars    .push_back(plotVarNameNow);
      plotVarForms.push_back(plotVarFormNameNow);
    }
    else if(doPlotVar) {
      plotVars    .push_back(plotVarNameNow);
      plotVarForms.push_back(utils->regularizeName(plotVarFormNameNow));

      aLOG(Log::DEBUG_1)<<coutBlue<<" - Added plotting variable - "<<coutYellow<<plotVarNameNow<<coutBlue<<" ..."<<coutDef<<endl;
    }
    else {
      aLOG(Log::INFO)<<coutRed<<" - Requested variable ("<<coutYellow<<plotVarNameNow<<coutRed
                     <<") is not a float, and will not be plotted against..."<<coutDef<<endl;
    }
  }
  plotVarNames.clear(); addVarInV.clear(); alwaysPlotV.clear();

  // recreate the var with the requested plotting variables as NewForm()
  DELNULL(var);
  var = new VarMaps(glob,utils,"treePlotVar");

  for(int nVarNow=0; nVarNow<(int)plotVars.size(); nVarNow++) {
    TString plotVarNameNow     = plotVars[nVarNow];
    TString plotVarFormNameNow = plotVarForms[nVarNow];

    var->NewForm(plotVarFormNameNow,plotVarNameNow);
  }

  // add formula for user defined weights for the plots
  if(hasUserWgt) {
    aLOG(Log::INFO)<<coutCyan<<" - detected user-requested weight expression for plotting [\"userWeights_metricPlots\" = "
                   <<coutYellow<<userWgtPlots<<coutCyan<<"]"<<coutDef<<endl;

    var->NewForm(userWgtFrm,getRegularStrForm(userWgtPlots,var));
  }

  var->connectTreeBranches(aChain);

  int nTypeBins = 2 + (int)plotVars.size();

  vector < vector<double> > varPlot_binE, varPlot_binC;
  if(nTypeBins > 2) {
    varPlot_binE.resize(nTypeBins-2,vector<double>(nBinsZ+1,0));
    varPlot_binC.resize(nTypeBins-2,vector<double>(nBinsZ  ,0));

    hisName = "his1_TMP";

    // get the cuts
    TString cutStr("_comn"), treeNamePostfix("");

    if(((TString)aChain->GetName()).EndsWith("_valid")) treeNamePostfix = "_valid";
    if(((TString)aChain->GetName()).EndsWith("_train")) treeNamePostfix = "_train";

    int nMLMnowTag = max(0,glob->GetOptI("nMLMnow")); // needed for e.g., single-regression
    if(treeNamePostfix != "") cutStr += ";"+getTagName(nMLMnowTag)+treeNamePostfix;

    TString treeCuts = (TString)getTrainTestCuts(cutStr,nMLMnowTag,0,0,var);

    for(int nTypeBinNow=0; nTypeBinNow<nTypeBins-2; nTypeBinNow++) {
      TString drawExprs = (TString)plotVars[nTypeBinNow]+">>"+hisName;

      bool useQuantileBins = (find(noQuantileBinsV.begin(),noQuantileBinsV.end(), plotVars[nTypeBinNow]) == noQuantileBinsV.end());
      int  nEvtPass        = utils->drawTree(aChain,drawExprs,treeCuts);

      double minVal(1), maxVal(-1);
      if(nEvtPass > 0) {
        TH1 * his1 = (TH1F*)gDirectory->Get(hisName);
        if(dynamic_cast<TH1*>(his1)) {
          his1->BufferEmpty();

          if(useQuantileBins) {
            vector <double> fracV;

            fracV.resize(nBinsZ+1,1);
            for(int nBinZnow=0; nBinZnow<nBinsZ; nBinZnow++) {
              fracV[nBinZnow] = nBinZnow/double(nBinsZ);
            }
            useQuantileBins = (useQuantileBins && (utils->getQuantileV(fracV,varPlot_binE[nTypeBinNow],his1)));
            fracV.clear();

            fracV.resize(nBinsZ,1);
            for(int nBinZnow=0; nBinZnow<nBinsZ; nBinZnow++) {
              fracV[nBinZnow] = (nBinZnow+0.5)/double(nBinsZ);
            }
            useQuantileBins = (useQuantileBins && (utils->getQuantileV(fracV,varPlot_binC[nTypeBinNow],his1)));
            fracV.clear();

            // just in case, if the quantile search failes for some reason (!?!), try to use equal-spaced bins
            if(!useQuantileBins) {
              varPlot_binE[nTypeBinNow].resize(nBinsZ+1,0);
              varPlot_binC[nTypeBinNow].resize(nBinsZ  ,0);
            }
          }

          if(!useQuantileBins) {
            minVal  = his1->GetXaxis()->GetBinLowEdge(his1->GetXaxis()->GetFirst());
            maxVal  = his1->GetXaxis()->GetBinUpEdge (his1->GetXaxis()->GetLast() );
          }

          DELNULL(his1);
        }
      }

      if(useQuantileBins) {
        aLOG(Log::DEBUG) <<coutGreen<<" - adding plotting variable "<<coutRed<<plotVars[nTypeBinNow]<<coutGreen<<" with "<<coutPurple
                         <<nBinsZ<<coutGreen<<" bins:"<<coutDef<<endl;
        
        for(int nBinZnow=0; nBinZnow<nBinsZ; nBinZnow++) {
          aLOG(Log::DEBUG) <<coutGreen<<"  -- bin "<<coutBlue<<nBinZnow<<coutGreen<<" with edgeLow,center,edgeHigh: [ "
                           <<coutPurple<<varPlot_binE[nTypeBinNow][nBinZnow]  <<coutGreen<<" , "
                           <<coutRed   <<varPlot_binC[nTypeBinNow][nBinZnow]  <<coutGreen<<" , "
                           <<coutPurple<<varPlot_binE[nTypeBinNow][nBinZnow+1]<<coutGreen<<" ]"<<coutDef<<endl;
        }
      }
      else {
        if(maxVal <= minVal) { minVal = 0; maxVal = 1; }

        double binW = (maxVal - minVal)/double(nBinsZ);

        aLOG(Log::DEBUG) <<coutGreen<<" - adding plotting variable "<<coutRed<<plotVars[nTypeBinNow]<<coutGreen<<" with "<<coutPurple
                         <<nBinsZ<<coutGreen<<" bins with width ("<<binW<<") within ["<<minVal<<","<<maxVal<<"]"<<coutDef<<endl;
        
        for(int nBinZnow=0; nBinZnow<nBinsZ; nBinZnow++) {
          double  binEdgeL  = minVal   + binW * nBinZnow;
          double  binCenter = binEdgeL + binW * 0.5;
          
          varPlot_binE[nTypeBinNow][nBinZnow] = binEdgeL;
          varPlot_binC[nTypeBinNow][nBinZnow] = binCenter;
        }
        varPlot_binE[nTypeBinNow][nBinsZ] = maxVal;
      }
    }
  }


  // -----------------------------------------------------------------------------------------------------------
  // derive the binning for the his_err* histograms
  // -----------------------------------------------------------------------------------------------------------
  double minErrTrgReg(1), maxErrTrgReg(-1);
  if(doKnnErrPlots) {
    TString cutStr("_comn"), hisTmpName("his1_TMP");

    int     nMLMnow   = max(0, glob->GetOptI("nMLMnow"));
    TString MLMname   = (TString)((nTagBestMLM == 0) ? getTagName(nMLMnow) : regBestNameVal);

    TString treeCuts  = (TString)getTrainTestCuts(cutStr,nMLMnow,0,0,var);
    TString drawExprs = (TString)MLMname+"-"+zTrgName+">>"+hisTmpName;
    int nEvtPass      = utils->drawTree(aChain,drawExprs,treeCuts);

    if(nEvtPass > 0) {
      TH1 * his1 = (TH1F*)gDirectory->Get(hisTmpName);
      if(dynamic_cast<TH1*>(his1)) {
        his1->BufferEmpty();
        
        double          bin0(0), bin1(0);
        vector <double> fracV, quantV(2,-1);
        fracV.push_back(0.01); fracV.push_back(0.99);
        
        bool hasQnt = utils->getQuantileV(fracV,quantV,his1);
        if(hasQnt) {
          // try to get rid of crazy outliers
          bin0 = quantV[0]; bin1 = quantV[1];
        }
        else {
          // if did not manage for some reason, use derived bin edges
          bin0 = his1->GetXaxis()->GetBinLowEdge(his1->GetXaxis()->GetFirst());
          bin1 = his1->GetXaxis()->GetBinUpEdge (his1->GetXaxis()->GetLast() );
        }
        if(bin0 < bin1) {
          // now add another 50% to the range (as only the true dist was used here, not the knn-estimator dist)
          double diff = 0.25 * (bin1 - bin0);
          bin0 -= diff; bin1 += diff;

          minErrTrgReg = bin0; maxErrTrgReg = bin1;
        }

        fracV.clear(); quantV.clear();
      }
    }
  }

  // -----------------------------------------------------------------------------------------------------------
  // initialize some histograms
  // -----------------------------------------------------------------------------------------------------------
  vector < TString >                      typeTitleV;
  map < TString,vector <TH1*> >           his_regTrgZ;
  map < TString,TH2* >                    his_corRegTrgZ;
  map < TString,vector < vector<TH1*> > > his_clos, his_relErr, his_errTrg, his_errReg, his_errRegNP;

  for(int nTypeMLMnow=0; nTypeMLMnow<2; nTypeMLMnow++) {
    int nTypeIn = (nTypeMLMnow == 0) ? nMLMsIn : nPDFsIn;

    for(int nTypeInNow=0; nTypeInNow<nTypeIn; nTypeInNow++) {
      TString typeName     = (nTypeMLMnow == 0) ? nameV_MLM_v[nTypeInNow] : nameV_PDF[nTypeInNow];

      TString hisTitle     = typeName; hisTitle.ReplaceAll("_"," ");
      typeTitleV.push_back(hisTitle);

      his_clos   [typeName].resize(nTypeBins,vector<TH1*>(nBinsZ+1,NULL));
      his_relErr [typeName].resize(nTypeBins,vector<TH1*>(nBinsZ+1,NULL));
      his_regTrgZ[typeName].resize(2,NULL);
      
      if(doKnnErrPlots) {
        his_errTrg  [typeName].resize(nTypeBins,vector<TH1*>(nBinsZ+1,NULL));
        his_errReg  [typeName].resize(nTypeBins,vector<TH1*>(nBinsZ+1,NULL));
        his_errRegNP[typeName].resize(nTypeBins,vector<TH1*>(nBinsZ+1,NULL));
      }

      // (nTypeBinNow == 1): bins of the regression value, (nTypeBinNow == 0): bins of the target value
      for(int nTypeBinNow=0; nTypeBinNow<nTypeBins; nTypeBinNow++) {
        TString typeBinZ = TString::Format("_typeBinZ%d",nTypeBinNow);

        for(int nBinZnow=0; nBinZnow<nBinsZ+1; nBinZnow++) {
          TString nameBinZ = TString::Format("_nBinZ%d",nBinZnow);

          for(int nHisType=0; nHisType<5; nHisType++) {
            int     nHisBins(closHisN);
            double  bin0(1), bin1(-1);
            TString hisNameTag("");
            TH1     * his1(NULL);

            if     (nHisType == 0                 ) hisNameTag = "closHis_";   
            else if(nHisType == 1                 ) hisNameTag = "relErrHis_"; 
            else if(nHisType == 2 && doKnnErrPlots) hisNameTag = "errTrgHis_"; 
            else if(nHisType == 3 && doKnnErrPlots) hisNameTag = "errRegHis_"; 
            else if(nHisType == 4 && doKnnErrPlots) hisNameTag = "errNregHis_";
            else continue;

            if(nHisType > 1) {
              nHisBins = errTrgHisN; bin0 = minErrTrgReg; bin1 = maxErrTrgReg;
            }
            
            hisName = (TString)hisNameTag+typeName+typeBinZ+nameBinZ;
            his1    = new TH1F(hisName,hisName,nHisBins,bin0,bin1);
            his1->SetDefaultBufferSize(hisBufSize);
            his1->SetTitle(hisTitle);

            if     (nHisType == 0) his_clos    [typeName][nTypeBinNow][nBinZnow] = his1;
            else if(nHisType == 1) his_relErr  [typeName][nTypeBinNow][nBinZnow] = his1;
            else if(nHisType == 2) his_errTrg  [typeName][nTypeBinNow][nBinZnow] = his1;
            else if(nHisType == 3) his_errReg  [typeName][nTypeBinNow][nBinZnow] = his1;
            else if(nHisType == 4) his_errRegNP[typeName][nTypeBinNow][nBinZnow] = his1;
          }
        }

        if(nTypeBinNow < 2) {
          hisName = (TString)"regTrgZ_"+typeName+typeBinZ;
          his_regTrgZ[typeName][nTypeBinNow] = new TH1F(hisName,hisName,nBinsZtrg,&(zTrgPlot_binE[0]));
          his_regTrgZ[typeName][nTypeBinNow]->SetTitle(hisTitle);
        }
      }

      hisName = (TString)"corRegTrgZ_"+typeName;
      his_corRegTrgZ[typeName] = new TH2F(hisName,hisName,nBinsZtrg,&(zTrgPlot_binE[0]),nBinsZtrg,&(zTrgPlot_binE[0]));
      his_corRegTrgZ[typeName]->SetTitle(hisTitle);
      his_corRegTrgZ[typeName]->GetXaxis()->SetTitle(zTrgTitle);
      his_corRegTrgZ[typeName]->GetYaxis()->SetTitle(zRegTitle);
    }
  }

  // -----------------------------------------------------------------------------------------------------------
  // loop on the tree
  // -----------------------------------------------------------------------------------------------------------
  bool  breakLoop(false);
  int   nObjectsToPrint(glob->GetOptI("nObjectsToPrint"));
  var->clearCntr();
  for(Long64_t loopEntry=0; true; loopEntry++) {
    if(!var->getTreeEntry(loopEntry)) breakLoop = true;

    if((var->GetCntr("nObj")+1 % nObjectsToPrint == 0) || breakLoop) { var->printCntr(aChainName,Log::DEBUG); }
    if(breakLoop) break;

    double usrWgt = hasUserWgt ? var->GetForm(userWgtFrm) : 1;
    double zTrg   = var->GetVarF(zTrgName);

    // the only cut which is applied here... all the rest should meanifest themselves as [weight==0]
    if(zTrg < minValZ) { var->IncCntr((TString)zTrgName+" < "+utils->doubleToStr(minValZ)); continue; }
    if(zTrg > maxValZ) { var->IncCntr((TString)zTrgName+" > "+utils->doubleToStr(maxValZ)); continue; }

    // all single-value solutions (single MLMs and MLM/PDF averages)
    // -----------------------------------------------------------------------------------------------------------
    for(int nMLMinNow=0; nMLMinNow<nMLMsIn; nMLMinNow++) {
      TString typeName = nameV_MLM_v[nMLMinNow];
      double  zRegV    = var->GetVarF(nameV_MLM_v[nMLMinNow]);
      double  zRegE    = var->GetVarF(nameV_MLM_e[nMLMinNow]);
      double  zRegW    = var->GetVarF(nameV_MLM_w[nMLMinNow]) * usrWgt;  if(zRegW < EPS) continue;

      // zRegV = min(max(zRegV,minValZ),maxValZ);

      his_regTrgZ   [typeName][0]->Fill(zTrg,      zRegW);
      his_regTrgZ   [typeName][1]->Fill(zRegV,     zRegW);
      his_corRegTrgZ[typeName]   ->Fill(zTrg,zRegV,zRegW);

      for(int nTypeBinNow=0; nTypeBinNow<nTypeBins; nTypeBinNow++) {
        // (nTypeBinNow == 1): bins of the regression value, (nTypeBinNow == 0): bins of the target value
        int nBinZnow(0);
        if     (nTypeBinNow == 0) nBinZnow = getBinZ(zTrg                                     ,zPlot_binE);
        else if(nTypeBinNow == 1) nBinZnow = getBinZ(zRegV                                    ,zPlot_binE);
        else                      nBinZnow = getBinZ(var->GetForm(plotVarForms[nTypeBinNow-2]),varPlot_binE[nTypeBinNow-2]);
        if(nBinZnow < 0) continue;

        double sclBias(zRegV-zTrg);
        if(plotWithSclBias) {
          double denom = 1 + zTrg;
          if(fabs(denom) < EPS) sclBias  = DefOpts::DefF;
          else                  sclBias /= denom;
        }

        his_clos[typeName][nTypeBinNow][nBinZnow]->Fill(sclBias , zRegW);
        his_clos[typeName][nTypeBinNow][nBinsZ]  ->Fill(sclBias , zRegW);

        if(zRegE > 0) {
          his_relErr[typeName][nTypeBinNow][nBinZnow]->Fill((zRegV-zTrg)/zRegE , zRegW);
          his_relErr[typeName][nTypeBinNow][nBinsZ]  ->Fill((zRegV-zTrg)/zRegE , zRegW);

          if(doKnnErrPlots) {
            his_errTrg[typeName][nTypeBinNow][nBinZnow]->Fill((zRegV-zTrg) , zRegW);
            his_errTrg[typeName][nTypeBinNow][nBinsZ]  ->Fill((zRegV-zTrg) , zRegW);

            his_errReg[typeName][nTypeBinNow][nBinZnow]->Fill(     zRegE , zRegW/2);
            his_errReg[typeName][nTypeBinNow][nBinZnow]->Fill(-1 * zRegE , zRegW/2);
            his_errReg[typeName][nTypeBinNow][nBinsZ]  ->Fill(     zRegE , zRegW/2);
            his_errReg[typeName][nTypeBinNow][nBinsZ]  ->Fill(-1 * zRegE , zRegW/2);

            if(nameV_MLM_eN[nMLMinNow] != "" && nameV_MLM_eP[nMLMinNow] != "") {
              double zRegE_N = -1 * var->GetVarF(nameV_MLM_eN[nMLMinNow]);
              double zRegE_P =      var->GetVarF(nameV_MLM_eP[nMLMinNow]);

              his_errRegNP[typeName][nTypeBinNow][nBinZnow]->Fill(zRegE_N , zRegW/2);
              his_errRegNP[typeName][nTypeBinNow][nBinZnow]->Fill(zRegE_P , zRegW/2);
              his_errRegNP[typeName][nTypeBinNow][nBinsZ]  ->Fill(zRegE_N , zRegW/2);
              his_errRegNP[typeName][nTypeBinNow][nBinsZ]  ->Fill(zRegE_P , zRegW/2);
            }
          }
        }
      }
    }

    // pdf solutions
    // -----------------------------------------------------------------------------------------------------------
    for(int nPDFinNow=0; nPDFinNow<nPDFsIn; nPDFinNow++) {
      TString typeName = nameV_PDF[nPDFinNow];
      double  pdfWgt   = var->GetVarF(pdfTagWgtV[nPDFinNow]) * usrWgt;  if(pdfWgt < EPS) continue;
      double  pdfErr   = var->GetVarF(pdfTagErrV[nPDFinNow]);

      his_regTrgZ[typeName][0]->Fill(zTrg,pdfWgt);

      double pdfSum(0);
      for(int nPdfBinNow=0; nPdfBinNow<nPDFbins; nPdfBinNow++) {
        double pdfBinCtr  = zPDF_binC[nPdfBinNow];
        double pdfBinValW = var->GetVarF(nameV_PDF_b[nPDFinNow][nPdfBinNow]) * pdfWgt;

        pdfSum += pdfBinValW;
        his_regTrgZ   [typeName][1]->Fill(pdfBinCtr,     pdfBinValW);
        his_corRegTrgZ[typeName]   ->Fill(zTrg,pdfBinCtr,pdfBinValW);

        for(int nTypeBinNow=0; nTypeBinNow<nTypeBins; nTypeBinNow++) {
          // (nTypeBinNow == 0): bins of the regression value, (nTypeBinNow == 1): bins of the target value
          int nBinZnow(0);
          if     (nTypeBinNow == 0) nBinZnow = getBinZ(zTrg                                     ,zPlot_binE);
          else if(nTypeBinNow == 1) nBinZnow = getBinZ(pdfBinCtr                                ,zPlot_binE);
          else                      nBinZnow = getBinZ(var->GetForm(plotVarForms[nTypeBinNow-2]),varPlot_binE[nTypeBinNow-2]);
          if(nBinZnow < 0) continue;

          double sclBias(pdfBinCtr-zTrg);
          if(plotWithSclBias) {
            double denom = 1 + zTrg;
            if(fabs(denom) < EPS) sclBias  = DefOpts::DefF;
            else                  sclBias /= denom;
          }

          his_clos[typeName][nTypeBinNow][nBinZnow]->Fill(sclBias , pdfBinValW);
          his_clos[typeName][nTypeBinNow][nBinsZ]  ->Fill(sclBias , pdfBinValW);

          if(pdfErr > 0) {
            his_relErr[typeName][nTypeBinNow][nBinZnow]->Fill((pdfBinCtr-zTrg)/pdfErr , pdfBinValW);
            his_relErr[typeName][nTypeBinNow][nBinsZ]  ->Fill((pdfBinCtr-zTrg)/pdfErr , pdfBinValW);

            if(doKnnErrPlots) {
              his_errTrg[typeName][nTypeBinNow][nBinZnow]->Fill((pdfBinCtr-zTrg) , pdfBinValW);
              his_errTrg[typeName][nTypeBinNow][nBinsZ]  ->Fill((pdfBinCtr-zTrg) , pdfBinValW);

              his_errReg[typeName][nTypeBinNow][nBinZnow]->Fill(     pdfErr , pdfBinValW/2);
              his_errReg[typeName][nTypeBinNow][nBinZnow]->Fill(-1 * pdfErr , pdfBinValW/2);
              his_errReg[typeName][nTypeBinNow][nBinsZ]  ->Fill(     pdfErr , pdfBinValW/2);
              his_errReg[typeName][nTypeBinNow][nBinsZ]  ->Fill(-1 * pdfErr , pdfBinValW/2);
            }
          }
        }
      }
      if(fabs(pdfSum/pdfWgt - 1) > 1e-5) {
        aLOG(Log::WARNING) <<coutRed<<(TString)"Sum(PDF weights) = "+utils->floatToStr(pdfSum/pdfWgt)+" , but should be 1 ?!?!"<<coutDef<<endl;
      }
    }

    var->IncCntr("nObj"); if(var->GetCntr("nObj") == maxNobj) breakLoop = true;
  }
  if(!breakLoop) { var->printCntr(aChainName,Log::DEBUG); }

  // -----------------------------------------------------------------------------------------------------------
  // 
  // -----------------------------------------------------------------------------------------------------------
  int nMetrics    = 7;
  int nAvgMetrics = nMetrics + 3;

  vector <TString>          metricNameV(nAvgMetrics), metricTitleV(nAvgMetrics);
  vector < vector<double> > graphAvg_Xv(nAvgMetrics,vector<double>(nMLMsIn+nPDFsIn,-1)), graphAvg_Yv(nAvgMetrics,vector<double>(nMLMsIn+nPDFsIn,-1)),
                            graphAvg_Xe(nAvgMetrics,vector<double>(nMLMsIn+nPDFsIn,-1)), graphAvg_Ye(nAvgMetrics,vector<double>(nMLMsIn+nPDFsIn,-1));

  for(int nMetricNow=0; nMetricNow<nAvgMetrics; nMetricNow++) {
    if     (nMetricNow == 0) { metricNameV[nMetricNow] =  "mean";        metricTitleV[nMetricNow] = biasTitle;                    }
    else if(nMetricNow == 1) { metricNameV[nMetricNow] =  "sigma";       metricTitleV[nMetricNow] = "#sigma";                     }
    else if(nMetricNow == 2) { metricNameV[nMetricNow] =  "sigma_68";    metricTitleV[nMetricNow] = "#sigma_{68}";                }
    else if(nMetricNow == 3) { metricNameV[nMetricNow] =  "fracSigma_2"; metricTitleV[nMetricNow] = "f(2#sigma)";                 }
    else if(nMetricNow == 4) { metricNameV[nMetricNow] =  "fracSigma_3"; metricTitleV[nMetricNow] = "f(3#sigma)";                 }
    else if(nMetricNow == 5) { metricNameV[nMetricNow] =  "fracSig68_2"; metricTitleV[nMetricNow] = "f(2#sigma_{68})";            }
    else if(nMetricNow == 6) { metricNameV[nMetricNow] =  "fracSig68_3"; metricTitleV[nMetricNow] = "f(3#sigma_{68})";            }
    else if(nMetricNow == 7) { metricNameV[nMetricNow] =  "sigmaRelErr"; metricTitleV[nMetricNow] = "#sigma_{68}(#delta/#sigma)"; }
    else if(nMetricNow == 8) { metricNameV[nMetricNow] =  "N_pois";      metricTitleV[nMetricNow] = "N_{pois}";                   }
    else if(nMetricNow == 9) { metricNameV[nMetricNow] =  "KS_test";     metricTitleV[nMetricNow] = "KS-test";                    }
  }

  for(int nPlotType=0; nPlotType<2; nPlotType++) {
    int     nMetricsNow(0);
    TString plotTypeTitle("");
    if     (nPlotType == 0) { nMetricsNow = nMetrics; plotTypeTitle = (TString)"";                }
    else if(nPlotType == 1) { nMetricsNow = 3;        plotTypeTitle = (TString)"(#delta/#sigma)"; }
    else break;

    vector <TString>                        metricTitlesNow = metricTitleV;
    map <TString , vector <TMultiGraph *> > mltGrphV;
    map <TString , TMultiGraph *>           mltGrphM;

    for(int nMetricNow=0; nMetricNow<nMetricsNow; nMetricNow++) {

      metricTitlesNow[nMetricNow] += plotTypeTitle;

      for(int nTypeBinNow=0; nTypeBinNow<nTypeBins; nTypeBinNow++) {
        TString xTitle("");
        if     (nTypeBinNow == 0) xTitle = zTrgTitle;
        else if(nTypeBinNow == 1) xTitle = zRegTitle;
        else                      xTitle = utils->regularizeName(plotVars[nTypeBinNow-2]);

        TString tagNow0(""), tagNow1("");
        tagNow0 = (TString)"all_"+xTitle;      tagNow1 = (TString)tagNow0+metricNameV[nMetricNow];
        mltGrphM[tagNow1] = new TMultiGraph(); mltGrphV[tagNow0].push_back(mltGrphM[tagNow1]);
      }
    }

    for(int nTypeMLMnow=0; nTypeMLMnow<2; nTypeMLMnow++) {
      int nTypeIn = (nTypeMLMnow == 0) ? nMLMsIn : nPDFsIn;

      for(int nTypeInNow=0; nTypeInNow<nTypeIn; nTypeInNow++) {
        TString typeName = (nTypeMLMnow == 0) ? nameV_MLM_v[nTypeInNow] : nameV_PDF[nTypeInNow];

        TString tagNow0(""), tagNow1("");
        tagNow0 = (TString)typeName+"single"; tagNow1 = (TString)tagNow0+"_mean";
        mltGrphM[tagNow1] = new TMultiGraph(); mltGrphV[tagNow0].push_back(mltGrphM[tagNow1]);

        tagNow0 = (TString)typeName+"single"; tagNow1 = (TString)tagNow0+"_sigma";
        mltGrphM[tagNow1] = new TMultiGraph(); mltGrphV[tagNow0].push_back(mltGrphM[tagNow1]);

        tagNow0 = (TString)typeName+"single"; tagNow1 = (TString)tagNow0+"_fracSig";
        mltGrphM[tagNow1] = new TMultiGraph(); mltGrphV[tagNow0].push_back(mltGrphM[tagNow1]);
        
        for(int nTypeBinNow=0; nTypeBinNow<nTypeBins-2; nTypeBinNow++) {
          TString plotVarName = (TString)typeName+" , "+utils->regularizeName(plotVars[nTypeBinNow]);

          TString tagNow0(""), tagNow1("");
          tagNow0 = (TString)plotVarName+"plotVars"; tagNow1 = (TString)tagNow0+"_mean"; tagNow1.ReplaceAll(" , ","");
          mltGrphM[tagNow1] = new TMultiGraph(); mltGrphV[tagNow0].push_back(mltGrphM[tagNow1]);

          tagNow0 = (TString)plotVarName+"plotVars"; tagNow1 = (TString)tagNow0+"_sigma"; tagNow1.ReplaceAll(" , ","");
          mltGrphM[tagNow1] = new TMultiGraph(); mltGrphV[tagNow0].push_back(mltGrphM[tagNow1]);

          tagNow0 = (TString)plotVarName+"plotVars"; tagNow1 = (TString)tagNow0+"_fracSig"; tagNow1.ReplaceAll(" , ","");
          mltGrphM[tagNow1] = new TMultiGraph(); mltGrphV[tagNow0].push_back(mltGrphM[tagNow1]);
        }
      }
    }

    for(int nTypeMLMnow=0; nTypeMLMnow<2; nTypeMLMnow++) {
      int nTypeIn = (nTypeMLMnow == 0) ? nMLMsIn : nPDFsIn;

      for(int nTypeInNow=0; nTypeInNow<nTypeIn; nTypeInNow++) {
        TString typeName    = (nTypeMLMnow == 0) ? nameV_MLM_v[nTypeInNow] : nameV_PDF[nTypeInNow];
        int     nTypesSoFar = (nTypeMLMnow == 0) ? nTypeInNow              : nTypeInNow + nMLMsIn;

        for(int nTypeBinNow=0; nTypeBinNow<nTypeBins; nTypeBinNow++) {
          TString xTitle("");
          if     (nTypeBinNow == 0) xTitle = zTrgTitle;
          else if(nTypeBinNow == 1) xTitle = zRegTitle;
          else                      xTitle = plotVars[nTypeBinNow-2];

          double def = DefOpts::DefD;
          vector < vector<double> > graph_Xv(nMetricsNow,vector<double>(nBinsZ,def)), graph_Yv(nMetricsNow,vector<double>(nBinsZ,def)),
                                    graph_Xe(nMetricsNow,vector<double>(nBinsZ,def)), graph_Ye(nMetricsNow,vector<double>(nBinsZ,def));

          for(int nMetricNow=0; nMetricNow<nMetricsNow; nMetricNow++) {
            if(nTypeBinNow < 2) graph_Xv[nMetricNow] = zPlot_binC;
            else                graph_Xv[nMetricNow] = varPlot_binC[nTypeBinNow-2];
          }

          TString hisNameNow(""), hisTitleNow("");
          for(int nBinZnow=0; nBinZnow<nBinsZ; nBinZnow++) {
            TH1 * his1(NULL);
            if     (nPlotType == 0) his1 = his_clos  [typeName][nTypeBinNow][nBinZnow];
            else if(nPlotType == 1) his1 = his_relErr[typeName][nTypeBinNow][nBinZnow];

            if(hisNameNow == "") { hisNameNow = his1->GetName();  hisTitleNow = his1->GetTitle(); }

            utils->param->clearAll();
            utils->param->NewOptB("doFracLargerSigma" , true);
            if(!utils->getInterQuantileStats(his1)) continue;

            for(int nMetricNow=0; nMetricNow<nMetricsNow; nMetricNow++) {
              double yVal = utils->param->GetOptF((TString)"quant_"+metricNameV[nMetricNow]);
              double yErr = (!metricNameV[nMetricNow].Contains("fracSig")) ? utils->param->GetOptF((TString)"quant_"+metricNameV[nMetricNow]+"Err") : EPS;
              
              graph_Xe[nMetricNow][nBinZnow] = EPS; graph_Yv[nMetricNow][nBinZnow] = yVal; graph_Ye[nMetricNow][nBinZnow] = yErr;
            }
          }

          // just once, do the inclusive metric - contribution from all bins
          if(nTypeBinNow == 0) {
            TH1 * hisSum(NULL);
            if     (nPlotType == 0) hisSum = his_clos  [typeName][nTypeBinNow][nBinsZ];
            else if(nPlotType == 1) hisSum = his_relErr[typeName][nTypeBinNow][nBinsZ];

            utils->param->clearAll();
            utils->param->NewOptB("doFracLargerSigma" , true);
            if(utils->getInterQuantileStats(hisSum)) {
              if(nPlotType == 0) {
                for(int nMetricNow=0; nMetricNow<nMetricsNow; nMetricNow++) {
                  double yVal = utils->param->GetOptF((TString)"quant_"+metricNameV[nMetricNow]);
                  double yErr = (!metricNameV[nMetricNow].Contains("fracSig")) ? utils->param->GetOptF((TString)"quant_"+metricNameV[nMetricNow]+"Err") : EPS;

                  graphAvg_Xv[nMetricNow][nTypesSoFar] = nTypesSoFar; graphAvg_Xe[nMetricNow][nTypesSoFar] = EPS;
                  graphAvg_Yv[nMetricNow][nTypesSoFar] = yVal;        graphAvg_Ye[nMetricNow][nTypesSoFar] = yErr;
                }
              }
              else if(nPlotType == 1) {
                double yVal(-1), yErr(-1);

                // get the Gaussian width, the sigma_68 or the sigma of the relative error distribution
                if(doGausSigmaRelErr) {
                  double mid = utils->param->GetOptF((TString)"quant_median");
                  double bot = mid - 2 * utils->param->GetOptF((TString)"quant_sigma_68");
                  double top = mid + 2 * utils->param->GetOptF((TString)"quant_sigma_68");

                  TString hisNameNow = (TString)hisSum->GetName()+"_tmpHis";
                  TF1     * fitFunc  = new TF1((TString)hisNameNow+"_func","gaus",bot,top);
                  TH1     * his1     = new TH1F(hisNameNow,hisNameNow,500,-10,10);
                  
                  int nBinsX = hisSum->GetXaxis()->GetNbins();
                  for(int nBinXnow=1; nBinXnow<nBinsX+1; nBinXnow++) {
                    his1->Fill(hisSum->GetBinCenter(nBinXnow),hisSum->GetBinContent(nBinXnow));
                  }
                  his1->Fit(fitFunc,"r0Q"); // his1->SaveAs((TString)his1->GetName()+".C");
                  
                  yVal = fitFunc->GetParameter(2);
                  yErr = fitFunc->GetParError(2);

                  DELNULL(fitFunc); DELNULL(his1);
                }
                else {
                  int nMetricNow = defErrBySigma68 ? 2 : 1;
                  yVal = utils->param->GetOptF((TString)"quant_"+metricNameV[nMetricNow]);
                  yErr = (!metricNameV[nMetricNow].Contains("fracSig")) ? utils->param->GetOptF((TString)"quant_"+metricNameV[nMetricNow]+"Err") : EPS;
                }
                if(yErr > maxSigmaRelErrToPlot) yErr = -1;

                // store it in the sigmaRelErr position
                int nMetricNow = nMetrics;
                VERIFY(LOCATION,(TString)"Sanity check",(metricNameV[nMetricNow] == "sigmaRelErr")); // sanity check that the nMetricNow is correct

                graphAvg_Xv[nMetricNow][nTypesSoFar] = nTypesSoFar; graphAvg_Xe[nMetricNow][nTypesSoFar] = EPS;
                graphAvg_Yv[nMetricNow][nTypesSoFar] = yVal;        graphAvg_Ye[nMetricNow][nTypesSoFar] = yErr;
              }
            }
          }

          // ----------------------------------------------------------------------------------------------------------- 
          // the metric plots
          // ----------------------------------------------------------------------------------------------------------- 
          for(int nMetricNow=0; nMetricNow<nMetricsNow; nMetricNow++) {
            
            vector<double> xV, yV, xE, yE;
            for(int nPointNow=0; nPointNow<int(graph_Xv[nMetricNow].size()); nPointNow++) {
              if(fabs(graph_Xe[nMetricNow][nPointNow]) > 1) continue;

              xV.push_back(graph_Xv[nMetricNow][nPointNow]); yV.push_back(graph_Yv[nMetricNow][nPointNow]);
              xE.push_back(graph_Xe[nMetricNow][nPointNow]); yE.push_back(graph_Ye[nMetricNow][nPointNow]);
            }
            if(int(xV.size()) == 0) continue;

            TString      grphName = (TString)hisNameNow+"_grph_"+metricNameV[nMetricNow];
            TGraphErrors * grph   = new TGraphErrors(int(xV.size()), &xV[0], &yV[0], &xE[0], &yE[0]);

            grph->GetYaxis()->SetTitle(metricTitlesNow[nMetricNow]);
            grph->SetName(grphName);

            TString tagNow0(""), plotVarName("");
            if(nTypeBinNow < 2) {
              tagNow0 = "single_";

              grph->GetXaxis()->SetTitle(zRegTitle+" , "+zTrgTitle);
              grph->SetTitle((TString)metricTitlesNow[nMetricNow]+" , "+xTitle);  
            }
            else {
              plotVarName = plotVars[nTypeBinNow-2];
              tagNow0     = (TString)utils->regularizeName(plotVarName)+"plotVars_";

              grph->GetXaxis()->SetTitle(plotVarName);
              grph->SetTitle((TString)metricTitlesNow[nMetricNow]);  
            }

            if     (metricNameV[nMetricNow].Contains("mean"))    mltGrphM[(TString)typeName+tagNow0+"mean"]   ->Add(grph);
            else if(metricNameV[nMetricNow].Contains("sigma"))   mltGrphM[(TString)typeName+tagNow0+"sigma"]  ->Add(grph);
            else if(metricNameV[nMetricNow].Contains("fracSig")) mltGrphM[(TString)typeName+tagNow0+"fracSig"]->Add(grph);
            
            grph = (TGraphErrors*)grph->Clone((TString)grphName+"_all");
            grph->GetXaxis()->SetTitle(xTitle);
            grph->SetTitle(hisTitleNow);

            mltGrphM[(TString)"all_"+utils->regularizeName(xTitle)+metricNameV[nMetricNow]]->Add(grph);

            xV.clear(); yV.clear(); xE.clear(); yE.clear();
          }

          graph_Xv.clear(); graph_Xe.clear(); graph_Yv.clear(); graph_Ye.clear();

          // ----------------------------------------------------------------------------------------------------------- 
          // plot the full distributions of the true and knn-estimated bias
          // ----------------------------------------------------------------------------------------------------------- 
          if(nPlotType == 0 && doKnnErrPlots) {
            vector < vector<TH1*> > hisErrTrgRegV(nBinsZ+1, vector<TH1*>(3,NULL));
            
            outputs->optClear();
            for(int nBinZnow=0; nBinZnow<nBinsZ+1; nBinZnow++) {
              hisErrTrgRegV[nBinZnow][0] = his_errTrg  [typeName][nTypeBinNow][nBinZnow];
              hisErrTrgRegV[nBinZnow][1] = his_errReg  [typeName][nTypeBinNow][nBinZnow];
              hisErrTrgRegV[nBinZnow][2] = his_errRegNP[typeName][nTypeBinNow][nBinZnow];

              hisErrTrgRegV[nBinZnow][0]->SetTitle("X #equiv "+zRegTitle+" - "+zTrgTitle);
              hisErrTrgRegV[nBinZnow][1]->SetTitle("X #equiv knn-uncert. (combined estimator)");
              hisErrTrgRegV[nBinZnow][2]->SetTitle("X #equiv knn-uncert. (neg/pos estimators)");

              bool useQuantileBins(false);
              if(nTypeBinNow > 1) {
                useQuantileBins = (find(noQuantileBinsV.begin(),noQuantileBinsV.end(), plotVars[nTypeBinNow-2]) == noQuantileBinsV.end());
              }
              TString binTypeTitle = (TString)( (useQuantileBins && nTypeBinNow > 1) ? "_constEntryBin" : "_constWidthBin");
              
              TString binTitle = TString::Format((TString)typeName+": "+xTitle+binTypeTitle+"%d",nBinZnow);
              outputs->draw->NewOptC(TString::Format("generalHeader_%d",nBinZnow), binTitle);
            }

            outputs->draw->NewOptB("doIndividualPlots", glob->OptOrNullB("doIndividualPlots"));
            outputs->draw->NewOptI("nPadsRow"         , 4);
            outputs->draw->NewOptB("multiCnvs"        , true);
            outputs->draw->NewOptB("doNormIntegral"   , true);
            outputs->draw->NewOptC("drawOpt_0"        , "HIST");
            outputs->draw->NewOptC("drawOpt_1"        , "e1p");
            outputs->draw->NewOptC("drawOpt_2"        , "e1p");
            outputs->draw->NewOptC("drawOpt_3"        , "e1p");
            outputs->draw->NewOptC("axisTitleX"       , "X");
            outputs->draw->NewOptC("axisTitleY"       , "Norm X dist.");
            outputs->drawHis1dMultiV(hisErrTrgRegV);

            hisErrTrgRegV.clear();
          }
        }
      }
    }

    for(map <TString,vector <TMultiGraph *> >::iterator Itr=mltGrphV.begin(); Itr!=mltGrphV.end(); ++Itr) {
      TString                  titleNow  = Itr->first;
      vector < TMultiGraph * > aMltGrphV = Itr->second;

      if     (titleNow.Contains("single"))   titleNow.ReplaceAll("single","");
      else if(titleNow.Contains("plotVars")) titleNow.ReplaceAll("plotVars","");
      else                                   titleNow = "";

      outputs->optClear();
      if(titleNow == "") outputs->draw->NewOptC("onlyThisPadLegend", "0");
      else               outputs->draw->NewOptC("generalHeader_0",   titleNow);
      outputs->draw->NewOptC("drawOpt" , "alp");
      outputs->draw->NewOptB("setGridX" , true);
      outputs->draw->NewOptB("setGridY" , true);
      outputs->drawMultiGraphV(aMltGrphV);

    }

    mltGrphV.clear(); mltGrphM.clear(); metricTitlesNow.clear();
  }


  // ----------------------------------------------------------------------------------------------------------- 
  // calculate the nPoisson metric and the Kolmogorov test of the distributions
  // ----------------------------------------------------------------------------------------------------------- 
  for(int nTypeMLMnow=0; nTypeMLMnow<2; nTypeMLMnow++) {
    int nTypeIn = (nTypeMLMnow == 0) ? nMLMsIn : nPDFsIn;

    for(int nTypeInNow=0; nTypeInNow<nTypeIn; nTypeInNow++) {
      TString typeName    = (nTypeMLMnow == 0) ? nameV_MLM_v[nTypeInNow] : nameV_PDF[nTypeInNow];
      int     nTypesSoFar = (nTypeMLMnow == 0) ? nTypeInNow              : nTypeInNow + nMLMsIn;

      // double intgr0 = his_regTrgZ[typeName][0]->Integral(); if(intgr0 < EPS) continue; else his_regTrgZ[typeName][0]->Scale(1/intgr0,"width");
      // double intgr1 = his_regTrgZ[typeName][1]->Integral(); if(intgr1 < EPS) continue; else his_regTrgZ[typeName][1]->Scale(1/intgr1,"width");

      // nPoisson metric
      // ----------------------------------------------------------------------------------------------------------- 
      utils->param->clearAll();
      utils->getNpoisson(his_regTrgZ[typeName][0],his_regTrgZ[typeName][1]);
      double yVal = utils->param->GetOptF("nPoisson");

      int nMetricNow = nMetrics+1;
      VERIFY(LOCATION,(TString)"Sanity check",(metricNameV[nMetricNow] == "N_pois")); // sanity check that the nMetricNow is correct

      graphAvg_Xv[nMetricNow][nTypesSoFar] = nTypesSoFar; graphAvg_Xe[nMetricNow][nTypesSoFar] = EPS;
      graphAvg_Yv[nMetricNow][nTypesSoFar] = yVal;        graphAvg_Ye[nMetricNow][nTypesSoFar] = EPS;

      // Kolmogorov test
      // ----------------------------------------------------------------------------------------------------------- 
      utils->param->clearAll();
      utils->param->NewOptC("Kolmogorov_opt" , "prob_dist");
      utils->getKolmogorov(his_regTrgZ[typeName][0],his_regTrgZ[typeName][1]);
      yVal = utils->param->GetOptF("Kolmogorov_dist");
      
      nMetricNow++;
      VERIFY(LOCATION,(TString)"Sanity check",(metricNameV[nMetricNow] == "KS_test")); // sanity check that the nMetricNow is correct

      graphAvg_Xv[nMetricNow][nTypesSoFar] = nTypesSoFar; graphAvg_Xe[nMetricNow][nTypesSoFar] = EPS;
      graphAvg_Yv[nMetricNow][nTypesSoFar] = yVal;        graphAvg_Ye[nMetricNow][nTypesSoFar] = EPS;
    }
  }

  // -----------------------------------------------------------------------------------------------------------
  // create graphs for the average metrics, comparing all of the different solutions
  // -----------------------------------------------------------------------------------------------------------
  vector < TMultiGraph *> mltGrphAvgV;

  for(int nMultGrphNow=0; nMultGrphNow<100; nMultGrphNow++) {
    TString      yAxisTitle("");
    int          nMetricNow(0);
    vector <int> nGrphV;

    if     (nMultGrphNow == 0) {
      nMetricNow = 0; nGrphV.push_back(nMetricNow);
      VERIFY(LOCATION,(TString)"Sanity check",(metricNameV[nMetricNow] == "mean")); // sanity check that the nMetricNow is correct
      yAxisTitle = (TString)"< "+biasTitle+" >";
    }
    else if(nMultGrphNow == 1) {
      nMetricNow = 1; nGrphV.push_back(nMetricNow);
      VERIFY(LOCATION,(TString)"Sanity check",(metricNameV[nMetricNow] == "sigma")); // sanity check that the nMetricNow is correct
      nMetricNow = 2; nGrphV.push_back(nMetricNow);
      VERIFY(LOCATION,(TString)"Sanity check",(metricNameV[nMetricNow] == "sigma_68")); // sanity check that the nMetricNow is correct
      yAxisTitle = "< #sigma >";
    }
    else if(nMultGrphNow == 2) {
      nMetricNow = 3; nGrphV.push_back(nMetricNow);
      VERIFY(LOCATION,(TString)"Sanity check",(metricNameV[nMetricNow] == "fracSigma_2")); // sanity check that the nMetricNow is correct
      nMetricNow = 4; nGrphV.push_back(nMetricNow);
      VERIFY(LOCATION,(TString)"Sanity check",(metricNameV[nMetricNow] == "fracSigma_3")); // sanity check that the nMetricNow is correct
      nMetricNow = 5; nGrphV.push_back(nMetricNow);
      VERIFY(LOCATION,(TString)"Sanity check",(metricNameV[nMetricNow] == "fracSig68_2")); // sanity check that the nMetricNow is correct
      nMetricNow = 6; nGrphV.push_back(nMetricNow);
      VERIFY(LOCATION,(TString)"Sanity check",(metricNameV[nMetricNow] == "fracSig68_3")); // sanity check that the nMetricNow is correct
      yAxisTitle = "< f >";
    }
    else if(nMultGrphNow == 3) {
      nMetricNow = 7; nGrphV.push_back(nMetricNow);
      VERIFY(LOCATION,(TString)"Sanity check",(metricNameV[nMetricNow] == "sigmaRelErr")); // sanity check that the nMetricNow is correct
      yAxisTitle = "#sigma(#delta/#sigma)";
    }
    else if(nMultGrphNow == 4) {
      nMetricNow = 8; nGrphV.push_back(nMetricNow);
      VERIFY(LOCATION,(TString)"Sanity check",(metricNameV[nMetricNow] == "N_pois")); // sanity check that the nMetricNow is correct
      yAxisTitle = metricTitleV[nMetricNow];
    }
    else if(nMultGrphNow == 5) {
      nMetricNow = 9; nGrphV.push_back(nMetricNow);
      VERIFY(LOCATION,(TString)"Sanity check",(metricNameV[nMetricNow] == "KS_test")); // sanity check that the nMetricNow is correct
      yAxisTitle = metricTitleV[nMetricNow];
    }
    else break;

    TMultiGraph * mltGrphNow = new TMultiGraph();
    mltGrphAvgV.push_back(mltGrphNow);

    for(int nGrphNow=0; nGrphNow<(int)nGrphV.size(); nGrphNow++) {
      nMetricNow = nGrphV[nGrphNow];

      TString grphName = (TString)"avgMetric"+"_grph_"+metricNameV[nMetricNow];
      TGraphErrors * grph = new TGraphErrors(int(graphAvg_Xv[nMetricNow].size()), &graphAvg_Xv[nMetricNow][0], &graphAvg_Yv[nMetricNow][0],
                                                                                  &graphAvg_Xe[nMetricNow][0], &graphAvg_Ye[nMetricNow][0]);
      grph->GetYaxis()->SetTitle(yAxisTitle);
      grph->SetName(grphName);
      grph->SetTitle(metricTitleV[nMetricNow]);

      mltGrphNow->Add(grph);
    }
    nGrphV.clear();
  }

  outputs->optClear();
  outputs->draw->NewOptC("drawOpt"       , "alp");
  outputs->draw->NewOptB("setGridX"      , true);
  outputs->draw->NewOptB("setGridY"      , true);
  outputs->draw->NewOptB("noTitleAxisX"  , true);
  outputs->draw->NewOptC("LabelsOptionX" , "u");
  outputs->draw->NewOptF("LabelsOffsetX" , 0.02);
  for(int nMultGrphNow=0; nMultGrphNow<(int)mltGrphAvgV.size(); nMultGrphNow++) {
    outputs->draw->NewOptB(TString::Format("hasDifferentLablesAxisX_%d",nMultGrphNow), true);

    for(int nAxisLabelNow=0; nAxisLabelNow<(int)typeTitleV.size(); nAxisLabelNow++) {
      outputs->draw->NewOptC(TString::Format("labelV_%d_X_%d",nMultGrphNow,nAxisLabelNow) , typeTitleV[nAxisLabelNow]);
    }
  }
  outputs->drawMultiGraphV(mltGrphAvgV);


  // -----------------------------------------------------------------------------------------------------------
  // create a plot of the distribution of the regression target, compared to all of the different solutions
  // first draw only the various PDF metrics vs the target distribution (nDrawNow<nPDFs), then draw all together
  // -----------------------------------------------------------------------------------------------------------
  vector <TH1*> hisToDelV;
  for(int nDrawNow=0; nDrawNow<nPDFs+1; nDrawNow++) {
    TString nDrawNowStr = utils->intToStr(nDrawNow);
    if(nPDFs == 1 && nDrawNow > 0) continue;
    
    vector <TH1*> hisRegTrgV;
    for(int nTypeMLMnow=0; nTypeMLMnow<2; nTypeMLMnow++) {
      int nTypeIn = (nTypeMLMnow == 0) ? nMLMsIn : nPDFsIn;

      for(int nTypeInNow=0; nTypeInNow<nTypeIn; nTypeInNow++) {
        TString typeName = (nTypeMLMnow == 0) ? nameV_MLM_v[nTypeInNow] : nameV_PDF[nTypeInNow];

        if(nDrawNow < nPDFs) {
          if(nTypeMLMnow == 0) {
            if(typeName.Contains(baseName_regMLM_avg) && typeName != (TString)baseName_regMLM_avg+nDrawNowStr) continue;
            if(typeName.Contains(baseName_regPDF_max) && typeName != (TString)baseName_regPDF_max+nDrawNowStr) continue;
            if(typeName.Contains(baseName_regPDF_avg) && typeName != (TString)baseName_regPDF_avg+nDrawNowStr) continue;
          }
          else { if(typeName != (TString)baseName_nPDF+nDrawNowStr) continue; }
        }

        TH1 * his1(NULL);
        TString yAxisTitle("Entries/bin");

        if((int)hisRegTrgV.size() == 0) {
          his1 = (TH1F*)his_regTrgZ[typeName][0]->Clone((TString)his_regTrgZ[typeName][0]->GetName()+"_cln_"+nDrawNowStr);
          his1->SetTitle(zTrgTitle);
          his1->GetXaxis()->SetTitle((TString)zTrgTitle+" , "+zRegTitle);
          his1->GetYaxis()->SetTitle(yAxisTitle);
          hisRegTrgV.push_back(his1);
        }

        his1 = (TH1F*)his_regTrgZ[typeName][1]->Clone((TString)his_regTrgZ[typeName][1]->GetName()+"_cln_"+nDrawNowStr);
        his1->GetXaxis()->SetTitle(zRegTitle);
        his1->GetYaxis()->SetTitle(yAxisTitle);
        hisRegTrgV.push_back(his1);
      }
    }

    if((int)hisRegTrgV.size() > 0) {
      outputs->optClear();
      outputs->draw->NewOptB("multiCnvs"           , true);
      outputs->draw->NewOptC("drawOpt"             , "e1p");
      outputs->draw->NewOptC("drawOpt_0"           , "HIST");
      outputs->draw->NewOptC("maxDrawMark"         , "100");
      outputs->draw->NewOptB("wideCnvs"            , true);
      // outputs->draw->NewOptB("doNormIntegralWidth" , true);
      outputs->draw->NewOptC("axisTitleX"          , hisRegTrgV[0]->GetXaxis()->GetTitle());
      outputs->draw->NewOptC("axisTitleY"          , hisRegTrgV[0]->GetYaxis()->GetTitle());
      outputs->drawHis1dV(hisRegTrgV);
    }

    for(int nHisNow=0; nHisNow<(int)hisRegTrgV.size(); nHisNow++) { hisToDelV.push_back(hisRegTrgV[nHisNow]); }
    hisRegTrgV.clear();
  }

  // -----------------------------------------------------------------------------------------------------------
  // correlation plots for the regression target with the various solutions
  // -----------------------------------------------------------------------------------------------------------
  outputs->optClear(); // clear before the loop on histograms

  vector <TH1*> hisCorRegTrgV;
  for(int nTypeMLMnow=0; nTypeMLMnow<2; nTypeMLMnow++) {
    int nTypeIn = (nTypeMLMnow == 0) ? nMLMsIn : nPDFsIn;

    for(int nTypeInNow=0; nTypeInNow<nTypeIn; nTypeInNow++) {
      TString typeName = (nTypeMLMnow == 0) ? nameV_MLM_v[nTypeInNow] : nameV_PDF[nTypeInNow];

      // histogram title
      outputs->draw->NewOptC(TString::Format("generalHeader_%d",(int)hisCorRegTrgV.size()),his_corRegTrgZ[typeName]->GetTitle());

      // the actual histogram
      hisCorRegTrgV.push_back(his_corRegTrgZ[typeName]);
    }
  }

  if((int)hisCorRegTrgV.size() > 0) {
    outputs->draw->NewOptB("multiCnvs" , true);
    outputs->draw->NewOptC("drawOpt"   , "box");
    outputs->draw->NewOptC("axisTitleX", hisCorRegTrgV[0]->GetXaxis()->GetTitle());
    outputs->draw->NewOptC("axisTitleY", hisCorRegTrgV[0]->GetYaxis()->GetTitle());
    outputs->drawHis1dMultiV(hisCorRegTrgV);
  }
  hisCorRegTrgV.clear();


  // -----------------------------------------------------------------------------------------------------------
  // print out the plots and move the output to a dedicated directory
  // -----------------------------------------------------------------------------------------------------------
  outputs->WriteOutObjects(true,true); outputs->ResetObjects();

  // // move the plots to a new sub-dir (create it if needed)
  // TString outDirNameNow = outputs->GetOutDirName();
  // TString plotDirName   = (TString)outDirNameNow+"plots/";
  // TString mkdirCmnd     = (TString)"mkdir -p "+plotDirName;
  // utils->exeShellCmndOutput(mkdirCmnd,true,true);

  // bool    hasPlotExt    = (plotExt != "" && plotExt != "NULL");
  // TString mvCmnd        = (TString)"mv "+outDirNameNow+"*.C "+(hasPlotExt ? (TString)outDirNameNow+"*."+plotExt : "")+" "+plotDirName;
  // utils->exeShellCmndOutput(mvCmnd,true,true);


  // -----------------------------------------------------------------------------------------------------------
  // cleanup
  // -----------------------------------------------------------------------------------------------------------
  for(map<TString,TH2*> ::iterator itr = his_corRegTrgZ.begin(); itr!=his_corRegTrgZ.end(); ++itr) {
    TString typeName = itr->first;

    for(int nTypeBinNow=0; nTypeBinNow<(int)his_clos[typeName].size(); nTypeBinNow++) {
      for(int nBinZnow=0; nBinZnow<(int)his_clos[typeName][nTypeBinNow].size(); nBinZnow++) {
        DELNULL(his_clos  [typeName][nTypeBinNow][nBinZnow]);
        DELNULL(his_relErr[typeName][nTypeBinNow][nBinZnow]);
        if(doKnnErrPlots) {
          DELNULL(his_errTrg  [typeName][nTypeBinNow][nBinZnow]);
          DELNULL(his_errReg  [typeName][nTypeBinNow][nBinZnow]);
          DELNULL(his_errRegNP[typeName][nTypeBinNow][nBinZnow]);
        }
      }
    }

    for(int nTypeBinNow=0; nTypeBinNow<(int)his_regTrgZ[typeName].size(); nTypeBinNow++) {
      DELNULL(his_regTrgZ[typeName][nTypeBinNow]);
    }

    DELNULL(his_corRegTrgZ[typeName]);
  }

  for(int nHisNow=0; nHisNow<(int)hisToDelV.size(); nHisNow++) { DELNULL(hisToDelV[nHisNow]); }

  DELNULL(var);
  nameV_MLM_v.clear(); nameV_MLM_w.clear();
  nameV_MLM_e.clear();  nameV_MLM_eN.clear();  nameV_MLM_eP.clear();
  titleV_MLM.clear(); titleV_PDF.clear(); nameV_PDF.clear(); nameV_PDF_b.clear();
  tagNameV.clear(); pdfTagWgtV.clear(); pdfTagErrV.clear();
  plotVars.clear(); plotVarForms.clear(); varPlot_binE.clear(); varPlot_binC.clear();
  typeTitleV.clear(); metricNameV.clear(); metricTitleV.clear();
  graphAvg_Xv.clear(); graphAvg_Xe.clear(); graphAvg_Yv.clear(); graphAvg_Ye.clear(); hisToDelV.clear();
  mltGrphAvgV.clear(); his_regTrgZ.clear(); his_corRegTrgZ.clear(); his_clos.clear();
  his_relErr.clear(); his_errTrg.clear(); his_errReg.clear(); his_errRegNP.clear();

  return;
}




