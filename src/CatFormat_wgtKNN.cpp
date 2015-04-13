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
void CatFormat::asciiToSplitTree_wgtKNN(TString inAsciiFiles, TString inAsciiVars, TString inAsciiFiles_wgtKNN, TString inAsciiVars_wgtKNN) {
// ==========================================================================================================================================
  aLOG(Log::INFO) <<coutWhiteOnBlack<<coutBlue<<" - starting ANNZ::asciiToSplitTree_wgtKNN() ... "<<coutDef<<endl;

  int     nSplit         = glob->GetOptI("nSplit");
  TString treeName       = glob->GetOptC("treeName");
  TString outDirNameFull = glob->GetOptC("outDirNameFull");
  TString wgtTreeName    = "_wgtTree";

  // -----------------------------------------------------------------------------------------------------------
  // the ascii input files from which the KNN weights are derived
  // -----------------------------------------------------------------------------------------------------------
  asciiToFullTree(inAsciiFiles_wgtKNN,inAsciiVars_wgtKNN,wgtTreeName);

  // -----------------------------------------------------------------------------------------------------------
  // the main ascii input files
  // -----------------------------------------------------------------------------------------------------------
  asciiToSplitTree(inAsciiFiles,inAsciiVars);

  // -----------------------------------------------------------------------------------------------------------
  // replace the trees created in asciiToSplitTree() with new trees which also have KNN weights according to
  // the reference dataset created in asciiToFullTree()
  // -----------------------------------------------------------------------------------------------------------
  // setup the reference chain for the KNN input
  // -----------------------------------------------------------------------------------------------------------
  TString treeNameRef = (TString)treeName+wgtTreeName;
  TString fileNameRef = (TString)outDirNameFull+treeNameRef+"*.root";

  TChain  * aChainRef = new TChain(treeNameRef,treeNameRef); aChainRef->SetDirectory(0); aChainRef->Add(fileNameRef);
  aLOG(Log::INFO) <<coutRed<<" - Created Knn  chain  "<<coutGreen<<treeNameRef<<"("<<aChainRef->GetEntries()<<")"
                  <<" from "<<coutBlue<<fileNameRef<<coutDef<<endl;

  // derive the names of the trees created by asciiToSplitTree()
  // -----------------------------------------------------------------------------------------------------------
  int nChains = min(nSplit,2);
  vector <TString> treeNames(nChains);

  if(nChains == 1) { treeNames[0] = (TString)treeName;                                                     }
  else             { treeNames[0] = (TString)treeName+"_train"; treeNames[1] = (TString)treeName+"_valid"; }

  // create a temporary sub-dir for the output trees of asciiToSplitTree()
  // -----------------------------------------------------------------------------------------------------------
  TString outDirNameTMP = (TString)outDirNameFull+"tmpDir"+wgtTreeName+"/";
  TString mkdirCmnd     = (TString)"mkdir -p "+outDirNameTMP;
  utils->exeShellCmndOutput(mkdirCmnd,inLOG(Log::DEBUG),true);

  for(int nChainNow=0; nChainNow<nChains; nChainNow++) {
    TString treeNameNow = (TString)treeNames[nChainNow];
    TString fileNameNow = (TString)outDirNameFull+treeNameNow+"*.root";
    TString mvCmnd      = (TString)"mv "+fileNameNow+" "+outDirNameTMP;

    // move the output trees of asciiToSplitTree() to the temporary sub-dir, and create a corresponding chain
    // -----------------------------------------------------------------------------------------------------------
    utils->exeShellCmndOutput(mvCmnd,inLOG(Log::DEBUG),true);
    fileNameNow.ReplaceAll(outDirNameFull,outDirNameTMP);

    TChain * aChain = new TChain(treeNameNow,treeNameNow); aChain->SetDirectory(0); aChain->Add(fileNameNow);
    aLOG(Log::INFO) <<coutRed<<" - Created main chain  "<<coutGreen<<treeNameNow<<"("<<aChain->GetEntries()<<")"
                    <<" from "<<coutBlue<<fileNameNow<<coutDef<<endl;

    addWgtKNNtoTree(aChain,aChainRef);

    DELNULL(aChain);
  }

  // cleanup
  DELNULL(aChainRef);

  // remove the temporary sub-dir and the intermediary trees inside
  utils->safeRM(outDirNameTMP,inLOG(Log::DEBUG));

  treeNames.clear();

  return;
}

// ===========================================================================================================
/**
 * @brief                       - create root trees from the input ascii files and add a weight branch, which estimates
 *                              if each objects is "near enough" to enough objects in the training dataset.
 * 
 * @param inAsciiFiles          - semicolon-separated list of input ascii files (main dataset)
 * @param inAsciiVars           - semicolon-separated list of input parameter names, corresponding to columns in the input files (main dataset)
 * @param treeNamePostfix       - postfix for the final output trees
 */
// ===========================================================================================================
void CatFormat::asciiToFullTree_wgtKNN(TString inAsciiFiles, TString inAsciiVars, TString treeNamePostfix) {
// =========================================================================================================
  aLOG(Log::INFO) <<coutWhiteOnBlack<<coutBlue<<" - starting ANNZ::asciiToFullTree_wgtKNN() ... "<<coutDef<<endl;

  TString treeName       = glob->GetOptC("treeName");
  TString outDirNameFull = glob->GetOptC("outDirNameFull");
  TString wgtTreeName    = "_wgtTree";

  // the input tree for which the weights will be calculated
  asciiToFullTree(inAsciiFiles,inAsciiVars,wgtTreeName);

  // setup the chain for the input which has just been produced by asciiToFullTree
  // -----------------------------------------------------------------------------------------------------------
  TString treeNameInp = (TString)treeName+wgtTreeName;
  TString fileNameInp = (TString)outDirNameFull+treeNameInp+"*.root";

  TChain  * aChainInp = new TChain(treeNameInp,treeNameInp); aChainInp->SetDirectory(0); aChainInp->Add(fileNameInp);
  aLOG(Log::INFO) <<coutRed<<" - Created input    chain  "<<coutGreen<<treeNameInp<<"("<<aChainInp->GetEntries()<<")"
                  <<" from "<<coutBlue<<fileNameInp<<coutDef<<endl;

  // setup reference chain from the training directory
  // -----------------------------------------------------------------------------------------------------------
  TString treeNameRef = (TString)treeName+"_train";
  TString fileNameRef = (TString)glob->GetOptC("inputTreeDirName")+treeNameRef+"*.root";

  TChain  * aChainRef = new TChain(treeNameRef,treeNameRef); aChainRef->SetDirectory(0); aChainRef->Add(fileNameRef);
  aLOG(Log::INFO) <<coutRed<<" - Created refrence chain  "<<coutGreen<<treeNameRef<<"("<<aChainRef->GetEntries()<<")"
                  <<" from "<<coutBlue<<fileNameRef<<coutDef<<endl;

  // the name of the final tree
  TString treeFinalName = (TString)treeName+treeNamePostfix;

  addWgtKNNtoTree(aChainInp,aChainRef,treeFinalName);

  // remove the intermidiate trees created by asciiToFullTree
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
 * @param outTreeName   - optional tree name (or else the name is extracted from aChainInp)
 */
// ===========================================================================================================
void CatFormat::addWgtKNNtoTree(TChain * aChainInp, TChain * aChainRef, TString outTreeName) {
// ===========================================================================================
  VERIFY(LOCATION,(TString)"Memory leak ?!?",(dynamic_cast<TChain*>(aChainInp) && dynamic_cast<TChain*>(aChainRef)));

  if(outTreeName == "") outTreeName = aChainInp->GetName();
  aLOG(Log::INFO) <<coutWhiteOnBlack<<coutBlue<<" - starting ANNZ::addWgtKNNtoTree("<<coutYellow<<outTreeName<<coutBlue<<") ... "<<coutDef<<endl;

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

  TString typePostfix      = (TString)(doRelWgts ? "_wgtKNN" : "_inTrain");
  TString wgtKNNname       = glob->GetOptC((TString)"baseName"      +typePostfix); // e.g., "baseName_wgtKNN"
  TString outAsciiVars     = glob->GetOptC((TString)"outAsciiVars"  +typePostfix); // e.g., "outAsciiVars_wgtKNN"
  TString weightVarNames   = glob->GetOptC((TString)"weightVarNames"+typePostfix); // e.g., "weightVarNames_wgtKNN"
  int     minNobjInVol     = glob->GetOptI((TString)"minNobjInVol"  +typePostfix); // e.g., "minNobjInVol_wgtKNN"
  double  sampleFracInp    = glob->GetOptF((TString)"sampleFracInp" +typePostfix); // e.g., "sampleFracInp_wgtKNN"
  double  sampleFracRef    = glob->GetOptF((TString)"sampleFracRef" +typePostfix); // e.g., "sampleFracRef_wgtKNN"

  vector <TString> chainWgtV(2), chainCutV(2);

  chainWgtV[0] = glob->GetOptC((TString)"weightInp"+typePostfix); chainWgtV[1] = glob->GetOptC((TString)"weightRef"+typePostfix);
  chainCutV[0] = glob->GetOptC((TString)"cutInp"   +typePostfix); chainCutV[1] = glob->GetOptC((TString)"cutRef"   +typePostfix);

  // force reasonable min/max values
  minNobjInVol     = max(minNobjInVol,20);
  maxRelRatioInRef = max(min(maxRelRatioInRef,0.999),0.001);

  int     maxNobj         = 0;  // maxNobj = glob->GetOptI("maxNobj"); // only allow maxNobj limits for debugging !! 
  TString outBaseName     = (TString)outDirNameFull+glob->GetOptC("treeName")+wgtKNNname;
  TString wgtNormTreeName = (TString)"_normWgt";

  // decompose the variable names for the KNN distance calculation
  vector <TString> varNames = utils->splitStringByChar(weightVarNames,';');

  int nVars = (int)varNames.size();
  VERIFY(LOCATION,(TString)"Did not find input variables for KNN weight computation [\"weightVarNames_wgtKNN\"/\"weightVarNames_inTrain\" = "
                          +weightVarNames+"] ... Something is horribly wrong !?!?",(nVars > 0));

  VERIFY(LOCATION,(TString)"sampleFracInp_wgtKNN must be a positive number, smaller or equal to 1. Currently is set to "+utils->floatToStr(sampleFracInp)
                 ,(sampleFracInp > 0 && sampleFracInp <= 1));
  VERIFY(LOCATION,(TString)"sampleFracRef_wgtKNN must be a positive number, smaller or equal to 1. Currently is set to "+utils->floatToStr(sampleFracRef)
                 ,(sampleFracRef > 0 && sampleFracRef <= 1));
  
  aLOG(Log::INFO) <<coutPurple<<" - will use the following variables for the KNN search: "<<coutYellow<<weightVarNames<<coutDef<<endl;

  vector < vector <double> > minMaxVarVals(2,vector<double>(nVars));

  // -----------------------------------------------------------------------------------------------------------
  // setup the kd-trees for the two chains
  // -----------------------------------------------------------------------------------------------------------
  vector <TChain          *> aChainV(2);
  vector <TFile           *> knnErrOutFile(2); vector <TMVA::Factory        *> knnErrFactory(2);
  vector <TMVA::MethodKNN *> knnErrMethod(2);  vector <TMVA::kNN::ModulekNN *> knnErrModule(2);

  vector <TString> outFileDirKnnErrV(2), outFileNameKnnErr(2);
  vector <double>  chainWgtNormV(2,0), chainEntV(2,0);

  for(int nChainNow=0; nChainNow<2; nChainNow++) {
    TString nChainKNNname = TString::Format("_nChainKNN%d",nChainNow);

    // clone the chain used for the kd-tree (needed, as the kd-tree will turn off branches which
    // are not used, but these are needed for var_0, var_1) - The Clone should not be deleted
    // as this may cause seg-fault (perhaps due to closing of underlying TFile somwhere by ROOT)
    // -----------------------------------------------------------------------------------------------------------
    TChain * aChain = (nChainNow == 0) ? aChainInp : aChainRef;
    aChainV[nChainNow]   = (TChain*)aChain->Clone((TString)aChain->GetName()+nChainKNNname);  aChainV[nChainNow]->SetDirectory(0);

    // make sure the input variables all exist in the chain
    vector <TString> branchNameV;
    utils->getTreeBranchNames(aChain,branchNameV);

    for(int nVarNow=0; nVarNow<nVars; nVarNow++) {
      bool hasBranch = (find(branchNameV.begin(),branchNameV.end(),varNames[nVarNow]) != branchNameV.end());

      VERIFY(LOCATION,(TString)"got KNN-variable = \""+varNames[nVarNow]+"\" ... variables included in \"weightVarNames"+typePostfix
                              +"\" must exist in the input file (complex expressions of input variables are not supported)",hasBranch);
    }
    
    branchNameV.clear();

    double  objFracNow   = (nChainNow == 0) ? sampleFracInp : sampleFracRef;
    int     nTrainObj    = static_cast<int>(floor(aChainV[nChainNow]->GetEntries() * objFracNow));

    VERIFY(LOCATION,(TString)"Following sampleFracInp_wgtKNN/sampleFracRef_wgtKNN cut, chain("+(TString)aChain->GetName()+") with initial "
                            +utils->lIntToStr(aChainV[nChainNow]->GetEntries())+" objects, now has "+utils->lIntToStr(nTrainObj)
                            +" objects (minimum is \"minObjTrainTest\" = "+utils->lIntToStr(minObjTrainTest)+") ...",(nTrainObj >= minObjTrainTest));

    int     split0   = 20;
    int     split1   = static_cast<int>(floor(split0/objFracNow));
    TString fracCut  = (TString)indexName+" % "+TString::Format("%d < %d",split1,split0);
    TCut    finalCut = ((TCut)fracCut) + ((TCut)chainCutV[nChainNow]);

    // add the "baseName_wgtKNN" variable to the weight expression - this is just unity if generating the
    // initial trees, but may hold non-trivial values for the [doRelWgts==false] mode
    chainWgtV[nChainNow] = utils->cleanWeightExpr((TString)"("+chainWgtV[nChainNow]+")*"+weightName);

    outFileDirKnnErrV[nChainNow] = outBaseName+"_weights"+nChainKNNname+"/";
    outFileNameKnnErr[nChainNow] = outBaseName+nChainKNNname+".root";

    (TMVA::gConfig().GetIONames()).fWeightFileDir = outFileDirKnnErrV[nChainNow];

    bool    debug           = inLOG(Log::DEBUG_2);
    TString verbLvlF        = (TString)(debug ? ":V:!Silent"             : ":!V:Silent");
    TString verbLvlM        = (TString)(debug ? ":V:H"                   : ":!V:!H");
    TString drawProgBarStr  = (TString)(debug ? ":Color:DrawProgressBar" : ":Color:!DrawProgressBar");
    TString optKNN          = (TString)":nkNN=0:ScaleFrac=0.0";
    TString transStr        = (TString)":Transformations=I,N";
    TString analysType      = (TString)":AnalysisType=Regression";
    TString trainValidStr   = (TString)"nTrain_Regression=0:nTest_Regression=0:SplitMode=Random:NormMode=NumEvents:!V";

    // setup the factory
    // -----------------------------------------------------------------------------------------------------------
    knnErrOutFile[nChainNow] = new TFile(outFileNameKnnErr[nChainNow],"RECREATE");
    knnErrFactory[nChainNow] = new TMVA::Factory(wgtKNNname, knnErrOutFile[nChainNow], (TString)verbLvlF+drawProgBarStr+transStr+analysType);    

    // define all input variables as floats in the factory
    for(int nVarNow=0; nVarNow<nVars; nVarNow++) knnErrFactory[nChainNow]->AddVariable(varNames[nVarNow],"","",'F');

    knnErrFactory[nChainNow]->AddRegressionTree(aChainV[nChainNow], 1, TMVA::Types::kTraining);
    knnErrFactory[nChainNow]->SetWeightExpression(chainWgtV[nChainNow],"Regression");

    knnErrFactory[nChainNow]->PrepareTrainingAndTestTree(finalCut,trainValidStr);

    knnErrMethod[nChainNow] = dynamic_cast<TMVA::MethodKNN*>(knnErrFactory[nChainNow]->BookMethod(TMVA::Types::kKNN, wgtKNNname,(TString)optKNN+verbLvlM));
    knnErrModule[nChainNow] = knnErrMethod[nChainNow]->fModule;

    // fill the module with events made from the tree entries and create the binary tree
    // -----------------------------------------------------------------------------------------------------------
    knnErrMethod[nChainNow]->Train();

    // calculate the sum of weights in the chain for the normalization
    // -----------------------------------------------------------------------------------------------------------
    double sumWgt(0), sumEnt(0);
    for(int nVarNow=0; nVarNow<nVars; nVarNow++) {
      if(nChainNow == 0 && nVarNow > 0) break;

      TString hisName   = (TString)nChainKNNname+"_hisTMP_"+varNames[nVarNow];
      TString drawExprs = (TString)varNames[nVarNow]+">>"+hisName;
      TString wgtCut    = (TString)"("+chainCutV[nChainNow]+")*("+chainWgtV[nChainNow]+")";
      wgtCut.ReplaceAll("()*()","").ReplaceAll("*()","").ReplaceAll("()*","").ReplaceAll("()","1");

      TCanvas * tmpCnvs = new TCanvas("tmpCnvs","tmpCnvs");
      aChain->Draw(drawExprs,wgtCut); DELNULL(tmpCnvs);

      TH1 * his1 = (TH1F*)gDirectory->Get(hisName); his1->SetDirectory(0); his1->BufferEmpty();
      VERIFY(LOCATION,(TString)"Could not derive histogram ("+hisName+") from chain ... Something is horribly wrong ?!?!",(dynamic_cast<TH1F*>(his1)));
      
      if(nVarNow == 0) { sumWgt = his1->Integral(); sumEnt = his1->GetEntries(); }
      
      if(nChainNow == 1) {
        vector <double> fracV, quantV(2,-1);
        fracV.push_back(0.0); fracV.push_back(1.0);

        int hasQuant = utils->getQuantileV(fracV,quantV,his1);
        VERIFY(LOCATION,(TString)"Got not compute quantiles for histogram("+hisName+") ... Something is horribly wrong ?!?! ",hasQuant);

        // pad the derived range by the maximal of the two: half the bin width of the historgam; 1% of the range of values
        double rangeAdd = max(his1->GetBinWidth(1)/2. , 0.01 * (quantV[1] - quantV[0]));

        minMaxVarVals[0][nVarNow] = quantV[0] - rangeAdd;
        minMaxVarVals[1][nVarNow] = quantV[1] + rangeAdd;

        fracV.clear(); quantV.clear();
      }
      
      DELNULL(his1);
    }

    aLOG(Log::INFO)  <<coutGreen<<" - "<<coutBlue<<aChainV[nChainNow]->GetName()<<coutGreen<<" - kd-tree (effective entries = "
                     <<knnErrMethod[nChainNow]->fEvent.size()<<")"<<coutYellow<<" , opts = "<<coutRed<<optKNN<<coutYellow<<" , "<<coutRed
                     <<trainValidStr<<coutYellow<<" , cuts = "<<coutRed<<finalCut<<coutYellow<<" , weight expression: "<<coutRed
                     <<chainWgtV[nChainNow]<<coutYellow<<" , sum of entries/weights = "<<coutPurple<<sumEnt<<coutYellow<<" / "
                     <<coutPurple<<sumWgt<<coutDef<<endl;

    // sanity check - if this is not true, the events in the binary tree will have the wrong "units"
    VERIFY(LOCATION,(TString)"Somehow the fScaleFrac for the kd-tree is not zero ... Something is horribly wrong ?!?!?"
                             ,(knnErrMethod[nChainNow]->fScaleFrac < EPS));

    VERIFY(LOCATION,(TString)"Got sum of entries = "+utils->floatToStr(sumEnt)+" ... Something is horribly wrong ?!?! ",(sumEnt > 0));
    VERIFY(LOCATION,(TString)"Got sum of weights = "+utils->floatToStr(sumWgt)+" ... Something is horribly wrong ?!?! ",(sumWgt > 0));
    
    chainEntV[nChainNow]     = sumEnt;
    chainWgtNormV[nChainNow] = 1/sumWgt;  

    outputs->BaseDir->cd();
  }

  // chainWgtNormV now holds normalization, such that the weighted sum of entries in the main and in the
  // reference chains is the same (multiplying by the max(chainEntV[0],chainEntV[1]) is not strictly
  // needed, but may reduce numerical errors when dealing with large input samples)
  // -----------------------------------------------------------------------------------------------------------
  for(int nChainNow=0; nChainNow<2; nChainNow++) { chainWgtNormV[nChainNow] *= max(chainEntV[0],chainEntV[1]); }

  // -----------------------------------------------------------------------------------------------------------
  // create the vars to read/write trees
  // -----------------------------------------------------------------------------------------------------------
  VarMaps * var_0 = new VarMaps(glob,utils,"treeWeightsKNNvar_0");
  VarMaps * var_1 = new VarMaps(glob,utils,"treeWeightsKNNvar_1");
  
  // add unique formula names (to avoid conflict with existing variable names)
  vector <TString> varFormNames(nVars);
  for(int nVarNow=0; nVarNow<nVars; nVarNow++) {
    varFormNames[nVarNow] = (TString)wgtKNNname+"_"+varNames[nVarNow];

    var_0->NewForm(varFormNames[nVarNow],varNames[nVarNow]);
  }

  var_0->connectTreeBranches(aChainInp);

  var_1->copyVarStruct(var_0);

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
  vector <int>      distIndexV(2);
  vector <double>   distV(2), weightSumV(2);
  TMVA::kNN::VarVec objNowV(nVars,0);

  int   nObjectsToPrint = min(static_cast<int>(aChainInp->GetEntries()/10.) , glob->GetOptI("nObjectsToPrint"));
  bool  breakLoop(false), mayWriteObjects(false);
  var_0->clearCntr();
  for(Long64_t loopEntry=0; true; loopEntry++) {
    if(!var_0->getTreeEntry(loopEntry)) breakLoop = true;

    if((mayWriteObjects && var_0->GetCntr("nObj") % nObjectsToWrite == 0) || breakLoop) {
      var_0->printCntr(outTreeName); outputs->WriteOutObjects(false,true); outputs->ResetObjects();
      mayWriteObjects = false;
    }
    else if(var_0->GetCntr("nObj") % nObjectsToPrint == 0) { var_0->printCntr(outTreeName); }

    if(breakLoop) break;

    var_1->copyVarData(var_0);

    // fill the current object vector and use it later in order to create a TMVA::kNN::Event object
    // if any of the variables is beyond the limits derived from the reference sample, the weight is
    // automatically set to zero, and not other computation is needed
    // -----------------------------------------------------------------------------------------------------------
    bool isInsideRef(true);
    for(int nVarNow=0; nVarNow<nVars; nVarNow++) {
      objNowV[nVarNow] = var_0->GetForm(varFormNames[nVarNow]);

      if(objNowV[nVarNow] < minMaxVarVals[0][nVarNow] || objNowV[nVarNow] > minMaxVarVals[1][nVarNow]) {
        isInsideRef = false;
        var_0->IncCntr("Found good weight"); var_0->IncCntr(wgtKNNname+" = 0 (outside input parameter range)");
        break;
      }
    }

    double weightKNN(0);
    if(isInsideRef) {
      // -----------------------------------------------------------------------------------------------------------
      // derive the weights as the ratio between input and reference samples, if needed
      // -----------------------------------------------------------------------------------------------------------
      if(doRelWgts) {
        const TMVA::kNN::Event evtNow(objNowV,1,0);

        // find the same number of near neighbours for each chain, and derive the distance this requires
        int    nObjKNN(minNobjInVol);
        for(int nChainNow=0; nChainNow<2; nChainNow++) {
          double wgtNorm = chainWgtNormV[nChainNow];

          knnErrModule[nChainNow]->Find(evtNow,nObjKNN);
          const TMVA::kNN::List & knnList = knnErrModule[nChainNow]->GetkNNList();

          weightSumV[nChainNow] = 0;
          for(TMVA::kNN::List::const_iterator lit=knnList.begin(); lit!=knnList.end(); ++lit) {
            double wgtNow = (lit->first->GetEvent()).GetWeight() * wgtNorm;

            VERIFY(LOCATION,(TString)"Found negative weight in reference sample ... Something is horribly wrong ?!?",(wgtNow > 0));

            weightSumV[nChainNow] += wgtNow;
          }
          distV[nChainNow] = evtNow.GetDist(knnList.back().first->GetEvent());
        }
        // the index of the chain with the shorter distance
        if(distV[0] < distV[1]) { distIndexV[0] = 0; distIndexV[1] = 1; }
        else                    { distIndexV[0] = 1; distIndexV[1] = 0; }

        bool   foundDist(false);
        double weightSumNow(0), wgtNorm(chainWgtNormV[distIndexV[0]]);
        int    preNobj(0);
        for(int nSearchNow=0; nSearchNow<10; nSearchNow++) {
          preNobj = nObjKNN; nObjKNN *= 5;

          knnErrModule[distIndexV[0]]->Find(evtNow,nObjKNN);
          const TMVA::kNN::List & knnList = knnErrModule[distIndexV[0]]->GetkNNList();

          for(TMVA::kNN::List::const_iterator lit=std::next(knnList.begin(),preNobj); lit!=knnList.end(); ++lit) {
            const TMVA::kNN::Event & eventNow = lit->first->GetEvent();

            double knnDistNow = evtNow.GetDist(eventNow);
            double weightObj  = eventNow.GetWeight() * wgtNorm;

            VERIFY(LOCATION,(TString)"Found negative weight in reference sample ... Something is horribly wrong ?!?",(weightObj > 0));

            if(knnDistNow < distV[distIndexV[1]]) { weightSumNow += weightObj;        }
            else                                  { foundDist     = true;      break; }
          }
          if(foundDist) {
            weightSumV[distIndexV[0]] += weightSumNow;
            if(weightSumV[1] * weightSumV[0] > EPS) weightKNN = weightSumV[1]/weightSumV[0];
            break;
          }
        }
        if(foundDist) { weightSum += weightKNN;
                        var_0->IncCntr("Found good weight");         }
        else          { var_0->IncCntr("Did not fined good weight"); }
      }
      // -----------------------------------------------------------------------------------------------------------
      // derive the weight from the approximated density estimation of near objects from the reference sample, if needed
      // -----------------------------------------------------------------------------------------------------------
      else {
        const TMVA::kNN::Event evtInp(objNowV,1,0);

        // find the closest object in the reference chain
        knnErrModule[1]->Find(evtInp,1);
        const TMVA::kNN::List & knnListInp = knnErrModule[1]->GetkNNList();

        // must make a sanith check before using the pointer to GetEvent()
        VERIFY(LOCATION,(TString)"could not find any near neighbours for objects ... Something is horribly wrong ?!?",(knnListInp.size() > 0));

        const TMVA::kNN::Event evtRef(knnListInp.back().first->GetEvent());
        
        // find the diatnace to the reference object we just found
        double dist_Ref_Inp = evtInp.GetDist(evtRef);
        double wgt_Ref_Inp  = evtRef.GetWeight();

        VERIFY(LOCATION,(TString)"Found negative weight in reference sample ... Something is horribly wrong ?!?",(wgt_Ref_Inp > 0));

        // find the minNobjInVol near objects in the reference chain, compared to the initial reference object. The number
        // of objects is estimated as the sum of their weights, scaled by the weight of the original refrence object. If weighs
        // are not defined, then the sum of weights will be exactly minNobjInVol. Otherwise, several searches with increasing
        // NN numbers may be needed...
        // -----------------------------------------------------------------------------------------------------------
        int    minNobjInVolNow(minNobjInVol), minNobjInVolPrev(0);
        bool   foundDist(false);
        double wgtSum_Ref0_RefNear(0), dist_Ref0_RefNear(0);

        for(int nSearchNow=0; nSearchNow<10; nSearchNow++) {
          knnErrModule[1]->Find(evtRef,minNobjInVolNow+1);
          const TMVA::kNN::List & knnListRef = knnErrModule[1]->GetkNNList();

          for(TMVA::kNN::List::const_iterator lit=std::next(knnListRef.begin(),minNobjInVolPrev); lit!=knnListRef.end(); ++lit) {
            const TMVA::kNN::Event & eventRefNow = lit->first->GetEvent();

            double distNow = evtRef.GetDist(eventRefNow); if(distNow < EPS) continue; // the first element is the initial object (-> zero distance)
            double wgtNow  = eventRefNow.GetWeight();

            VERIFY(LOCATION,(TString)"Found negative weight in reference sample ... Something is horribly wrong ?!?",(wgtNow > 0));

            dist_Ref0_RefNear    = distNow;
            wgtSum_Ref0_RefNear += wgtNow;

            if(wgtSum_Ref0_RefNear >= minNobjInVol * wgt_Ref_Inp) { foundDist = true; break; }
          }
          if(foundDist) break;

          minNobjInVolPrev = minNobjInVolNow; minNobjInVolNow *= 5;
        }

        if(foundDist) {
          // -----------------------------------------------------------------------------------------------------------
          // finally, compute the weight as the relative difference beween the distance between the diatance
          // measures then compute a binary decision, based on the minimal threshold set by maxRelRatioInRef
          // -----------------------------------------------------------------------------------------------------------
          weightKNN = max( ((dist_Ref0_RefNear - dist_Ref_Inp) / dist_Ref0_RefNear) , 0.);
          weightKNN = (weightKNN > maxRelRatioInRef) ? 1 : 0;

          var_0->IncCntr("Found good weight");
          if(weightKNN > maxRelRatioInRef) var_0->IncCntr(wgtKNNname+" = 1"); else var_0->IncCntr(wgtKNNname+" = 0");
        }
        else {
          // assign zero weight if could not complete the calculation
          weightKNN = 0;

          var_0->IncCntr("Did not find good weight");
        }
      }
    }

    var_1->SetVarF(wgtKNNname,weightKNN);

    outTree->Fill();

    mayWriteObjects = true; var_0->IncCntr("nObj"); if(var_0->GetCntr("nObj") == maxNobj) breakLoop = true;
  }
  if(!breakLoop) { var_0->printCntr(outTreeName); outputs->WriteOutObjects(false,true); outputs->ResetObjects(); }
  
  DELNULL(var_0); DELNULL(var_1); DELNULL(outTree); outputs->TreeMap.erase(outTreeName);

  TChain * aChainInpWgt(NULL);

  // -----------------------------------------------------------------------------------------------------------
  // normalize the derived weights - the normalization factor is such that the entire input sample will have
  // a sum of weights which is equal to the original number of entries in the sample
  // -----------------------------------------------------------------------------------------------------------
  if(doRelWgts) {
    VERIFY(LOCATION,(TString)"Got sum of weights from the entire main sample = "+utils->floatToStr(weightSum)
                            +" ... Something is horribly wrong ?!?! ",(weightSum > 0));

    double weightNorm = chainEntV[0] / weightSum;

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
    var_1->copyVarStruct(var_0);

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

      var_1->copyVarData(var_0);

      double weightKNN = var_0->GetVarF(wgtKNNname) * weightNorm;
      var_1->SetVarF(wgtKNNname,weightKNN);

      outTree->Fill();

      mayWriteObjects = true; var_0->IncCntr("nObj"); if(var_0->GetCntr("nObj") == maxNobj) breakLoop = true;
    }
    if(!breakLoop) { var_0->printCntr(outTreeName); outputs->WriteOutObjects(false,true); outputs->ResetObjects(); }

    DELNULL(var_1); DELNULL(outTree); outputs->TreeMap.erase(outTreeName);

    // remove the temporary sub-dir and the intermediary trees inside
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
  if(doStoreToAscii && doRelWgts) {
    // extract the names of the variables which will be written out to the ascii output
    // including the derived KNN weights. Add the actuall weight variable if not already included
    vector <TString> outVarNames = utils->splitStringByChar(outAsciiVars,';');
    if(find(outVarNames.begin(),outVarNames.end(), wgtKNNname) == outVarNames.end()) outVarNames.push_back(wgtKNNname);

    // create a VarMaps, connect it to the tree, and write out the requested variables
    VarMaps * var_2 = new VarMaps(glob,utils,"treeRegClsVar_2");
    var_2->connectTreeBranches(aChainOut);
    var_2->storeTreeToAscii((TString)wgtKNNname+aChainInp->GetName(),"",0,nObjectsToWrite,"",&outVarNames,NULL);

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
      if(branchName.BeginsWith(glob->GetOptC("baseName_ANNZ")))               continue;
      if(find(varNames.begin(),varNames.end(), branchName) != varNames.end()) continue;

      // only accept branches common to all chains
      int hasBranch(0);
      for(int nChainNow=0; nChainNow<3; nChainNow++) {
        TChain  * aChain(NULL);
        if     (nChainNow == 0) { aChain = aChainRef; }
        else if(nChainNow == 1) { aChain = aChainInp; }
        else if(nChainNow == 2) { aChain = aChainOut; }

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

          TString weightNow(""), hisTitle("");
          TChain  * aChain(NULL);
          if     (nChainNow == 0) { aChain = aChainRef; hisTitle = "Reference"; weightNow = "1";        }
          else if(nChainNow == 1) { aChain = aChainInp; hisTitle = "Original";  weightNow = "1";        }
          else if(nChainNow == 2) { aChain = aChainOut; hisTitle = "Weighted";  weightNow = wgtKNNname; }

          if(nChainNow == 0) {
            if(chainWgtV[1] != "") weightNow = (TString)"("+weightNow+")*("+chainWgtV[1]+")";
            if(chainCutV[1] != "") weightNow = (TString)"("+weightNow+")*("+chainCutV[1]+")";
          }

          if(varNameNow == wgtKNNname) {
            if(nChainNow != 2) continue;
            else               weightNow = "1";
          }

          TString hisName   = (TString)baseName+aChainInp->GetName()+nVarName+nChainKNNname;
          TString drawExprs = (TString)varNameNow+">>"+hisName;
          if(nDrawNow == 1) drawExprs += TString::Format("(%d,%f,%f)",nDrawBins,drawLim0,drawLim1);

          TCanvas * tmpCnvs = new TCanvas("tmpCnvs","tmpCnvs");
          aChain->Draw(drawExprs,weightNow); DELNULL(tmpCnvs);

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
    DELNULL(knnErrFactory[nChainNow]); DELNULL(knnErrOutFile[nChainNow]);
    // after closing a TFile, need to return to the correct directory, or else histogram pointers will be affected
    outputs->BaseDir->cd();

    utils->safeRM(outFileDirKnnErrV[nChainNow],inLOG(Log::DEBUG));
    utils->safeRM(outFileNameKnnErr[nChainNow],inLOG(Log::DEBUG));
  }

  knnErrOutFile.clear(); knnErrFactory.clear(); knnErrMethod.clear(); knnErrModule.clear(); aChainV.clear();
  outFileDirKnnErrV.clear(); minMaxVarVals.clear(); outFileNameKnnErr.clear(); varNames.clear(); chainWgtV.clear(); chainCutV.clear();
  varFormNames.clear(); objNowV.clear(); distV.clear(); weightSumV.clear(); distIndexV.clear();
  chainWgtNormV.clear(); chainEntV.clear();

  return;
}














