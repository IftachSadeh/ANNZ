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
 * @brief                       - create root trees from the input ascii files and add a weight branch, calculated with the KNN method
 * 
 * @param inAsciiFiles          - semicolon-separated list of input ascii files (main dataset)
 * @param inAsciiVars           - semicolon-separated list of input parameter names, corresponding to columns in the input files (main dataset)
 * @param inAsciiFiles_wgtKNN   - semicolon-separated list of input ascii files (reference dataset)
 * @param inAsciiVars_wgtKNN    - semicolon-separated list of input parameter names, corresponding to columns in the input files (reference dataset)
 */
// ===========================================================================================================
void CatFormat::inputToSplitTree_wgtKNN(TString inAsciiFiles, TString inAsciiVars, TString inAsciiFiles_wgtKNN, TString inAsciiVars_wgtKNN) {
// ==========================================================================================================================================
  aLOG(Log::INFO) <<coutWhiteOnBlack<<coutBlue<<" - starting ANNZ::inputToSplitTree_wgtKNN() ... "<<coutDef<<endl;

  int     nSplit            = glob->GetOptI("nSplit");
  bool    trainTestTogether = glob->GetOptB("trainTestTogether_wgtKNN");
  TString treeName          = glob->GetOptC("treeName");
  TString outDirNameFull    = glob->GetOptC("outDirNameFull");
  TString inTreeName_wgtKNN = glob->GetOptC("inTreeName_wgtKNN");
  TString wgtTreeName       = "_wgtTree";

  // -----------------------------------------------------------------------------------------------------------
  // the ascii input files from which the KNN weights are derived
  // -----------------------------------------------------------------------------------------------------------
  inputToFullTree(inAsciiFiles_wgtKNN,inAsciiVars_wgtKNN,wgtTreeName,inTreeName_wgtKNN);

  // -----------------------------------------------------------------------------------------------------------
  // the main ascii input files
  // -----------------------------------------------------------------------------------------------------------
  inputToSplitTree(inAsciiFiles,inAsciiVars);

  // -----------------------------------------------------------------------------------------------------------
  // replace the trees created in inputToSplitTree() with new trees which also have KNN weights according to
  // the reference dataset created in inputToFullTree()
  // -----------------------------------------------------------------------------------------------------------
  // setup the reference chain for the KNN input
  // -----------------------------------------------------------------------------------------------------------
  TString treeNameRef = (TString)treeName+wgtTreeName;
  TString fileNameRef = (TString)outDirNameFull+treeNameRef+"*.root";

  TChain  * aChainRef = new TChain(treeNameRef,treeNameRef); aChainRef->SetDirectory(0); aChainRef->Add(fileNameRef);
  aLOG(Log::INFO) <<coutRed<<" - Created Knn  chain  "<<coutGreen<<treeNameRef<<"("<<aChainRef->GetEntries()<<")"
                  <<" from "<<coutBlue<<fileNameRef<<coutDef<<endl;

  // derive the names of the trees created by inputToSplitTree()
  // -----------------------------------------------------------------------------------------------------------
  int nChains = min(nSplit,2);
  vector <TString> treeNames(nChains);

  if(nChains == 1) { treeNames[0] = (TString)treeName+"_full";                                             }
  else             { treeNames[0] = (TString)treeName+"_train"; treeNames[1] = (TString)treeName+"_valid"; }

  // create a temporary sub-dir for the output trees of inputToSplitTree()
  // -----------------------------------------------------------------------------------------------------------
  TString outDirNameTMP = (TString)outDirNameFull+"tmpDir"+wgtTreeName+"/";
  TString mkdirCmnd     = (TString)"mkdir -p "+outDirNameTMP;
  utils->exeShellCmndOutput(mkdirCmnd,inLOG(Log::DEBUG),true);

  TChain              * aChainMerge(NULL);
  vector < TChain * > aChainV(nChains,NULL);

  TString mergeChainName("");
  for(int nChainNow=0; nChainNow<nChains; nChainNow++) {
    if(mergeChainName != "") mergeChainName += "_";
    mergeChainName += treeNames[nChainNow];
  }

  for(int nChainNow=0; nChainNow<nChains; nChainNow++) {
    TString treeNameNow = (TString)treeNames[nChainNow];
    TString fileNameNow = (TString)outDirNameFull+treeNameNow+"*.root";
    TString mvCmnd      = (TString)"mv "+fileNameNow+" "+outDirNameTMP;

    // move the output trees of inputToSplitTree() to the temporary sub-dir, and create a corresponding chain
    // -----------------------------------------------------------------------------------------------------------
    utils->exeShellCmndOutput(mvCmnd,inLOG(Log::DEBUG),true);
    fileNameNow.ReplaceAll(outDirNameFull,outDirNameTMP);

    TChain * aChain = new TChain(treeNameNow,treeNameNow); aChain->SetDirectory(0); aChain->Add(fileNameNow);
    aLOG(Log::INFO) <<coutRed<<" - Created main chain  "<<coutGreen<<treeNameNow<<"("<<aChain->GetEntries()<<")"
                    <<" from "<<coutBlue<<fileNameNow<<coutDef<<endl;

    aChainV[nChainNow] = aChain;

    if(nChains > 1 && trainTestTogether) {
      if(!aChainMerge) {
        TString treeNameMerged = (TString)treeNames[0]+"_"+treeNames[1]+"_merged";
        aChainMerge = new TChain(treeNameMerged,treeNameMerged);
      }

      aChainMerge->Add(aChainV[nChainNow]);

      // if(!aChainMerge) aChainMerge = new TChain(treeNameNow,treeNameNow);
      // else             aChainMerge->SetName(treeNameNow);

      // aChainMerge->SetDirectory(0); aChainMerge->Add(fileNameNow);
      // aChainMerge->SetName(mergeChainName);

      aLOG(Log::INFO) <<coutRed<<" - Adding to merged chain  "<<coutYellow<<mergeChainName<<"("
                      <<aChainMerge->GetEntries()<<")"<<" from "<<coutBlue<<fileNameNow<<coutDef<<endl;
    }
  }

  // if only has train/valid do together based on the merged chain
  // (train/valid are supposed to be representative of each other)
  if(nSplit == 2) {
    for(int nChainNow=0; nChainNow<nChains; nChainNow++) {
      if(trainTestTogether) addWgtKNNtoTree(aChainMerge,aChainRef,aChainV[nChainNow]);
      else                  addWgtKNNtoTree(aChainV[nChainNow],aChainRef);
    }
  }
  // if has tran/valid/test, do each file type on its own (test may have independent dists)
  else {
    for(int nChainNow=0; nChainNow<nChains; nChainNow++) {
      addWgtKNNtoTree(aChainV[nChainNow],aChainRef);
    }
  }

  // cleanup
  for(int nChainNow=0; nChainNow<nChains; nChainNow++) DELNULL(aChainV[nChainNow]);
  if(aChainMerge) DELNULL(aChainMerge);
  DELNULL(aChainRef);

  // remove the temporary sub-dir and the intermediate trees inside
  utils->safeRM(outDirNameTMP,inLOG(Log::DEBUG));

  treeNames.clear(); aChainV.clear();

  return;
}

// ===========================================================================================================
/**
 * @brief                   - create root trees from the input ascii files and add a weight branch, which estimates
 *                          if each objects is "near enough" to enough objects in the training dataset.
 * 
 * @param inAsciiFiles      - semicolon-separated list of input ascii files (main dataset)
 * @param inAsciiVars       - semicolon-separated list of input parameter names, corresponding to columns in the input files (main dataset)
 * @param treeNamePostfix   - postfix for the final output trees
 */
// ===========================================================================================================
void CatFormat::inputToFullTree_wgtKNN(TString inAsciiFiles, TString inAsciiVars, TString treeNamePostfix) {
// =========================================================================================================
  aLOG(Log::INFO) <<coutWhiteOnBlack<<coutBlue<<" - starting ANNZ::inputToFullTree_wgtKNN() ... "<<coutDef<<endl;

  int     nSplit         = glob->GetOptI("nSplit");
  TString treeName       = glob->GetOptC("treeName");
  TString outDirNameFull = glob->GetOptC("outDirNameFull");
  TString wgtTreeName    = "_wgtTree";

  // the input tree for which the weights will be calculated
  inputToFullTree(inAsciiFiles,inAsciiVars,wgtTreeName);

  // setup the chain for the input which has just been produced by inputToFullTree
  // -----------------------------------------------------------------------------------------------------------
  TString treeNameInp = (TString)treeName+wgtTreeName;
  TString fileNameInp = (TString)outDirNameFull+treeNameInp+"*.root";

  TChain  * aChainInp = new TChain(treeNameInp,treeNameInp); aChainInp->SetDirectory(0); aChainInp->Add(fileNameInp);
  aLOG(Log::INFO) <<coutRed<<" - Created input    chain  "<<coutGreen<<treeNameInp<<"("<<aChainInp->GetEntries()<<")"
                  <<" from "<<coutBlue<<fileNameInp<<coutDef<<endl;

  // setup reference chain from the training directory
  // -----------------------------------------------------------------------------------------------------------
  TString treeNameRef = (TString)treeName+(TString)((nSplit == 1) ? "_full" : "_train");
  TString fileNameRef = (TString)glob->GetOptC("inputTreeDirName")+treeNameRef+"*.root";

  TChain  * aChainRef = new TChain(treeNameRef,treeNameRef); aChainRef->SetDirectory(0); aChainRef->Add(fileNameRef);
  aLOG(Log::INFO) <<coutRed<<" - Created refrence chain  "<<coutGreen<<treeNameRef<<"("<<aChainRef->GetEntries()<<")"
                  <<" from "<<coutBlue<<fileNameRef<<coutDef<<endl;

  // the name of the final tree
  TString treeFinalName = (TString)treeName+treeNamePostfix;

  addWgtKNNtoTree(aChainInp,aChainRef,NULL,treeFinalName);

  // remove the intermidiate trees created by inputToFullTree
  utils->safeRM(fileNameInp,inLOG(Log::DEBUG));

  return;
}

// ===========================================================================================================
/**
 * @brief               - Generate weights based on the KNN method, or determine if an object is "close" to enough
 *                      objects in the reference sample (see: Cunha et al. (2008), http://arxiv.org/abs/0810.2991v4)
 * 
 * @details             - **doRelWgts == true:**
 *                        - Weights are calculated as the ratio  r_ref/R_same = (N_ref/V_nn) / (N_same/V_nn) = N_ref/N_same.
 *                        Given the "reference-position" of a given object from the 
 *                        main dataset (aChainInp), N_ref,N_same are the number of near neighbours from the reference-position
 *                        within a given volume in the parameter space, V_nn. The difference between the two parameters, is that
 *                        N_ref is extracted from the reference dataset (aChainRef) and N_same is extracted from the main dataset (aChainInp).
 *                        - The kd-tree is used to search for a fixed number of near neighbours, it is not constrained by
 *                        the distance parameter. We therefore first search for minNobjInVol_wgtKNN near neighbours from
 *                        each chain. The chain for which the distance to the minNobjInVol_wgtKNN neighbour is shortest
 *                        is then selected for another search, with a higher number of near neighbours.
 *                        - It is possible for the new search to still be constrained to be within a distance which
 *                        is shorter than that of the minNobjInVol_wgtKNN neighbour from the other chain. In such a case,
 *                        this step is repeated again and again. In each iteration we look for a larger number
 *                        of near neighbours around the original reference-position. We end the search when we recover some number of near neighbours
 *                        for which the distance from the reference-position for both aChainInp and aChainRef is the same.
 *                        - The main and reference samples are each normalized, such that the sum of weighted entries in both is the same.
 *                        - For each object the weight calculation therefore uses a different volume in the parameter-space. However,
 *                        the volume element for a given object (V_nn) cancels out, since the weights are defined as a ratio. The condition
 *                        on a minimal number of neighbors, minNobjInVol_wgtKNN, prevents shoot noise from dominating the result.
 * @details             - **doRelWgts == false:**
 *                        - Weights are computed for each object from the input sample using 
 *                          wgtTest = max( (dist_Ref0_RefNear - dist_Ref_Inp) / dist_Ref0_RefNear , 0)
 *                        where dist_Ref_Inp is the distance between the input object and its nearest neighbour from the
 *                        reference sample, objInpRef; dist_Ref0_RefNear is the disance between objInpRef and its Nth near neighbour,
 *                        where N is given by minNobjInVol_inTrain.
 *                        - The final weight is currently just a binary estimator:
 *                          wgt = (wgtTest > minWgtTest) ? 1 : 0
 *                         where minWgtTest is a number in the range [0,1], given by the variable, maxRelRatioInRef
 *                        - This way, we derive a measure of the density in the reference sample, in the neighbourhood of the
 *                        input object. This metric is zero (or very small) in cases where the input object is "very far" from
 *                        other objects in the reference sample, where the scale which defines what "very" means, is derived
 *                        in-situ from the reference sample.
 * 
 * @param aChainInp     - a chain corresponding to the main dataset
 * @param aChainRef     - a chain corresponding to the reference dataset
 * @param aChainEvl     - a chain corresponding to the evaluated dataset
 * @param outTreeName   - optional tree name (or else the name is extracted from aChainInp)
 */
// ===========================================================================================================
void CatFormat::addWgtKNNtoTree(TChain * aChainInp, TChain * aChainRef, TChain * aChainEvl, TString outTreeName) {
// ===============================================================================================================
  VERIFY(LOCATION,(TString)"Memory leak ?!?",(dynamic_cast<TChain*>(aChainInp) && dynamic_cast<TChain*>(aChainRef)));

  aLOG(Log::INFO) <<coutWhiteOnBlack<<coutBlue<<" - starting ANNZ::addWgtKNNtoTree() ... "<<coutDef<<endl;

  TString basePrefix       = glob->GetOptC("basePrefix");
  TString outDirNameFull   = glob->GetOptC("outDirNameFull");
  TString indexName        = glob->GetOptC("indexName");
  TString plotExt          = glob->GetOptC("printPlotExtension");
  bool    doRelWgts        = glob->GetOptB("doRelWgts");
  bool    doStoreToAscii   = glob->GetOptB("doStoreToAscii");
  bool    doPlots          = glob->GetOptB("doPlots");
  int     nObjectsToWrite  = glob->GetOptI("nObjectsToWrite");
  int     minObjTrainTest  = glob->GetOptI("minObjTrainTest");
  double  maxRelRatioInRef = glob->GetOptF("maxRelRatioInRef_inTrain");
  TString weightName       = glob->GetOptC("baseName_wgtKNN");
  // number of KNN modules (with decreasing object fractions) for hierarchical searches, and
  // fraction-cut factor for each module-level
  int     nKnnFracs        = glob->GetOptI("nKnnFracs_wgtKNN");
  int     knnFracFact      = glob->GetOptI("knnFracFact_wgtKNN");

  TString typePostfix      = (TString)(doRelWgts ? "_wgtKNN" : "_inTrain");
  TString wgtKNNname       = glob->GetOptC((TString)"baseName"      +typePostfix); // e.g., "baseName_wgtKNN"
  TString outAsciiVars     = glob->GetOptC((TString)"outAsciiVars"  +typePostfix); // e.g., "outAsciiVars_wgtKNN"
  TString weightVarNames   = glob->GetOptC((TString)"weightVarNames"+typePostfix); // e.g., "weightVarNames_wgtKNN"
  int     minNobjInVol     = glob->GetOptI((TString)"minNobjInVol"  +typePostfix); // e.g., "minNobjInVol_wgtKNN"
  double  sampleFracInp    = glob->GetOptF((TString)"sampleFracInp" +typePostfix); // e.g., "sampleFracInp_wgtKNN"
  double  sampleFracRef    = glob->GetOptF((TString)"sampleFracRef" +typePostfix); // e.g., "sampleFracRef_wgtKNN"
  bool    doWidthRescale   = glob->GetOptB((TString)"doWidthRescale"+typePostfix);
  bool    debug            = inLOG(Log::DEBUG_2);
  
  bool    hasChainEvl      = dynamic_cast<TChain*>(aChainEvl);
  TChain  * aChainInpEvl   = hasChainEvl ? aChainEvl : aChainInp;

  if(outTreeName == "") outTreeName = aChainInpEvl->GetName();
  aLOG(Log::INFO) <<coutBlue<<" - output tree name: "<<coutYellow<<outTreeName<<coutDef<<endl;

  vector <TString> chainWgtV(2), chainCutV(2);

  chainWgtV[0] = glob->GetOptC((TString)"weightInp"+typePostfix); chainWgtV[1] = glob->GetOptC((TString)"weightRef"+typePostfix);
  chainCutV[0] = glob->GetOptC((TString)"cutInp"   +typePostfix); chainCutV[1] = glob->GetOptC((TString)"cutRef"   +typePostfix);

  // force reasonable min/max values
  minNobjInVol = max(minNobjInVol,20);

  double maxRelRatioInRef_0(0.001), maxRelRatioInRef_1(0.999);
  if(maxRelRatioInRef > 0 && (maxRelRatioInRef < maxRelRatioInRef_0 || maxRelRatioInRef > maxRelRatioInRef_1)) {
    aLOG(Log::WARNING) <<coutRed<<" - Found [\"maxRelRatioInRef_inTrain\" = "<<coutPurple<<maxRelRatioInRef
                       <<coutRed<<"] expected either a negative value, or one in the range ["<<maxRelRatioInRef_0
                       <<","<<maxRelRatioInRef_1<<"] ..."<<" Will modify to "<<coutYellow<<
                       ((maxRelRatioInRef < maxRelRatioInRef_0)?maxRelRatioInRef_0:maxRelRatioInRef_1)<<coutDef<<endl;
  }
  maxRelRatioInRef = (maxRelRatioInRef > 0) ? max(min(maxRelRatioInRef,maxRelRatioInRef_1),maxRelRatioInRef_0) : -1;

  int     maxNobj         = 0;  // maxNobj = glob->GetOptI("maxNobj"); // only allow maxNobj limits for debugging !! 
  TString outBaseName     = (TString)outDirNameFull+glob->GetOptC("treeName")+wgtKNNname;
  TString wgtNormTreeName = (TString)"_normWgt";

  // decompose the variable names for the KNN distance calculation
  vector <TString> varNames = utils->splitStringByChar(weightVarNames,';');
  int              nVars    = (int)varNames.size();
  
  // sanity checks
  VERIFY(LOCATION,(TString)"Did not find input variables for KNN weight computation [\"weightVarNames_wgtKNN\"/\"weightVarNames_inTrain\" = "
                          +weightVarNames+"] ... Something is horribly wrong !?!?",(nVars > 0));

  VERIFY(LOCATION,(TString)"sampleFracInp_wgtKNN must be a positive number, smaller or equal to 1. Currently is set to "+utils->floatToStr(sampleFracInp)
                 ,(sampleFracInp > 0 && sampleFracInp <= 1));
  VERIFY(LOCATION,(TString)"sampleFracRef_wgtKNN must be a positive number, smaller or equal to 1. Currently is set to "+utils->floatToStr(sampleFracRef)
                 ,(sampleFracRef > 0 && sampleFracRef <= 1));
  
  VERIFY(LOCATION,(TString)"Must set nKnnFracs_wgtKNN >= 2 (recommended > 5), while now set [\"nKnnFracs_wgtKNN\" = "
                          +utils->intToStr(nKnnFracs)+"] ... Something is horribly wrong !?!?",(nKnnFracs > 1));

  VERIFY(LOCATION,(TString)"Must set knnFracFact_wgtKNN >= 2 (recommended 3,4 or 5), while now set [\"knnFracFact_wgtKNN\" = "
                          +utils->intToStr(knnFracFact)+"] ... Something is horribly wrong !?!?",(knnFracFact > 1));

  // for(int nChainNow=0; nChainNow<3; nChainNow++) {
  //   TChain * aChain(NULL);
  //   if     (nChainNow == 0) aChain = aChainInp;
  //   else if(nChainNow == 1) aChain = aChainRef;
  //   else if(nChainNow == 2) {
  //     if(hasChainEvl) aChain = aChainEvl;
  //     else            continue;
  //   }

  //   vector <TString> branchNameV;
  //   utils->getTreeBranchNames(aChain,branchNameV);

  //   for(int nVarNow=0; nVarNow<nVars; nVarNow++) {
  //     VERIFY(LOCATION,(TString)" - Could not find variable "+varNames[nVarNow]+" in list of inputs from "+aChain->GetName(),
  //                     (find(branchNameV.begin(),branchNameV.end(), varNames[nVarNow]) != branchNameV.end()));
  //   }
    
  //   branchNameV.clear();
  // }

  aLOG(Log::INFO) <<coutPurple<<" - will use the following variables for the KNN search: "<<coutYellow<<weightVarNames<<coutDef<<endl;

  vector < vector<double> > minMaxVarVals (2,vector<double>(nVars,0)   );
  vector < vector<TH1*> >   hisVarV       (2,vector<TH1*>  (nVars,NULL));
  vector < TString >        varNamesScaled(nVars,"");

  // -----------------------------------------------------------------------------------------------------------
  // setup the kd-trees for the two chains
  // -----------------------------------------------------------------------------------------------------------
  double          effEntRatio(1);
  vector <double> chainEntV(3,0);

  vector < TChain * >                        aChainV          (2, NULL);
  vector < vector <TFile *> >                knnErrOutFile    (2, vector<TFile *>               (nKnnFracs,NULL));
  vector < vector <TMVA::Factory *> >        knnErrFactory    (2, vector<TMVA::Factory *>       (nKnnFracs,NULL));
  vector < vector <TMVA::MethodKNN *> >      knnErrMethod     (2, vector<TMVA::MethodKNN *>     (nKnnFracs,NULL));
  vector < vector <TMVA::kNN::ModulekNN *> > knnErrModule     (2, vector<TMVA::kNN::ModulekNN *>(nKnnFracs,NULL));
  vector < vector <TString> >                outFileNameKnnErr(2, vector<TString>               (nKnnFracs,"")  );

  #if ROOT_TMVA_V0
  vector < vector <TMVA::Factory *> >        knnErrDataLdr    (2, vector<TMVA::Factory *>       (nKnnFracs,NULL));
  #else
  vector < vector <TMVA::DataLoader *> >     knnErrDataLdr    (2, vector<TMVA::DataLoader *>    (nKnnFracs,NULL));
  #endif

  TString outFileDirKnnErrV = (TString)outBaseName+"_weights"+"/";
  (TMVA::gConfig().GetIONames()).fWeightFileDir = outFileDirKnnErrV;

  // -----------------------------------------------------------------------------------------------------------
  // setup some objects and get histograms for the variable range limits
  // -----------------------------------------------------------------------------------------------------------
  for(int nChainNow=0; nChainNow<2; nChainNow++) {
    TString nChainKNNname = TString::Format("_nChainKNN%d",nChainNow);

    // clone the chain used for the kd-tree (needed, as the kd-tree will turn off branches which
    // are not used, but these are needed for var_0, var_1) - The Clone should not be deleted
    // as this may cause seg-fault (perhaps due to closing of underlying TFile somwhere by ROOT)
    // -----------------------------------------------------------------------------------------------------------
    TChain * aChain = (nChainNow == 0) ? aChainInp : aChainRef;
    aChainV[nChainNow]   = (TChain*)aChain->Clone((TString)aChain->GetName()+nChainKNNname);  aChainV[nChainNow]->SetDirectory(0);

    // make sure that string variables are in the correct access format for cuts and weights
    VarMaps * varTMP = new VarMaps(glob,utils,"varTMP");
    varTMP->connectTreeBranches(aChainV[nChainNow]);

    chainWgtV[nChainNow] = varTMP->regularizeStringForm(chainWgtV[nChainNow]);
    chainCutV[nChainNow] = varTMP->regularizeStringForm(chainCutV[nChainNow]);

    DELNULL(varTMP);

    // add the "baseName_wgtKNN" variable to the weight expression - this is just unity if generating the
    // initial trees, but may hold non-trivial values for the [doRelWgts==false] mode
    chainWgtV[nChainNow] = utils->cleanWeightExpr((TString)"("+chainWgtV[nChainNow]+")*"+weightName);


    TString verbLvlF        = (TString)(debug ? ":V:!Silent"             : ":!V:Silent");
    TString drawProgBarStr  = (TString)(debug ? ":Color:DrawProgressBar" : ":Color:!DrawProgressBar");
    TString transStr        = (TString)":Transformations=I,N";
    TString analysType      = (TString)":AnalysisType=Regression";
    TString allOpts         = (TString)verbLvlF+drawProgBarStr+transStr+analysType;

    // setup the factories
    // -----------------------------------------------------------------------------------------------------------
    for(int nFracNow=0; nFracNow<nKnnFracs; nFracNow++) {
      TString nFracNameNow = (TString)nChainKNNname+TString::Format("_nFrac%d",nFracNow);
      TString wgtNameNow   = (TString)wgtKNNname+nFracNameNow;

      outFileNameKnnErr[nChainNow][nFracNow] = outBaseName+nFracNameNow+".root";

      knnErrOutFile[nChainNow][nFracNow] = new TFile(outFileNameKnnErr[nChainNow][nFracNow],"RECREATE");
      knnErrFactory[nChainNow][nFracNow] = new TMVA::Factory(wgtNameNow, knnErrOutFile[nChainNow][nFracNow], allOpts);    

      #if ROOT_TMVA_V0
      knnErrDataLdr[nChainNow][nFracNow] = knnErrFactory[nChainNow][nFracNow];
      #else
      knnErrDataLdr[nChainNow][nFracNow] = new TMVA::DataLoader("./");
      #endif
    }

    // -----------------------------------------------------------------------------------------------------------
    // calculate the sum of weights in the chain for the normalization
    // -----------------------------------------------------------------------------------------------------------
    for(int nVarNow=0; nVarNow<nVars; nVarNow++) {
      TString hisName   = (TString)nChainKNNname+utils->regularizeName(varNames[nVarNow])+"_hisVar";
      TString drawExprs = (TString)"("+varNames[nVarNow]+")>>"+hisName;
      TString wgtCut    = (TString)"("+chainCutV[nChainNow]+")*("+chainWgtV[nChainNow]+")";
      wgtCut.ReplaceAll("()*()","").ReplaceAll("*()","").ReplaceAll("()*","").ReplaceAll("()","1");

      utils->drawTree(aChain,drawExprs,wgtCut);

      hisVarV[nChainNow][nVarNow] = (TH1F*)gDirectory->Get(hisName); 
      VERIFY(LOCATION,(TString)"Could not derive histogram ("+hisName+") from chain ... Something is horribly wrong ?!?!",(dynamic_cast<TH1F*>(hisVarV[nChainNow][nVarNow])));

      hisVarV[nChainNow][nVarNow]->SetDirectory(0); hisVarV[nChainNow][nVarNow]->BufferEmpty(); outputs->BaseDir->cd();
      
      if(nVarNow == 0) {
        double sumEnt = hisVarV[nChainNow][nVarNow]->GetEntries();
        VERIFY(LOCATION,(TString)"Got sum of entries = "+utils->floatToStr(sumEnt)+" ... Something is horribly wrong ?!?! ",(sumEnt > 0));
        
        chainEntV[nChainNow] = sumEnt;

        // if there is a separate evaluation chain, get the weighted/cut sum of entries for later normalization
        if(nChainNow == 0 && hasChainEvl) {
          hisName   += (TString)"_TMP";
          drawExprs  = (TString)"("+varNames[nVarNow]+")>>"+hisName;

          utils->drawTree(aChainEvl,drawExprs,wgtCut);

          TH1 * hisTMP = (TH1F*)gDirectory->Get(hisName);
          VERIFY(LOCATION,(TString)"Could not derive histogram ("+hisName+") from chain ... Something is horribly wrong ?!?!",(dynamic_cast<TH1F*>(hisTMP)));

          sumEnt = hisTMP->GetEntries();

          VERIFY(LOCATION,(TString)"Got sum of entries = "+utils->floatToStr(sumEnt)+" ... Something is horribly wrong ?!?! ",(sumEnt > 0));
          
          chainEntV[2] = sumEnt;
          DELNULL(hisTMP);
        }
      } 
    }
  }

  // -----------------------------------------------------------------------------------------------------------
  // calculate the range limits, rescale if needed and set the minMaxVarVals variable
  // -----------------------------------------------------------------------------------------------------------
  for(int nVarNow=0; nVarNow<nVars; nVarNow++) {
    vector <double> fracV(2,0), quantV(2,0);

    // scale the variables to the range [-1,1] based on the width of the corresponding distributions
    // -----------------------------------------------------------------------------------------------------------
    varNamesScaled[nVarNow] = varNames[nVarNow];

    if(doWidthRescale) {
      double valMin(1), valMax(-1);
      for(int nChainNow=0; nChainNow<2; nChainNow++) {

        fracV[0] = 0.0; fracV[1] = 1.0; quantV.resize(2,-1);

        int hasQuant = utils->getQuantileV(fracV,quantV,hisVarV[nChainNow][nVarNow]);
        VERIFY(LOCATION,(TString)"Got not compute quantiles for histogram ... Something is horribly wrong ?!?! ",hasQuant);

        if(valMin > valMax) { valMin = quantV[0];             valMax = quantV[1];             }
        else                { valMin = min(valMin,quantV[0]); valMax = max(valMax,quantV[1]); }
      }

      if(valMin < valMax) {
        TString shiftStr = (TString)((valMin > 0) ? " - " : " + ") + utils->floatToStr(fabs(valMin));

        varNamesScaled[nVarNow] = (TString)"(("+varNames[nVarNow]+") "+shiftStr+") * "+utils->floatToStr( 2/(valMax - valMin) )+" - 1";

        aLOG(Log::INFO)<<coutYellow<<" - Transformation to range [-1,1] from "<<coutGreen<<varNames[nVarNow]
                       <<coutYellow<<" to:  "<<coutBlue<<varNamesScaled[nVarNow]<<coutDef<<endl;
      }
    }


    // fill the minMaxVarVals variable from the rescalled variables
    // -----------------------------------------------------------------------------------------------------------
    int     nChainNow = 1;
    TString hisName   = (TString)hisVarV[nChainNow][nVarNow]->GetName()+"_scaled";
    TString drawExprs = (TString)"("+varNamesScaled[nVarNow]+")>>"+hisName;
    TString wgtCut    = (TString)"("+chainCutV[nChainNow]+")*("+chainWgtV[nChainNow]+")";
    wgtCut.ReplaceAll("()*()","").ReplaceAll("*()","").ReplaceAll("()*","").ReplaceAll("()","1");

    utils->drawTree(aChainV[nChainNow],drawExprs,wgtCut);
  
    TH1 * his1        = (TH1F*)gDirectory->Get(hisName);
    VERIFY(LOCATION,(TString)"Could not derive histogram ("+hisName+") from chain ... Something is horribly wrong ?!?!",(dynamic_cast<TH1F*>(his1)));
    
    his1->SetDirectory(0); his1->BufferEmpty(); outputs->BaseDir->cd();
    // his1->SaveAs((TString)varNames[nVarNow]+"_"+utils->intToStr(nChainNow)+"_scaled.C");

    fracV[0] = 0.0; fracV[1] = 1.0; quantV.resize(2,-1);

    int hasQuant = utils->getQuantileV(fracV,quantV,his1);
    VERIFY(LOCATION,(TString)"Got not compute quantiles for histogram("+hisName+") ... Something is horribly wrong ?!?! ",hasQuant);

    // pad the derived range by the maximal of the two: half the bin width of the historgam; 1% of the range of values
    double rangeAdd = max(his1->GetBinWidth(1)/2. , 0.01 * (quantV[1] - quantV[0]));

    minMaxVarVals[0][nVarNow] = quantV[0] - rangeAdd;
    minMaxVarVals[1][nVarNow] = quantV[1] + rangeAdd;

    DELNULL(his1);
    fracV.clear(); quantV.clear();
  }


  // -----------------------------------------------------------------------------------------------------------
  // book the variables and chains in the factory and initialize the kd-tree
  // -----------------------------------------------------------------------------------------------------------
  for(int nChainNow=0; nChainNow<2; nChainNow++) {
    double  objFracNow   = (nChainNow == 0) ? sampleFracInp : sampleFracRef;
    int     nTrainObj    = static_cast<int>(floor(aChainV[nChainNow]->GetEntries() * objFracNow));

    VERIFY(LOCATION,(TString)"Following sampleFracInp_wgtKNN/sampleFracRef_wgtKNN cut, chain("+(TString)aChainV[nChainNow]->GetName()+") with initial "
                            +utils->lIntToStr(aChainV[nChainNow]->GetEntries())+" objects, now has "+utils->lIntToStr(nTrainObj)
                            +" objects (minimum is \"minObjTrainTest\" = "+utils->lIntToStr(minObjTrainTest)+") ...",(nTrainObj >= minObjTrainTest));

    TString verbLvlM        = (TString)(debug ? ":V:H"                   : ":!V:!H");
    TString optKNN          = (TString)":nkNN=10:ScaleFrac=0.0";
    TString trainValidStr   = (TString)"nTrain_Regression=0:nTest_Regression=0:SplitMode=Random:NormMode=NumEvents:!V";

    // setup multiple modules with decreasing object fractions (for search of far neighbours)
    // -----------------------------------------------------------------------------------------------------------
    int nKnnFracsIn(0);
    for(int nFracNow=0; nFracNow<nKnnFracs; nFracNow++) {
      int     split0     = 1;
      int     split1     = static_cast<int>(floor(EPS + split0 * pow(knnFracFact,nFracNow) / objFracNow));
      TString fracCut    = (TString)indexName+" % "+TString::Format("%d < %d",split1,split0);
      TCut    finalCut   = ((TCut)fracCut) + ((TCut)chainCutV[nChainNow]);
      TString wgtNameNow = (TString)wgtKNNname+TString::Format("_%d",nFracNow);

      // define all (scaled) input variables as floats in the factory
      for(int nVarNow=0; nVarNow<nVars; nVarNow++) {
        knnErrDataLdr[nChainNow][nFracNow]->AddVariable(varNamesScaled[nVarNow],varNamesScaled[nVarNow],"",'F');
      }

      knnErrDataLdr[nChainNow][nFracNow]->AddRegressionTree(aChainV[nChainNow], 1, TMVA::Types::kTesting);
      knnErrDataLdr[nChainNow][nFracNow]->AddRegressionTree(aChainV[nChainNow], 1, TMVA::Types::kTraining);
      knnErrDataLdr[nChainNow][nFracNow]->SetWeightExpression(chainWgtV[nChainNow],"Regression");

      knnErrDataLdr[nChainNow][nFracNow]->PrepareTrainingAndTestTree(finalCut,trainValidStr);

      #if ROOT_TMVA_V0
      knnErrMethod[nChainNow][nFracNow] = dynamic_cast<TMVA::MethodKNN*>(
        knnErrFactory[nChainNow][nFracNow]->BookMethod(
          TMVA::Types::kKNN, wgtNameNow, (TString)optKNN+verbLvlM
        )
      );
      #else
      knnErrMethod[nChainNow][nFracNow] = dynamic_cast<TMVA::MethodKNN*>(
        knnErrFactory[nChainNow][nFracNow]->BookMethod(
          knnErrDataLdr[nChainNow][nFracNow], TMVA::Types::kKNN, wgtNameNow, (TString)optKNN+verbLvlM
        )
      );
      #endif
      
      // fill the module with events made from the tree entries and create the binary tree
      knnErrMethod[nChainNow][nFracNow]->Train();

      // make sure we have enough objects after the fraction-cut, and that we aren't repeating ourselves
      int nEffObj = knnErrMethod[nChainNow][nFracNow]->fEvent.size();
      if(nEffObj < minNobjInVol*5) break;

      if(nKnnFracsIn == 0) {
        if(nChainNow == 0) effEntRatio *= nEffObj;
        else               effEntRatio /= nEffObj;
      }
      
      knnErrModule[nChainNow][nFracNow] = knnErrMethod[nChainNow][nFracNow]->fModule;

      aLOG(Log::INFO)  <<coutGreen<<" - "<<coutBlue<<aChainV[nChainNow]->GetName()<<coutGreen<<" - kd-tree (effective entries = "
                       <<nEffObj<<")"<<coutYellow<<" , opts = "<<coutRed<<optKNN<<coutYellow<<" , "<<coutRed
                       <<trainValidStr<<coutYellow<<" , cuts = "<<coutRed<<finalCut<<coutYellow<<" , weight expression: "<<coutRed
                       <<chainWgtV[nChainNow]<<coutYellow<<" , sum of entries = "<<coutPurple<<chainEntV[nChainNow]<<coutDef<<endl;

      // sanity check - if this is not true, the distance calculations will be off
      VERIFY(LOCATION,(TString)"Somehow the fScaleFrac for the kd-tree is not zero ... Something is horribly wrong ?!?!?"
                               ,(knnErrMethod[nChainNow][nFracNow]->fScaleFrac < EPS));
      nKnnFracsIn++;
    }

    // final check on nKnnFracs - at least two knnErrModule need to be accepted
    VERIFY(LOCATION,(TString)"Could not find enough objects for the KNN search."
                    +" Try to decrease the value of "+"minNobjInVol"+typePostfix+" ...",(nKnnFracsIn > 1));
  }


  // -----------------------------------------------------------------------------------------------------------
  // create the vars to read/write trees
  // -----------------------------------------------------------------------------------------------------------
  VarMaps * var_0 = new VarMaps(glob,utils,"treeWeightsKNNvar_0");
  VarMaps * var_1 = new VarMaps(glob,utils,"treeWeightsKNNvar_1");
  
  // add unique formula names (to avoid conflict with existing variable names)
  vector <TString> varFormNames(nVars);
  for(int nVarNow=0; nVarNow<nVars; nVarNow++) {
    varFormNames[nVarNow] = (TString)wgtKNNname+"_"+varNames[nVarNow];

    var_0->NewForm(varFormNames[nVarNow],varNamesScaled[nVarNow]);
  }

  var_0->connectTreeBranches(aChainInpEvl);

  vector < pair<TString,TString> > varTypeNameV;
  var_1->varStruct(var_0,NULL,NULL,&varTypeNameV);

  // the wgtKNNname variable for "baseName_wgtKNN" was created by CatFormat if all went
  // well, but not for "baseName_inTrain"; we therefore add it here if not already defined
  if(!var_1->HasVarF(wgtKNNname)) var_1->NewVarF(wgtKNNname);

  TTree * outTree = new TTree(outTreeName,outTreeName); outTree->SetDirectory(0); outputs->TreeMap[outTreeName] = outTree;
  var_1->createTreeBranches(outTree); 

  aLOG(Log::INFO) <<coutBlue<<" - Will write weights to "<<coutYellow<<(TString)outDirNameFull+outTreeName<<coutBlue<<" ... "<<coutDef<<endl;

  // -----------------------------------------------------------------------------------------------------------
  // loop on the tree
  // -----------------------------------------------------------------------------------------------------------
  double            weightSum(0);
  vector <int>      distIndexV(2,0);
  vector <double>   distV(2,0), weightSumV(2,0);
  TMVA::kNN::VarVec objNowV(nVars,0);

  int  nObjectsToPrint = min(static_cast<int>(aChainInpEvl->GetEntries()/10.) , glob->GetOptI("nObjectsToPrint"));
  bool breakLoop(false), mayWriteObjects(false);
  var_0->clearCntr();
  for(Long64_t loopEntry=0; true; loopEntry++) {
    if(!var_0->getTreeEntry(loopEntry)) breakLoop = true;

    if((mayWriteObjects && var_0->GetCntr("nObj") % nObjectsToWrite == 0) || breakLoop) {
      var_0->printCntr(outTreeName); outputs->WriteOutObjects(false,true); outputs->ResetObjects();
      mayWriteObjects = false;
    }
    else if(var_0->GetCntr("nObj") % nObjectsToPrint == 0) { var_0->printCntr(outTreeName); }

    if(breakLoop) break;

    var_1->copyVarData(var_0,&varTypeNameV);

    // fill the current object vector and use it later in order to create a TMVA::kNN::Event object
    // if any of the variables is beyond the limits derived from the reference sample, the weight is
    // automatically set to zero, with no other computation is needed
    // -----------------------------------------------------------------------------------------------------------
    bool isInsideRef(true);
    for(int nVarNow=0; nVarNow<nVars; nVarNow++) {
      objNowV[nVarNow] = var_0->GetForm(varFormNames[nVarNow]);

      if(objNowV[nVarNow] < minMaxVarVals[0][nVarNow] || objNowV[nVarNow] > minMaxVarVals[1][nVarNow]) {
        isInsideRef = false;
        var_0->IncCntr(wgtKNNname+" = 0 (outside input parameter range)");
        break;
      }
    }

    double weightKNN(0);
    
    if(isInsideRef) {
      const TMVA::kNN::Event evtNow(objNowV,1,0);

      // -----------------------------------------------------------------------------------------------------------
      // derive the weights as the ratio between input and reference samples, if needed
      // -----------------------------------------------------------------------------------------------------------
      if(doRelWgts) {
        // find the same number of near neighbours for each chain, and derive the distance this requires
        int    nObjKNN(minNobjInVol);
        for(int nChainNow=0; nChainNow<2; nChainNow++) {
          knnErrModule[nChainNow][0]->Find(evtNow,nObjKNN);
          const TMVA::kNN::List & knnList = knnErrModule[nChainNow][0]->GetkNNList();

          weightSumV[nChainNow] = 0;
          for(TMVA::kNN::List::const_iterator lit=knnList.begin(); lit!=knnList.end(); ++lit) {
            double wgtNow = (lit->first->GetEvent()).GetWeight();

            VERIFY(LOCATION,(TString)"Found negative weight in reference sample ... Something is horribly wrong ?!?",(wgtNow > 0));

            weightSumV[nChainNow] += wgtNow;
          }

          distV[nChainNow] = evtNow.GetDist(knnList.back().first->GetEvent());
        }

        // the index of the chain with the shorter distance
        if(distV[0] < distV[1]) { distIndexV[0] = 0; distIndexV[1] = 1; }
        else                    { distIndexV[0] = 1; distIndexV[1] = 0; }

        bool   foundDist(false);
        for(int nFracNow=1; nFracNow<nKnnFracs; nFracNow++) {
          if(!knnErrModule[distIndexV[0]][nFracNow]) break;

          knnErrModule[distIndexV[0]][nFracNow]->Find(evtNow,nObjKNN);
          const TMVA::kNN::List & knnList = knnErrModule[distIndexV[0]][nFracNow]->GetkNNList();

          weightSumV[distIndexV[0]] = 0;
          for(TMVA::kNN::List::const_iterator lit=knnList.begin(); lit!=knnList.end(); ++lit) {
            const TMVA::kNN::Event & evtLst = lit->first->GetEvent();

            double knnDistNow = evtNow.GetDist(evtLst);
            double weightObj  = evtLst.GetWeight();

            VERIFY(LOCATION,(TString)"Found negative weight in reference sample ... Something is horribly wrong ?!?",(weightObj > 0));

            if(knnDistNow < distV[distIndexV[1]]) { weightSumV[distIndexV[0]] += weightObj;        }
            else                                  { foundDist                  = true;      break; }
          }
          if(foundDist) {
            if(weightSumV[0] > EPS && weightSumV[1] > EPS) {
              // use effEntRatio, a constant normalization - this does not change the result, but may help
              // to prevent numerical errors if the sizes of the input/reference samples is very different
              weightSumV[1] *= effEntRatio;

              // correct for the hierarchical search-level
              weightSumV[distIndexV[0]] *= pow(knnFracFact,nFracNow);
              
              // finally, calculate the weight for this object
              weightKNN = weightSumV[1]/weightSumV[0];
              
              break;
            }
          }
        }

        if(foundDist) { var_0->IncCntr("Found good weight");        weightSum += weightKNN; }
        else          { var_0->IncCntr("Did not find good weight");                         }
      }
      // -----------------------------------------------------------------------------------------------------------
      // derive the weight from the approximated density estimation of near objects
      // from the reference sample, if needed
      // -----------------------------------------------------------------------------------------------------------
      else {
        // find the closest object in the reference chain
        knnErrModule[1][0]->Find(evtNow,1);
        const TMVA::kNN::List & knnListInp = knnErrModule[1][0]->GetkNNList();

        // must make a sanity check before using the pointer to GetEvent()
        VERIFY(LOCATION,(TString)"could not find any near neighbours for objects ... Something is horribly wrong ?!?",(knnListInp.size() > 0));

        const TMVA::kNN::Event evtRef(knnListInp.back().first->GetEvent());
        
        // find the distnace to the reference object we just found
        double dist_Ref_Inp = evtNow.GetDist(evtRef);
        double wgt_Ref_Inp  = evtRef.GetWeight();

        VERIFY(LOCATION,(TString)"Found negative weight in reference sample ... Something is horribly wrong ?!?",(wgt_Ref_Inp > 0));

        // find the minNobjInVol near objects in the reference chain, compared to the initial reference object. The number
        // of objects is estimated as the sum of their weights, scaled by the weight of the original refrence object. If weighs
        // are not defined, then the sum of weights will be exactly minNobjInVol. Otherwise, several searches may be needed...
        // -----------------------------------------------------------------------------------------------------------
        bool   foundDist(false);
        double dist_Ref0_RefNear(0);
        for(int nFracNow=0; nFracNow<nKnnFracs; nFracNow++) {
          if(!knnErrModule[1][nFracNow]) break;

          double wgtSum_Ref0_RefNear = 0;
          double minNobjInVolWgt     = minNobjInVol * wgt_Ref_Inp / pow(knnFracFact,nFracNow);

          knnErrModule[1][nFracNow]->Find(evtRef,minNobjInVol);
          const TMVA::kNN::List & knnListRef = knnErrModule[1][nFracNow]->GetkNNList();

          for(TMVA::kNN::List::const_iterator lit=knnListRef.begin(); lit!=knnListRef.end(); ++lit) {
            const TMVA::kNN::Event & evtLst = lit->first->GetEvent();

            double distNow = evtRef.GetDist(evtLst); if(distNow < EPS) continue; // the first element is the initial object (-> zero distance)
            double wgtNow  = evtLst.GetWeight();

            VERIFY(LOCATION,(TString)"Found negative weight in reference sample ... Something is horribly wrong ?!?",(wgtNow > 0));

            dist_Ref0_RefNear    = distNow;
            wgtSum_Ref0_RefNear += wgtNow;

            if(wgtSum_Ref0_RefNear >= minNobjInVolWgt) { foundDist = true; break; }
          }
          if(foundDist) break;
        }

        if(foundDist) {
          // -----------------------------------------------------------------------------------------------------------
          // finally, compute the weight as the relative difference beween the distance between the distance
          // measures then compute a binary decision, based on the minimal threshold set by maxRelRatioInRef
          // -----------------------------------------------------------------------------------------------------------
          weightKNN = max( ((dist_Ref0_RefNear - dist_Ref_Inp) / dist_Ref0_RefNear) , 0.);
          if(maxRelRatioInRef > 0) weightKNN = (weightKNN > maxRelRatioInRef) ? 1 : 0;
          weightKNN = max(min(weightKNN,1.),0.);

          var_0->IncCntr("Found good weight");
          if(maxRelRatioInRef > 0) {
            if(weightKNN > maxRelRatioInRef) var_0->IncCntr(wgtKNNname+" = 1"); else var_0->IncCntr(wgtKNNname+" = 0");
          }
        }
        else {
          // assign zero weight if could not complete the calculation
          weightKNN = 0;

          var_0->IncCntr("Did not find good weight");
        }
      }
    }

    var_1->SetVarF(wgtKNNname,weightKNN);

    var_1->fillTree();

    mayWriteObjects = true; var_0->IncCntr("nObj"); /// Cant use this here !!! if(var_0->GetCntr("nObj") == maxNobj) breakLoop = true;
  }
  if(!breakLoop) { var_0->printCntr(outTreeName); outputs->WriteOutObjects(false,true); outputs->ResetObjects(); }
  
  DELNULL(var_0); DELNULL(var_1); DELNULL(outTree); outputs->TreeMap.erase(outTreeName);
  varTypeNameV.clear();

  TChain * aChainInpWgt(NULL);

  // -----------------------------------------------------------------------------------------------------------
  // normalize the derived weights - the normalization factor is such that the entire input sample will have
  // a sum of weights which is equal to the original number of entries in the sample
  // -----------------------------------------------------------------------------------------------------------
  if(doRelWgts) {
    VERIFY(LOCATION,(TString)"Got sum of weights from the entire main sample = "+utils->floatToStr(weightSum)
                            +" ... Something is horribly wrong ?!?! ",(weightSum > 0));

    double weightNorm = (hasChainEvl ? chainEntV[2] : chainEntV[0]) / weightSum;

    // create a temporary sub-dir for the output trees of the above
    // -----------------------------------------------------------------------------------------------------------
    TString outDirNameTMP = (TString)outDirNameFull+"tmpDir"+wgtNormTreeName+"/";
    TString mkdirCmnd     = (TString)"mkdir -p "+outDirNameTMP;
    utils->exeShellCmndOutput(mkdirCmnd,inLOG(Log::DEBUG),true);

    TString fileNameNow = (TString)outDirNameFull+outTreeName+"*.root";
    TString mvCmnd      = (TString)"mv "+fileNameNow+" "+outDirNameTMP;

    utils->exeShellCmndOutput(mvCmnd,inLOG(Log::DEBUG),true);
    fileNameNow.ReplaceAll(outDirNameFull,outDirNameTMP);

    aChainInpWgt = new TChain(outTreeName,outTreeName); aChainInpWgt->SetDirectory(0); aChainInpWgt->Add(fileNameNow);

    aLOG(Log::INFO) <<coutBlue<<" - Will move previous results into "<<coutPurple<<outDirNameTMP<<coutBlue
                    <<", and write normalized weights to "<<coutYellow<<fileNameNow<<coutBlue<<" ... "<<coutDef<<endl;

    // create the vars to read/write trees
    // -----------------------------------------------------------------------------------------------------------
    var_0 = new VarMaps(glob,utils,"treeWeightsKNNvar_0");
    var_1 = new VarMaps(glob,utils,"treeWeightsKNNvar_1");

    var_0->connectTreeBranches(aChainInpWgt);

    vector < pair<TString,TString> > varTypeNameV;
    var_1->varStruct(var_0,NULL,NULL,&varTypeNameV);

    outTree = new TTree(outTreeName,outTreeName); outTree->SetDirectory(0); outputs->TreeMap[outTreeName] = outTree;
    var_1->createTreeBranches(outTree); 

    // -----------------------------------------------------------------------------------------------------------
    // loop on the tree
    // -----------------------------------------------------------------------------------------------------------
    breakLoop = false; mayWriteObjects = false;
    var_0->clearCntr();
    for(Long64_t loopEntry=0; true; loopEntry++) {
      if(!var_0->getTreeEntry(loopEntry)) breakLoop = true;

      if((mayWriteObjects && var_0->GetCntr("nObj") % nObjectsToWrite == 0) || breakLoop) {
        var_0->printCntr(outTreeName); outputs->WriteOutObjects(false,true); outputs->ResetObjects();
        mayWriteObjects = false;
      }
      if(breakLoop) break;

      var_1->copyVarData(var_0,&varTypeNameV);

      double weightKNN = var_0->GetVarF(wgtKNNname) * weightNorm;
      var_1->SetVarF(wgtKNNname,weightKNN);

      var_1->fillTree();

      mayWriteObjects = true; var_0->IncCntr("nObj"); if(var_0->GetCntr("nObj") == maxNobj) breakLoop = true;
    }
    if(!breakLoop) { var_0->printCntr(outTreeName); outputs->WriteOutObjects(false,true); outputs->ResetObjects(); }

    DELNULL(var_1); DELNULL(outTree); outputs->TreeMap.erase(outTreeName);
    varTypeNameV.clear();

    // remove the temporary sub-dir and the intermediate trees inside
    utils->safeRM(outDirNameTMP,inLOG(Log::DEBUG));
  }
  // -----------------------------------------------------------------------------------------------------------
  // if there is no need to normalize the weights, just create a new chain and connect var_0 to it
  // -----------------------------------------------------------------------------------------------------------
  else {
    TString fileNameNow = (TString)outDirNameFull+outTreeName+"*.root";

    aChainInpWgt = new TChain(outTreeName,outTreeName); aChainInpWgt->SetDirectory(0); aChainInpWgt->Add(fileNameNow);

    aLOG(Log::INFO) <<coutRed<<" - Created refrence chain  "<<coutGreen<<outTreeName<<"("<<aChainInpWgt->GetEntries()<<")"
                    <<" from "<<coutBlue<<fileNameNow<<coutDef<<endl;

    // create the vars to read/write trees
    var_0 = new VarMaps(glob,utils,"treeWeightsKNNvar_0");
  }

  // -----------------------------------------------------------------------------------------------------------
  // setup chains for the output which has just been derived (needed for storeTreeToAscii() and for plotting)
  // -----------------------------------------------------------------------------------------------------------
  TString inFileName  = (TString)outDirNameFull+outTreeName+"*.root";
  TChain  * aChainOut = new TChain(outTreeName,outTreeName); aChainOut->SetDirectory(0); aChainOut->Add(inFileName);
  aLOG(Log::INFO) <<coutRed<<" - Created chain "<<coutGreen<<outTreeName<<"("<<aChainOut->GetEntries()<<")"<<" from "<<coutBlue<<inFileName<<coutDef<<endl;

  // -----------------------------------------------------------------------------------------------------------
  // write out the weights to an ascii file
  // -----------------------------------------------------------------------------------------------------------
  if(doStoreToAscii) {
    // extract the names of the variables which will be written out to the ascii output
    // including the derived KNN weights. Add the actuall weight variable if not already included
    vector <TString> outVarNames = utils->splitStringByChar(outAsciiVars,';');
    if(find(outVarNames.begin(),outVarNames.end(), wgtKNNname) == outVarNames.end()) outVarNames.push_back(wgtKNNname);

    // create a VarMaps, connect it to the tree, and write out the requested variables
    VarMaps * var_2 = new VarMaps(glob,utils,"treeRegClsVar_2");
    var_2->connectTreeBranches(aChainOut);
    var_2->storeTreeToAscii((TString)wgtKNNname+aChainInpEvl->GetName(),"",0,nObjectsToWrite,"",&outVarNames,NULL);

    DELNULL(var_2); outVarNames.clear();
  }

  // -----------------------------------------------------------------------------------------------------------
  // some histograms to asses the weights
  // -----------------------------------------------------------------------------------------------------------
  if(doPlots) {
    vector <TString> branchNameV_0, branchNameV;

    var_0->GetAllVarNames(branchNameV_0);
    int nBranches = (int)branchNameV_0.size();

    branchNameV.push_back(wgtKNNname);
    for(int nBranchNow=0; nBranchNow<nBranches; nBranchNow++) {
      TString branchName = branchNameV_0[nBranchNow];
      TString branchType = var_0->GetVarType(branchName);

      if(branchType != "F" && branchType != "D" && branchType != "I")         continue;
      if(branchName.BeginsWith(basePrefix))                                   continue;
      if(find(varNames.begin(),varNames.end(), branchName) != varNames.end()) continue;

      // only accept branches common to all chains
      int hasBranch(0);
      for(int nChainNow=0; nChainNow<3; nChainNow++) {
        TChain  * aChain(NULL);
        if     (nChainNow == 0) { aChain = aChainRef;    }
        else if(nChainNow == 1) { aChain = aChainInpEvl; }
        else if(nChainNow == 2) { aChain = aChainOut;    }

        if(aChain->FindBranch(branchName)) hasBranch++;
      }
      if(hasBranch < 3) continue;      

      branchNameV.push_back(branchName);
    }
    nBranches = (int)branchNameV.size();

    for(int nVarNow=0; nVarNow<nVars+nBranches; nVarNow++) {
      TString nVarName = TString::Format("_nVar%d",nVarNow);
      
      TString varNameNow(""), baseName("");
      if(nVarNow < nVars) { baseName = "varWeightKNN_"; varNameNow = varNames[nVarNow];          }
      else                { baseName = "varFullChain_"; varNameNow = branchNameV[nVarNow-nVars]; }

      // draw each variable twice for each chain - the first time is for deriving the common limits for the
      // histogram across all chains, the second one is for using these common limits
      // -----------------------------------------------------------------------------------------------------------
      int           nDrawBins(50);
      double        drawLim0(1), drawLim1(-1);
      vector <TH1*> his1V;
      for(int nDrawNow=0; nDrawNow<2; nDrawNow++) {
        for(int nChainNow=0; nChainNow<3; nChainNow++) {
          TString nChainKNNname = TString::Format("_nChain%d",nChainNow);

          TString weightNow("1"), hisTitle("");
          TChain  * aChain(NULL);
          if     (nChainNow == 0) { aChain = aChainRef;    hisTitle = "Reference"; }
          else if(nChainNow == 1) { aChain = aChainInpEvl; hisTitle = "Original"; }
          else if(nChainNow == 2) { aChain = aChainOut;    hisTitle = "Weighted ("+typePostfix+")"; }
          hisTitle.ReplaceAll("_","");

          if(nChainNow == 2 && wgtKNNname != weightName) weightNow = wgtKNNname;

          if(nChainNow == 0) {
            if(chainWgtV[1] != "") weightNow = (TString)"("+weightNow+")*("+chainWgtV[1]+")";
            if(chainCutV[1] != "") weightNow = (TString)"("+weightNow+")*("+chainCutV[1]+")";
          }
          else {
            if(chainWgtV[0] != "") weightNow = (TString)"("+weightNow+")*("+chainWgtV[0]+")";
            if(chainCutV[0] != "") weightNow = (TString)"("+weightNow+")*("+chainCutV[0]+")";
          }

          if(nChainNow == 1 && wgtKNNname == weightName) weightNow.ReplaceAll(weightName,"1");

          if(varNameNow == wgtKNNname) {
            if(nChainNow != 2) continue;
            else               weightNow = "1";
          }
          weightNow = utils->cleanWeightExpr(weightNow);

          TString hisName   = (TString)baseName+aChainInpEvl->GetName()+nVarName+nChainKNNname;
          TString drawExprs = (TString)"("+varNameNow+")>>"+hisName;
          if(nDrawNow == 1) drawExprs += TString::Format("(%d,%f,%f)",nDrawBins,drawLim0,drawLim1);
          // cout <<drawExprs<<" \t --> "<<weightNow <<endl;

          utils->drawTree(aChain,drawExprs,weightNow);

          TH1 * his1 = (TH1F*)gDirectory->Get(hisName); his1->SetDirectory(0); his1->BufferEmpty(); his1->SetTitle(hisTitle);
          VERIFY(LOCATION,(TString)"Could not derive histogram ("+hisName+") from chain ... Something is horribly wrong ?!?!",(dynamic_cast<TH1F*>(his1)));
          
          // derive the common histogram limits for all chains
          if(nDrawNow == 0) {
            bool   needUpdate  = (drawLim0 > drawLim1);
            double drawLimNow0 = his1->GetXaxis()->GetBinLowEdge(his1->GetXaxis()->GetFirst());
            double drawLimNow1 = his1->GetXaxis()->GetBinUpEdge (his1->GetXaxis()->GetLast() );

            if(needUpdate || drawLim0 > drawLimNow0) drawLim0 = drawLimNow0;
            if(needUpdate || drawLim1 < drawLimNow1) drawLim1 = drawLimNow1;

            DELNULL(his1);
          }
          // store the histogram
          else {
            his1V.push_back(his1);
          }
        }
      }

      outputs->optClear();
      outputs->draw->NewOptC("drawOpt_0" , "HIST");
      outputs->draw->NewOptC("drawOpt_1" , "e1p");
      outputs->draw->NewOptC("drawOpt_2" , "e1p");
      outputs->draw->NewOptC("axisTitleX" , varNameNow);
      outputs->draw->NewOptC("axisTitleY" , "1/N dN/dx");
      outputs->draw->NewOptB("doNormIntegralWidth" , true);
      outputs->drawHis1dV(his1V);

      his1V.clear();
    }

    outputs->WriteOutObjects(true,true); outputs->ResetObjects();
    branchNameV.clear(); branchNameV_0.clear();
  }

  // cleanup
  DELNULL(var_0); DELNULL(aChainInpWgt);

  for(int nChainNow=0; nChainNow<2; nChainNow++) {
    for(int nFracNow=0; nFracNow<nKnnFracs; nFracNow++) {
      DELNULL(knnErrFactory[nChainNow][nFracNow]);
      DELNULL(knnErrOutFile[nChainNow][nFracNow]);

      if(!ROOT_TMVA_V0) DELNULL(knnErrDataLdr[nChainNow][nFracNow]);

      // after closing a TFile, need to return to the correct directory, or else histogram pointers will be affected
      outputs->BaseDir->cd();

      utils->safeRM(outFileNameKnnErr[nChainNow][nFracNow],inLOG(Log::DEBUG));
    }
  }
  utils->safeRM(outFileDirKnnErrV,inLOG(Log::DEBUG));

  knnErrOutFile.clear(); knnErrFactory.clear(); knnErrDataLdr.clear(); knnErrMethod.clear(); knnErrModule.clear(); aChainV.clear();
  minMaxVarVals.clear(); outFileNameKnnErr.clear(); varNames.clear(); chainWgtV.clear(); chainCutV.clear();
  varFormNames.clear(); objNowV.clear(); distV.clear(); weightSumV.clear(); distIndexV.clear();
  chainEntV.clear(); varNamesScaled.clear();

  for(int nChainNow=0; nChainNow<2; nChainNow++) { for(int nVarNow=0; nVarNow<nVars; nVarNow++) { DELNULL(hisVarV[nChainNow][nVarNow]); } }
  hisVarV.clear();

  return;
}














