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
 * @brief  - Interface for training methods.
 */
// ===========================================================================================================
void ANNZ::Train() {
// =================
  if(!glob->GetOptB("trainingNeeded")) return;

  // choose the training type
  if     (glob->GetOptB("doBinnedCls"))      Train_binnedCls();
  else if(glob->GetOptB("doClassification")) Train_singleCls();
  else                                       Train_singleReg();

  return;
}

// ===========================================================================================================
/**
 * @brief    - Training for a single classification MLM.
 *
 * @details
 *           - Train a single MLM. Begin by splittting the input dataset into signal and background trees,
 *           then do the training. Finally, use makeTreeRegClsOneMLM() to write output trees with the
 *           result of the training, which will be used later on.
 */
// ===========================================================================================================
void ANNZ::Train_singleCls() {
// =========================== 
  aLOG(Log::INFO) <<coutWhiteOnBlack<<coutBlue<<" - starting ANNZ::Train_singleCls() ... "<<coutDef<<endl;

  int     minObjTrainTest = glob->GetOptI("minObjTrainTest");
  int     nMLMnow         = glob->GetOptI("nMLMnow");
  TString MLMname         = getTagName(nMLMnow);

  setNominalParams(nMLMnow,glob->GetOptC("inputVariables"),glob->GetOptC("inputVarErrors"));

  OptMaps * optMap = new OptMaps("localOptMap");

  optMap->NewOptI("nRnd0",nMLMnow);
  optMap->NewOptI("nRnd1",0);

  // define basic TMVA setup, create a new root output file and a factory
  // -----------------------------------------------------------------------------------------------------------
  TString       outFileNameTrain = getKeyWord(MLMname,"trainXML","outFileNameTrain");
  TFile         * outputFile     = new TFile(outFileNameTrain,"RECREATE");
  TMVA::Factory * factory        = new TMVA::Factory(glob->GetOptC("typeANNZ"), outputFile, glob->GetOptC("factoryFlags"));    

  prepFactory(nMLMnow,factory);

  // generate the MLM type/options and store the results in optMap
  // -----------------------------------------------------------------------------------------------------------
  generateOptsMLM(optMap,glob->GetOptC("userMLMopts"));

  TString factoryNorm = optMap->GetOptC("factoryNorm");
  TString mlmType     = optMap->GetOptC("type");
  TString mlmOpt      = optMap->GetOptC("opt");

  // input root files for training
  // -----------------------------------------------------------------------------------------------------------
  TString                 inFileName(""), inTreeName("");
  map < TString,TChain* > chainM;
  map < TString,TCut >    cutM;

  for(int trainValidType=0; trainValidType<2; trainValidType++) {
    TString trainValidName  = (TString)((trainValidType == 0) ? "_train" : "_valid");

    inTreeName  = (TString)glob->GetOptC("treeName")+trainValidName;
    inFileName  = (TString)glob->GetOptC("inputTreeDirName")+inTreeName+"*.root";
    chainM[trainValidName] = new TChain(inTreeName,inTreeName); chainM[trainValidName]->SetDirectory(0);  chainM[trainValidName]->Add(inFileName);
    aLOG(Log::DEBUG) <<coutRed<<" - added chain  "<<coutGreen<<inTreeName<<" from "<<coutBlue<<inFileName<<coutDef<<endl;
  }

  // -----------------------------------------------------------------------------------------------------------
  // set input variables and cuts and connect them to the factory
  // - TRAINING CUTS:
  //     these cuts include (glob->GetOptC("testValidType_train")) if has separate sub-sample for training 
  //     convergence and for testing, as this condition is true for all training sample and for half of the testing sample
  // -----------------------------------------------------------------------------------------------------------
  VarMaps * var = new VarMaps(glob,utils,"mainTrainVar");

  var->connectTreeBranches(chainM["_train"]);  // connect the tree so as to allocate memory for cut variables

  setMethodCuts(var,nMLMnow);

  // replace the training/validation cut definition in var, os that cutM["_valid"] will get the testing objects  
  if(glob->GetOptB("separateTestValid")) {
    int nFoundCuts = var->replaceTreeCut(glob->GetOptC("testValidType_valid"),glob->GetOptC("testValidType_train"));
    VERIFY(LOCATION,(TString)"Did not find cut \""+glob->GetOptC("testValidType_valid")+"\". Something is horribly wrong... ?!?",(nFoundCuts != 0));
  }

  TCut cutTrain  = var->getTreeCuts(MLMname+"_train");
  TCut cutValid  = var->getTreeCuts(MLMname+"_valid");

  cutM["_comn"]  = var->getTreeCuts("_comn");
  cutM["_sig"]   = var->getTreeCuts("_sig");
  cutM["_bck"]   = var->getTreeCuts("_bck");
  cutM["_train"] = var->getTreeCuts("_train") + cutTrain;
  cutM["_valid"] = var->getTreeCuts("_valid") + cutValid;

  DELNULL(var);

  // crate new chains with unique signal or background objects
  splitToSigBckTrees(chainM,cutM,optMap);
 
  int nTrain_sig = optMap->GetOptI("ANNZ_nTrain_sig");
  int nTrain_bck = optMap->GetOptI("ANNZ_nTrain_bck");
  int nValid_sig = optMap->GetOptI("ANNZ_nValid_sig");
  int nValid_bck = optMap->GetOptI("ANNZ_nValid_bck");

  TString sigBckStr  = TString::Format("nTrain_Signal=%d:nTrain_Background=%d:nTest_Signal=%d:nTest_Background=%d"
                                       ,nTrain_sig,nTrain_bck,nValid_sig,nValid_bck);

  VERIFY(LOCATION,(TString)"Got the following ["+sigBckStr+"] , where all should be larger than "+utils->intToStr(minObjTrainTest)
                          +" ... Something is horribly wrong ?!?!" ,(   nTrain_bck >= minObjTrainTest && nValid_bck >= minObjTrainTest
                                                                     && nTrain_sig >= minObjTrainTest && nValid_sig >= minObjTrainTest ));

  double  clsWeight(1.0); // weight for the entire sample
  factory->AddSignalTree    (chainM["_train_sig"],clsWeight,TMVA::Types::kTraining);
  factory->AddSignalTree    (chainM["_valid_sig"],clsWeight,TMVA::Types::kTesting );
  factory->AddBackgroundTree(chainM["_train_bck"],clsWeight,TMVA::Types::kTraining);
  factory->AddBackgroundTree(chainM["_valid_bck"],clsWeight,TMVA::Types::kTesting );

  // set the sample-weights
  factory->SetWeightExpression(userWgtsM[MLMname+"_train"],"Signal");
  factory->SetWeightExpression(userWgtsM[MLMname+"_train"],"Background");

  TString trainValidStr = (TString)sigBckStr+":SplitMode=Random:"+factoryNorm;

  aLOG(Log::INFO) <<coutLightBlue<<" - will book ("<<coutYellow<<MLMname<<coutLightBlue<<") method("
                  <<coutYellow<<mlmType<<coutLightBlue<<") with options: "<<coutCyan<<mlmOpt<<coutDef<<endl;

  aLOG(Log::INFO) <<coutLightBlue<<"   - factory settings:   "<<coutCyan<<trainValidStr                                   <<coutDef<<endl;
  aLOG(Log::INFO) <<coutLightBlue<<"   - cuts (all):         "<<coutCyan<<cutM["_comn"]              <<coutLightBlue<<" ,"<<coutDef<<endl;
  aLOG(Log::INFO) <<coutLightBlue<<"     cuts (train):       "<<coutCyan<<cutTrain                   <<coutLightBlue<<" ,"<<coutDef<<endl;
  aLOG(Log::INFO) <<coutLightBlue<<"     cuts (valid):       "<<coutCyan<<cutValid                                        <<coutDef<<endl;
  aLOG(Log::INFO) <<coutLightBlue<<"   - weights (train):    "<<coutCyan<<userWgtsM[MLMname+"_train"]                     <<coutDef<<endl;
  aLOG(Log::INFO) <<coutLightBlue<<"     weights (valid):    "<<coutCyan<<userWgtsM[MLMname+"_valid"]                     <<coutDef<<endl;

  // cuts have already been applied during splitToSigBckTrees(), so leave empty here
  factory->PrepareTrainingAndTestTree((TCut)"",trainValidStr);

  TMVA::Types::EMVA typeNow = getTypeMLMbyName(mlmType);
  factory->BookMethod(typeNow,MLMname,mlmOpt+glob->GetOptC("trainFlagsMLM")); typeMLM[nMLMnow] = typeNow;
  
  // -----------------------------------------------------------------------------------------------------------
  // train the factory  
  // -----------------------------------------------------------------------------------------------------------
  doFactoryTrain(factory);  

  // save the configuration of this MLM - this info will be loaded and used for the reader
  // -----------------------------------------------------------------------------------------------------------
  TString saveFileName = getKeyWord(MLMname,"trainXML","configSaveFileName");
  aLOG(Log::INFO)<<coutYellow<<" - Saving MLM information in "<<coutGreen<<saveFileName<<coutYellow<<" ..."<<coutDef<<endl;

  TString          saveName("");
  vector <TString> optNames;
  saveName = "configSave_name";   optNames.push_back(saveName); optMap->NewOptC(saveName, MLMname);
  saveName = "configSave_type";   optNames.push_back(saveName); optMap->NewOptC(saveName, mlmType);
  saveName = "configSave_opts";   optNames.push_back(saveName); optMap->NewOptC(saveName, mlmOpt);
  saveName = "inputVariables";    optNames.push_back(saveName); optMap->NewOptC(saveName, glob->GetOptC(saveName));
  saveName = "inputVarErrors";    optNames.push_back(saveName); optMap->NewOptC(saveName, glob->GetOptC(saveName));
  saveName = "userWeights_train"; optNames.push_back(saveName); optMap->NewOptC(saveName, userWgtsM[MLMname+"_train"]);
  saveName = "userWeights_valid"; optNames.push_back(saveName); optMap->NewOptC(saveName, userWgtsM[MLMname+"_valid"]);
  saveName = "userCuts_train";    optNames.push_back(saveName); optMap->NewOptC(saveName, (TString)userCutsM[MLMname+"_train"]);
  saveName = "userCuts_valid";    optNames.push_back(saveName); optMap->NewOptC(saveName, (TString)userCutsM[MLMname+"_valid"]);  
  saveName = "userCuts_sig";      optNames.push_back(saveName); optMap->NewOptC(saveName, (TString)userCutsM[        "_sig"]);
  saveName = "userCuts_bck";      optNames.push_back(saveName); optMap->NewOptC(saveName, (TString)userCutsM[        "_bck"]);

  utils->optToFromFile(&optNames,optMap,saveFileName,"WRITE");

  optNames.clear();

  // cleanup
  // -----------------------------------------------------------------------------------------------------------
  aLOG(Log::DEBUG)<<coutYellow<<"Delete outputFile " <<outputFile<<coutDef<<endl; DELNULL(outputFile);
  aLOG(Log::DEBUG)<<coutYellow<<"Delete factory    " <<factory   <<coutDef<<endl; DELNULL(factory);

  if(!glob->GetOptB("keepTrainingTrees_factory")) utils->safeRM(outFileNameTrain,inLOG(Log::DEBUG));

  for(map <TString,TChain*>::iterator itr = chainM.begin(); itr!=chainM.end(); ++itr) {
    if(!itr->second) continue;

    TString chainName = itr->second->GetName();
    aLOG(Log::DEBUG)<<coutYellow<<"Delete "<<chainName<<"      " <<itr->second<<coutDef<<endl;

    if((chainName.Contains("_sig") || chainName.Contains("_bck")) && !glob->GetOptB("keepTrainingTrees_sigBckCut")) {
      TString sysCmnd = (TString)glob->GetOptC("trainDirNameFull")+chainName+"*.root";
      utils->safeRM(sysCmnd,inLOG(Log::DEBUG));
    }

    DELNULL(itr->second);
  }
  chainM.clear(); cutM.clear();

  DELNULL(optMap);

  // -----------------------------------------------------------------------------------------------------------
  // generate result-trees from the trained MLM
  // -----------------------------------------------------------------------------------------------------------
  makeTreeRegClsOneMLM(nMLMnow);

  return;
}

// ===========================================================================================================
/**
 * @brief    - Training for a single regression MLM.
 *
 * @details
 *           - Train a single MLM. Begin by creating a cut version of the trees, in case the training and validation
 *           cuts are different. After doing the training, use makeTreeRegClsOneMLM() to write output trees with the
 *           result of the training, which will be used later on.
 */
// ===========================================================================================================
void ANNZ::Train_singleReg() {
// ===========================
  aLOG(Log::INFO) <<coutWhiteOnBlack<<coutBlue<<" - starting ANNZ::Train_singleReg() ... "<<coutDef<<endl;

  int     minObjTrainTest = glob->GetOptI("minObjTrainTest");
  int     nMLMnow         = glob->GetOptI("nMLMnow");
  TString MLMname         = getTagName(nMLMnow);
  TString zTrgName        = glob->GetOptC("zTrg");
  int     maxNobj         = glob->GetOptI("maxNobj");

  setNominalParams(nMLMnow,glob->GetOptC("inputVariables"),glob->GetOptC("inputVarErrors"));

  OptMaps * optMap = new OptMaps("localOptMap");

  optMap->NewOptI("nRnd0",nMLMnow);
  optMap->NewOptI("nRnd1",0);

  // define basic TMVA setup, create a new root output file and a factory
  // -----------------------------------------------------------------------------------------------------------
  TString       outFileNameTrain = getKeyWord(MLMname,"trainXML","outFileNameTrain");
  TFile         * outputFile     = new TFile(outFileNameTrain,"RECREATE");
  TMVA::Factory * factory        = new TMVA::Factory(glob->GetOptC("typeANNZ"), outputFile, glob->GetOptC("factoryFlags"));    

  prepFactory(nMLMnow,factory);

  // generate the MLM type/options and store the results in optMap
  // -----------------------------------------------------------------------------------------------------------
  generateOptsMLM(optMap,glob->GetOptC("userMLMopts"));

  TString factoryNorm = optMap->GetOptC("factoryNorm");
  TString mlmType     = optMap->GetOptC("type");
  TString mlmOpt      = optMap->GetOptC("opt");

  // input root files for training
  // -----------------------------------------------------------------------------------------------------------
  map < TString,TChain* > chainM;
  map < TString,TCut >    cutM;

  for(int trainValidType=0; trainValidType<2; trainValidType++) {
    TString trainValidName  = (TString)((trainValidType == 0) ? "_train" : "_valid");

    TString inTreeName = (TString)glob->GetOptC("treeName")+trainValidName;
    TString inFileName = (TString)glob->GetOptC("inputTreeDirName")+inTreeName+"*.root";
    chainM[trainValidName] = new TChain(inTreeName,inTreeName); chainM[trainValidName]->SetDirectory(0);  chainM[trainValidName]->Add(inFileName);
    aLOG(Log::DEBUG) <<coutRed<<" - added chain  "<<coutGreen<<inTreeName<<" from "<<coutBlue<<inFileName<<coutDef<<endl;
  }
  verifTarget(chainM["_train"]); verifTarget(chainM["_valid"]);

  // -----------------------------------------------------------------------------------------------------------
  // set input variables and cuts and connect them to the factory
  // - TRAINING CUTS:
  //     these cuts include (glob->GetOptC("testValidType_train")) if has separate sub-sample for training 
  //     convergence and for testing, as this condition is true for all training sample and for half of the testing sample
  // -----------------------------------------------------------------------------------------------------------
  VarMaps * var = new VarMaps(glob,utils,"mainTrainVar");

  var->connectTreeBranches(chainM["_train"]);  // connect the tree so as to allocate memory for cut variables

  setMethodCuts(var,nMLMnow);

  // replace the training/validation cut definition in var, os that cutM["_valid"] will get the testing objects  
  if(glob->GetOptB("separateTestValid")) {
    int nFoundCuts = var->replaceTreeCut(glob->GetOptC("testValidType_valid"),glob->GetOptC("testValidType_train"));
    VERIFY(LOCATION,(TString)"Did not find cut \""+glob->GetOptC("testValidType_valid")+"\". Something is horribly wrong... ?!?",(nFoundCuts != 0));
  }

  cutM["_comn"]    = var->getTreeCuts("_comn");
  cutM["_train"]   = var->getTreeCuts("_train") + var->getTreeCuts(MLMname+"_train");
  cutM["_valid"]   = var->getTreeCuts("_valid") + var->getTreeCuts(MLMname+"_valid");

  TString cutTrain = ((TString)var->getTreeCuts(MLMname+"_train")).ReplaceAll(" ","");
  TString cutValid = ((TString)var->getTreeCuts(MLMname+"_valid")).ReplaceAll(" ","");

  DELNULL(var);

  // if the cuts for training and validation are different, create new trees
  // for each of these with the corresponding cuts.
  if(cutTrain != cutValid) {
    createCutTrainTrees(chainM,cutM,optMap);
    cutM["_combined"] = "";
  }
  else {
    cutM  ["_combined"]  = cutM["_comn"] + cutM["_train"];
    chainM["_train_cut"] = chainM["_train"]; chainM.erase("_train");
    chainM["_valid_cut"] = chainM["_valid"]; chainM.erase("_valid");
  }

  double regWeight(1.0); // weight for the entire sample
  factory->AddRegressionTree(chainM["_train_cut"], regWeight, TMVA::Types::kTraining);
  factory->AddRegressionTree(chainM["_valid_cut"], regWeight, TMVA::Types::kTesting );

  // set the sample-weights  
  factory->SetWeightExpression(userWgtsM[MLMname+"_train"],"Regression");

  TCanvas * tmpCnvs = new TCanvas("tmpCnvs","tmpCnvs");
  int nTrain = chainM["_train_cut"]->Draw(zTrgName,cutM["_combined"]); if(maxNobj > 0 && maxNobj < nTrain) nTrain = maxNobj;
  int nValid = chainM["_valid_cut"]->Draw(zTrgName,cutM["_combined"]); if(maxNobj > 0 && maxNobj < nValid) nValid = maxNobj;
  DELNULL(tmpCnvs);

  TString trainValidStr = TString::Format( (TString)"nTrain_Regression=%d:nTest_Regression=%d:SplitMode=Random:",nTrain,nValid )
                        + factoryNorm;

  VERIFY(LOCATION,(TString)"Got the following ["+trainValidStr+"] , where all should be larger than "+utils->intToStr(minObjTrainTest)
                          +" ... Something is horribly wrong ?!?!" ,(nTrain >= minObjTrainTest && nValid >= minObjTrainTest));

  aLOG(Log::INFO) <<coutLightBlue<<" - will book ("<<coutYellow<<MLMname<<coutLightBlue<<") method("
                  <<coutYellow<<mlmType<<coutLightBlue<<") with options: "<<coutCyan<<mlmOpt<<coutDef<<endl;

  aLOG(Log::INFO) <<coutLightBlue<<"   - factory settings:   "<<coutCyan<<trainValidStr                                   <<coutDef<<endl;
  aLOG(Log::INFO) <<coutLightBlue<<"   - cuts (all):         "<<coutCyan<<cutM["_comn"]              <<coutLightBlue<<" ,"<<coutDef<<endl;
  aLOG(Log::INFO) <<coutLightBlue<<"     cuts (train):       "<<coutCyan<<cutTrain                   <<coutLightBlue<<" ,"<<coutDef<<endl;
  aLOG(Log::INFO) <<coutLightBlue<<"     cuts (valid):       "<<coutCyan<<cutValid                                        <<coutDef<<endl;
  aLOG(Log::INFO) <<coutLightBlue<<"   - weights (train):    "<<coutCyan<<userWgtsM[MLMname+"_train"]                     <<coutDef<<endl;
  aLOG(Log::INFO) <<coutLightBlue<<"     weights (valid):    "<<coutCyan<<userWgtsM[MLMname+"_valid"]                     <<coutDef<<endl;

  factory->PrepareTrainingAndTestTree(cutM["_combined"],trainValidStr);

  TMVA::Types::EMVA typeNow = getTypeMLMbyName(mlmType);
  factory->BookMethod(typeNow,MLMname,mlmOpt+glob->GetOptC("trainFlagsMLM")); typeMLM[nMLMnow] = typeNow;
  
  // -----------------------------------------------------------------------------------------------------------
  // train the factory  
  // -----------------------------------------------------------------------------------------------------------
  doFactoryTrain(factory);  

  // save the configuration of this MLM - this info will be loaded and used for the reader
  // -----------------------------------------------------------------------------------------------------------
  TString saveFileName = getKeyWord(MLMname,"trainXML","configSaveFileName");
  aLOG(Log::INFO)<<coutYellow<<" - Saving MLM information in "<<coutGreen<<saveFileName<<coutYellow<<" ..."<<coutDef<<endl;

  TString          saveName("");
  vector <TString> optNames;
  saveName = "configSave_name";   optNames.push_back(saveName); optMap->NewOptC(saveName, MLMname);
  saveName = "configSave_type";   optNames.push_back(saveName); optMap->NewOptC(saveName, mlmType);
  saveName = "configSave_opts";   optNames.push_back(saveName); optMap->NewOptC(saveName, mlmOpt);
  saveName = "inputVariables";    optNames.push_back(saveName); optMap->NewOptC(saveName, glob->GetOptC(saveName));
  saveName = "inputVarErrors";    optNames.push_back(saveName); optMap->NewOptC(saveName, glob->GetOptC(saveName));
  saveName = "userWeights_train"; optNames.push_back(saveName); optMap->NewOptC(saveName, userWgtsM[MLMname+"_train"]);
  saveName = "userWeights_valid"; optNames.push_back(saveName); optMap->NewOptC(saveName, userWgtsM[MLMname+"_valid"]);
  saveName = "userCuts_train";    optNames.push_back(saveName); optMap->NewOptC(saveName, (TString)userCutsM[MLMname+"_train"]);
  saveName = "userCuts_valid";    optNames.push_back(saveName); optMap->NewOptC(saveName, (TString)userCutsM[MLMname+"_valid"]);
  saveName = "zTrg";              optNames.push_back(saveName); optMap->NewOptC(saveName, glob->GetOptC(saveName));

  utils->optToFromFile(&optNames,optMap,saveFileName,"WRITE");

  optNames.clear();

  // cleanup
  // -----------------------------------------------------------------------------------------------------------
  aLOG(Log::DEBUG)<<coutYellow<<"Delete outputFile " <<outputFile<<coutDef<<endl; DELNULL(outputFile);
  aLOG(Log::DEBUG)<<coutYellow<<"Delete factory    " <<factory   <<coutDef<<endl; DELNULL(factory);

  if(!glob->GetOptB("keepTrainingTrees_factory")) utils->safeRM(outFileNameTrain,inLOG(Log::DEBUG));

  for(map <TString,TChain*>::iterator itr = chainM.begin(); itr!=chainM.end(); ++itr) {
    if(!dynamic_cast<TChain*>(itr->second)) continue;

    aLOG(Log::DEBUG)<<coutYellow<<"Delete "<<itr->second->GetName()<<"      " <<itr->second<<coutDef<<endl;

    DELNULL(itr->second);
  }
  chainM.clear(); cutM.clear();

  if(!glob->GetOptB("keepTrainingTrees_sigBckCut")) {
    utils->safeRM((TString)glob->GetOptC("trainDirNameFull")+glob->GetOptC("treeName")+"_train"+"*.root",inLOG(Log::DEBUG));
    utils->safeRM((TString)glob->GetOptC("trainDirNameFull")+glob->GetOptC("treeName")+"_valid"+"*.root",inLOG(Log::DEBUG));
  }

  DELNULL(optMap);

  // -----------------------------------------------------------------------------------------------------------
  // generate result-trees from the trained MLM
  // -----------------------------------------------------------------------------------------------------------
  makeTreeRegClsOneMLM(nMLMnow);

  return;
}

// ===========================================================================================================
/**
 * @brief    - Training for a single binned-classification MLM.
 *
 * @details
 *           - Train a collection of MLMs in a given classification-bin. The "signal" region of the
 *           classification-bin is defined as a region of values in the traget variable (zTrg) corresponding
 *           to the current value of nMLMnow. The "background" region is defined as all zTrg values smaller
 *           or larger than the lower and upper edges of the signal region respectively.
 *           Out of the collection of MLMs, only the "best" solution is kept. The latter is defined
 *           as the one for which the separation between signal and background is the best (highest).
 */
// ===========================================================================================================
void ANNZ::Train_binnedCls() {
// ===========================  
  aLOG(Log::INFO) <<coutWhiteOnBlack<<coutBlue<<" - starting ANNZ::Train_binnedCls() ... "<<coutDef<<endl;

  int      nTries                = glob->GetOptI("binCls_nTries");
  TString  inputVariables        = glob->GetOptC("inputVariables");
  TString  inputVarErrors        = glob->GetOptC("inputVarErrors");
  int      nMLMnow               = glob->GetOptI("nMLMnow");
  TString  trainDirNameFull      = glob->GetOptC("trainDirNameFull");
  UInt_t   seed0                 = glob->GetOptI("initSeedRnd") * (76640+nMLMnow);
  UInt_t   seed1                 = glob->GetOptI("initSeedRnd") * (12098+nMLMnow);
  TString  zTrgName              = glob->GetOptC("zTrg");
  TString  bckSubsetRange        = glob->GetOptC("binCls_bckSubsetRange");
  int      minObjTrainTest       = glob->GetOptI("minObjTrainTest");
  double   bckShiftMin           = glob->GetOptF("binCls_bckShiftMin");
  double   bckShiftMax           = glob->GetOptF("binCls_bckShiftMax");
  bool     useBckShift           = ((fabs(bckShiftMax - bckShiftMin) < EPS || bckShiftMax > bckShiftMin) && (bckShiftMax > 0 && bckShiftMin > 0));

  TString  MLMname               = getTagName(nMLMnow);
  TString  saveFileName          = getKeyWord(MLMname,"trainXML","configSaveFileName");
  TString  outFileDirTrain       = getKeyWord(MLMname,"trainXML","outFileDirTrain");
  TString  postTrainDirName      = getKeyWord(MLMname,"postTrain","postTrainDirName");
  TString  postTrainSaveFileName = getKeyWord(MLMname,"postTrain","configSaveFileName");
  TRandom3 * rnd0                = new TRandom3(seed0);
  TRandom3 * rnd1                = new TRandom3(seed1);

  OptMaps                 * optMap(NULL);
  TString                 inFileName(""), inTreeName(""), userMLMopts(""), clsBins(""), sysCmnd("");
  map < TString,TChain* > chainM;
  map < TString,TCut >    cutM;
  vector <TString>        optNameV, binClsInpVarV, binClsInpErrV, binClsUsrOptV, tryDirNameV(nTries,"");

  // define internal tag-names for the different MLM candidates
  for(int nTryNow=0; nTryNow<nTries; nTryNow++) {
    tryDirNameV[nTryNow] = (TString)trainDirNameFull+"nTry_"+utils->intToStr(nTryNow)+"/";
  }

  // extract all input-variable and MLM-options provided by the user
  glob->GetAllOptNames(optNameV,"C");
  for(int nOptNow=0; nOptNow<(int)optNameV.size(); nOptNow++) {
    TString optNameNow = optNameV[nOptNow];
    
    if(optNameNow.BeginsWith("inputVariables_")) binClsInpVarV.push_back(glob->GetOptC(optNameNow));
    if(optNameNow.BeginsWith("inputVarErrors_")) binClsInpErrV.push_back(glob->GetOptC(optNameNow));
    if(optNameNow.BeginsWith("userMLMopts_"))    binClsUsrOptV.push_back(glob->GetOptC(optNameNow));
  }
  optNameV.clear();

  int nRndInpVar = (int)binClsInpVarV.size();
  int nRndInpErr = (int)binClsInpErrV.size();
  int nRndUsrOpt = (int)binClsUsrOptV.size();

  if(nRndInpVar == 0) {
    VERIFY(LOCATION,(TString)"Must specify either common input variables for all tries (\"inputVariables\")"
                            +" or at least one randimozation input variable set (\"inputVariables_0\", \"inputVariables_1\"...)"
                            ,(inputVariables != ""));
  }
  else if(inputVariables != "") {
    aLOG(Log::INFO) <<coutBlue<<" - found some \"inputVariables_XXX\" randimozation input variable set. Will ignore "
                    <<"the content of the common input variable setting (\"inputVariables\") ... "<<coutDef<<endl;
  }
  VERIFY(LOCATION,(TString)"If using inputVarErrors_, must specify \"inputVarErrors_XXX\" corresponding to every \"inputVariables_XXX\""
                          ,(nRndInpVar == nRndInpErr || nRndInpErr == 0));

  // -----------------------------------------------------------------------------------------------------------
  // generate an MLM classifier for each candidate-configuration, produce MLM output trees
  // with makeTreeRegClsOneMLM(), and compute the corresponding value of the separation between signal
  // and background.
  // -----------------------------------------------------------------------------------------------------------
  TString sigBckStr("");
  TCut    cutTrain(""), cutValid("");
  int     nTrain_sig(0), nTrain_bck(0), nValid_sig(0), nValid_bck(0);

  for(int nTryNow=0; nTryNow<nTries; nTryNow++) {
    TString nTryName = TString::Format("nTry_%d",nTryNow);

    // the input parameters cycle through the list of configurations provided by the user
    if(nRndInpVar > 0)       inputVariables = binClsInpVarV[ nTryNow % nRndInpVar ];
    if(nRndInpErr > 0)       inputVarErrors = binClsInpErrV[ nTryNow % nRndInpErr ];

    // use each one of the list of user-options for MLM configurations provided by the user. If
    // nTries is larger than nRndUsrOpt, randomized options will be generated by generateOptsMLM()
    if(nTryNow < nRndUsrOpt) userMLMopts    = binClsUsrOptV[nTryNow];
    else                     userMLMopts    = "";

    // the nominal training parameters are re-set for each nTryNow
    setNominalParams(nMLMnow,inputVariables,inputVarErrors);

    optMap = new OptMaps("localOptMap");

    optMap->NewOptI("nRnd0",nMLMnow);
    optMap->NewOptI("nRnd1",nTryNow);

    // define basic TMVA setup, create a new root output file and a factory
    // -----------------------------------------------------------------------------------------------------------
    TString       outFileNameTrain = getKeyWord(MLMname,"trainXML","outFileNameTrain");
    TFile         * outputFile     = new TFile(outFileNameTrain,"RECREATE");
    TMVA::Factory * factory        = new TMVA::Factory(glob->GetOptC("typeANNZ"), outputFile, glob->GetOptC("factoryFlags"));    

    prepFactory(nMLMnow,factory);

    // generate the MLM type/options and store the results in optMap
    // -----------------------------------------------------------------------------------------------------------
    generateOptsMLM(optMap,userMLMopts);

    TString factoryNorm = optMap->GetOptC("factoryNorm");
    TString mlmType     = optMap->GetOptC("type");
    TString mlmOpt      = optMap->GetOptC("opt");

    // create the chains once (they can be re-used as-is for each nTryNow)
    if(nTryNow == 0) {
      // input root files for training
      // -----------------------------------------------------------------------------------------------------------
      for(int trainValidType=0; trainValidType<2; trainValidType++) {
        TString trainValidName  = (TString)((trainValidType == 0) ? "_train" : "_valid");

        inTreeName  = (TString)glob->GetOptC("treeName")+trainValidName;
        inFileName  = (TString)glob->GetOptC("inputTreeDirName")+inTreeName+"*.root";
        chainM[trainValidName] = new TChain(inTreeName,inTreeName); chainM[trainValidName]->SetDirectory(0);  chainM[trainValidName]->Add(inFileName);
        aLOG(Log::DEBUG) <<coutRed<<" - added chain  "<<coutGreen<<inTreeName<<" from "<<coutBlue<<inFileName<<coutDef<<endl;
      }
      verifTarget(chainM["_train"]); verifTarget(chainM["_valid"]);

      // -----------------------------------------------------------------------------------------------------------
      // set input variables and cuts and connect them to the factory
      // - TRAINING CUTS:
      //     these cuts include (glob->GetOptC("testValidType_train")) if has separate sub-sample for training 
      //     convergence and for testing, as this condition is true for all training sample and for half of the testing sample
      // -----------------------------------------------------------------------------------------------------------
      VarMaps * var = new VarMaps(glob,utils,"mainTrainVar");

      var->connectTreeBranches(chainM["_train"]);  // connect the tree so as to allocate memory for cut variables

      setMethodCuts(var,nMLMnow);

      // replace the training/validation cut definition in var, os that cutM["_valid"] will get the testing objects  
      if(glob->GetOptB("separateTestValid")) {
        int nFoundCuts = var->replaceTreeCut(glob->GetOptC("testValidType_valid"),glob->GetOptC("testValidType_train"));
        VERIFY(LOCATION,(TString)"Did not find cut \""+glob->GetOptC("testValidType_valid")+"\". Something is horribly wrong... ?!?",(nFoundCuts != 0));
      }

      cutTrain       = var->getTreeCuts(MLMname+"_train");
      cutValid       = var->getTreeCuts(MLMname+"_valid");

      cutM["_comn"]  = var->getTreeCuts("_comn");
      cutM["_train"] = var->getTreeCuts("_train") + cutTrain;
      cutM["_valid"] = var->getTreeCuts("_valid") + cutValid;

      DELNULL(var);

      // derive the bins and store them in cutM["_sig"], cutM["_bck"] and in userCutsM["_sig"], userCutsM["_bck"]
      // (userCutsM is used in makeTreeRegClsOneMLM() later on, so that var_0->hasFailedTreeCuts("_sig") is well defined)
      clsBins = deriveBinClsBins(chainM,cutM);

      // crate new chains with unique signal or background objects
      splitToSigBckTrees(chainM,cutM,optMap);

      nTrain_sig = optMap->GetOptI("ANNZ_nTrain_sig");
      nTrain_bck = optMap->GetOptI("ANNZ_nTrain_bck");
      nValid_sig = optMap->GetOptI("ANNZ_nValid_sig");
      nValid_bck = optMap->GetOptI("ANNZ_nValid_bck");

      sigBckStr = TString::Format("nTrain_Signal=%d:nTrain_Background=%d:nTest_Signal=%d:nTest_Background=%d"
                                  ,nTrain_sig,nTrain_bck,nValid_sig,nValid_bck);

      VERIFY(LOCATION,(TString)"Got the following ["+sigBckStr+"] , where all should be larger than "+utils->intToStr(minObjTrainTest)+", and the "
                              +"background samples are expected to be larger than the corresponding  signal samples... Something is horribly wrong ?!?!"
                              ,(nTrain_bck >= nTrain_sig && nValid_bck >= nValid_sig && nTrain_sig >= minObjTrainTest && nValid_sig >= minObjTrainTest));
    }

    double  clsWeight(1.0); // weight for the entire sample
    factory->AddSignalTree    (chainM["_train_sig"],clsWeight,TMVA::Types::kTraining);
    factory->AddSignalTree    (chainM["_valid_sig"],clsWeight,TMVA::Types::kTesting );
    factory->AddBackgroundTree(chainM["_train_bck"],clsWeight,TMVA::Types::kTraining);
    factory->AddBackgroundTree(chainM["_valid_bck"],clsWeight,TMVA::Types::kTesting );

    // -----------------------------------------------------------------------------------------------------------
    // set the sample-weights, including special background cuts (if requested by the user)
    // -----------------------------------------------------------------------------------------------------------
    TString wgt_sig(userWgtsM[MLMname+"_train"]), wgt_bck(userWgtsM[MLMname+"_train"]);    
    
    // use bckShiftMax to define cuts (zero weights) for a region of background just around the signal region
    if(useBckShift) {
      double  bckShiftVal = bckShiftMin + rnd0->Rndm() * (bckShiftMax - bckShiftMin);
      TString bckShiftCut = TString::Format( (TString)"("+zTrgName+" <= %f || "+zTrgName+"  > %f)",
                                              zBinCls_binE[nMLMnow]-bckShiftVal, zBinCls_binE[nMLMnow+1]+bckShiftVal );
    
      wgt_bck = (TString)"("+wgt_bck+")*("+bckShiftCut+")";

      aLOG(Log::INFO) <<coutPurple<<" - background cut, nTryNow("<<coutGreen<<nTryNow<<coutPurple<<"): "<<coutBlue<<bckShiftCut<<coutDef<<endl;
    }
    // only use some percentage of the background
    if(bckSubsetRange != "") {
      vector <TString> objRatioV; objRatioV = utils->splitStringByChar(bckSubsetRange,'_');
      
      bool isGoodFormat(objRatioV.size() == 3);
      if(isGoodFormat) isGoodFormat = (objRatioV[0].IsDigit() && objRatioV[1].IsDigit() && objRatioV[2].IsDigit());
      
      VERIFY(LOCATION,(TString)"If using binCls_bckSubsetRange, must set format like e.g., \"5_50_90\" (use between 5 and "
                              +"50 times the number of background objects as signal objects, used 90 percent of the time).",isGoodFormat);

      // low and high limits on the requested ratio between signal and background
      int  bckSubsetMin(utils->strToInt(objRatioV[0])), bckSubsetMax(utils->strToInt(objRatioV[1])), useProb(utils->strToInt(objRatioV[2]));
      VERIFY(LOCATION,(TString)"If using binCls_bckSubsetRange, than for format \"x_y_p\", set (0 < x <= y) and (0 < p <= 100)"
                               ,(bckSubsetMin <= bckSubsetMax && bckSubsetMin > 0 && objRatioV[2] > 0 && objRatioV[2] <= 100));

      // generate a random ratio within the requested limits (bckSubsetVal) and the baseine ratio for the sample (bckSubsetRatio)
      int  bckSubsetVal   = static_cast<int>(floor(0.500001 + rnd1->Rndm() * (bckSubsetMax - bckSubsetMin))) + bckSubsetMin;
      int  bckSubsetRatio = static_cast<int>(ceil(nTrain_bck/double(nTrain_sig)));

      if(useProb/100. > rnd1->Rndm()) {
        if(bckSubsetVal < bckSubsetRatio) {
          // cout << bckSubsetVal <<CT<< bckSubsetRatio <<CT<< (bckSubsetMax - bckSubsetMin) <<CT<<  bckSubsetMin  <<endl;
          TString bckSubsetCut = (TString)getTrainTestCuts("split",0,bckSubsetVal,bckSubsetRatio);

          wgt_bck = (TString)"("+wgt_bck+")*("+bckSubsetCut+")";

          aLOG(Log::INFO) <<coutPurple<<" - background cut, nTryNow("<<coutGreen<<nTryNow<<coutPurple<<"): "<<coutYellow<<bckSubsetRange
                          <<coutPurple<<" -> "<<coutBlue<<bckSubsetCut<<coutDef<<endl;
        }
        else {
          aLOG(Log::INFO) <<coutPurple<<" - background cut, nTryNow("<<coutGreen<<nTryNow<<coutPurple<<"): "<<coutYellow<<bckSubsetRange
                          <<coutPurple<<" larger than the ratio [bck/sig = "<<coutYellow<<bckSubsetRatio
                          <<coutPurple<<"] in the entire sample... No further cut needed."<<coutDef<<endl;
        }
      }
      else {
        aLOG(Log::INFO) <<coutPurple<<" - background cut, nTryNow("<<coutGreen<<nTryNow<<coutPurple<<"): "<<coutRed
                        <<" will not use cut due to "<<coutGreen<<useProb<<coutRed<<"\% use-limit..."<<coutDef<<endl;
      }
    }
    wgt_sig = utils->cleanWeightExpr(wgt_sig); wgt_bck = utils->cleanWeightExpr(wgt_bck);
    
    factory->SetWeightExpression(wgt_sig,"Signal");
    factory->SetWeightExpression(wgt_bck,"Background");

    TString trainValidStr = (TString)sigBckStr+":SplitMode=Random:"+factoryNorm;

    aLOG(Log::INFO) <<coutCyan<<"----------------------------------------------------------------------------------------------------------------"<<coutDef<<endl;
    aLOG(Log::INFO) <<coutGreen<<" - For try "<<coutYellow<<nTryNow+1<<coutGreen<<"/"<<coutPurple<<nTries<<coutGreen<<" ..."<<coutDef<<endl;
    aLOG(Log::INFO) <<coutGreen<<"   got input variables:   "<<coutPurple<<inputVariables<<coutDef<<endl;
    if(inputVarErrors != "") aLOG(Log::INFO) <<coutGreen<<"   got input var' errors: "<<coutPurple<<inputVarErrors<<coutDef<<endl;

    aLOG(Log::INFO) <<coutLightBlue<<" - will book ("<<coutYellow<<MLMname<<coutLightBlue<<") method("
                    <<coutYellow<<mlmType<<coutLightBlue<<") with options: "<<coutCyan<<mlmOpt<<coutDef<<endl;

    aLOG(Log::INFO) <<coutLightBlue<<"   - factory settings:     "<<coutCyan<<trainValidStr                                   <<coutDef<<endl;
    aLOG(Log::INFO) <<coutLightBlue<<"   - cuts (all):           "<<coutCyan<<cutM["_comn"]              <<coutLightBlue<<" ,"<<coutDef<<endl;
    aLOG(Log::INFO) <<coutLightBlue<<"     cuts (train):         "<<coutCyan<<cutTrain                   <<coutLightBlue<<" ,"<<coutDef<<endl;
    aLOG(Log::INFO) <<coutLightBlue<<"     cuts (valid):         "<<coutCyan<<cutValid                                        <<coutDef<<endl;
    aLOG(Log::INFO) <<coutLightBlue<<"   - weights (signal):     "<<coutCyan<<wgt_sig                    <<coutLightBlue<<" ,"<<coutDef<<endl;
    aLOG(Log::INFO) <<coutLightBlue<<"     weights (background): "<<coutCyan<<wgt_bck                                         <<coutDef<<endl;
    aLOG(Log::INFO) <<coutLightBlue<<"   - weights (train):      "<<coutCyan<<userWgtsM[MLMname+"_train"]                     <<coutDef<<endl;
    aLOG(Log::INFO) <<coutLightBlue<<"     weights (valid):      "<<coutCyan<<userWgtsM[MLMname+"_valid"]                     <<coutDef<<endl;
    aLOG(Log::INFO) <<coutCyan<<"----------------------------------------------------------------------------------------------------------------"<<coutDef<<endl;

    // cuts have already been applied during splitToSigBckTrees(), so leave empty here
    factory->PrepareTrainingAndTestTree((TCut)"",trainValidStr);

    TMVA::Types::EMVA typeNow = getTypeMLMbyName(mlmType);
    factory->BookMethod(typeNow,MLMname,mlmOpt+glob->GetOptC("trainFlagsMLM")); typeMLM[nMLMnow] = typeNow;
    
    // -----------------------------------------------------------------------------------------------------------
    // train the factory  
    // -----------------------------------------------------------------------------------------------------------
    doFactoryTrain(factory);  

    // save the configuration of this MLM - this info will be loaded and used for the reader
    // -----------------------------------------------------------------------------------------------------------
    aLOG(Log::INFO)<<coutYellow<<" - Saving MLM information in "<<coutGreen<<saveFileName<<coutYellow<<" ..."<<coutDef<<endl;

    TString          saveName("");
    vector <TString> optNames;
    saveName = "configSave_name";   optNames.push_back(saveName); optMap->NewOptC(saveName, MLMname);
    saveName = "configSave_type";   optNames.push_back(saveName); optMap->NewOptC(saveName, mlmType);
    saveName = "configSave_opts";   optNames.push_back(saveName); optMap->NewOptC(saveName, mlmOpt);
    saveName = "inputVariables";    optNames.push_back(saveName); optMap->NewOptC(saveName, inputVariables);
    saveName = "inputVarErrors";    optNames.push_back(saveName); optMap->NewOptC(saveName, inputVarErrors);
    saveName = "userWeights_train"; optNames.push_back(saveName); optMap->NewOptC(saveName, userWgtsM[MLMname+"_train"]);
    saveName = "userWeights_valid"; optNames.push_back(saveName); optMap->NewOptC(saveName, userWgtsM[MLMname+"_valid"]);
    saveName = "userCuts_train";    optNames.push_back(saveName); optMap->NewOptC(saveName, (TString)cutTrain);
    saveName = "userCuts_valid";    optNames.push_back(saveName); optMap->NewOptC(saveName, (TString)cutValid); 
    saveName = "zTrg";              optNames.push_back(saveName); optMap->NewOptC(saveName, glob->GetOptC(saveName));
    saveName = "minValZ";           optNames.push_back(saveName); optMap->NewOptF(saveName, glob->GetOptF(saveName));
    saveName = "maxValZ";           optNames.push_back(saveName); optMap->NewOptF(saveName, glob->GetOptF(saveName));
    saveName = "binCls_nBins";      optNames.push_back(saveName); optMap->NewOptI(saveName, glob->GetOptI("nMLMs"));
    saveName = "binCls_maxBinW";    optNames.push_back(saveName); optMap->NewOptF(saveName, glob->GetOptF(saveName));
    saveName = "binCls_clsBins";    optNames.push_back(saveName); optMap->NewOptC(saveName, clsBins);
    saveName = MLMname+"_sig";      optNames.push_back(saveName); optMap->NewOptC(saveName, (TString)cutM["_sig"]);
    saveName = MLMname+"_bck";      optNames.push_back(saveName); optMap->NewOptC(saveName, (TString)cutM["_bck"]);

    utils->optToFromFile(&optNames,optMap,saveFileName,"WRITE",true);

    optNames.clear();

    // cleanup
    // -----------------------------------------------------------------------------------------------------------
    aLOG(Log::DEBUG)<<coutYellow<<"Delete outputFile " <<outputFile<<coutDef<<endl; DELNULL(outputFile);
    aLOG(Log::DEBUG)<<coutYellow<<"Delete factory    " <<factory   <<coutDef<<endl; DELNULL(factory);

    if(!glob->GetOptB("keepTrainingTrees_factory")) utils->safeRM(outFileNameTrain,inLOG(Log::DEBUG));

    if(nTryNow == nTries-1) {
      for(map <TString,TChain*>::iterator itr = chainM.begin(); itr!=chainM.end(); ++itr) {
        if(!itr->second) continue;

        TString chainName = itr->second->GetName();
        aLOG(Log::DEBUG)<<coutYellow<<"Delete "<<chainName<<"      " <<itr->second<<coutDef<<endl;

        if((chainName.Contains("_sig") || chainName.Contains("_bck")) && !glob->GetOptB("keepTrainingTrees_sigBckCut")) {
          sysCmnd = (TString)trainDirNameFull+chainName+"*.root";
          utils->safeRM(sysCmnd,inLOG(Log::DEBUG));
        }

        DELNULL(itr->second);
      }
      chainM.clear(); cutM.clear();
    }

    DELNULL(optMap);

    // -----------------------------------------------------------------------------------------------------------
    // generate result-trees from the trained MLM
    // -----------------------------------------------------------------------------------------------------------
    makeTreeRegClsOneMLM(nMLMnow);

    // create a temporary subdir, and move the outputr of the training into it
    // -----------------------------------------------------------------------------------------------------------
    sysCmnd = (TString)"mkdir -p "+tryDirNameV[nTryNow]+" ; mv "+outFileDirTrain+" "+postTrainDirName+" "+tryDirNameV[nTryNow];
    utils->exeShellCmndOutput(sysCmnd,inLOG(Log::DEBUG));
  }

  // -----------------------------------------------------------------------------------------------------------
  // get the separation parameters from file, choose the nTryNow with the highest separation, store
  // all the configuration files for the different files anc cleanup the discarded tries
  // -----------------------------------------------------------------------------------------------------------
  TString hisSepName = "hisSep_nTries";
  TH1     * hisSep   = new TH1D(hisSepName,hisSepName,26,-0.02,1.02);

  optMap = new OptMaps("localOptMap");
  TString          saveName(""), allSeps("");
  vector <TString> optNames;
  saveName = "separation"; optNames.push_back(saveName); optMap->NewOptF(saveName);

  double maxSep(-1);
  int    maxSepIndex(0);
  for(int nTryNow=0; nTryNow<nTries; nTryNow++) {
    TString saveFileNameNow(postTrainSaveFileName); saveFileNameNow.ReplaceAll(trainDirNameFull,tryDirNameV[nTryNow]);

    utils->optToFromFile(&optNames,optMap,saveFileNameNow,"READ","SILENT_KeepFile",inLOG(Log::DEBUG_2));

    double sepNow = optMap->GetOptF(saveName);
    if(sepNow > maxSep) { maxSep = sepNow; maxSepIndex = nTryNow; }

    hisSep->Fill(sepNow);
    allSeps += (TString)TString::Format((TString)coutGreen+"nTry_%d:"+coutPurple+"%1.4f"+coutGreen+", ",nTryNow,sepNow);
  }
  optNames.clear();
  DELNULL(optMap);

  outputs->optClear();
  outputs->draw->NewOptC("generalHeader",MLMname);
  outputs->draw->NewOptC("drawOpt" , "HIST");
  outputs->draw->NewOptB("doNormIntegralWidth" , true);
  outputs->draw->NewOptC("axisTitleX"  , "S_{s/b}");
  outputs->draw->NewOptC("axisTitleY"  , "1/N dN/dS_{s/b}");
  outputs->drawHis1dV(hisSep);
  
  outputs->WriteOutObjects(true,true);
  DELNULL(hisSep);

  // keep all the configSaveFileName files from the different tries
  for(int nTryNow=0; nTryNow<nTries; nTryNow++) {
    for(int nSaveOptTypeNow=0; nSaveOptTypeNow<2; nSaveOptTypeNow++) {
      TString saveFileNameNow = (TString)((nSaveOptTypeNow == 0) ? postTrainSaveFileName : saveFileName);
      saveFileNameNow.ReplaceAll(trainDirNameFull,tryDirNameV[nTryNow]);
      
      TString nTriesTag         = getKeyWord(MLMname,"postTrain","nTriesTag");
      TString namePrefix        = saveFileNameNow(saveFileNameNow.Last('/')+1,saveFileNameNow.Length());
      TString nameAdd           =  (TString)"_nTry_"+utils->intToStr(nTryNow);
      TString configFileDirName = (TString)trainDirNameFull+namePrefix+"/";

      if(configFileDirName.EndsWith(".txt/")) {
        configFileDirName = (TString)configFileDirName(0,configFileDirName.Length()-5)+nTriesTag+"/"; 
      }

      sysCmnd = (TString)"mkdir -p "+configFileDirName;
      if(nTryNow == 0) utils->exeShellCmndOutput(sysCmnd,inLOG(Log::DEBUG_1));

      if(namePrefix.Contains(".")) namePrefix.ReplaceAll(".",nameAdd+".");
      else                         namePrefix += nameAdd;

      sysCmnd = (TString)"cp "+saveFileNameNow+" "+configFileDirName+namePrefix;
      utils->exeShellCmndOutput(sysCmnd,inLOG(Log::DEBUG_1));
    }
  }

  // move the MLM with the best separation to the main directory
  sysCmnd = (TString)"mv "+tryDirNameV[maxSepIndex]+"/* "+trainDirNameFull;
  utils->exeShellCmndOutput(sysCmnd,inLOG(Log::DEBUG));

  // remove the discarded solutions
  for(int nTryNow=0; nTryNow<nTries; nTryNow++) {
    utils->safeRM(tryDirNameV[nTryNow],inLOG(Log::DEBUG_1));
  }

  aLOG(Log::INFO)<<coutYellow<<" - All  separation values:      "<<allSeps<<coutDef<<endl;
  aLOG(Log::INFO)<<coutYellow<<" - Best separation index,value: "<<coutGreen<<maxSepIndex<<" , "<<coutPurple<<maxSep<<coutDef<<endl;

  // cleanup
  binClsInpVarV.clear(); binClsInpErrV.clear(); binClsUsrOptV.clear(); tryDirNameV.clear();
  DELNULL(rnd0); DELNULL(rnd1);

  return;
}

// ===========================================================================================================
/**
 * @brief              - Generate randomized MLM training options
 * 
 * @details
 *                     - For all possible user options, see: http://tmva.sourceforge.net/optionRef.html
 *                     - All TMVA options for the various methods are allowed. The only ANNZ requirements are that the option
 *                     string contains the type of MLM. Also, it is allowed to specify a global normalization, which normaly in
 *                     TMVA is given as part of the TMVA::Factory options.
 *                     - Factory normalization (number of input objects in different sub-samples -> global reweighting of samples) -
 *                     - By default, if [alwaysUseNormalization==true], we use [NormMode=EqualNumEvents]
 *                     the user can specify explicitly [NormMode=NumEvents] or [NormMode=EqualNumEvents], or choose not to
 *                     use normalization by setting [alwaysUseNormalization==false] and not setting NormMode:
 *                       - For example: "ANNZ_MLM=BDT:VarTransform=Norm:NTrees=110:NormMode=NumEvents:BoostType=AdaBoost:" 
 *                       where "ANNZ_MLM=BDT" and "NormMode=NumEvents" are the two options not part of the usual TMVA.
 *                     - Variable transformations (transforme numerical value of inputs to the training).
 *                     If transformation are not booked by user, we randomize the possible combinations.
 *                     (the user may e.g., set ["VarTransform=P"] to force only PCA, or set ["VarTransform=None"] to avoid any transformation.)
 *                     
 * @param optMap       - An OptMaps object which holds some input information (e.g., random number seeds)
 *                     and some output information (e.g., type of generated MLM, the options for the factory etc.).
 * @param userMLMopts  - An optional string from the user with set training options, which is parsed here. If
 *                     instead [userMLMopts=""], the options are generated from scratch.
 */
// ===========================================================================================================
void ANNZ::generateOptsMLM(OptMaps * optMap, TString userMLMopts) {
// ================================================================
  
  TString type(""), opt("");
  int     seed        = glob->GetOptI("initSeedRnd") + (optMap->GetOptI("nRnd0")+1) * 1e5 + optMap->GetOptI("nRnd1");
  int     nRnd        = 14;
  double  * rndAr     = new double[nRnd];
  TRandom * rnd       = new TRandom3(seed); rnd->RndmArray(nRnd,rndAr);
  TString RandomSeed  = TString::Format(":RandomSeed=%d",seed); // doArbitraryRandomization is just for RandomSeed of ANN

  // -----------------------------------------------------------------------------------------------------------
  // built-in randomized options
  // -----------------------------------------------------------------------------------------------------------
  if(userMLMopts == "") {
    aLOG(Log::INFO) <<coutBlue<<" - Found userMLMopts = \"\" - will generate randomized options... "<<coutDef<<endl;

    bool doRndOpt_ANN(glob->GetOptC("rndOptTypes").Contains("ANN")), doRndOpt_BDT(glob->GetOptC("rndOptTypes").Contains("BDT"));
    
    VERIFY(LOCATION,(TString)"Found rndOptTypes = \""+glob->GetOptC("rndOptTypes")+"\", which does not either contain \"ANN\" or \"BDT\", but also "
                            +"userMLMopts = \"\". Please set the former (generate random MLMs) or the latter (define specific MLM options)."
                            , (doRndOpt_ANN || doRndOpt_BDT) );

    if(doRndOpt_ANN && doRndOpt_BDT) {
      if(rndAr[0] < 0.5) { doRndOpt_BDT = false; doRndOpt_ANN = true; }
      else               { doRndOpt_ANN = false; doRndOpt_BDT = true; }
    }

    // random ANN
    // -----------------------------------------------------------------------------------------------------------
    if(doRndOpt_ANN) {
      type = "ANN";

      int nLayer0 = (int)floor(rndAr[1]*5); int nLayer1 = (int)floor(rndAr[2]*10); int nLayer2 = (int)floor(rndAr[3]*5);
      TString layerArc = TString::Format("N+%d,N+%d",nLayer0,nLayer1);
      if(rndAr[1] < 0.3 && rndAr[4] < 0.3) layerArc += TString::Format(",N+%d",nLayer2);
      layerArc.ReplaceAll("+0","");
      
      TString neuronInput = "sum";
      TString neuronType = (TString)((rndAr[5] < 0.5) ? "tanh" : "sigmoid");
      TString useReg     = (TString)((rndAr[6] < 0.5) ? "True" : "False");
      
      TString resetStep(""), convTests(""), testRate("");
      if(rndAr[7] < 0.3) resetStep = "100"; else if(rndAr[7] < 0.65) resetStep = "250"; else resetStep = "500";
      if(rndAr[8] < 0.3) convTests = "-1";  else if(rndAr[8] < 0.65) convTests = "25";  else convTests = "50";
      if(rndAr[9] < 0.3) testRate  = "5";   else if(rndAr[9] < 0.65) testRate  = "25";  else testRate  = "50";

      TString trainMethod = (TString)((rndAr[10] < 0.7) ? "BFGS" : "BP");
      if(glob->GetOptB("doBinnedCls") || glob->GetOptB("doClassification")) trainMethod = "BP";  // XXX - sometimes BFGS crashed for classification...!!!! XXX
      
      TString varTrans("");
      if(rndAr[11] < 0.9) varTrans += "N";
      if(rndAr[12] < 0.5) varTrans += "P";
      varTrans.ReplaceAll("NP","N,P"); if(varTrans != "") varTrans = (TString)":VarTransform="+varTrans;

      opt = (TString)":HiddenLayers="+layerArc+varTrans+":NeuronType="+neuronType+":NeuronInputType="+neuronInput
                             +":TrainingMethod="+trainMethod+":TestRate="+testRate+":NCycles=5000"
                             +":UseRegulator="+useReg+":ConvergenceTests="+convTests+":ConvergenceImprove=1e-30"
                             +":SamplingTraining=False:SamplingTesting=False"+":ResetStep="+resetStep+RandomSeed;
    }

    // random BDT
    // -----------------------------------------------------------------------------------------------------------
    if(doRndOpt_BDT) {
      type = "BDT";

      TString varTrans("");
      if(rndAr[1] < 0.9) varTrans += "N";
      if(rndAr[2] < 0.5) varTrans += "P";
      varTrans.ReplaceAll("NP","N,P"); if(varTrans != "") varTrans = (TString)":VarTransform="+varTrans;

      int     nTreesAdd = floor(rndAr[3]*300/10.) * 10; if(rndAr[10] < 0.2) nTreesAdd *= 3; nTreesAdd = min(nTreesAdd,800);
      TString nTrees    = TString::Format(":NTrees=%d", 50+max(0,nTreesAdd) );

      TString boostType(":BoostType=");
      if     (rndAr[5] < 0.4) boostType += (TString)"Bagging";
      else if(rndAr[5] < 0.8) boostType += (TString)"AdaBoost";
      else {
        if(glob->GetOptB("doBinnedCls") || glob->GetOptB("doClassification")) boostType += (TString)"Grad";       // only for calssification
        else                                                                  boostType += (TString)"AdaBoostR2"; // only for regression
      }

      // XXX try to experiment with different MinNodeSize ??? which replaced nEventsMin XXX is by default: Classification: 5%, Regression: 0.2%
      TString nEventsMin = "";//TString::Format(":nEventsMin=%d",3+(int)floor(rndAr[7]*57));

      TString nCuts      = "";//(TString)((rndAr[8] < 0.3) ? TString::Format(":nCuts=%d",10+(int)floor(rndAr[9]*30/5.)*5) : "");

      opt = (TString)varTrans+nTrees+boostType+nEventsMin+nCuts;
    }

    // factory normalization
    TString normMode = (TString)((rndAr[13] < 0.5) ? "EqualNumEvents" : "NumEvents");
    optMap->NewOptC("factoryNorm",(TString)":NormMode="+normMode);
  }
  // -----------------------------------------------------------------------------------------------------------
  // parse user-provided options
  // -----------------------------------------------------------------------------------------------------------
  else {
    userMLMopts.ReplaceAll(" ","");
    
    // user options
    opt = "NULL";
    for(map <TString,TMVA::Types::EMVA>::iterator Itr = nameToTypeMLM.begin(); Itr!=nameToTypeMLM.end(); ++Itr) {
      TString mlmTypePattern = (TString)"ANNZ_MLM="+Itr->first;
      if(!userMLMopts.Contains(mlmTypePattern)) continue;
      
      type = Itr->first;
      opt  = userMLMopts; opt.ReplaceAll(mlmTypePattern,"");

      if(type == "ANN" && !opt.Contains("RandomSeed")) opt += RandomSeed;
    }
    if(opt == "NULL") {
      TString allTypes("");
      for(map <TString,TMVA::Types::EMVA>::iterator Itr = nameToTypeMLM.begin(); Itr!=nameToTypeMLM.end(); ++Itr) {
        allTypes += (TString)"\""+Itr->first+"\" ";
      }
      VERIFY(LOCATION,(TString)"user options (\""+userMLMopts+"\") do not have the format \"ANNZ_MLM=TYPE:opts\" ..."+
                               "Allowed types are: [ "+allTypes+"]",false);
    }
    // factory normalization (number of input objects in different sub-samples)
    TString optNorm0 = "NormMode=EqualNumEvents"; bool hasNorm0 = opt.Contains(optNorm0);
    TString optNorm1 = "NormMode=NumEvents";      bool hasNorm1 = opt.Contains(optNorm1);
    TString optNorm  = (TString)(glob->GetOptB("alwaysUseNormalization") ? optNorm0 : "NormMode=None");

    if(opt.Contains("NormMode=")) {
      VERIFY(LOCATION,(TString)"User options (\""+userMLMopts+"\") contains \"NormMode\" directive which is not "+
                               "either \""+optNorm0+"\" or \""+optNorm1+"\" ...",(hasNorm0 || hasNorm1));

      optNorm = (TString)":"+(hasNorm0 ? optNorm0 : optNorm1);
      opt.ReplaceAll(optNorm0,""); opt.ReplaceAll(optNorm1,"");
    }
    optMap->NewOptC("factoryNorm",optNorm);

    // variable transformations
    if(!opt.Contains("VarTransform=")) {
      TString varTrans("");
      if(rndAr[0] < 0.9) varTrans += "N";
      if(rndAr[1] < 0.5) varTrans += "P";
      if(rndAr[2] < 0.3) varTrans += "D";
      varTrans.ReplaceAll("NP","N,P").ReplaceAll("PD","P,D").ReplaceAll("ND","N,D");
      if(varTrans != "") opt += (TString)":VarTransform="+varTrans;
    }
  }

  // -----------------------------------------------------------------------------------------------------------
  // add PDFInterpol options for binned classification
  // -----------------------------------------------------------------------------------------------------------
  opt += glob->GetOptC("pdfStr");

  optMap->NewOptC("opt", opt);
  optMap->NewOptC("type",type);

  // cleanup
  DELNULL(rnd); delete [] rndAr;
  return;
}

// ===========================================================================================================
/**
 * @brief         - verify that a tree contains the zTrg branch
 *                     
 * @param aChain  - the input tree which is checked
 */
// ===========================================================================================================
void ANNZ::verifTarget(TTree * aTree) {
// ====================================
  VERIFY(LOCATION,(TString)"Memory leak ?! ",(dynamic_cast<TTree*>(aTree)));

  TString zTrgName = glob->GetOptC("zTrg");

  vector <TString> branchNameV;
  utils->getTreeBranchNames(aTree,branchNameV);

  bool hasBranch = (find(branchNameV.begin(),branchNameV.end(), zTrgName) != branchNameV.end());

  VERIFY(LOCATION,(TString)"Currently the target of the regression (\"zTrg\") can only be set as one of the input variables !",hasBranch);

  branchNameV.clear();
  return;
}
