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

#include "Wrapper.hpp"
#include "Utils.hpp"
#include "OptMaps.hpp"
#include "VarMaps.hpp"
#include "OutMngr.hpp"
#include "ANNZ.hpp"
#include "myANNZ.hpp"
#include "CatFormat.hpp"

// ===========================================================================================================
/**
 * @brief  - map to keep track of instances of the Wrapper class, which are
 *         managed by eg python external calls
 */
// ===========================================================================================================
map <TString, Wrapper*> WrapperRegistry::wrapper;

// ===========================================================================================================
/**
 * @brief  - external C resources, used to access class functionality from eg python
 */
// ===========================================================================================================
extern "C" {
  // -----------------------------------------------------------------------------------------------------------
  // interface to check if a Wrapper with a given name already exists
  // -----------------------------------------------------------------------------------------------------------
  bool wrapperExists(char * name) {
    return (WrapperRegistry::wrapper.find(name) != WrapperRegistry::wrapper.end());
  }

  // -----------------------------------------------------------------------------------------------------------
  // interface to initialize an instance of a new Wrapper with a given set of user-options
  // -----------------------------------------------------------------------------------------------------------
  bool wrapperNew(char * name, int argc, char ** argv) {
    if(wrapperExists(name)) return false;

    WrapperRegistry::wrapper[name] = new Wrapper(name);
    WrapperRegistry::wrapper[name]->Init(argc, argv);
    return true;    
  }

  // -----------------------------------------------------------------------------------------------------------
  // interface to delete an instance of a Wrapper
  // -----------------------------------------------------------------------------------------------------------
  void wrapperDel(char * name) {
    if(!wrapperExists(name)) return;

    DELNULL(WrapperRegistry::wrapper[name]);
    WrapperRegistry::wrapper.erase(name);

    return;
  }

  // -----------------------------------------------------------------------------------------------------------
  // interface to perform evaluation for a single event (to eg
  // be called once, or from a python loop over multiple events)
  // -----------------------------------------------------------------------------------------------------------
  char * wrapperEval(char * name, char * evalId, char * nObjsVars, char ** varNames, char ** varVals) {
    return WrapperRegistry::wrapper[name]->Eval(evalId, nObjsVars, varNames, varVals);
  }

  // -----------------------------------------------------------------------------------------------------------
  // interface to release the (dynamic memory) TString output variable which is produced by
  // a given call to Wrapper::Eval(), needed in order to keep the memory intact until the
  // output has been proccessed by eg an external python call
  // -----------------------------------------------------------------------------------------------------------
  void wrapperRelease(char * name, char * evalId) {
    WrapperRegistry::wrapper[name]->Release(evalId);
    return;
  }
}

// ===========================================================================================================
/**
 * @brief  - interface class for evaluation for external calls made with eg python
 */
// ===========================================================================================================
Wrapper::Wrapper(std::string nameIn) {
// ===================================
  aLOG(Log::INFO)<<coutWhiteOnBlack<<coutBlue<<" - starting Wrapper::Wrapper("<<nameIn<<") ... "<<coutDef<<endl;
  
  bool isNewWrapper = (WrapperRegistry::wrapper.find(nameIn) == WrapperRegistry::wrapper.end());
  VERIFY(LOCATION,(TString)"trying to instantiate multiple instances of Wrapper with name = "
                           +nameIn+" ... Something is horribly wrong !!!",isNewWrapper);
  name     = nameIn;
  aManager = NULL;
  aANNZ    = NULL;
  loopTree = NULL;
 
  return;
}
Wrapper::~Wrapper() {
// ==================
  aLOG(Log::DEBUG_1)<<coutWhiteOnBlack<<coutYellow<<" - starting Wrapper::~Wrapper("<<name<<") ... "<<coutDef<<endl;

  if(aANNZ) {
    if(glob->GetOptB("doRegression")) aANNZ->evalRegWrapperCleanup();
    else                              aANNZ->evalClsWrapperCleanup();
  }

  // release all remainig dynamic memory allocated to TString outputs
  for(map <TString,TString*>::iterator itr=registry.begin(); itr!=registry.end(); ++itr) {
    DELNULL(itr->second);
  }
  registry.clear();

  DELNULL(aANNZ);
  DELNULL(loopTree);
  DELNULL(aManager);

  return;
}
// ===========================================================================================================


// -----------------------------------------------------------------------------------------------------------
// Wrapper functions
// ===========================================================================================================
/**
 * @brief  - initialization of the Wrapper, with correspondig calls to initialize evaluation with ANNZ
 */
// ===========================================================================================================
void Wrapper::Init(int argc, char ** argv) {
// =========================================
  aLOG(Log::DEBUG_1) <<coutWhiteOnBlack<<coutPurple<<" - starting Wrapper::Init() ... "<<coutDef<<endl;
  
  aManager = new Manager();
  aManager->Init(argc, argv);

  utils   = aManager->utils;
  glob    = aManager->glob;
  outputs = aManager->outputs;

  aANNZ           = new ANNZ("aANNZ"+name,utils,glob,outputs);
  aANNZ->aRegEval = new RegEval("aRegEval"+name,utils,glob,outputs);

  // -----------------------------------------------------------------------------------------------------------
  // setup the resources needed for event-by-event evaluation, using Wrapper::Eval()
  // -----------------------------------------------------------------------------------------------------------
  VERIFY(LOCATION,(TString)"found \"doEval\" = false ... Something is horribly wrong !?!?",glob->GetOptB("doEval"));
  
  if(glob->GetOptB("doRegression")) aANNZ->evalRegWrapperSetup();
  else                              aANNZ->evalClsWrapperSetup();

  // aANNZ->aRegEval->varWrapper->printVars();
  TString treeName   = (TString)glob->GetOptC("treeName")+name;
  TString inTreeName = (TString)treeName+glob->GetOptC("evalTreeWrapperPostfix");
  loopTree = new TTree(inTreeName,inTreeName);  loopTree->SetDirectory(0);

  // -----------------------------------------------------------------------------------------------------------
  // parse the input variable list and create the corresponding tree structure
  // -----------------------------------------------------------------------------------------------------------
  TString inVars = glob->GetOptC("inVars");
  VERIFY(LOCATION,(TString)" - \"inVars\" is not set ... Something is horribly wrong !?!?",(inVars != ""));
  
  aLOG(Log::INFO) <<coutBlue<<" - will derive evaluation variables from \"inVars\" = "
                  <<coutPurple<<inVars<<coutDef<<endl;

  VarMaps   * var        = new VarMaps(glob,utils,"varCatFormat"+name);
  CatFormat * aCatFormat = new CatFormat("aCatFormat"+name,utils,glob,outputs);

  vector <TString> inVarNames, inVarTypes;
  aCatFormat->parseInputVars(var, inVars, inVarNames, inVarTypes);
  
  // create the tree from var
  var->createTreeBranches(loopTree);     

  // -----------------------------------------------------------------------------------------------------------
  // fill one entry of the tree with default values, so as to allow to getTreeEntry(0) as part of
  // connectTreeBranchesForm. connect the tree to aANNZ->aRegEval->varWrapper , with corresponding formulae
  // -----------------------------------------------------------------------------------------------------------
  loopTree->Fill();

  aANNZ->aRegEval->varWrapper->connectTreeBranchesForm(loopTree,&(aANNZ->readerInptV));

  // after filling an entry in the tree, the var and CatFormat can be cleanuped up
  // (the loopTree is now connected to aANNZ->aRegEval->varWrapper ...)
  DELNULL(var);       DELNULL(aCatFormat);
  inVarNames.clear(); inVarTypes.clear();

  aLOG(Log::DEBUG_1) <<coutWhiteOnBlack<<coutPurple<<" - finished Wrapper::Init() ... ready to evaluate! "<<coutDef<<endl;

  return;
}

// ===========================================================================================================
/**
 * @brief  - event-by-event evaluation, callable eg by python using the external wrapperEval function
 */
// ===========================================================================================================
char * Wrapper::Eval(char * evalId, char * nObjsVars, char ** varNames, char ** varVals) {
// =======================================================================================
  aLOG(Log::DEBUG_2) <<coutWhiteOnBlack<<coutPurple<<" - starting Wrapper::Eval() ... "<<coutDef<<endl;

  vector <TString> nObjsVarsV = utils->splitStringByChar((TString)nObjsVars,';');

  int nObjs = utils->strToInt(nObjsVarsV[0]);
  int nVars = utils->strToInt(nObjsVarsV[1]);

  nObjsVarsV.clear();

  TString output("");
  for(int nObjNow=0; nObjNow<nObjs; nObjNow++) {
    // reset possible previous entries in the tree
    loopTree->Reset();
    aANNZ->aRegEval->varWrapper->setDefaultVals();

    // fill in the variables connected to the tree with the current values
    for(int nVarNow=0; nVarNow<nVars; nVarNow++) {
      int nObjVarNow = nVarNow + nObjNow * nVars;
      // cout <<nObjVarNow<<CT<<nObjNow <<CT<<nVarNow<<CT<<(TString)varNames[nVarNow]<<CT<< (TString)varVals[nObjVarNow] <<endl;
      aANNZ->aRegEval->varWrapper->SetVarAuto((TString)varNames[nVarNow], (TString)varVals[nObjVarNow]);
    }

    loopTree->Fill();

    // force a calculation of the TTreeFormula (not really needed, but just in case...)
    aANNZ->aRegEval->varWrapper->getTreeEntry(0);
    
    // proceed to evaluate the object
    if(glob->GetOptB("doRegression")) output += aANNZ->evalRegWrapperLoop();
    else                              output += aANNZ->evalClsWrapperLoop();

    output += ",";
  }
  output = ((TString)"["+output+"]").ReplaceAll(" ","").ReplaceAll(",}","}").ReplaceAll(",]","]");

  TString evalIdStr(evalId);
  registry[evalIdStr] = new TString(output);

  return (char*)(registry[evalIdStr]->Data());
}

// ===========================================================================================================
/**
 * @brief  - release dynamic memory allocated to TString outputs
 */
// ===========================================================================================================
void Wrapper::Release(char * evalId) {
// ===================================
  aLOG(Log::DEBUG_2) <<coutWhiteOnBlack<<coutPurple<<" - starting Wrapper::Release("+(TString)evalId+") ... "<<coutDef<<endl;

  TString evalIdStr(evalId);
  if(registry.find(evalIdStr) != registry.end()) {
    DELNULL(registry[evalIdStr]);
    registry.erase(evalIdStr);
  }

  return;
}

