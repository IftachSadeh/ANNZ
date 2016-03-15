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

#ifndef OptMaps_h
#define OptMaps_h

#include "commonInclude.hpp"

// ===========================================================================================================
// namespace for default options/variable values - see http://root.cern.ch/root/html/Rtypes.h
// (needs namespace in order to use values in default initialization of OptMaps functions)
// =======================================================================================
namespace DefOpts {
  extern Bool_t    DefB,  NullB;  // comparable to bool
  extern Short_t   DefS,  NullS;  // comparable to short
  extern Int_t     DefI,  NullI;  // comparable to int
  extern Long64_t  DefL,  NullL;  // comparable to long long
  extern UShort_t  DefUS, NullUS; // comparable to Unsigned short
  extern UInt_t    DefUI, NullUI; // comparable to Unsigned int
  extern ULong64_t DefUL, NullUL; // comparable to unsigned long long
  extern Float_t   DefF,  NullF;  // comparable to float
  extern Double_t  DefD,  NullD;  // comparable to double
  extern TString   DefC,  NullC;
}

// ===========================================================================================================
class OptMaps {
// ============

public :
  OptMaps(TString aName = "anOptMaps");
  virtual ~OptMaps();
  
  // variables
  // -----------------------------------------------------------------------------------------------------------
  TString name, failSize;

protected:
  Map <TString,bool>    optB;
  Map <TString,int>     optI;
  Map <TString,double>  optF;
  Map <TString,TString> optC;

  bool                  isLocked;

public:
  TString   CT;
  TString   coutDef, coutRed, coutGreen, coutBlue, coutLightBlue, coutYellow, coutPurple, coutCyan,
            coutUnderLine, coutWhiteOnBlack, coutWhiteOnRed, coutWhiteOnGreen, coutWhiteOnYellow;

  // functions
  // -----------------------------------------------------------------------------------------------------------
  inline TString baseOutDirName() { return (TString)"./output/"; }; // this values must never be changed !!!
  inline TString baseInDirName()  { return (TString)"rootIn/";   }; // this values must never be changed !!!
  // base-prefix (may be defined by user) for general naming
  inline TString basePrefix()     { return (TString)(HasOptC("basePrefix")?GetOptC("basePrefix"):"basePrefix_"); };

  void                        setColors();
  
  void                        setLock(bool setStatus)  { isLocked = setStatus; return; }
  bool                        getLock()                { return isLocked;              }
  void                        checkLock(TString aName) { VERIFY(LOCATION,(TString)"Tried to change \""+aName+"\" in OptMaps which is locked",
                                                         !isLocked);           return; }

  void                        checkName(TString messageTag, TString aName);
  void                        copyOptStruct(OptMaps * inObj);
  void                        printMap(Map <TString,TObjString*> & input, TString message = "", int nPrintRow = 0, int width = 0);
  void                        printOpts(int nPrintRow = 0, int width = 0);
  inline virtual void         printMapAll(int nPrintRow = 0, int width = 0) { printOpts(nPrintRow,width); return; }
  inline void                 clearOpt()      { optB.clear(); optI.clear(); optF.clear(); optC.clear(); return; }
  inline virtual void         clearAll()      { clearOpt(); return; };
  void                        setDefaultOpts();

  void                        GetAllOptNames(vector <TString> & optNames, TString type = "ALL");
  void                        getOptPattern(TString type, vector <TString> & optV, TString pattern, bool ignorCase = false);

  // inline functions for variable manipulatios
  // -----------------------------------------------------------------------------------------------------------
  inline void     NewOptB(TString aName, bool    input = DefOpts::DefB) { checkLock(aName); checkName("OptMaps",aName); optB[aName] = input; return; }
  inline void     NewOptI(TString aName, int     input = DefOpts::DefI) { checkLock(aName); checkName("OptMaps",aName); optI[aName] = input; return; }
  inline void     NewOptF(TString aName, double  input = DefOpts::DefD) { checkLock(aName); checkName("OptMaps",aName); optF[aName] = input; return; }
  inline void     NewOptC(TString aName, TString input = DefOpts::DefC) { checkLock(aName); checkName("OptMaps",aName); optC[aName] = input; return; }
  
  inline void     DelOptB(TString aName) { if(HasOptB(aName)) { checkLock(aName); optB.erase(aName); } return; }
  inline void     DelOptI(TString aName) { if(HasOptI(aName)) { checkLock(aName); optI.erase(aName); } return; }
  inline void     DelOptF(TString aName) { if(HasOptF(aName)) { checkLock(aName); optF.erase(aName); } return; }
  inline void     DelOptC(TString aName) { if(HasOptC(aName)) { checkLock(aName); optC.erase(aName); } return; }

  inline void     SetOptB(TString aName, bool    input) { AsrtOpt(HasOptB(aName),aName); checkLock(aName); optB[aName] = input; return; }
  inline void     SetOptI(TString aName, int     input) { AsrtOpt(HasOptI(aName),aName); checkLock(aName); optI[aName] = input; return; }
  inline void     SetOptF(TString aName, double  input) { AsrtOpt(HasOptF(aName),aName); checkLock(aName); optF[aName] = input; return; }
  inline void     SetOptC(TString aName, TString input) { AsrtOpt(HasOptC(aName),aName); checkLock(aName); optC[aName] = input; return; }

  inline bool     GetOptB(TString aName) { AsrtOpt(HasOptB(aName),aName); return optB[aName]; }
  inline int      GetOptI(TString aName) { AsrtOpt(HasOptI(aName),aName); return optI[aName]; }
  inline double   GetOptF(TString aName) { AsrtOpt(HasOptF(aName),aName); return optF[aName]; }
  inline TString  GetOptC(TString aName) { AsrtOpt(HasOptC(aName),aName); return optC[aName]; }
  
  inline bool     OptOrNullB(TString aName) { return (HasOptB(aName) ? optB[aName] : DefOpts::NullB); }
  inline int      OptOrNullI(TString aName) { return (HasOptI(aName) ? optI[aName] : DefOpts::NullI); }
  inline double   OptOrNullF(TString aName) { return (HasOptF(aName) ? optF[aName] : DefOpts::NullD); }
  inline TString  OptOrNullC(TString aName) { return (HasOptC(aName) ? optC[aName] : DefOpts::NullC); }

  inline bool     HasOptB(TString aName) { return (optB.find(aName) != optB.end()); }
  inline bool     HasOptI(TString aName) { return (optI.find(aName) != optI.end()); }
  inline bool     HasOptF(TString aName) { return (optF.find(aName) != optF.end()); }
  inline bool     HasOptC(TString aName) { return (optC.find(aName) != optC.end()); }
  inline bool     HasOpt (TString aName) { return (HasOptB(aName) || HasOptI(aName) || HasOptF(aName) || HasOptC(aName)); }
  
  inline TString  GetOptType(TString aName) { TString type("");
                                              if(HasOptB(aName)) type += "B"; if(HasOptI(aName)) type += "I";
                                              if(HasOptF(aName)) type += "F"; if(HasOptC(aName)) type += "C";
                                              AsrtOpt(type.Length() < 2,"multiple definitions");    // sanity check - this really shouldn't happen!
                                              AsrtOpt(type.Length() > 0,"undefined opt requested");
                                              return type;
                                            };

  inline void     IncOptI(TString aName, int val = 1) { AsrtOpt(HasOptI(aName),aName); checkLock(aName); optI[aName] += val; }
  inline void     DecOptI(TString aName, int val = 1) { AsrtOpt(HasOptI(aName),aName); checkLock(aName); optI[aName] -= val; }

  void            AsrtOpt(bool cond, TString name) { VERIFY(LOCATION,(TString)"Something wrong with OptMaps member (\""+name+"\")",cond); return; }

  // -----------------------------------------------------------------------------------------------------------
  // template functions should be defined completely in the header file, to prevent linker problems
  // ===========================================================================================================
  template <typename T> void  printMap(Map <TString,T> & input, TString message = "", int nPrintRow = 0, int width = 0) {
  // ====================================================================================================================
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
    for(typename Map <TString,T>::iterator itr = input.begin(); itr!=input.end(); ++itr) {
      if(nItr == 0) aCleanLOG()<<endl<<coutYellow<<" - "<<message<<endl<<"  ";
      if(nIncPatts) {
        bool hasIncPatt(false);
        for(int nIncPattNow=0; nIncPattNow<nIncPatts; nIncPattNow++) if((itr->first).Contains(incPatrns[nIncPattNow])) hasIncPatt = true;
        if(!hasIncPatt) continue;
      }
      nItr++;
      aCleanLOG() <<coutBlue<<std::setw(width)<< itr->first << " = " <<coutGreen<<std::setw(width)<<std::left<< itr->second <<std::right<< "  "<<coutDef;
      if(nItr%nPrintRow == 0) aCleanLOG()<<endl<<"  ";
    }
    if(nItr) aCleanLOG() <<coutDef<<endl;

    incPatrns.clear();
    return ;
  };
  // ===========================================================================================================
  template <typename T> void  getElePattern(Map <TString,T> & input, vector <TString> & optVarV, TString pattern, bool ignorCase = false) {
  // ======================================================================================================================================
    for(typename Map <TString,T>::iterator itr = input.begin(); itr!=input.end(); ++itr) {
      bool  hasEle = (ignorCase) ? ((TString)itr->first).Contains(pattern,TString::kIgnoreCase)
                                 : ((TString)itr->first).Contains(pattern);

      if(hasEle) optVarV.push_back(itr->first);
    }
    return ;
  };

};


#endif
