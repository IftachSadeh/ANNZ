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


// -----------------------------------------------------------------------------------------------------------
// common included libraries
// -----------------------------------------------------------------------------------------------------------
#ifndef __COMMONINCLUDE_H__
#define __COMMONINCLUDE_H__

#include <stdio.h>
#include <limits>
#include <cmath>
#include <cstdlib>
#include <sys/stat.h>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <stdlib.h>
#include <algorithm>
#include <numeric>
#include <vector>
#include <assert.h>
#include <map>
#include <set>

#include <TROOT.h>
#include <TSystem.h>
#include <TEnv.h>
#include <TStyle.h>
#include <TFile.h>
#include <TChain.h>
#include <TTreeFormula.h>
#include <TMath.h>
#include <TRandom3.h>
#include <TObjString.h>
#include <TString.h>
#include <TCut.h>
#include <TCanvas.h>
#include <TH1.h>
#include <TH2.h>
#include <TH3.h>
#include <TF1.h>
#include <TSpline.h>
#include <TProfile2D.h>
#include <TMultiGraph.h>
#include <TGraphErrors.h>
#include <TGraphAsymmErrors.h>
#include <TLine.h>
#include <TKey.h>
#include <TXMLEngine.h>

#include "TMVA/Tools.h"
#include "TMVA/Config.h"
#include "TMVA/Factory.h"
#include "TMVA/Reader.h"
#include "TMVA/MethodBase.h"
#include "TMVA/PDF.h"

#if ROOT_VERSION_CODE >= ROOT_VERSION(6,8,0) 
#include "TMVA/DataLoader.h"
#endif

// -----------------------------------------------------------------------------------------------------------
// hack to make all the private elements of MethodKNN accecible
// -----------------------------------------------------------------------------------------------------------
#define private public
#include "TMVA/MethodKNN.h"
#undef  private
// -----------------------------------------------------------------------------------------------------------


using std::cout;
using std::endl;
using std::vector;
using std::map;
using std::pair;
using std::min;
using std::max;

#include <unordered_map>
using std::unordered_map;

namespace std {
  template <>
  struct hash<TString> {
    std::size_t operator()(const TString & k) const {
      return std::hash<std::string>()(k.Data());
      // return std::hash<std::string>()((std::string)k);
    };
  };
}

// -----------------------------------------------------------------------------------------------------------
// namespace for default options/variable values - see http://root.cern.ch/root/html/Rtypes.h
// (needs namespace in order to use values in default initialization of OptMaps functions)
// -----------------------------------------------------------------------------------------------------------
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
// namespace for sorting logic functions
// ===========================================================================================================
namespace sortFunc {
  bool highToLowI(int a, int b) { return (a > b); }
  bool lowToHighI(int a, int b) { return (a < b); }

  bool highToLowD(double a, double b) { return (a > b); }
  bool lowToHighD(double a, double b) { return (a < b); }

  namespace pairID {
    // bool highToLowBy0 (pair<int,double> a, pair<int,double> b) { return (a.first  > b.first ); }
    // bool lowToHighBy0 (pair<int,double> a, pair<int,double> b) { return (a.first  < b.first ); }
    bool highToLowBy1(pair<int,double> a, pair<int,double> b) { return (a.second > b.second); }
    bool lowToHighBy1(pair<int,double> a, pair<int,double> b) { return (a.second < b.second); }

    struct equalToFirst {
      int b;
      equalToFirst(int input) : b(input) { }
      bool operator () (pair<int,double> const& a) { return (a.first == b); }
    };
  }
}



// -----------------------------------------------------------------------------------------------------------
// color output
// -----------------------------------------------------------------------------------------------------------
#define UseCoutCol true
namespace CoutCols {
  TString CT                = " \t ";
  TString coutDef           = UseCoutCol ? "\033[0m"      : "";
  TString coutRed           = UseCoutCol ? "\033[31m"     : "";
  TString coutGreen         = UseCoutCol ? "\033[32m"     : "";
  TString coutBlue          = UseCoutCol ? "\033[34m"     : "";
  TString coutLightBlue     = UseCoutCol ? "\033[94m"     : "";
  TString coutYellow        = UseCoutCol ? "\033[33m"     : "";
  TString coutPurple        = UseCoutCol ? "\033[35m"     : "";
  TString coutCyan          = UseCoutCol ? "\033[36m"     : "";
  TString coutUnderLine     = UseCoutCol ? "\033[4;30m"   : "";
  TString coutWhiteOnBlack  = UseCoutCol ? "\33[40;37;1m" : "";
  TString coutWhiteOnRed    = UseCoutCol ? "\33[41;37;1m" : "";
  TString coutWhiteOnGreen  = UseCoutCol ? "\33[42;37;1m" : "";
  TString coutWhiteOnYellow = UseCoutCol ? "\33[43;37;1m" : "";
}

using CoutCols::CT;
using CoutCols::coutDef;
using CoutCols::coutRed;
using CoutCols::coutGreen;
using CoutCols::coutBlue;
using CoutCols::coutLightBlue;
using CoutCols::coutYellow;
using CoutCols::coutPurple;
using CoutCols::coutCyan;
using CoutCols::coutUnderLine;
using CoutCols::coutWhiteOnBlack;
using CoutCols::coutWhiteOnRed;
using CoutCols::coutWhiteOnGreen;
using CoutCols::coutWhiteOnYellow;

#endif // __COMMONINCLUDE_H__


// -----------------------------------------------------------------------------------------------------------
// logging class, adapted from the code of Petru Marginean.
// See: http://www.drdobbs.com/cpp/logging-in-c/201804215?pgno=1
// -----------------------------------------------------------------------------------------------------------
#ifndef __MyLOG1_H__
#define __MyLOG1_H__

namespace Log {
  enum LOGtypes { ERROR, WARNING, INFO, DEBUG, DEBUG_1, DEBUG_2, DEBUG_3, DEBUG_4 };
  
  inline std::string NowTime() {
    char   buffer[80];
    time_t rawtime; time(&rawtime);
    struct tm * timeinfo = localtime(&rawtime);
    strftime (buffer,80,"%H:%M:%S",timeinfo);
    return buffer;
  };

  class MyLog {
    public:
      MyLog();
      virtual ~MyLog();
      std::ostringstream & Get(LOGtypes level = INFO);
      std::ostringstream & Clean(std::string str = "");

      static LOGtypes    & ReportingLevel();
      static std::string ToString(LOGtypes level);
      static LOGtypes    FromString(const std::string& level);
    protected:
      std::ostringstream os;
    private:
      MyLog(const MyLog&);
      MyLog & operator = (const MyLog &);
  };

  inline MyLog::MyLog(){};

  inline std::ostringstream& MyLog::Get(LOGtypes level) {
    os << "[" << NowTime() << " " << ToString(level) << "] ";
    return os;
  }

  inline std::ostringstream& MyLog::Clean(std::string str) {
    if(str != "") { os << "[" << NowTime() << " " << str << "] "; }
    return os;
  }

  inline MyLog::~MyLog() {
    fprintf(stderr, "%s", os.str().c_str());
    fflush(stderr);
  }

  inline LOGtypes & MyLog::ReportingLevel() {
    static LOGtypes reportingLevel = DEBUG_4;
    return reportingLevel;
  }

  static const char* const levelV[] = {
    "  ERROR", "WARNING", "   INFO", "  DEBUG", "DEBUG_1", "DEBUG_2", "DEBUG_3", "DEBUG_4"
  };
  inline std::string MyLog::ToString(LOGtypes level) { return levelV[level]; }

  inline LOGtypes MyLog::FromString(const std::string & level) {
    if     (level == "DEBUG_4") return DEBUG_4;
    else if(level == "DEBUG_3") return DEBUG_3;
    else if(level == "DEBUG_2") return DEBUG_2;
    else if(level == "DEBUG_1") return DEBUG_1;
    else if(level == "DEBUG")   return DEBUG;
    else if(level == "INFO")    return INFO;
    else if(level == "WARNING") return WARNING;
    else if(level == "ERROR")   return ERROR;
    else MyLog().Get(WARNING) << "Unknown logging level '" << level << "'. Using INFO level as default.";
    return INFO;
  }

  typedef MyLog theLog;
}

#define aLOG(level) \
  if(level <= Log::theLog::ReportingLevel()) Log::MyLog().Get(level)

#define aCleanLOG() \
  Log::MyLog().Clean()

#define aCustomLOG(str) \
  Log::MyLog().Clean(str)

#define inLOG(level) \
  (level <= Log::theLog::ReportingLevel())

#endif //__MyLOG1_H__


// -----------------------------------------------------------------------------------------------------------
// specific definitions of constants and macro functions
// -----------------------------------------------------------------------------------------------------------
#ifndef __MY_DEFINES__
#define __MY_DEFINES__

#define Map unordered_map
// #define Map map // can use this to go back to std::map instead of unordered_map

// general changes to TMVA affecting this code marked with ROOT_TMVA_V*
#define ROOT_TMVA_V0 \
  (ROOT_VERSION_CODE >= ROOT_VERSION(5,34,11) && ROOT_VERSION_CODE < ROOT_VERSION(6,8,0))
#define ROOT_TMVA_V1 \
  (ROOT_VERSION_CODE >= ROOT_VERSION(6,8,0))

// location and date/time string
#define LOCATION \
  (TString)TString::Format((TString)"FILE: "+__FILE__+" , LINE: %d , ("+__DATE__+" "+__TIME__+")",__LINE__)

// very small number
#define EPS \
  std::numeric_limits<double>::epsilon() 

// -----------------------------------------------------------------------------------------------------------
// "safe" delete/initialization macro for pointers
// ( the do {} while() suntax forces a semicolon to be placed after calls to DELNULL() )
// -----------------------------------------------------------------------------------------------------------
#define DELNULL(ptr) \
  do { \
    delete ptr; \
    ptr = NULL; \
  } while(false)

// a version with some verbosity for debugging
#define DELNULL_(loc,ptr,name,verb) \
  do { \
    if(verb) { aCustomLOG("DELNULL")<<coutRed<<" - "<<loc<<" - deleting \""<<coutYellow<<name<<coutRed<<"\" ("<<ptr<<") ..."<<coutDef<<endl; } \
    DELNULL(ptr); \
  } while(false)

// -----------------------------------------------------------------------------------------------------------
// verify a condition, send a message and terminate if failed
// ( the do {} while() suntax forces a semicolon to be placed after calls to VERIFY() )
// -----------------------------------------------------------------------------------------------------------
#define VERIFY(location,message,state) \
  do { if(!state) { \
    aLOG(Log::ERROR) <<coutWhiteOnBlack<<" - ... ------------------------------------------------------------ "<<coutDef<< endl; \
    aLOG(Log::ERROR) <<coutWhiteOnBlack<<coutRed<<" - MESSAGE - "<<coutWhiteOnBlack<<message <<" "             <<coutDef<< endl; \
    aLOG(Log::ERROR) <<coutWhiteOnBlack<<coutRed<<" - FROM    - "<<coutWhiteOnBlack<<location<<" "             <<coutDef<< endl; \
    aLOG(Log::ERROR) <<coutWhiteOnBlack<<coutRed<<" - ..... ABORTING !!! "                                     <<coutDef<< endl; \
    aLOG(Log::ERROR) <<coutWhiteOnBlack<<" ------------------------------------------------------------ ... - "<<coutDef<< endl; \
    exit(1); \
  } } while(false)

#define LINE_FILL(charFill,len) \
  std::setfill(charFill)<<std::setw(len)<<""<<std::setfill(' ')

#endif // __MY_DEFINES__






