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

#ifndef CntrMap_h
#define CntrMap_h

#include "commonInclude.hpp"
#include "OptMaps.hpp"
#include "Utils.hpp"

// ===========================================================================================================
class CntrMap {
// ============
  public:
    CntrMap(OptMaps * aOptMaps = NULL, Utils * aUtils = NULL, TString aName = "aCntrMap") {
      glob  = aOptMaps;
      utils = aUtils;
      name  = aName;
    };
    ~CntrMap() {
      clearCntr();
      return;
    };

  private:
    TString           name;
    map <TString,int> cntr;
    OptMaps           * glob;
    Utils             * utils;

  public:
    inline void resetCntr() { for(map <TString,int>::iterator itr = cntr.begin(); itr!=cntr.end(); ++itr) itr->second = 0; return; };
    inline void clearCntr() { cntr.clear(); return; };
    inline void copyCntr(CntrMap * cntrIn) {
      for(map <TString,int>::iterator itr = cntrIn->cntr.begin(); itr!=cntrIn->cntr.end(); ++itr) {
        cntr[itr->first] = itr->second;
      }
      return;
    };

    inline void NewCntr(TString aName, int input = 0) { cntr[aName] = input;                     return;                     }
    inline void IncCntr(TString aName, int val   = 1) { if(!HasCntr(aName)) NewCntr   (aName,0); cntr[aName] += val; return; }
    inline void DecCntr(TString aName, int val   = 1) { if(!HasCntr(aName)) NewCntr   (aName,0); cntr[aName] -= val; return; }
    inline int  GetCntr(TString aName)                { if(!HasCntr(aName)) NewCntr   (aName,0); return cntr[aName];         }
    inline bool HasCntr(TString aName)                { return (cntr.find(aName) != cntr.end());                             }
    inline void DelCntr(TString aName)                { if( HasCntr(aName)) cntr.erase(aName);   return;                     }

    void printCntr(TString nameTag = "", Log::LOGtypes logLevel = Log::INFO) {
      if(!inLOG(logLevel)) return;

      // time_t rawtime; time(&rawtime); struct tm * timeinfo = localtime ( &rawtime ); TString dateTimeNow = (TString) asctime(timeinfo);
      // size_t pos = std::string(dateTimeNow).find("CEST"); std::string choped = std::string(dateTimeNow).substr(0,pos);
      // dateTimeNow = (TString)choped; dateTimeNow.ReplaceAll("  "," "); dateTimeNow.ReplaceAll("\n","");
      // aLOG(logLevel)<<coutPurple<<"Counters ("<<dateTimeNow<<"):"<<coutDef<<endl;
      aLOG(logLevel)<<coutPurple<<" -- COUNTERS ----------------------------------------"
                    <<"--------------------------------------"<<coutDef<<endl; //-----------------

      int     maxLength = 0;
      TString baseName  = (TString)((nameTag == "") ? glob->OptOrNullC("baseName") : nameTag);

      for(map <TString,int>::iterator cutCounterItr = cntr.begin(); cutCounterItr!=cntr.end(); ++cutCounterItr) {
        TString str = TString::Format("%d",cutCounterItr->second);
        int     strLenght = str.Length();   if(maxLength < strLenght) maxLength = strLenght;
      }
      maxLength += 10;  if(maxLength < 20) maxLength = 20;
      for(map <TString,int>::iterator cutCounterItr = cntr.begin(); cutCounterItr!=cntr.end(); ++cutCounterItr) {
        TString cutNameNow  = cutCounterItr->first;
        int     cutCountNow = cutCounterItr->second;

        aLOG(logLevel)  <<coutPurple<<" -- "<<std::setw(35)<<std::left<<std::setfill('.')
                        <<TString(coutBlue+baseName+" "+coutPurple)<<""
                        <<std::setw(maxLength)<<std::setfill('.')<<std::right
                        <<TString(coutYellow)+TString::Format(" %d ",cutCountNow)<<std::setfill(' ')
                        <<" "<<coutGreen<<cutNameNow<<coutDef<<endl;
      }
      aLOG(logLevel)<<coutPurple<<" ----------------------------------------------------"
                    <<"-------------------------------------------------------"<<coutDef<<endl;
      
      return;
    };
};

#endif
