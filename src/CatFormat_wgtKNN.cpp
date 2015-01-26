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
 * @brief               - Generate weights based on the KNN method
 *                      (see: Cunha et al. (2008), http://arxiv.org/abs/0810.2991v4)
 * 
 * @details
 *                      - Weights are calculated as the ratio  r_ref/R_same = (N_ref/V_nn) / (N_same/V_nn) = N_ref/N_same.
 *                      Given the "reference-position" of a given object from the 
 *                      main dataset (aChainInp), N_ref,N_same are the number of near neighbours from the reference-position
 *                      within a given volume in the parameter space, V_nn. The difference between the two parameters, is that
 *                      N_ref is extracted from the reference dataset (aChainRef) and N_same is extracted from the main dataset (aChainInp).
 *                      - The kd-tree is used to search for a fixed number of near neighbours, it is not constrained by
 *                      the distance parameter. We therefore first search for minNobjInVol_wgtKNN near neighbours from
 *                      each chain. The chain for which the distance to the minNobjInVol_wgtKNN neighbour is shortest
 *                      is then selected for another search, with a higher number of near neighbours.
 *                      - It is possible for the new search to still be constrained to be within a distance which
 *                      is shorter than that of the minNobjInVol_wgtKNN
 *                      neighbour from the other chain. In such a case, this step is repeated again and again.
 *                      In each iteration we look for a larger number
 *                      of near neighbours around the original reference-position. We end the search when we recover some number of near neighbours
 *                      for which the distance from the reference-position for both aChainInp and aChainRef is the same.
 *                      - For each object the weight calculation therefore uses a different volume in the parameter-space. However,
 *                      the volume element for a given object (V_nn) cancels out, since the weights are defined as a ratio. The condition
 *                      on a minimal number of neighbors, minNobjInVol_wgtKNN, prevents shoot noise from dominating the result.
 * 
 * @param aChainInp     - a chain corresponding to the main dataset
 * @param aChainRef     - a chain corresponding to the reference dataset
 * @param outTreeName   - optional tree name (or else the name is extracted from aChainInp)
 */
// ===========================================================================================================
void CatFormat::addWgtKNNtoTree(TChain * aChainInp, TChain * aChainRef, TString outTreeName) {
// ===========================================================================================
  VERIFY(LOCATION,(TString)"Memory leak ?! ",(dynamic_cast<TChain*>(aChainInp)));
  VERIFY(LOCATION,(TString)"Memory leak ?! ",(dynamic_cast<TChain*>(aChainRef)));

  if(outTreeName == "") outTreeName = aChainInp->GetName();

  aLOG(Log::INFO) <<coutWhiteOnBlack<<coutBlue<<" - starting ANNZ::addWgtKNNtoTree("<<coutYellow<<outTreeName<<coutBlue<<") ... "<<coutDef<<endl;

  TString outDirNameFull  = glob->GetOptC("outDirNameFull");
  TString weightName      = glob->GetOptC("baseName_weightKNN");
  TString outBaseName     = (TString)outDirNameFull+glob->GetOptC("treeName")+weightName;
  TString outAsciiVars    = glob->GetOptC("outAsciiVars_wgtKNN");
  TString weightVarNames  = glob->GetOptC("weightVarNames_wgtKNN");
  TString plotExt         = glob->GetOptC("printPlotExtension");
  bool    doNormWeights   = glob->GetOptB("doNormWeights_wgtKNN");
  bool    doStoreToAscii  = glob->GetOptB("doStoreToAscii");
  bool    doPlots         = glob->GetOptB("doPlots");
  int     nObjectsToWrite = glob->GetOptI("nObjectsToWrite");
  int     minNobjInVol    = glob->GetOptI("minNobjInVol_wgtKNN");
  int     maxNobj         = 0;   maxNobj = glob->GetOptI("maxNobj"); // only allow limits in case of debugging !! 
  
  TString wgtNormTreeName = "_normWgt";

  vector <TString> chainWgtV(2,""); chainWgtV[1] = glob->GetOptC("refWeight_wgtKNN");
  vector <TString> chainCutV(2,""); chainCutV[1] = glob->GetOptC("refCut_wgtKNN");
  
  vector <TString> varNames = utils->splitStringByChar(weightVarNames,';');

  int nVars = (int)varNames.size();
  VERIFY(LOCATION,(TString)"Did not find input variables for KNN weight computation [\"weightVarNames_wgtKNN\" ="
                          +weightVarNames+"] ... Something is horribly wrong !?!?",(nVars > 0));

  // -----------------------------------------------------------------------------------------------------------
  // setup the kd-trees for the two chains
  // -----------------------------------------------------------------------------------------------------------
  vector <TChain          *> aChainV(2);
  vector <TFile           *> knnErrOutFile(2); vector <TMVA::Factory        *> knnErrFactory(2);
  vector <TMVA::MethodKNN *> knnErrMethod(2);  vector <TMVA::kNN::ModulekNN *> knnErrModule(2);

  vector <TString> outFileDirKnnErrV(2), outFileNameKnnErr(2);

  for(int nChainNow=0; nChainNow<2; nChainNow++) {
    TString nChainKNNname = TString::Format("_nChainKNN%d",nChainNow);

    // clone the chain used for the kd-tree (needed, as the kd-tree will turn off branches which
    // are not used, but these are needed for var_0, var_1) - The Clone should not be deleted
    // as this may cause seg-fault (perhaps due to closing of underlying TFile somwhere by ROOT)
    // -----------------------------------------------------------------------------------------------------------
    TChain * aChain = (nChainNow == 0) ? aChainInp : aChainRef;
    aChainV[nChainNow] = (TChain*)aChain->Clone((TString)aChain->GetName()+nChainKNNname);  aChainV[nChainNow]->SetDirectory(0);

    outFileDirKnnErrV[nChainNow] = outBaseName+"_weights"+nChainKNNname+"/";
    outFileNameKnnErr[nChainNow] = outBaseName+nChainKNNname+".root";

    (TMVA::gConfig().GetIONames()).fWeightFileDir = outFileDirKnnErrV[nChainNow];

    bool    debug           = inLOG(Log::DEBUG_2);
    TString verbLvlF        = (TString)(debug ? ":V:!Silent"             : ":!V:Silent");
    TString verbLvlM        = (TString)(debug ? ":V:H"                   : ":!V:!H");
    TString drawProgBarStr  = (TString)(debug ? ":Color:DrawProgressBar" : ":Color:!DrawProgressBar");
    TString optKNN          = (TString)":nkNN=0:ScaleFrac=0.0";
    TString transStr        = (TString)":Transformations=I";
    TString analysType      = (TString)":AnalysisType=Regression";
    TString trainValidStr   = (TString)"nTrain_Regression=0:nTest_Regression=0:SplitMode=Random:NormMode=NumEvents:!V";

    // setup the factory
    // -----------------------------------------------------------------------------------------------------------
    knnErrOutFile[nChainNow] = new TFile(outFileNameKnnErr[nChainNow],"RECREATE");
    knnErrFactory[nChainNow] = new TMVA::Factory(weightName, knnErrOutFile[nChainNow], (TString)verbLvlF+drawProgBarStr+transStr+analysType);    

    // define all input variables as floats in the factory
    for(int nVarNow=0; nVarNow<nVars; nVarNow++) knnErrFactory[nChainNow]->AddVariable(varNames[nVarNow],"","",'F');

    knnErrFactory[nChainNow]->AddRegressionTree(aChainV[nChainNow], 1, TMVA::Types::kTraining);
    knnErrFactory[nChainNow]->SetWeightExpression(chainWgtV[nChainNow],"Regression");

    knnErrFactory[nChainNow]->PrepareTrainingAndTestTree((TCut)chainCutV[nChainNow],trainValidStr);

    knnErrMethod[nChainNow] = dynamic_cast<TMVA::MethodKNN*>(knnErrFactory[nChainNow]->BookMethod(TMVA::Types::kKNN, weightName,(TString)optKNN+verbLvlM));
    knnErrModule[nChainNow] = knnErrMethod[nChainNow]->fModule;

    // fill the module with events made from the tree entries and create the binary tree
    // -----------------------------------------------------------------------------------------------------------
    knnErrMethod[nChainNow]->Train();

    aLOG(Log::INFO)  <<coutGreen<<" - "<<coutBlue<<aChainV[nChainNow]->GetName()<<coutGreen<<" - kd-tree ("<<knnErrMethod[nChainNow]->fEvent.size()
                     <<")"<<coutYellow<<" , opts = "<<coutRed<<optKNN<<coutYellow<<" , "<<coutRed<<trainValidStr
                     <<coutYellow<<" , cuts = "<<coutRed<<chainCutV[nChainNow]<<coutYellow<<" , weights = "<<coutRed<<chainWgtV[nChainNow]
                     <<coutDef<<endl;

    // sanity check - if this is not true, the events in the binary tree will have the wrong "units"
    VERIFY(LOCATION,(TString)"Somehow the fScaleFrac for the kd-tree is not zero ... Something is horribly wrong ?!?!?"
                             ,(knnErrMethod[nChainNow]->fScaleFrac < EPS));

    outputs->BaseDir->cd();
  }


  // -----------------------------------------------------------------------------------------------------------
  // create the vars to read/write trees
  // -----------------------------------------------------------------------------------------------------------
  VarMaps * var_0 = new VarMaps(glob,utils,"treeWeightsKNNvar_0");
  VarMaps * var_1 = new VarMaps(glob,utils,"treeWeightsKNNvar_1");
  
  // add unique formula names (to avoid conflict with existing variable names)
  vector <TString> varFormNames(nVars);
  for(int nVarNow=0; nVarNow<nVars; nVarNow++) {
    varFormNames[nVarNow] = (TString)weightName+"_"+varNames[nVarNow];

    var_0->NewForm(varFormNames[nVarNow],varNames[nVarNow]);
  }

  var_0->connectTreeBranches(aChainInp);

  var_1->copyVarStruct(var_0);

  TTree * outTree = new TTree(outTreeName,outTreeName); outTree->SetDirectory(0); outputs->TreeMap[outTreeName] = outTree;
  var_1->createTreeBranches(outTree); 

  
  // -----------------------------------------------------------------------------------------------------------
  // loop on the tree
  // -----------------------------------------------------------------------------------------------------------
  double          weightSum(0);
  vector <int>    distIndexV(2);
  vector <double> distV(2), weightSumV(2);
  TMVA::kNN::VarVec     objNowV(nVars,0);

  bool  breakLoop(false), mayWriteObjects(false);
  int   nObjectsToPrint(aChainInp->GetEntries()/10.);
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

    // fill the current object vector and use it to create a TMVA::kNN::Event object
    for(int nVarNow=0; nVarNow<nVars; nVarNow++) {
      objNowV[nVarNow] = var_0->GetForm(varFormNames[nVarNow]);
    }
    const TMVA::kNN::Event evtNow(objNowV,1,0);

    // find the same number of near neighbours for each chain, and derive the distance this requires
    int nObjKNN(minNobjInVol);
    for(int nChainNow=0; nChainNow<2; nChainNow++) {
      knnErrModule[nChainNow]->Find(evtNow,nObjKNN);
      const TMVA::kNN::List & knnList = knnErrModule[nChainNow]->GetkNNList();

      weightSumV[nChainNow] = 0;
      for(TMVA::kNN::List::const_iterator lit=knnList.begin(); lit!=knnList.end(); ++lit) {
        weightSumV[nChainNow] += max((lit->first->GetEvent()).GetWeight(),0.); // just in case, prevent negative weights
      }
      distV[nChainNow] = evtNow.GetDist(knnList.back().first->GetEvent());
    }
    // the index of the chain with the shorter distance
    if(distV[0] < distV[1]) { distIndexV[0] = 0; distIndexV[1] = 1; }
    else                    { distIndexV[0] = 1; distIndexV[1] = 0; }

    bool   foundDist(false);
    double weightKNN(0), weightSumNow(0);
    int    nObjsIn(0), preNobj(0);
    for(int nSearchNow=0; nSearchNow<100; nSearchNow++) {
      preNobj = nObjsIn = nObjKNN; nObjKNN *= 5;

      knnErrModule[distIndexV[0]]->Find(evtNow,nObjKNN);
      const TMVA::kNN::List & knnList = knnErrModule[distIndexV[0]]->GetkNNList();

      for(TMVA::kNN::List::const_iterator lit=std::next(knnList.begin(),preNobj); lit!=knnList.end(); ++lit) {
        const TMVA::kNN::Event & event_ = lit->first->GetEvent();

        double knnDistNow = evtNow.GetDist(event_);
        double weightObj  = max(event_.GetWeight(),0.); // just in case, prevent negative weights

        // cout <<"--- "<<weightObj <<CT<<event_.GetWeight()<<endl;
        // if(knnDistNow < distV[distIndexV[1]]) cout <<loopEntry<<CT<<nSearchNow<<CT<< std::distance(knnList.begin(), lit)
        //                                            <<CT<<nObjsIn<<CT<<distIndexV[0]<<CT<<distV[0]<<CT<<distV[1]<<CT<<knnDistNow<<endl;
        if(knnDistNow < distV[distIndexV[1]]) { weightSumNow += weightObj; nObjsIn++; }
        else                                  { foundDist     = true;      break;     }
      }
      if(foundDist) {
        weightSumV[distIndexV[0]] += weightSumNow;
        if(weightSumV[1] * weightSumV[0] > EPS) weightKNN = weightSumV[1]/weightSumV[0];

        // if(distV[0] < distV[1]) {
        //   if(fabs(weightKNN - minNobjInVol/double(nObjsIn)) > EPS) { 
        //     cout <<"-1- "<< weightKNN <<CT<< minNobjInVol/double(nObjsIn) <<endl; assert(0);
        //   }
        // }
        // else {
        //   if(fabs(weightKNN - nObjsIn/double(minNobjInVol)) > EPS) {
        //     cout <<"-2- "<< weightKNN <<CT<< nObjsIn/double(minNobjInVol)<<endl; assert(0);
        //   }
        // }

        // cout <<"aaaaaaaaa "<<loopEntry<<CT<<nSearchNow<<CT<<nObjsIn<<CT<<weightKNN<<endl;
        break;
      }
    }
    if(foundDist) { weightSum += weightKNN;
                    var_0->IncCntr("Found good weight");        }
    else          { var_0->IncCntr("Did not fine good weight"); }

    var_1->SetVarF(weightName,weightKNN); // the weightName variable was created by CatFormat if all went well

    outTree->Fill();

    mayWriteObjects = true; var_0->IncCntr("nObj"); if(var_0->GetCntr("nObj") == maxNobj) breakLoop = true;
  }
  if(!breakLoop) { var_0->printCntr(outTreeName); outputs->WriteOutObjects(false,true); outputs->ResetObjects(); }
  


  if(doNormWeights && (weightSum > 0)) {
    // double weightNorm = var_0->GetCntr("nObj") / weightSum;
    double weightNorm = 1 / weightSum;

    // create a temporary sub-dir for the output trees of the above
    // -----------------------------------------------------------------------------------------------------------
    TString outDirNameTMP = (TString)outDirNameFull+"tmpDir"+wgtNormTreeName+"/";
    TString mkdirCmnd     = (TString)"mkdir -p "+outDirNameTMP;
    utils->exeShellCmndOutput(mkdirCmnd,inLOG(Log::DEBUG),true);

    TString fileNameNow = (TString)outDirNameFull+outTreeName+"*.root";
    TString mvCmnd      = (TString)"mv "+fileNameNow+" "+outDirNameTMP;

    utils->exeShellCmndOutput(mvCmnd,inLOG(Log::DEBUG),true);
    fileNameNow.ReplaceAll(outDirNameFull,outDirNameTMP);

    TChain  * aChainNormWgt = new TChain(outTreeName,outTreeName); aChainNormWgt->SetDirectory(0); aChainNormWgt->Add(fileNameNow);

    // create the vars to read/write trees
    // -----------------------------------------------------------------------------------------------------------
    DELNULL(var_0); DELNULL(var_1);
    DELNULL(outTree); outputs->TreeMap.erase(outTreeName);

    var_0 = new VarMaps(glob,utils,"treeWeightsKNNvar_0");
    var_1 = new VarMaps(glob,utils,"treeWeightsKNNvar_1");

    var_0->connectTreeBranches(aChainNormWgt);
    var_1->copyVarStruct(var_0);

    TTree * outTree = new TTree(outTreeName,outTreeName); outTree->SetDirectory(0); outputs->TreeMap[outTreeName] = outTree;
    var_1->createTreeBranches(outTree); 

    // -----------------------------------------------------------------------------------------------------------
    // loop on the tree
    // -----------------------------------------------------------------------------------------------------------
    bool  breakLoop(false), mayWriteObjects(false);
    var_0->clearCntr();
    for(Long64_t loopEntry=0; true; loopEntry++) {
      if(!var_0->getTreeEntry(loopEntry)) breakLoop = true;

      if((mayWriteObjects && var_0->GetCntr("nObj") % nObjectsToWrite == 0) || breakLoop) {
        var_0->printCntr(outTreeName); outputs->WriteOutObjects(false,true); outputs->ResetObjects();
        mayWriteObjects = false;
      }
      if(breakLoop) break;

      var_1->copyVarData(var_0);

      double weightKNN = var_0->GetVarF(weightName) * weightNorm;
      var_1->SetVarF(weightName,weightKNN);

      outTree->Fill();

      mayWriteObjects = true; var_0->IncCntr("nObj"); if(var_0->GetCntr("nObj") == maxNobj) breakLoop = true;
    }
    if(!breakLoop) { var_0->printCntr(outTreeName); outputs->WriteOutObjects(false,true); outputs->ResetObjects(); }

    // remove the temporary sub-dir and the intermediary trees inside
    utils->safeRM(outDirNameTMP,inLOG(Log::DEBUG));
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
    if(find(outVarNames.begin(),outVarNames.end(), weightName) == outVarNames.end()) outVarNames.push_back(weightName);

    // create a VarMaps, connect it to the tree, and write out the requested variables
    VarMaps * var_2 = new VarMaps(glob,utils,"treeRegClsVar_2");
    var_2->connectTreeBranches(aChainOut);
    var_2->storeTreeToAscii((TString)weightName+aChainInp->GetName(),"",0,nObjectsToWrite,"",&outVarNames,NULL);

    DELNULL(var_2); outVarNames.clear();
  }


  // -----------------------------------------------------------------------------------------------------------
  // some histograms to asses the weights
  // -----------------------------------------------------------------------------------------------------------
  if(doPlots) {
    vector <TString> branchNameV_0, branchNameV;

    var_0->GetAllVarNames(branchNameV_0);
    int nBranches = (int)branchNameV_0.size();

    branchNameV.push_back(weightName);
    for(int nBranchNow=0; nBranchNow<nBranches; nBranchNow++) {
      TString branchName = branchNameV_0[nBranchNow];
      TString branchType = var_0->GetVarType(branchName);

      if(branchType == "C" || branchType == "FM")                             continue;
      if(branchName.BeginsWith(glob->GetOptC("baseName_ANNZ")))               continue;
      if(find(varNames.begin(),varNames.end(), branchName) != varNames.end()) continue;

      branchNameV.push_back(branchName);
    }
    nBranches = (int)branchNameV.size();

    for(int nVarNow=0; nVarNow<nVars+nBranches; nVarNow++) {
      TString nVarName = TString::Format("_nVar%d",nVarNow);
      
      TString varNameNow(""), baseName("");
      if(nVarNow < nVars) { baseName = "varWeightKNN_"; varNameNow = varNames[nVarNow];          }
      else                { baseName = "varFullChain_"; varNameNow = branchNameV[nVarNow-nVars]; }

      vector <TH1*> his1V;
      for(int nChainNow=0; nChainNow<3; nChainNow++) {
        TString nChainKNNname = TString::Format("_nChain%d",nChainNow);

        TString weightNow(""), hisTitle("");
        TChain  * aChain(NULL);
        if     (nChainNow == 0) { aChain = aChainRef; hisTitle = "Reference"; weightNow = "1";        }
        else if(nChainNow == 1) { aChain = aChainInp; hisTitle = "Original";  weightNow = "1";        }
        else if(nChainNow == 2) { aChain = aChainOut; hisTitle = "Weighted";  weightNow = weightName; }

        if(nChainNow == 0) {
          if(chainWgtV[1] != "") weightNow = (TString)"("+weightNow+")*("+chainWgtV[1]+")";
          if(chainCutV[1] != "") weightNow = (TString)"("+weightNow+")*("+chainCutV[1]+")";
        }

        if(varNameNow == weightName) {
          if(nChainNow != 2) continue;
          else               weightNow = "1";
        }

        TString hisName   = (TString)baseName+aChainInp->GetName()+nVarName+nChainKNNname;
        TString drawExprs = (TString)varNameNow+">>"+hisName;
        // drawExprs = TString::Format(varNameNow+">>"+hisName+"(%d,%f,%f)",glob->GetOptI("closHisN"),glob->GetOptF("minValZ"),glob->GetOptF("maxValZ"));

        TCanvas * tmpCnvs = new TCanvas("tmpCnvs","tmpCnvs");
        aChain->Draw(drawExprs,weightNow); DELNULL(tmpCnvs);

        TH1 * his1 = (TH1F*)gDirectory->Get(hisName); his1->SetDirectory(0); his1->SetTitle(hisTitle); assert(dynamic_cast<TH1F*>(his1));
        his1V.push_back(his1);
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

    // // move the plots to a new sub-dir (create it if needed)
    // TString plotDirName   = (TString)outDirNameFull+"plots/";
    // TString mkdirCmnd     = (TString)"mkdir -p "+plotDirName;
    // utils->exeShellCmndOutput(mkdirCmnd,inLOG(Log::DEBUG),true);

    // bool    hasPlotExt    = (plotExt != "" && plotExt != "NULL");
    // TString mvCmnd        = (TString)"mv "+outDirNameFull+"*.C "+(hasPlotExt ? (TString)outDirNameFull+"*."+plotExt : "")+" "+plotDirName;
    // utils->exeShellCmndOutput(mvCmnd,inLOG(Log::DEBUG),true);

    branchNameV.clear(); branchNameV_0.clear();
  }


  // cleanup
  DELNULL(var_0); DELNULL(var_1);
  DELNULL(outTree); outputs->TreeMap.erase(outTreeName);

  for(int nChainNow=0; nChainNow<2; nChainNow++) {
    DELNULL(knnErrFactory[nChainNow]); DELNULL(knnErrOutFile[nChainNow]);
    // after closing a TFile, need to return to the correct directory, or else histogram pointers will be affected
    outputs->BaseDir->cd();

    utils->safeRM(outFileDirKnnErrV[nChainNow],inLOG(Log::DEBUG));
    utils->safeRM(outFileNameKnnErr[nChainNow],inLOG(Log::DEBUG));
  }

  knnErrOutFile.clear(); knnErrFactory.clear(); knnErrMethod.clear(); knnErrModule.clear(); aChainV.clear();
  outFileDirKnnErrV.clear(); outFileNameKnnErr.clear(); varNames.clear(); chainWgtV.clear(); chainCutV.clear();
  varFormNames.clear(); objNowV.clear(); distV.clear(); weightSumV.clear(); distIndexV.clear();

  return;
}














