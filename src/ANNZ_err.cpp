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
 * @brief          - Create trees from the _train dataset which contain the input for
 *                 the KNN error estimator
 *              
 * @param nMLMnow  - The index of the primary MLM.
 */
// ===========================================================================================================
void ANNZ::createTreeErrKNN(int nMLMnow) {
// ===========================================================================================================
  aLOG(Log::DEBUG) <<coutWhiteOnBlack<<coutGreen<<" - starting ANNZ::createTreeErrKNN() - "
                   <<"will create errKNN trees for "<<coutPurple<<getTagName(nMLMnow)<<coutGreen<<" ... "<<coutDef<<endl;

  TString zTrgName       = glob->GetOptC("zTrg");
  TString indexName      = glob->GetOptC("indexName");
  TString sigBckTypeName = glob->GetOptC("sigBckTypeName");
  bool    isCls          = glob->GetOptB("doClassification") || glob->GetOptB("doBinnedCls");
  TString MLMname        = getTagName(nMLMnow);
  TString errKNNname     = getErrKNNname(nMLMnow);
  TString MLMname_i      = getTagIndex(nMLMnow);
 
  VarMaps * var_0 = new VarMaps(glob,utils,"treeErrKNN_0");
  VarMaps * var_1 = new VarMaps(glob,utils,"treeErrKNN_1");

  TString inTreeName = (TString)glob->GetOptC("treeName")+"_train";
  TString inFileName = (TString)glob->GetOptC("inputTreeDirName")+inTreeName+"*.root";

  // prepare the chain and input variables
  TChain * aChain = new TChain(inTreeName,inTreeName); aChain->SetDirectory(0); aChain->Add(inFileName); 
  aLOG(Log::DEBUG) <<coutRed<<" - added chain "<<coutGreen<<inTreeName<<"("<<aChain->GetEntries()<<")"
                   <<" from "<<coutBlue<<inFileName<<coutDef<<endl;

  var_0->connectTreeBranchesForm(aChain,&readerInptV);

  // setup cuts for the vars we loop on for sig/bck determination in case of classification
  if(isCls) {
    // override signal/background cuts, just in case...
    TCut sigCuts = (userCutsM["_sig"] != "") ? userCutsM["_sig"] : userCutsM[MLMname+"_sig"];
    TCut bckCuts = (userCutsM["_bck"] != "") ? userCutsM["_bck"] : userCutsM[MLMname+"_bck"];

    VERIFY(LOCATION,(TString)"Could not determine the signal-cut ... Something is horribly wrong !!!",(sigCuts != ""));
    VERIFY(LOCATION,(TString)"Could not determine the signal-cut ... Something is horribly wrong !!!",(bckCuts != ""));

    var_0->setTreeCuts("_sig",sigCuts);
    var_0->setTreeCuts("_bck",bckCuts);
  }

  var_1->NewVarF(MLMname); var_1->NewVarF(errKNNname); var_1->NewVarI(MLMname_i);
  if(isCls) var_1->NewVarI(sigBckTypeName); else var_1->NewVarF(zTrgName);
  
  TString outTreeName = getKeyWord("","treeErrKNN","treeErrKNNname");
  TTree   * outTree   = new TTree(outTreeName,outTreeName);
  outTree->SetDirectory(0); outputs->TreeMap[outTreeName] = outTree;

  var_1->createTreeBranches(outTree); 
  var_1->setDefaultVals();
  
  vector <bool> errFakeShift(2,true);

  // -----------------------------------------------------------------------------------------------------------
  // 
  // -----------------------------------------------------------------------------------------------------------
  bool  breakLoop(false), mayWriteObjects(false);
  int   nObjectsToWrite(glob->GetOptI("nObjectsToWrite")), nObjectsToPrint(glob->GetOptI("nObjectsToPrint"));
  var_0->clearCntr();
  for(Long64_t loopEntry=0; true; loopEntry++) {
    if(!var_0->getTreeEntry(loopEntry)) breakLoop = true;

    if((var_0->GetCntr("nObj") % nObjectsToPrint == 0 && var_0->GetCntr("nObj") > 0) || breakLoop) { var_0->printCntr(inTreeName,Log::DEBUG); }
    if((mayWriteObjects && var_0->GetCntr("nObj") % nObjectsToWrite == 0) || breakLoop) {
      outputs->WriteOutObjects(false,true); outputs->ResetObjects(); mayWriteObjects = false;
    }
    if(breakLoop) break;

    var_1->SetVarI(MLMname_i,var_0->GetVarI(indexName));

    // compute the KNN error for this object
    if(isCls) {
      double clsPrb = getReader(var_0,ANNZ_readType::PRB,true,nMLMnow);

      // expect for background that clsPrb=0, and for signal that clsPrb=1
      int    sigBckType(-1);
      double errKNN(-1);
      if     (!var_0->hasFailedTreeCuts("_bck")) { sigBckType = 0; errKNN =   clsPrb; }
      else if(!var_0->hasFailedTreeCuts("_sig")) { sigBckType = 1; errKNN = 1-clsPrb; }

      // TMVA forces a check that a target variable is not constant. In order to avoid this,
      // change the value of one signal and one background object by some insignificant amount
      if(errFakeShift[sigBckType]) {
        errFakeShift[sigBckType] = false;
        errKNN += (sigBckType == 0 ? 1 : -1) * 0.000001;
      }

      var_1->SetVarF(MLMname,        clsPrb);
      var_1->SetVarF(errKNNname,     errKNN);
      var_1->SetVarI(sigBckTypeName, sigBckType);
    }
    else {
      double zTrg   = var_0->GetVarF(zTrgName);
      double regVal = getReader(var_0,ANNZ_readType::REG,true,nMLMnow);
      
      // the error of each object wrt its own true value
      // see: http://arxiv.org/abs/0810.2991 - Estimating the Redshift Distribution of Photometric Galaxy... - Sect. 4.2
      double errKNN = regVal - zTrg;

      if(errFakeShift[0]) {
        errFakeShift[0]  = false;
        errKNN      *= 1.000001;
      }
      
      var_1->SetVarF(MLMname,   regVal);
      var_1->SetVarF(errKNNname,errKNN);
      var_1->SetVarF(zTrgName,  zTrg);
    }

    var_1->fillTree();

    var_0->IncCntr("nObj"); mayWriteObjects = true;
  }
  if(!breakLoop) { var_0->printCntr(inTreeName,Log::DEBUG); outputs->WriteOutObjects(false,true); outputs->ResetObjects(); }

  DELNULL(var_0);  DELNULL(var_1);
  DELNULL(aChain); DELNULL(outTree); outputs->TreeMap.erase(outTreeName);
  
  errFakeShift.clear();

  aLOG(Log::DEBUG) <<coutGreen<<" - finished ANNZ::createTreeErrKNN()"<<coutDef<<endl;

  return;
}

// ===========================================================================================================
/**
 * @brief                - Setup a TMVA::Factory which will be used for KNN-error estimation.
 * 
 * @details              - The TMVA::Factory is linked to the training dataset, and is initiated with the same
 *                       input-variables as the "primary MLM", for which the error are calculated.
 * 
 * @param aChainKnn      - A chain linked to the training dataset.
 * @param knnErrOutFile  - A TFile which is created as part of the setup of the TMVA::Factory (needs to be deleted suring cleanup).
 * @param knnErrFactory  - A pointer to the TMVA::Factory which is created here.
 * @param knnErrDataLdr  - A pointer to the TMVA::DataLoader, needed for ROOT versions > 6.8.
 * @param knnErrModule   - A pointer to the TMVA::kNN::ModulekNN which is created by the TMVA::Factory, and is later
 *                       used to get the near-neighbours in getRegClsErrKNN().
 * @param trgIndexV      - container to keep track of how MLM indices are arranged in the KNN target list
 * @param nMLMnow        - The index of the primary MLM.
 * @param cutsAll        - Cuts used on the dataset, which should match the cuts on the primary MLM.
 * @param wgtAll         - Weights for the entire dataset.
 */
// ===========================================================================================================
void ANNZ::setupKdTreeKNN(
  TChain * aChainKnn, TFile *& knnErrOutFile, TMVA::Factory *& knnErrFactory,
  TMVA::Configurable *& knnErrDataLdr, TMVA::kNN::ModulekNN *& knnErrModule,
  vector <int> & trgIndexV, int nMLMnow, TCut cutsAll, TString wgtAll
) {
// ===========================================================================================================
  bool debug = inLOG(Log::DEBUG_2) || glob->OptOrNullB("debugErrANNZ");
  if(debug) aCustomLOG("") <<coutWhiteOnBlack<<coutBlue<<" - starting ANNZ::setupKdTreeKNN() ... "<<coutDef<<endl;

  int     nMLMs           = glob->GetOptI("nMLMs");
  TString MLMname         = getTagName(nMLMnow);
  TString baseName_knnErr = glob->GetOptC("baseName_knnErr");
  TString errKNNname      = getErrKNNname(nMLMnow);
  TString baseTag_errKNN  = glob->GetOptC("baseTag_errKNN");
  int     nErrKNN         = glob->GetOptI("nErrKNN");
  bool    doWidthRescale  = glob->GetOptB((TString)"doWidthRescale_errKNN");
  TString indexName       = glob->GetOptC("indexName");
  int     minObjTrainTest = glob->GetOptI("minObjTrainTest");
  double  sampleFrac      = glob->GetOptF("sampleFrac_errKNN");

  TString verbLvlF        = (TString)(debug ? ":V:!Silent" : ":!V:Silent");
  TString verbLvlM        = (TString)(debug ? ":V:H"       : ":!V:!H");
  TString drawProgBarStr  = (TString)(debug ? ":Color:DrawProgressBar" : ":Color:!DrawProgressBar");

  TString optKNN          = TString::Format(":nkNN=%d:ScaleFrac=0.0",nErrKNN);
  TString transStr        = (TString)":Transformations=I";
  TString analysType      = (TString)":AnalysisType=Regression";
  TString typeANNZ        = (TString)glob->GetOptC("typeANNZ")+baseName_knnErr;
  TString trainValidStr   = (TString)"nTrain_Regression=0:nTest_Regression=0:SplitMode=Random:NormMode=NumEvents:!V";

  // bool  isReg = (glob->GetOptB("doRegression") && !glob->GetOptB("doBinnedCls"));
  // if(!isReg) { analysType = (TString)":AnalysisType=Classification"; trainValidStr = ""; }
  
  (TMVA::gConfig().GetIONames()).fWeightFileDir = getKeyWord(MLMname,"knnErrXML","outFileDirKnnErr");

  // setup the factory
  // -----------------------------------------------------------------------------------------------------------
  TString outFileNameTrain = getKeyWord(MLMname,"knnErrXML","outFileNameKnnErr");
  if(glob->GetOptB("isReadOnlySys")) knnErrOutFile = NULL;
  else                               knnErrOutFile = new TFile(outFileNameTrain,"RECREATE");
  
  knnErrFactory = new TMVA::Factory(typeANNZ, knnErrOutFile, (TString)verbLvlF+drawProgBarStr+transStr+analysType);

  #if ROOT_TMVA_V0
  typedef TMVA::Factory def_dataLoader;

  knnErrDataLdr = knnErrFactory;
  #else
  typedef TMVA::DataLoader def_dataLoader;

  knnErrDataLdr =  new TMVA::DataLoader("./");;
  #endif

  // since all variables are read-in from TTreeFormula, we define them as floats ("F") in the factory
  int  nInVar = (int)inNamesVar[nMLMnow].size();
  vector <double> fracV(2,0), quantV(2,0);

  if(doWidthRescale) inVarsScaleFunc[nMLMnow].resize(nInVar,NULL);

  for(int nVarNow=0; nVarNow<nInVar; nVarNow++) {
    TString varScaled = inNamesVar[nMLMnow][nVarNow];

    // -----------------------------------------------------------------------------------------------------------
    // if requested, scale the input variables in the knn tree to the range [-1,1]
    // -----------------------------------------------------------------------------------------------------------
    if(doWidthRescale) {
      // fill a histogram with the distribution of the input variable
      // -----------------------------------------------------------------------------------------------------------
      TString hisName   = (TString)aChainKnn->GetName()+utils->regularizeName(inNamesVar[nMLMnow][nVarNow])+"_hisVar";
      TString drawExprs = (TString)inNamesVar[nMLMnow][nVarNow]+">>"+hisName;
      TString wgtCut    = (TString)"("+(TString)cutsAll+")*("+wgtAll+")";
      wgtCut.ReplaceAll("()*()","").ReplaceAll("*()","").ReplaceAll("()*","").ReplaceAll("()","1");

      utils->drawTree(aChainKnn,drawExprs,wgtCut);
 
      TH1 * his_var = (TH1F*)gDirectory->Get(hisName); 
      VERIFY(LOCATION,(TString)"Could not derive histogram ("+hisName+") from chain with drawExprs = \'"
                              +drawExprs+"\' , wgtCut = \'"+wgtCut+"\'"+"... Something is horribly wrong ?!?!",
                              (dynamic_cast<TH1F*>(his_var)));

      his_var->SetDirectory(0); his_var->BufferEmpty(); outputs->BaseDir->cd();

      aLOG(Log::DEBUG_1)<<coutBlue<<" - "<<coutYellow<<MLMname<<coutBlue<<" - deriving var-range: "<<coutPurple
                        <<drawExprs<<coutBlue<<" , "<<coutGreen<<wgtCut<<coutDef<<endl;

      // derive the ranges of the distribution
      // -----------------------------------------------------------------------------------------------------------
      fracV[0] = 0.0; fracV[1] = 1.0; quantV.resize(2,-1);

      int hasQuant = utils->getQuantileV(fracV,quantV,his_var);
      VERIFY(LOCATION,(TString)"Got not compute quantiles for histogram ... Something is horribly wrong ?!?! ",hasQuant);

      // transform the input variable
      // -----------------------------------------------------------------------------------------------------------
      if(quantV[0] < quantV[1]) {
        TString shiftStr = (TString)((quantV[0] > 0) ? " - " : " + ") + utils->floatToStr(fabs(quantV[0]));

        varScaled = (TString)"(("+inNamesVar[nMLMnow][nVarNow]+")"+shiftStr+") * "+utils->floatToStr( 2/(quantV[1] - quantV[0]) )+" - 1";

        TString varScaledFunc(varScaled);  varScaledFunc.ReplaceAll(inNamesVar[nMLMnow][nVarNow],"x");  
        TString varScaledFuncName(utils->regularizeName(varScaledFunc));

        inVarsScaleFunc[nMLMnow][nVarNow] = new TF1(varScaledFuncName,varScaledFunc);

        aLOG(Log::DEBUG_1)<<coutYellow<<"   --> Transformation to range [-1,1] from "<<coutGreen<<inNamesVar[nMLMnow][nVarNow]
                          <<coutYellow<<" to:  "<<coutBlue<<varScaled<<coutDef<<endl;
      }
      else {
        // identity function in order to avoid seg-fault, if the qualtile calculation
        // failed for some reason... (name of object must be different than "x")
        inVarsScaleFunc[nMLMnow][nVarNow] = new TF1("x+0","x");
      }

      // his_var->SaveAs((TString)hisName+"_scaled.C");
      DELNULL(his_var);
    }

    // set the input variable in the tree
    ((def_dataLoader*)knnErrDataLdr)->AddVariable(varScaled,varScaled,"",'F');
  }

  // add targets for all available MLMs
  trgIndexV.resize(nMLMs,-1);

  int              nTrgIn(0);
  vector <TString> branchNameV;
  utils->getTreeBranchNames(aChainKnn,branchNameV);
  for(int nBranchNameNow=0; nBranchNameNow<(int)branchNameV.size(); nBranchNameNow++) {
    TString branchName = branchNameV[nBranchNameNow];  if(!branchName.Contains(baseTag_errKNN)) continue;

    ((def_dataLoader*)knnErrDataLdr)->AddTarget(branchName,branchName);
    
    int indexErrKNN = getErrKNNtagNow(branchName);
    trgIndexV[indexErrKNN] = nTrgIn;
    
    nTrgIn++;
  }

  // -----------------------------------------------------------------------------------------------------------
  // set a cut to sub-sample the dataset if requested, as [0 < sampleFrac_errKNN < 1]
  // -----------------------------------------------------------------------------------------------------------
  if(sampleFrac > 1+EPS || sampleFrac < 0) {
    aLOG(Log::WARNING) <<coutRed<<" - Found \"sampleFrac_errKNN\" = "<<coutYellow<<sampleFrac<<coutRed
                       <<" ... Value must be between 0 and 1 !!!"<<coutDef<<endl;

    sampleFrac = max(0., min(1., sampleFrac));
  }
  if(sampleFrac < 1-EPS) {
    int nTrainObj = max(0, static_cast<int>(floor(aChainKnn->GetEntries() * sampleFrac)));

    VERIFY(LOCATION,(TString)"Following sampleFracInp_wgtKNN/sampleFracRef_wgtKNN cut, chain("
                            +(TString)aChainKnn->GetName()+") with initial "+utils->lIntToStr(aChainKnn->GetEntries())
                            +" objects, now has "+utils->lIntToStr(nTrainObj)+" objects (minimum is \"minObjTrainTest\" = "
                            +utils->lIntToStr(minObjTrainTest)+") ...",(nTrainObj >= minObjTrainTest));

    TString fracCut("");
    for(int nTrySplit=0; nTrySplit<5; nTrySplit++) {
      int split0  = pow(10,nTrySplit);
      int split1  = static_cast<int>(floor(EPS + split0 / sampleFrac));

      if(split0 != split1) {
        fracCut = (TString)indexName+" % "+TString::Format("%d < %d",split1,split0);
        break;
      }
    }
    if(fracCut != "") {
      cutsAll += (TCut)fracCut;

      aLOG(Log::INFO) <<coutGreen<<" - will use sampleFrac_errKNN = "<<coutYellow<<sampleFrac<<coutGreen
                      <<" --> cut on sample by: "<<coutYellow<<cutsAll<<coutDef<<endl;
    }
  }

  // let TMVA know the name of the XML file
  (TMVA::gConfig().GetIONames()).fWeightFileDir = getKeyWord(MLMname,"trainXML","outFileDirTrain");

  ((def_dataLoader*)knnErrDataLdr)->AddRegressionTree(aChainKnn, 1, TMVA::Types::kTraining);
  ((def_dataLoader*)knnErrDataLdr)->SetWeightExpression(wgtAll,"Regression");
  ((def_dataLoader*)knnErrDataLdr)->PrepareTrainingAndTestTree(cutsAll,trainValidStr);

  #if ROOT_TMVA_V0
  TMVA::MethodKNN * knnErrMethod = dynamic_cast<TMVA::MethodKNN*>(
    knnErrFactory->BookMethod(TMVA::Types::kKNN, baseName_knnErr, (TString)optKNN+verbLvlM)
  );
  #else
  TMVA::MethodKNN * knnErrMethod = dynamic_cast<TMVA::MethodKNN*>(
    knnErrFactory->BookMethod(((def_dataLoader*)knnErrDataLdr), TMVA::Types::kKNN, baseName_knnErr, (TString)optKNN+verbLvlM)
  );
  #endif
  
  knnErrModule = knnErrMethod->fModule;

  // fill the module with events made from the tree entries and create the binary tree
  // -----------------------------------------------------------------------------------------------------------
  knnErrMethod->Train();

 //   // create the events
 //  knnErrMethod->fEvent.clear();
 //  for(UInt_t ievt=0; ievt<knnErrMethod->GetNEvents(); ++ievt) {
 //     // read the training event
 //   const Event * evt_ = knnErrMethod->GetEvent(ievt);
 //   Double_t    weight = evt_->GetWeight();          if(weight <= 0) continue;

 //   TMVA::kNN::VarVec vvec(knnErrMethod->GetNVariables(), 0.0);      
 //   for(UInt_t ivar=0; ivar<evt_->GetNVariables(); ++ivar) vvec[ivar] = evt_->GetValue(ivar);

 //     // Create event and add it to knnErrMethod->fEvent
 //     TMVA::kNN::Event evt_orig(vvec, weight, 1);
 //   evt_orig.SetTargets(evt_->GetTargets());
 //   knnErrMethod->fEvent.push_back(evt_orig);
 // }
 //   // fill the module with the events
 // knnErrModule = knnErrMethod->fModule;  knnErrModule->Clear();
 // for (TMVA::kNN::EventVec::const_iterator event=knnErrMethod->fEvent.begin(); event!=knnErrMethod->fEvent.end(); ++event) knnErrModule->Add(*event);
 //   // create the binary tree
 //   string option; if(knnErrMethod->fScaleFrac > 0.0) option += "metric";  if(knnErrMethod->fTrim) option += "trim";
 // knnErrModule->Fill(static_cast<UInt_t>(knnErrMethod->fBalanceDepth), static_cast<UInt_t>(100.0*knnErrMethod->fScaleFrac), option);

  aLOG(Log::DEBUG_1) <<coutGreen<<" - "<<coutBlue<<MLMname<<coutGreen<<" - kd-tree ("<<knnErrMethod->fEvent.size()
                     <<")"<<coutYellow<<" , opts = "<<coutRed<<optKNN<<coutYellow<<" , "<<coutRed<<trainValidStr<<coutYellow
                     <<" , cuts = "<<coutRed<<cutsAll<<coutYellow<<" , weights = "<<coutRed<<wgtAll<<coutDef<<endl;

  // sanity check - if this is not true, the distance calculations will be off
  VERIFY(LOCATION,(TString)"Somehow the fScaleFrac for the kd-tree is not zero ... Something is horribly wrong ?!?!?",(knnErrMethod->fScaleFrac < EPS));

  outputs->BaseDir->cd();

  return;
}

// ===========================================================================================================
/**
 * @brief                - Clean up the objects created for KNN error estimation.
 * 
 * @param knnErrOutFile  - A TFile which was created as part of the setup of the TMVA::Factory in setupKdTreeKNN().
 * @param knnErrFactory  - A pointer to the TMVA::Factory which was created in setupKdTreeKNN().
 * @param knnErrDataLdr  - A pointer to the TMVA::DataLoader, needed for ROOT versions > 6.8.
 * @param verb           - Flag for activating debugging output.
 */
// ===========================================================================================================
void ANNZ::cleanupKdTreeKNN(
  TFile *& knnErrOutFile, TMVA::Factory *& knnErrFactory,
  TMVA::Configurable *& knnErrDataLdr, bool verb
) {
// ===========================================================================================================
  TString message("");
  
  message = "knnErrFactory"; if(knnErrFactory) message += (TString)": "+knnErrFactory->GetName();
  DELNULL_(LOCATION,knnErrFactory,message,verb);

  // we use the if-condition for knnErrDataLdr even for ROOT_TMVA_V0, and avoid an "unused warning"
  if(knnErrDataLdr) {
    #if !ROOT_TMVA_V0
    // since knnErrDataLdr points to knnErrFactory in case of ROOT_TMVA_V0, then the "if(knnErrDataLdr)"
    // check will result in a seg fault - knnErrDataLdr will be valid, but not point to an object...
    // therefore we must bracket all of this block within the ROOT_TMVA_V0 check!
    message = "knnErrDataLdr"; if(knnErrDataLdr) message += (TString)": "+knnErrDataLdr->GetName();
    DELNULL_(LOCATION,knnErrDataLdr,message,verb);
    #endif
  }

  message = "knnErrOutFile"; if(knnErrOutFile) message += (TString)": "+knnErrOutFile->GetName();
  DELNULL_(LOCATION,knnErrOutFile,message,verb);

  // after closing a TFile, need to return to the correct directory, or else histogram pointers will be affected
  outputs->BaseDir->cd();
  
  return;
}



// ===========================================================================================================
/**
 * @brief               - Derive the KNN-error estimation for regression and classification MLMs.
 * 
 * @details             - The errors are derived using nErrKNN near-neighbours of the current object from
 *                      the "primary MLM". For each neighbour we compute the result of the TMVA::Reader of the
 *                      uncertainty estimator of the KNN TMVA::Factory. This is compared to the "true" value -
 *                      either the value of the traget in regression problems, or the true classification of
 *                      signal/background in the case of classification problems. The distribution of
 *                      [estimator - truth] is used to derive the final KNN error. The KNN factory is connected
 *                      to a tree which has all of the regression/classification variables (in order to compute
 *                      the distances). The factory also has "regression targets", which correspond to the
 *                      difference between the estimated and the "true" result - see ANNZ::createTreeErrKNN().
 *                  
 * @param var           - A VarMaps object which may update the values of the input-variables which are
 *                      linked to the TMVA::Reader object.
 * @param knnErrModule  - A pointer to the TMVA::kNN::ModulekNN which is created by the TMVA::Factory, and is
 *                      used to get the near-neighbours in.
 * @param trgIndexV     - vector of indices of the position of a given MLM-index in the target-list of the KNN factory.
 * @param nMLMv         - vector of MLM indices for which the errors are computed.
 * @param isREG         - flag to indicate if the error is for a regression target or for classification.
 * @param zErrV         - vector to hold negative/average/positive error estimates for each MLM.
 */
// ===========================================================================================================
void ANNZ::getRegClsErrKNN(
  VarMaps * var, TMVA::kNN::ModulekNN * knnErrModule, vector <int> & trgIndexV,
  vector <int> & nMLMv, bool isREG, vector < vector <double> > & zErrV
) {
// ===========================================================================================================
  aLOG(Log::DEBUG_3) <<coutWhiteOnBlack<<coutBlue<<" - starting ANNZ::getRegClsErrKNN() ... "<<coutDef<<endl;
  
  int  nMLMsIn        = (int)nMLMv.size();  if(nMLMsIn == 0) return;
  int  nMLMs          = glob->GetOptI("nMLMs");
  int  nErrKNN        = glob->GetOptI("nErrKNN");
  bool doWidthRescale = glob->GetOptB((TString)"doWidthRescale_errKNN");
  int  nMLM_0         = nMLMv[0];
  int  nInVar         = (int)inNamesVar[nMLM_0].size();

  // sanith check of the initialization of trgIndexV
  VERIFY(LOCATION,(TString)" - trgIndexV is not initialized in ANNZ::getRegClsErrKNN() !!!",((int)trgIndexV.size() == nMLMs));

  for(int nMLMinNow=0; nMLMinNow<nMLMsIn; nMLMinNow++) {
    VERIFY(LOCATION,(TString)" - trgIndexV is not initialized in ANNZ::getRegClsErrKNN() !!!",(trgIndexV[nMLMv[nMLMinNow]] >= 0));
  }

  vector <TH1 *> his1V(nMLMsIn);
  for(int nMLMinNow=0; nMLMinNow<nMLMsIn; nMLMinNow++) {
    TString hisName  = TString::Format("hisErrKNN_%d",nMLMinNow);
    his1V[nMLMinNow] = new TH1F(hisName,hisName,1e3,1,-1);
    his1V[nMLMinNow]->SetDefaultBufferSize(nErrKNN+2);
  }

  // update the variables connected to the reader
  var->updateReaderFormulae(readerInptV,true);

  // get the content of readerInptV into a VarVec
  TMVA::kNN::VarVec vvec(nInVar,0);
  for(int nInVarNow=0; nInVarNow<nInVar; nInVarNow++) {
    int    readerInptIndex = readerInptIndexV[nMLM_0][nInVarNow];
    double readerInptVal   = readerInptV[readerInptIndex].second;

    VERIFY(LOCATION,(TString)"There's a mixup with input variables and the reader... Something is horribly wrong... ?!?"
                             ,(inNamesVar[nMLM_0][nInVarNow] == readerInptV[readerInptIndex].first));
    if(doWidthRescale) {
      bool hasScaleFunf(false);
      if((int)inVarsScaleFunc[nMLM_0].size() > nInVarNow) hasScaleFunf = dynamic_cast<TF1*>(inVarsScaleFunc[nMLM_0][nInVarNow]);
      VERIFY(LOCATION,(TString)"Has not defined a scaling function for \""+readerInptV[readerInptIndex].first
                               +"\"... Something is horribly wrong... ?!?",hasScaleFunf);

      vvec[nInVarNow] = inVarsScaleFunc[nMLM_0][nInVarNow]->Eval(readerInptVal);
      // cout<<nMLM_0<<CT<<readerInptV[readerInptIndex].first<<CT<<readerInptVal<<CT<<inVarsScaleFunc[nMLM_0][nInVarNow]->GetName()<<CT<<vvec[nInVarNow]<<endl;
    }
    else {
      vvec[nInVarNow] = readerInptVal;
    }
  }
  // for(int nMLMinNow=0; nMLMinNow<nMLMsIn; nMLMinNow++) cout <<" ---- "<<nMLMinNow<<CT<<nMLMv[nMLMinNow]<<endl;

  const TMVA::kNN::Event evt_orig(vvec,1,0);
  knnErrModule->Find(evt_orig,nErrKNN+2);
 
  const TMVA::kNN::List & listKNN = knnErrModule->GetkNNList();

  for(TMVA::kNN::List::const_iterator itrKNN=listKNN.begin(); itrKNN!=listKNN.end(); ++itrKNN) {
    if(itrKNN->second < EPS) continue; // the distance to this neighbour must be positive

    const TMVA::kNN::Event & evt_knn = itrKNN->first->GetEvent();
    double                 knnWgt    = evt_knn.GetWeight();

    for(int nMLMinNow=0; nMLMinNow<nMLMsIn; nMLMinNow++) {
      int nMLMnow = nMLMv[nMLMinNow];
      int nTrgKNN = trgIndexV[nMLMnow];

      his1V[nMLMinNow]->Fill(evt_knn.GetTgt(nTrgKNN),knnWgt);
      // cout <<evt_knn.GetNTgt()<<CT<<nMLMnow<<CT<<nTrgKNN<<"  -> "<<evt_knn.GetTgt(nTrgKNN)<<endl;
    }
  }
  
  // derive the errors from the histograms
  vector <double> fracV(3), quantV(3,-1);
  fracV[0] = 0.16; fracV[1] = 0.5; fracV[2] = 0.84;

  for(int nMLMinNow=0; nMLMinNow<nMLMsIn; nMLMinNow++) {
    int nMLMnow = nMLMv[nMLMinNow];

    his1V[nMLMinNow]->BufferEmpty();

    double zErr(-1), zErrP(-1), zErrN(-1);

    utils->param->clearAll();
    if(utils->getQuantileV(fracV,quantV,his1V[nMLMinNow])) {
      if(isREG) { zErr = (quantV[2] - quantV[0])/2.; zErrP = (quantV[2] - quantV[1]); zErrN = (quantV[1] - quantV[0]); }
      else      { zErr = quantV[1];                  zErrP = 0;                       zErrN = 0;                       }
    }
    else if(inLOG(Log::DEBUG_2)) {
      aLOG(Log::DEBUG_2) <<coutRed<< " - got undefined err calculation for inputs:"<<coutDef<<endl;
      for(int nInVarNow=0; nInVarNow<nInVar; nInVarNow++) {
        int readerInptIndex = readerInptIndexV[nMLM_0][nInVarNow];
        aLOG(Log::DEBUG_2) <<coutRed<<"   - "<<coutYellow<< readerInptV[readerInptIndex].first<<coutGreen
                           <<CT<<readerInptV[readerInptIndex].second<<coutDef<<endl;
      }
    }

    zErrV[nMLMnow].resize(3);
    zErrV[nMLMnow][0] = zErrN; zErrV[nMLMnow][1] = zErr; zErrV[nMLMnow][2] = zErrP;

    DELNULL(his1V[nMLMinNow]);
  }

  fracV.clear(); quantV.clear(); his1V.clear();

  return;
}


// ===========================================================================================================
/**
 * @brief           - A simple method to propagate the uncertainty on input-parameters to an uncertainty
 *                  on an MLM. This may be used instead of the nominal KNN error estimation, provided that
 *                  input-variables errors are provided in inputVarErrors.
 * 
 * @details         - The errors on the input-parameters are assumed to be Gaussian and uncorrelated. The
 *                  uncertainty on the MLM is computed by generating random shifts to each input-parameter
 *                  based on the respective error. The value of the MLM estimator is then computed using the
 *                  shifted inputs. The distribution of the smeared MLM estimations is used to derive the final
 *                  uncertainty.
 *         
 * @param var       - A VarMaps object which may update the values of the input-variables which are
 *                  linked to the TMVA::Reader object.
 * @param isREG     - The type of MLM used (regression, or one of two classification estimators).
 * @param nMLMnow   - The index of the current MLM.
 * @param seedP     - A seed for random number generation, which should be dfferent every time the
 *                  function is called.
 * @param zErrV      - optional vector to hold negative/average/positive error estimates.
 *                  
 * @return          - The value of the KNN error (returns -1 in case of failure).
 */
// ===========================================================================================================
double ANNZ::getRegClsErrINP(
  VarMaps * var, bool isREG, int nMLMnow, UInt_t * seedP, vector <double> * zErrV
) {
// ===========================================================================================================
  VERIFY(LOCATION,(TString)"Memory leak ?! ",(dynamic_cast<VarMaps*>(var)));

  aLOG(Log::DEBUG_3) <<coutWhiteOnBlack<<coutBlue<<" - starting ANNZ::getRegClsErrINP() ... "<<coutDef<<endl;

  UInt_t seed(0);
  if(seedP) { seed = *seedP; (*seedP) += 1; }

  TRandom * rnd       = new TRandom(seed);
  TString MLMname     = getTagName(nMLMnow);
  int     nInVar      = (int)inNamesVar[nMLMnow].size();
  int     nErrINP     = glob->GetOptI("nErrINP");
  int     nErrINPHalf = static_cast<int>(floor(0.01 + nErrINP/2.));

  vector <double> inVarErrV(nInVar);
  for(int nInVarNow=0; nInVarNow<nInVar; nInVarNow++) {
    inVarErrV[nInVarNow] = var->GetForm(getTagInVarErr(nMLMnow,nInVarNow));

    aLOG(Log::DEBUG_3) <<coutGreen<<" - "<<coutRed<<nInVarNow<<coutGreen<<" - var,err: "
                       <<coutYellow<<inNamesVar[nMLMnow][nInVarNow]<<coutGreen<<" , "
                       <<coutYellow<<inNamesErr[nMLMnow][nInVarNow]<<coutGreen<<"\t -> current value of err: "
                       <<coutRed<<inVarErrV[nInVarNow]<<coutDef<<endl;
  }

  // make sure the values of the variables which are connected to the reader are up to date
  // and store the original evaluation-result of the MLM, as well as the original values of the inputs
  // also - var->updateReaderFormulae(readerInptV,forceUpdate) can only work with forceUpdate=false
  // after it was caalled once with forceUpdate=true since the previousvar0>getTreeEntry()
  ANNZ_readType readType = isREG ? ANNZ_readType::REG : ANNZ_readType::PRB;

  double  regClsOrig(getReader(var,readType,true,nMLMnow)), regClsSmear(0);
  vector < pair<TString,Float_t> > readerInptVorig = readerInptV;

  TH1 * his1 = new TH1F("hisErrINP","hisErrINP",1e3,1,-1);
  his1->SetDefaultBufferSize(glob->GetOptI("nErrINP") + 10);

  for(int nSmearRndNow=0; nSmearRndNow<nErrINP; nSmearRndNow++) {
    // go over all input variables and smear each according to the corresponding error
    for(int nInVarNow=0; nInVarNow<nInVar; nInVarNow++) {
      int    readerInptIndex = readerInptIndexV[nMLMnow][nInVarNow];
      double sfNow           = fabs(rnd->Gaus(0,inVarErrV[nInVarNow])); if(nSmearRndNow < nErrINPHalf) sfNow *= -1;

      readerInptV[readerInptIndex].second = readerInptVorig[readerInptIndex].second + sfNow;
    }
    // evaluate the MLM without updating the reader variables from the formulae of var
    regClsSmear = getReader(var,readType,false,nMLMnow);

    his1->Fill(regClsSmear);
  }

  his1->BufferEmpty();

  double          zErr(-1), zErrP(-1),zErrN(-1);
  vector <double> fracV(3), quantV(3,-1);
  fracV[0] = 0.16; fracV[1] = 0.5; fracV[2] = 0.84;

  utils->param->clearAll();
  if(utils->getQuantileV(fracV,quantV,his1)) {
    zErr = (quantV[2] - quantV[0])/2.; zErrP = (quantV[2] - quantV[1]); zErrN = (quantV[1] - quantV[0]);
  }
  if(zErrV) {
    zErrV->resize(3);
    (*zErrV)[0] = zErrN; (*zErrV)[1] = zErr; (*zErrV)[2] = zErrP;
  }
  fracV.clear(); quantV.clear();

  // force a reset of the variables which are connected to the reader to the correct values and check that we get the
  // original evaluation result back
  regClsSmear = getReader(var,readType,true,nMLMnow);
  VERIFY(LOCATION,(TString)"Somehow the error calculation messed up the MLM reader ... Something is horribly wrong ?!?!?"
                 ,(fabs(regClsOrig-regClsSmear) < 1e-10));

  DELNULL(his1); inVarErrV.clear(); readerInptVorig.clear();

  if(inLOG(Log::DEBUG_3)) {
    TString debugStr("");
    if(zErrV) debugStr = (TString) getTagError(nMLMnow,"N")+" = "+utils->floatToStr(zErrN)+", "
                                 + getTagError(nMLMnow,"") +" = "+utils->floatToStr(zErr )+", "
                                 + getTagError(nMLMnow,"P")+" = "+utils->floatToStr(zErrP);
    else      debugStr = (TString) getTagError(nMLMnow)+" = "+utils->floatToStr(zErr);
   
    aLOG(Log::DEBUG_3) <<coutBlue<<" - Got "<<coutYellow<<debugStr<<coutDef<<endl;
  }

  return zErr;
}




