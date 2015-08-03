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

#ifndef VarMaps_h
#define VarMaps_h

#include "commonInclude.hpp"
#include "OptMaps.hpp"
#include "Utils.hpp"
#include "CntrMap.hpp"

// ===========================================================================================================
class VarMaps {
// ============

public :
  VarMaps(OptMaps * aOptMaps = NULL, Utils * aUtils = NULL, TString aName = "aVarMaps");
  ~VarMaps();
 
private:
  map <TString,Bool_t>          varB;
  map <TString,TObjString*>     varC;
  map <TString,Short_t>         varS;
  map <TString,Int_t>           varI;
  map <TString,Long64_t>        varL;
  map <TString,UShort_t>        varUS;
  map <TString,UInt_t>          varUI;
  map <TString,ULong64_t>       varUL;
  map <TString,Float_t>         varF;
  map <TString,Double_t>        varD;
  map <TString,TString>         varFM;

  map <TString,TString>         treeCutsM;
  map <TString,TTreeFormula*>   treeCutsFormM, varFormM;

  map <TString,bool>            hasB, hasC, hasS, hasI, hasL, hasUS, hasUI, hasUL, hasF, hasD, hasFM;

  TTree                         * treeRead, * treeWrite;
  int                           nTreeInChain;
  vector <TTree*>               chainFriendV;
  vector <int>                  nTreeFriendInChainV;
  bool                          areCutsEnabled, needReaderUpdate;

  TString   name;
  TString   CT;
  TString   coutDef, coutRed, coutGreen, coutBlue, coutLightBlue, coutYellow, coutPurple, coutCyan,
            coutUnderLine, coutWhiteOnBlack, coutWhiteOnRed, coutWhiteOnGreen, coutWhiteOnYellow;

  OptMaps * glob;
  Utils   * utils;
  CntrMap * cntrMap;

  TString         readerFormNameKey, failedCutType;
  void            setTreeForms(bool isFirstEntry);


public: 
  // -----------------------------------------------------------------------------------------------------------
  inline TString  getName() { return name; };
  void            clearVar();
  void            clearTrees();
  inline void     clearAll()       { clearTrees(); clearVar(); return; }
  void            varStruct       (VarMaps * inObj, vector <TString> * acceptV = NULL, vector <TString> * rejectV = NULL,
                                                    vector < pair<TString,TString> > * varTypeNameV = NULL, bool isCopy = true);
  void            copyVarData     (VarMaps * inObj, vector < pair<TString,TString> > * varTypeNameV = NULL);
  void            rmVarPattern(TString pattern, TString type);
  void            setDefaultVals(vector < pair<TString,TString> > * varTypeNameV = NULL);

  void            getVarPattern(TString type, vector <TString> & optV, TString pattern, bool ignorCase);
  void            GetAllVarNames(vector <TString> & optNames, TString type = "ALL");
  void            GetAllVarNameTypes(vector < pair<TString,TString> > & varTypeNameV, TString type = "ALL");
  void            printVarNames(TString type = "ALL", TString title = "");
  
  void            printCut(TString cutType, bool debug = false);
  inline void     setTreeCutStatus(bool cutStatus) { areCutsEnabled = cutStatus; return; }
  inline bool     getTreeCutStatus()               { return areCutsEnabled; }
  void            addTreeCuts(TString cutType, TCut aCut);
  void            setTreeCuts(TString cutType, TCut aCut);
  void            getTreeCutsM(map <TString,TCut> & aTreeCutsM);
  void            setTreeCutsM(map <TString,TCut> & aTreeCutsM);
  TCut            getTreeCuts(TString cutType = "");
  void            updateReaderFormulae(vector < pair<TString,Float_t> > & readerInptV, bool forceUpdate = false);
  void            addReaderFormulae(vector < pair<TString,Float_t> > & readerInptV);

  void            eraseTreeCutsPattern(TString cutPattern, bool ignorCase = false);  
  int             replaceTreeCut(TString oldCut, TString newCut);  
  bool            hasFailedTreeCuts(vector <TString> & cutTypeV);
  bool            hasFailedTreeCuts(TString cutType);
  TString         getFailedCutType();

  inline TTree *  getTreeWrite()                    { if(dynamic_cast<TTree*>(treeWrite)) return treeWrite; else return NULL;          }
  inline TTree *  getTreeRead ()                    { if(dynamic_cast<TTree*>(treeRead )) return treeRead;  else return NULL;          }
  void            setTreeWrite(TTree * tree = NULL);
  void            setTreeRead (TTree * tree = NULL);

  void            createTreeBranches(TTree * tree = NULL, TString prefix = "", TString postfix = "", vector <TString> * excludedBranchNames = NULL);
  void            connectTreeBranches(TTree * tree = NULL, vector <TString> * excludedBranchNames = NULL);
  void            connectTreeBranchesForm(TTree * tree = NULL, vector < pair<TString,Float_t> > * readerInptV = NULL,
                                          vector <TString> * excludedBranchNames = NULL);
  void            resetTreeBrancheAddresses(TTree * tree = NULL);
  bool            getTreeEntry(int nEntry, bool getEntryIndex = false);

  bool            excludeThisBranch(TString branchName = "", vector <TString> * excludedBranchNames = NULL);
  bool            treeHasBranch(TTree * tree = NULL, TString branchName = "");
//bool            hasBranchWithName(TString branchName, TString branchType = "ALL", bool isExactName = true);
//template <typename T> bool  hasBranchWithName(map <TString,T> & input, TString branchName, bool isExactName = true);
  void            storeTreeToAscii(TString outFilePrefix, TString outFileDir = "", int maxNobj = 0, int nLinesFile = 0,
                                   TString treeCuts = "", vector <TString> * acceptV = NULL, vector <TString> * rejectV = NULL);

  void            printVars(int nPrintRow = 0, int width = 0);
  //inline void     printMapAll(int nPrintRow = 0, int width = 0) { printOpts(nPrintRow,width); printVars(nPrintRow,width); return; }

  // -----------------------------------------------------------------------------------------------------------
  // add a new variable
  // -----------------------------------------------------------------------------------------------------------
  void NewVarB(TString aName, Bool_t    input = DefOpts::DefB);
  void NewVarC(TString aName, TString   input = DefOpts::DefC); 
  void NewVarI(TString aName, Long64_t  input = DefOpts::DefL,  TString type = "");
  void NewVarU(TString aName, ULong64_t input = DefOpts::DefUL, TString type = "");
  void NewVarF(TString aName, Double_t  input = DefOpts::DefD,  TString type = "");
  void NewForm(TString aName, TString   input = DefOpts::DefC);

  // remove a variable
  // -----------------------------------------------------------------------------------------------------------
  inline void DelVarB(TString aName) { checkLock(aName); DelVarB_ (aName);                                     }
  inline void DelVarC(TString aName) { checkLock(aName); DelVarC_ (aName);                                     }
  inline void DelVarI(TString aName) { checkLock(aName); DelVarS_ (aName); DelVarI_ (aName); DelVarL_ (aName); }
  inline void DelVarU(TString aName) { checkLock(aName); DelVarUS_(aName); DelVarUI_(aName); DelVarUL_(aName); }
  inline void DelVarF(TString aName) { checkLock(aName); DelVarF_ (aName); DelVarD_ (aName);                   }
  inline void DelForm(TString aName) { checkLock(aName); DelForm_ (aName);                                     }

  // set the value of a variable
  // -----------------------------------------------------------------------------------------------------------
  void SetVarB(TString aName, Bool_t    input);
  void SetVarC(TString aName, TString   input);
  void SetVarI(TString aName, Long64_t  input);
  void SetVarU(TString aName, ULong64_t input);
  void SetVarF(TString aName, Double_t  input);
  void SetForm(TString aName, TString   input);
  // overloaded methos with TString inputs
  void SetVarB(TString aName, TString   input);
  void SetVarI(TString aName, TString   input);
  void SetVarU(TString aName, TString   input);
  void SetVarF(TString aName, TString   input);

  // get the value of a variable
  // -----------------------------------------------------------------------------------------------------------
  inline Bool_t    GetVarB(TString aName) { return GetVarB_(aName); }
  inline TString   GetVarC(TString aName) { return GetVarC_(aName); }
  inline Long64_t  GetVarI(TString aName) {
    Long64_t val (0);
    if     (HasVarI_(aName)) val = static_cast<Long64_t> (GetVarI_(aName)); else if(HasVarS_(aName)) val = static_cast<Long64_t> (GetVarS_(aName));
    else if(HasVarL_(aName)) val = GetVarL_(aName);                         else AsrtVar(false,aName+" (GetVarI)");
    return val;
  };
  inline ULong64_t GetVarU(TString aName) {
    ULong64_t val(0);
    if     (HasVarUI_(aName)) val = static_cast<ULong64_t>(GetVarUI_(aName)); else if(HasVarUS_(aName)) val = static_cast<ULong64_t>(GetVarUS_(aName));
    else if(HasVarUL_(aName)) val = GetVarUL_(aName);                         else AsrtVar(false,aName+" (GetVarU)");
    return val;
  };
  inline Double_t  GetVarF(TString aName) {
    Double_t val (0);
    if     (HasVarF_ (aName)) val = static_cast<Double_t> (GetVarF_ (aName));
    else if(HasVarD_ (aName)) val = GetVarD_ (aName);                         else AsrtVar(false,aName+" (GetVarF)");
    return val;
  };
  Double_t  GetForm(TString aName);

  // check if a variable is already defined
  // -----------------------------------------------------------------------------------------------------------
  inline bool HasVarB(TString aName) { return HasVarB_ (aName);                                         }
  inline bool HasVarC(TString aName) { return HasVarC_ (aName);                                         }
  inline bool HasVarI(TString aName) { return HasVarS_ (aName) || HasVarI_ (aName) || HasVarL_ (aName); }
  inline bool HasVarU(TString aName) { return HasVarUS_(aName) || HasVarUI_(aName) || HasVarUL_(aName); }
  inline bool HasVarF(TString aName) { return HasVarF_ (aName) || HasVarD_ (aName);                     }
  inline bool HasForm(TString aName) { return HasForm_ (aName);                                         }

  inline bool HasVar  (TString aName) { return (HasVarB(aName)||HasVarC(aName)||HasVarI(aName)||HasVarU(aName)||HasVarF(aName)||HasForm(aName)); }

  // // check if a defined vriable has the default value for its type
  // // -----------------------------------------------------------------------------------------------------------
  // inline bool IsDefVarC(TString aName) {
  //   bool val(false);
  //   if(HasVarC_ (aName)) val = IsDefVarC_ (aName);      else AsrtVar(false,aName+" (IsDefVarC)");
  //   return val;
  // }
  // inline bool IsDefVarI(TString aName) {
  //   bool val(false);
  //   if     (HasVarS_ (aName)) val = IsDefVarS_ (aName); else if(HasVarI_ (aName)) val = IsDefVarI_ (aName);
  //   else if(HasVarL_ (aName)) val = IsDefVarL_ (aName); else AsrtVar(false,aName+" (IsDefVarI)");
  //   return val;
  // }
  // inline bool IsDefVarU(TString aName) {
  //   bool val(false);
  //   if     (HasVarUS_(aName)) val = IsDefVarUS_(aName); else if(HasVarUI_(aName)) val = IsDefVarUI_(aName);
  //   else if(HasVarUL_(aName)) val = IsDefVarUL_(aName); else AsrtVar(false,aName+" (IsDefVarU)");
  //   return val;
  // }
  // inline bool IsDefVarF(TString aName) {
  //   bool val(false);
  //   if     (HasVarF_ (aName)) val = IsDefVarF_ (aName);
  //   else if(HasVarD_ (aName)) val = IsDefVarD_ (aName); else AsrtVar(false,aName+" (IsDefVarD)");
  //   return val;
  // }

  // get the type of a var
  // -----------------------------------------------------------------------------------------------------------
  inline TString GetVarType(TString aName) {
    TString type(""); int nHas(0);
    if(HasVarB_ (aName)) { type += "B";  nHas++; } if(HasVarC_ (aName)) { type += "C";  nHas++; } if(HasVarS_ (aName)) { type += "S";  nHas++; }
    if(HasVarI_ (aName)) { type += "I";  nHas++; } if(HasVarL_ (aName)) { type += "L";  nHas++; } if(HasVarUS_(aName)) { type += "US"; nHas++; }
    if(HasVarUI_(aName)) { type += "UI"; nHas++; } if(HasVarUL_(aName)) { type += "UL"; nHas++; } if(HasVarF_ (aName)) { type += "F";  nHas++; }
    if(HasVarD_ (aName)) { type += "D";  nHas++; }
    if(HasForm_ (aName)) { type += "FM"; nHas++; }

    if(nHas != 1)        { AsrtVar(nHas < 2,(TString)"multiple definitions - "+aName);  AsrtVar(nHas > 0,(TString)"undefined var - "+aName);   }
    return type;
  }

  // some functionality for simple operations with vars
  // -----------------------------------------------------------------------------------------------------------
  inline void AddVarC(TString aName, TString   input) {
    SetVarC_(aName, GetVarC_(aName) + input);
  }
  inline void AddVarI(TString aName, Long64_t  input) {
    if     (HasVarI_ (aName)) SetVarI_ (aName, GetVarI_ (aName) + input);
    else if(HasVarS_ (aName)) SetVarS_ (aName, GetVarS_ (aName) + input);
    else if(HasVarL_ (aName)) SetVarL_ (aName, GetVarL_ (aName) + input);
    else                      AsrtVar(false,aName+" (AddVarI)");
  }
  inline void AddVarU(TString aName, ULong64_t input) {
    if     (HasVarUI_(aName)) SetVarUI_(aName, GetVarUI_(aName) + input);
    else if(HasVarUS_(aName)) SetVarUS_(aName, GetVarUS_(aName) + input);
    else if(HasVarUL_(aName)) SetVarUL_(aName, GetVarUL_(aName) + input);
    else                      AsrtVar(false,aName+" (AddVarU)");
  }
  inline void AddVarF(TString aName, Double_t  input) {
    if     (HasVarF_ (aName)) SetVarF_ (aName, GetVarF_ (aName) + input);
    else if(HasVarD_ (aName)) SetVarD_ (aName, GetVarD_ (aName) + input);
    else                      AsrtVar(false,aName+" (AddVarF)");
  }
  // -----------------------------------------------------------------------------------------------------------
  inline void MultVarI(TString aName, Long64_t  input) {
    if     (HasVarI_ (aName)) SetVarI_ (aName, GetVarI_ (aName) * input);
    else if(HasVarS_ (aName)) SetVarS_ (aName, GetVarS_ (aName) * input);
    else if(HasVarL_ (aName)) SetVarL_ (aName, GetVarL_ (aName) * input);
    else                      AsrtVar(false,aName+" (MultVarI)");
  }
  inline void MultVarU(TString aName, ULong64_t input) {
    if     (HasVarUI_(aName)) SetVarUI_(aName, GetVarUI_(aName) * input);
    else if(HasVarUS_(aName)) SetVarUS_(aName, GetVarUS_(aName) * input);
    else if(HasVarUL_(aName)) SetVarUL_(aName, GetVarUL_(aName) * input);
    else                      AsrtVar(false,aName+" (MultVarU)");
  }
  inline void MultVarF(TString aName, Double_t  input) {
    if     (HasVarF_ (aName)) SetVarF_ (aName, GetVarF_ (aName) * input);
    else if(HasVarD_ (aName)) SetVarD_ (aName, GetVarD_ (aName) * input);
    else                      AsrtVar(false,aName+" (MultVarF)" );
  }
  // -----------------------------------------------------------------------------------------------------------
  inline void AndVarB(TString aName, Bool_t    input) { SetVarB_ (aName, GetVarB_(aName) && input);  }
  inline void OrVarB (TString aName, Bool_t    input) { SetVarB_ (aName, GetVarB_(aName) || input);  }
  // -----------------------------------------------------------------------------------------------------------

  // counter access
  // -----------------------------------------------------------------------------------------------------------
  inline void resetCntr()                            { cntrMap->resetCntr();           }
  inline void clearCntr()                            { cntrMap->clearCntr();           }
  inline void NewCntr(TString  aName, int input = 0) { cntrMap->NewCntr(aName,input);  }
  inline void IncCntr(TString  aName, int val   = 1) { cntrMap->IncCntr(aName,val);    }
  inline void DecCntr(TString  aName, int val   = 1) { cntrMap->DecCntr(aName,val);    }
  inline int  GetCntr(TString  aName)                { return cntrMap->GetCntr(aName); }
  inline bool HasCntr(TString  aName)                { return cntrMap->HasCntr(aName); }
  inline void DelCntr(TString  aName)                { cntrMap->DelCntr(aName);        }
  
  inline void printCntr(TString nameTag = "", Log::LOGtypes logLevel = Log::INFO) {
    cntrMap->printCntr(nameTag,logLevel); return;
  }

private:
  // -----------------------------------------------------------------------------------------------------------
  // internal functions for variable manipulatios
  // -----------------------------------------------------------------------------------------------------------
  inline void NewVarB_ (TString aName, Bool_t    input) { verifyType(aName,"B");  DelVarB_ (aName); varB [aName] = input; hasB [aName] = true; }
  inline void NewVarC_ (TString aName, TString   input) { verifyType(aName,"C");  DelVarC_ (aName); varC [aName] = new TObjString(aName);
                                                                                          varC [aName]->SetString(input); hasC [aName] = true; } 
  inline void NewVarS_ (TString aName, Short_t   input) { verifyType(aName,"S");  DelVarS_ (aName); varS [aName] = input; hasS [aName] = true; }
  inline void NewVarI_ (TString aName, Int_t     input) { verifyType(aName,"I");  DelVarI_ (aName); varI [aName] = input; hasI [aName] = true; }
  inline void NewVarL_ (TString aName, Long64_t  input) { verifyType(aName,"L");  DelVarL_ (aName); varL [aName] = input; hasL [aName] = true; }
  inline void NewVarUS_(TString aName, UShort_t  input) { verifyType(aName,"US"); DelVarUS_(aName); varUS[aName] = input; hasUS[aName] = true; }
  inline void NewVarUI_(TString aName, UInt_t    input) { verifyType(aName,"UI"); DelVarUI_(aName); varUI[aName] = input; hasUI[aName] = true; }
  inline void NewVarUL_(TString aName, ULong64_t input) { verifyType(aName,"UL"); DelVarUL_(aName); varUL[aName] = input; hasUL[aName] = true; }
  inline void NewVarF_ (TString aName, Float_t   input) { verifyType(aName,"F");  DelVarF_ (aName); varF [aName] = input; hasF [aName] = true; }
  inline void NewVarD_ (TString aName, Double_t  input) { verifyType(aName,"D");  DelVarD_ (aName); varD [aName] = input; hasD [aName] = true; }
  inline void NewForm_ (TString aName, TString   input) { verifyType(aName,"FM"); DelForm_ (aName); varFM[aName] = input; hasFM[aName] = true; }
  
  // -----------------------------------------------------------------------------------------------------------
  inline void DelVarB_ (TString aName) { checkLock(aName); if(HasVarB_ (aName)) {                     varB .erase(aName); hasB .erase(aName); } }
  inline void DelVarC_ (TString aName) { checkLock(aName); if(HasVarC_ (aName)) { delete varC[aName]; varC .erase(aName); hasC .erase(aName); } }
  inline void DelVarS_ (TString aName) { checkLock(aName); if(HasVarS_ (aName)) {                     varS .erase(aName); hasS .erase(aName); } }
  inline void DelVarI_ (TString aName) { checkLock(aName); if(HasVarI_ (aName)) {                     varI .erase(aName); hasI .erase(aName); } }
  inline void DelVarL_ (TString aName) { checkLock(aName); if(HasVarL_ (aName)) {                     varL .erase(aName); hasL .erase(aName); } }
  inline void DelVarUS_(TString aName) { checkLock(aName); if(HasVarUS_(aName)) {                     varUS.erase(aName); hasUS.erase(aName); } }
  inline void DelVarUI_(TString aName) { checkLock(aName); if(HasVarUI_(aName)) {                     varUI.erase(aName); hasUI.erase(aName); } }
  inline void DelVarUL_(TString aName) { checkLock(aName); if(HasVarUL_(aName)) {                     varUL.erase(aName); hasUL.erase(aName); } }
  inline void DelVarF_ (TString aName) { checkLock(aName); if(HasVarF_ (aName)) {                     varF .erase(aName); hasF .erase(aName); } }
  inline void DelVarD_ (TString aName) { checkLock(aName); if(HasVarD_ (aName)) {                     varD .erase(aName); hasD .erase(aName); } }  
  inline void DelForm_ (TString aName) { checkLock(aName); if(HasForm_ (aName)) {                     varFM.erase(aName); hasFM.erase(aName); } }  
  
  // -----------------------------------------------------------------------------------------------------------
  inline void SetVarB_ (TString aName, Bool_t    input, bool asrtVar = true) { if(asrtVar) AsrtVar(HasVarB_ (aName),aName+" (SetVarB)" ); varB [aName] = input; }
  inline void SetVarC_ (TString aName, TString   input, bool asrtVar = true) { if(asrtVar) AsrtVar(HasVarC_ (aName),aName+" (SetVarC)" ); varC [aName]->SetString(input); }
  inline void SetVarS_ (TString aName, Short_t   input, bool asrtVar = true) { if(asrtVar) AsrtVar(HasVarS_ (aName),aName+" (SetVarS)" ); varS [aName] = input; }
  inline void SetVarI_ (TString aName, Int_t     input, bool asrtVar = true) { if(asrtVar) AsrtVar(HasVarI_ (aName),aName+" (SetVarI)" ); varI [aName] = input; }
  inline void SetVarL_ (TString aName, Long64_t  input, bool asrtVar = true) { if(asrtVar) AsrtVar(HasVarL_ (aName),aName+" (SetVarL)" ); varL [aName] = input; }
  inline void SetVarUS_(TString aName, UShort_t  input, bool asrtVar = true) { if(asrtVar) AsrtVar(HasVarUS_(aName),aName+" (SetVarUS)"); varUS[aName] = input; }
  inline void SetVarUI_(TString aName, UInt_t    input, bool asrtVar = true) { if(asrtVar) AsrtVar(HasVarUI_(aName),aName+" (SetVarUI)"); varUI[aName] = input; }
  inline void SetVarUL_(TString aName, ULong64_t input, bool asrtVar = true) { if(asrtVar) AsrtVar(HasVarUL_(aName),aName+" (SetVarUL)"); varUL[aName] = input; }
  inline void SetVarF_ (TString aName, Float_t   input, bool asrtVar = true) { if(asrtVar) AsrtVar(HasVarF_ (aName),aName+" (SetVarF)" ); varF [aName] = input; }
  inline void SetVarD_ (TString aName, Double_t  input, bool asrtVar = true) { if(asrtVar) AsrtVar(HasVarD_ (aName),aName+" (SetVarD)" ); varD [aName] = input; }
  inline void SetForm_ (TString aName, TString   input, bool asrtVar = true) { if(asrtVar) AsrtVar(HasForm_ (aName),aName+" (SetVarFM)"); varFM[aName] = input; }
  
  // -----------------------------------------------------------------------------------------------------------  
  inline Bool_t    GetVarB_ (TString aName) { AsrtVar(HasVarB_ (aName),aName+" (GetVarB)" ); return varB [aName];           }
  inline TString   GetVarC_ (TString aName) { AsrtVar(HasVarC_ (aName),aName+" (GetVarC)" ); return varC [aName]->String(); }
  inline Short_t   GetVarS_ (TString aName) { AsrtVar(HasVarS_ (aName),aName+" (GetVarS)" ); return varS [aName];           }
  inline Int_t     GetVarI_ (TString aName) { AsrtVar(HasVarI_ (aName),aName+" (GetVarI)" ); return varI [aName];           }
  inline Long64_t  GetVarL_ (TString aName) { AsrtVar(HasVarL_ (aName),aName+" (GetVarL)" ); return varL [aName];           }
  inline UShort_t  GetVarUS_(TString aName) { AsrtVar(HasVarUS_(aName),aName+" (GetVarUS)"); return varUS[aName];           }
  inline UInt_t    GetVarUI_(TString aName) { AsrtVar(HasVarUI_(aName),aName+" (GetVarUI)"); return varUI[aName];           }
  inline ULong64_t GetVarUL_(TString aName) { AsrtVar(HasVarUL_(aName),aName+" (GetVarUL)"); return varUL[aName];           }
  inline Float_t   GetVarF_ (TString aName) { AsrtVar(HasVarF_ (aName),aName+" (GetVarF)" ); return varF [aName];           }
  inline Double_t  GetVarD_ (TString aName) { AsrtVar(HasVarD_ (aName),aName+" (GetVarD)" ); return varD [aName];           }

  // -----------------------------------------------------------------------------------------------------------
  inline bool HasVarB_ (TString aName) { if (hasB [aName]) return true; else { hasB .erase(aName); return false; } } //return (varB .find(aName) != varB .end());
  inline bool HasVarC_ (TString aName) { if (hasC [aName]) return true; else { hasC .erase(aName); return false; } } //return (varC .find(aName) != varC .end());
  inline bool HasVarS_ (TString aName) { if (hasS [aName]) return true; else { hasS .erase(aName); return false; } } //return (varS .find(aName) != varS .end());
  inline bool HasVarI_ (TString aName) { if (hasI [aName]) return true; else { hasI .erase(aName); return false; } } //return (varI .find(aName) != varI .end());
  inline bool HasVarL_ (TString aName) { if (hasL [aName]) return true; else { hasL .erase(aName); return false; } } //return (varL .find(aName) != varL .end());
  inline bool HasVarUS_(TString aName) { if (hasUS[aName]) return true; else { hasUS.erase(aName); return false; } } //return (varUS.find(aName) != varUS.end());
  inline bool HasVarUI_(TString aName) { if (hasUI[aName]) return true; else { hasUI.erase(aName); return false; } } //return (varUI.find(aName) != varUI.end());
  inline bool HasVarUL_(TString aName) { if (hasUL[aName]) return true; else { hasUL.erase(aName); return false; } } //return (varUL.find(aName) != varUL.end());
  inline bool HasVarF_ (TString aName) { if (hasF [aName]) return true; else { hasF .erase(aName); return false; } } //return (varF .find(aName) != varF .end());
  inline bool HasVarD_ (TString aName) { if (hasD [aName]) return true; else { hasD .erase(aName); return false; } } //return (varD .find(aName) != varD .end());
  inline bool HasForm_ (TString aName) { if (hasFM[aName]) return true; else { hasFM.erase(aName); return false; } } //return (varFM.find(aName) != varFM.end());
  
  // -----------------------------------------------------------------------------------------------------------
  void verifyType(TString aName, TString varType) {
    if(HasVar(aName)) AsrtVar((GetVarType(aName) == varType), (TString)"Var"+varType+" with name = "+aName+" also defined as Var"+GetVarType(aName));
    return;
  }
  void AsrtVar(bool cond, TString name) { 
    VERIFY(LOCATION,(TString)"Tried to get non existing Var (\""+name+"\") ...",cond);
    return;
  }
  void checkLock(TString aName) { 
    if(dynamic_cast<TTree*>(treeRead)||dynamic_cast<TTree*>(treeWrite)) {
      AsrtVar(false,(TString)"Tried to add \""+aName+"\" with tree already defined !");
    }
    return;
  }

  // // -----------------------------------------------------------------------------------------------------------
  // inline bool IsDefVarC_ (TString aName) { return (          varC [aName]->String() == DefOpts::DefC        ); }
  // inline bool IsDefVarS_ (TString aName) { return (          varS [aName]           == DefOpts::DefS        ); }
  // inline bool IsDefVarI_ (TString aName) { return (          varI [aName]           == DefOpts::DefI        ); }
  // inline bool IsDefVarL_ (TString aName) { return (          varL [aName]           == DefOpts::DefL        ); }
  // inline bool IsDefVarUS_(TString aName) { return (          varUS[aName]           == DefOpts::DefUS       ); }
  // inline bool IsDefVarUI_(TString aName) { return (          varUI[aName]           == DefOpts::DefUI       ); }
  // inline bool IsDefVarUL_(TString aName) { return (          varUL[aName]           == DefOpts::DefUL       ); }
  // inline bool IsDefVarF_ (TString aName) { return (std::fabs(varF [aName]            - DefOpts::DefF ) < EPS); }
  // inline bool IsDefVarD_ (TString aName) { return (std::fabs(varD [aName]            - DefOpts::DefD ) < EPS); }
};

#endif
