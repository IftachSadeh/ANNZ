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
 * @brief    - Evaluate method in the classification setup.
 * 
 * @details  - By default, only the "best" MLM is used, as derived by optimCls().
 *           Following the evaluation, both outputt trees and ascii output are created, which contain
 *           the "best" MLM, as derived by optimCls(), and any other individual MLM
 *           estimators which are requested by the user, using MLMsToStore. In addition, any variables from
 *           the original dataset which the user requests (using addOutputVars) are also included in
 *           the output catalog.
 *           - MLM uncertainty estimation may also be included (not added by default).
 */
// ===========================================================================================================
void  ANNZ::doEvalCls() {
// ===========================================================================================================
  aLOG(Log::INFO) <<coutWhiteOnBlack<<coutPurple<<" - starting ANNZ::doEvalCls() ... "<<coutDef<<endl;

  aRegEval = new RegEval("aRegEval",utils,glob,outputs);

  evalClsSetup();
  
  evalClsLoop();

  clearReaders();

  DELNULL(aRegEval);

  return;
}

// ===========================================================================================================
/**
 * @brief  - setups the evaluation.
 */
// ===========================================================================================================
void  ANNZ::evalClsSetup() {
// ===========================================================================================================
  aLOG(Log::DEBUG_1) <<coutWhiteOnBlack<<coutPurple<<" - starting ANNZ::evalClsSetup() ... "<<coutDef<<endl;

  int     nMLMs       = glob->GetOptI("nMLMs");
  bool    optimMLMprb = glob->GetOptB("optimMLMprb");
  aRegEval->hasErrs   = glob->GetOptB("addClsKNNerr");
  aRegEval->hasErrKNN = false;

  aRegEval->mlmSkip  .clear();
  aRegEval->isErrKNNv.resize(nMLMs,aRegEval->hasErrs);
  aRegEval->isErrINPv.resize(nMLMs,false);

  // -----------------------------------------------------------------------------------------------------------
  // figure out which MLMs to generate an error for, using which method
  // (KNN errors or propagation of user-defined parameter-errors)
  // -----------------------------------------------------------------------------------------------------------
  TString allErrsKNN(""), allErrsInp("");
  for(int nMLMnow=0; nMLMnow<nMLMs; nMLMnow++) {
    TString MLMname = getTagName(nMLMnow); if(mlmSkip[MLMname]) continue;

    // this check is safe, since (inNamesErr[nMLMnow].size() > 0) was confirmed in setNominalParams()
    VERIFY(LOCATION,(TString)"inNamesErr["+utils->intToStr(nMLMnow)+"] not initialized..."
                            +" something is horribly wrong ?!?",(inNamesErr[nMLMnow].size() > 0));

    if(aRegEval->isErrKNNv[nMLMnow] && (inNamesErr[nMLMnow][0] != "")) {
      aRegEval->isErrKNNv[nMLMnow] = false;
      aRegEval->isErrINPv[nMLMnow] = true;
    }

    if(aRegEval->isErrKNNv[nMLMnow]) aRegEval->hasErrKNN = true; // at least one is needed

    if(aRegEval->isErrKNNv[nMLMnow]) allErrsKNN += coutBlue+getTagName(nMLMnow)+coutPurple+",";
    if(aRegEval->isErrINPv[nMLMnow]) allErrsInp += coutBlue+getTagName(nMLMnow)+coutPurple+",";
  }

  if(allErrsKNN != "") aLOG(Log::INFO)<<coutYellow<<" - Will gen. errors by KNN method for:   "<<allErrsKNN<<coutDef<<endl;
  if(allErrsInp != "") aLOG(Log::INFO)<<coutYellow<<" - Will gen. input-parameter errors for: "<<allErrsInp<<coutDef<<endl;

  // -----------------------------------------------------------------------------------------------------------
  //
  // -----------------------------------------------------------------------------------------------------------
  TString saveFileName = getKeyWord("","optimResults","configSaveFileName");
  aLOG(Log::INFO)<<coutYellow<<" - Getting optimization results from "<<coutGreen<<saveFileName<<coutYellow<<" ..."<<coutDef<<endl;

  vector <TString> optNames;
  TString          saveName(""), saveNameP(""), saveNameC(""), optimMLMs("");
  OptMaps * optMap = new OptMaps("localOptMap");

  saveName  = glob->versionTag(); optNames.push_back(saveName);  optMap->NewOptC(saveName);
  saveNameP = "optimMLMs_PRB";    optNames.push_back(saveNameP); optMap->NewOptC(saveNameP);
  saveNameC = "optimMLMs_CLS";    optNames.push_back(saveNameC); optMap->NewOptC(saveNameC);

  utils->optToFromFile(&optNames,optMap,saveFileName,"READ","SILENT_KeepFile",inLOG(Log::DEBUG_2));

  optimMLMs = (TString)(optimMLMprb ? optMap->GetOptC(saveNameP) : optMap->GetOptC(saveNameC));

  optNames.clear(); DELNULL(optMap);

  // create a vector of the optimized MLMs from the list string, and check its consistancy
  // -----------------------------------------------------------------------------------------------------------
  aRegEval->mlmInV = utils->splitStringByChar(optimMLMs,';');
  int nMLMsIn      = (int)aRegEval->mlmInV.size();

  // make sure that all the MLMs needed from the optimization are indeed accepted in the current run
  for(int nMLMinNow=0; nMLMinNow<nMLMsIn; nMLMinNow++) {
    TString MLMname = aRegEval->mlmInV[nMLMinNow];
   
    VERIFY(LOCATION,(TString)"Requested MLM ("+MLMname+") not found. Need to retrain ?!?",(!mlmSkip[MLMname]));
  }
  VERIFY(LOCATION,(TString)"Found no accepted MLMs !!!",(nMLMsIn > 0));

  // set the list of selected MLMs to use in the loop
  // -----------------------------------------------------------------------------------------------------------  
  selectUserMLMlist(aRegEval->mlmInV,aRegEval->mlmSkip);

  // load the accepted readers
  // -----------------------------------------------------------------------------------------------------------  
  loadReaders(aRegEval->mlmSkip);

  // setup the knn error estimator, if needed
  // -----------------------------------------------------------------------------------------------------------  
  evalRegErrSetup();  

  return;
}

// ===========================================================================================================
/**
 * @brief  - loop on the input catalogue and perform the evaluation
 */
// ===========================================================================================================
void  ANNZ::evalClsLoop() {
// ===========================================================================================================
  aLOG(Log::DEBUG_1) <<coutWhiteOnBlack<<coutPurple<<" - starting ANNZ::evalClsLoop() ... "<<coutDef<<endl;

  int     nMLMs          = glob->GetOptI("nMLMs");
  bool    optimMLMprb    = glob->GetOptB("optimMLMprb");
  int     maxNobj        = glob->GetOptI("maxNobj");
  TString indexName      = glob->GetOptC("indexName");
  bool    doStoreToAscii = glob->GetOptB("doStoreToAscii");
  UInt_t  seed           = glob->GetOptI("initSeedRnd"); if(seed > 0) seed += 14320;
  int     nMLMsIn        = (int)aRegEval->mlmInV.size();

  // -----------------------------------------------------------------------------------------------------------
  // now create the _valid tree
  // -----------------------------------------------------------------------------------------------------------
  VarMaps * var_0 = new VarMaps(glob,utils,"treeRegClsVar_0");
  VarMaps * var_1 = new VarMaps(glob,utils,"treeRegClsVar_1");

  // create the chain for the loop
  // -----------------------------------------------------------------------------------------------------------
  TString inTreeName = (TString)glob->GetOptC("treeName")+glob->GetOptC("evalTreePostfix");
  TString inFileName = (TString)glob->GetOptC("outDirNameFull")+inTreeName+"*.root";

  // prepare the chain and input variables. Set cuts to match the TMVAs
  // -----------------------------------------------------------------------------------------------------------
  TChain * aChain = new TChain(inTreeName,inTreeName); aChain->SetDirectory(0); aChain->Add(inFileName); 
  int nEntriesChain = aChain->GetEntries();
  aLOG(Log::DEBUG) <<coutRed<<" - added chain "<<coutGreen<<inTreeName<<"("<<nEntriesChain<<")"
                   <<" from "<<coutBlue<<inFileName<<coutDef<<endl;

  // indexing varible
  var_1->NewVarI(indexName);

  for(int nMLMinNow=0; nMLMinNow<nMLMsIn; nMLMinNow++) {
    TString MLMname   = aRegEval->mlmInV[nMLMinNow];  if(aRegEval->mlmSkip[MLMname]) continue;
    int     nMLMnow   = getTagNow(MLMname);    TString MLMname_e = getTagError(nMLMnow);
    TString MLMname_w = getTagWeight(nMLMnow); TString MLMname_v = getTagClsVal(nMLMnow);

    // create MLM-weight formulae for the input variables
    TString wgtStr    = getRegularStrForm(userWgtsM[MLMname+"_valid"],var_0);
    var_0->NewForm(MLMname_w,wgtStr);

    // formulae for inpput-variable errors, to be used by getRegClsErrINP()
    if(aRegEval->isErrINPv[nMLMnow]) {
      int nInErrs = (int)inNamesErr[nMLMnow].size();
      for(int nInErrNow=0; nInErrNow<nInErrs; nInErrNow++) {
        TString inVarErr = getTagInVarErr(nMLMnow,nInErrNow);

        var_0->NewForm(inVarErr,inNamesErr[nMLMnow][nInErrNow]);
      }
    }

    // create MLM, MLM-eror and MLM-weight variables for the output vars
    var_1->NewVarF(MLMname); var_1->NewVarF(MLMname_w); var_1->NewVarF(MLMname_v);
    if(aRegEval->hasErrs) { var_1->NewVarF(MLMname_e); }
  }

  // connect the input vars to the tree before looping
  // -----------------------------------------------------------------------------------------------------------
  var_0->connectTreeBranchesForm(aChain,&readerInptV);

  // extract the requested added variables and check that they exist in the input tree
  // -----------------------------------------------------------------------------------------------------------
  aRegEval->addVarV = utils->splitStringByChar(glob->GetOptC("addOutputVars"),';');

  for(int nVarsInNow=0; nVarsInNow<(int)aRegEval->addVarV.size(); nVarsInNow++) {
    TString addVarName = aRegEval->addVarV[nVarsInNow];
    VERIFY(LOCATION,(TString)"from addOutputVars - trying to use undefined variable (\""+addVarName+"\") ...",var_0->HasVar(addVarName));
  }

  // possible additional variables added to the output (do once after connectTreeBranchesForm of the input tree)
  // -----------------------------------------------------------------------------------------------------------
  vector < pair<TString,TString> > varTypeNameV;
  var_1->varStruct(var_0,&aRegEval->addVarV,NULL,&varTypeNameV);

  // create the output tree and connect it to the output vars
  // -----------------------------------------------------------------------------------------------------------
  TString outTreeName = (TString)glob->GetOptC("treeName")+glob->GetOptC("_typeANNZ");
  TString outFileName = (TString)glob->GetOptC("outDirNameFull")+outTreeName+"*.root"; // to be used later on...
  TTree   * treeOut   = new TTree(outTreeName,outTreeName); treeOut->SetDirectory(0);
  outputs->TreeMap[outTreeName] = treeOut;
  var_1->createTreeBranches(treeOut); 
  var_1->setDefaultVals();

  // -----------------------------------------------------------------------------------------------------------
  // loop on the tree
  // -----------------------------------------------------------------------------------------------------------
  aRegEval->regErrV.resize(nMLMs,vector<double>(3,0));

  bool  breakLoop(false), mayWriteObjects(false);
  int   nObjectsToWrite(glob->GetOptI("nObjectsToWrite"));
  var_0->clearCntr();
  for(Long64_t loopEntry=0; true; loopEntry++) {
    if(!var_0->getTreeEntry(loopEntry)) breakLoop = true;

    if((mayWriteObjects && var_0->GetCntr("nObj") % nObjectsToWrite == 0) || breakLoop) {
      var_0->printCntr(inTreeName); outputs->WriteOutObjects(false,true); outputs->ResetObjects();
      mayWriteObjects = false;
    }
    if(breakLoop) break;
    
    // copy current content of all common variables (index + content of aRegEval->addVarV)
    var_1->copyVarData(var_0,&varTypeNameV);

    // -----------------------------------------------------------------------------------------------------------
    // calculate the KNN errors if needed, for each variation of aRegEval->knnErrModule
    // -----------------------------------------------------------------------------------------------------------
    if(aRegEval->hasErrKNN) {
      for(map < TMVA::kNN::ModulekNN*,vector<int> >::iterator Itr=aRegEval->getErrKNN.begin(); Itr!=aRegEval->getErrKNN.end(); ++Itr) {
        getRegClsErrKNN(var_0,Itr->first,aRegEval->trgIndexV,Itr->second,false,aRegEval->regErrV);
      }
    }

    // fill the output tree
    for(int nMLMinNow=0; nMLMinNow<nMLMsIn; nMLMinNow++) {
      TString MLMname   = aRegEval->mlmInV[nMLMinNow];  if(aRegEval->mlmSkip[MLMname]) continue;
      int     nMLMnow   = getTagNow(MLMname);    TString MLMname_e = getTagError(nMLMnow);
      TString MLMname_w = getTagWeight(nMLMnow); TString MLMname_v = getTagClsVal(nMLMnow);

      double  clsPrb = getReader(var_0,ANNZ_readType::PRB,false,nMLMnow);
      double  clsVal = getReader(var_0,ANNZ_readType::CLS,true,nMLMnow);
      double  clsWgt = var_0->GetForm(MLMname_w);

      // sanity check that weights are properly defined
      if(clsWgt < 0) {
        var_0->printVars();
        VERIFY(LOCATION,(TString)"Weights can only be >= 0 ... Something is horribly wrong ?!?",false);
      }

      double  clsErr  = -1; 
      if     (aRegEval->isErrKNNv[nMLMnow]) clsErr = aRegEval->regErrV[nMLMnow][1];
      else if(aRegEval->isErrINPv[nMLMnow]) clsErr = getRegClsErrINP(var_0,false,nMLMnow,&seed);

      var_1->SetVarF(MLMname,  clsPrb); var_1->SetVarF(MLMname_v,clsVal); var_1->SetVarF(MLMname_w,clsWgt);
      if(aRegEval->hasErrs) { var_1->SetVarF(MLMname_e,clsErr); }
      // if(var_0->GetCntr("nObj") < 15) cout <<" x0x "<<clsPrb<<CT<<clsVal<<CT<<clsWgt<<endl;
    }

    var_1->fillTree();

    // to increment the loop-counter, at least one method should have passed the cuts
    mayWriteObjects = true; var_0->IncCntr("nObj"); if(var_0->GetCntr("nObj") == maxNobj) breakLoop = true;
  }
  if(!breakLoop) { var_0->printCntr(inTreeName); outputs->WriteOutObjects(false,true); outputs->ResetObjects(); }

  DELNULL(var_0); DELNULL(var_1); varTypeNameV.clear();
  DELNULL(treeOut); outputs->TreeMap.erase(outTreeName);

  // -----------------------------------------------------------------------------------------------------------
  // write the combined output to an ascii file
  // -----------------------------------------------------------------------------------------------------------
  if(doStoreToAscii) {
    TChain  * aChainCls = new TChain(outTreeName,outTreeName); aChainCls->SetDirectory(0); aChainCls->Add(outFileName); 
    nEntriesChain       = aChainCls->GetEntries();
    aLOG(Log::DEBUG) <<coutRed<<" - added chain "<<coutGreen<<outTreeName<<"("
                     <<nEntriesChain<<")"<<" from "<<coutBlue<<outFileName<<coutDef<<endl;

    TChain * aChain_toFriend = (TChain*)aChain->Clone();
    aChain_toFriend->AddFriend(aChainCls,utils->nextTreeFriendName(aChain_toFriend));

    VarMaps * var_2 = new VarMaps(glob,utils,"treeRegClsVar_2");

    for(int nMLMinNow=0; nMLMinNow<nMLMsIn; nMLMinNow++) {
      TString MLMname   = aRegEval->mlmInV[nMLMinNow];  if(aRegEval->mlmSkip[MLMname]) continue;
      int     nMLMnow   = getTagNow(MLMname);    TString MLMname_e = getTagError(nMLMnow);
      TString MLMname_w = getTagWeight(nMLMnow); TString MLMname_v = getTagClsVal(nMLMnow);

      if(optimMLMprb) { aRegEval->addVarV.push_back(MLMname); }
      else            { aRegEval->addVarV.push_back(MLMname_v); }
      aRegEval->addVarV.push_back(MLMname_w);
      if(aRegEval->hasErrs) { aRegEval->addVarV.push_back(MLMname_e); }
    }
    var_2->connectTreeBranches(aChain_toFriend);

    var_2->storeTreeToAscii("ANNZ"+glob->GetOptC("_typeANNZ"),"",0,glob->GetOptI("nObjectsToWrite"),"",&aRegEval->addVarV,NULL);

    DELNULL(var_2);
    aChain_toFriend->RemoveFriend(aChainCls); DELNULL(aChain_toFriend); DELNULL(aChainCls);
  }

  DELNULL(aChain);

  if(!glob->GetOptB("keepEvalTrees")) {
    utils->safeRM(inFileName, inLOG(Log::DEBUG));
    utils->safeRM(outFileName,inLOG(Log::DEBUG));
  }

  aLOG(Log::INFO) <<coutWhiteOnBlack<<coutGreen<<" - ending doEvalCls() ... "<<coutDef<<endl;

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
void ANNZ::evalClsWrapperSetup() {
// ===========================================================================================================
  aLOG(Log::INFO) <<coutWhiteOnBlack<<coutPurple<<" - starting ANNZ::evalClsWrapperSetup() ... "<<coutDef<<endl;
  
  evalClsSetup();

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
TString ANNZ::evalClsWrapperLoop() {
// ===========================================================================================================
  aLOG(Log::DEBUG_2) <<coutWhiteOnBlack<<coutPurple<<" - starting ANNZ::evalClsWrapperLoop() ... "<<coutDef<<endl;

  TString output("");

  VarMaps * var   = aRegEval->varWrapper;
  int     nMLMs   = glob->GetOptI("nMLMs");
  UInt_t  seed    = glob->GetOptI("initSeedRnd"); if(seed > 0) seed += 14320;
  int     nMLMsIn = (int)aRegEval->mlmInV.size();

  // -----------------------------------------------------------------------------------------------------------
  // calculation of errors (if needed)
  // -----------------------------------------------------------------------------------------------------------
  aRegEval->regErrV.resize(nMLMs,vector<double>(3,0));

  if(aRegEval->hasErrKNN) {
    for(map < TMVA::kNN::ModulekNN*,vector<int> >::iterator Itr=aRegEval->getErrKNN.begin(); Itr!=aRegEval->getErrKNN.end(); ++Itr) {
      getRegClsErrKNN(var,Itr->first,aRegEval->trgIndexV,Itr->second,false,aRegEval->regErrV);
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
  // 
  // -----------------------------------------------------------------------------------------------------------
  for(int nMLMsInNow=0; nMLMsInNow<nMLMsIn; nMLMsInNow++) {
    TString MLMname   = aRegEval->mlmInV[nMLMsInNow];  if(aRegEval->mlmSkip[MLMname]) continue;
    int     nMLMnow   = getTagNow(MLMname);    TString MLMname_e = getTagError(nMLMnow);
    TString MLMname_w = getTagWeight(nMLMnow); TString MLMname_v = getTagClsVal(nMLMnow);

    double  clsPrb = getReader(var,ANNZ_readType::PRB,false,nMLMnow);
    double  clsVal = getReader(var,ANNZ_readType::CLS,true,nMLMnow);
    double  clsWgt = var->GetForm(MLMname_w);
    double  clsErr = aRegEval->regErrV[nMLMnow][1]; 

    // sanity check that weights are properly defined
    if(clsWgt < 0) {
      var->printVars();
      VERIFY(LOCATION,(TString)"Weights can only be >= 0 ... Something is horribly wrong ?!?",false);
    }

    output += (TString)"\""+MLMname   +"\":"+utils->floatToStr(clsPrb) +",";
    output += (TString)"\""+MLMname_v +"\":"+utils->floatToStr(clsVal) +",";
    output += (TString)"\""+MLMname_w +"\":"+utils->floatToStr(clsWgt) +",";
    
    if(aRegEval->hasErrs) {
      output += (TString)"\""+MLMname_e +"\":"+utils->floatToStr(clsErr) +",";
    }
    // cout <<" x1x "<<clsPrb<<CT<<clsVal<<CT<<clsWgt<<endl;
  }

  output = (TString)"{"+output+"}";

  return output;
}

// ===========================================================================================================
/**
 * @brief    - cleanup of evaluate regression - wrapper interface.
 */
// ===========================================================================================================
void ANNZ::evalClsWrapperCleanup() {
// ===========================================================================================================
  aLOG(Log::DEBUG_1) <<coutWhiteOnBlack<<coutPurple<<" - starting ANNZ::evalClsWrapperCleanup() ... "<<coutDef<<endl;

  evalRegErrCleanup();
  
  clearReaders();
  
  DELNULL(aRegEval->loopChain);

  return;
}
