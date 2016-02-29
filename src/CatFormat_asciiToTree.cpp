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

// ===========================================================================================================
/**
 * @brief    - Initialization of CatFormat
 */
// ===========================================================================================================
void CatFormat::Init() {
// =====================  

  // unlock the global variables
  glob->setLock(false);

  int minNobjInVol = 50;
  if(glob->GetOptI("minNobjInVol_wgtKNN") < minNobjInVol) {
    glob->SetOptI("minNobjInVol_wgtKNN",minNobjInVol);
    aLOG(Log::INFO) <<coutBlue<<" - setting \"minNobjInVol_wgtKNN\" to the default minimal value: "<<coutRed<<minNobjInVol<<coutDef<<endl;
  }

  // finally, lock the global variables
  glob->setLock(true);

  return;
}


// ===========================================================================================================
/**
 * @brief                   - Convert ascii file into a root tree (no splitting).
 * 
 * @param inAsciiFiles      - semicolon-separated list of input ascii files
 * @param inAsciiVars       - semicolon-separated list of input parameter names, corresponding to columns in the input files
 * @param treeNamePostfix   - Optional additional postfix for the tree name
 * @param inTreeName        - Optional name of tree, if using root tree inputs instead of ascii inputs
 */
// ===========================================================================================================
void CatFormat::inputToFullTree(TString inAsciiFiles, TString inAsciiVars, TString treeNamePostfix, TString inTreeName) {
// ======================================================================================================================
  aLOG(Log::INFO) <<coutWhiteOnBlack<<coutBlue<<" - starting inputToFullTree("<<coutRed<< inAsciiFiles
                  <<coutBlue<<") ... "<<coutDef<<endl;

  // global to local variables
  int     maxNobj           = glob->GetOptI("maxNobj");
  int     nObjectsToPrint   = glob->GetOptI("nObjectsToPrint");
  int     nObjectsToWrite   = glob->GetOptI("nObjectsToWrite");
  TString treeName          = glob->GetOptC("treeName")+treeNamePostfix;
  TString origFileName      = glob->GetOptC("origFileName");
  TString indexName         = glob->GetOptC("indexName");
  TString weightName        = glob->GetOptC("baseName_wgtKNN");
  bool    storeOrigFileName = glob->GetOptB("storeOrigFileName");
  
  TString sigBckInpName     = glob->GetOptC("sigBckInpName");
  TString inpFiles_sig      = glob->GetOptC("inpFiles_sig");
  TString inpFiles_bck      = glob->GetOptC("inpFiles_bck");
  bool    addSigBckInp      = (inpFiles_sig != "" || inpFiles_bck != "");

  if(inTreeName == "") inTreeName = glob->GetOptC("inTreeName");

  vector <int>      nLineV;
  map <TString,int> intMap;
  vector <TString>  inFileNameV, inVarNames, inVarTypes, inpFiles_sigV, inpFiles_bckV;

  if(addSigBckInp) {
    inpFiles_sigV = utils->splitStringByChar(inpFiles_sig.ReplaceAll(" ",""),';');
    inpFiles_bckV = utils->splitStringByChar(inpFiles_bck.ReplaceAll(" ",""),';');
  }

  // -----------------------------------------------------------------------------------------------------------
  // input files
  // -----------------------------------------------------------------------------------------------------------
  inFileNameV  = utils->splitStringByChar(inAsciiFiles.ReplaceAll(" ",""),';');
  int nInFiles = (int)inFileNameV.size();
  VERIFY(LOCATION,(TString)"found no input files defined in \"inAsciiFiles\"...",(nInFiles > 0));

  // add the path to the file names
  for(int nInFileNow=0; nInFileNow<nInFiles; nInFileNow++) inFileNameV[nInFileNow] = glob->GetOptC("inDirName")+inFileNameV[nInFileNow];

  bool isRootInput = inFileNameV[0].EndsWith(".root");

  // make sure the input files are consistently of the same type
  if(isRootInput) {
    for(int nInFileNow=0; nInFileNow<nInFiles; nInFileNow++) {
      TString inFileNameNow    = inFileNameV[nInFileNow];
      bool    isExpectedFormat = (isRootInput && inFileNameV[nInFileNow].EndsWith(".root")) || (!isRootInput && !inFileNameV[nInFileNow].EndsWith(".root"));

      VERIFY(LOCATION,(TString)"Found some files ending with \".root\" and some without... must give one type of input!",isExpectedFormat);
    }
  }
  else {
    int nLinesTot = utils->getNlinesAsciiFile(inFileNameV,true,&nLineV);
    VERIFY(LOCATION,(TString)"found no content in \"inAsciiFiles\"...",(nLinesTot > 0));
  }

  // clear tree objects and reserve variables which are not part of the input file
  VarMaps * var = new VarMaps(glob,utils,"treeVars");  //var->printMapOpt(nPrintRow,width); cout<<endl;
  var->NewVarI(indexName); var->NewVarF(weightName);
  if(storeOrigFileName) var->NewVarC(origFileName);
  if(addSigBckInp)      var->NewVarI(sigBckInpName);

  // create tree variables from the linput ist
  // -----------------------------------------------------------------------------------------------------------
  parseInputVars(var,inAsciiVars,inVarNames,inVarTypes);

  // create the output tree(s) now thah all the variables are defined
  // -----------------------------------------------------------------------------------------------------------
  TTree * treeOut = new TTree(treeName,treeName);treeOut->SetDirectory(0);
  outputs->TreeMap[treeName] = treeOut;

  var->createTreeBranches(treeOut);

  // -----------------------------------------------------------------------------------------------------------
  // loop on all the input files
  // -----------------------------------------------------------------------------------------------------------
  bool breakLoop(false), mayWriteObjects(false);
  var->NewCntr("nObj",0);
  for(int nInFileNow=0; nInFileNow<nInFiles; nInFileNow++) {
    TString  inFileNameNow   = inFileNameV[nInFileNow];
    unsigned posSlash        = ((std::string)inFileNameNow).find_last_of("/");
    TString  reducedFileName = (TString)(((std::string)inFileNameNow).substr(posSlash+1));

    // optional parameter to mark if an object is of type signal (1), background (0) or undefined (-1), based on the name of the input file
    int sigBckInp(-1);
    if(addSigBckInp) {
      int nSigBckFound(0);

      if(find(inpFiles_bckV.begin(),inpFiles_bckV.end(),reducedFileName) != inpFiles_bckV.end()) { nSigBckFound++; sigBckInp = 0; }
      if(find(inpFiles_sigV.begin(),inpFiles_sigV.end(),reducedFileName) != inpFiles_sigV.end()) { nSigBckFound++; sigBckInp = 1; }

      if(nSigBckFound == 0) {
        aLOG(Log::WARNING)<<coutBlue<<" - Skipping "<<coutRed<<inFileNameNow<<coutBlue
                          <<" \"inpFiles_bck\" and \"inpFiles_sig\" are defined, but do not include it ... "<<coutDef<<endl;
        continue;
      }
      else if(nSigBckFound == 1) {
        aLOG(Log::INFO)<<coutBlue<<" - Will add \"sigBckInpName\" = "<<coutYellow<<sigBckInp<<" ("<<(TString)((sigBckInp == 0)?"background":"signal")
                       <<")"<<coutBlue<<" for all objects from "<<coutGreen<<reducedFileName<<coutDef<<endl;
      }
      else {
        VERIFY(LOCATION,(TString)"Input file \""+reducedFileName+"\" found in both \"inpFiles_bck\" and \"inpFiles_sig\".",false);
      }
    }

    // skip input files with no content
    if(!isRootInput) {
      if(nLineV[nInFileNow] == 0) {
        aLOG(Log::WARNING)<<coutBlue<<" - Skipping "<<coutRed<<inFileNameNow<<coutBlue<<" (no content in file) ... "<<coutDef<<endl;
        continue;
      }
    }
    aLOG(Log::INFO)<<coutGreen<<" - Now reading-in "<<coutYellow<<inFileNameNow<<coutGreen<<" ... "<<coutDef<<endl;

    // -----------------------------------------------------------------------------------------------------------
    // the loop
    // -----------------------------------------------------------------------------------------------------------
    var->NewCntr("nLine",0);
    
    if(isRootInput) {
      TChain  * inChain = new TChain(inTreeName,inTreeName); inChain->SetDirectory(0); inChain->Add(inFileNameNow);
      aLOG(Log::DEBUG) <<coutRed<<" - added chain "<<coutGreen<<inTreeName<<"("<<inChain->GetEntries()<<")"<<" from "<<coutBlue<<inFileNameNow<<coutDef<<endl;

      VarMaps * var_0   = new VarMaps(glob,utils,(TString)"inputTree_"+inTreeName);
      var_0->connectTreeBranches(inChain);

      // get the full list of variables common to both var and var_0
      vector < pair<TString,TString> > varTypeNameV;
      var->varStruct(var_0,NULL,NULL,&varTypeNameV,false);

      bool breakLoopTree(false);
      for(Long64_t loopEntry=0; true; loopEntry++) {
        if(!var_0->getTreeEntry(loopEntry)) breakLoopTree = true;

        if((mayWriteObjects && var->GetCntr("nObj") % nObjectsToWrite == 0) || breakLoop || breakLoopTree) {
          mayWriteObjects = false;
          var->printCntr(treeName,Log::INFO); outputs->WriteOutObjects(false,true); outputs->ResetObjects(); 
        }
        if(breakLoop || breakLoopTree) break;

        if(var->GetCntr("nLine") % nObjectsToPrint == 0) {
          aLOG(Log::DEBUG) <<coutGreen<<" - "<<coutBlue<<glob->GetOptC("outDirName")<<coutGreen<<" - "<<coutBlue<<glob->GetOptC("baseName")<<coutGreen<<" - "
          <<coutGreen<<" Objects in current file = "<<coutYellow<<TString::Format("%3.3g \t",(double)var->GetCntr("nLine"))
          <<coutRed<<" Total = "<<coutYellow<<TString::Format("%3.3g \t",(double)var->GetCntr("nObj"))<<coutDef<<endl;
        }

        var->copyVarData(var_0,&varTypeNameV);

        // update variable with input file name
        if(storeOrigFileName) var->SetVarC(origFileName,reducedFileName);
        // sig/bck tag based on the name of the input file
        if(addSigBckInp) var->SetVarI(sigBckInpName, sigBckInp);
        // the object index
        var->SetVarI(indexName, var->GetCntr("nObj"));
        // update the placeholder of the KNN weights
        var->SetVarF(weightName,1);
        
        // fill the tree with the current variables
        treeOut->Fill();

        if(inLOG(Log::DEBUG_3)) {
          int nPrintRow(4), width(14);
          cout <<coutYellow<<"Line # "<<var->GetCntr("nObj")<<endl<<coutYellow<<std::setw(100)<<std::setfill('.')<<" "<<std::setfill(' ')<<coutDef;
          var->printVars(nPrintRow,width);
        }
        
        // update counters
        var->IncCntr("nObj"); var->IncCntr("nLine"); mayWriteObjects = true; if(var->GetCntr("nObj") == maxNobj) breakLoop = true;
      }

      DELNULL(var_0); DELNULL(inChain); varTypeNameV.clear();
    }
    else {
      std::ifstream inputFile(inFileNameNow,std::ios::in);
      std::string   line;

      while(!inputFile.eof()) {
        // get an object
        // -----------------------------------------------------------------------------------------------------------
        getline(inputFile, line);  if(!inputLineToVars((TString)line,var,inVarNames,inVarTypes)) continue;

        if((mayWriteObjects && var->GetCntr("nObj") % nObjectsToWrite == 0) || breakLoop) {
          mayWriteObjects = false;
          var->printCntr(treeName,Log::INFO); outputs->WriteOutObjects(false,true); outputs->ResetObjects(); 
        }
        if(breakLoop) break;

        if(var->GetCntr("nLine") % nObjectsToPrint == 0) {
          aLOG(Log::DEBUG) <<coutGreen<<" - "<<coutBlue<<glob->GetOptC("outDirName")<<coutGreen<<" - "<<coutBlue<<glob->GetOptC("baseName")<<coutGreen<<" - "
          <<coutGreen<<" Objects in current file = "<<coutYellow<<TString::Format("%3.3g \t",(double)var->GetCntr("nLine"))
          <<coutRed<<" Total = "<<coutYellow<<TString::Format("%3.3g \t",(double)var->GetCntr("nObj"))<<coutDef<<endl;
        }

        // update variable with input file name
        if(storeOrigFileName) var->SetVarC(origFileName,reducedFileName);
        // sig/bck tag based on the name of the input file
        if(addSigBckInp) var->SetVarI(sigBckInpName, sigBckInp);
        // the object index
        var->SetVarI(indexName, var->GetCntr("nObj"));
        // update the placeholder to KNN weights
        var->SetVarF(weightName,1);
        
        // fill the tree with the current variables
        treeOut->Fill();

        if(inLOG(Log::DEBUG_3)) {
          int nPrintRow(4), width(14);
          cout <<coutYellow<<"Line # "<<var->GetCntr("nObj")<<endl<<coutYellow<<std::setw(100)<<std::setfill('.')<<" "<<std::setfill(' ')<<coutDef;
          var->printVars(nPrintRow,width);
        }
        
        // update counters
        var->IncCntr("nObj"); var->IncCntr("nLine"); mayWriteObjects = true; if(var->GetCntr("nObj") == maxNobj) breakLoop = true;
      }
    }

  }
  if(!breakLoop) { var->printCntr(treeName,Log::INFO); outputs->WriteOutObjects(false,true); outputs->ResetObjects(); }

  DELNULL(var);

  inVarNames.clear(); inVarTypes.clear(); inFileNameV.clear(); nLineV.clear(); inpFiles_sigV.clear(); inpFiles_bckV.clear();

  return;
}

// ===========================================================================================================
/**
 * @brief                - Convert ascii file into a root tree (optional splitting for train/test/valid subsamples).
 * 
 * 
 * @details              - For training and testing/validation the input is divided into two (test,train) or into three (test,train,valid)
 *                         sub-samples.
 *                       - The user needs to define the number of sub-samples (e.g., nSplit = 1,2 or 3) and the way to divide the
 *                         inputs in one of 4 ways (e.g., splitType = "serial", "blocks", "random" or "byInFiles" (default)):
 *                           - serial: -> test;train;valid;test;train;valid;test;train;valid;test;train;valid...
 *                           - blocks: -> test;test;test;test;train;train;train;train;valid;valid;valid;valid...
 *                           - random: -> valid;test;test;train;valid;test;valid;valid;test;train;valid;train...
 *                           - separate input files. Must supplay at least one file in splitTypeTrain and one in splitTypeTest.
 *                             In this case, [nSplit = 2]. Optionally can set [nSplit = 3] and provide a list of files in "splitTypeValid" as well.
 * 
 * @param inAsciiFiles   - semicolon-separated list of input ascii files
 * @param inAsciiVars    - semicolon-separated list of input parameter names, corresponding to columns in the input files
 */
// ===========================================================================================================
void CatFormat::inputToSplitTree(TString inAsciiFiles, TString inAsciiVars) {
// ==========================================================================
  aLOG(Log::INFO) <<coutWhiteOnBlack<<coutBlue<<" - starting inputToSplitTree("<<coutRed<< inAsciiFiles
                  <<coutBlue<<") ... "<<coutDef<<endl;

  // global to local variables
  int     maxNobj           = glob->GetOptI("maxNobj");
  int     nObjectsToPrint   = glob->GetOptI("nObjectsToPrint");
  int     nObjectsToWrite   = glob->GetOptI("nObjectsToWrite");
  TString treeName          = glob->GetOptC("treeName");
  TString origFileName      = glob->GetOptC("origFileName");
  bool    storeOrigFileName = glob->GetOptB("storeOrigFileName");
  int     nSplit            = glob->GetOptI("nSplit");
  TString splitType         = glob->GetOptC("splitType");
  TString indexName         = glob->GetOptC("indexName");
  TString splitName         = glob->GetOptC("splitName");
  TString testValidType     = glob->GetOptC("testValidType");
  TString weightName        = glob->GetOptC("baseName_wgtKNN");
  bool    doPlots           = glob->GetOptB("doPlots");
  TString plotExt           = glob->GetOptC("printPlotExtension");
  TString outDirNameFull    = glob->GetOptC("outDirNameFull");
  TString inTreeName        = glob->GetOptC("inTreeName");

  TString sigBckInpName     = glob->GetOptC("sigBckInpName");
  TString inpFiles_sig      = glob->GetOptC("inpFiles_sig");
  TString inpFiles_bck      = glob->GetOptC("inpFiles_bck");
  bool    addSigBckInp      = (inpFiles_sig != "" || inpFiles_bck != "");
  
  VERIFY(LOCATION,(TString)"found unsupported number of splittings ("+utils->intToStr(nSplit)+"). Allowed values are: 1,2,3"
                  ,(nSplit >= 1 && nSplit <= 3));

  // random number generator for the (splitType == "random") option
  TRandom * rnd = new TRandom3(glob->GetOptI("splitSeed"));
  VERIFY(LOCATION,(TString)"Random seed must be >= 0",(glob->GetOptI("splitSeed") >= 0));

  bool              isRootInput(false);
  int               nInFiles(0), inFileTypeChange(0);
  map <TString,int> intMap;
  vector <TString>  inFileNameV, inVarNames, inVarTypes, inpFiles_sigV, inpFiles_bckV;
  vector <int>      inFileTypeV;

  if(addSigBckInp) {
    inpFiles_sigV = utils->splitStringByChar(inpFiles_sig.ReplaceAll(" ",""),';');
    inpFiles_bckV = utils->splitStringByChar(inpFiles_bck.ReplaceAll(" ",""),';');
  }

  // -----------------------------------------------------------------------------------------------------------
  // input files - if diffrent for each type
  // -----------------------------------------------------------------------------------------------------------
  if(splitType == "byInFiles" && nSplit > 1) {
    vector <TString> inFileNameV_now;
    for(int nSplitNow=0; nSplitNow<nSplit; nSplitNow++) {
      TString splitTypeNow("");
      if     (nSplitNow == 0) splitTypeNow = "splitTypeTrain";
      else if(nSplitNow == 1) splitTypeNow = "splitTypeTest";
      else if(nSplitNow == 2) splitTypeNow = "splitTypeValid";
      else VERIFY(LOCATION,"How did we get here... ?!?",false);

      inFileNameV_now = utils->splitStringByChar((glob->GetOptC(splitTypeNow)).ReplaceAll(" ",""),';');
      nInFiles        = (int)inFileNameV_now.size();
      VERIFY(LOCATION,(TString)"found no input files defined in "+splitTypeNow+" (nSplitNow = "+utils->intToStr(nSplitNow)+") ?!?",(nInFiles > 0));

      for(int nInFileNow=0; nInFileNow<nInFiles; nInFileNow++) {
        inFileNameV_now[nInFileNow] = glob->GetOptC("inDirName")+inFileNameV_now[nInFileNow];        
        
        inFileTypeV.push_back(nSplitNow);
        inFileNameV.push_back(inFileNameV_now[nInFileNow]);
      }

      isRootInput = inFileNameV[0].EndsWith(".root");
      if(!isRootInput) {
        utils->getNlinesAsciiFile(inFileNameV_now,true); // make sure the files are not all empty
      }
      
      inFileNameV_now.clear();
    }

    nInFiles = (int)inFileNameV.size();
  }
  // -----------------------------------------------------------------------------------------------------------
  // input files - if common files for training,testing(,validating)
  // -----------------------------------------------------------------------------------------------------------
  else {
    inFileNameV = utils->splitStringByChar(inAsciiFiles.ReplaceAll(" ",""),';');
    nInFiles    = (int)inFileNameV.size();
    VERIFY(LOCATION,(TString)"found no input files defined in \"inAsciiFiles\"...",(nInFiles > 0));

    isRootInput = inFileNameV[0].EndsWith(".root");

    // add the path to the file names
    for(int nInFileNow=0; nInFileNow<nInFiles; nInFileNow++) inFileNameV[nInFileNow] = glob->GetOptC("inDirName")+inFileNameV[nInFileNow];
  }

  // make sure the input files are consistently of the same type
  // -----------------------------------------------------------------------------------------------------------
  if(isRootInput) {
    for(int nInFileNow=0; nInFileNow<nInFiles; nInFileNow++) {
      TString inFileNameNow    = inFileNameV[nInFileNow];
      bool    isExpectedFormat = (isRootInput && inFileNameV[nInFileNow].EndsWith(".root")) || (!isRootInput && !inFileNameV[nInFileNow].EndsWith(".root"));

      VERIFY(LOCATION,(TString)"Found some files ending with \".root\" and some without... must give one type of input!",isExpectedFormat);
    }
  }

  // derive the number of lines to use in case of (splitType == "blocks") option
  // -----------------------------------------------------------------------------------------------------------
  if(splitType == "blocks") {
    int nLinesTot(0);
    if(isRootInput) {
      for(int nInFileNow=0; nInFileNow<nInFiles; nInFileNow++) {
        TString inFileNameNow = inFileNameV[nInFileNow];
        TChain  * inChain     = new TChain(inTreeName,inTreeName); inChain->SetDirectory(0); inChain->Add(inFileNameNow);

        nLinesTot += inChain->GetEntries();

        DELNULL(inChain);
      }
    }
    else {
      nLinesTot = utils->getNlinesAsciiFile(inFileNameV,true);
    }
    if(maxNobj > 0) nLinesTot = min(nLinesTot,maxNobj);
    
    intMap["nLine_splitBlocks"] = static_cast<int>(floor(0.1+nLinesTot/double(nSplit)));
  }

  // clear tree objects and reserve variables which are not part of the input file
  VarMaps * var = new VarMaps(glob,utils,"treeVars");  //var->printMapOpt(nPrintRow,width); cout<<endl;
  
  var->NewVarI(indexName);     var->NewVarI(splitName);
  var->NewVarI(testValidType); var->NewVarF(weightName);
  if(storeOrigFileName) var->NewVarC(origFileName);
  if(addSigBckInp)      var->NewVarI(sigBckInpName);

  // create tree variables from the linput ist
  // -----------------------------------------------------------------------------------------------------------
  parseInputVars(var,inAsciiVars,inVarNames,inVarTypes);

  // create the output tree(s) now thah all the variables are defined
  // -----------------------------------------------------------------------------------------------------------
  int nTrees = min(nSplit,2);
  vector <TTree *> treeOut  (nTrees); 
  vector <TString> treeNames(nTrees);

  if(nTrees == 1) { treeNames[0] = (TString)treeName+"_full";                                             }
  else            { treeNames[0] = (TString)treeName+"_train"; treeNames[1] = (TString)treeName+"_valid"; }

  for(int nTreeNow=0; nTreeNow<nTrees; nTreeNow++) {
    TString treeNameNow = treeNames[nTreeNow];
    treeOut[nTreeNow]   = new TTree(treeNameNow,treeNameNow);
    treeOut[nTreeNow]->SetDirectory(0); outputs->TreeMap[treeNameNow] = treeOut[nTreeNow];

    var->createTreeBranches(treeOut[nTreeNow]);
  }

  // loop control-variables (outside the for() of file-names)
  // -----------------------------------------------------------------------------------------------------------
  var->NewCntr("nObj",0);   var->NewCntr("nLine",0);
  var->NewCntr("nTrain",0); var->NewCntr("nTest",0); var->NewCntr("nValid",0);

  int   nSplitType(0);
  bool  breakLoop(false), mayWriteObjects(false);

  if(nSplit > 1) {
    if     (splitType == "serial")    nSplitType = 0;
    else if(splitType == "blocks")    nSplitType = 1;
    else if(splitType == "random")    nSplitType = 2;
    else if(splitType == "byInFiles") nSplitType = 3;
    else VERIFY(LOCATION,(TString)"found unsupported splitType ("+splitType+"). "+
                                         "Allowed values are: \"serial\",\"blocks\",\"random\",\"byInFiles\"",false);
  }
  intMap["nSplitType"] = nSplitType;

  // -----------------------------------------------------------------------------------------------------------
  // loop on all the input files
  // -----------------------------------------------------------------------------------------------------------
  for(int nInFileNow=0; nInFileNow<nInFiles; nInFileNow++) {
    TString  inFileNameNow   = inFileNameV[nInFileNow];
    unsigned posSlash        = ((std::string)inFileNameNow).find_last_of("/");
    TString  reducedFileName = (TString)(((std::string)inFileNameNow).substr(posSlash+1));

    // optional parameter to mark if an object is of type signal (1), background (0) or undefined (-1), based on the name of the input file
    int sigBckInp(-1);
    if(addSigBckInp) {
      int nSigBckFound(0);

      if(find(inpFiles_bckV.begin(),inpFiles_bckV.end(),reducedFileName) != inpFiles_bckV.end()) { nSigBckFound++; sigBckInp = 0; }
      if(find(inpFiles_sigV.begin(),inpFiles_sigV.end(),reducedFileName) != inpFiles_sigV.end()) { nSigBckFound++; sigBckInp = 1; }

      if(nSigBckFound == 0) {
        aLOG(Log::WARNING)<<coutBlue<<" - Skipping "<<coutRed<<inFileNameNow<<coutBlue
                          <<" \"inpFiles_bck\" and \"inpFiles_sig\" are defined, but do not include it ... "<<coutDef<<endl;
        continue;
      }
      else if(nSigBckFound == 1) {
        aLOG(Log::INFO)<<coutBlue<<" - Will add \"sigBckInpName\" = "<<coutYellow<<sigBckInp<<" ("<<(TString)((sigBckInp == 0)?"background":"signal")
                       <<")"<<coutBlue<<" for all objects from "<<coutGreen<<reducedFileName<<coutDef<<endl;
      }
      else {
        VERIFY(LOCATION,(TString)"Input file \""+reducedFileName+"\" found in both \"inpFiles_bck\" and \"inpFiles_sig\".",false);
      }
    }

    // skip input files with no content
    if(!isRootInput) {
      if(utils->getNlinesAsciiFile(inFileNameNow,false) == 0) {
        aLOG(Log::WARNING)<<coutBlue<<" - Skipping "<<coutRed<<inFileNameNow<<coutBlue<<" (no content in file) ... "<<coutDef<<endl;
        continue;
      }
    }

    // write out trees and initialize counters if moving from one type to another (e.g., from train to test)
    if(nSplitType == 3) {
      intMap["inFileSplitIndex"] = inFileTypeV[nInFileNow];

      if(inFileTypeChange != inFileTypeV[nInFileNow]) {
        inFileTypeChange = inFileTypeV[nInFileNow];

        var->printCntr(treeName,Log::INFO); outputs->WriteOutObjects(false,true); outputs->ResetObjects();
        
        var->NewCntr("nObj",0);
        breakLoop = mayWriteObjects = false;
      }
    }

    aLOG(Log::INFO)<<coutGreen<<" - Now reading-in "<<coutYellow<<inFileNameNow<<coutGreen<<" ... "<<coutDef<<endl;

    // -----------------------------------------------------------------------------------------------------------
    // the loop
    // -----------------------------------------------------------------------------------------------------------
    var->NewCntr("nLineFile",0);

    if(isRootInput) {
      TChain  * inChain = new TChain(inTreeName,inTreeName); inChain->SetDirectory(0); inChain->Add(inFileNameNow);
      aLOG(Log::DEBUG) <<coutRed<<" - added chain "<<coutGreen<<inTreeName<<"("<<inChain->GetEntries()<<")"<<" from "<<coutBlue<<inFileNameNow<<coutDef<<endl;

      VarMaps * var_0   = new VarMaps(glob,utils,(TString)"inputTree_"+inTreeName);
      var_0->connectTreeBranches(inChain);

      // get the full list of variables common to both var and var_0
      vector < pair<TString,TString> > varTypeNameV;
      var->varStruct(var_0,NULL,NULL,&varTypeNameV,false);

      bool breakLoopTree(false);
      for(Long64_t loopEntry=0; true; loopEntry++) {
        if(!var_0->getTreeEntry(loopEntry)) breakLoopTree = true;

        if((mayWriteObjects && var->GetCntr("nObj") % nObjectsToWrite == 0) || breakLoop || breakLoopTree) {
          mayWriteObjects = false;
          var->printCntr(treeName,Log::INFO); outputs->WriteOutObjects(false,true); outputs->ResetObjects(); 
        }
        if(breakLoop || breakLoopTree) break;

        if(var->GetCntr("nLine") % nObjectsToPrint == 0) {
          aLOG(Log::DEBUG) <<coutGreen<<" - "<<coutBlue<<glob->GetOptC("outDirName")<<coutGreen<<" - "<<coutBlue<<glob->GetOptC("baseName")<<coutGreen<<" - "
          <<coutGreen<<" Objects in current file = "<<coutYellow<<TString::Format("%3.3g \t",(double)var->GetCntr("nLineFile"))
          <<coutRed<<" Total = "<<coutYellow<<TString::Format("%3.3g \t",(double)var->GetCntr("nObj"))<<coutDef<<endl;
        }

        var->copyVarData(var_0,&varTypeNameV);

        // update variable with input file name
        if(storeOrigFileName) var->SetVarC(origFileName,reducedFileName);
        // sig/bck tag based on the name of the input file
        if(addSigBckInp) var->SetVarI(sigBckInpName, sigBckInp);
        // update the placeholder to KNN weights
        var->SetVarF(weightName,1);

        // set the indexing variables, and get the corresponding intMap["nSplitTree"]
        setSplitVars(var,rnd,intMap);

        // fill the tree with the current variables
        treeOut[intMap["nSplitTree"]]->Fill();

        if(inLOG(Log::DEBUG_3)) {
          int nPrintRow(4), width(14);
          cout <<coutYellow<<"Line # "<<var->GetCntr("nObj")<<endl<<coutYellow<<std::setw(100)<<std::setfill('.')<<" "<<std::setfill(' ')<<coutDef;
          var->printVars(nPrintRow,width);
        }
        
        // update counters
        var->IncCntr("nObj"); var->IncCntr("nLineFile"); mayWriteObjects = true; if(var->GetCntr("nObj") == maxNobj) breakLoop = true;
      }

      DELNULL(var_0); DELNULL(inChain); varTypeNameV.clear();
    }
    else {
      std::ifstream inputFile(inFileNameNow,std::ios::in);
      std::string   line;

      while(!inputFile.eof()) {
        // get an object
        // -----------------------------------------------------------------------------------------------------------
        getline(inputFile, line);  if(!inputLineToVars((TString)line,var,inVarNames,inVarTypes)) continue;

        if((mayWriteObjects && var->GetCntr("nObj") % nObjectsToWrite == 0) || breakLoop) {
          mayWriteObjects = false;
          var->printCntr(treeName,Log::INFO); outputs->WriteOutObjects(false,true); outputs->ResetObjects(); 
        }
        if(breakLoop) break;

        if(var->GetCntr("nLine") % nObjectsToPrint == 0) {
          aLOG(Log::DEBUG) <<coutGreen<<" - "<<coutBlue<<glob->GetOptC("outDirName")<<coutGreen<<" - "<<coutBlue<<glob->GetOptC("baseName")<<coutGreen<<" - "
          <<coutGreen<<" Objects in current file = "<<coutYellow<<TString::Format("%3.3g \t",(double)var->GetCntr("nLineFile"))
          <<coutRed<<" Total = "<<coutYellow<<TString::Format("%3.3g \t",(double)var->GetCntr("nObj"))<<coutDef<<endl;
        }

        // update variable with input file name
        if(storeOrigFileName) var->SetVarC(origFileName,reducedFileName);
        // sig/bck tag based on the name of the input file
        if(addSigBckInp) var->SetVarI(sigBckInpName, sigBckInp);
        // update the placeholder to KNN weights
        var->SetVarF(weightName,1);

        // set the indexing variables, and get the corresponding intMap["nSplitTree"]
        setSplitVars(var,rnd,intMap);

        // fill the tree with the current variables
        treeOut[intMap["nSplitTree"]]->Fill();

        if(inLOG(Log::DEBUG_3)) {
          int nPrintRow(4), width(14);
          cout <<coutYellow<<"Line # "<<var->GetCntr("nObj")<<endl<<coutYellow<<std::setw(100)<<std::setfill('.')<<" "<<std::setfill(' ')<<coutDef;
          var->printVars(nPrintRow,width);
        }
        
        // update counters
        var->IncCntr("nObj"); var->IncCntr("nLineFile"); mayWriteObjects = true; if(var->GetCntr("nObj") == maxNobj) breakLoop = true;
      }
    }

  }
  if(!breakLoop) { var->printCntr(treeName,Log::INFO); outputs->WriteOutObjects(false,true); outputs->ResetObjects(); }

  // -----------------------------------------------------------------------------------------------------------
  // some histograms of the input branches
  // -----------------------------------------------------------------------------------------------------------
  if(doPlots) {
    for(int nTreeNow=0; nTreeNow<nTrees; nTreeNow++) {
      // create the chain from the output of the above
      TString treeNameNow = treeNames[nTreeNow];
      TString fileNameNow = (TString)outDirNameFull+treeNameNow+"*.root";
      TString plotTag     = (TString)"asciiToTree_"+treeNameNow;//+TString::Format("_nTree%d",nTreeNow);

      TChain  * aChain = new TChain(treeNameNow,treeNameNow); aChain->SetDirectory(0); aChain->Add(fileNameNow);
      aLOG(Log::DEBUG) <<coutRed<<" - Created chain  "<<coutGreen<<treeNameNow<<"("<<aChain->GetEntries()<<")"<<" from "<<coutBlue<<fileNameNow<<coutDef<<endl;

      // derive the names of all numeric branches in the chain
      vector <TString> branchNameV_0, branchNameV;

      var->GetAllVarNames(branchNameV_0);
      int nBranches = (int)branchNameV_0.size();

      branchNameV.push_back(weightName);
      for(int nBranchNow=0; nBranchNow<nBranches; nBranchNow++) {
        TString branchName = branchNameV_0[nBranchNow];
        TString branchType = var->GetVarType(branchName);

        if(branchType == "C" || branchType == "FM")               continue;
        if(branchName.BeginsWith(glob->GetOptC("baseName_ANNZ"))) continue;

        branchNameV.push_back(branchName);
      }
      nBranches = (int)branchNameV.size();

      // create the plots
      for(int nBranchNow=0; nBranchNow<nBranches; nBranchNow++) {
        TString hisName   = (TString)plotTag+"_treeBranch"+TString::Format("_nBranch%d",nBranchNow);
        TString drawExprs = (TString)branchNameV[nBranchNow]+">>"+hisName;

        TCanvas * tmpCnvs = new TCanvas("tmpCnvs","tmpCnvs");
        aChain->Draw(drawExprs,""); DELNULL(tmpCnvs);

        TH1 * his1 = (TH1F*)gDirectory->Get(hisName); assert(dynamic_cast<TH1F*>(his1));
        his1->SetDirectory(0); his1->BufferEmpty(); his1->SetTitle(branchNameV[nBranchNow]);

        outputs->optClear();
        outputs->draw->NewOptC("drawOpt"    , "HIST");
        outputs->draw->NewOptC("axisTitleX" , branchNameV[nBranchNow]);
        outputs->draw->NewOptC("axisTitleY" , (TString)"dN/d "+branchNameV[nBranchNow]);
        outputs->drawHis1dV(his1);
      }

      outputs->WriteOutObjects(true,true); outputs->ResetObjects();

      branchNameV.clear(); branchNameV_0.clear();
    }
  }


  // cleanup
  DELNULL(var); DELNULL(rnd);

  for(int nTreeNow=0; nTreeNow<nTrees; nTreeNow++) {
    TString treeNameNow = treeNames[nTreeNow];
    
    DELNULL(treeOut[nTreeNow]); outputs->TreeMap.erase(treeNameNow);
  }

  treeOut.clear(); treeNames.clear();
  intMap.clear(); inVarNames.clear(); inVarTypes.clear(); inFileNameV.clear(); inFileTypeV.clear(); inpFiles_sigV.clear(); inpFiles_bckV.clear();


  // -----------------------------------------------------------------------------------------------------------
  // store some variables which should be consistent whenever anyone is to use the output trees generated here
  // -----------------------------------------------------------------------------------------------------------
  OptMaps          * optMap = new OptMaps("localOptMap");
  TString          saveName = "";
  vector <TString> optNames;
  saveName = "nSplit";        optNames.push_back(saveName); optMap->NewOptI(saveName, glob->GetOptI(saveName));
  saveName = "treeName";      optNames.push_back(saveName); optMap->NewOptC(saveName, glob->GetOptC(saveName));
  saveName = "indexName";     optNames.push_back(saveName); optMap->NewOptC(saveName, glob->GetOptC(saveName));
  saveName = "splitName";     optNames.push_back(saveName); optMap->NewOptC(saveName, glob->GetOptC(saveName));
  saveName = "testValidType"; optNames.push_back(saveName); optMap->NewOptC(saveName, glob->GetOptC(saveName));
  saveName = "useWgtKNN";     optNames.push_back(saveName); optMap->NewOptB(saveName, glob->GetOptB(saveName));

  if(storeOrigFileName) { saveName = "origFileName";  optNames.push_back(saveName); optMap->NewOptC(saveName, glob->GetOptC(saveName)); }

  utils->optToFromFile(&optNames,optMap,glob->GetOptC("userOptsFile_genInputTrees"),"WRITE");

  optNames.clear(); DELNULL(optMap);

  return;
}


// ===========================================================================================================
/**
 * @brief               - Parse the input variable list into pairs of variable names/types.
 * 
 * @param var           - The VarMaps() object to which the variables are added
 * @param inAsciiVars   - semicolon-separated list of input parameter names, corresponding to columns in the input files
 * @param inVarNames    - vector which is filled with the list of input parameter names
 * @param inVarTypes    - vector which is filled with the list of input parameter types
 */
// ===========================================================================================================
void CatFormat::parseInputVars(VarMaps * var, TString inAsciiVars, vector <TString> & inVarNames, vector <TString> & inVarTypes) {
// ===============================================================================================================================
  aLOG(Log::DEBUG_3) <<coutGreen<<" - CatFormat::parseInputVars(): "<<coutYellow<<inAsciiVars<<coutDef<<endl;

  // -----------------------------------------------------------------------------------------------------------
  // inAsciiVars -
  //   paired list with format "TYPE:NAME,TYPE:NAME,TYPE:NAME,TYPE:NAME" , where TYPE can take values:
  //     (B-> [boolean]),                        (F-> [32 bit loating point]),           (D-> [64 bit floating point]),          (C->[string])
  //     (S-> [signed   16 bit signed integer]), (I-> [signed   32 bit signed integer]), (L-> [signed   64 bit signed integer]),
  //     (US->[unsigned 16 bit signed integer]), (UI->[unsigned 32 bit signed integer]), (UL->[unsigned 64 bit signed integer]),
  //   and NAME can be any string without spaces (special characters will be replaces...)
  // -----------------------------------------------------------------------------------------------------------

  inAsciiVars.ReplaceAll(" ",""); inVarNames = utils->splitStringByChar(inAsciiVars.ReplaceAll(" ",""),';');
  int nVars  = (int)inVarNames.size();

  VERIFY(LOCATION,(TString)"found no input variables defined in \"inAsciiVars\"...",(nVars > 0));

  inVarTypes.resize(nVars);

  for(int nVarNow=0; nVarNow<nVars; nVarNow++) {
    int lenType     = inVarNames[nVarNow].First(':');
    VERIFY(LOCATION,(TString)"found unsupported variable-type ("+inVarNames[nVarNow]+")",(lenType==1||lenType==2));

    TString varType = inVarNames[nVarNow](0,lenType);
    TString varVal  = inVarNames[nVarNow](lenType+1,inVarNames[nVarNow].Length());
    
    varType.ToUpper(); varVal = utils->regularizeName(varVal); inVarNames[nVarNow] = varVal;

    if     (varType == "B" ) { var->NewVarB(varVal);                     inVarTypes[nVarNow] = "B";  } // Bool_t
    else if(varType == "C" ) { var->NewVarC(varVal);                     inVarTypes[nVarNow] = "C";  } // TString
    else if(varType == "S" ) { var->NewVarI(varVal,DefOpts::DefS, "S");  inVarTypes[nVarNow] = "S";  } // Short_t
    else if(varType == "I" ) { var->NewVarI(varVal,DefOpts::DefI, "I");  inVarTypes[nVarNow] = "I";  } // Int_t
    else if(varType == "L" ) { var->NewVarI(varVal,DefOpts::DefL, "L");  inVarTypes[nVarNow] = "L";  } // Long64_t
    else if(varType == "US") { var->NewVarU(varVal,DefOpts::DefUS,"US"); inVarTypes[nVarNow] = "US"; } // UShort_t
    else if(varType == "UI") { var->NewVarU(varVal,DefOpts::DefUI,"UI"); inVarTypes[nVarNow] = "UI"; } // UInt_t
    else if(varType == "UL") { var->NewVarU(varVal,DefOpts::DefUL,"UL"); inVarTypes[nVarNow] = "UL"; } // ULong64_t
    else if(varType == "F" ) { var->NewVarF(varVal,DefOpts::DefF, "F");  inVarTypes[nVarNow] = "F";  } // Float_t
    else if(varType == "D" ) { var->NewVarF(varVal,DefOpts::DefD, "D");  inVarTypes[nVarNow] = "F";  } // Double_t
    else {
      VERIFY(LOCATION,(TString)"found unsupported variable-type ("+varType+")",false);
    }

    bool prefoxCond = !varVal.BeginsWith("ANNZ",TString::kIgnoreCase);
    VERIFY(LOCATION,(TString)"Input variable names can not begin with \"ANNZ\" . Found ("+varVal+")",prefoxCond);
  }

  return;
}

// ===========================================================================================================
/**
 * @brief              - Parse a single line of input parameter values and fill them in a VarMaps() object
 * 
 * @param line         - a string which contains the list of input parameter values, which are fed into var
 * @param var          - The VarMaps() object in which the variables are filled
 * @param inVarNames   - vector which is contains the list of input parameter names
 * @param inVarTypes   - vector which is contains the list of input parameter types
 */
// ===========================================================================================================
bool CatFormat::inputLineToVars(TString line, VarMaps * var, vector <TString> & inVarNames, vector <TString> & inVarTypes) {
// =========================================================================================================================
  aLOG(Log::DEBUG_3) <<coutGreen<<" - CatFormat::inputLineToVars(): "<<coutYellow<<line<<coutDef<<endl;

  TString lineTest(line); lineTest.ReplaceAll(" ","");
  if(lineTest == "" || lineTest.BeginsWith("#")) return false;

  // transformations to regularize the line
  // -----------------------------------------------------------------------------------------------------------
  line.ReplaceAll("  "," ").ReplaceAll("  "," ").ReplaceAll(" ",",").ReplaceAll(",,",",").ReplaceAll(",,",",");
  line.ReplaceAll("\r\n", "").ReplaceAll("\n", "").ReplaceAll("\r", ""); // remove line-breaks
  if(line.BeginsWith(",")) line = line(1,line.Length());

  vector <std::string> words = utils->splitStringByChar((std::string)line,',');
  int nWords((int)words.size()), nVars((int)inVarNames.size());
  
  // verify that the line as the correct number of input variables  
  if(nWords != nVars) {
    aLOG(Log::ERROR) <<endl<< "line: " << line <<endl;
    for(int nWordNow=0; nWordNow<nWords; nWordNow++) aLOG(Log::ERROR) <<"words: "<<nWords<<CT<<nWordNow<<CT<<words     [nWordNow]<<endl;
    for(int nWordNow=0; nWordNow<nVars; nWordNow++)  aLOG(Log::ERROR) <<"vars:  "<<nVars <<CT<<nWordNow<<CT<<inVarNames[nWordNow]<<endl;
    VERIFY(LOCATION,(TString)"Input line has wrong number of variables !!!",false);
  }
  
  var->setDefaultVals();

  // -----------------------------------------------------------------------------------------------------------
  // go over the variable list extracted from the line and fill the var
  // -----------------------------------------------------------------------------------------------------------
  for(int nWordNow=0; nWordNow<nVars; nWordNow++) {
    TString wordNow = words[nWordNow];
    TString nameNow = inVarNames[nWordNow];
    TString typeNow = inVarTypes[nWordNow];

    VERIFY(LOCATION,(TString)" - got empty input for "+nameNow+" of type "+typeNow+" from input-line = "+line,(wordNow != ""));

    if     (typeNow == "F"  || typeNow == "D"                    ) { var->SetVarF(nameNow,(TString)wordNow); }
    else if(typeNow == "S"  || typeNow == "I"  || typeNow == "L" ) { var->SetVarI(nameNow,(TString)wordNow); }
    else if(typeNow == "B"                                       ) { var->SetVarB(nameNow,(TString)wordNow); }
    else if(typeNow == "C"                                       ) {
      if((wordNow.BeginsWith("\"") && wordNow.EndsWith("\"")) || (wordNow.BeginsWith("\'") && wordNow.EndsWith("\'"))) {
        int length = wordNow.Length();
        if     (length  > 2) wordNow = wordNow(1,length-2);
        else if(length == 2) wordNow = "";
      }
      var->SetVarC(nameNow,(TString)wordNow);
    }
    else if(typeNow == "US" || typeNow == "UI" || typeNow == "UL") { var->SetVarU(nameNow,(TString)wordNow); }
    else VERIFY(LOCATION,(TString)"found unsupported variable-type ("+typeNow+")",false);
  }

  words.clear();

  return true;
}

// ===========================================================================================================
/**
 * @brief          - set splitting variables
 * 
 * @param var      - The VarMaps() object into which the splitting variables which are written
 * @param rnd      - a random number generator
 * @param intMap   - input options
 */
// ===========================================================================================================
void CatFormat::setSplitVars(VarMaps * var, TRandom * rnd, map <TString,int> & intMap) {
// =====================================================================================
  int     nSplit            = glob->GetOptI("nSplit");
  int     nSplitType        = intMap["nSplitType"];
  int     nLine_splitBlocks = (nSplitType == 1) ? intMap["nLine_splitBlocks"] : 0;
  int     inFileSplitIndex  = (nSplitType == 3) ? intMap["inFileSplitIndex"]  : 0;
  TString indexName         = glob->GetOptC("indexName");
  TString splitName         = glob->GetOptC("splitName");
  TString testValidType     = glob->GetOptC("testValidType");

  // -----------------------------------------------------------------------------------------------------------
  // no splitting
  // -----------------------------------------------------------------------------------------------------------
  if(nSplit == 1) {
    intMap["nSplitTree"] = 0;
    var->SetVarI(splitName,     0);
    var->SetVarI(testValidType, 0);
  }
  // -----------------------------------------------------------------------------------------------------------
  // split into two or three sub-sets within two trees
  // -----------------------------------------------------------------------------------------------------------
  else {
    // -----------------------------------------------------------------------------------------------------------
    // "serial" - 
    // -----------------------------------------------------------------------------------------------------------
    if(nSplit == 2) {
      int resid2(0);

      // "serial" - 
      if     (nSplitType == 0) { resid2 = var->GetCntr("nLine") % 2; }
      // - "blocks" -
      else if(nSplitType == 1) { resid2 = (var->GetCntr("nLine") < nLine_splitBlocks) ? 0 : 1; }
      // - "random" -
      else if(nSplitType == 2) { resid2 = static_cast<int>(floor(rnd->Rndm() * 2)); }
      // - "byInFiles" -
      else if(nSplitType == 3) { resid2 = inFileSplitIndex; }
      // -----------------------------------------------------------------------------------------------------------
      // now set the variables
      // -----------------------------------------------------------------------------------------------------------
      if     (resid2 == 0) {
        intMap["nSplitTree"] = 0;
        var->SetVarI(splitName, var->GetCntr("nTrain"));
        var->IncCntr("nTrain");
      }
      else if(resid2 == 1) {
        intMap["nSplitTree"] = 1;
        var->SetVarI(splitName, var->GetCntr("nTest"));
        var->IncCntr("nTest");
      }
      var->SetVarI(testValidType, 0); // this will not be used if there is no three-way splitting
    }
    else {
      int resid3(0);

      // - "serial" - 
      if     (nSplitType == 0) { resid3 = var->GetCntr("nLine") % 3; }
      // - "blocks" -
      else if(nSplitType == 1) {
        if     (var->GetCntr("nLine") <   nLine_splitBlocks) resid3 = 0;
        else if(var->GetCntr("nLine") < 2*nLine_splitBlocks) resid3 = 1;
        else                                                 resid3 = 2;
      }
      // - "random" -
      else if(nSplitType == 2) { resid3 = static_cast<int>(floor(rnd->Rndm() * 3)); }
      // - "byInFiles" -
      else if(nSplitType == 3) { resid3 = inFileSplitIndex; }

      // -----------------------------------------------------------------------------------------------------------
      // now set the variables
      // -----------------------------------------------------------------------------------------------------------
      if     (resid3 == 0) {
        intMap["nSplitTree"] = 0;
        var->SetVarI(splitName,var->GetCntr("nTrain"));
        var->SetVarI(testValidType, 0);
        var->IncCntr("nTrain");
      }
      else if(resid3 == 1) {
        intMap["nSplitTree"] = 1;
        var->SetVarI(splitName,var->GetCntr("nTest"));
        var->SetVarI(testValidType, 0);
        var->IncCntr("nTest");
      }
      else if(resid3 == 2) {
        intMap["nSplitTree"] = 1;
        var->SetVarI(splitName,var->GetCntr("nValid"));
        var->SetVarI(testValidType, 1);
        var->IncCntr("nValid");
      }
    }
  }
  
  // the main counter
  var->SetVarI(indexName, var->GetCntr("nLine"));
  var->IncCntr("nLine");

  return;
}

// -----------------------------------------------------------------------------------------------------------
// general way to read in a csv file and split each line into local ariables
// -----------------------------------------------------------------------------------------------------------
// std::string inFileNameNow = "a.csv";
// std::ifstream  inputFile(inFileNameNow,ios::in);
// std::string line, item;
// while(!inputFile.eof()) {
//   // get an object
//   // -----------------------------------------------------------------------------------------------------------
//   getline(inputFile, line); if(line == "") break;

//   stringstream   ss(line);
//   vector<std::string> elems;
//   while(getline(ss, item, ',')) { elems.push_back(item); }

//   if(elems[0].at(0) == (std::string)"#") continue; // skip line begining with "#"


//   double val0 = atof(elems[1].c_str()); // convert std::string to double
//   int    val1 = atoi(elems[1].c_str()); // convert std::string to int

//   elems.clear();
// }
