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

#include "VarMaps.hpp"

// ===========================================================================================================
VarMaps::VarMaps(OptMaps * aOptMaps, Utils * aUtils, TString aName) {
// ==================================================================
  VERIFY(LOCATION,(TString)"Can't create VarMaps without a valid OptMaps ...",(dynamic_cast<OptMaps*>(aOptMaps)));

  glob              = aOptMaps;
  utils             = aUtils;
  name              = aName;
  treeWrite         = NULL;
  treeRead          = NULL;
  nTreeInChain      = -1;
  areCutsEnabled    = true;
  needReaderUpdate  = true;
  readerFormNameKey = "ANNZ_readerFormulae_";
  failedCutType     = "";

  cntrMap           = new CntrMap(glob,utils,(TString)name+"_cntrMap");

  CT                = glob->CT;               coutDef           = glob->coutDef;
  coutRed           = glob->coutRed;          coutGreen         = glob->coutGreen;
  coutBlue          = glob->coutBlue;         coutLightBlue     = glob->coutLightBlue;
  coutYellow        = glob->coutYellow;       coutPurple        = glob->coutPurple;
  coutCyan          = glob->coutCyan;         coutUnderLine     = glob->coutUnderLine;
  coutWhiteOnBlack  = glob->coutWhiteOnBlack; coutWhiteOnRed    = glob->coutWhiteOnRed;
  coutWhiteOnGreen  = glob->coutWhiteOnGreen; coutWhiteOnYellow = glob->coutWhiteOnYellow;

  return;
}
VarMaps::~VarMaps() { 
// ==================
  clearAll();
  DELNULL(cntrMap);
}
// ===========================================================================================================

// -----------------------------------------------------------------------------------------------------------
// VarMaps functions
// ===========================================================================================================
void VarMaps::clearVar() {
// =======================  
  eraseTreeCutsPattern("");
  for(Map <TString,TTreeFormula*>::iterator itr=treeCutsFormM.begin(); itr!=treeCutsFormM.end(); ++itr) DELNULL(itr->second);
  treeCutsFormM.clear();
      
  vector <TString> varNames;
  GetAllVarNames(varNames,"B");  for(int nVarNow=0; nVarNow<(int)varNames.size(); nVarNow++) DelVarB_ (varNames[nVarNow]);
  GetAllVarNames(varNames,"C");  for(int nVarNow=0; nVarNow<(int)varNames.size(); nVarNow++) DelVarC_ (varNames[nVarNow]);
  GetAllVarNames(varNames,"S");  for(int nVarNow=0; nVarNow<(int)varNames.size(); nVarNow++) DelVarS_ (varNames[nVarNow]);
  GetAllVarNames(varNames,"I");  for(int nVarNow=0; nVarNow<(int)varNames.size(); nVarNow++) DelVarI_ (varNames[nVarNow]);
  GetAllVarNames(varNames,"L");  for(int nVarNow=0; nVarNow<(int)varNames.size(); nVarNow++) DelVarL_ (varNames[nVarNow]);
  GetAllVarNames(varNames,"US"); for(int nVarNow=0; nVarNow<(int)varNames.size(); nVarNow++) DelVarUS_(varNames[nVarNow]);
  GetAllVarNames(varNames,"UI"); for(int nVarNow=0; nVarNow<(int)varNames.size(); nVarNow++) DelVarUI_(varNames[nVarNow]);
  GetAllVarNames(varNames,"UL"); for(int nVarNow=0; nVarNow<(int)varNames.size(); nVarNow++) DelVarUL_(varNames[nVarNow]);
  GetAllVarNames(varNames,"F");  for(int nVarNow=0; nVarNow<(int)varNames.size(); nVarNow++) DelVarF_ (varNames[nVarNow]);
  GetAllVarNames(varNames,"D");  for(int nVarNow=0; nVarNow<(int)varNames.size(); nVarNow++) DelVarD_ (varNames[nVarNow]);
  varNames.clear();

  varB .clear(); varC .clear(); varS .clear(); varI.clear(); varL.clear();
  varUS.clear(); varUI.clear(); varUL.clear(); varF.clear(); varD.clear();
  hasB .clear(); hasC .clear(); hasS .clear(); hasI.clear(); hasL.clear();
  hasUS.clear(); hasUI.clear(); hasUL.clear(); hasF.clear(); hasD.clear();
  hasFM.clear();

  treeCutsM.clear(); nTreeInChain = -1; chainFriendV.clear(); nTreeFriendInChainV.clear();
  
  return;
}
// ===========================================================================================================
void VarMaps::clearTrees() {
// =========================

  if(dynamic_cast<TTree*>(treeWrite)) { treeWrite->ResetBranchAddresses(); treeWrite = NULL; }
  if(dynamic_cast<TTree*>(treeRead )) { treeRead ->ResetBranchAddresses(); treeRead  = NULL; }

  nTreeInChain = -1; chainFriendV.clear(); nTreeFriendInChainV.clear();

  return;
}


// ===========================================================================================================
void VarMaps::NewVarB(TString aName, Bool_t    input) { glob->checkName("VarMaps",aName); NewVarB_(aName,input); return; }
void VarMaps::NewVarC(TString aName, TString   input) { glob->checkName("VarMaps",aName); NewVarC_(aName,input); return; }
// ===========================================================================================================
void VarMaps::NewVarI(TString aName, Long64_t input, TString type) {
// =================================================================
  glob->checkName("VarMaps",aName);

  TString defVarSIL = glob->GetOptC("defVarSIL");
  bool    isS(false), isI(false), isL(false);
  if(type != "") { isS = (type      == "S"); isI = (type      == "I"); isL = (type      == "L"); }
  else           { isS = (defVarSIL == "S"); isI = (defVarSIL == "I"); isL = (defVarSIL == "L"); }

  if(input == DefOpts::DefL) { if(isS) input = DefOpts::DefS; else if(isI) input = DefOpts::DefI; }


  if     (isI) NewVarI_(aName,input); else if(isS) NewVarS_(aName,input);
  else if(isL) NewVarL_(aName,input); else AsrtVar(false,aName+" (NewVarI - type="+type+")");

  return;
}

// ===========================================================================================================
void VarMaps::NewVarU(TString aName, ULong64_t input, TString type) {
// ==================================================================
  glob->checkName("VarMaps",aName);

  TString defVarUSUIUL = glob->GetOptC("defVarUSUIUL");
  bool    isUS(false), isUI(false), isUL(false);
  if(type != "") { isUS = (type         == "US"); isUI = (type         == "UI"); isUL = (type         == "UL"); }
  else           { isUS = (defVarUSUIUL == "US"); isUI = (defVarUSUIUL == "UI"); isUL = (defVarUSUIUL == "UL"); }

  if(input == DefOpts::DefUL) { if(isUS) input = DefOpts::DefUS; else if(isUI) input = DefOpts::DefUI; }

  if     (isUI) NewVarUI_(aName,input); else if(isUS) NewVarUS_(aName,input);
  else if(isUL) NewVarUL_(aName,input); else AsrtVar(false,aName+" (NewVarU - type="+type+")");

  return;
}
// ===========================================================================================================
void VarMaps::NewVarF(TString aName, Double_t input, TString type) {
// =================================================================
  glob->checkName("VarMaps",aName);

  TString defVarFD = glob->GetOptC("defVarFD");
  bool    isF(false), isD(false);
  if(type != "") { isF = (type     == "F"); isD = (type     == "D"); }
  else           { isF = (defVarFD == "F"); isD = (defVarFD == "D"); }

  if(std::fabs(input - DefOpts::DefD) < EPS) { if(isF) input = DefOpts::DefF; }

  if   (isF) NewVarF_(aName,input); else if(isD) NewVarD_(aName,input);
  else AsrtVar(false,aName+" (NewVarF - type="+type+")");

  return;
}
// ===========================================================================================================
void VarMaps::NewForm(TString aName, TString input) {
// ==================================================
  glob->checkName("VarMaps",aName);

  if(input == "" || input == DefOpts::DefC) {
    VERIFY(LOCATION,(TString)"TTreeFormula is not valid (\""+aName+"\") ...",(aName != ""));
    aLOG(Log::WARNING) <<coutRed<<"initializing VarMaps::NewForm(aName=\""<<coutBlue<<aName<<coutRed<<"\", input\""<<coutBlue<<input
                       <<coutRed<<"\") by setting input=aName ... Better to explicitly define input !"<<coutDef<<endl;
    input = aName;
  }
  input = regularizeStringForm(input);

  NewForm_(aName,input);
  return;
}
// ===========================================================================================================
void VarMaps::SetVarB(TString aName, Bool_t    input) { SetVarB_(aName,input); return; }
void VarMaps::SetVarC(TString aName, TString   input) { SetVarC_(aName,input); return; }
// ===========================================================================================================
void VarMaps::SetVarI(TString aName, Long64_t  input) {
  if     (HasVarI_ (aName)) varI [aName] = input; else if(HasVarS_ (aName)) varS [aName] = input;
  else if(HasVarL_ (aName)) varL [aName] = input; else AsrtVar(false,aName+" (SetVarI)"); return;
}
// ===========================================================================================================
void VarMaps::SetVarU(TString aName, ULong64_t input) {
  if     (HasVarUI_(aName)) varUI[aName] = input; else if(HasVarUS_(aName)) varUS[aName] = input;
  else if(HasVarUL_(aName)) varUL[aName] = input; else AsrtVar(false,aName+" (SetVarU)"); return;
}
// ===========================================================================================================
void VarMaps::SetVarF(TString aName, Double_t  input) {
  if     (HasVarF_ (aName)) varF [aName] = input;
  else if(HasVarD_ (aName)) varD [aName] = input; else AsrtVar(false,aName+" (SetVarF)"); return;
}
// ===========================================================================================================
void VarMaps::SetVarB(TString aName, TString input) {
// ==================================================
  bool failed(false);

  if     (input.EqualTo("true" ,TString::kIgnoreCase))  SetVarB_(aName,true ); // format is "true"  or "TRUE"
  else if(input.EqualTo("false",TString::kIgnoreCase))  SetVarB_(aName,false); // format is "false" or "FALSE"
  else if(input.IsDigit()) {
    if     (fabs(utils->strToFloat(input)    ) < EPS)   SetVarB_(aName,false); // format is "0"
    else if(fabs(utils->strToFloat(input) - 1) < EPS)   SetVarB_(aName,true ); // format is "1"
    else failed = true;
  }
  else   failed = true;

  if(failed) {
    aLOG(Log::ERROR) <<coutWhiteOnBlack<< "!!! ERROR : " <<coutRed<< (TString)"UNKNOWN BOOL FORMAT "+input
                     <<coutWhiteOnBlack<< " [ CALLED FROM " <<LOCATION<<" ]"<<coutDef<< endl;
    assert(false);
  }
  return;
}
// ===========================================================================================================
void VarMaps::SetVarI(TString aName, TString input) {
// ==================================================
  if     (HasVarI_ (aName)) varI [aName] = utils->strToInt(input);  else if(HasVarS_ (aName)) varS [aName] = utils->strToInt (input);
  else if(HasVarL_ (aName)) varL [aName] = utils->strToLong(input); else AsrtVar(false,aName+" (SetVarI)"); return;
}
// ===========================================================================================================
void VarMaps::SetVarU(TString aName, TString input) {
// ==================================================
  if     (HasVarUI_(aName)) varUI[aName] = utils->strToUint(input);  else if(HasVarUS_(aName)) varUS[aName] = utils->strToUint (input);
  else if(HasVarUL_(aName)) varUL[aName] = utils->strToUlong(input); else AsrtVar(false,aName+" (SetVarU)"); return;
}
// ===========================================================================================================
void VarMaps::SetVarF(TString aName, TString input) {
// ==================================================
  if     (HasVarF_ (aName)) varF [aName] = utils->strToFloat(input);
  else if(HasVarD_ (aName)) varD [aName] = utils->strToDouble(input); else AsrtVar(false,aName+" (SetVarF)"); return;
}


// ===========================================================================================================
void VarMaps::SetForm(TString aName, TString input) {
// ==================================================
  VERIFY(LOCATION,(TString)"TTreeFormula is not valid (\""+input+"\") ...",(input != ""));
  input = regularizeStringForm(input);

  SetForm_(aName,input);
  return;
}
// ===========================================================================================================
Double_t VarMaps::GetForm(TString aName) {
// =======================================
  bool        hasForm = (varFormM.find(aName) != varFormM.end());
  if(hasForm) hasForm = dynamic_cast<TTreeFormula*>(varFormM[aName]);
  VERIFY(LOCATION,(TString)"Has not setup varFormM (\""+aName+"\") ...",hasForm);

  return varFormM[aName]->EvalInstance();
}

// ===========================================================================================================
void VarMaps::varStruct(VarMaps * inObj, vector <TString> * acceptV, vector <TString> * rejectV, vector < pair<TString,TString> > * varTypeNameV, bool isCopy) {
// =============================================================================================================================================================
  bool hasAccept   = dynamic_cast<vector< TString               >*>(acceptV);
  bool hasReject   = dynamic_cast<vector< TString               >*>(rejectV);
  bool hasTypeName = dynamic_cast<vector< pair<TString,TString> >*>(varTypeNameV);

  if(hasTypeName) varTypeNameV->clear();

  for(Map <TString,Bool_t>     ::iterator itr=inObj->varB .begin(); itr!=inObj->varB .end(); ++itr) {
    if(hasAccept) { if(find(acceptV->begin(),acceptV->end(),itr->first) == acceptV->end()) continue; }
    if(hasReject) { if(find(rejectV->begin(),rejectV->end(),itr->first) != rejectV->end()) continue; }
    
    if(isCopy)                               NewVarB_ (itr->first,itr->second);
    if(hasTypeName && HasVarB_ (itr->first)) varTypeNameV->push_back(pair<TString,TString>("B" ,itr->first));
  }
  for(Map <TString,TObjString*>::iterator itr=inObj->varC .begin(); itr!=inObj->varC .end(); ++itr) {
    if(hasAccept) { if(find(acceptV->begin(),acceptV->end(),itr->first) == acceptV->end()) continue; }
    if(hasReject) { if(find(rejectV->begin(),rejectV->end(),itr->first) != rejectV->end()) continue; }
    
    if(isCopy)                               NewVarC_ (itr->first,itr->second->String());
    if(hasTypeName && HasVarC_ (itr->first)) varTypeNameV->push_back(pair<TString,TString>("C" ,itr->first));
  }
  for(Map <TString,Short_t>    ::iterator itr=inObj->varS .begin(); itr!=inObj->varS .end(); ++itr) {
    if(hasAccept) { if(find(acceptV->begin(),acceptV->end(),itr->first) == acceptV->end()) continue; }
    if(hasReject) { if(find(rejectV->begin(),rejectV->end(),itr->first) != rejectV->end()) continue; }
    
    if(isCopy)                               NewVarS_ (itr->first,itr->second);
    if(hasTypeName && HasVarS_ (itr->first)) varTypeNameV->push_back(pair<TString,TString>("S" ,itr->first));
  }
  for(Map <TString,Int_t>      ::iterator itr=inObj->varI .begin(); itr!=inObj->varI .end(); ++itr) {
    if(hasAccept) { if(find(acceptV->begin(),acceptV->end(),itr->first) == acceptV->end()) continue; }
    if(hasReject) { if(find(rejectV->begin(),rejectV->end(),itr->first) != rejectV->end()) continue; }
    
    if(isCopy)                               NewVarI_ (itr->first,itr->second);
    if(hasTypeName && HasVarI_ (itr->first)) varTypeNameV->push_back(pair<TString,TString>("I" ,itr->first));
  }
  for(Map <TString,Long64_t>   ::iterator itr=inObj->varL .begin(); itr!=inObj->varL .end(); ++itr) {
    if(hasAccept) { if(find(acceptV->begin(),acceptV->end(),itr->first) == acceptV->end()) continue; }
    if(hasReject) { if(find(rejectV->begin(),rejectV->end(),itr->first) != rejectV->end()) continue; }
    
    if(isCopy)                               NewVarL_ (itr->first,itr->second);
    if(hasTypeName && HasVarL_ (itr->first)) varTypeNameV->push_back(pair<TString,TString>("L" ,itr->first));
  }
  for(Map <TString,UShort_t>   ::iterator itr=inObj->varUS.begin(); itr!=inObj->varUS.end(); ++itr) {
    if(hasAccept) { if(find(acceptV->begin(),acceptV->end(),itr->first) == acceptV->end()) continue; }
    if(hasReject) { if(find(rejectV->begin(),rejectV->end(),itr->first) != rejectV->end()) continue; }
    
    if(isCopy)                               NewVarUS_(itr->first,itr->second);
    if(hasTypeName && HasVarUS_(itr->first)) varTypeNameV->push_back(pair<TString,TString>("US",itr->first));
  }
  for(Map <TString,UInt_t>     ::iterator itr=inObj->varUI.begin(); itr!=inObj->varUI.end(); ++itr) {
    if(hasAccept) { if(find(acceptV->begin(),acceptV->end(),itr->first) == acceptV->end()) continue; }
    if(hasReject) { if(find(rejectV->begin(),rejectV->end(),itr->first) != rejectV->end()) continue; }
    
    if(isCopy)                               NewVarUI_(itr->first,itr->second);
    if(hasTypeName && HasVarUI_(itr->first)) varTypeNameV->push_back(pair<TString,TString>("UI",itr->first));
  }
  for(Map <TString,ULong64_t>  ::iterator itr=inObj->varUL.begin(); itr!=inObj->varUL.end(); ++itr) {
    if(hasAccept) { if(find(acceptV->begin(),acceptV->end(),itr->first) == acceptV->end()) continue; }
    if(hasReject) { if(find(rejectV->begin(),rejectV->end(),itr->first) != rejectV->end()) continue; }
    
    if(isCopy)                               NewVarUL_(itr->first,itr->second);
    if(hasTypeName && HasVarUL_(itr->first)) varTypeNameV->push_back(pair<TString,TString>("UL",itr->first));
  }
  for(Map <TString,Float_t>    ::iterator itr=inObj->varF .begin(); itr!=inObj->varF .end(); ++itr) {
    if(hasAccept) { if(find(acceptV->begin(),acceptV->end(),itr->first) == acceptV->end()) continue; }
    if(hasReject) { if(find(rejectV->begin(),rejectV->end(),itr->first) != rejectV->end()) continue; }
    
    if(isCopy)                               NewVarF_ (itr->first,itr->second);
    if(hasTypeName && HasVarF_ (itr->first)) varTypeNameV->push_back(pair<TString,TString>("F" ,itr->first));
  }
  for(Map <TString,Double_t>   ::iterator itr=inObj->varD .begin(); itr!=inObj->varD .end(); ++itr) {
    if(hasAccept) { if(find(acceptV->begin(),acceptV->end(),itr->first) == acceptV->end()) continue; }
    if(hasReject) { if(find(rejectV->begin(),rejectV->end(),itr->first) != rejectV->end()) continue; }
    
    if(isCopy)                               NewVarD_ (itr->first,itr->second);
    if(hasTypeName && HasVarD_ (itr->first)) varTypeNameV->push_back(pair<TString,TString>("D" ,itr->first));
  }
  for(Map <TString,TString>    ::iterator itr=inObj->varFM.begin(); itr!=inObj->varFM.end(); ++itr) {
    if(hasAccept) { if(find(acceptV->begin(),acceptV->end(),itr->first) == acceptV->end()) continue; }
    if(hasReject) { if(find(rejectV->begin(),rejectV->end(),itr->first) != rejectV->end()) continue; }
    
    if(isCopy)                               NewForm_ (itr->first,itr->second);
    if(hasTypeName && HasForm_ (itr->first)) varTypeNameV->push_back(pair<TString,TString>("FM",itr->first));
  }

  return;    
}

// ===========================================================================================================
void VarMaps::copyVarData(VarMaps * inObj, vector < pair<TString,TString> > * varTypeNameV) {
// ==========================================================================================
  
  if(dynamic_cast<vector< pair<TString,TString> >*>(varTypeNameV)) {
    vector < pair<TString,TString> >::iterator itr, itrEnd;
    for(itr=varTypeNameV->begin(), itrEnd=varTypeNameV->end(); itr!=itrEnd; ++itr) {
      if     (itr->first == "F" ) { SetVarF_ ( itr->second , inObj->GetVarF(itr->second) ); }
      else if(itr->first == "I" ) { SetVarI_ ( itr->second , inObj->GetVarI(itr->second) ); }
      else if(itr->first == "B" ) { SetVarB_ ( itr->second , inObj->GetVarB(itr->second) ); }
      else if(itr->first == "C" ) { SetVarC_ ( itr->second , inObj->GetVarC(itr->second) ); }
      else if(itr->first == "D" ) { SetVarD_ ( itr->second , inObj->GetVarF(itr->second) ); }
      else if(itr->first == "FM") { SetForm_ ( itr->second , inObj->GetForm(itr->second) ); }
      else if(itr->first == "S" ) { SetVarS_ ( itr->second , inObj->GetVarI(itr->second) ); }
      else if(itr->first == "L" ) { SetVarL_ ( itr->second , inObj->GetVarI(itr->second) ); }
      else if(itr->first == "US") { SetVarUS_( itr->second , inObj->GetVarU(itr->second) ); }
      else if(itr->first == "UI") { SetVarUI_( itr->second , inObj->GetVarU(itr->second) ); }
      else if(itr->first == "UL") { SetVarUL_( itr->second , inObj->GetVarU(itr->second) ); }
    }
  }
  else {
    for(Map <TString,Bool_t>     ::iterator itr=inObj->varB .begin(); itr!=inObj->varB .end(); ++itr) { if(HasVarB_ (itr->first)) SetVarB_ (itr->first,itr->second,false); }
    for(Map <TString,TObjString*>::iterator itr=inObj->varC .begin(); itr!=inObj->varC .end(); ++itr) { if(HasVarC_ (itr->first)) SetVarC_ (itr->first,itr->second->String(),false); }
    for(Map <TString,Short_t>    ::iterator itr=inObj->varS .begin(); itr!=inObj->varS .end(); ++itr) { if(HasVarS_ (itr->first)) SetVarS_ (itr->first,itr->second,false); }
    for(Map <TString,Int_t>      ::iterator itr=inObj->varI .begin(); itr!=inObj->varI .end(); ++itr) { if(HasVarI_ (itr->first)) SetVarI_ (itr->first,itr->second,false); }
    for(Map <TString,Long64_t>   ::iterator itr=inObj->varL .begin(); itr!=inObj->varL .end(); ++itr) { if(HasVarL_ (itr->first)) SetVarL_ (itr->first,itr->second,false); }
    for(Map <TString,UShort_t>   ::iterator itr=inObj->varUS.begin(); itr!=inObj->varUS.end(); ++itr) { if(HasVarUS_(itr->first)) SetVarUS_(itr->first,itr->second,false); }
    for(Map <TString,UInt_t>     ::iterator itr=inObj->varUI.begin(); itr!=inObj->varUI.end(); ++itr) { if(HasVarUI_(itr->first)) SetVarUI_(itr->first,itr->second,false); }
    for(Map <TString,ULong64_t>  ::iterator itr=inObj->varUL.begin(); itr!=inObj->varUL.end(); ++itr) { if(HasVarUL_(itr->first)) SetVarUL_(itr->first,itr->second,false); }
    for(Map <TString,Float_t>    ::iterator itr=inObj->varF .begin(); itr!=inObj->varF .end(); ++itr) { if(HasVarF_ (itr->first)) SetVarF_ (itr->first,itr->second,false); }
    for(Map <TString,Double_t>   ::iterator itr=inObj->varD .begin(); itr!=inObj->varD .end(); ++itr) { if(HasVarD_ (itr->first)) SetVarD_ (itr->first,itr->second,false); }
    for(Map <TString,TString>    ::iterator itr=inObj->varFM.begin(); itr!=inObj->varFM.end(); ++itr) { if(HasForm_ (itr->first)) SetForm_ (itr->first,itr->second,false); }
  }

  return;    
}


// ===========================================================================================================
void VarMaps::setDefaultVals(vector < pair<TString,TString> > * varTypeNameV) {
// ============================================================================

  if(dynamic_cast<vector< pair<TString,TString> >*>(varTypeNameV)) {
    vector < pair<TString,TString> >::iterator itr, itrEnd;
    for(itr=varTypeNameV->begin(), itrEnd=varTypeNameV->end(); itr!=itrEnd; ++itr) {
      if     (itr->first == "F" ) { SetVarF_ ( itr->second , DefOpts::DefF  ); }
      else if(itr->first == "I" ) { SetVarI_ ( itr->second , DefOpts::DefI  ); }
      else if(itr->first == "B" ) { SetVarB_ ( itr->second , DefOpts::DefB  ); }
      else if(itr->first == "C" ) { SetVarC_ ( itr->second , DefOpts::DefC  ); }
      else if(itr->first == "D" ) { SetVarD_ ( itr->second , DefOpts::DefD  ); }
      else if(itr->first == "S" ) { SetVarS_ ( itr->second , DefOpts::DefS  ); }
      else if(itr->first == "L" ) { SetVarL_ ( itr->second , DefOpts::DefL  ); }
      else if(itr->first == "US") { SetVarUS_( itr->second , DefOpts::DefUS ); }
      else if(itr->first == "UI") { SetVarUI_( itr->second , DefOpts::DefUI ); }
      else if(itr->first == "UL") { SetVarUL_( itr->second , DefOpts::DefUL ); }
    }
  }
  else {
    for(Map <TString,Bool_t>     ::iterator itr=varB .begin(); itr!=varB .end(); ++itr) { SetVarB_ (itr->first , DefOpts::DefB ); }
    for(Map <TString,TObjString*>::iterator itr=varC .begin(); itr!=varC .end(); ++itr) { SetVarC_ (itr->first , DefOpts::DefC ); }
    for(Map <TString,Short_t>    ::iterator itr=varS .begin(); itr!=varS .end(); ++itr) { SetVarS_ (itr->first , DefOpts::DefS ); }
    for(Map <TString,Int_t>      ::iterator itr=varI .begin(); itr!=varI .end(); ++itr) { SetVarI_ (itr->first , DefOpts::DefI ); }
    for(Map <TString,Long64_t>   ::iterator itr=varL .begin(); itr!=varL .end(); ++itr) { SetVarL_ (itr->first , DefOpts::DefL ); }
    for(Map <TString,UShort_t>   ::iterator itr=varUS.begin(); itr!=varUS.end(); ++itr) { SetVarUS_(itr->first , DefOpts::DefUS); }
    for(Map <TString,UInt_t>     ::iterator itr=varUI.begin(); itr!=varUI.end(); ++itr) { SetVarUI_(itr->first , DefOpts::DefUI); }
    for(Map <TString,ULong64_t>  ::iterator itr=varUL.begin(); itr!=varUL.end(); ++itr) { SetVarUL_(itr->first , DefOpts::DefUL); }
    for(Map <TString,Float_t>    ::iterator itr=varF .begin(); itr!=varF .end(); ++itr) { SetVarF_ (itr->first , DefOpts::DefF ); }
    for(Map <TString,Double_t>   ::iterator itr=varD .begin(); itr!=varD .end(); ++itr) { SetVarD_ (itr->first , DefOpts::DefD ); }
  }

  return;
}

// ===========================================================================================================
void VarMaps::printVars(int nPrintRow, int width) {
// ================================================
  glob->printMap(varB ,TString::Format(name+": varB (%d)",(int)varB .size()),nPrintRow,width);
  glob->printMap(varC ,TString::Format(name+": varC (%d)",(int)varC .size()),nPrintRow,width);
  glob->printMap(varS ,TString::Format(name+": varS (%d)",(int)varS .size()),nPrintRow,width);
  glob->printMap(varI ,TString::Format(name+": varI (%d)",(int)varI .size()),nPrintRow,width);
  glob->printMap(varL ,TString::Format(name+": varL (%d)",(int)varL .size()),nPrintRow,width);
  glob->printMap(varUS,TString::Format(name+": varUS(%d)",(int)varUS.size()),nPrintRow,width);
  glob->printMap(varUI,TString::Format(name+": varUI(%d)",(int)varUI.size()),nPrintRow,width);
  glob->printMap(varUL,TString::Format(name+": varUL(%d)",(int)varUL.size()),nPrintRow,width);
  glob->printMap(varF ,TString::Format(name+": varF (%d)",(int)varF .size()),nPrintRow,width);
  glob->printMap(varD ,TString::Format(name+": varD (%d)",(int)varD .size()),nPrintRow,width);

  Map <TString,Double_t> tmpM;
  for(Map <TString,TString>::iterator itr=varFM.begin(); itr!=varFM.end(); ++itr) {
    TString nameNow = (TString)"("+itr->first+") "+itr->second;
    double  valNaw  = GetForm(itr->first);
    tmpM[nameNow]   = valNaw;
  }
  glob->printMap(tmpM,TString::Format(name+": varFM(%d)",(int)tmpM.size()),nPrintRow,width);
  tmpM.clear();

  return ;
};

// ===========================================================================================================
void VarMaps::rmVarPattern(TString pattern, TString type) {
// ========================================================
  vector <TString> varNames;

  if     (type == "I") {
    for(Map <TString,Short_t>    ::iterator itr=varS .begin(); itr!=varS .end(); ++itr) if((itr->first).Contains(pattern)) varNames.push_back(itr->first);
    for(Map <TString,Int_t>      ::iterator itr=varI .begin(); itr!=varI .end(); ++itr) if((itr->first).Contains(pattern)) varNames.push_back(itr->first);
    for(Map <TString,Long64_t>   ::iterator itr=varL .begin(); itr!=varL .end(); ++itr) if((itr->first).Contains(pattern)) varNames.push_back(itr->first);
  }
  else if(type == "F") {
    for(Map <TString,Float_t>    ::iterator itr=varF .begin(); itr!=varF .end(); ++itr) if((itr->first).Contains(pattern)) varNames.push_back(itr->first);
    for(Map <TString,Double_t>   ::iterator itr=varD .begin(); itr!=varD .end(); ++itr) if((itr->first).Contains(pattern)) varNames.push_back(itr->first);
  }
  else if(type == "B" ) { for(Map <TString,Bool_t>     ::iterator itr=varB .begin(); itr!=varB .end(); ++itr) if((itr->first).Contains(pattern)) varNames.push_back(itr->first); }
  else if(type == "C" ) { for(Map <TString,TObjString*>::iterator itr=varC .begin(); itr!=varC .end(); ++itr) if((itr->first).Contains(pattern)) varNames.push_back(itr->first); }
  else if(type == "U") {
    for(Map <TString,UShort_t>   ::iterator itr=varUS.begin(); itr!=varUS.end(); ++itr) if((itr->first).Contains(pattern)) varNames.push_back(itr->first);
    for(Map <TString,UInt_t>     ::iterator itr=varUI.begin(); itr!=varUI.end(); ++itr) if((itr->first).Contains(pattern)) varNames.push_back(itr->first);
    for(Map <TString,ULong64_t>  ::iterator itr=varUL.begin(); itr!=varUL.end(); ++itr) if((itr->first).Contains(pattern)) varNames.push_back(itr->first);
  }
  
  for(int nVarNameNow=0; nVarNameNow<(int)varNames.size(); nVarNameNow++) {
    if     (type == "I") { DelVarS_ (varNames[nVarNameNow]); DelVarI_ (varNames[nVarNameNow]); DelVarL_ (varNames[nVarNameNow]); }
    else if(type == "F") { DelVarF_ (varNames[nVarNameNow]); DelVarD_ (varNames[nVarNameNow]);                                   }
    else if(type == "B")   DelVarB_ (varNames[nVarNameNow]);
    else if(type == "C")   DelVarC_ (varNames[nVarNameNow]);
    else if(type == "U") { DelVarUS_(varNames[nVarNameNow]); DelVarUI_(varNames[nVarNameNow]); DelVarUL_(varNames[nVarNameNow]); }
  }
  
  varNames.clear();
  return;    
}

// ===========================================================================================================
void VarMaps::getVarPattern(TString type, vector <TString> & optV, TString pattern, bool ignorCase) {
// ==================================================================================================
  
  if     (type == "I" ) glob->getElePattern(varI ,optV,pattern,ignorCase); else if(type == "F" ) glob->getElePattern(varF ,optV,pattern,ignorCase);
  else if(type == "B" ) glob->getElePattern(varB ,optV,pattern,ignorCase); else if(type == "C" ) glob->getElePattern(varC ,optV,pattern,ignorCase);
  else if(type == "D" ) glob->getElePattern(varD ,optV,pattern,ignorCase); else if(type == "S" ) glob->getElePattern(varS ,optV,pattern,ignorCase); 
  else if(type == "L" ) glob->getElePattern(varL ,optV,pattern,ignorCase); else if(type == "US") glob->getElePattern(varUS,optV,pattern,ignorCase);
  else if(type == "UI") glob->getElePattern(varUI,optV,pattern,ignorCase); else if(type == "UL") glob->getElePattern(varUL,optV,pattern,ignorCase);  
  else assert(false);

  return;
}

// ===========================================================================================================
void VarMaps::GetAllVarNames(vector <TString> & varNames, TString type) {
// ======================================================================
  varNames.clear();
  if(type == "ALL" || type == "B" ) { for(Map <TString,Bool_t>     ::iterator itr=varB .begin(); itr!=varB .end(); ++itr) varNames.push_back(itr->first); }
  if(type == "ALL" || type == "C" ) { for(Map <TString,TObjString*>::iterator itr=varC .begin(); itr!=varC .end(); ++itr) varNames.push_back(itr->first); }
  if(type == "ALL" || type == "S" ) { for(Map <TString,Short_t>    ::iterator itr=varS .begin(); itr!=varS .end(); ++itr) varNames.push_back(itr->first); }
  if(type == "ALL" || type == "I" ) { for(Map <TString,Int_t>      ::iterator itr=varI .begin(); itr!=varI .end(); ++itr) varNames.push_back(itr->first); }
  if(type == "ALL" || type == "L" ) { for(Map <TString,Long64_t>   ::iterator itr=varL .begin(); itr!=varL .end(); ++itr) varNames.push_back(itr->first); }
  if(type == "ALL" || type == "US") { for(Map <TString,UShort_t>   ::iterator itr=varUS.begin(); itr!=varUS.end(); ++itr) varNames.push_back(itr->first); }
  if(type == "ALL" || type == "UI") { for(Map <TString,UInt_t>     ::iterator itr=varUI.begin(); itr!=varUI.end(); ++itr) varNames.push_back(itr->first); }
  if(type == "ALL" || type == "UL") { for(Map <TString,ULong64_t>  ::iterator itr=varUL.begin(); itr!=varUL.end(); ++itr) varNames.push_back(itr->first); }
  if(type == "ALL" || type == "F" ) { for(Map <TString,Float_t>    ::iterator itr=varF .begin(); itr!=varF .end(); ++itr) varNames.push_back(itr->first); }
  if(type == "ALL" || type == "D" ) { for(Map <TString,Double_t>   ::iterator itr=varD .begin(); itr!=varD .end(); ++itr) varNames.push_back(itr->first); }
  return;    
}

// ===========================================================================================================
void VarMaps::GetAllVarNameTypes(vector < pair<TString,TString> > & varTypeNameV, TString type) {
// ==============================================================================================
  TString typeNow("");

  varTypeNameV.clear();

  typeNow = "B";  if(type == "ALL" || type == typeNow) {
    for(Map <TString,Bool_t>     ::iterator itr=varB .begin(); itr!=varB .end(); ++itr) varTypeNameV.push_back(pair<TString,TString>(typeNow,itr->first));
  }
  typeNow = "C";  if(type == "ALL" || type == typeNow) {
    for(Map <TString,TObjString*>::iterator itr=varC .begin(); itr!=varC .end(); ++itr) varTypeNameV.push_back(pair<TString,TString>(typeNow,itr->first));
  }
  typeNow = "S";  if(type == "ALL" || type == typeNow) {
    for(Map <TString,Short_t>    ::iterator itr=varS .begin(); itr!=varS .end(); ++itr) varTypeNameV.push_back(pair<TString,TString>(typeNow,itr->first));
  }
  typeNow = "I";  if(type == "ALL" || type == typeNow) {
    for(Map <TString,Int_t>      ::iterator itr=varI .begin(); itr!=varI .end(); ++itr) varTypeNameV.push_back(pair<TString,TString>(typeNow,itr->first));
  }
  typeNow = "L";  if(type == "ALL" || type == typeNow) {
    for(Map <TString,Long64_t>   ::iterator itr=varL .begin(); itr!=varL .end(); ++itr) varTypeNameV.push_back(pair<TString,TString>(typeNow,itr->first));
  }
  typeNow = "US"; if(type == "ALL" || type == typeNow) {
    for(Map <TString,UShort_t>   ::iterator itr=varUS.begin(); itr!=varUS.end(); ++itr) varTypeNameV.push_back(pair<TString,TString>(typeNow,itr->first));
  }
  typeNow = "UI"; if(type == "ALL" || type == typeNow) {
    for(Map <TString,UInt_t>     ::iterator itr=varUI.begin(); itr!=varUI.end(); ++itr) varTypeNameV.push_back(pair<TString,TString>(typeNow,itr->first));
  }
  typeNow = "UL"; if(type == "ALL" || type == typeNow) {
    for(Map <TString,ULong64_t>  ::iterator itr=varUL.begin(); itr!=varUL.end(); ++itr) varTypeNameV.push_back(pair<TString,TString>(typeNow,itr->first));
  }
  typeNow = "F";  if(type == "ALL" || type == typeNow) {
    for(Map <TString,Float_t>    ::iterator itr=varF .begin(); itr!=varF .end(); ++itr) varTypeNameV.push_back(pair<TString,TString>(typeNow,itr->first));
  }
  typeNow = "D";  if(type == "ALL" || type == typeNow) {
    for(Map <TString,Double_t>   ::iterator itr=varD .begin(); itr!=varD .end(); ++itr) varTypeNameV.push_back(pair<TString,TString>(typeNow,itr->first));
  }
  return;    
}

// ===========================================================================================================
void VarMaps::printVarNames(TString type, TString title) {
// =======================================================
  aCleanLOG() <<coutWhiteOnBlack<<coutCyan<<"Now starting printVarNames(type="<<type<<" , title="<<title<<") ... "<<coutDef<<endl;

  if(type == "ALL" || type == "B" ) {
    int nVar(0); aCleanLOG() <<coutBlue<<title<<" varB - "<<coutGreen;
    for(Map <TString,Bool_t>     ::iterator itr=varB .begin(); itr!=varB .end(); ++itr) { aCleanLOG()<<" ("<<nVar<<","<<itr->first<<")"; nVar++; }
    aCleanLOG()<<coutDef<<endl;
  }
  if(type == "ALL" || type == "C" ) {
    int nVar(0); aCleanLOG() <<coutBlue<<title<<" varC  - "<<coutGreen;
    for(Map <TString,TObjString*>::iterator itr=varC .begin(); itr!=varC .end(); ++itr) { aCleanLOG()<<" ("<<nVar<<","<<itr->first<<")"; nVar++; }
    aCleanLOG()<<coutDef<<endl;
  }
  if(type == "ALL" || type == "S" ) {
    int nVar(0); aCleanLOG() <<coutBlue<<title<<" varS  - "<<coutGreen;
    for(Map <TString,Short_t>    ::iterator itr=varS .begin(); itr!=varS .end(); ++itr) { aCleanLOG()<<" ("<<nVar<<","<<itr->first<<")"; nVar++; }
    aCleanLOG()<<coutDef<<endl;
  }
  if(type == "ALL" || type == "I" ) {
    int nVar(0); aCleanLOG() <<coutBlue<<title<<" varI  - "<<coutGreen;
    for(Map <TString,Int_t>      ::iterator itr=varI .begin(); itr!=varI .end(); ++itr) { aCleanLOG()<<" ("<<nVar<<","<<itr->first<<")"; nVar++; }
    aCleanLOG()<<coutDef<<endl;
  }
  if(type == "ALL" || type == "L" ) {
    int nVar(0); aCleanLOG() <<coutBlue<<title<<" varL  - "<<coutGreen;
    for(Map <TString,Long64_t>   ::iterator itr=varL .begin(); itr!=varL .end(); ++itr) { aCleanLOG()<<" ("<<nVar<<","<<itr->first<<")"; nVar++; }
    aCleanLOG()<<coutDef<<endl;
  }
  if(type == "ALL" || type == "US") {
    int nVar(0); aCleanLOG() <<coutBlue<<title<<" varUS - "<<coutGreen;
    for(Map <TString,UShort_t>   ::iterator itr=varUS.begin(); itr!=varUS.end(); ++itr) { aCleanLOG()<<" ("<<nVar<<","<<itr->first<<")"; nVar++; }
    aCleanLOG()<<coutDef<<endl;
  }
  if(type == "ALL" || type == "UI") {
    int nVar(0); aCleanLOG() <<coutBlue<<title<<" varUI - "<<coutGreen;
    for(Map <TString,UInt_t>     ::iterator itr=varUI.begin(); itr!=varUI.end(); ++itr) { aCleanLOG()<<" ("<<nVar<<","<<itr->first<<")"; nVar++; }
    aCleanLOG()<<coutDef<<endl;
  }
  if(type == "ALL" || type == "UL") {
    int nVar(0); aCleanLOG() <<coutBlue<<title<<" varUL - "<<coutGreen;
    for(Map <TString,ULong64_t>  ::iterator itr=varUL.begin(); itr!=varUL.end(); ++itr) { aCleanLOG()<<" ("<<nVar<<","<<itr->first<<")"; nVar++; }
    aCleanLOG()<<coutDef<<endl;
  }
  if(type == "ALL" || type == "F" ) {
    int nVar(0); aCleanLOG() <<coutBlue<<title<<" varF  - "<<coutGreen;
    for(Map <TString,Float_t>    ::iterator itr=varF .begin(); itr!=varF .end(); ++itr) { aCleanLOG()<<" ("<<nVar<<","<<itr->first<<")"; nVar++; }
    aCleanLOG()<<coutDef<<endl;
  }
  if(type == "ALL" || type == "D" ) {
    int nVar(0); aCleanLOG() <<coutBlue<<title<<" varD  - "<<coutGreen;
    for(Map <TString,Double_t>   ::iterator itr=varD .begin(); itr!=varD .end(); ++itr) { aCleanLOG()<<" ("<<nVar<<","<<itr->first<<")"; nVar++; }
    aCleanLOG()<<coutDef<<endl;
  }

  return;    
}

// ===========================================================================================================
void VarMaps::setTreeWrite(TTree * tree) { 
// =======================================
  VERIFY(LOCATION,(TString)"trying to use setTreeWrite() with undefined tree ...",(dynamic_cast<TTree*>(tree)));
  treeWrite = tree;
  return;
}

// ===========================================================================================================
void VarMaps::setTreeRead(TTree * tree) { 
  // ====================================
  VERIFY(LOCATION,(TString)"trying to use setTreeRead() with undefined tree ...",(dynamic_cast<TTree*>(tree)));
  treeRead     = tree;
  nTreeInChain = -1;
  chainFriendV.clear(); nTreeFriendInChainV.clear();
  return;
}

// ===========================================================================================================
void VarMaps::fillTree() { 
// =======================
  VERIFY(LOCATION,(TString)"trying to use fillTree() with undefined tree ...",(dynamic_cast<TTree*>(treeWrite)));
  treeWrite->Fill();
  return;
}


// ===========================================================================================================
void VarMaps::eraseTreeCutsPattern(TString cutPattern, bool ignorCase) {
// =====================================================================
  vector <TString> eraseEle;
  for(Map <TString,TString>::iterator itr=treeCutsM.begin(); itr!=treeCutsM.end(); ++itr) {
    bool  hasEle = (ignorCase) ? ((TString)itr->second).Contains(cutPattern,TString::kIgnoreCase)
                               : ((TString)itr->second).Contains(cutPattern);

    if(hasEle || cutPattern == "") eraseEle.push_back(itr->first);
  }

  for(int nEraseEleNow=0; nEraseEleNow<(int)eraseEle.size(); nEraseEleNow++) { treeCutsM.erase(eraseEle[nEraseEleNow]); }
  
  eraseEle.clear();
  return;
}
// ===========================================================================================================
int VarMaps::replaceTreeCut(TString oldCut, TString newCut) {
// ==========================================================
  TString cutExpr("");
  int     foundOldCut(0);

  oldCut = regularizeStringForm(oldCut);
  newCut = regularizeStringForm(newCut);

  for(Map <TString,TString>::iterator itr=treeCutsM.begin(); itr!=treeCutsM.end(); ++itr) {
    if(!(((TString)itr->second).Contains(oldCut))) continue;
    
    cutExpr     = itr->second;   cutExpr.ReplaceAll(oldCut,newCut);
    itr->second = (TCut)cutExpr;
    foundOldCut++;
  }

  return foundOldCut;
}
// ===========================================================================================================
void VarMaps::addTreeCuts(TString cutType, TCut aCut) {
// ====================================================
  TString cutStr0 = treeCutsM[cutType];  cutStr0 = (regularizeStringForm(cutStr0)).ReplaceAll(" ","");
  TString cutStr1 = (TString)aCut;       cutStr1 = (regularizeStringForm(cutStr1)).ReplaceAll(" ","");
  
  if(cutStr1 != "" && !cutStr0.Contains(cutStr1)) {
    treeCutsM[cutType] = (TString)( (TCut)(treeCutsM[cutType]) + (TCut)(aCut) );
  }

  printCut(cutType);
  return;
}
// ===========================================================================================================
void VarMaps::setTreeCuts(TString cutType, TCut aCut) {
// ====================================================
  treeCutsM[cutType] = regularizeStringForm((TString)aCut);
  printCut(cutType);
  return;
}
// ===========================================================================================================
void VarMaps::getTreeCutsM(map <TString,TCut> & aTreeCutsM) {
// ==========================================================
  aTreeCutsM.clear();
  for(Map <TString,TString>::iterator itr=treeCutsM.begin(); itr!=treeCutsM.end(); ++itr) {
    aTreeCutsM[itr->first] = (TCut)itr->second;
  }
  return;
}
// ===========================================================================================================
void VarMaps::setTreeCutsM(map <TString,TCut> & aTreeCutsM) {
// ==========================================================
  treeCutsM.clear();
  for(map <TString,TCut>::iterator itr=aTreeCutsM.begin(); itr!=aTreeCutsM.end(); ++itr) {
    treeCutsM[itr->first] = regularizeStringForm((TString)itr->second);
  }
  return;
}
// ===========================================================================================================
TCut VarMaps::getTreeCuts(TString cutType) {
// =========================================
  TCut aCut("");
  if(cutType == "") { for(Map <TString,TString>::iterator itr=treeCutsM.begin(); itr!=treeCutsM.end(); ++itr) aCut += (TCut)itr->second; }
  else              { assert(treeCutsM.find(cutType) != treeCutsM.end()); aCut = (TCut)treeCutsM[cutType]; }
  return aCut;
}
// ===========================================================================================================
bool VarMaps::hasFailedTreeCuts(vector<TString> & cutTypeV) {
// ==========================================================
  if(!areCutsEnabled) return false;

  for(int nCutTypeNow=0; nCutTypeNow<(int)cutTypeV.size(); nCutTypeNow++) {
    TString cutType = cutTypeV[nCutTypeNow];

    bool        hasForm = (treeCutsFormM.find(cutType) != treeCutsFormM.end());
    if(hasForm) hasForm = dynamic_cast<TTreeFormula*>(treeCutsFormM[cutType]);
    VERIFY(LOCATION,(TString)"Has not setup treeCutsFormM (\""+cutType+"\") ...",hasForm);

    if(treeCutsFormM[cutType]->EvalInstance() < 0.5)  { 
      failedCutType = (TString)cutType+" [ "+treeCutsM[cutType]+" ]";
      IncCntr((TString)"failedCut: "+failedCutType);
      return true;
    }
  }
      
  return false;
}
// ===========================================================================================================
// accept e.g., [cutType="ANNZ_0"] or [cutType="ANNZ_0;_valid"] , which is
// split into multiple tests by ";" chars
// ===========================================================================================================
bool VarMaps::hasFailedTreeCuts(TString cutType) {
// ===============================================
  if(!areCutsEnabled) return false;

  vector<TString> cutsV = utils->splitStringByChar(cutType,';');

  for(int nCutTypeNow=0; nCutTypeNow<(int)cutsV.size(); nCutTypeNow++) {
    TString cutTypeNow = cutsV[nCutTypeNow];

    bool        hasForm = (treeCutsFormM.find(cutTypeNow) != treeCutsFormM.end());
    if(hasForm) hasForm = dynamic_cast<TTreeFormula*>(treeCutsFormM[cutTypeNow]);
    VERIFY(LOCATION,(TString)"Has not setup treeCutsFormM (\""+cutTypeNow+"\") ...",hasForm);

    if(treeCutsFormM[cutTypeNow]->EvalInstance() < 0.5)  { 
      failedCutType = (TString)cutTypeNow+" [ "+treeCutsM[cutTypeNow]+" ]";
      IncCntr((TString)"failedCut: "+failedCutType);
      return true;
    }
  }

  cutsV.clear();
  return false;
}
// ===========================================================================================================
TString VarMaps::getFailedCutType(){ 
// =================================
  return failedCutType;
}

// // ===========================================================================================================
// bool VarMaps::hasFiledTreeActiveCuts() {
// // =====================================
//   if(!areCutsEnabled) return false;

//   bool              hasFailed(false);
//   TString           prefix((TString)name+"_cut_");
//   vector <TString>  cutTypeV;  glob->getOptPattern("B",cutTypeV,prefix);

//   int nCutTypes = (int)cutTypeV.size();
//   for(int nCutTypeNow=0; nCutTypeNow<nCutTypes; nCutTypeNow++) {
//     TString cutType = cutTypeV[nCutTypeNow];  cutType.ReplaceAll(prefix,"");

//     if(hasFailedTreeCuts(cutType)) { hasFailed = true; break; }
//   }
//   for(int nCutTypeNow=0; nCutTypeNow<nCutTypes; nCutTypeNow++)
//     glob->DelOptB((TString)cutTypeV[nCutTypeNow]);

//   cutTypeV.clear();
//   return hasFailed;
// }
// ===========================================================================================================
void VarMaps::printCut(TString cutType, bool debug) {
// ==================================================
  if(cutType == "") {
    aCustomLOG("") <<coutRed<<" - Has cuts: ";
    for(Map <TString,TString>::iterator itr=treeCutsM.begin(); itr!=treeCutsM.end(); ++itr)
      aCustomLOG("") <<coutBlue<<"["<<itr->first<<" "<<coutGreen<<itr->second<<coutBlue<<"]  ,  ";
    aCustomLOG("")<<coutDef<<endl;
  } else {
    if(debug || inLOG(Log::DEBUG_2))
      aCustomLOG("") <<coutBlue<<"  -- Has cut ("<<coutGreen<<cutType<<coutBlue<<")  =  "<<coutRed<<treeCutsM[cutType]<<coutDef<<endl;
  }
  return;
}


// ===========================================================================================================
bool VarMaps::excludeThisBranch(TString branchName, vector <TString> * excludedBranchNames) {
// ==========================================================================================
  int nExludeBranches = (dynamic_cast<vector<TString>*>(excludedBranchNames)) ? (int)excludedBranchNames->size() : 0;

  if(branchName == "" || nExludeBranches == 0)                    return false;

  for(int nExcldNow=0; nExcldNow<nExludeBranches; nExcldNow++) {
    if(branchName.Contains(excludedBranchNames->at(nExcldNow)))   return true;
  }

  assert(dynamic_cast<TTree*>(treeWrite));
  if(dynamic_cast<TBranch*>(treeWrite->GetBranch(branchName)))    return true;

  return false;
}

// ===========================================================================================================
bool VarMaps::treeHasBranch(TTree * tree, TString branchName) {
// ============================================================
  assert(dynamic_cast<TTree*>(tree));
  if(branchName == "")  return false;
  
  TObjArray * brnchList = tree->GetListOfBranches();
  for (int nBrnchNow=0; nBrnchNow<=brnchList->GetLast(); nBrnchNow++)
    if(branchName == ((TBranch*)(brnchList->At(nBrnchNow)))->GetName()) return true;

  return false;
}

// ===========================================================================================================
void VarMaps::createTreeBranches(TTree * tree, TString prefix, TString postfix, vector <TString> * excludedBranchNames) {
// ======================================================================================================================
  int  width(20);
  bool debug(glob->OptOrNullB("debugBranches"));
  setTreeWrite(tree);

  if(debug) aCleanLOG() <<coutPurple<<" - createTreeBranches()  - Creating branches of "<<coutYellow<<tree->GetName()<<coutDef<<endl;

  for(Map <TString,Bool_t>::iterator itr=varB.begin(); itr!=varB.end(); ++itr) {
    TString branchName = (TString)prefix+(itr->first)+postfix;
    if(excludeThisBranch(branchName,excludedBranchNames) || treeHasBranch(treeWrite,branchName)) {
      if(debug) aCleanLOG() <<coutGreen<<"Skip create branch:  "<<coutRed<<std::setw(width)<<branchName<<CT<<coutBlue<<std::setw(width)<<"(B)"<<coutDef<<endl;
    }
    else { 
      if(debug) aCleanLOG() <<coutGreen<<"Creating branch:     "<<coutRed<<std::setw(width)<<branchName<<CT<<coutBlue<<std::setw(width)<<CT<<"(B)"<<coutDef<<endl;
      treeWrite->Branch(branchName, &itr->second);
    }
  }
  for(Map <TString,TObjString*>::iterator itr=varC.begin(); itr!=varC.end(); ++itr) {
    TString branchName = (TString)prefix+(itr->first)+postfix;
    if(excludeThisBranch(branchName,excludedBranchNames) || treeHasBranch(treeWrite,branchName)) {
      if(debug) aCleanLOG() <<coutGreen<<"Skip create branch:  "<<coutRed<<std::setw(width)<<branchName<<CT<<coutBlue<<std::setw(width)<<"(C)"<<coutDef<<endl;
    }
    else { 
      if(debug) aCleanLOG() <<coutGreen<<"Creating branch:     "<<coutRed<<std::setw(width)<<branchName<<CT<<coutBlue<<std::setw(width)<<CT<<"(C)"<<coutDef<<endl;
      treeWrite->Branch(branchName,"TObjString", &itr->second);
    }
  }
  for(Map <TString,Short_t>::iterator itr=varS.begin(); itr!=varS.end(); ++itr) {
    TString branchName = (TString)prefix+(itr->first)+postfix;
    if(excludeThisBranch(branchName,excludedBranchNames) || treeHasBranch(treeWrite,branchName)) {
      if(debug) aCleanLOG() <<coutGreen<<"Skip create branch:  "<<coutRed<<std::setw(width)<<branchName<<CT<<coutBlue<<std::setw(width)<<"(S)"<<coutDef<<endl;
    }
    else { 
      if(debug) aCleanLOG() <<coutGreen<<"Creating branch:     "<<coutRed<<std::setw(width)<<branchName<<CT<<coutBlue<<std::setw(width)<<CT<<"(S)"<<coutDef<<endl;
      treeWrite->Branch(branchName, &itr->second);
    }
  }
  for(Map <TString,Int_t>::iterator itr=varI.begin(); itr!=varI.end(); ++itr) {
    TString branchName = (TString)prefix+(itr->first)+postfix;
    if(excludeThisBranch(branchName,excludedBranchNames) || treeHasBranch(treeWrite,branchName)) {
      if(debug) aCleanLOG() <<coutGreen<<"Skip create branch:  "<<coutRed<<std::setw(width)<<branchName<<CT<<coutBlue<<std::setw(width)<<"(I)"<<coutDef<<endl;
    }
    else { 
      if(debug) aCleanLOG() <<coutGreen<<"Creating branch:     "<<coutRed<<std::setw(width)<<branchName<<CT<<coutBlue<<std::setw(width)<<CT<<"(I)"<<coutDef<<endl;
      treeWrite->Branch(branchName, &itr->second);
    }
  }
  for(Map <TString,Long64_t>::iterator itr=varL.begin(); itr!=varL.end(); ++itr) {
    TString branchName = (TString)prefix+(itr->first)+postfix;
    if(excludeThisBranch(branchName,excludedBranchNames) || treeHasBranch(treeWrite,branchName)) {
      if(debug) aCleanLOG() <<coutGreen<<"Skip create branch:  "<<coutRed<<std::setw(width)<<branchName<<CT<<coutBlue<<std::setw(width)<<"(L)"<<coutDef<<endl;
    }
    else { 
      if(debug) aCleanLOG() <<coutGreen<<"Creating branch:     "<<coutRed<<std::setw(width)<<branchName<<CT<<coutBlue<<std::setw(width)<<CT<<"(L)"<<coutDef<<endl;
      treeWrite->Branch(branchName, &itr->second);
    }
  }
  for(Map <TString,UShort_t>::iterator itr=varUS.begin(); itr!=varUS.end(); ++itr) {
    TString branchName = (TString)prefix+(itr->first)+postfix;
    if(excludeThisBranch(branchName,excludedBranchNames) || treeHasBranch(treeWrite,branchName)) {
      if(debug) aCleanLOG() <<coutGreen<<"Skip create branch:  "<<coutRed<<std::setw(width)<<branchName<<CT<<coutBlue<<std::setw(width)<<"(US)"<<coutDef<<endl;
    }
    else { 
      if(debug) aCleanLOG() <<coutGreen<<"Creating branch:     "<<coutRed<<std::setw(width)<<branchName<<CT<<coutBlue<<std::setw(width)<<CT<<"(US)"<<coutDef<<endl;
      treeWrite->Branch(branchName, &itr->second);
    }
  }
  for(Map <TString,UInt_t>::iterator itr=varUI.begin(); itr!=varUI.end(); ++itr) {
    TString branchName = (TString)prefix+(itr->first)+postfix;
    if(excludeThisBranch(branchName,excludedBranchNames) || treeHasBranch(treeWrite,branchName)) {
      if(debug) aCleanLOG() <<coutGreen<<"Skip create branch:  "<<coutRed<<std::setw(width)<<branchName<<CT<<coutBlue<<std::setw(width)<<"(UI)"<<coutDef<<endl;
    }
    else { 
      if(debug) aCleanLOG() <<coutGreen<<"Creating branch:     "<<coutRed<<std::setw(width)<<branchName<<CT<<coutBlue<<std::setw(width)<<CT<<"(UI)"<<coutDef<<endl;
      treeWrite->Branch(branchName, &itr->second);
    }
  }
  for(Map <TString,ULong64_t>::iterator itr=varUL.begin(); itr!=varUL.end(); ++itr) {
    TString branchName = (TString)prefix+(itr->first)+postfix;
    if(excludeThisBranch(branchName,excludedBranchNames) || treeHasBranch(treeWrite,branchName)) {
      if(debug) aCleanLOG() <<coutGreen<<"Skip create branch:  "<<coutRed<<std::setw(width)<<branchName<<CT<<coutBlue<<std::setw(width)<<"(UL)"<<coutDef<<endl;
    }
    else { 
      if(debug) aCleanLOG() <<coutGreen<<"Creating branch:     "<<coutRed<<std::setw(width)<<branchName<<CT<<coutBlue<<std::setw(width)<<CT<<"(UL)"<<coutDef<<endl;
      treeWrite->Branch(branchName, &itr->second);
    }
  }
  for(Map <TString,Float_t>::iterator itr=varF.begin(); itr!=varF.end(); ++itr) {
    TString branchName = (TString)prefix+(itr->first)+postfix;
    if(excludeThisBranch(branchName,excludedBranchNames) || treeHasBranch(treeWrite,branchName)) {
      if(debug) aCleanLOG() <<coutGreen<<"Skip create branch:  "<<coutRed<<std::setw(width)<<branchName<<CT<<coutBlue<<std::setw(width)<<"(F)"<<coutDef<<endl;
    }
    else { 
      if(debug) aCleanLOG() <<coutGreen<<"Creating branch:     "<<coutRed<<std::setw(width)<<branchName<<CT<<coutBlue<<std::setw(width)<<CT<<"(F)"<<coutDef<<endl;
      treeWrite->Branch(branchName, &itr->second);
    }
  }
  for(Map <TString,Double_t>::iterator itr=varD.begin(); itr!=varD.end(); ++itr) {
    TString branchName = (TString)prefix+(itr->first)+postfix;
    if(excludeThisBranch(branchName,excludedBranchNames) || treeHasBranch(treeWrite,branchName)) {
      if(debug) aCleanLOG() <<coutGreen<<"Skip create branch:  "<<coutRed<<std::setw(width)<<branchName<<CT<<coutBlue<<std::setw(width)<<"(D)"<<coutDef<<endl;
    }
    else { 
      if(debug) aCleanLOG() <<coutGreen<<"Creating branch:     "<<coutRed<<std::setw(width)<<branchName<<CT<<coutBlue<<std::setw(width)<<CT<<"(D)"<<coutDef<<endl;
      treeWrite->Branch(branchName, &itr->second);
    }
  }

  return;
}

// ===========================================================================================================
void VarMaps::connectTreeBranches(TTree * tree, vector <TString> * excludedBranchNames) {
// ======================================================================================
  // -----------------------------------------------------------------------------------------------------------
  // make sure there is no writing tree, as new vars will be added, which will change the
  // current addresses. Then, remove any possible reading tree.
  // The current vars are NOT CLEARED! just add more vars according to the tree branch names
  // -----------------------------------------------------------------------------------------------------------
  VERIFY(LOCATION,(TString)"trying to use connectTreeBranches() with no input tree defined ...",    ( dynamic_cast<TTree*>(tree)     ));
  VERIFY(LOCATION,(TString)"trying to use connectTreeBranches() with treeWrite already defined ...",(!dynamic_cast<TTree*>(treeWrite)));

  clearTrees();

  Map < TString,int >           nBranchesVar;
  Map < int,vector <TString> >  registeredBranches;

  int  width(20);
  bool debug(glob->OptOrNullB("debugBranches"));
  int  nExludeBranches = (dynamic_cast<vector<TString>*>(excludedBranchNames)) ? (int)excludedBranchNames->size() : 0;

  if(debug) aCustomLOG("connectTreeBranches()") <<coutPurple<<" - connectTreeBranches() - Connecting branches of "
                                                <<coutYellow<<tree->GetName()<<coutPurple<<" ..."<<coutDef<<endl;
  if(debug) aCustomLOG("connectTreeBranches()") <<coutCyan<<" - deriving branch names ..."<<coutDef<<endl;

  // -----------------------------------------------------------------------------------------------------------
  // go over all branches, determine the type and connect to the relevant Map
  // -----------------------------------------------------------------------------------------------------------
  TList * friendList = tree->GetListOfFriends();
  int   nTreeFriends = (dynamic_cast<TList*>(friendList)) ? friendList->GetEntries() : 0;
  
  TObjLink * friends(NULL);  // = aChainTrainTest->GetListOfFriends()->FirstLink();
  for(int nTreeNow=0; nTreeNow<nTreeFriends+1; nTreeNow++) {
    TTree * treeNow(NULL);
    if(nTreeNow == 0)   treeNow = tree;
    else {
      if(nTreeNow == 1) friends = tree->GetListOfFriends()->FirstLink();
      else              friends = friends->Next();
      
      TString friendName = ((TTree*)friends->GetObject())->GetName();
      treeNow = (TTree*)tree->GetFriend(friendName);
      if(debug) aCustomLOG("connectTreeBranches()") <<coutYellow<<"Now in tree-friend number "<<coutRed
                                                    <<nTreeNow<<coutYellow<<" with name: "<<coutRed<<friendName<<coutDef<<endl;
    }
  
    TObjArray * brnchList = treeNow->GetListOfBranches();
    for(int nBrnchNow=0; nBrnchNow<=brnchList->GetLast(); nBrnchNow++) {
      TBranch * aBranch  = (TBranch*)(brnchList->At(nBrnchNow));
      TString brnchName  = aBranch->GetName();
      TString brnchTitle = aBranch->GetTitle();
      TString brnchType  = (brnchTitle.Length() > 2) ? brnchTitle(brnchTitle.Length()-2,brnchTitle.Length()) : (TString)"";

      bool skipBranch(false);

      // test for array variables, which e.g., have the format "varName[4]/D"
      if(brnchTitle.Length() > 3) {
        TString brnchPref = brnchTitle(brnchTitle.Length()-3,brnchTitle.Length());
        if(brnchTitle.Contains("[") && brnchPref(0,1) == "]") skipBranch = true;
      }

      // go over exclusion list and search for skipped branches
      // -----------------------------------------------------------------------------------------------------------
      for(int nExlBranches=0; nExlBranches<nExludeBranches; nExlBranches++) {
        if(brnchName == excludedBranchNames->at(nExlBranches)) { skipBranch = true; break; }
      }
      if(!treeNow->GetBranchStatus(brnchName)) skipBranch = true;

      if(!skipBranch) {
        // count number of times a branch is accepted
        nBranchesVar[brnchName]++;
        // exclude repeate variables from tree friends
        // -----------------------------------------------------------------------------------------------------------
        if(nTreeNow > 0 && nBranchesVar[brnchName] > 1) {
          skipBranch = true; nBranchesVar[brnchName]--; assert(nBranchesVar[brnchName] == 1);
        }
      }
      if(debug) {
        if(skipBranch) { aCustomLOG("connectTreeBranches()") <<coutPurple<<"Skip connect branch: "<<coutRed<<std::setw(width)<<brnchName
                                                             <<CT<<coutBlue<<std::setw(width)<<brnchTitle<<CT<<brnchType<<coutDef<<endl; }
        else           { aCustomLOG("connectTreeBranches()") <<coutGreen <<"Will connect branch: "<<coutRed<<std::setw(width)<<brnchName
                                                             <<CT<<coutBlue<<std::setw(width)<<brnchTitle<<CT<<brnchType<<coutDef<<endl; }
      }
      
      if(skipBranch) treeNow->SetBranchStatus(brnchName,0);
      else           treeNow->SetBranchStatus(brnchName,1);

      // -----------------------------------------------------------------------------------------------------------
      // make sure all variables which will be connected to the tree are declared first. this is done
      // before actually setting any branch address in order to avoid changes of variable address as
      // the maps are filled up.
      // -----------------------------------------------------------------------------------------------------------
      if(!skipBranch) {
        if(brnchType == "/C" || brnchType == "/B" || brnchType == "/b") {
          aLOG(Log::ERROR) <<coutWhiteOnRed<<"Found un-supported branch type ("<<coutBlue<<brnchTitle<<coutWhiteOnRed
                           <<" , "<<coutYellow<<brnchType<<coutWhiteOnRed<<") - Please cast [Char_t(/B), to Int_t] or [UChar_t(/b) to UInt_t]."
                           <<" String(/C) should be part of TObjString.     ABORTING.... "<<coutDef<<endl;
          assert(false);
        }

        if     (brnchType == "/I") NewVarI_ (brnchName,DefOpts::DefI);
        else if(brnchType == "/F") NewVarF_ (brnchName,DefOpts::DefF);
        else if(brnchType == "/O") NewVarB_ (brnchName,DefOpts::DefB);
        else if(brnchType == "/D") NewVarD_ (brnchName,DefOpts::DefD);
        else if(brnchType == "/S") NewVarS_ (brnchName,DefOpts::DefS);
        else if(brnchType == "/L") NewVarL_ (brnchName,DefOpts::DefL);
        else if(brnchType == "/s") NewVarUS_(brnchName,DefOpts::DefUS);
        else if(brnchType == "/i") NewVarUI_(brnchName,DefOpts::DefUI);
        else if(brnchType == "/l") NewVarUL_(brnchName,DefOpts::DefUL);
        else {
          bool isTObjString = ((TString)aBranch->GetClassName() == "TObjString");
          if(!isTObjString) {
            aLOG(Log::ERROR) <<coutWhiteOnRed<<"Found un-supported branch type ("<<coutBlue<<brnchTitle<<coutWhiteOnRed
                             <<" , "<<coutYellow<<brnchType<<coutWhiteOnRed<<").  ABORTING.... "<<coutDef<<endl;
            assert(false);
          }
          NewVarC_(brnchName,DefOpts::DefC);
        }
      }
      if(!skipBranch) registeredBranches[nTreeNow].push_back(brnchName);
    }
  }
  setDefaultVals();

  // -----------------------------------------------------------------------------------------------------------
  // sanity check that variables have not been registered twice (thus unpredictable address?!?)
  // -----------------------------------------------------------------------------------------------------------
  for(Map <TString,int>::iterator itr=nBranchesVar.begin(); itr!=nBranchesVar.end(); ++itr) {
    if(itr->second == 1) continue;

    aLOG(Log::ERROR)  <<coutBlue<<"Var "<<coutRed<<itr->first<<coutBlue<<" of type "<<coutRed<<GetVarType(itr->first)
                      <<coutBlue<<" registered "<<coutRed<<itr->second<<coutBlue<<" time !!!"<<coutDef<<endl;
    assert(false);
  }

  // ===========================================================================================================
  // from this point on there will be no changes to the var maps so long as the tree is connected
  // this is imposed by explicitly setting the tree (adding variables checks the existence of treeRead)
  // ===========================================================================================================
  setTreeRead(tree);

  // -----------------------------------------------------------------------------------------------------------
  // now finally can connect the branches
  // -----------------------------------------------------------------------------------------------------------
  if(debug) aCustomLOG("connectTreeBranches()") <<coutCyan<<" - connecting branches ..."<<coutDef<<endl;

  friends = NULL;  // = aChainTrainTest->GetListOfFriends()->FirstLink();
  for(int nTreeNow=0; nTreeNow<nTreeFriends+1; nTreeNow++) {
    TTree * treeNow(NULL);
    if(nTreeNow == 0) {
      treeNow = tree;

      if(debug && nTreeFriends > 0) aCustomLOG("connectTreeBranches()") <<coutYellow<<"Now in primary tree "<<coutDef<<endl;
    }
    else {
      if(nTreeNow == 1) friends = tree->GetListOfFriends()->FirstLink();
      else              friends = friends->Next();
      
      TString friendName = ((TTree*)friends->GetObject())->GetName();
      treeNow = (TTree*)tree->GetFriend(friendName);
      if(debug) aCustomLOG("connectTreeBranches()") <<coutYellow<<"Now in tree-friend number "<<coutRed<<nTreeNow
                                                    <<coutYellow<<" with name: "<<coutRed<<friendName<<coutDef<<endl;
    }

    for (int nBrnchNow=0; nBrnchNow<(int)registeredBranches[nTreeNow].size(); nBrnchNow++) {
      TString brnchName = registeredBranches[nTreeNow][nBrnchNow];

      if(debug) aCustomLOG("connectTreeBranches()") <<coutGreen<<"Now connecting:      "<<coutRed<<std::setw(width)<<brnchName
                                                    <<CT<<coutBlue<<std::setw(width)<<GetVarType(brnchName)<<coutDef<<endl;

      if     (GetVarType(brnchName) == "I" ) { AsrtVar(HasVarI_ (brnchName),brnchName); treeNow->SetBranchAddress(brnchName, &(varI [brnchName])); }
      else if(GetVarType(brnchName) == "F" ) { AsrtVar(HasVarF_ (brnchName),brnchName); treeNow->SetBranchAddress(brnchName, &(varF [brnchName])); }
      else if(GetVarType(brnchName) == "B" ) { AsrtVar(HasVarB_ (brnchName),brnchName); treeNow->SetBranchAddress(brnchName, &(varB [brnchName])); }
      else if(GetVarType(brnchName) == "C" ) { AsrtVar(HasVarC_ (brnchName),brnchName); treeNow->SetBranchAddress(brnchName, &(varC [brnchName])); }
      else if(GetVarType(brnchName) == "D" ) { AsrtVar(HasVarD_ (brnchName),brnchName); treeNow->SetBranchAddress(brnchName, &(varD [brnchName])); }
      else if(GetVarType(brnchName) == "S" ) { AsrtVar(HasVarS_ (brnchName),brnchName); treeNow->SetBranchAddress(brnchName, &(varS [brnchName])); }
      else if(GetVarType(brnchName) == "L" ) { AsrtVar(HasVarL_ (brnchName),brnchName); treeNow->SetBranchAddress(brnchName, &(varL [brnchName])); }
      else if(GetVarType(brnchName) == "US") { AsrtVar(HasVarUS_(brnchName),brnchName); treeNow->SetBranchAddress(brnchName, &(varUS[brnchName])); }
      else if(GetVarType(brnchName) == "UI") { AsrtVar(HasVarUI_(brnchName),brnchName); treeNow->SetBranchAddress(brnchName, &(varUI[brnchName])); }
      else if(GetVarType(brnchName) == "UL") { AsrtVar(HasVarUL_(brnchName),brnchName); treeNow->SetBranchAddress(brnchName, &(varUL[brnchName])); }
      else assert(false);
    }
  }

  // cleanup
  registeredBranches.clear(); nBranchesVar.clear();

  return;
}
// ===========================================================================================================
void VarMaps::connectTreeBranchesForm(TTree * tree, vector < pair<TString,Float_t> > * readerInptV, vector <TString> * excludedBranchNames) {
// ==========================================================================================================================================
  VERIFY(LOCATION,(TString)"trying to use connectTreeBranchesForm() with no input readerInptV defined ...",
                           ( dynamic_cast< vector < pair<TString,Float_t> >* >(readerInptV) ));

  addReaderFormulae(*readerInptV);
  connectTreeBranches(tree,excludedBranchNames);
  return;
}



// ===========================================================================================================
// reset the address of a tree's branches to the current variables - this allows to
// change the var from which the tree is filled
// (called after var->createTreeBranches(tree) such that all the branches already exist)
// ===========================================================================================================
void VarMaps::resetTreeBrancheAddresses(TTree * tree) {
// ====================================================

  for(Map <TString,Bool_t>     ::iterator itr=varB .begin(); itr!=varB .end(); ++itr) { tree->GetBranch(itr->first)->SetAddress(&itr->second); }
  for(Map <TString,TObjString*>::iterator itr=varC .begin(); itr!=varC .end(); ++itr) { tree->GetBranch(itr->first)->SetAddress(&itr->second); }  
  for(Map <TString,Short_t>    ::iterator itr=varS .begin(); itr!=varS .end(); ++itr) { tree->GetBranch(itr->first)->SetAddress(&itr->second); }
  for(Map <TString,Int_t>      ::iterator itr=varI .begin(); itr!=varI .end(); ++itr) { tree->GetBranch(itr->first)->SetAddress(&itr->second); }
  for(Map <TString,Long64_t>   ::iterator itr=varL .begin(); itr!=varL .end(); ++itr) { tree->GetBranch(itr->first)->SetAddress(&itr->second); }
  for(Map <TString,UShort_t>   ::iterator itr=varUS.begin(); itr!=varUS.end(); ++itr) { tree->GetBranch(itr->first)->SetAddress(&itr->second); }
  for(Map <TString,UInt_t>     ::iterator itr=varUI.begin(); itr!=varUI.end(); ++itr) { tree->GetBranch(itr->first)->SetAddress(&itr->second); }
  for(Map <TString,ULong64_t>  ::iterator itr=varUL.begin(); itr!=varUL.end(); ++itr) { tree->GetBranch(itr->first)->SetAddress(&itr->second); }
  for(Map <TString,Float_t>    ::iterator itr=varF .begin(); itr!=varF .end(); ++itr) { tree->GetBranch(itr->first)->SetAddress(&itr->second); }
  for(Map <TString,Double_t>   ::iterator itr=varD .begin(); itr!=varD .end(); ++itr) { tree->GetBranch(itr->first)->SetAddress(&itr->second); }
  return ;
}

// ===========================================================================================================
void VarMaps::addReaderFormulae(vector < pair<TString,Float_t> > & readerInptV) {
// ==============================================================================
  for(int nReaderInputNow=0; nReaderInputNow<(int)readerInptV.size(); nReaderInputNow++) {
    TString inputName = readerInptV[nReaderInputNow].first;
    TString formName  = readerFormNameKey+inputName;
    NewForm(formName,inputName);
  }
  return ;
}
// ===========================================================================================================
void VarMaps::updateReaderFormulae(vector < pair<TString,Float_t> > & readerInptV, bool forceUpdate) {
// ===================================================================================================
  if(!forceUpdate && !needReaderUpdate) return;
  
  for(int nReaderInputNow=0; nReaderInputNow<(int)readerInptV.size(); nReaderInputNow++) {
    TString inputName = readerInptV[nReaderInputNow].first;
    TString formName  = readerFormNameKey+inputName;

    readerInptV[nReaderInputNow].second = GetForm(formName);
  }
  needReaderUpdate = false;

  return ;
}

// ===========================================================================================================
// special version for TString, as wihout the explicit type in the Branch() method, a TString* is written...
bool VarMaps::getTreeEntry(int nEntry, bool getEntryIndex) {
// =========================================================
  VERIFY(LOCATION,(TString)"trying to use getTreeEntry() with no treeRead defined ...",(dynamic_cast<TTree*>(treeRead)));

  needReaderUpdate = true;

  // setDefaultVals();  // this is not really needed, and saves significant computational load 

  Int_t loopTreeEntryTest(0);
  if(!getEntryIndex) loopTreeEntryTest = treeRead->GetEntry(nEntry);
  else               loopTreeEntryTest = treeRead->GetEntryWithIndex(nEntry);

  if(loopTreeEntryTest == 0 || loopTreeEntryTest == -1) return false;  //{ setDefaultVals(); return false; }

  // -----------------------------------------------------------------------------------------------------------
  // check if need to reset formulae for cuts and for varForm due to change in number of tree file in the chain
  // or to number of tree-friend file
  // -----------------------------------------------------------------------------------------------------------
  bool  needUpdateMain(false), needUpdateFriend(false), isFirstEntry(nTreeInChain == -1);

  // check if the primary tree has changed file
  // -----------------------------------------------------------------------------------------------------------
  if(nTreeInChain != treeRead->GetTreeNumber()) { 
    nTreeInChain      = treeRead->GetTreeNumber();
    TString FileName  = (TString)((dynamic_cast<TChain*>(treeRead)) ? dynamic_cast<TChain*>(treeRead)->GetFile()->GetName() : "");

    aLOG(Log::DEBUG) <<coutRed<<" - Switched to tree("<<coutBlue<<nTreeInChain<<coutRed<<") in chain \""<<coutGreen<<treeRead->GetName()
                     <<coutRed<<"\" from file "<<coutYellow<<FileName<<coutRed<<" ..."<<coutDef<<endl;

    needUpdateMain = true;
  }

  // if first time using this treeRead, store pointers to all tree friends and initialize
  // the counter for the tree number for each
  // -----------------------------------------------------------------------------------------------------------
  if(isFirstEntry) {
    chainFriendV.clear(); nTreeFriendInChainV.clear();

    TList * friendList = treeRead->GetListOfFriends();
    int   nTreeFriends = (dynamic_cast<TList*>(friendList)) ? friendList->GetEntries() : 0;
    
    TObjLink * friends(NULL);
    for(int nTreeNow=0; nTreeNow<nTreeFriends; nTreeNow++) {
      if(nTreeNow == 0) friends = treeRead->GetListOfFriends()->FirstLink();
      else              friends = friends->Next();
      
      TString friendNameNow = ((TTree*)friends->GetObject())->GetName();
      TTree   * friendNow   = (TTree*)treeRead->GetFriend(friendNameNow);

      chainFriendV.push_back(friendNow); nTreeFriendInChainV.push_back(0);
      
      aLOG(Log::DEBUG_1) <<coutBlue<<" - Found tree friend ("<<coutRed<<nTreeNow<<coutBlue<<") for primary tree \""
                         <<coutGreen<<treeRead->GetName()<<coutBlue<<"\" with name \""
                         <<coutYellow<<friendNameNow<<coutBlue<<"\" ..."<<coutDef<<endl;
    }
  }

  // check if tree friends have changed file
  // -----------------------------------------------------------------------------------------------------------
  for(int nFriendNow=0; nFriendNow<(int)chainFriendV.size(); nFriendNow++) {
    if(nTreeFriendInChainV[nFriendNow] == chainFriendV[nFriendNow]->GetTreeNumber()) continue;

    nTreeFriendInChainV[nFriendNow] = chainFriendV[nFriendNow]->GetTreeNumber();
 
    aLOG(Log::DEBUG) <<coutRed<<" - Switched to tree friend("<<coutBlue<<nFriendNow<<","<<nTreeFriendInChainV[nFriendNow]<<coutRed<<") in chain: "
                     <<coutBlue<<treeRead->GetName()<<coutRed<<" ..."<<coutDef<<endl;

    needUpdateFriend = true;
  }

  // create/update the formulae if needed
  // -----------------------------------------------------------------------------------------------------------
  if(needUpdateMain || needUpdateFriend) {
    aLOG(Log::DEBUG) <<coutPurple<<" - (Re)Set TTreeFormula for tree ("<<coutYellow<<nTreeInChain<<coutPurple<<") for (main,friend = "
                     <<coutYellow<<needUpdateMain<<coutPurple<<","<<coutYellow<<needUpdateFriend<<coutPurple<<") ..."<<coutDef<<endl;

    setTreeForms(isFirstEntry);
  }

  return true;
}

// ===========================================================================================================
void VarMaps::setTreeForms(bool isFirstEntry) {
// ============================================
  VERIFY(LOCATION,(TString)"trying to use setTreeForms() with no treeRead defined ...",(dynamic_cast<TTree*>(treeRead)));
  
  for(int nFormType=0; nFormType<2; nFormType++) {
    Map <TString,TString>       * nameMap;
    Map <TString,TTreeFormula*> * formMap;
    // cuts
    if(nFormType == 0) {
      nameMap = & treeCutsM;
      formMap = & treeCutsFormM;

      if(!areCutsEnabled) continue;
    }
    // formula variables
    else {
      nameMap = & varFM;
      formMap = & varFormM;

      if((int)nameMap->size() == 0) continue;    
    }

    if(isFirstEntry) {
      for(Map <TString,TString>::iterator itr=nameMap->begin(); itr!=nameMap->end(); ++itr) {
        TString treeForm     = (regularizeStringForm(itr->second)).ReplaceAll(" ","");
        TString treeFormName = utils->regularizeName( (TString)treeRead->GetName()+"_"+itr->first );
        TString aCut         = (TString)((treeForm == "") ? "1" : treeForm);

        // cout <<"setTreeForms  "<<treeForm<<CT<<treeFormName<<CT<<(TString)aCut<<endl;
        
        if(formMap->find(itr->first) != formMap->end()) DELNULL((*formMap)[itr->first]);
        (*formMap)[itr->first] = new TTreeFormula(treeFormName,(TCut)aCut,treeRead);
       
        VERIFY(LOCATION,(TString)"TTreeFormula is not valid (\""+(TString)aCut+"\") ...",((*formMap)[itr->first]->GetNdim() != 0));
      }
    }
    else {
      for(Map <TString,TTreeFormula*>::iterator itr=formMap->begin(); itr!=formMap->end(); ++itr) itr->second->UpdateFormulaLeaves();
    }
  }

  return;
}

// ===========================================================================================================
void VarMaps::storeTreeToAscii(TString outFilePrefix, TString outFileDir, int maxNobj, int nLinesFile,
                               TString treeCuts, vector <TString> * acceptV, vector <TString> * rejectV) {
// =======================================================================================================
  aLOG(Log::INFO) <<coutWhiteOnBlack<<coutPurple<<" - starting storeTreeToAscii() ... "<<coutDef<<endl;

  VERIFY(LOCATION,(TString)"trying to use storeTreeToAscii() with no treeRead defined ...",(dynamic_cast<TTree*>(treeRead)));
  VERIFY(LOCATION,(TString)"did not define outFilePrefix in storeTreeToAscii() ...",(outFilePrefix != ""));

  TObjLink         * friends(NULL);
  TString          acceptedBranches(""), outFileName("");
  std::ofstream    * fout(NULL);
  int              nVarsIn(0), nOutFileNow(0);
  vector <TString> varNames, varTypes;
  
  bool hasAccept = dynamic_cast<vector<TString>*>(acceptV);
  bool hasReject = dynamic_cast<vector<TString>*>(rejectV);

  TString csvPostfix(".csv"), rootPostfix(".root"), treeName(treeRead->GetName());
  int nEntriesChain = treeRead->GetEntries(); if(maxNobj > 0) nEntriesChain = min(nEntriesChain,maxNobj);

  if(outFileDir == "") { outFileDir = utils->getFilePath(dynamic_cast<TChain*>(treeRead)->GetFile()->GetName()); }
  
  if(!outFileDir.EndsWith("/")) outFileDir += "/";

  if(hasAccept) {
    // use only variable from the accepted list
    nVarsIn = (int)acceptV->size();
    for(int nVarsInNow=0; nVarsInNow<nVarsIn; nVarsInNow++) {
      TString brnchName = acceptV->at(nVarsInNow);
      TString brnchType = GetVarType(brnchName);

      varNames.push_back(brnchName); varTypes.push_back(brnchType);
      acceptedBranches += (TString)coutGreen+brnchName+coutYellow+",";
    }
  }
  else {
    // get the list of accepted branches
    TList * friendList = treeRead->GetListOfFriends();
    int   nTreeFriends = (dynamic_cast<TList*>(friendList)) ? friendList->GetEntries() : 0;
    for(int nTreeNow=0; nTreeNow<nTreeFriends+1; nTreeNow++) {
      TTree * treeNow(NULL);
      if(nTreeNow == 0)   treeNow = treeRead;
      else {
        if(nTreeNow == 1) friends = treeRead->GetListOfFriends()->FirstLink();
        else              friends = friends->Next();
        
        TString friendName = ((TTree*)friends->GetObject())->GetName();
        treeNow = (TTree*)treeRead->GetFriend(friendName);
      }
    
      TObjArray * brnchList = treeNow->GetListOfBranches();
      for(int nBrnchNow=0; nBrnchNow<=brnchList->GetLast(); nBrnchNow++) {
        TBranch * aBranch = (TBranch*)(brnchList->At(nBrnchNow));
        TString brnchName = aBranch->GetName();
        TString brnchType = GetVarType(brnchName);

        // avoid double counting
        if(find(varNames.begin(),varNames.end(),brnchName) != varNames.end()) continue;
        // may have rejection list
        if(hasReject) { if(find(rejectV->begin(),rejectV->end(),brnchName) != rejectV->end()) continue; }

        varNames.push_back(brnchName); varTypes.push_back(brnchType);
        acceptedBranches += (TString)coutGreen+brnchName+coutYellow+",";
      }
    }
    nVarsIn = (int)varNames.size();
  }
  VERIFY(LOCATION,(TString)"trying to use setTreeForms() with no active branches ...",(nVarsIn > 0));

  aLOG(Log::INFO) <<coutBlue<<" - will write to file the following branches: "<<acceptedBranches<<coutDef<<endl;

  // header line with variable names/types
  TString header = "#";
  for(int nVarsInNow=0; nVarsInNow<nVarsIn; nVarsInNow++) {
    header += (TString)varTypes[nVarsInNow]+":"+varNames[nVarsInNow]+";";
  }
  header = header(0,header.Length()-1); // remove trailing ","

  // -----------------------------------------------------------------------------------------------------------
  // the final loop
  // -----------------------------------------------------------------------------------------------------------
  CntrMap * cntrMapIn = new CntrMap(glob,utils,(TString)name+"_cntrMapIn");
  cntrMapIn->copyCntr(cntrMap); clearCntr();

  for(Long64_t loopEntry=0; true; loopEntry++) {
    if(!getTreeEntry(loopEntry)) break;

    // skip if failed selection cuts (if (treeCuts==""), no cuts will be applied)
    // -----------------------------------------------------------------------------------------------------------
    if(hasFailedTreeCuts(treeCuts)) continue;

    TString line = "";
    for(int nVarsInNow=0; nVarsInNow<nVarsIn; nVarsInNow++) {
      TString nameNow(varNames[nVarsInNow]), typeNow(varTypes[nVarsInNow]);

      // line += (TString)"  "+(TString)((loopEntry%2==0)?coutYellow:coutRed)+nameNow+" = "+coutDef; // uncomment for debugging ease

      if     (typeNow == "F"  || typeNow == "D"   ) { line += utils->doubleToStr(     GetVarF(nameNow)        ); }
      else if(typeNow == "S"  || typeNow == "I"   ) { line += utils->intToStr   (     GetVarI(nameNow)        ); }
      else if(typeNow == "B"                      ) { line += (TString)         (     GetVarB(nameNow)?"1":"0"); }
      else if(typeNow == "C"                      ) { line += (TString)         ("\""+GetVarC(nameNow)+"\""   ); }
      else if(typeNow == "L"                      ) { line += utils->lIntToStr  (     GetVarI(nameNow)        ); }
      else if(typeNow == "US" || typeNow == "UI"  ) { line += utils->uIntToStr  (     GetVarU(nameNow)        ); }
      else if(typeNow == "UL"                     ) { line += utils->ULIntToStr (     GetVarU(nameNow)        ); }
      else VERIFY(LOCATION,(TString)"found unsupported variable-type ("+typeNow+")",false);

      line += ",";
    }
    line = line(0,line.Length()-1); // remove trailing ","

    IncCntr("nObj"); if(GetCntr("nObj") == maxNobj) break;

    if(!dynamic_cast<std::ofstream*>(fout) || (dynamic_cast<std::ofstream*>(fout) && (nLinesFile > 0) && ((GetCntr("nObj")-1) % nLinesFile == 0))) {
      nOutFileNow += 1;
      outFileName  = (TString)outFileDir+outFilePrefix+"_"+TString::Format("%4.4d",nOutFileNow-1)+csvPostfix;
      
      DELNULL(fout);
      fout = new std::ofstream(outFileName, std::ios::trunc);
      *fout <<header<<endl;

      aLOG(Log::INFO) <<coutRed<<" - Will parse  "<<coutGreen<<treeName<<"("<<nEntriesChain<<")"<<"... Now in "<<coutBlue<<outFileName<<coutDef<<endl;
    }

    *fout <<line<<endl;
  }
  printCntr((TString)outFilePrefix);

  clearCntr(); cntrMap->copyCntr(cntrMapIn);
  DELNULL(cntrMapIn);

  DELNULL(fout);
  varNames.clear(); varTypes.clear();

  return;
}
