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
 * @brief                - Setup a TMVA::Factory which will be used for KNN-error estimation.
 * 
 * @details              
 *                       - The TMVA::Factory is linked to the training dataset, and is initiated with the same
 *                       input-variables as the "primary MLM", for which the error are calculated.
 *              
 * @param aChain         - A chain linked to the training dataset.
 * @param cutsAll        - Cuts used on the dataset, which should match the cuts on the primary MLM.
 * @param nMLMnow        - The index of the primary MLM.
 * @param knnErrOutFile  - A TFile which is created as part of the setup of the TMVA::Factory (needs to be deleted suring cleanup).
 * @param knnErrFactory  - A pointed to the TMVA::Factory which is created here.
 * @param knnErrModule   - A pointer to the TMVA::kNN::ModulekNN which is created by the TMVA::Factory, and is later
 *                       used to get the near-neighbours in getRegClsErrKNN().
 * @param cutsSig        - Cuts on the signal dataset, used for classification MLMs.
 * @param cutsBck        - Cuts on the background dataset, used for classification MLMs.
 * @param wgtReg         - Weights for the entire dataset, used for regression MLMs.
 * @param wgtSig         - Weights for the signal dataset, used for classification MLMs.
 * @param wgtBck         - Weights for the background dataset, used for classification MLMs.
 */
// ===========================================================================================================
void ANNZ::setupKdTreeKNN(TChain * aChain, TCut cutsAll, int nMLMnow, TFile *& knnErrOutFile,
                          TMVA::Factory *& knnErrFactory, TMVA::kNN::ModulekNN *& knnErrModule,
                          TCut cutsSig, TCut cutsBck, TString wgtReg, TString wgtSig, TString wgtBck) {
// ====================================================================================================

  TString MLMname     = getTagName(nMLMnow);
  TString knnErrName  = glob->GetOptC("baseName_knnErr");
  int     nErrKNN     = glob->GetOptI("nErrKNN");
  bool    debug       = inLOG(Log::DEBUG_2) || glob->OptOrNullB("debugErrANNZ");

  if(debug) aCustomLOG("") <<coutWhiteOnBlack<<coutBlue<<" - starting ANNZ::setupKdTreeKNN() ... "<<coutDef<<endl;

  TString verbLvlF        = (TString)(debug ? ":V:!Silent" : ":!V:Silent");
  TString verbLvlM        = (TString)(debug ? ":V:H"       : ":!V:!H");
  TString drawProgBarStr  = (TString)(debug ? ":Color:DrawProgressBar" : ":Color:!DrawProgressBar");

  bool    isReg           = (glob->GetOptB("doRegression") && !glob->GetOptB("doBinnedCls"));
  TString optKNN          = TString::Format(":nkNN=%d:ScaleFrac=0.0",nErrKNN);
  TString transStr        = (TString)":Transformations=I";
  TString analysType      = (TString)":AnalysisType="+(isReg ? "Regression" : "Classification");
  TString typeANNZ        = (TString)glob->GetOptC("typeANNZ")+knnErrName;
  TString trainValidStr   = (TString)(!isReg ? "" :   TString::Format("nTrain_Regression=%d:nTest_Regression=%d",0,0)
                                                    + TString::Format(":SplitMode=Random:NormMode=NumEvents:!V") );
  
  (TMVA::gConfig().GetIONames()).fWeightFileDir = getKeyWord(MLMname,"knnErrXML","outFileDirKnnErr");

  // setup the factory
  // -----------------------------------------------------------------------------------------------------------
  TString outFileNameTrain = getKeyWord(MLMname,"knnErrXML","outFileNameKnnErr");
  knnErrOutFile            = new TFile(outFileNameTrain,"RECREATE");
  knnErrFactory            = new TMVA::Factory(typeANNZ, knnErrOutFile, (TString)verbLvlF+drawProgBarStr+transStr+analysType);    

  prepFactory(nMLMnow,knnErrFactory);

  if(isReg) {  
    knnErrFactory->AddRegressionTree(aChain, 1, TMVA::Types::kTraining);
    knnErrFactory->SetWeightExpression(wgtReg,"Regression");
  }
  else {
    knnErrFactory->AddSignalTree    (aChain,1,TMVA::Types::kTraining);
    knnErrFactory->AddBackgroundTree(aChain,1,TMVA::Types::kTraining);

    knnErrFactory->AddCut(cutsSig, "Signal");     knnErrFactory->SetWeightExpression(wgtSig,"Signal");
    knnErrFactory->AddCut(cutsBck, "Background"); knnErrFactory->SetWeightExpression(wgtBck,"Background");

    VERIFY(LOCATION,(TString)"Must define sig/bck cuts in setupKdTreeKNN()",(cutsSig != "" && cutsBck != ""));
  }

  knnErrFactory->PrepareTrainingAndTestTree(cutsAll,trainValidStr);

  TMVA::MethodKNN * knnErrMethod = dynamic_cast<TMVA::MethodKNN*>(knnErrFactory->BookMethod(TMVA::Types::kKNN, knnErrName,(TString)optKNN+verbLvlM));
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
                     <<")"<<coutYellow<<" , opts = "<<coutRed<<optKNN<<coutYellow<<" , "<<coutRed<<trainValidStr<<coutYellow<<" , cuts(all,sig,bck) = "
                     <<coutRed<<cutsAll<<coutYellow<<" , "<<coutRed<<cutsSig<<coutYellow<<" , "<<coutRed<<cutsBck<<coutYellow
                     <<" , weights(reg,sig,bck) = "<<coutRed<<wgtReg<<coutYellow<<" , "<<coutRed<<wgtSig
                     <<coutYellow<<" , "<<coutRed<<wgtBck<<coutDef<<endl;

  // sanity check - if this is not true, the events in the binary tree will have the wrong "units"
  VERIFY(LOCATION,(TString)"Somehow the fScaleFrac for the kd-tree is not zero ... Something is horribly wrong ?!?!?",(knnErrMethod->fScaleFrac < EPS));

  outputs->BaseDir->cd();

  return;
}

// ===========================================================================================================
/**
 * @brief                - Clean up the objects created for KNN error estimation.
 * 
 * @param knnErrOutFile  - A TFile which was created as part of the setup of the TMVA::Factory in setupKdTreeKNN().
 * @param knnErrFactory  - A pointed to the TMVA::Factory which was created in setupKdTreeKNN().
 * @param verb           - Flag for activating debugging output.
 */
// ===========================================================================================================
void ANNZ::cleanupKdTreeKNN(TFile *& knnErrOutFile, TMVA::Factory *& knnErrFactory, bool verb) {
// =============================================================================================
  TString message("");
  
  message = "knnErrFactory"; if(knnErrFactory) message += (TString)": "+knnErrFactory->GetName();
  DELNULL_(LOCATION,knnErrFactory,message,verb);

  message = "knnErrOutFile"; if(knnErrOutFile) message += (TString)": "+knnErrOutFile->GetName();
  DELNULL_(LOCATION,knnErrOutFile,message,verb);

  // after closing a TFile, need to return to the correct directory, or else histogram pointers will be affected
  outputs->BaseDir->cd();
  
  return;
}

// ===========================================================================================================
/**
 * @brief                - Derive the KNN-error estimation for regression and classification MLMs.
 * 
 * @details              
 *                       - The errors are derived using nErrKNN near-neighbours of the current object from
 *                       the "primary MLM". For each neighbour we compute the result of the TMVA::Reader of the
 *                       uncertainty estimator of the KNN TMVA::Factory. This is compared to the "true" value -
 *                       either the value of the traget in regression problems, or the true classification of
 *                       signal/background in the case of classification problems. The distribution of
 *                       [estimator - truth] is used to derive the final KNN error.
 *              
 * @param var            - A VarMaps object which may update the values of the input-variables which are
 *                       linked to the TMVA::Reader object.
 * @param readType       - The type of MLM used (regression, or one of two classification estimators).
 * @param nMLMnow        - The index of the current MLM.
 * @param knnErrModule   - A pointer to the TMVA::kNN::ModulekNN which is created by the TMVA::Factory, and is
 *                       used to get the near-neighbours in.
* @param zErrV           - optional vector to hold negative/average/positive error estimates.
 *                       
 * @return               - The value of the KNN error (returns -1 in case of failure).
 */
// ===========================================================================================================
double ANNZ::getRegClsErrKNN(VarMaps * var, ANNZ_readType readType, int nMLMnow, TMVA::kNN::ModulekNN * knnErrModule, vector <double> * zErrV) {
// =============================================================================================================================================
  VERIFY(LOCATION,(TString)"Memory leak ?! ",(dynamic_cast<VarMaps*>(var)));
  VERIFY(LOCATION,(TString)"Only REG,PRB options currently supported in getRegClsErrKNN()",(readType == ANNZ_readType::REG || readType == ANNZ_readType::PRB));

  bool debug = inLOG(Log::DEBUG_3);
  aLOG(Log::DEBUG_2) <<coutWhiteOnBlack<<coutBlue<<" - starting ANNZ::getRegClsErrKNN() ... "<<coutDef<<endl;

  TString MLMname = getTagName(nMLMnow);
  int     nInVar  = (int)inNamesVar[nMLMnow].size();
  bool    isREG   = (readType == ANNZ_readType::REG);

  int    nkNNErrMin(0);
  double deltaMax(0), errMaxDifZ(0);
  if(isREG) {
    deltaMax   = glob->GetOptF("maxValZ") - glob->GetOptF("minValZ");
    errMaxDifZ = glob->GetOptF("kNNErrMaxDifZ");
    nkNNErrMin = glob->GetOptI("nkNNErrMin");
  }

  TH1 * his1 = new TH1D("hisErrKNN","hisErrKNN",1e3,1,-1);
  his1->SetDefaultBufferSize(glob->GetOptI("nErrKNN") + 10);

  // make sure the values of the variables which are connected to the reader are up to date and
  // extract the original MLM output before changing the content of readerInptV
  double  regClsOrig = getReader(var,readType,true,nMLMnow);
  vector < pair<TString,Float_t> > readerInptVorig = readerInptV;

  TMVA::kNN::VarVec vvec(nInVar,0);
  for(int nInVarNow=0; nInVarNow<nInVar; nInVarNow++) {
    int readerInptIndex = readerInptIndexV[nMLMnow][nInVarNow];
    vvec[nInVarNow]     = readerInptV[readerInptIndex].second;

    VERIFY(LOCATION,(TString)"There's a mixup with input variables and the reader... Something is horribly wrong... ?!?"
                             ,(inNamesVar[nMLMnow][nInVarNow] == readerInptV[readerInptIndex].first));
    // aLOG(Log::DEBUG_2) <<"var check: "<<MLMname<<CT<<nInVarNow<<CT<<readerInptIndex<<CT<<inNamesVar[nMLMnow][nInVarNow]<<CT<<readerInptV[readerInptIndex].first<<endl;
  }

  if(inLOG(Log::DEBUG_2)) {
    aLOG(Log::DEBUG_2) <<coutGreen<<" - Original reg/cls value = "<<coutYellow<<regClsOrig<<coutDef<<endl;
    for(int nInVarNow=0; nInVarNow<nInVar; nInVarNow++) {
      aLOG(Log::DEBUG_2)<<coutPurple<<" --- Input variable value "<<nInVarNow<<CT<<coutGreen
                        <<inNamesVar[nMLMnow][nInVarNow]<<CT<<vvec[nInVarNow]<<coutDef<<endl;
    }
  }

  const TMVA::kNN::Event evt_orig(vvec,1,0);
  knnErrModule->Find(evt_orig, glob->GetOptI("nErrKNN")+2);
 
  const TMVA::kNN::List & listKNN = knnErrModule->GetkNNList();

  int     nkNNErrNow(0), knnType(0);
  double  knnTrue0(0), knnDist(0), knnWgt(0), knnNow(0), knnTrue(0), knnDelta(0);//, knnDist0(0);  //double evtZ[2],evtDist[2];
  for(TMVA::kNN::List::const_iterator itrKNN=listKNN.begin(); itrKNN!=listKNN.end(); ++itrKNN) {
    if(itrKNN->second < EPS) continue; // the distance to this neighbour must be positive

    const TMVA::kNN::Event & evt_knn = itrKNN->first->GetEvent();

    // update the content of readerInptV before evaluating the MLM
    for(int nInVarNow=0; nInVarNow<nInVar; nInVarNow++) {
      int readerInptIndex = readerInptIndexV[nMLMnow][nInVarNow];
      readerInptV[readerInptIndex].second = evt_knn.GetVar(nInVarNow);
    }

    knnDist  = evt_orig.GetDist(evt_knn);
    knnWgt   = evt_knn.GetWeight();
    knnNow   = getReader(var,readType,false,nMLMnow); // get the reader estimate without re-computing the content of readerInptV

    if(isREG) {
      knnTrue  = evt_knn.GetTgt(0);
      knnDelta = knnNow - knnTrue;

      if(fabs(knnDelta) > deltaMax)                 { continue;                                           }
      if(nkNNErrNow == 0)                           { knnTrue0 = knnTrue;                                 }
      if(nkNNErrNow > nkNNErrMin && errMaxDifZ > 0) { if(fabs(knnTrue - knnTrue0) > errMaxDifZ) continue; }
    }
    else {
      knnType  = evt_knn.GetType();
      if     (knnType == 1) knnDelta = 1-knnNow; // signal     (GetType() == 1) - expect PRG = 1
      else if(knnType == 2) knnDelta =   knnNow; // background (GetType() == 2) - expect PRG = 0
      else VERIFY(LOCATION,(TString)"Unrecognized type (of expected signal/background) ... Something is horribly wrong ?!?!",false);
    }

    // the error of each object wrt its own true value
    // see: http://arxiv.org/abs/0810.2991 - Estimating the Redshift Distribution of Photometric Galaxy... - Sect. 4.2
    his1->Fill(knnDelta,knnWgt);

    if(debug) {
      aLOG(Log::DEBUG_3) <<coutBlue<<"Target  "<<nkNNErrNow<<CT<<knnDelta<<CT<<knnWgt<<"\t ->  "<<knnDist<<CT<<knnNow<<CT<<knnTrue<<CT<<knnType<<coutDef<<endl;
      if(nkNNErrNow < 5) {
        for(int nInVarNow=0; nInVarNow<nInVar; nInVarNow++) {
          int readerInptIndex = readerInptIndexV[nMLMnow][nInVarNow];
          aLOG(Log::DEBUG_3)<<coutRed<<"   --- Input variable value "<<nInVarNow<<CT<<coutGreen<<inNamesVar[nMLMnow][nInVarNow]
              <<CT<<readerInptVorig[readerInptIndex].second<<CT<<readerInptV[readerInptIndex].second <<coutDef<<endl;
        }
      }
    }
    nkNNErrNow++;
  }
  
  his1->BufferEmpty();

  double          zErr(-1), zErrP(-1),zErrN(-1);
  vector <double> fracV(3), quantV(3,-1);
  fracV[0] = 0.16; fracV[1] = 0.5; fracV[2] = 0.84;

  utils->param->clearAll();
  if(utils->getQuantileV(fracV,quantV,his1)) {
    if(isREG) {
      zErr  = (quantV[2] - quantV[0])/2.; zErrP = (quantV[2] - quantV[1]); zErrN = (quantV[1] - quantV[0]);
    }
    else {
      zErr = quantV[1]; zErrP = zErrN = 0;
    }
  }
  if(zErrV) {
    zErrV->resize(3);
    (*zErrV)[0] = zErrN; (*zErrV)[1] = zErr; (*zErrV)[2] = zErrP;
  }
  fracV.clear(); quantV.clear();
  // if(utils->getInterQuantileStats(his1)) {
  //   if(isREG) {
  //     zErr = glob->GetOptB("defErrBySigma68") ? utils->param->GetOptF("quant_sigma_68") : utils->param->GetOptF("quant_sigma") ;
  //   }
  //   else {
  //     zErr = utils->param->GetOptF("quant_median");
  //   }
  // }

  // force a reset of the variables which are connected to the reader to the correct values and check if
  // we get back the correct result from the MLM
  double regClsTest = getReader(var,readType,true,nMLMnow);

  // sanity check
  VERIFY(LOCATION,(TString)"Somehow the error calculation messed up the MLM reader ... Something is horribly wrong ?!?!?"
                           , (fabs(regClsOrig-regClsTest) < 1e-10));

  DELNULL(his1); readerInptVorig.clear();

  if(inLOG(Log::DEBUG_2)) {
    TString debugStr("");
    if(zErrV) debugStr = (TString) getTagError(nMLMnow,"N")+" = "+utils->floatToStr(zErrN)+", "
                                 + getTagError(nMLMnow,"") +" = "+utils->floatToStr(zErr )+", "
                                 + getTagError(nMLMnow,"P")+" = "+utils->floatToStr(zErrP);
    else      debugStr = (TString) getTagError(nMLMnow)+" = "+utils->floatToStr(zErr);
   
    aLOG(Log::DEBUG_2) <<coutBlue<<" - Got "<<coutYellow<<debugStr<<coutDef<<endl;
  }

  return zErr;
}

// ===========================================================================================================
/**
 * @brief           - A simple method to propagate the uncertainty on input-parameters to an uncertainty
 *                  on an MLM. This may be used instead of the nominal KNN error estimation, provided that
 *                  input-variables errors are provided in inputVarErrors.
 * 
 * @details              
 *                  - The errors on the input-parameters are assumed to be Gaussian and uncorrelated. The
 *                  uncertainty on the MLM is computed by generating random shifts to each input-parameter
 *                  based on the respective error. The value of the MLM estimator is then computed using the
 *                  shifted inputs. The distribution of the smeared MLM estimations is used to derive the final
 *                  uncertainty.
 *         
 * @param var       - A VarMaps object which may update the values of the input-variables which are
 *                  linked to the TMVA::Reader object.
 * @param readType  - The type of MLM used (regression, or one of two classification estimators).
 * @param nMLMnow   - The index of the current MLM.
 * @param seedP     - A seed for random number generation, which should be dfferent every time the
 *                  function is called.
* @param zErrV      - optional vector to hold negative/average/positive error estimates.
 *                  
 * @return          - The value of the KNN error (returns -1 in case of failure).
 */
// ===========================================================================================================
double ANNZ::getRegClsErrINP(VarMaps * var, ANNZ_readType readType, int nMLMnow, UInt_t * seedP, vector <double> * zErrV) {
// ========================================================================================================================
  VERIFY(LOCATION,(TString)"Memory leak ?! ",(dynamic_cast<VarMaps*>(var)));

  aLOG(Log::DEBUG_2) <<coutWhiteOnBlack<<coutBlue<<" - starting ANNZ::getRegClsErrINP() ... "<<coutDef<<endl;

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

    aLOG(Log::DEBUG_2) <<coutGreen<<" - "<<coutRed<<nInVarNow<<coutGreen<<" - var,err: "<<coutYellow<<inNamesVar[nMLMnow][nInVarNow]<<coutGreen<<" , "
                       <<coutYellow<<inNamesErr[nMLMnow][nInVarNow]<<coutGreen<<"\t -> current value of err: "<<coutRed<<inVarErrV[nInVarNow]<<coutDef<<endl;
  }

  // make sure the values of the variables which are connected to the reader are up to date
  // and store the original evaluation-result of the MLM, as well as the original values of the inputs
  // also - var->updateReaderFormulae(readerInptV,forceUpdate) can only work with forceUpdate=false
  // after it was caalled once with forceUpdate=true since the previousvar0>getTreeEntry()
  double  regClsOrig(getReader(var,readType,true,nMLMnow)), regClsSmear(0);
  vector < pair<TString,Float_t> > readerInptVorig = readerInptV;

  TH1 * his1 = new TH1D("hisErrINP","hisErrINP",1e3,1,-1);
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
  // double zErr(-1);
  // utils->param->clearAll();
  // if(utils->getInterQuantileStats(his1)) {
  //   zErr = glob->GetOptB("defErrBySigma68") ? utils->param->GetOptF("quant_sigma_68") : utils->param->GetOptF("quant_sigma") ;
  // }
  fracV.clear(); quantV.clear();

  // fore a reset of the variables which are connected to the reader to the correct values and check that we get the
  // original evaluation result back
  regClsSmear = getReader(var,readType,true,nMLMnow);
  VERIFY(LOCATION,(TString)"Somehow the error calculation messed up the MLM reader ... Something is horribly wrong ?!?!?",(fabs(regClsOrig-regClsSmear) < 1e-10));

  DELNULL(his1); inVarErrV.clear(); readerInptVorig.clear();

  if(inLOG(Log::DEBUG_2)) {
    TString debugStr("");
    if(zErrV) debugStr = (TString) getTagError(nMLMnow,"N")+" = "+utils->floatToStr(zErrN)+", "
                                 + getTagError(nMLMnow,"") +" = "+utils->floatToStr(zErr )+", "
                                 + getTagError(nMLMnow,"P")+" = "+utils->floatToStr(zErrP);
    else      debugStr = (TString) getTagError(nMLMnow)+" = "+utils->floatToStr(zErr);
   
    aLOG(Log::DEBUG_2) <<coutBlue<<" - Got "<<coutYellow<<debugStr<<coutDef<<endl;
  }

  return zErr;
}




