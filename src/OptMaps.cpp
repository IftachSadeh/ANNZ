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

#include "OptMaps.hpp"

// ===========================================================================================================
// namespace for default options/variable values
// =============================================
namespace DefOpts {
  Bool_t    DefB  = false;                                           Bool_t    NullB  = false;
  TString   DefC  = "";                                              TString   NullC  = "";
  Short_t   DefS  = std::numeric_limits<short int>         ::max();  Short_t   NullS  = 0;
  Int_t     DefI  = std::numeric_limits<int>               ::max();  Int_t     NullI  = 0;
  Long64_t  DefL  = std::numeric_limits<long int>          ::max();  Long64_t  NullL  = 0;
  UShort_t  DefUS = std::numeric_limits<unsigned short int>::max();  UShort_t  NullUS = 0;
  UInt_t    DefUI = std::numeric_limits<unsigned int>      ::max();  UInt_t    NullUI = 0;
  ULong64_t DefUL = std::numeric_limits<unsigned long int> ::max();  ULong64_t NullUL = 0;
  Float_t   DefF  = std::numeric_limits<float>             ::max();  Float_t   NullF  = 0;
  Double_t  DefD  = std::numeric_limits<double>            ::max();  Double_t  NullD  = 0;
}
// ===========================================================================================================
OptMaps::OptMaps(TString aName) {
// ==============================
  // set the name of the object
  name = aName;
  // initial lock status
  isLocked = false;
  // set the cout colour scheme
  setColors();
  return;
}
// ===========================================================================================================
OptMaps::~OptMaps() {
// ==================
  clearAll();
  return;
}
// ===========================================================================================================
void OptMaps::setColors() {
// ========================
  bool useCoutCol = OptOrNullB("useCoutCol");

  CT                = " \t ";
  coutDef           = useCoutCol ? "\033[0m"       : "";
  coutRed           = useCoutCol ? "\033[31m"      : "";
  coutGreen         = useCoutCol ? "\033[32m"      : "";
  coutBlue          = useCoutCol ? "\033[34m"      : "";
  coutLightBlue     = useCoutCol ? "\033[94m"      : "";
  coutYellow        = useCoutCol ? "\033[33m"      : "";
  coutPurple        = useCoutCol ? "\033[35m"      : "";
  coutCyan          = useCoutCol ? "\033[36m"      : "";
  coutUnderLine     = useCoutCol ? "\033[4;30m"    : "";
  coutWhiteOnBlack  = useCoutCol ? "\33[40;37;1m"  : "";
  coutWhiteOnRed    = useCoutCol ? "\33[41;37;1m"  : "";
  coutWhiteOnGreen  = useCoutCol ? "\33[42;37;1m"  : "";
  coutWhiteOnYellow = useCoutCol ? "\33[43;37;1m"  : "";

  return;
}

// ===========================================================================================================
void OptMaps::checkName(TString messageTag, TString aName) {
// =========================================================
  VERIFY(LOCATION,(TString)"Trying to define "+messageTag+" member with no name",(aName != ""));

  TString badExpr(""), message("");

  for(int nBadCharNow=0; nBadCharNow<100; nBadCharNow++) {
    if     (nBadCharNow == 0) badExpr = "'";
    else if(nBadCharNow == 1) badExpr = ";";
    else break;

    if(!aName.Contains(badExpr)) continue;

    message = (TString)"Trying to define "+messageTag+" member (\""+aName+"\") which includes a forbiden element (\""+badExpr+"\")";
    VERIFY(LOCATION,message,false);
  }

  return;    
};

// ===========================================================================================================
void OptMaps::copyOptStruct(OptMaps * inObj) {
// ===========================================
  // use NewOptB() in order to make sure the variable is replaced if it already exists, holding the current value of inObj
  for(Map <TString,bool>   ::iterator itr = inObj->optB.begin(); itr!=inObj->optB.end(); ++itr) { NewOptB(itr->first,itr->second); }
  for(Map <TString,int>    ::iterator itr = inObj->optI.begin(); itr!=inObj->optI.end(); ++itr) { NewOptI(itr->first,itr->second); }
  for(Map <TString,double> ::iterator itr = inObj->optF.begin(); itr!=inObj->optF.end(); ++itr) { NewOptF(itr->first,itr->second); }
  for(Map <TString,TString>::iterator itr = inObj->optC.begin(); itr!=inObj->optC.end(); ++itr) { NewOptC(itr->first,itr->second); }
  return;    
};

// ===========================================================================================================
void OptMaps::GetAllOptNames(vector <TString> & optNames, TString type) {
// ======================================================================
  optNames.clear();
  if(type == "ALL" || type == "B") { for(Map <TString,bool>   ::iterator itr = optB.begin(); itr!=optB.end(); ++itr) optNames.push_back(itr->first); }
  if(type == "ALL" || type == "I") { for(Map <TString,int>    ::iterator itr = optI.begin(); itr!=optI.end(); ++itr) optNames.push_back(itr->first); }
  if(type == "ALL" || type == "F") { for(Map <TString,double> ::iterator itr = optF.begin(); itr!=optF.end(); ++itr) optNames.push_back(itr->first); }
  if(type == "ALL" || type == "C") { for(Map <TString,TString>::iterator itr = optC.begin(); itr!=optC.end(); ++itr) optNames.push_back(itr->first); }
  return;    
};


// ===========================================================================================================
void OptMaps::printMap(Map <TString,TObjString*> & input, TString message, int nPrintRow, int width) {
// ===================================================================================================
  if(nPrintRow == 0) nPrintRow = 4; if(width == 0) width = 15;

  // example argument use: printOnlyPattern=MAGERR,SPECOBJ
  vector <TString> incPatrns;
  if(OptOrNullC("printOnlyPattern") != DefOpts::NullC) {
    TString            str = GetOptC("printOnlyPattern"); str.ReplaceAll(","," ");
    std::istringstream iss((std::string)str);
    do { std::string sub; iss >> sub; if(sub != "") incPatrns.push_back(sub); } while (iss);
  }
  int nIncPatts = (int)incPatrns.size();

  int nItr(0);
  for(Map <TString,TObjString*>::iterator itr = input.begin(); itr!=input.end(); ++itr, nItr++) {
    if(nItr == 0) aCleanLOG()<<coutYellow<<" - "<<message<<endl<<"  ";
    if(nIncPatts) {
      bool hasIncPatt(false);
      for(int nIncPattNow=0; nIncPattNow<nIncPatts; nIncPattNow++) if((itr->first).Contains(incPatrns[nIncPattNow])) hasIncPatt = true;
      if(!hasIncPatt) continue;
    }
    nItr++;
    aCleanLOG() <<coutBlue<<std::setw(width)<< itr->first << " = " <<coutGreen<<std::setw(width)<<std::left<< itr->second->String() <<std::right<< "  "<<coutDef;
    if(nItr%nPrintRow == 0) aCleanLOG()<<endl<<"  ";
  }
  if(nItr) aCleanLOG() <<coutDef<<endl;

  incPatrns.clear();
  return ;
};

// ===========================================================================================================
void OptMaps::printOpts(int nPrintRow, int width) {
// ================================================
  printMap(optB,TString::Format(name+": optB (%d)",optB.size()),nPrintRow,width);
  printMap(optC,TString::Format(name+": optC (%d)",optC.size()),nPrintRow,width);
  printMap(optI,TString::Format(name+": optI (%d)",optI.size()),nPrintRow,width);
  printMap(optF,TString::Format(name+": optF (%d)",optF.size()),nPrintRow,width);
  return ;
};

// ===========================================================================================================
void OptMaps::getOptPattern(TString type, vector <TString> & optV, TString pattern, bool ignorCase) {
// ==================================================================================================
  
  if(type == "B") getElePattern(optB,optV,pattern,ignorCase);
  if(type == "C") getElePattern(optC,optV,pattern,ignorCase);
  if(type == "I") getElePattern(optI,optV,pattern,ignorCase);
  if(type == "F") getElePattern(optF,optV,pattern,ignorCase);

  return;
}


// ===========================================================================================================
void OptMaps::setDefaultOpts() {
// =============================
  for(Map <TString,bool>   ::iterator itr = optB.begin(); itr!=optB.end(); ++itr) { SetOptB(itr->first,DefOpts::DefB); }
  for(Map <TString,int>    ::iterator itr = optI.begin(); itr!=optI.end(); ++itr) { SetOptI(itr->first,DefOpts::DefI); }
  for(Map <TString,double> ::iterator itr = optF.begin(); itr!=optF.end(); ++itr) { SetOptF(itr->first,DefOpts::DefD); }
  for(Map <TString,TString>::iterator itr = optC.begin(); itr!=optC.end(); ++itr) { SetOptC(itr->first,DefOpts::DefC); }
  return;
}
