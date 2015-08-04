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

#ifndef __COMMONINCLUDE_H__
#define __COMMONINCLUDE_H__

// -----------------------------------------------------------------------------------------------------------
// standard includes:
#include <stdio.h>
#include <limits>
#include <cmath>
#include <cstdlib>
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

// root includes:
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

// using namespace std;
// using namespace TMath;

using std::cout;
using std::endl;
using std::vector;
using std::map;
using std::pair;
using std::min;
using std::max;

// #include "RVersion.h"
// #if ROOT_VERSION_CODE >= 393218  // 393218 is for ROOT_RELEASE "6.00/02"
// #endif // ROOT_VERSION_CODE > 393218

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

#define Map unordered_map
// #define Map map // can use this instead to go back to std::map


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
  public:
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
    //os << std::string(level > DEBUG ? level - DEBUG : 0, '\t');
    return os;
  }

  inline std::ostringstream& MyLog::Clean(std::string str) {
    if(str != "") { os << "[" << NowTime() << " " << str << "] "; }
    return os;
  }

  inline MyLog::~MyLog() {
    //os << std::endl;
    fprintf(stderr, "%s", os.str().c_str());
    fflush(stderr);
  }

  inline LOGtypes & MyLog::ReportingLevel() {
    static LOGtypes reportingLevel = DEBUG_4;
    return reportingLevel;
  }

  inline std::string MyLog::ToString(LOGtypes level) {
    static const char* const buffer[] = {"  ERROR", "WARNING", "   INFO", "  DEBUG", "DEBUG_1", "DEBUG_2", "DEBUG_3", "DEBUG_4"};
//static const char* const buffer[] = {" ERR", "WARN", "INFO", " DBG", "DBG1", "DBG2", "DBG3", "DBG4"};
    return buffer[level];
  }

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



#ifndef __MY_DEFINES__
#define __MY_DEFINES__

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
    do { delete ptr; ptr = NULL; } while(false)

  // a version with some verbosity for debugging
  #define DELNULL_(loc,ptr,name,verb) \
    do { \
      if(verb) { aCustomLOG("DELNULL")<<coutGreen<<loc<<coutRed<<" - deleting "<<coutBlue<<name<<coutRed<<" ("<<coutPurple<<ptr<<coutRed<<") ..."<<coutDef<<endl; } \
      delete ptr; ptr = NULL; \
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






