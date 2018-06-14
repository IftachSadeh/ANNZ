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

#include "Utils.hpp"

// ===========================================================================================================
// namespace for fitting functions
// ===========================================================================================================
namespace fitFuncs {

  // ===========================================================================================================
  Double_t fitFuncByHisContent(Double_t * x, Double_t * par) {
  // =========================================================
    TH1     * his1(NULL);
    TString hisFitName("");
    double  binCont(0), retVal(0);

    for(int nHisFitNow=0; nHisFitNow<100; nHisFitNow++) {
      hisFitName  = TString::Format("fitFuncByHisContentHis_%d",nHisFitNow);
      his1        = (TH1*)gROOT->FindObject(hisFitName);  if(!dynamic_cast<TH1*>(his1)) break;
      binCont     = his1->GetBinContent(his1->FindBin(*x));
      retVal     += par[nHisFitNow] * binCont;
    }

    return  retVal;
  }
}


// ===========================================================================================================
Utils::Utils(OptMaps * aMaps) { 
// ============================
  glob  = aMaps;
  param = new OptMaps("param");

  tmpDirName = (TString)std::getenv("TMPDIR");
  tmpDirName.ReplaceAll(" ", "").ReplaceAll("\r\n", "").ReplaceAll("\n", "").ReplaceAll("\r", "");
  if     (tmpDirName == "")          tmpDirName = "/tmp/";
  else if(!tmpDirName.EndsWith("/")) tmpDirName += "/";

  struct stat info;
  bool   isValidDir(false);
  if(stat(std::getenv("TMPDIR"), &info) != 0) isValidDir = false;
  else if(info.st_mode & S_IFDIR)             isValidDir = true;
  else                                        isValidDir = false;
  if(!isValidDir) tmpDirName = "/tmp/";
  
  setColors();

  bool isLocked = glob->getLock(); if(isLocked) glob->setLock(false);

  validDirExists(glob->GetOptC("outDirNamePath")); 
  if(!glob->GetOptC("outDirNamePath").EndsWith("/")) glob->SetOptC("outDirNamePath",(TString)glob->GetOptC("outDirNamePath")+"/");

  if(!glob->HasOptC("UserInfoStr")) glob->NewOptC("UserInfoStr","UserInfo;");

  if(isLocked) glob->setLock(true);

  rnd = new TRandom3(0);

  return;
}
// ===========================================================================================================
Utils::~Utils() { 
// ==============
  DELNULL(param); DELNULL(rnd);
  colours.clear(); markers.clear(); greens.clear(); blues.clear(); reds.clear(); fillStyles.clear();
  return;
}
// ===========================================================================================================


// -----------------------------------------------------------------------------------------------------------
// Utils functions
// ===========================================================================================================
void Utils::setColors() {
// ======================
  fillStyles.clear();
  for(int i=0; i<5; i++) {
    fillStyles.push_back(3013); fillStyles.push_back(3016); fillStyles.push_back(3012);
    fillStyles.push_back(3010); fillStyles.push_back(3020); fillStyles.push_back(3021);
    fillStyles.push_back(3014);
  }

  markers.clear();
  for(int j=0; j<15; j++) { for(int i=0; i<10; i++) { int markNow = 20+i; if(i>=7) markNow += 5; markers.push_back(markNow); } }
  
  colours.clear();
  for(int i=0;i<15;i++){
    colours.push_back(kGray+3);     colours.push_back(kPink-3);     colours.push_back(kViolet+4);   colours.push_back(kOrange-2);
    colours.push_back(kSpring+4);   colours.push_back(kBlue+3);     colours.push_back(kMagenta-4);  colours.push_back(kTeal+9);          
    colours.push_back(kViolet+7);   colours.push_back(kOrange);     colours.push_back(kViolet-4);   colours.push_back(kAzure-3);
    colours.push_back(kSpring-6);   colours.push_back(kBlack);      colours.push_back(kRed-4);      colours.push_back(kOrange-7);
  }
  
  greens.clear(); blues.clear(); reds.clear();
  for(int i=0;i<10;i++){
    greens.push_back(kSpring-7);  greens.push_back(kSpring-8);  greens.push_back(kSpring+5);  greens.push_back(kSpring+2);
    greens.push_back(kTeal+2);    greens.push_back(kTeal-1);    greens.push_back(kSpring+7);
    
    blues.push_back(kAzure+3);    blues.push_back(kAzure-4);    blues.push_back(kAzure+10);   blues.push_back(kViolet+5);
    blues.push_back(kMagenta-8);  blues.push_back(kCyan-8);     blues.push_back(kCyan-4);
    
    reds.push_back(kPink-3);      reds.push_back(kRed-4);       reds.push_back(kOrange-3);    reds.push_back(kPink+9);
    reds.push_back(kOrange+8);    reds.push_back(kRed-7);       reds.push_back(kPink-8);
  }

  return;
}


// ===========================================================================================================
Int_t Utils::strToInt(TString input) {
// ===================================
  Int_t val(0);
  try         { val = stoi((std::string)input);                                                                           }
  catch (...) { VERIFY(LOCATION,(TString)" - Could not perform conversion from string to int for stoi("+input+")",false); }
  return val;
}
// ===========================================================================================================
Long64_t Utils::strToLong(TString input) {
// =======================================
  Long64_t val(0);
  try         { val = stoll((std::string)input);                                                                            }
  catch (...) { VERIFY(LOCATION,(TString)" - Could not perform conversion from string to long for stoll("+input+")",false); }
  return val;
}
// ===========================================================================================================
UInt_t Utils::strToUint(TString input) {
// =====================================
  UInt_t val(0);
  try         { val = stoul((std::string)input);                                                                                    }
  catch (...) { VERIFY(LOCATION,(TString)" - Could not perform conversion from string to insigned-int for stoul("+input+")",false); }
  return val;
}
// ===========================================================================================================
ULong64_t Utils::strToUlong(TString input) {
// =========================================
  ULong64_t val(0);
  try         { val = stoull((std::string)input);                                                                                     }
  catch (...) { VERIFY(LOCATION,(TString)" - Could not perform conversion from string to insigned-long for stoull("+input+")",false); }
  return val;
}
// ===========================================================================================================
Float_t Utils::strToFloat(TString input) {
// =======================================
  Float_t val(0);
  try         { val = stof((std::string)input);                                                                             }
  catch (...) { VERIFY(LOCATION,(TString)" - Could not perform conversion from string to float for stof("+input+")",false); }
  return val;
}
// ===========================================================================================================
Double_t Utils::strToDouble(TString input) {
// =========================================
  Double_t val(0);
  try         { val = stod((std::string)input);                                                                              }
  catch (...) { VERIFY(LOCATION,(TString)" - Could not perform conversion from string to double for stod("+input+")",false); }
  return val;
}
// ===========================================================================================================
Bool_t Utils::strToBool(TString input) {
// =====================================
  if     (input == "1" || input.EqualTo("true" ,TString::kIgnoreCase)) return true;
  else if(input == "0" || input.EqualTo("false",TString::kIgnoreCase)) return false;
  else VERIFY(LOCATION,(TString)"Unsupported input in strToBool(input = \""+input+"\")",false);
};

// ===========================================================================================================
void Utils::findObjPatternInCurrentDir(vector <TString> & patternV, vector <TString> & matchedObjV, TString clasType) {
// ====================================================================================================================

  matchedObjV.clear();
  int nPatterns = (int)patternV.size();
  if(nPatterns == 0 && clasType == 0) return; // nothing to be done...

  TKey  * key;
  TIter next(gDirectory->GetListOfKeys());
  while((key = (TKey*)next())) { 
    if (key->IsFolder()) continue; // if the Key is a folder read the directory and skip the rest of the block

    TClass  * clas    = gROOT->GetClass(key->GetClassName());
    TString clasName  = key->GetName();
    
    // check that the object is of the correct type (if given)
    if(clasType != "") { if(!clas->InheritsFrom(clasType)) continue; } //e.g., clas->InheritsFrom("TH1")

    // check that the object name matches the given patterns
    bool matchAll(true);
    for(int nPatternNow=0; nPatternNow<nPatterns; nPatternNow++) {
      if(patternV[nPatternNow] == "") continue;
      if(clasName.Contains(patternV[nPatternNow])) continue;
      matchAll = false;
      break;
    }
    if(matchAll) matchedObjV.push_back(clasName);  
  }

  return;
}

// ===========================================================================================================
vector<std::string> & Utils::splitStringByChar(const std::string &s, char delim, vector<std::string> &elems) {
// ===========================================================================================================

  std::string        item;
  std::stringstream  ss(s);
  while(getline(ss, item, delim)) elems.push_back(item);

  return elems;
}
vector<std::string> Utils::splitStringByChar(const std::string &s, char delim) {
// =============================================================================
    vector<std::string> elems;
    return splitStringByChar(s, delim, elems);
}
vector<TString> Utils::splitStringByChar(TString s, char delim) {
// ==============================================================
    vector<std::string> strV = splitStringByChar((std::string)s, delim);
    vector<TString>     tStrV;
    for(int i=0;i<(int)strV.size();i++) tStrV.push_back(strV[i]);
    strV.clear();
    return tStrV;
}

// ===========================================================================================================
void Utils::safeRM(TString cmnd, bool verbose, bool checkExitStatus) {
// ===================================================================

  checkCmndSafety(cmnd);
  int sysReturn = exeShellCmndOutput((TString)"rm -rf "+cmnd,verbose,false);
  
  if(checkExitStatus && sysReturn != 0) {
    TString junkDirName = (TString)tmpDirName+regularizeName(glob->basePrefix(),"")+"_junk/"
                          +((doubleToStr(rnd->Rndm(),"%.20f")).ReplaceAll("0.",""))+"/";

    aLOG(Log::WARNING) <<coutRed<<" - Could not execute command ["<<coutYellow<<(TString)"rm -rf "+cmnd<<coutRed
                       <<"] - will move to junk-dir, "<<coutPurple<<junkDirName<<coutRed<<" , instead ..."<<endl;

    exeShellCmndOutput((TString)"mkdir -p "+junkDirName+" ; mv "+cmnd+" "+junkDirName,verbose,checkExitStatus);
  }

  return;
}
// ===========================================================================================================
void Utils::checkCmndSafety(TString cmnd, bool verbose) {
// ======================================================
  if(verbose) aCustomLOG("       ")<<coutRed<<"checkCmndSafety("<<coutYellow<<cmnd<<coutRed<<") ..."<<coutDef<<endl;

  TString dirName      = cmnd(0,cmnd.Last('/'));
  bool    hasSpace     = cmnd.Contains(" ");
  bool    endsWithWild = cmnd.EndsWith("*");

  validDirExists(dirName); checkPathPrefix(cmnd); 

  VERIFY(LOCATION,(TString)"Can not safely use ("+cmnd+"), contains empty chars" ,!hasSpace);
  VERIFY(LOCATION,(TString)"Can not safely use ("+cmnd+"), ending with '*' for rm-command" ,!endsWithWild);

  return;
}
// ===========================================================================================================
void Utils::checkPathPrefix(TString pathName) {
// ============================================
  TString pathPrefix    = glob->baseOutDirName(); 
  bool    isValidPrefix = (pathPrefix == "./output/");
  bool    isValidPath   = (pathName != "") && (pathName != DefOpts::NullC) && pathName.BeginsWith(pathPrefix);
  bool    containsSpace = pathName.Contains(" ");
  TString strippedPath  = pathName; strippedPath.ReplaceAll("/","").ReplaceAll(".","");
  bool    isNotBasePath = (strippedPath != "output");

  VERIFY(LOCATION,(TString)"Trying use directory ("+pathName+"), which contains spaces" ,!containsSpace);
  VERIFY(LOCATION,(TString)"Trying use directory with invalid prefix ("+pathPrefix+"). Valid names must look like (./something)" ,isValidPrefix);
  VERIFY(LOCATION,(TString)"Trying use directory with invalid name ("+pathName+"). Valid names must begin with ("+pathPrefix+")" ,isValidPath);
  VERIFY(LOCATION,(TString)"Trying use directory ("+pathName+"), which is the same as baseOutDirName (./output/)" ,isNotBasePath);
  
  return;
}

// ===========================================================================================================
void Utils::resetDirectory(TString OutDirName, bool verbose, bool copyCode) {
// ==========================================================================
  if(glob->OptOrNullB("debugSysCmnd"))     verbose  = true;
  if(glob->OptOrNullB("copyCodeToOutDir")) copyCode = true;

  TString cmnd_Rm      = "rm -rf   ";
  TString cmnd_Mk      = "mkdir -p ";
  TString cmnd_Cp      = (TString)glob->GetOptC("copyCodeCmnd")+" ";  if(verbose) cmnd_Cp.ReplaceAll("rsync -R","rsync -Rv");
  TString codeBaseName = (TString)"sourceCode_"+getdateDateTimeStr(); codeBaseName.ReplaceAll(" ","_").ReplaceAll(":","_");
  TString codeDirName  = (TString)OutDirName+codeBaseName+"/";
  TString cmnd_Bz      = (TString)"cd "+codeDirName+".. ; tar cfj "+codeBaseName+".tar.bz2 "+codeBaseName;

  checkPathPrefix(OutDirName);

  exeShellCmndOutput((TString)cmnd_Rm+OutDirName,verbose);
  exeShellCmndOutput((TString)cmnd_Mk+OutDirName,verbose);

  if(copyCode && (glob->GetOptC("copyCodeCmnd") != "")) {
    exeShellCmndOutput((TString)cmnd_Mk+codeDirName,(verbose || inLOG(Log::DEBUG_2)));
    exeShellCmndOutput((TString)cmnd_Cp+codeDirName,verbose,false);
    exeShellCmndOutput((TString)cmnd_Bz,(verbose || inLOG(Log::DEBUG_2)));
    exeShellCmndOutput((TString)cmnd_Rm+codeDirName,(verbose || inLOG(Log::DEBUG_2)));
  }

  validDirExists(OutDirName);

  return;
}
// ===========================================================================================================
void Utils::validDirExists(TString dirName, bool verbose) {
// ========================================================
  if(glob->OptOrNullB("debugSysCmnd")) verbose = true;

  VERIFY(LOCATION,(TString)"Trying to create directory with empty name ("+dirName+")",(dirName != ""));
  
  TString sysCmnd   = (TString)"mkdir -p "+dirName;
  int     sysReturn = system(sysCmnd);
  if(verbose) aCustomLOG("       ")<<coutRed<<" - Sys-comnd (exit="<<sysReturn<<") : "<<coutBlue<<sysCmnd<<coutDef<<endl; 

  VERIFY(LOCATION,(TString)"Trying to create directory ("+dirName+") and got exit-status ("+intToStr(sysReturn)+")",(sysReturn == 0));

  return;
}
// ===========================================================================================================
bool Utils::validFileExists(TString fileName, bool verif) {
// ========================================================
  VERIFY(LOCATION,(TString)"Trying to check existance of file with empty name ("+fileName+")",(fileName != ""));

  std::ifstream inputFile(fileName,std::ios::in);
  bool isGood = inputFile.good();
  inputFile.close();

  if(verif) VERIFY(LOCATION,(TString)"Trying to access file \""+fileName+"\" which doesn't exist ...",isGood);

  return isGood;
}
// ===========================================================================================================
TString Utils::getShellCmndOutput(TString cmnd, vector <TString> * outV, bool verbose, bool checkExitStatus, int * getSysReturn) {
// ===============================================================================================================================
  if(glob->OptOrNullB("debugSysCmnd")) verbose = true;
  if(verbose) aCleanLOG()<<coutPurple<<" - Utils::getShellCmndOutput("<<coutYellow<<cmnd<<coutPurple<<") ..."<<coutDef<<endl;

  TString outStr("");
  FILE    * pipe(popen(cmnd, "r"));

  if(pipe) {
    char buffer[1024];

    while(!feof(pipe)) {
    if(fgets(buffer, sizeof(buffer), pipe) == NULL) continue;
      TString buffStr(buffer); buffStr.ReplaceAll("\n","").ReplaceAll("\r","");

      if(buffStr == "") continue;
      if(outStr  == "") outStr = buffStr;
      if(outV)          outV->push_back(buffStr);

      if(verbose) aCleanLOG()<<coutGreen<<"  -- Got("<<coutYellow<<((outV)?outV->size():0)<<coutGreen<<") "<<coutBlue<<buffStr<<coutDef<<endl;
    }    
    pclose(pipe);
  }

  int sysReturn = (outStr != "") ? 0 : 1;
  if(verbose)         aCustomLOG("       ")<<coutRed<<" - Sys-comnd (exit="<<sysReturn<<") : "<<coutBlue<<cmnd<<coutDef<<endl;
  if(checkExitStatus) VERIFY(LOCATION,(TString)" - Failed system-call ("+cmnd+") ...",(sysReturn == 0));
  if(getSysReturn)    (*getSysReturn) = sysReturn;

  return outStr;
}

// ===========================================================================================================
int Utils::exeShellCmndOutput(TString cmnd, bool verbose, bool checkExitStatus) {
// ==============================================================================
 if(glob->OptOrNullB("debugSysCmnd")) verbose = true;

 int sysReturn = system(cmnd);
 if(verbose)         aCustomLOG("       ")<<coutRed<<" - Sys-comnd (exit="<<sysReturn<<") : "<<coutBlue<<cmnd<<coutDef<<endl;
 if(checkExitStatus) VERIFY(LOCATION,(TString)" - Failed system-call ("+cmnd+") - sysReturn = "+intToStr(sysReturn)+"...",(sysReturn == 0));

 return sysReturn;
}

// ===========================================================================================================
TString Utils::cleanWeightExpr(TString wgtIn) {
// ============================================
  TString wgtOut(wgtIn);
  
  wgtOut.ReplaceAll(" ","");
  wgtOut.ReplaceAll("*()","").ReplaceAll("()*","");
  wgtOut.ReplaceAll("*(1)","").ReplaceAll("(1)*","");
  wgtOut.ReplaceAll("(1)","1");

  if(wgtOut == "") wgtOut = "1";

  return wgtOut;
}

// ===========================================================================================================
bool Utils::isSameWeightExpr(TString wgt0, TString wgt1) {
// =======================================================
  wgt0 = cleanWeightExpr(wgt0);
  wgt1 = cleanWeightExpr(wgt1);

  return ((wgt0 == wgt1) || (wgt0 == "" && wgt1 == "1") || (wgt0 == "1" && wgt1 == ""));
}

// ===========================================================================================================
int Utils::getNlinesAsciiFile(TString fileName, bool checkNonEmpty) {
// ==================================================================
  validFileExists(fileName,true);

  // get the number of lines (exclusing those which start with "#"
  TString cmnd    = (TString)"grep -v \"#\" "+fileName+" | wc -l  | head -1 | awk {'print $1'}";
  TString cmndOut = getShellCmndOutput(cmnd);
  int     nLines  = (cmndOut.IsDigit()) ? strToInt(cmndOut) : 0;

  if(checkNonEmpty) VERIFY(LOCATION,(TString)"found empty input file for command->output: ["+cmnd+"] -> ["+cmndOut+"] ...",(nLines > 0));

  return nLines;
}
// ===========================================================================================================
int Utils::getNlinesAsciiFile(vector<TString> & fileNameV, bool checkNonEmpty, vector <int> * nLineV) {
// ====================================================================================================
  TString inFileNames("");
  int     nLinesTot(0), nInFiles((int)fileNameV.size());

  for(int nInFileNow=0; nInFileNow<nInFiles; nInFileNow++) {
    int nLines   = getNlinesAsciiFile(fileNameV[nInFileNow],checkNonEmpty);
    nLinesTot   += nLines;
    inFileNames += (TString)fileNameV[nInFileNow]+" , ";

    if(nLineV) nLineV->push_back(nLines);

    aLOG(Log::INFO)<<coutGreen<<" - Found "<<coutYellow<<nLines<<coutGreen<<" lines in file "<<coutRed<<fileNameV[nInFileNow]
                                   <<coutGreen<<coutGreen<<" -> total so far = "<<coutYellow<<nLinesTot<<" ... "<<coutDef<<endl;
  }
  
  if(checkNonEmpty) VERIFY(LOCATION,(TString)"found no files, or only empty input files ("+inFileNames+") ...",(nLinesTot > 0));

  return nLinesTot;
}


// ===========================================================================================================
void Utils::optToFromFile(vector<TString> * optNames, OptMaps * optMap, TString fileName, TString writeReadStr, TString compOrOver, bool debug) {
// ==============================================================================================================================================
  if(debug && compOrOver == "SILENT_KeepFile") compOrOver = "WARNING_KeepFile";

  if(debug) {
    aCustomLOG("optFile")<<coutGreen
                         <<"-----------------------------------------------------------------------------------------------------------"<<coutDef<<endl;
    aCustomLOG("optFile")<<coutBlue<<" - starting optToFromFile("<<coutPurple<<fileName<<coutBlue<<") "<<" ... "<<coutDef<<endl;
    aCustomLOG("optFile")<<coutGreen
                         <<"-----------------------------------------------------------------------------------------------------------"<<coutDef<<endl;
  }


  VERIFY(LOCATION,(TString)"Must provide file name in optToFromFile(fileName = "+fileName+") ...",(fileName != ""));
  
  VERIFY(LOCATION,(TString)"Unrecognized option in optToFromFile(writeReadStr = "+writeReadStr+") "+
                           "allowed are \"WRITE\" or \"READ\" ...",(writeReadStr == "WRITE" || writeReadStr == "READ"));

  VERIFY(LOCATION,(TString)"Trying to use optToFromFile() with invalid pointer to optNames" ,dynamic_cast<vector<TString>*>(optNames));

  TString timeStrPrefix  = "# time(";
  
  // 
  vector < TString > alwaysOverWriteV;
  alwaysOverWriteV.push_back(glob->versionTag());

  // -----------------------------------------------------------------------------------------------------------
  // write the options given in optNames to fileName in a format like:
  //   nSplit="3";splitType="serial";maxValZ="-1";copyCodeToOutDir="TRUE"
  // -----------------------------------------------------------------------------------------------------------
  if(writeReadStr == "WRITE") {
    TString headerStr(""), cmnd(""), optTypeNow(""), optStr("");

    time_t timeNow; time(&timeNow);
    TString timeStr = (TString)timeStrPrefix+lIntToStr(static_cast<long int>(timeNow))+") - "+getdateDateTimeStr();
    
    headerStr = (TString) "# -----------------------------------------------------------------";
    cmnd      = (TString) "echo \'"+headerStr+"\' >  "+fileName; exeShellCmndOutput(cmnd,debug);
    headerStr = (TString) "# "+fileName;
    cmnd      = (TString) "echo \'"+headerStr+"\' >> "+fileName; exeShellCmndOutput(cmnd,debug);
    headerStr = (TString) timeStr;
    cmnd      = (TString) "echo \'"+headerStr+"\' >> "+fileName; exeShellCmndOutput(cmnd,debug);
    headerStr = (TString) "# -----------------------------------------------------------------";
    cmnd      = (TString) "echo \'"+headerStr+"\' >> "+fileName; exeShellCmndOutput(cmnd,debug);

    int nOpts = (int)optNames->size();
    for(int nOptNow=0; nOptNow<nOpts; nOptNow++) {
      TString optNameNow = optNames->at(nOptNow);

      VERIFY(LOCATION,(TString)"Found non-existing option name optNameNow = \""+optNameNow+
                               "\" in optNames in optToFromFile() ... somthing is wrong ...",(optMap->HasOpt(optNameNow)));

      optTypeNow = optMap->GetOptType(optNameNow);

      optStr     = (TString)"["+optNameNow + "]=";
      if     (optTypeNow == "B") optStr += (TString)  (optMap->GetOptB(optNameNow) ? "TRUE" : "FALSE");
      else if(optTypeNow == "I") optStr += intToStr   (optMap->GetOptI(optNameNow));
      else if(optTypeNow == "F") optStr += doubleToStr(optMap->GetOptF(optNameNow));
      else if(optTypeNow == "C") optStr += optMap->GetOptC(optNameNow);

      cmnd = (TString)"echo \'"+optStr+"\' >> "+fileName;
      exeShellCmndOutput(cmnd,debug);
    }
  }
  // -----------------------------------------------------------------------------------------------------------
  // read all options from fileName, checking for the correct format, and compare with the existing options
  // if (compOrOver == "COMPARE") then make sure the same option are defined in the file as in current memory
  // otherwise, overwrite the options in memory
  // -----------------------------------------------------------------------------------------------------------
  else if(writeReadStr == "READ") {
    VERIFY(LOCATION,(TString)"Unrecognized option in optToFromFile(compOrOver = "+compOrOver+")"+
                             " allowed are \"AssertSame\", \"SILENT_KeepFile\", \"WARNING_KeepFile\" or \"WARNING_KeepOrig\" ...",
                             ( compOrOver == "AssertSame"       || compOrOver == "SILENT_KeepFile"  ||
                               compOrOver == "WARNING_KeepFile" || compOrOver == "WARNING_KeepOrig"    ) );

    bool    assertSame = ( compOrOver == "AssertSame"                    ) ? true : false;
    bool    keepOrig   = ( compOrOver == "WARNING_KeepOrig"              ) ? true : false;
    bool    noWarning  = ( compOrOver == "SILENT_KeepFile"               ) ? true : false;
    bool    overWrite  = ( compOrOver == "WARNING_KeepFile" || noWarning ) ? true : false;

    // read the input files
    vector <TString> inputLineV;
    std::ifstream inputFile(fileName,std::ios::in); validFileExists(fileName);

    while(!inputFile.eof()) {
      std::string line;     getline(inputFile, line);
      TString     lineStr = (TString)line;
      lineStr.ReplaceAll("\r\n", "").ReplaceAll("\n", "").ReplaceAll("\r", ""); // remove line-breaks

      if(lineStr.BeginsWith("#")) {
        if(lineStr.BeginsWith(timeStrPrefix)) {
          Ssiz_t  time_posEnd   = lineStr.First(")");
          TString time_subStr0  = lineStr(0,time_posEnd+1);
          TString time_subStr1  = time_subStr0; time_subStr1.ReplaceAll(timeStrPrefix,"").ReplaceAll(")","");
          bool    time_isGood   = time_subStr0.EndsWith(")") && time_subStr1.IsDigit();

          VERIFY(LOCATION,(TString)"Can not interpret the time tag in "+fileName+" [\""+lineStr+"\"] ... "
                                  +"Expect format like: \""+timeStrPrefix+"1404848179) - Tue Jul 8 22:36:19 2014\"",time_isGood);

          time_t inputTime = static_cast<time_t>(strToLong(time_subStr1));
          optMap->NewOptF(glob->GetOptC("aTimeName"),inputTime);
        }
        continue;
      }
      if(lineStr != "") inputLineV.push_back(lineStr);
    }
    inputFile.close();

    int nOpts = (int)inputLineV.size();
    for(int nOptNow=0; nOptNow<nOpts; nOptNow++) {
      bool overWriteNow(overWrite);

      // derive the option-name and the value from each input line
      bool    checkOptFormat(false), isInOptNames(false), isSameOpt(false);
      TString subStr0(""), subStr1("");
      int     strLen(inputLineV[nOptNow].Length());
      for(int nPosNow=0; nPosNow<strLen; nPosNow++) {
        subStr0 = inputLineV[nOptNow](0,nPosNow);
        subStr1 = inputLineV[nOptNow](nPosNow,strLen);

        //cout <<subStr0<<CT<<CT<<subStr1<<endl;
        if(subStr0.BeginsWith("[") && subStr0.EndsWith("]") && subStr1.BeginsWith("=")) {
          subStr0        = subStr0(1,subStr0.Length()-2);
          subStr1        = subStr1(1,subStr1.Length()  );
          checkOptFormat = true;
          break;
        }
      }

      if(find(alwaysOverWriteV.begin(),alwaysOverWriteV.end(),subStr0) != alwaysOverWriteV.end()) {
        overWriteNow = true;
      }

      isInOptNames = (find(optNames->begin(),optNames->end(),subStr0) != optNames->end());

      if(!isInOptNames) {
        if(debug) aCustomLOG("optFile") <<coutYellow<<" - For \""<<inputLineV[nOptNow]<<"\""
                                        <<" -> did not find \""<<subStr0<<"\" in optNames and so will skip..."<<coutDef<<endl;
        continue;
      }

      if(!optMap->HasOpt(subStr0)) checkOptFormat = false;
      VERIFY(LOCATION,(TString)"Found unknown option format in: [ "+inputLineV[nOptNow]+" ] ...",checkOptFormat);

      TString origOptStr(""), optNameNow(subStr0), optValStrNow(subStr1), optTypeNow(optMap->GetOptType(optNameNow));

      if     (optTypeNow == "B") {
        bool valBol = ( optValStrNow == "TRUE" );
        isSameOpt   = ( valBol == optMap->GetOptB(optNameNow) );
        origOptStr  = (TString)(optMap->GetOptB(optNameNow) ? "TRUE" : "FALSE");

        if(debug) aCustomLOG("optFile") <<coutYellow<<" - Got(B) "<<optNameNow<<" -> \""<<valBol<<"\" compared to existing \""
                                        <<optMap->GetOptB(optNameNow)<<"\" --> isSame = "<<isSameOpt<<coutDef<<endl;
        
        if(overWriteNow) optMap->SetOptB(optNameNow,valBol);
        
        VERIFY(LOCATION,(TString)"Found wrong format of option: "+optNameNow+"=\""+optValStrNow+"\" , should be \"TRUE\" or \"FALSE\" ...",
                                 (optValStrNow == "TRUE" || optValStrNow == "FALSE"));
      }
      else if(optTypeNow == "I") {
        int valInt = strToInt(optValStrNow);
        isSameOpt  = ( valInt == optMap->GetOptI(optNameNow) );
        origOptStr = intToStr(optMap->GetOptI(optNameNow));

        if(debug) aCustomLOG("optFile") <<coutYellow<<" - Got(I) "<<optNameNow<<" -> \""<<valInt<<"\" compared to existing \""
                                        <<optMap->GetOptI(optNameNow)<<"\" --> isSame = "<<isSameOpt<<coutDef<<endl;
        
        if(overWriteNow) optMap->SetOptI(optNameNow,valInt);
      }
      else if(optTypeNow == "F") {
        double valDbl = strToDouble(optValStrNow);
        double relDif = fabs((valDbl - optMap->GetOptF(optNameNow))/valDbl);
        if     (fabs(valDbl) > EPS && fabs(optMap->GetOptF(optNameNow)) > EPS) isSameOpt = ( relDif < 1e-6 );
        else if(fabs(valDbl) < EPS && fabs(optMap->GetOptF(optNameNow)) < EPS) isSameOpt = true;
        else                                                                   isSameOpt = false;

        origOptStr = doubleToStr(optMap->GetOptF(optNameNow));

        if(debug) aCustomLOG("optFile") <<coutYellow<<" - Got(F) "<<optNameNow<<" -> \""<<valDbl<<"\" compared to existing \""
                                        <<optMap->GetOptF(optNameNow)<<"\" --> isSame = "<<isSameOpt
                                        <<"  (relative numerical difference is "<<relDif<<")"<<coutDef<<endl;
        
        if(overWriteNow) optMap->SetOptF(optNameNow,valDbl);
      }
      else if(optTypeNow == "C") {
        isSameOpt  = ( optValStrNow == optMap->GetOptC(optNameNow) );
        origOptStr = optMap->GetOptC(optNameNow);

        if(debug) aCustomLOG("optFile") <<coutYellow<<" - Got(C) "<<optNameNow<<" -> \""<<optValStrNow<<"\" compared to existing \""
                                        <<optMap->GetOptC(optNameNow)<<"\" --> isSame = "<<isSameOpt<<coutDef<<endl;
        
        if(overWriteNow) optMap->SetOptC(optNameNow,optValStrNow);
      }

      // -----------------------------------------------------------------------------------------------------------
      // if found inconsistent options, either exit with an error (AssertSame), warn and keep the original value (WARNING_KeepOrig)
      // or warn and set the value from the file (WARNING_KeepFile)
      // -----------------------------------------------------------------------------------------------------------
      if(!isSameOpt) {
        TString message = (TString)coutRed+"Found inconsistent options: "+coutGreen+optNameNow+coutRed+
                                   " is set from-file as: \""+coutBlue+optValStrNow+coutRed
                                   +"\" but currently set as: \""+coutPurple+origOptStr+coutRed+"\"."+coutDef;
        
        if(assertSame) { aLOG(Log::ERROR) <<message<<endl; VERIFY(LOCATION,"",false); }

        if(overWriteNow) message += (TString)coutRed+" Setting value from file !!!"+coutDef;
        if(keepOrig)     message += (TString)coutRed+" Keeping original value  !!!"+coutDef;

        if(!noWarning) aLOG(Log::WARNING) <<message<<endl;
      }
    }
    inputLineV.clear();
  }

  alwaysOverWriteV.clear();
  return;
}

// ===========================================================================================================
TString Utils::regularizeName(TString nameIn, TString newPattern) {
// ================================================================
  if(nameIn == "") return nameIn;

  TString nameOut(nameIn), regExp(":;$&%|!?*/+-[]=><()^,'.` ");
  if(newPattern != "") {
    for(int nRegExpNow=0; nRegExpNow<(int)regExp.Length(); nRegExpNow++) nameOut.ReplaceAll(regExp[nRegExpNow],newPattern);
  }

  nameOut.ReplaceAll( ":", "_COL_" );  nameOut.ReplaceAll( ";", "_SCOL_" );  nameOut.ReplaceAll( "$", "_S_" );
  nameOut.ReplaceAll( "&", "_A_" );    nameOut.ReplaceAll( "%", "_MOD_" );   nameOut.ReplaceAll( "|", "_O_" );
  nameOut.ReplaceAll( "!", "_O_" );    nameOut.ReplaceAll( "?", "_O_" );     nameOut.ReplaceAll( "*", "_T_" );
  nameOut.ReplaceAll( "/", "_D_" );    nameOut.ReplaceAll( "+", "_P_" );     nameOut.ReplaceAll( "-", "_M_" );
  nameOut.ReplaceAll( "[", "_" );      nameOut.ReplaceAll( "]", "_" );       nameOut.ReplaceAll( "=", "_E_" );
  nameOut.ReplaceAll( ">", "_GT_" );   nameOut.ReplaceAll( "<", "_LT_" );    nameOut.ReplaceAll( "(", "_" );
  nameOut.ReplaceAll( ")", "_" );      nameOut.ReplaceAll( "^", "_" );       nameOut.ReplaceAll( ",", "_" );
  nameOut.ReplaceAll( "'", "_" );      nameOut.ReplaceAll( "`", "_" );       nameOut.ReplaceAll( " ", "_" );
  nameOut.ReplaceAll( ".", "_" );

  return nameOut;
}


// ===========================================================================================================
TString Utils::getdateDateTimeStr(time_t rawtime) {
// ================================================
  // get the time in a reasonable formate
  if(rawtime == 0) time(&rawtime);
  struct tm * timeinfo = localtime ( &rawtime ); TString dateTimeNow = (TString) asctime(timeinfo);
  size_t pos = std::string(dateTimeNow).find("CEST"); std::string choped = std::string(dateTimeNow).substr(0,pos); dateTimeNow = (TString)choped;
  dateTimeNow.ReplaceAll("  "," "); dateTimeNow.ReplaceAll("\n","");

  return dateTimeNow;
}

// ===========================================================================================================
void Utils::addTCuts(TCut & aCut0, TCut & aCut1) {
// ===============================================
  TString cutStr0 = (TString)aCut0;  cutStr0.ReplaceAll(" ","");
  TString cutStr1 = (TString)aCut1;  cutStr1.ReplaceAll(" ","");
  
  if(cutStr1 == "")             return;
  if(cutStr0.Contains(cutStr1)) return;

  aCut0 += aCut1;

  return;
}

// ===========================================================================================================
void Utils::getCodeVersionV(vector <ULong64_t> & strV, TString versStrIn) {
// ==================================================================
  strV.clear();

  TString versStr((TString)((versStrIn == "") ? glob->GetOptC(glob->versionTag()) : versStrIn));
  versStr.ReplaceAll(glob->basePrefix(),"");

  vector <TString> versV = splitStringByChar(versStr,'.');
  for(int nVersOrder=0; nVersOrder<(int)versV.size(); nVersOrder++) {
    strV.push_back( strToUlong(versV[nVersOrder]) );
  }
  versV.clear();
  
  return;
}

// ===========================================================================================================
// return [1] if compVers is of a higher version, [0] if the same and [-1] if of a lower version
// ===========================================================================================================
int Utils::getCodeVersionDiff(TString compVers) {
// ==============================================  
  vector <ULong64_t> currentVersV, compVersV;
  getCodeVersionV(currentVersV);
  getCodeVersionV(compVersV,compVers);

  VERIFY(LOCATION,(TString)"Found incompatible version numbers - Current version is ["
                          +(glob->GetOptC(glob->versionTag())).ReplaceAll(glob->basePrefix(),"")
                          +"] and compared version is ["
                          +compVers.ReplaceAll(glob->basePrefix(),"")+"] ..."
                          ,(currentVersV.size() == compVersV.size()));

  int versDiff(0);
  for(int nVersOrder=0; nVersOrder<(int)currentVersV.size(); nVersOrder++) {
    if(compVersV[nVersOrder] > currentVersV[nVersOrder]) {
      versDiff = 1;
      break;
    }
    else if(compVersV[nVersOrder] < currentVersV[nVersOrder]) {
      versDiff = -1;
      break;
    }
  }

  currentVersV.clear(); compVersV.clear();
  return versDiff;
}

// ===========================================================================================================
vector <TTree*> Utils::getTreeFriends(TTree * tree) {
// ==================================================
  vector <TTree*> chainFriendV;

  TList * friendList = tree->GetListOfFriends();
  if(!dynamic_cast<TList*>(friendList)) return chainFriendV;

  TObjLink  * friends    = NULL;
  int       nTreeFriends = friendList->GetEntries();
  for(int nTreeNow=0; nTreeNow<nTreeFriends; nTreeNow++) {
    if(nTreeNow == 0) friends = tree->GetListOfFriends()->FirstLink();
    else              friends = friends->Next();
    
    TString friendName = ((TTree*)friends->GetObject())->GetName();
    chainFriendV.push_back((TTree*)tree->GetFriend(friendName));
  }

  return chainFriendV;
};

// ===========================================================================================================
void Utils::getSetActiveTreeBranches(TTree * tree, vector < pair<TString,bool> > & branchNameStatusV, TString getSet, bool verbose) {
// ==================================================================================================================================
  VERIFY(LOCATION,(TString)"Trying to use getSetActiveTreeBranches() with invalid tree" ,dynamic_cast<TTree*>(tree));

  if(verbose) {
    aCleanLOG()<<coutBlue<<"Starting getSetActiveTreeBranches("
               <<coutRed<<tree->GetName()<<" , "<<getSet<<coutBlue<<")..."<<coutDef<<endl;
  }

  if(getSet == "get") {
    // -----------------------------------------------------------------------------------------------------------
    // go over all branches from the tree and all it's friends, and store the status of each branch
    // -----------------------------------------------------------------------------------------------------------
    branchNameStatusV.clear();

    TList * friendList = tree->GetListOfFriends();
    int   nTreeFriends = (dynamic_cast<TList*>(friendList)) ? friendList->GetEntries() : 0;
    
    TObjLink * friends(NULL);  // = aChainTrainTest->GetListOfFriends()->FirstLink();
    for(int nTreeNow=0; nTreeNow<nTreeFriends+1; nTreeNow++) {
      TString treeName(tree->GetName());
      TTree * treeNow(NULL);
      if(nTreeNow == 0)   treeNow = tree;
      else {
        if(nTreeNow == 1) friends = tree->GetListOfFriends()->FirstLink();
        else              friends = friends->Next();
        
        treeName = ((TTree*)friends->GetObject())->GetName();
        treeNow  = (TTree*)tree->GetFriend(treeName);
        if(verbose) aCleanLOG()<<coutYellow<<"Now in tree-friend number "<<coutRed<<nTreeNow
                               <<coutYellow<<" with name: "<<coutRed<<treeName<<coutDef<<endl;
      }

      if(dynamic_cast<TTree*>(treeNow)) {
        TObjArray * brnchList = treeNow->GetListOfBranches();
        if(dynamic_cast<TObjArray*>(brnchList)) {
          for(int nBrnchNow=0; nBrnchNow<=brnchList->GetLast(); nBrnchNow++) {
            TBranch * aBranch  = (TBranch*)(brnchList->At(nBrnchNow));
            if(dynamic_cast<TBranch*>(aBranch)) {
              TString brnchName  = aBranch->GetName();
              bool    brnchStat  = treeNow->GetBranchStatus(brnchName);

              branchNameStatusV.push_back( pair<TString,bool>(brnchName,brnchStat) );

              if(verbose) aCleanLOG()<<coutGreen<<"getSetActiveTreeBranches() - Got "<<coutRed
                                     <<brnchName<<CT<<coutBlue<<brnchStat<<coutDef<<endl;
            }
            else aLOG(Log::WARNING) <<coutWhiteOnRed<<" --- no aBranch "<<nBrnchNow<<CT
                                    <<treeName<<CT<<treeNow->GetName()<<coutDef<<endl;
          }
        }
        else aLOG(Log::WARNING) <<coutWhiteOnRed<<" --- no brnchList "<<CT
                                <<treeName<<treeNow->GetName()<<coutDef<<endl;
      }
      else aLOG(Log::WARNING) <<coutWhiteOnRed<<" --- no tree "<<treeName<<coutDef<<endl;
    }
  }
  else if(getSet == "set") {
    // -----------------------------------------------------------------------------------------------------------
    // go over the list of branches and set the corresponding status of each
    // -----------------------------------------------------------------------------------------------------------
    int nBranches = (int)branchNameStatusV.size();
    for(int nBrnchNow=0; nBrnchNow<nBranches; nBrnchNow++) {
      TString brnchName  = branchNameStatusV[nBrnchNow].first;
      bool    brnchStat  = branchNameStatusV[nBrnchNow].second;
      
      tree->SetBranchStatus(brnchName,brnchStat);
       
      if(verbose) aCleanLOG()<<coutGreen<<"getSetActiveTreeBranches() - Set "<<coutRed
                             <<brnchName<<CT<<coutBlue<<brnchStat<<coutDef<<endl;
    }
  }
  else {
    VERIFY(LOCATION,(TString)"Trying to use getSetActiveTreeBranches() with invalid option (getSet = \'"+getSet+"\')" ,false);
  }

  return;
}

// ===========================================================================================================
void Utils::getTreeBranchNames(TTree * tree, vector <TString> & branchNameV) {
// ===========================================================================
  VERIFY(LOCATION,(TString)"Trying to use getTreeBranchNames() with invalid tree" , dynamic_cast<TTree*>(tree));

  branchNameV.clear();

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
    }

    TObjArray * brnchList = treeNow->GetListOfBranches(); if(!dynamic_cast<TObjArray*>(brnchList)) continue;

    for(int nBrnchNow=0; nBrnchNow<=brnchList->GetLast(); nBrnchNow++) {
      TBranch * aBranch  = (TBranch*)(brnchList->At(nBrnchNow));
      TString brnchName  = aBranch->GetName();

      // skip double entries
      if(find(branchNameV.begin(),branchNameV.end(),brnchName) != branchNameV.end()) continue;

      branchNameV.push_back(brnchName);
    }
  }

  return;
}

// ===========================================================================================================
void Utils::copyTreeUserInfo(TTree * inTree, TTree * outTree, bool debug) {
// ========================================================================
  VERIFY(LOCATION,(TString)"Trying to use copyTreeUserInfo() with invalid trees" ,(dynamic_cast<TTree*>(inTree) && dynamic_cast<TTree*>(outTree)));

  TTree * aTreeIn     = dynamic_cast<TChain*>(inTree)  ? inTree ->GetTree() : inTree;
  TTree * aTreeOut    = dynamic_cast<TChain*>(outTree) ? outTree->GetTree() : outTree;
  TList * userInfoIn  = aTreeIn ->GetUserInfo();
  TList * userInfoOut = aTreeOut->GetUserInfo();
  
  VERIFY(LOCATION,(TString)"Could not do GetUserInfo() from trees... something is horribly wrong ?!?!"
         ,(dynamic_cast<TList*>(userInfoIn) && dynamic_cast<TList*>(userInfoOut)));

  int nUserInfoEntries = userInfoIn->GetEntries();
  for(int nUserEntryNow=0; nUserEntryNow<nUserInfoEntries; nUserEntryNow++) {
    TObject * useEntry = userInfoIn->At(nUserEntryNow);
    userInfoOut->Add((TObject*)useEntry->Clone());    
    
    if(debug && dynamic_cast<TObjString*>(useEntry))
      aCleanLOG()<<coutBlue<<"Added to tree "<<coutRed<<outTree->GetName()
                        <<coutBlue<<" user-info: "<<coutYellow<<((TObjString*)useEntry)->String()<<coutDef<<endl;
  }

  return;
}

// ===========================================================================================================
void Utils::addToTreeUserInfoStr(TTree * tree, TString newInfo, bool debug) {
// ==========================================================================
  VERIFY(LOCATION,(TString)"Trying to use addToTreeUserInfoStr() with invalid tree" ,dynamic_cast<TTree*>(tree));

  TTree * aTreeIn   = dynamic_cast<TChain*>(tree) ? tree ->GetTree() : tree;
  TList * userInfo  = aTreeIn ->GetUserInfo();

  VERIFY(LOCATION,(TString)"Could not do GetUserInfo() from tree... something is horribly wrong ?!?!" ,(dynamic_cast<TList*>(userInfo)));

  bool hasSetStr        = false;
  int  nUserInfoEntries = userInfo->GetEntries();
  for(int nUserEntryNow=0; nUserEntryNow<nUserInfoEntries; nUserEntryNow++) {
    TObject * useEntry = userInfo->At(nUserEntryNow);

    if(!dynamic_cast<TObjString*>(useEntry)) continue;

    TString strNow = ((TObjString*)useEntry)->String();
    if(strNow != "") strNow += ";";
    ((TObjString*)useEntry)->SetString(strNow+newInfo);
    hasSetStr = true;    
  }
  if(!hasSetStr) {
    TString    basicName    = glob->GetOptC("UserInfoStr"); 
    TObjString * theUsrInfo = new TObjString(basicName+newInfo);
    userInfo->Add(theUsrInfo);
  }

  if(debug || inLOG(Log::DEBUG_1))
    aCustomLOG("DBG1")<<coutBlue<<" - added to tree "<<coutRed<<tree->GetName()<<coutBlue<<" user-info: "<<coutYellow<<newInfo<<coutDef<<endl;
  
  return;
}

// ===========================================================================================================
TString Utils::getTreeUserInfoStr(TTree * tree, bool debug) {
// =======================================================
  VERIFY(LOCATION,(TString)"Trying to use getTreeUserInfoStr() with invalid tree" ,dynamic_cast<TTree*>(tree));

  int     nFoundStr = 0;
  TString outStr    = "";
  TTree * aTreeIn   = dynamic_cast<TChain*>(tree) ? tree ->GetTree() : tree;
  TList * userInfo  = aTreeIn ->GetUserInfo();

  VERIFY(LOCATION,(TString)"Could not do GetUserInfo() from tree... something is horribly wrong ?!?!" ,(dynamic_cast<TList*>(userInfo)));

  int nUserInfoEntries = userInfo->GetEntries();
  for(int nUserEntryNow=0; nUserEntryNow<nUserInfoEntries; nUserEntryNow++) {
    TObject * useEntry = userInfo->At(nUserEntryNow);

    if(!dynamic_cast<TObjString*>(useEntry)) continue;

    TString strNow = ((TObjString*)useEntry)->String();
    if(!strNow.BeginsWith(glob->GetOptC("UserInfoStr"))) continue;

    outStr = strNow;
    nFoundStr++;  
  }
  VERIFY(LOCATION,(TString)"In getTreeUserInfoStr(), tree("+tree->GetName()+") has more than one string user-info defined",(nFoundStr < 2));

  if(debug || inLOG(Log::DEBUG_1))
    aCustomLOG("DBG1")<<coutBlue<<" - found in tree "<<coutRed<<tree->GetName()<<coutBlue<<" user-info: "<<coutYellow<<outStr<<coutDef<<endl;
  
  return outStr;
}


// ===========================================================================================================
int Utils::drawTree(TTree * tree, TString drawExprs, TString wgtCut) {
// ===================================================================
  VERIFY(LOCATION,(TString)"Trying to use drawTree() with invalid tree" ,dynamic_cast<TTree*>(tree));
  VERIFY(LOCATION,(TString)"Trying to use drawTree() with empty drawExprs" ,(drawExprs != ""));

  bool debug(inLOG(Log::DEBUG_2));
  vector < pair<TString,bool> > branchNameStatusV;
  getSetActiveTreeBranches(tree, branchNameStatusV, "get", debug);
  tree->SetBranchStatus("*",1);

  TString cnvsName  = (TString)tree->GetName()+"_tmpCnvs_"+(doubleToStr(rnd->Rndm(),"%.20f"));
  TCanvas * tmpCnvs = new TCanvas(cnvsName,cnvsName);
  
  int nEvt = tree->Draw(drawExprs,wgtCut);
  
  DELNULL(tmpCnvs);
  
  getSetActiveTreeBranches(tree, branchNameStatusV, "set", debug);
  branchNameStatusV.clear();
  
  return nEvt;
}

// ===========================================================================================================
void Utils::flushHisBufferBinsZ(TH1 * his, int nBinsZ) {
// =====================================================
  VERIFY(LOCATION,(TString)"Trying to use flushHisBufferBinsZ() with invalid histogram" , dynamic_cast<TH1*>(his));
  VERIFY(LOCATION,(TString)"Trying to use flushHisBufferBinsZ() with invalid number of bins" ,(nBinsZ > 0));

  if(glob->OptOrNullB((TString)"hasFlushed_"+his->GetName())) { his->BufferEmpty(); return; }
  else  glob->NewOptB((TString)"hasFlushed_"+his->GetName(),true);

  if(dynamic_cast<TH3*>(his)) {
    his->BufferEmpty();
    TH3 * his3 = (TH3*)his->Clone("TMPhis");

    double xLim0 = his3->GetXaxis()->GetBinLowEdge(his3->GetXaxis()->GetFirst());
    double xLim1 = his3->GetXaxis()->GetBinUpEdge (his3->GetXaxis()->GetLast ());
    double yLim0 = his3->GetYaxis()->GetBinLowEdge(his3->GetYaxis()->GetFirst());
    double yLim1 = his3->GetYaxis()->GetBinUpEdge (his3->GetYaxis()->GetLast ());
    double zLim0 = -0.5;
    double zLim1 = nBinsZ-0.5;
    
    his->GetXaxis()->SetLimits(xLim0,xLim1); his->GetYaxis()->SetLimits(yLim0,yLim1); his->GetZaxis()->SetLimits(zLim0,zLim1);
    his->BufferEmpty();
    DELNULL(his3);
  }
  else if(dynamic_cast<TH2*>(his)) {
    TH2 * his2 = (TH2*)his->Clone("TMPhis");
    his2->BufferEmpty();

    double xLim0 = his2->GetXaxis()->GetBinLowEdge(his2->GetXaxis()->GetFirst());
    double xLim1 = his2->GetXaxis()->GetBinUpEdge (his2->GetXaxis()->GetLast ());
    double yLim0 = -0.5;
    double yLim1 = nBinsZ-0.5;
    
    his->GetXaxis()->SetLimits(xLim0,xLim1); his->GetYaxis()->SetLimits(yLim0,yLim1);
    his->BufferEmpty();
    DELNULL(his2);
  }
  else assert(false);

  return;
}

// ===========================================================================================================
void Utils::doPolyFit(TNamed * inputObject, map < TString , double > * fitParMap, TString theFunc) {
// ===================================================================================================
 
  TString hisName("");
  TH1     * his  = (dynamic_cast<TH1*>   (inputObject));
  TGraph  * grph = (dynamic_cast<TGraph*>(inputObject));
  VERIFY(LOCATION,(TString)"Trying to use doPolyFit() without a valid TH1 or TGraph object... Something is horribly wrong !!!",(his || grph));
  
  if(his) his->BufferEmpty();

  gStyle->SetOptFit(0000);
  int      fitStatus(-1);
  TString  fitOpt     = "+rQ";   //fitOpt += "MEV";

  TF1 * fitFunc1 = new TF1(regularizeName(theFunc), theFunc, (*fitParMap)["fitBottom"], (*fitParMap)["fitTop"]);
  
  if(fitFunc1 == NULL){
    cout << endl;
    cout << coutRed     <<"==========================================================================="      << endl;
    cout << coutGreen   <<"Fit to " << theFunc << endl;
    cout << coutGreen   <<"Fit failed !!! " << endl;
    cout << coutRed     <<"==========================================================================="      << endl;
    cout << coutDef << endl;
    return;
  }

  fitFunc1->SetLineWidth(2);

  TString cnvsName = "tmpCnvs";   TCanvas * tmpCnvs = new TCanvas(cnvsName,cnvsName,712,23,500,500);
  if(his)  { fitStatus = his ->Fit(fitFunc1,fitOpt); hisName = his->GetName();  }
  if(grph) { fitStatus = grph->Fit(fitFunc1,fitOpt); hisName = grph->GetName(); }
  DELNULL(tmpCnvs);

  (*fitParMap)["fitStatus"]     = fitStatus;
  (*fitParMap)["fitChiSquare"]  = fitFunc1->GetChisquare();
  (*fitParMap)["fitNDF"]        = (double)fitFunc1->GetNDF();

  for(int nParNow=0; nParNow<fitFunc1->GetNpar(); nParNow++) {
    (*fitParMap)[TString::Format("fitPar_%d",nParNow)] = fitFunc1->GetParameter(nParNow);
  }

  if((*fitParMap)["VERBOSE"]) {
    cout << endl;
    cout << coutRed     <<"==========================================================================="          << endl;
    cout << coutGreen   <<"Fit to           " <<  theFunc                                                        << endl;
    cout << coutGreen   <<"hisName       =  " <<  hisName                                                        << endl;
    cout << coutBlue    <<"fit status    =  " <<  fitStatus                                                      << endl;
    for(int nParNow=0; nParNow<fitFunc1->GetNpar(); nParNow++)
      cout << coutBlue    <<"fit par "<<nParNow<<"     =  " <<(*fitParMap)[TString::Format("fitPar_%d",nParNow)] << endl;
    cout << coutRed     <<"==========================================================================="          << endl;
    cout << coutDef << endl;
  }

  DELNULL(fitFunc1);
}

// ===========================================================================================================
// create histograms (in memory) named fitFuncByHisContentHis_0, fitFuncByHisContentHis_1,... and set
// fitOpts->GetOptI("nHisFit") to the number of histograms (as a safety measure, to avoid these his
// existing by accident in memory from a previous fit...)
// ===========================================================================================================
void Utils::doFitFuncByHisContent(OptMaps * fitOpts, TNamed * inputObject, vector <TH1*> & fitHisV) {
// ==================================================================================================
  int     nHisFits = (int)fitHisV.size(); if(nHisFits == 0) return;
  int     fitStatus(-1);
  TString fitFuncName("fitFuncByHisContent"), fitOpt("R0M");  if(!fitOpts->OptOrNullB("doVerboseFit")) fitOpt += "Q";
  gStyle->SetOptFit(0000);

  TH1 * hisIn = dynamic_cast<TH1*>(inputObject);
  VERIFY(LOCATION,(TString)"Trying to use doFitFuncByHisContent() without a valid hisIn object... Something is horribly wrong !!!",(hisIn));

  hisIn = (TH1*)hisIn->Clone("fitFuncByHisContentHisToFit");
  
  // cleanup possible old fir histograms
  for(int nHisFitNow=0; nHisFitNow<100; nHisFitNow++) {
    TString hisFitName = TString::Format("fitFuncByHisContentHis_%d",nHisFitNow);
    TH1     * his1     = (TH1*)gROOT->FindObject(hisFitName);

    DELNULL(his1);
  }
  // registed the current fit histograms with the correct names
  vector <TH1*>  fitHisClnV(nHisFits);
  for(int nHisFitNow=0; nHisFitNow<nHisFits; nHisFitNow++) {
    TString hisFitName     = TString::Format("fitFuncByHisContentHis_%d",nHisFitNow);
    fitHisClnV[nHisFitNow] = (TH1*)fitHisV[nHisFitNow]->Clone(hisFitName);
  }

  // top and bottom fit range
  double  fitBottom(0), fitTop(1);
  if(fitOpts->OptOrNullF("fitBottom") < fitOpts->OptOrNullF("fitTop")) {
    fitBottom = fitOpts->GetOptF("fitBottom");  fitTop = fitOpts->GetOptF("fitTop");
  }

  TF1 * fitFunc = new TF1(fitFuncName,fitFuncs::fitFuncByHisContent,fitBottom,fitTop,nHisFits);
  fitFunc->SetLineStyle(2);  fitFunc->SetLineWidth(2); fitFunc->SetParameter(0,1);

  // parameter limits
  for(int nHisFitNow=0; nHisFitNow<nHisFits; nHisFitNow++) {
    TString optName = TString::Format("parLimits_%d",nHisFitNow);
    if(fitOpts->OptOrNullC(optName) == "") continue;

    vector <TString> parLimitsV = splitStringByChar(fitOpts->GetOptC(optName),':');
    fitFunc->SetParLimits(nHisFitNow,parLimitsV[0].Atof(),parLimitsV[1].Atof());
    parLimitsV.clear();
  }

  TString cnvsName = "tmpCnvs";           TCanvas * tmpCnvs = new TCanvas(cnvsName,cnvsName);
  fitStatus = hisIn->Fit(fitFunc,fitOpt); DELNULL(tmpCnvs);

  fitOpts->NewOptI("fitStatus",          fitStatus);
  fitOpts->NewOptI("fitNDF",             fitFunc->GetNDF());
  fitOpts->NewOptF("fitChiSquare",       fitFunc->GetChisquare());
  for(int nHisFitNow=0; nHisFitNow<nHisFits; nHisFitNow++) {
    TString hisFitName = TString::Format("_%d",nHisFitNow);
    fitOpts->NewOptF((TString)"fitParVal"+hisFitName,  fitFunc->GetParameter(nHisFitNow));
    fitOpts->NewOptF((TString)"fitParErr"+hisFitName,  fitFunc->GetParError(nHisFitNow));
  }

  if(fitOpts->OptOrNullB("doVerboseFit")) {
    cout << endl;
    cout << coutRed     <<"==========================================================================="        << endl;
    cout << coutGreen   <<" - hisName        =  " <<  inputObject->GetName()                                   << endl;
    cout << coutBlue    <<" - fit status     =  " <<  fitStatus                                                << endl;
    cout << coutBlue    <<" - NDF            =  " <<  fitOpts->GetOptI("fitNDF")                               << endl;
    cout << coutBlue    <<" - ChiSquare      =  " <<  fitOpts->GetOptF("fitChiSquare")                         << endl;
    for(int nHisFitNow=0; nHisFitNow<nHisFits; nHisFitNow++) {
      TString hisFitName = TString::Format("_%d",nHisFitNow);
      cout<<coutYellow<<" - fitPar("<<nHisFitNow<<") =  " << fitOpts->GetOptF((TString)"fitParVal"+hisFitName) <<" (+-) "
                                                          << fitOpts->GetOptF((TString)"fitParErr"+hisFitName) << endl;
    }
    cout << coutRed     <<"==========================================================================="        << endl;
    cout << coutDef     << endl;
  }

  DELNULL(hisIn); DELNULL(fitFunc);
  for(int nHisFitNow=0; nHisFitNow<nHisFits; nHisFitNow++) DELNULL(fitHisClnV[nHisFitNow]);

  return;
}

// ===========================================================================================================
void Utils::his2d_to_his1dV(OptMaps * optMap, TH1 * his2, vector <TH1*> & hisV) {
// ==============================================================================
  VERIFY(LOCATION,(TString)"Trying to use his2d_to_his1dV() without a valid his2 object... Something is horribly wrong !!!",(his2));
  his2->BufferEmpty();

  bool  isInvAxis(false);
  TAxis * xAxis(his2->GetXaxis()), * yAxis(his2->GetYaxis());
  if(optMap) {
    if(optMap->OptOrNullC("invertAxes") == "XY") {
      xAxis = his2->GetYaxis(); yAxis = his2->GetXaxis(); isInvAxis = true;
    }
  }

  TString hisName2d = (TString)his2->GetName();
  int     nBinsX    = xAxis->GetNbins();
  int     nBinsY    = yAxis->GetNbins();
  double  lowEdgeX  = xAxis->GetBinLowEdge( xAxis->GetFirst() );
  double  lowEdgeY  = yAxis->GetBinLowEdge( yAxis->GetFirst() );
  double  highEdgeX = xAxis->GetBinUpEdge ( xAxis->GetLast()  );
  double  highEdgeY = yAxis->GetBinUpEdge ( yAxis->GetLast()  );

  for(int nBinNowY=1; nBinNowY<nBinsY+1; nBinNowY++) {
    double  binCenterY = yAxis->GetBinCenter(nBinNowY);
    if(binCenterY < lowEdgeY || binCenterY > highEdgeY) continue;

    TString hisName1d = (TString)hisName2d+TString::Format("_bin%d",nBinNowY-1);  
    TString hisTitle  = (TString)his2->GetTitle()+TString::Format(" bin%d",nBinNowY-1);

    if(optMap) {
      if(optMap->OptOrNullC("parseTitleRange") != "") {
        TString yAxisTitle = yAxis->GetTitle();
        double  lowEdge    = yAxis->GetBinLowEdge(nBinNowY);
        double  highEdge   = yAxis->GetBinUpEdge(nBinNowY);
        hisTitle = (TString)TString::Format(optMap->GetOptC("parseTitleRange"),lowEdge)
                           +" < " + yAxisTitle
                           +" < " + TString::Format(optMap->GetOptC("parseTitleRange"),highEdge);
      }
      if(optMap->OptOrNullC("parseTitleCenter") != "") {
        TString yAxisTitle = yAxis->GetTitle();
        int     binCenter  = (int)yAxis->GetBinCenter(nBinNowY);

        hisTitle = (TString)yAxisTitle + " = " + TString::Format(optMap->GetOptC("parseTitleCenter"),binCenter);
      }
    }

    TH1 * his1(NULL);
    if     (dynamic_cast<TH2D*>(his2)) {
      if(xAxis->GetXbins()->GetSize() > 0)  his1 = new TH1D(hisName1d,hisName1d,nBinsX,xAxis->GetXbins()->GetArray());
      else                                  his1 = new TH1D(hisName1d,hisName1d,nBinsX,lowEdgeX,highEdgeX);
    }
    else if(dynamic_cast<TH2F*>(his2)) {
      if(xAxis->GetXbins()->GetSize() > 0)  his1 = new TH1F(hisName1d,hisName1d,nBinsX,xAxis->GetXbins()->GetArray());
      else                                  his1 = new TH1F(hisName1d,hisName1d,nBinsX,lowEdgeX,highEdgeX);
    }
    else VERIFY(LOCATION,(TString)" - Unsupported histogram type (expected TH2D or TH1D) for "+his2->GetName(),false);

    hisV.push_back(his1);

    TString binLabel = (TString)yAxis->GetBinLabel(nBinNowY);
    if(binLabel == "") his1->SetTitle(hisTitle);  else his1->SetTitle(binLabel);

    if(optMap) {
      if(optMap->HasOptC("axisTitleX")) his1->GetXaxis()->SetTitle(optMap->GetOptC("axisTitleX"));
      if(optMap->HasOptC("axisTitleY")) his1->GetYaxis()->SetTitle(optMap->GetOptC("axisTitleY"));
      if(optMap->HasOptC("axisTitleZ")) his1->GetZaxis()->SetTitle(optMap->GetOptC("axisTitleZ"));
    }

    // fill 1d histograms from a single bin of the 2d histograms
    for(int nBinNowX=1; nBinNowX<nBinsX+1; nBinNowX++) {
      double binCenterX = xAxis->GetBinCenter(nBinNowX);
      if(binCenterX < lowEdgeX || binCenterX > highEdgeX) continue;

      double binContent(0), binError(0);
      if(isInvAxis) {
        binContent = his2->GetBinContent(nBinNowY,nBinNowX);
        binError   = his2->GetBinError  (nBinNowY,nBinNowX);
      } else {
        binContent = his2->GetBinContent(nBinNowX,nBinNowY);
        binError   = his2->GetBinError  (nBinNowX,nBinNowY);
      }

      if(fabs(binContent) < EPS || isNanInf(binContent) || isNanInf(binError)) continue;

      his1->Fill(        binCenterX, binContent );
      his1->SetBinError( nBinNowX,   binError   );
    }
  }

  return;
}

// ===========================================================================================================
void Utils::getNpoisson(vector <double> & data0, vector <double> & data1) {
// ========================================================================
  param->NewOptF("nPoisson",-1);

  int nEle0 = (int)data0.size();
  int nEle1 = (int)data1.size();
  if(nEle0 == 0 || nEle0 != nEle1) return;

  double  nPoissNow(0);
  for(int nBinNow=0; nBinNow<nEle0; nBinNow++) {
    double  nData0 = data0[nBinNow];
    double  nData1 = data1[nBinNow];
    
    if(nData0 > 0) nPoissNow += pow((nData1 - nData0),2)/nData0;
  }
  if(nPoissNow > 0) {
    nPoissNow = sqrt(nPoissNow/double(nEle0));
    param->NewOptF("nPoisson",nPoissNow);
  }

  return;
}
// -----------------------------------------------------------------------------------------------------------
void Utils::getNpoisson(TH1 * data0, TH1 * data1) {
// ================================================
  param->NewOptF("nPoisson",-1);

  if(!data0 || !data1) return;

  data0->BufferEmpty(); data1->BufferEmpty();

  int nBins0 = data0->GetXaxis()->GetNbins();
  int nBins1 = data1->GetXaxis()->GetNbins();
  if(nBins0 != nBins1) return;

  double nPoissNow(0);
  for(int nBinNow=1; nBinNow<nBins0+1; nBinNow++) {
    double  nData0 = data0->GetBinContent(nBinNow);
    double  nData1 = data1->GetBinContent(nBinNow);
    
    if(nData0 > 0) nPoissNow += pow((nData1 - nData0),2)/nData0;
  }
  if(nPoissNow > 0) {
    nPoissNow = sqrt(nPoissNow/double(nBins0));
    param->SetOptF("nPoisson",nPoissNow);
  }

  return;
}

// ===========================================================================================================
void Utils::getKolmogorov(vector <double> & data0, vector <double> & data1) {
// ==========================================================================
  if(int(data0.size()*data1.size()) == 0) {   
    param->NewOptF("Kolmogorov_prob",-1);
    param->NewOptF("Kolmogorov_dist",-1);
    return;
  }

  // transfer the data from the vectors to arrays
  param->NewOptI("nArrEntries" , min((int)data0.size(),(int)data1.size()));
  // perform the Kolmogorov test
  getKolmogorov(data0.data(),data1.data());

  return;
}
// -----------------------------------------------------------------------------------------------------------
void Utils::getKolmogorov(TH1 * data0, TH1 * data1) {
// ==================================================
  param->NewOptF("Kolmogorov_prob",-1);
  param->NewOptF("Kolmogorov_dist",-1);
 
  if(!data0 || !data1) return;

  data0->BufferEmpty(); data1->BufferEmpty();

  TH1 * cumulatData0(NULL), * cumulatData1(NULL);
  if     (dynamic_cast<TH1D*>(data0) && dynamic_cast<TH1D*>(data1)) {
    cumulatData0 = (TH1D*)data0->Clone((TString)data0->GetName()+"_cumul");
    cumulatData1 = (TH1D*)data1->Clone((TString)data1->GetName()+"_cumul");
  }
  else if(dynamic_cast<TH1F*>(data0) && dynamic_cast<TH1F*>(data1)) {
    cumulatData0 = (TH1F*)data0->Clone((TString)data0->GetName()+"_cumul");
    cumulatData1 = (TH1F*)data1->Clone((TString)data1->GetName()+"_cumul");
  }
  else VERIFY(LOCATION,(TString)" - Unsupported histogram type (expected TH2D or TH1D) for "+data0->GetName()+" , "+data1->GetName(),false);

  cumulatData0->Reset(); cumulatData1->Reset();

  double  sum0(0), sum1(0);
  for(int nBinNow=1; nBinNow<data0->GetNbinsX()+1; nBinNow++) {
    double  binCont0 = data0->GetBinContent(nBinNow); assert(binCont0 >= 0); sum0 += binCont0; cumulatData0->SetBinContent(nBinNow,sum0);
    double  binCont1 = data1->GetBinContent(nBinNow); assert(binCont1 >= 0); sum1 += binCont1; cumulatData1->SetBinContent(nBinNow,sum1);
  }

  double maxDist(-1);
  if(sum0*sum1 > 0) {
    cumulatData0->Scale(1/sum0); cumulatData1->Scale(1/sum1);
    
    maxDist = 0;
    for(int nBinNow=1; nBinNow<data0->GetNbinsX()+1; nBinNow++) {
      double distNow = fabs(cumulatData0->GetBinContent(nBinNow) - cumulatData1->GetBinContent(nBinNow));
      if(maxDist < distNow) maxDist = distNow;
    }
  }

  param->SetOptF("Kolmogorov_prob",-1);
  param->SetOptF("Kolmogorov_dist",maxDist);

  DELNULL(cumulatData0); DELNULL(cumulatData1);
  return;
}

// -----------------------------------------------------------------------------------------------------------
void Utils::getKolmogorov(double * data0, double * data1) {
// ========================================================
  if(param->GetOptI("nArrEntries") == 0) {   
    param->NewOptF("Kolmogorov_prob",-1);
    param->NewOptF("Kolmogorov_dist",-1);
    return;
  }

  double * sortedData0(NULL), * sortedData1(NULL); 

  if(param->OptOrNullB("areSortedArrays")) { sortedData0 = data0; sortedData1 = data1; }
  else {
    sortedData0 = new double[param->GetOptI("nArrEntries")]; getSortedArray(data0,sortedData0);
    sortedData1 = new double[param->GetOptI("nArrEntries")]; getSortedArray(data1,sortedData1);
  }

  double  prob(0), dist(0);
  if(param->GetOptC("Kolmogorov_opt").Contains("prob")) {
    prob = TMath::KolmogorovTest(param->GetOptI("nArrEntries"),sortedData0,param->GetOptI("nArrEntries"),sortedData1,"");
  }
  if(param->GetOptC("Kolmogorov_opt").Contains("dist")) {
    dist = TMath::KolmogorovTest(param->GetOptI("nArrEntries"),sortedData0,param->GetOptI("nArrEntries"),sortedData1,"M");
  }

  param->NewOptF("Kolmogorov_prob",prob);
  param->NewOptF("Kolmogorov_dist",dist);

  if(!param->OptOrNullB("areSortedArrays")) { delete [] sortedData0; delete [] sortedData1; }
  return;
}

// ===========================================================================================================
void Utils::getSortedArray(double * data, double *& sortedData) {
// ==============================================================
  assert(param->GetOptI("nArrEntries") > 0);

  int * sortIndex = new int[param->GetOptI("nArrEntries")];

  // get the indices of the sorted array
  TMath::Sort(param->GetOptI("nArrEntries"),data,sortIndex,param->OptOrNullB("sortArr_decending"));
  // copy the sorted values into the data array
  for(int nEleNow=0; nEleNow<param->GetOptI("nArrEntries"); nEleNow++) sortedData[nEleNow] = data[sortIndex[nEleNow]];

  delete [] sortIndex;
  return;
}


// ===========================================================================================================
int Utils::getQuantileV(vector <double> & fracV, vector <double> & quantV, double * dataArr) {
  return getQuantileV(fracV,quantV,dataArr,NULL);
}
int Utils::getQuantileV(vector <double> & fracV, vector <double> & quantV, TH1    * dataHis) {
  return getQuantileV(fracV,quantV,NULL,dataHis);
}
// -----------------------------------------------------------------------------------------------------------
int Utils::getQuantileV(vector <double> & fracV, vector <double> & quantV, vector <double> & dataArrV) {
// =====================================================================================================
  param->NewOptI("nArrEntries" , (int)dataArrV.size());
  return getQuantileV(fracV,quantV,dataArrV.data(),NULL);
}
// -----------------------------------------------------------------------------------------------------------
int Utils::getQuantileV(vector <double> & fracV, vector <double> & quantV, double * dataArr, TH1 * dataHis) {
// ==========================================================================================================
  int nQuant = (int)fracV.size();
  if(!param->HasOptI("nArrEntries")) param->NewOptI("nArrEntries" , 0);

  bool hasArr = dataArr && (param->GetOptI("nArrEntries") > 0);
  bool hasHis = dynamic_cast<TH1*>(dataHis);
  if(hasHis) {
    dataHis->BufferEmpty();
    hasHis = (dataHis->GetEntries() > EPS && dataHis->Integral() > EPS);
  }
  if(!hasArr && !hasHis) return 0;
  if(nQuant == 0)        return 0;

  // -----------------------------------------------------------------------------------------------------------
  // basic quantile calculation
  // -----------------------------------------------------------------------------------------------------------
  int    * sortIndices  = new int   [param->GetOptI("nArrEntries")];
  double * quantiles    = new double[nQuant];
  double * probQuant    = new double[nQuant];

  for(int nQuantNow=0; nQuantNow<nQuant; nQuantNow++) probQuant[nQuantNow] = fracV[nQuantNow];
  
  if(hasHis) dataHis->GetQuantiles(nQuant,quantiles,probQuant);
  else       TMath::Quantiles(param->GetOptI("nArrEntries"),nQuant,dataArr,quantiles,probQuant,false,sortIndices,7);

  quantV.resize(nQuant);
  for(int nQuantNow=0; nQuantNow<nQuant; nQuantNow++) quantV[nQuantNow] = quantiles[nQuantNow];

  delete [] sortIndices; delete [] quantiles; delete [] probQuant;
  return 1;
}
// ===========================================================================================================
int Utils::getInterQuantileStats(double * dataArr) { return getInterQuantileStats(dataArr,NULL); }
int Utils::getInterQuantileStats(TH1    * dataHis) { return getInterQuantileStats(NULL,dataHis); }
// -----------------------------------------------------------------------------------------------------------
int Utils::getInterQuantileStats(vector <double> & dataArrV) {
// ===========================================================
  param->NewOptI("nArrEntries" , (int)dataArrV.size());
  return getInterQuantileStats(dataArrV.data(),NULL);
}
// -----------------------------------------------------------------------------------------------------------
int Utils::getInterQuantileStats(double * dataArr, TH1 * dataHis) {
// ================================================================
  if(!param->HasOptI("nArrEntries")) param->NewOptI("nArrEntries" , 0);

  bool hasArr = (dataArr != NULL);
  if(hasArr) { hasArr = (param->GetOptI("nArrEntries") > EPS); }
  bool hasHis = dynamic_cast<TH1*>(dataHis);
  if(hasHis) {
    dataHis->BufferEmpty();
    hasHis = (dataHis->GetEntries() > EPS && dataHis->Integral() > EPS);
  }
  if(!hasArr && !hasHis) return 0;
  
  if(param->OptOrNullB("doFracLargerSigma"))  param->NewOptB("doNotComputeNominalParams" , false);

  // -----------------------------------------------------------------------------------------------------------
  // basic quantile calculation
  // -----------------------------------------------------------------------------------------------------------
  int      closHisN     = 50000;
  int      nQuant       = 9;
  double   nArrEntriesF = double    (param->GetOptI("nArrEntries"));
  int    * sortIndices  = new int   [param->GetOptI("nArrEntries")];
  double * quantiles    = new double[nQuant];
  double * probQuant    = new double[nQuant];

  probQuant[0] = 0.15; probQuant[1] = 0.16; probQuant[2] = 0.17;
  probQuant[3] = 0.49; probQuant[4] = 0.50; probQuant[5] = 0.51;
  probQuant[6] = 0.83; probQuant[7] = 0.84; probQuant[8] = 0.85;

  if(hasHis) dataHis->GetQuantiles(nQuant,quantiles,probQuant);
  else       TMath::Quantiles(param->GetOptI("nArrEntries"),nQuant,dataArr,quantiles,probQuant,false,sortIndices,7);

  double quantile_16  = quantiles[1];
  double quantile_84  = quantiles[7];

  double sig68        = fabs(quantile_84 - quantile_16)/2.;
  double sig68Err     = sqrt(   pow(quantiles[0]-quantiles[1],2)+pow(quantiles[2]-quantiles[1],2)
                              + pow(quantiles[6]-quantiles[7],2)+pow(quantiles[8]-quantiles[7],2) ) / 2.;

  double median       = quantiles[4];
  double medianErr    = sqrt( (pow(quantiles[3]-quantiles[4],2)+pow(quantiles[5]-quantiles[4],2)) / 2. );
  
  param->NewOptF("quant_quantile_16"  , quantile_16);
  param->NewOptF("quant_quantile_84"  , quantile_84);
  param->NewOptF("quant_sigma_68"     , sig68);
  param->NewOptF("quant_sigma_68Err"  , sig68Err);
  param->NewOptF("quant_median"       , median);
  param->NewOptF("quant_medianErr"    , medianErr);

  double mean(0), meanErr(0), sigma(0), sigmaErr(0);
  if(!param->OptOrNullB("doNotComputeNominalParams")) {
    if(hasHis) {
      mean      = dataHis->GetMean();
      sigma     = dataHis->GetRMS();
      meanErr   = dataHis->GetMeanError();
      sigmaErr  = dataHis->GetRMSError();
    }
    else {
      mean      = TMath::Mean(param->GetOptI("nArrEntries"),dataArr,NULL);
      sigma     = TMath::RMS (param->GetOptI("nArrEntries"),dataArr);
      meanErr   = sigma  /sqrt(nArrEntriesF);  if(isNanInf(meanErr)) meanErr = -1;
      sigmaErr  = meanErr/sqrt(2.);
    }

    param->NewOptF("quant_mean"     , mean);
    param->NewOptF("quant_meanErr"  , meanErr);
    param->NewOptF("quant_sigma"    , sigma);
    param->NewOptF("quant_sigmaErr" , sigmaErr);
  }

  // -----------------------------------------------------------------------------------------------------------
  // mean after suppression of outliers beyond (param->GetOptF("meanWithoutOutliers")*sig68)
  // -----------------------------------------------------------------------------------------------------------
  if(param->OptOrNullF("meanWithoutOutliers") != 0) {
    double mean_Nsig68(0), meanErr_Nsig68(0);
    if(hasHis) {
      double outlierDist  = param->GetOptF("meanWithoutOutliers")*sig68;
      double origMinX     = dataHis->GetXaxis()->GetXmin();
      double origMaxX     = dataHis->GetXaxis()->GetXmax();

      dataHis->GetXaxis()->SetRangeUser(median-outlierDist,median+outlierDist);

      mean_Nsig68     = dataHis->GetMean();
      meanErr_Nsig68  = dataHis->GetMeanError();

      dataHis->GetXaxis()->SetRangeUser(origMinX,origMaxX);
    }
    else {
      double  outlierDist   = param->GetOptF("meanWithoutOutliers")*sig68;
      double  sigma_Nsig68  = 0;
      int     nOutliers     = 0;
      
      for(int nEleNow=0; nEleNow<param->GetOptI("nArrEntries"); nEleNow++) {
        if(fabs(dataArr[nEleNow] - median) > outlierDist) continue;
        nOutliers++;
      }
      if(nOutliers) {
        double * dataArrPart = new double[nOutliers];

        nOutliers = 0;
        for(int nEleNow=0; nEleNow<param->GetOptI("nArrEntries"); nEleNow++) {
          if(fabs(dataArr[nEleNow] - median) > outlierDist) continue;

          dataArrPart[nOutliers] = dataArr[nEleNow];
          nOutliers++;
        }
        mean_Nsig68     = TMath::Mean(nOutliers,dataArrPart,NULL);
        sigma_Nsig68    = TMath::RMS (nOutliers,dataArrPart);
        meanErr_Nsig68  = sigma_Nsig68/sqrt(double(nOutliers));

        delete [] dataArrPart;
      }
    }

    param->NewOptF("quant_mean_Nsig68"    , mean_Nsig68);
    param->NewOptF("quant_meanErr_Nsig68" , meanErr_Nsig68);
  }

  // Median absolute deviation
  // -----------------------------------------------------------------------------------------------------------
  if(param->OptOrNullB("getMAD")) {
    TH1 * madH = new TH1F("madHisTMP","madHisTMP",closHisN,1,-1); madH->SetDefaultBufferSize(closHisN);

    if(hasHis) {
      int nBins = dataHis->GetXaxis()->GetNbins();
      for(int nBinNow=1; nBinNow<nBins+1; nBinNow++) {
        double  binContent = dataHis->GetBinContent           (nBinNow);  if(binContent < EPS) continue;
        double  binCenter  = dataHis->GetXaxis()->GetBinCenter(nBinNow);

        madH->Fill(fabs(binCenter - median),binContent);
      }
    }
    else {
      for(int nEleNow=0; nEleNow<param->GetOptI("nArrEntries"); nEleNow++) {
        madH->Fill(fabs(dataArr[nEleNow] - median));
      }
    }
    madH->BufferEmpty();

    double * madQuant = new double[1];
    double * madProb  = new double[1]; madProb[0] = 0.50;

    if(madH->GetEntries() > EPS && madH->Integral() > EPS) madH->GetQuantiles(1,madQuant,madProb);
    else                                                   madQuant[0] = DefOpts::DefF;
    
    param->NewOptF("quant_MAD", madQuant[0]);

    delete [] madQuant; delete [] madProb; delete madH;
  }

  // -----------------------------------------------------------------------------------------------------------
  // number of entries and their fraction beyond 2,3 sigma,sigma68
  // -----------------------------------------------------------------------------------------------------------
  if(param->OptOrNullB("doFracLargerSigma")) {
    double nLargerSig68_2(0), nLargerSig68_3(0), nLargerSigma_2(0), nLargerSigma_3(0);
    double sig68_2(sig68*2),  sig68_3(sig68*3),  sigma_2(sigma*2),  sigma_3(sigma*3);

    if(hasHis) {
      nArrEntriesF = 0;
      int nBins = dataHis->GetXaxis()->GetNbins();
      for(int nBinNow=1; nBinNow<nBins+1; nBinNow++) {
        double  binContent = dataHis->GetBinContent            (nBinNow);  if(binContent < EPS) continue;
        double  binCenter  = dataHis->GetXaxis()->GetBinCenter (nBinNow);
      //double  binEdgeL   = dataHis->GetXaxis()->GetBinLowEdge(nBinNow);
      //double  binEdgeH   = dataHis->GetXaxis()->GetBinUpEdge (nBinNow);
      //double  binWidth   = dataHis->GetXaxis()->GetBinWidth  (nBinNow);
        double  distMedian = fabs(median - binCenter);
        
        if(distMedian > sig68_2) nLargerSig68_2 += binContent; if(distMedian > sig68_3) nLargerSig68_3 += binContent;
        if(distMedian > sigma_2) nLargerSigma_2 += binContent; if(distMedian > sigma_3) nLargerSigma_3 += binContent;
        
        nArrEntriesF += binContent;
      }
    }
    else {
      for(int nEleNow=0; nEleNow<param->GetOptI("nArrEntries"); nEleNow++) {
        double  distMedian = fabs(dataArr[nEleNow]-median);

        if(distMedian > sig68_2) nLargerSig68_2++; if(distMedian > sig68_3) nLargerSig68_3++;
        if(distMedian > sigma_2) nLargerSigma_2++; if(distMedian > sigma_3) nLargerSigma_3++;
      }
    }

    double fracSig68_2(0), fracSig68_3(0), fracSigma_2(0), fracSigma_3(0);
    if(nArrEntriesF > 0) {
      fracSig68_2 = nLargerSig68_2/nArrEntriesF; fracSig68_3 = nLargerSig68_3/nArrEntriesF;
      fracSigma_2 = nLargerSigma_2/nArrEntriesF; fracSigma_3 = nLargerSigma_3/nArrEntriesF;
    }
    
    param->NewOptI("quant_nLargerSig68_2" , nLargerSig68_2);
    param->NewOptI("quant_nLargerSig68_3" , nLargerSig68_3);
    param->NewOptI("quant_nLargerSigma_2" , nLargerSigma_2);
    param->NewOptI("quant_nLargerSigma_3" , nLargerSigma_3);
    param->NewOptF("quant_fracSig68_2"    , fracSig68_2);
    param->NewOptF("quant_fracSig68_3"    , fracSig68_3);
    param->NewOptF("quant_fracSigma_2"    , fracSigma_2);
    param->NewOptF("quant_fracSigma_3"    , fracSigma_3);
  }

  delete [] sortIndices; delete [] quantiles; delete [] probQuant;
  return 1;
}
