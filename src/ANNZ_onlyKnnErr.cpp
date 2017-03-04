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
 * @brief  - create and evaluate a KNN error estimator for a general dataset
 */
// ===========================================================================================================
void ANNZ::KnnErr() {
// ==================
  aLOG(Log::INFO) <<coutWhiteOnBlack<<coutGreen<<" - starting ANNZ::KnnErr() ... "<<coutDef<<endl;

  // some sanity checks
  VERIFY(LOCATION,(TString)"Configuration problem ... must set \"zReg_onlyKnnErr\"",(glob->GetOptC("zReg_onlyKnnErr") != ""));

  if     (glob->GetOptB("doGenInputTrees")) onlyKnnErr_createTreeErrKNN();
  else if(glob->GetOptB("doEval"))          onlyKnnErr_eval();

  return;
}


// ===========================================================================================================
/**
 * @brief  - create requred a KNN error reference trees for later use
 */
// ===========================================================================================================
void ANNZ::onlyKnnErr_createTreeErrKNN() {
// =======================================
    aLOG(Log::INFO) <<coutWhiteOnBlack<<coutGreen<<" - starting ANNZ::onlyKnnErr_createTreeErrKNN() ... "<<coutDef<<endl;

    int     nMLMnow    = glob->GetOptI("nMLMnow");
    TString zTrgName   = glob->GetOptC("zTrg");
    TString zRegName   = glob->GetOptC("zReg_onlyKnnErr");
    TString indexName  = glob->GetOptC("indexName");

    TString MLMname    = getTagName(nMLMnow);
    TString errKNNname = getErrKNNname(nMLMnow);
    TString MLMname_i  = getTagIndex(nMLMnow);

    // -----------------------------------------------------------------------------------------------------------
    // create a dedicated tree for the error
    // -----------------------------------------------------------------------------------------------------------
    VarMaps * var_0 = new VarMaps(glob,utils,"treeErrKNN_0");
    VarMaps * var_1 = new VarMaps(glob,utils,"treeErrKNN_1");

    TString inTreeName = (TString)glob->GetOptC("treeName")+"_full";
    TString inFileName = (TString)glob->GetOptC("inputTreeDirName")+inTreeName+"*.root";

    // prepare the chain and input variables
    TChain * aChain = new TChain(inTreeName,inTreeName); aChain->SetDirectory(0); aChain->Add(inFileName); 
    aLOG(Log::DEBUG) <<coutRed<<" - added chain "<<coutGreen<<inTreeName<<"("<<aChain->GetEntries()<<")"
                     <<" from "<<coutBlue<<inFileName<<coutDef<<endl;

    var_0->connectTreeBranches(aChain);

    var_1->NewVarF(MLMname); var_1->NewVarF(errKNNname); var_1->NewVarI(MLMname_i); var_1->NewVarF(zTrgName);

    TString outTreeName = getKeyWord("","treeErrKNN","treeErrKNNname");
    TTree   * outTree   = new TTree(outTreeName,outTreeName); outTree->SetDirectory(0); outputs->TreeMap[outTreeName] = outTree;

    var_1->createTreeBranches(outTree); 
    var_1->setDefaultVals();

    // loop on the input tree and create the output
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
      // see: http://arxiv.org/abs/0810.2991 - Estimating the Redshift Distribution of Photometric Galaxy... - Sect. 4.2
      double zTrg   = var_0->GetVarF(zTrgName);
      double regVal = var_0->GetVarF(zRegName);
      double errKNN = regVal - zTrg;
      
      var_1->SetVarF(zTrgName,  zTrg);
      var_1->SetVarF(MLMname,   regVal);
      var_1->SetVarF(errKNNname,errKNN);

      var_1->fillTree();

      var_0->IncCntr("nObj"); mayWriteObjects = true;
    }
    if(!breakLoop) { var_0->printCntr(inTreeName,Log::DEBUG); outputs->WriteOutObjects(false,true); outputs->ResetObjects(); }

    // cleanup
    DELNULL(var_0); DELNULL(var_1); DELNULL(aChain); DELNULL(outTree); outputs->TreeMap.erase(outTreeName);

    return;
}


// ===========================================================================================================
/**
 * @brief  - evaluate a KNN error estimator for a general dataset
 */
// ===========================================================================================================
void ANNZ::onlyKnnErr_eval() {
// ===========================
  aLOG(Log::INFO) <<coutWhiteOnBlack<<coutGreen<<" - starting ANNZ::onlyKnnErr_eval() ... "<<coutDef<<endl;

  int     nMLMs          = glob->GetOptI("nMLMs");
  int     nMLMnow        = glob->GetOptI("nMLMnow");
  TString zTrgName       = glob->GetOptC("zTrg");
  double  minValZ        = glob->GetOptF("minValZ");
  double  maxValZ        = glob->GetOptF("maxValZ");
  TString indexName      = glob->GetOptC("indexName");
  bool    doStoreToAscii = glob->GetOptB("doStoreToAscii");
  TString weightKNN      = glob->GetOptC("baseName_wgtKNN");
  TString zRegName       = glob->GetOptC("zReg_onlyKnnErr");
  TString knnErrCut      = glob->GetOptC("cuts_onlyKnnErr");
  TString knnErrWgt      = glob->GetOptC("weights_onlyKnnErr");
  TString knnVars        = glob->GetOptC("knnVars_onlyKnnErr");
  TString asciiPostfix   = glob->GetOptC("baseName_onlyKnnErr");
  bool    doPlots        = glob->GetOptB("doPlots_onlyKnnErr");

  TString MLMname        = getTagName(nMLMnow);
  TString errKNNname     = getErrKNNname(nMLMnow);
  TString MLMname_i      = getTagIndex(nMLMnow);
  TString MLMname_eN     = getTagError(nMLMnow,"N");  MLMname_eN.ReplaceAll(MLMname,zRegName);
  TString MLMname_e      = getTagError(nMLMnow,"");   MLMname_e .ReplaceAll(MLMname,zRegName);
  TString MLMname_eP     = getTagError(nMLMnow,"P");  MLMname_eP.ReplaceAll(MLMname,zRegName);

  // list of variables from the original tree to copy  to the output
  vector <TString> addVarV = utils->splitStringByChar(glob->GetOptC("addOutputVars"),';');

  // -----------------------------------------------------------------------------------------------------------
  // setup the variables used for the knn search
  // -----------------------------------------------------------------------------------------------------------
  inNamesVar.resize(nMLMs);
  inNamesVar[nMLMnow] = utils->splitStringByChar(knnVars,';');

  int nKnnVars = (int)inNamesVar[nMLMnow].size();
  VERIFY(LOCATION,(TString)"No input variables defined for "+MLMname+" . Something is horribly wrong ?!?",(nKnnVars > 0));

  readerInptV.clear();
  for(int nVarNow=0; nVarNow<nKnnVars; nVarNow++) {
    TString inVarNameNow = inNamesVar[nMLMnow][nVarNow];
    VERIFY(LOCATION,(TString)"Should not have empty input-variable. Something is horribly wrong... ?!?",(inVarNameNow != ""));

    bool hasVar(false);
    for(int nVarNow=0; nVarNow<(int)readerInptV.size(); nVarNow++) {
      if(readerInptV[nVarNow].first == inVarNameNow) { hasVar = true; break; }
    }
    if(hasVar) continue;

    readerInptV.push_back(pair<TString,Float_t>(inVarNameNow,0));

    aLOG(Log::DEBUG) <<coutPurple<<" -- Adding input variable "<<coutGreen<<readerInptV.size()-1<<coutPurple
                     <<" (from "<<coutGreen<<MLMname<<coutPurple<<") - \""<<coutRed<<inVarNameNow<<coutPurple<<"\""<<coutDef<<endl;
  }

  readerInptIndexV.resize(nMLMs, vector<int>(nKnnVars,0));
  for(int nReaderInputNow=0; nReaderInputNow<nKnnVars; nReaderInputNow++) {
    int     readerInptIndex = -1;
    TString inVarNameNow    = inNamesVar[nMLMnow][nReaderInputNow];

    // find the position in readerInptV of the current variable and add it
    for(int nVarNow=0; nVarNow<(int)readerInptV.size(); nVarNow++) {
      if(readerInptV[nVarNow].first == inVarNameNow) { readerInptIndex = nVarNow; break; }
    }
    VERIFY(LOCATION,(TString)"Adding reader-var which does not exist in readerInptV... "
                            +"Something is horribly wrong... ?!?",(readerInptIndex >= 0));

    readerInptIndexV[nMLMnow][nReaderInputNow] = readerInptIndex;
  }

  // -----------------------------------------------------------------------------------------------------------
  // setup the kd-tree and associated objects
  // -----------------------------------------------------------------------------------------------------------
  VarMaps * varKNN(NULL);        vector <TChain *> aChainKnn(2,NULL);   vector <int>         trgIndexV;
  TFile   * knnErrOutFile(NULL); TMVA::Factory *   knnErrFactory(NULL); TMVA::kNN::ModulekNN * knnErrModule(NULL);
  TMVA::Configurable * knnErrDataLdr(NULL);

  TString inTreeNameKnn = getKeyWord("","treeErrKNN","treeErrKNNname");
  TString inFileNameKnn = glob->GetOptC("inputTreeDirName")+inTreeNameKnn+"*.root";

  aChainKnn[0] = new TChain(inTreeNameKnn,inTreeNameKnn); aChainKnn[0]->SetDirectory(0); aChainKnn[0]->Add(inFileNameKnn);

  TString inTreeKnnFrnd = (TString)glob->GetOptC("treeName")+"_full";
  TString inFileKnnFrnd = (TString)glob->GetOptC("inputTreeDirName")+inTreeKnnFrnd+"*.root";
  aChainKnn[1] = new TChain(inTreeKnnFrnd,inTreeKnnFrnd); aChainKnn[1]->SetDirectory(0); aChainKnn[1]->Add(inFileKnnFrnd);

  aChainKnn[0]->AddFriend(aChainKnn[1],utils->nextTreeFriendName(aChainKnn[0]));

  int nEntriesChainKnn = aChainKnn[0]->GetEntries();
  aLOG(Log::DEBUG) <<coutRed<<" - Created KnnErr chain  "<<coutGreen<<inTreeNameKnn
                   <<"("<<nEntriesChainKnn<<")"<<" from "<<coutBlue<<inFileNameKnn<<coutDef<<endl;

  varKNN = new VarMaps(glob,utils,"varKNN");
  varKNN->connectTreeBranches(aChainKnn[0]);  // connect the tree so as to allocate memory for cut variables

  TCut    cutsNow = (TCut)knnErrCut;
  TString wgtReg  = (TString)((knnErrWgt == "") ? weightKNN : (TString)"("+weightKNN+")*("+knnErrWgt+")");
  wgtReg = getRegularStrForm(wgtReg,varKNN);

  setupKdTreeKNN( aChainKnn[0],knnErrOutFile,knnErrFactory,knnErrDataLdr,
                  knnErrModule,trgIndexV,nMLMnow,cutsNow,wgtReg );

  // -----------------------------------------------------------------------------------------------------------
  // define VarMaps to access the evaluated dataset and for writing the output
  // -----------------------------------------------------------------------------------------------------------
  VarMaps * var_0 = new VarMaps(glob,utils,"treeErrKNN_0");
  VarMaps * var_1 = new VarMaps(glob,utils,"treeErrKNN_1");

  TString inTreeName = (TString)glob->GetOptC("treeName")+"_full";
  TString inFileName = (TString)glob->GetOptC("outDirNameFull")+inTreeName+"*.root";

  // prepare the chain and input variables
  TChain * aChain = new TChain(inTreeName,inTreeName); aChain->SetDirectory(0); aChain->Add(inFileName); 
  aLOG(Log::DEBUG) <<coutRed<<" - added chain "<<coutGreen<<inTreeName<<"("<<aChain->GetEntries()<<")"
                   <<" from "<<coutBlue<<inFileName<<coutDef<<endl;

  var_0->connectTreeBranchesForm(aChain,&readerInptV);

  var_1->NewVarI(MLMname_i); var_1->NewVarF(MLMname_e); var_1->NewVarF(MLMname_eN); var_1->NewVarF(MLMname_eP);


  // possible additional variables added to the output (do once after connectTreeBranchesForm of the input tree)
  // -----------------------------------------------------------------------------------------------------------
  for(int nVarsInNow=0; nVarsInNow<(int)addVarV.size(); nVarsInNow++) {
    VERIFY(LOCATION,(TString)"from addOutputVars - trying to use undefined variable (\""
                             +addVarV[nVarsInNow]+"\") ...",var_0->HasVar(addVarV[nVarsInNow]));
  }

  vector < pair<TString,TString> > varTypeNameV;
  var_1->varStruct(var_0,&addVarV,NULL,&varTypeNameV);

  TString outTreeName = (TString)glob->GetOptC("treeName")+glob->GetOptC("baseName_onlyKnnErr");
  TString outFileName = (TString)glob->GetOptC("outDirNameFull")+outTreeName+"*.root"; // to be used later on...
  TTree * outTree     = new TTree(outTreeName,outTreeName); outTree->SetDirectory(0); outputs->TreeMap[outTreeName] = outTree;

  var_1->createTreeBranches(outTree); 
  var_1->setDefaultVals();

  // -----------------------------------------------------------------------------------------------------------
  // loop over the evaluated chain and derive the errors
  // -----------------------------------------------------------------------------------------------------------
  vector <int>               nMLMv(1,nMLMnow);
  vector < vector <double> > regErrV(nMLMs,vector<double>(3,0));

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

    // copy current content of all common variables (index + content of addVarV)
    var_1->copyVarData(var_0,&varTypeNameV);

    // the index variable
    var_1->SetVarI(MLMname_i,var_0->GetVarI(indexName));

    // derive the knn-errors and copy to the output tree
    getRegClsErrKNN(var_0,knnErrModule,trgIndexV,nMLMv,true,regErrV);

    var_1->SetVarF(MLMname_eN,regErrV[nMLMnow][0]);
    var_1->SetVarF(MLMname_e, regErrV[nMLMnow][1]);
    var_1->SetVarF(MLMname_eP,regErrV[nMLMnow][2]);

    // fill the tree
    var_1->fillTree();

    var_0->IncCntr("nObj"); mayWriteObjects = true;
  }
  if(!breakLoop) { var_0->printCntr(inTreeName,Log::DEBUG); outputs->WriteOutObjects(false,true); outputs->ResetObjects(); }

  // cleanup
  DELNULL(var_0); DELNULL(var_1); DELNULL(outTree); outputs->TreeMap.erase(outTreeName);


  // -----------------------------------------------------------------------------------------------------------
  // write the results to an ascii file
  // -----------------------------------------------------------------------------------------------------------
  if(doStoreToAscii || doPlots) {
    TChain  * aChainAsc = new TChain(outTreeName,outTreeName); aChainAsc->SetDirectory(0); aChainAsc->Add(outFileName); 
    int nEntriesChain   = aChainAsc->GetEntries();
    aLOG(Log::DEBUG) <<coutRed<<" - added chain "<<coutGreen<<outTreeName<<"("<<nEntriesChain<<")"<<" from "<<coutBlue<<outFileName<<coutDef<<endl;

    TChain * aChain_toFriend = (TChain*)aChain->Clone();
    aChain_toFriend->AddFriend(aChainAsc,utils->nextTreeFriendName(aChain_toFriend));

    if(doPlots) {
      vector <TString> branchNameV;
      utils->getTreeBranchNames(aChain_toFriend,branchNameV);
    
      int     hasTrgReg(0);
      TString allBranchNames("");
      for(int nBranchNow=0; nBranchNow<(int)branchNameV.size(); nBranchNow++) {
        TString branchName = branchNameV[nBranchNow];
        if(nBranchNow > 0) allBranchNames += ", ";
        allBranchNames += branchName;

        if     (branchName == zRegName) hasTrgReg++;
        else if(branchName == zTrgName) hasTrgReg++;
      }

      if(hasTrgReg != 2) {
        doPlots = false;

        aLOG(Log::WARNING) <<coutRed<<" - Found \"doPlots_onlyKnnErr\", but did not find either \"zReg_onlyKnnErr\" ("
                           <<coutBlue<<zRegName<<coutRed<<"), or \"zTrg\" ("<<coutBlue<<zTrgName<<coutRed
                           <<") in the branch list ("<<coutGreen<<allBranchNames<<coutRed<<") ---> will skip plotting..."<<coutDef<<endl;
      }
      else if(maxValZ < minValZ) {
        doPlots = false;

        aLOG(Log::WARNING) <<coutRed<<" - Found \"doPlots_onlyKnnErr\", but did not find either \"minValZ\" "
                           <<"or \"maxValZ\" ---> will skip plotting..."<<coutDef<<endl;
      }

      if(doPlots) doMetricPlots(aChain_toFriend);
      
      branchNameV.clear();
    }

    if(doStoreToAscii) {
      VarMaps * var_2 = new VarMaps(glob,utils,"aChainAsc");

      vector <TString> varInV;
      varInV.push_back(MLMname_e); varInV.push_back(MLMname_eN); varInV.push_back(MLMname_eP);
      
      // copy the content of addVarV to a new vector (will now be ordered such that the knn-error variables come first)
      for(int nVarsInNow=0; nVarsInNow<(int)addVarV.size(); nVarsInNow++) {
        if(find(varInV.begin(),varInV.end(), addVarV[nVarsInNow]) == varInV.end()) {
          varInV.push_back(addVarV[nVarsInNow]);
        }
      }

      var_2->connectTreeBranches(aChain_toFriend);

      var_2->storeTreeToAscii((TString)"ANNZ"+asciiPostfix,"",0,glob->GetOptI("nObjectsToWrite"),"",&varInV,NULL);

      // cleanup
      DELNULL(var_2); varInV.clear();
    }

    aChain_toFriend->RemoveFriend(aChainAsc); DELNULL(aChain_toFriend); DELNULL(aChainAsc);
  }

  // cleanup
  addVarV.clear(); varTypeNameV.clear(); nMLMv.clear(); regErrV.clear();

  DELNULL(aChain); DELNULL(varKNN); cleanupKdTreeKNN(knnErrOutFile,knnErrFactory,knnErrDataLdr);

  aChainKnn[0]->RemoveFriend(aChainKnn[1]); DELNULL(aChainKnn[0]); DELNULL(aChainKnn[1]);

  utils->safeRM(getKeyWord(MLMname,"knnErrXML","outFileDirKnnErr"), inLOG(Log::DEBUG));
  utils->safeRM(getKeyWord(MLMname,"knnErrXML","outFileNameKnnErr"),inLOG(Log::DEBUG));

  aChainKnn.clear(); trgIndexV.clear();

  return;
}

