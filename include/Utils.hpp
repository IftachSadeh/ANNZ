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

#ifndef Utils_h
#define Utils_h

#include "commonInclude.hpp"
#include "OptMaps.hpp"
#include <TKey.h>
#include <sys/stat.h>

// ===========================================================================================================
// namespace for sorting logic functions
// ===========================================================================================================
namespace sortFunctors {

  extern bool double_descend(double a, double b);
  extern bool double_ascend (double a, double b);
 
  extern bool pairIntInt_descendSecond(pair<int,int> a, pair<int,int> b);
  extern bool pairIntInt_ascendSecond (pair<int,int> a, pair<int,int> b);
  extern bool pairIntInt_descendFirst (pair<int,int> a, pair<int,int> b);
  extern bool pairIntInt_ascendFirst  (pair<int,int> a, pair<int,int> b);

  extern bool pairIntDouble_descendSecond(pair<int,double> a, pair<int,double> b);
  extern bool pairIntDouble_ascendSecond (pair<int,double> a, pair<int,double> b);
  extern bool pairIntDouble_descendFirst (pair<int,double> a, pair<int,double> b);
  extern bool pairIntDouble_ascendFirst  (pair<int,double> a, pair<int,double> b);
  
  struct pairIntDouble_equalFirst {
    int b;
    pairIntDouble_equalFirst(int input) : b(input) { }
    bool operator () (pair<int,double> const& a) { return (a.first == b); }
  };
  
}

// ===========================================================================================================
// namespace for fitting functions
// ===========================================================================================================
namespace fitFuncs {

  extern Double_t fitFuncByHisContent(Double_t * x, Double_t * par);

}

// ===========================================================================================================
class Utils {
// ==========

public:  
  Utils(OptMaps * aMaps = NULL);
  ~Utils();

  // functions
  // ===========================================================================================================
  inline bool     isNan   (double x) { volatile double d = x;    return d!=d;                             };
  inline bool     isInf   (double x) { if((x==x)&&((x-x)!= 0.0)) return true; return false;               };
  inline bool     isNanInf(double x) { if(isNan(x)) return true; if(isInf(x)) return true;  return false; };

  inline bool     isDirFile(TString inputName) { assert(!inputName.Contains("*")); 
                                                 struct stat sb; return ((stat(inputName, &sb) == 0 && S_ISDIR(sb.st_mode))); };
  
  inline TString  nextTreeFriendName(TTree * tree) { TList * friendList = tree->GetListOfFriends();
                                                     int nTreeFriends   = (dynamic_cast<TList*>(friendList)) ? friendList->GetEntries() : 0;
                                                     return TString::Format((TString)tree->GetName()+"_friend_%d",nTreeFriends);              };

  // see: http://www.cplusplus.com/reference/cstdio/printf/
  inline TString  boolToStr  (bool              input)                           { return (TString)(input ? "1" : "0");  };
  inline TString  intToStr   (int               input, TString format = "%d"   ) { return TString::Format(format,input); };
  inline TString  lIntToStr  (long int          input, TString format = "%ld"  ) { return TString::Format(format,input); };
  inline TString  uIntToStr  (unsigned int      input, TString format = "%u"   ) { return TString::Format(format,input); };
  inline TString  ULIntToStr (unsigned long int input, TString format = "%lu"  ) { return TString::Format(format,input); };
  inline TString  floatToStr (double            input, TString format = "%f"   ) { return TString::Format(format,input); };
  inline TString  doubleToStr(double            input, TString format = "%.10g") { return TString::Format(format,input); };

  Int_t     strToInt   (TString input);
  Long64_t  strToLong  (TString input);
  UInt_t    strToUint  (TString input);
  ULong64_t strToUlong (TString input);
  Float_t   strToFloat (TString input);
  Double_t  strToDouble(TString input);
  Bool_t    strToBool  (TString input);

  inline TString  getFilePath(TString fileName) { return (TString)fileName(0,fileName.Last('/'))+"/"; };

  vector<TTree*>  getTreeFriends(TTree * tree);

  void            getSetActiveTreeBranches(TTree * tree, vector < pair<TString,bool> > & branchNameStatusV, bool isGetStatus = true, bool verbose = false);
  void            getTreeBranchNames(TTree * tree, vector <TString> & branchNameV);

  void            flushHisBufferBinsZ(TH1 * his = NULL, int nBinsZ = 0);

  void     his2d_to_his1dV(OptMaps * optMap, TH1 * his2, vector <TH1*> & hisV);
  void     doPolyFit(TNamed * his, map < TString , double > * fitParMap, TString theFunc);
  void     doFitFuncByHisContent(OptMaps * fitOpts, TNamed * inputObject, vector <TH1*> & fitHisV);

  vector<std::string> & splitStringByChar(const std::string &s, char delim, vector<std::string> &elems);
  vector<std::string> splitStringByChar(const std::string &s, char delim);
  vector<TString>     splitStringByChar(TString s, char delim);
  TString             regularizeName(TString nameIn, TString newPattern = "");
  void                addTCuts(TCut & aCut0, TCut & aCut1);

  void    checkPathPrefix(TString pathName = "");
  void    validDirExists(TString dirName = "", bool verbose = false);
  bool    validFileExists(TString fileName = "", bool verif = true);
  void    resetDirectory(TString OutDirName = "", bool verbose = false, bool copyCode = false);
  void    checkCmndSafety(TString cmnd = "", bool verbose = false);
  void    safeRM(TString cmnd = "", bool verbose = false, bool checkExitStatus = true);
  TString getShellCmndOutput(TString cmnd = "", vector <TString> * outV = NULL, bool verbose = false, bool checkExitStatus = true, int * getSysReturn = NULL);
  int     exeShellCmndOutput(TString cmnd = "", bool verbose = false, bool checkExitStatus = true);
 
  TString cleanWeightExpr(TString wgtIn);
  bool    isSameWeightExpr(TString wgt0, TString wgt1);

  int     getNlinesAsciiFile(TString fileName, bool checkNonEmpty = true);
  int     getNlinesAsciiFile(vector<TString> & fileNameV, bool checkNonEmpty = true, vector <int> * nLineV = NULL);

  void    findObjPatternInCurrentDir(vector <TString> & patternV, vector <TString> & matchedObjV, TString clasType = "");
  void    getSortedArray(double * data, double *& sortedData);
  void    copyTreeUserInfo(TTree * inTree = NULL, TTree * outTree = NULL, bool debug = false);
  void    addToTreeUserInfoStr(TTree * tree = NULL, TString newInfo = "", bool debug = false);
  TString getTreeUserInfoStr(TTree * tree = NULL, bool debug = false);

  void    optToFromFile(vector<TString> * optNames = NULL, OptMaps * optMap = NULL, TString fileName = "",
                        TString writeReadStr = "READ", TString compOrOver = "AssertSame", bool debug = false);

  void    getNpoisson(vector <double> & data0, vector <double> & data1);
  void    getNpoisson(TH1 * data0, TH1 * data1);

  void    getKolmogorov(vector <double> & data0, vector <double> & data1);
  void    getKolmogorov(TH1 * data0, TH1 * data1);
  void    getKolmogorov(double * data0, double * data1);

  int     getQuantileV(vector <double> & fracV, vector <double> & quantV, double * dataArr, TH1 * dataHis);
  int     getQuantileV(vector <double> & fracV, vector <double> & quantV, vector <double> & dataArrV);
  int     getQuantileV(vector <double> & fracV, vector <double> & quantV, double * dataArr);
  int     getQuantileV(vector <double> & fracV, vector <double> & quantV, TH1 * dataHis); 
 
  int     getInterQuantileStats(double * dataArr, TH1 * dataHis);
  int     getInterQuantileStats(vector <double> & dataArrV);
  int     getInterQuantileStats(double * dataArr);
  int     getInterQuantileStats(TH1 * dataHis); 

  void    setColors();
  TString getdateDateTimeStr(time_t rawtime = 0);

  // variables
  // ===========================================================================================================
  TRandom3          * rnd;
  vector <int>      colours, markers, greens, blues, reds, fillStyles;   
  
  OptMaps            * glob, * param;
  
  TString            CT;
  TString            coutDef, coutRed, coutGreen, coutBlue, coutLightBlue, coutYellow, coutPurple, coutCyan,
                     coutUnderLine, coutWhiteOnBlack, coutWhiteOnRed, coutWhiteOnGreen, coutWhiteOnYellow;
};
#endif

