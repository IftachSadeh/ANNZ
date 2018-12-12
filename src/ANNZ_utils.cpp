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
 * @brief    - Initialization of ANNZ
 *
 * @details  - Set internal glob variables and name-tags, perform sanity checks for the different
 *           configurations (training, optimization, verification or evaluation) and fix defualt values.
 *           - Validate consistency with the parameters used for the initial tree generation with CatFormat.
 *           - Load results from previous stages of ANNZ (e.g., load training results for an optimization run).
 */
// ===========================================================================================================
void ANNZ::Init() {
// ===========================================================================================================
  aLOG(Log::INFO)<<coutWhiteOnBlack<<coutBlue<<" - starting ANNZ::Init() " <<coutBlue<<" ... "<<coutDef<<endl;

  // unlock the global variables
  glob->setLock(false);

  // special case where we just create KNN weight trees
  // -----------------------------------------------------------------------------------------------------------
  if(glob->GetOptB("doGenInputTrees") && !glob->GetOptB("doOnlyKnnErr")) return;

  // -----------------------------------------------------------------------------------------------------------
  // TMVA stuff
  // -----------------------------------------------------------------------------------------------------------
  TMVA::Tools::Instance(); // This loads the TMVA library
  setupTypesTMVA();        // set-up local TMVA variables

  // -----------------------------------------------------------------------------------------------------------
  // internal names
  // -----------------------------------------------------------------------------------------------------------
  glob->NewOptC("sigBckTypeName","ANNZ_sigBckType");  // internal flag name for truth information
  glob->NewOptC("aTimeName","ANNZ_aTime");            // internal flag name for time operations
  glob->NewOptB("hasTruth",!glob->GetOptB("doEval")); // internal flag for adding a cut on the range of values of zTrg

  // deprecated
  // // internal parameters - the condition for defining the test/valid sub-smaples from the _valid tree in case of nSplit==3
  // glob->NewOptC("testValidType_train", glob->GetOptC("testValidType")+"<0.5"); // where [ testValidType == 0 ] for train
  // glob->NewOptC("testValidType_valid", glob->GetOptC("testValidType")+">0.5"); // where [ testValidType == 1 ] for valid

  if(glob->GetOptC("userWeights_train") == "") glob->SetOptC("userWeights_train","1"); // set proper init value for weights
  if(glob->GetOptC("userWeights_valid") == "") glob->SetOptC("userWeights_valid","1"); // set proper init value for weights

  // internal variable for sub-directory names, histogram names etc.
  TString typeANNZ("");
  if(glob->GetOptB("doRegression")) {
    if     (glob->GetOptB("doSingleReg")) typeANNZ = "singleReg";
    else if(glob->GetOptB("doRandomReg")) typeANNZ = "randomReg";
    else if(glob->GetOptB("doBinnedCls")) typeANNZ = "binnedCls";
  }
  else {
    if     (glob->GetOptB("doSingleCls")) typeANNZ = "singleCls";
    else if(glob->GetOptB("doRandomCls")) typeANNZ = "randomCls";
  }
  glob->NewOptC("typeANNZ",typeANNZ); glob->NewOptC("_typeANNZ",(TString)+"_"+typeANNZ);

  // -----------------------------------------------------------------------------------------------------------
  // get some previously used options if in evaluation
  // -----------------------------------------------------------------------------------------------------------
  if(glob->GetOptB("doEval")) {
    OptMaps * optMap = new OptMaps("localOptMap");
    optMap->copyOptStruct(glob);

    TString optimType    = glob->GetOptB("doBinnedCls") ? "verif" : "optim";
    TString saveFileName = getKeyWord("","baseConfig",optimType);
    aLOG(Log::DEBUG_1)<<coutYellow<<" - Getting some previous run information from "<<coutGreen
                      <<saveFileName<<coutYellow<<" ..."<<coutDef<<endl;

    vector <TString> prevOptNames; glob->GetAllOptNames(prevOptNames);
    utils->optToFromFile(&prevOptNames,optMap,saveFileName,"READ","SILENT_KeepFile",inLOG(Log::DEBUG_2));
    prevOptNames.clear();
  
    glob->copyOpt("nMLMs",         optMap,Log::DEBUG_1);
    glob->copyOpt("zTrg",          optMap,Log::DEBUG_1);
    glob->copyOpt("minValZ",       optMap,Log::DEBUG_1);
    glob->copyOpt("maxValZ",       optMap,Log::DEBUG_1);
    glob->copyOpt("binCls_nBins",  optMap,Log::DEBUG_1);
    glob->copyOpt("binCls_maxBinW",optMap,Log::DEBUG_1);
    glob->copyOpt("binCls_clsBins",optMap,Log::DEBUG_1);
    glob->copyOpt("userCuts_sig",  optMap,Log::DEBUG_1);
    glob->copyOpt("userCuts_bck",  optMap,Log::DEBUG_1);

    DELNULL(optMap);
  }

  // -----------------------------------------------------------------------------------------------------------
  // internal variables for controlling the debugging output of the TMVA factory
  // -----------------------------------------------------------------------------------------------------------
  TString analysisType(":AnalysisType=");
  if     ( glob->GetOptB("doBinnedCls") && glob->GetOptB("doMultiCls"))   analysisType += "Multiclass:";
  else if(!glob->GetOptB("doBinnedCls") && glob->GetOptB("doRegression")) analysisType += "Regression:";
  else                                                                    analysisType += "Classification:";

  TString factoryFlags(analysisType);
  factoryFlags += (TString)((glob->GetOptC("transANNZ") == "")  ? ""                  : (TString)":Transformations="+glob->GetOptC("transANNZ"));
  factoryFlags += (TString)( UseCoutCol                         ? ":Color"            : ":!Color");
  factoryFlags += (TString)( glob->GetOptB("isBatch")           ? ":!DrawProgressBar" : ":DrawProgressBar");
  factoryFlags += (TString)( inLOG(Log::DEBUG_1)                ? ":!Silent:V"        : ":Silent:!V");

  glob->NewOptC("factoryFlags" ,factoryFlags);
  glob->NewOptC("trainFlagsMLM","");
  // glob->NewOptC("trainFlagsMLM",(TString)(inLOG(Log::DEBUG_2) ? ":!H": ":!H"));

  if(glob->GetOptB("doTrain") && !glob->GetOptB("isBatch")) {
    aLOG(Log::WARNING) <<coutGreen<<" - training progress bar is active - if using a log file, it will become very large... (Can set "
                       <<coutYellow<<"[\"isBatch\""<<coutGreen<<" = true] instead.)" <<coutDef<<endl;
  }

  // -----------------------------------------------------------------------------------------------------------
  // check that the settings used to generate the input trees are consistent with the current settings,
  // and that the input trees exist (needed for all stages, not just for training)
  // -----------------------------------------------------------------------------------------------------------
  VERIFY(LOCATION,(TString)"Did not find directory with input trees at [ "+glob->GetOptC("inputTreeDirName")+" ] "+
                           "Must run GenerateInputTrees() first ...",(utils->isDirFile(glob->GetOptC("inputTreeDirName"))));

  vector <TString> optNames;
  optNames.push_back("nSplit"); optNames.push_back("treeName");  optNames.push_back("indexName"); optNames.push_back("useWgtKNN");
  // optNames.push_back("splitName"); optNames.push_back("testValidType"); // deprecated
  
  if(glob->GetOptB("storeOrigFileName")) optNames.push_back("origFileName");

  utils->optToFromFile(&optNames,glob,glob->GetOptC("userOptsFile_genInputTrees"),"READ","WARNING_KeepFile",inLOG(Log::DEBUG));

  optNames.clear();

  // -----------------------------------------------------------------------------------------------------------
  // check the versionTag from the generated input trees
  // -----------------------------------------------------------------------------------------------------------
  OptMaps * optMap = new OptMaps("localOptMap");

  optNames.push_back(glob->versionTag()); optMap->NewOptC(glob->versionTag(), glob->GetOptC(glob->versionTag()));

  utils->optToFromFile(&optNames,optMap,glob->GetOptC("userOptsFile_genInputTrees"),"READ","SILENT_KeepFile",inLOG(Log::DEBUG));

  if(utils->getCodeVersionDiff(optMap->GetOptC(glob->versionTag())) != 0) {
    aLOG(Log::WARNING) <<coutWhiteOnBlack<<" - Got \"versionTag\" = "<<coutPurple<<optMap->GetOptC(glob->versionTag())
                       <<coutWhiteOnBlack<<" from "<<coutYellow<<glob->GetOptC("userOptsFile_genInputTrees")
                       <<coutWhiteOnBlack<<", while the current running version is "
                       <<coutPurple<<glob->GetOptC(glob->versionTag())<<coutWhiteOnBlack
                       <<" - we can go on, but please consider generating this directory from scratch ..." <<coutDef<<endl;
  }

  DELNULL(optMap);
  optNames.clear();


  int nSplit(glob->GetOptI("nSplit"));
  if(!glob->GetOptB("doEval") && !glob->GetOptB("doOnlyKnnErr")) {
    VERIFY(LOCATION,(TString)"Cant run with [\"nSplit\" = "+utils->intToStr(nSplit)+"] ..."
                            +" Must set to 2 when generating the input trees !",(nSplit == 2));
  }

  // deprecated
  // // flag for splitting the sample into two (train,valid) or three (train,test,valid)
  // // (must come after reading-in userOptsFile_genInputTrees, which may override nSplit)
  // glob->NewOptB("separateTestValid",(nSplit == 3)); // internal flag

  // -----------------------------------------------------------------------------------------------------------
  // single regression/classification overrides
  // -----------------------------------------------------------------------------------------------------------
  if(glob->GetOptB("doSingleReg") || glob->GetOptB("doSingleCls")) {
    // common settings
    if(glob->GetOptI("nMLMs") > 1) {
      VERIFY(LOCATION,(TString)"Must set (nMLMs = 1) in single regression/classification mode ...",(glob->GetOptI("nMLMs") == 1));
    }
  }

  // -----------------------------------------------------------------------------------------------------------
  // binned classification
  // -----------------------------------------------------------------------------------------------------------
  if(glob->GetOptB("doBinnedCls")) {
    // number of MLMs to generate for each classification-bin (only the 'best' one is kept)
    if(glob->GetOptI("binCls_nTries") < 1) {
      int nTriesDef = 5;
      glob->SetOptI("binCls_nTries" , nTriesDef);
      aLOG(Log::WARNING) <<coutRed<<" - Found \"binCls_nTries\" = "<<coutPurple<<glob->GetOptI("binCls_nTries")
                         <<coutRed<<" ... Setting to "<<coutGreen<<nTriesDef<<coutDef<<endl;
    }

    // if binCls_nBins<0 the defaut number of bins is used. otherwise, this will be the number of bins
    if(glob->GetOptC("binCls_clsBins") == "") {
      if(glob->GetOptI("binCls_nBins") < 1) {
        int nBinsDef = 100;
        glob->SetOptI("binCls_nBins" , nBinsDef);
        aLOG(Log::WARNING) <<coutRed<<" - Found \"binCls_nBins\" = "<<coutPurple<<glob->GetOptI("binCls_nBins")
                           <<coutRed<<" ... Setting to "<<coutGreen<<nBinsDef<<coutDef<<endl;
      }
      VERIFY(LOCATION,(TString)"Configuration problem... if setting \"binCls_nBins\" (dynamic bins for binned classification) "+
                               "than need to also set \"binCls_maxBinW\" (max bin width)",(glob->GetOptF("binCls_maxBinW") > 0));
    }
    else {
      vector <TString> userBinV  = utils->splitStringByChar(glob->GetOptC("binCls_clsBins"),';');
      int              nUserBins = (int)userBinV.size() - 1;

      VERIFY(LOCATION,(TString)"Configuration problem... if setting \"binCls_clsBins\" must set the correct "+
                               "corresponding number of bins in \"binCls_nBins\"",(glob->GetOptI("binCls_nBins") == nUserBins));
      VERIFY(LOCATION,(TString)"Must set exactly one of the two bin settings, either [binCls_clsBins != \"\"] or [\"binCls_maxBinW\" > 0]"
                              ,(glob->GetOptF("binCls_maxBinW") < EPS));

      userBinV.clear();
    }

    // in binned classification, nMLMs should be set to binCls_nBins so that setTags() and all 
    // the rest of the internal naming settings work... (must happen at the top of Init() !!!)
    // -----------------------------------------------------------------------------------------------------------
    glob->SetOptI("nMLMs",  glob->GetOptI("binCls_nBins"));
    glob->SetOptI("nMLMnow",glob->GetOptI("nBinNow"));
  }


  // -----------------------------------------------------------------------------------------------------------
  // sanity checks
  // -----------------------------------------------------------------------------------------------------------
  VERIFY(LOCATION,(TString)"Must set (nMLMs>0)"      ,(glob->GetOptI("nMLMs") > 0));
  VERIFY(LOCATION,(TString)"Must set (nMLMnow<nMLMs)",(glob->GetOptI("nMLMs") > glob->GetOptI("nMLMnow")));

  double minValZ(glob->GetOptF("minValZ")), maxValZ(glob->GetOptF("maxValZ")), diffValZ(maxValZ - minValZ);

  if(glob->GetOptB("doRegression") && !glob->GetOptB("doOnlyKnnErr")) {
    VERIFY(LOCATION,(TString)"Must set (minValZ < maxValZ)",(minValZ < maxValZ));
  }

  // underflow/overflow options for plotting
  if(glob->GetOptI("nUnderflowBins")  < 0)   glob->SetOptI("nUnderflowBins",  0);
  if(glob->GetOptI("nOverflowBins")   < 0)   glob->SetOptI("nOverflowBins",   0);
  if(glob->GetOptF("underflowZwidth") < EPS) glob->SetOptF("underflowZwidth", (glob->GetOptI("nUnderflowBins") > 0) ? diffValZ * 0.1 : 0);
  if(glob->GetOptF("overflowZwidth")  < EPS) glob->SetOptF("overflowZwidth",  (glob->GetOptI("nOverflowBins")  > 0) ? diffValZ * 0.1 : 0);
  if(glob->GetOptF("underflowZ")      < EPS) glob->SetOptF("underflowZ",      glob->GetOptF("minValZ") - glob->GetOptF("underflowZwidth"));
  if(glob->GetOptF("overflowZ")       < EPS) glob->SetOptF("overflowZ",       glob->GetOptF("maxValZ") + glob->GetOptF("overflowZwidth"));

  // -----------------------------------------------------------------------------------------------------------
  // check the definition of PDF bins, and compute nPDFbins from pdfBinWidth if needed
  // -----------------------------------------------------------------------------------------------------------
  if(!glob->GetOptB("doTrain")) {
    // for single-regression there is no pdf, so just set a sensible default value just in case
    if(glob->GetOptB("doSingleReg")) {
      glob->SetOptI("nPDFbins",1);
    }
    // for randomized regression there is the option for the user to define a bin-width instead of the number of bins
    else if(glob->GetOptB("doRandomReg")) {
      if(glob->GetOptI("nPDFs") <= 0) glob->SetOptI("nPDFbins",1);

      // check if a specialized binning definition has been given by the user
      if(glob->GetOptC("userPdfBins") != "") {
        aLOG(Log::INFO) <<coutPurple<<" - Found \"userPdfBins\" = "<<coutRed<<glob->GetOptC("userPdfBins")<<coutPurple
                        <<" - will ignore \"nPDFbins\" and \"pdfBinWidth\" ..."<<coutDef<<endl;
      }
      else {
        double binWidth = glob->GetOptF("pdfBinWidth");

        // check if need to both nPDFbins and pdfBinWidth are set and give warning
        if(glob->GetOptI("nPDFbins") > 0) {
          if(binWidth > 0) {
            aLOG(Log::WARNING) <<coutRed <<" - Both \"nPDFbins\" and \"pdfBinWidth\" are set..."
                               <<" - will ignore \"pdfBinWidth\" "<<coutDef<<endl;
          }
        }
        // compute nPDFbins from pdfBinWidth if needed
        else {
          VERIFY(LOCATION,(TString)"Configuration problem... must set either \"nPDFbins\" or \"binWidth\" ...",(binWidth > 0));

          int    nPDFbins  = static_cast<int>(floor(0.00001+diffValZ/binWidth));
          double finalBinW = diffValZ/double(nPDFbins);

          VERIFY(LOCATION,(TString)"Configuration problem... \"binWidth\" inconsistent with max/minValZ range ...",(nPDFbins >= 1));

          glob->SetOptI("nPDFbins",nPDFbins);

          aLOG(Log::INFO) <<coutGreen<<" - For \"pdfBinWidth\" = "<<binWidth<<", derived \"nPDFbins\" = "<<coutBlue<<nPDFbins<<coutDef<<endl;

          if(fabs(binWidth - finalBinW)/binWidth > 1e-5) {
            aLOG(Log::WARNING) <<coutRed <<" - \"nPDFbins\" must be an integer --> using a modified the value, "
                               <<"\"pdfBinWidth\" = "<<coutBlue<<finalBinW<<coutRed<<" (instead of "<<binWidth<<")"<<coutDef<<endl;
          }
        }
      }
    }
    // for randomized regression, this is checked in setInfoBinsZ(), as it is possible to define userPdfBins instead of nPDFbins.
    // however, for binned classification, there is no cohice but to explicitly set nPDFbins
    else if(glob->GetOptB("doBinnedCls")) {
      VERIFY(LOCATION,(TString)"Must set number of PDF bins (\"nPDFbins\" > 0)",(glob->GetOptI("nPDFbins") > 0));
    }
  }


  // -----------------------------------------------------------------------------------------------------------
  // consistency checks for the different analysis types
  // -----------------------------------------------------------------------------------------------------------
  bool isGoodRegClsType = ( (glob->GetOptB("doClassification")?1:0) + (glob->GetOptB("doRegression")?1:0) == 1 );
  VERIFY(LOCATION,(TString)"Configuration problem... Need exactly one of \"doClassification\", \"doRegression\" options ...",isGoodRegClsType);

  if(glob->GetOptB("doRegression")) {
    int nOpts(0);
    if(glob->GetOptB("doSingleReg")) nOpts++; if(glob->GetOptB("doRandomReg")) nOpts++; if(glob->GetOptB("doBinnedCls")) nOpts++;
    VERIFY(LOCATION,(TString)"Configuration problem... Exactly one of "
                             +"\"doSingleReg\", \"doRandomReg\" and \"doBinnedCls\" neeeds to be true",(nOpts == 1));
  }
  else {
    int nOpts(0);
    if(glob->GetOptB("doSingleCls")) nOpts++; if(glob->GetOptB("doRandomCls")) nOpts++;
    VERIFY(LOCATION,(TString)"Configuration problem... Exactly one of "
                             +"\"doSingleCls\" and \"doRandomCls\" neeeds to be true",(nOpts == 1));
    
    if(glob->GetOptC("MLMsToStore") == "") glob->SetOptC("MLMsToStore","BEST");
  }

  // check for errors in addOutputVars
  vector<TString> addOutputVarsV = utils->splitStringByChar(glob->GetOptC("addOutputVars"),';');
  for(int nOutputVarNow=0; nOutputVarNow<(int)addOutputVarsV.size(); nOutputVarNow++) {
    TString outputVar(addOutputVarsV[nOutputVarNow]);

    VERIFY(LOCATION,(TString)"Can not include "+outputVar+" in \"addOutputVars\", as "
                            +"it begins with \""+glob->GetOptC("basePrefix")+"\" ... "
                            +"did you mean to add it to \"MLMsToStore\" ???",
                            !(outputVar.BeginsWith(glob->GetOptC("basePrefix"))));
  }

  if(!glob->GetOptB("doRegression")) glob->SetOptB("doBiasCorPDF",false);

  // number of PDF types - either generate no PDF, or choose up to two types
  // -----------------------------------------------------------------------------------------------------------
  // currently implemented in getRndMethodBestPDF()
  if     (glob->GetOptB("doSingleReg")) {
    if(glob->GetOptI("nPDFs") > 0) {
      aLOG(Log::WARNING) <<coutRed<<" - found nPDFs = "<<coutGreen<<glob->GetOptI("nPDFs")<<coutRed<<" ... setting to 0"<<coutDef<<endl;
    }
    glob->SetOptI("nPDFs", 0);
  }
  else if(glob->GetOptB("doRandomReg")) {
    if(glob->GetOptI("nPDFs") == 1 && glob->GetOptB("addOldStylePDFs")) {
      aLOG(Log::WARNING) <<coutRed<<" - found addOldStylePDFs = true and nPDFs = 1 ... addOldStylePDFs will be ignored"
                         <<" - in order to derive the old-style PDFs in addition, set nPDFs > 1"<<coutDef<<endl;
    }
    else if(glob->GetOptI("nPDFs") > 1 && !glob->GetOptB("addOldStylePDFs")) {
      aLOG(Log::WARNING) <<coutRed<<" - found nPDFs = "<<coutGreen<<glob->GetOptI("nPDFs")<<coutRed<<" ... setting to 1"
                         <<" - in order to derive the old-style PDFs in addition, set addOldStylePDFs to true"<<coutDef<<endl;
      glob->SetOptI("nPDFs", 1);
    }
    else if(glob->GetOptI("nPDFs") > 3) {
      aLOG(Log::WARNING) <<coutRed<<" - found nPDFs = "<<coutGreen<<glob->GetOptI("nPDFs")<<coutRed<<" ... setting to 3"<<coutDef<<endl;
    }
    glob->SetOptI("nPDFs", min(max(glob->GetOptI("nPDFs"),0),3));
  }
  // either the baseline PDF, or that as well as another, which includes uncertainty-smearing, see doEvalReg()
  else if(glob->GetOptB("doBinnedCls")) {
    if(glob->GetOptI("nPDFs") > 2) {
      aLOG(Log::WARNING) <<coutRed<<" - found nPDFs = "<<coutGreen<<glob->GetOptI("nPDFs")<<coutRed<<" ... setting to 2"<<coutDef<<endl;
    }
    glob->SetOptI("nPDFs", min(max(glob->GetOptI("nPDFs"),1),2));
  }
  // no PDFs currently defined for classification
  else {
    glob->SetOptI("nPDFs", 0);
  }

  if(glob->GetOptB("doRegression") && !glob->GetOptB("doTrain") && !glob->GetOptB("doOnlyKnnErr")) {
    aLOG(Log::INFO) <<coutBlue<<" - Will generate "<<coutYellow<<glob->GetOptI("nPDFs")<<coutBlue<<" PDFs ... "<<coutDef<<endl;

    // set this to true, so that we always look for bis-correction
    // MLMs (if these are not found, nothing bad happens...)
    glob->SetOptB("doBiasCorMLM",true);
  }

  if(glob->GetOptB("doRegression")) {
    VERIFY(LOCATION,(TString)"Must set the name of the regression target, \"zTrg\" ",(glob->GetOptC("zTrg") != ""));

    if(glob->GetOptC("zTrgTitle") == "") glob->SetOptC("zTrgTitle",glob->GetOptC("zTrg"));
    if(glob->GetOptC("zRegTitle") == "") glob->SetOptC("zRegTitle","regression value");

    // if(glob->GetOptF("zClosBinWidth") < EPS) {
    //   double norm = 50;
    //   if(glob->GetOptI("nPDFs") > 0 && glob->GetOptI("nPDFbins") > norm) norm = glob->GetOptI("nPDFbins");
      
    //   glob->SetOptF("zClosBinWidth", (maxValZ - minValZ)/norm);
    // }
    if(glob->GetOptF("zPlotBinWidth") < EPS) {
      glob->SetOptF("zPlotBinWidth", (maxValZ - minValZ)/10.);
    }
    if(glob->GetOptI("nDrawBins_zTrg") < 2) {
      glob->SetOptI("nDrawBins_zTrg", static_cast<int>(floor(0.00001+(maxValZ - minValZ)/0.05)));
    }

    bool   hasGoodBinWidthZ = (    (glob->GetOptF("zClosBinWidth") > 0 && glob->GetOptF("zClosBinWidth") <= diffValZ/5.)
                                || (glob->GetOptF("zClosBinWidth") < 0                                                 ) );

    VERIFY(LOCATION,(TString)"Must set ((zClosBinWidth > 0) and (zClosBinWidth <= (maxValZ-minValZ)/5)) or (zClosBinWidth < 0) ...",hasGoodBinWidthZ);

    setInfoBinsZ();

    double excludeRange = glob->GetOptF("excludeRangePdfModelFit");
    if(excludeRange < 0 || excludeRange > 0.4) {
      aLOG(Log::WARNING) <<coutRed <<" - Found [\"excludeRangePdfModelFit\" = "<<coutYellow<<excludeRange
                         <<coutRed<<"], which must be within [0,0.4]. Setting to 0 ..."<<coutDef<<endl;

      glob->SetOptF("excludeRangePdfModelFit", 0);
    }

    // validate correct setting for the primary condition for optimization, and set the corresponding title
    if(glob->GetOptC("optimCondReg") == "fracSig68") {
      aLOG(Log::WARNING) <<coutRed <<" - Found [\"optimCondReg\" = "<<coutYellow<<glob->GetOptC("optimCondReg")
                         <<coutRed<<"] which is deprecated...  Will change this to "<<coutGreen<<"sig68"<<coutDef<<endl;
      
      glob->SetOptC("optimCondReg","sig68");
    }
   
    if     (glob->GetOptC("optimCondReg") == "sig68")     glob->NewOptC("optimCondRegtitle", "#sigma_{68}");
    else if(glob->GetOptC("optimCondReg") == "bias")      glob->NewOptC("optimCondRegtitle", "Bias");
    else if(glob->GetOptC("optimCondReg") == "fracSig68") glob->NewOptC("optimCondRegtitle", "f(2,3#sigma_{68})");
    else VERIFY(LOCATION,(TString)"Configuration problem... \"optimCondReg\" should have one of the "+
                                  "following values: \"sig68\", \"bias\" or \"fracSig68\" options ...",false);
    
    if(glob->GetOptF("minPdfWeight") > 1 || glob->GetOptF("minPdfWeight") < EPS) {
      aLOG(Log::WARNING) <<coutRed<<" - found minPdfWeight > 1 ... minPdfWeight will be ignored"<<coutDef<<endl;
      glob->SetOptF("minPdfWeight", -1);
    }
  }

  // set flag for generating uncertainty estimators for classification MLMs (needed for the second PDF in binned classification)
  glob->NewOptB("needBinClsErr", (!glob->GetOptB("doTrain") && glob->GetOptB("doBinnedCls") && glob->GetOptI("nPDFs") == 2));

  // number of random MLM weighting schemes to generate as part of getRndMethodBestPDF()
  if(glob->GetOptI("nRndPdfWeightTries") <= 0) glob->NewOptI("nRndPdfWeightTries",30);

  // number of random smearing to perform when folding uncertainty estimates into MLM solutions for PDF generation
  glob->SetOptI("nSmearsRnd",max(glob->GetOptI("nSmearsRnd"),50));                                    // set minimal value
  if(glob->GetOptI("nSmearsRnd") % 2 == 1) glob->SetOptI("nSmearsRnd",glob->GetOptI("nSmearsRnd")+1); // needs to be an even number

  glob->SetOptI("nSmearUnf",max(glob->GetOptI("nSmearUnf"),glob->GetOptI("nSmearsRnd") * 2));           // set minimal value

  // a lower acceptance bound to check if too few MLMs are trained or if something went wrong with the optimization procedure
  // (e.g., not enough trained MLMs have 'good' combinations of scatter, bias and outlier-fraction metrics).
  int minAcptMLMsForPDFs(3);
  if(glob->GetOptI("minAcptMLMsForPDFs") < minAcptMLMsForPDFs) {
    aLOG(Log::WARNING) <<coutRed <<" - Found minAcptMLMsForPDFs = "
                       <<coutBlue<<glob->GetOptI("minAcptMLMsForPDFs")
                       <<coutRed<<" Setting to minimal value: "
                       <<coutYellow<<minAcptMLMsForPDFs<<coutDef<<endl;
    glob->NewOptI("minAcptMLMsForPDFs",minAcptMLMsForPDFs);
  }

  // number of times to divide the collection of MLMs needed for evaluation - in principle, no division is neccessary, however,
  // some MLMs (notable BDTs) require a lot of memory. Therefore, it might be better to only evaluate a sub-sample of the MLMs
  // at a time, and then combine the results when the actuall pdfs are evaluated. A high value of nDivEvalLoops will reduce the
  // memory usage, but will also slow down the code
  if(glob->GetOptI("nDivEvalLoops") < 1) glob->SetOptI("nDivEvalLoops",1);

  // number of initial bins and rebin factor for classification response histograms
  int hisBinsN  = glob->GetOptI("clsResponseHisN");  int rebinFactor = glob->GetOptI("clsResponseHisR");
  int compPureN = static_cast<int>(floor(hisBinsN/double(rebinFactor)));
  VERIFY(LOCATION,(TString)"\"clsResponseHisN\" divided by \"clsResponseHisR\" must produce an integer",(hisBinsN % compPureN == 0));

  // defualt values (if not given by the user) for the lower and upper bounds for closure histograms
  if(fabs(glob->GetOptF("closHisL") - glob->GetOptF("closHisH")) < 1e-10) {
    glob->SetOptF("closHisL",( (maxValZ - minValZ)/2. - 4.5 * (maxValZ - minValZ) ));
    glob->SetOptF("closHisH",( (maxValZ - minValZ)/2. + 4.5 * (maxValZ - minValZ) ));
  } 

  // round up closHisN, so may be divided by some factor to finally equal nDrawBins_zTrg
  int nDrawBins_zTrg = glob->GetOptI("nDrawBins_zTrg");
  if(nDrawBins_zTrg < 2) { nDrawBins_zTrg = 10; glob->SetOptI("nDrawBins_zTrg",10); }

  int closHisN  = glob->GetOptI("closHisN");
  int roundFact = static_cast<int>(floor(10*EPS+closHisN/double(nDrawBins_zTrg)));
  closHisN      = static_cast<int>(closHisN/float(roundFact)) * roundFact;
  glob->SetOptI("closHisN",closHisN);

  // which types of MLM should be randomely generated
  TString rndOptTypes = glob->GetOptC("rndOptTypes");
  if(rndOptTypes == "") {
    aLOG(Log::WARNING) <<coutRed <<" - Found rndOptTypes = \"\". "
                       <<coutBlue<<"No randomized MLM options will be generated... Make sure that userMLMopts is defined."<<coutDef<<endl;
  }
  else {
    VERIFY(LOCATION,(TString)"Unsupported MLM type given in \"rndOptTypes\". Choose \"\", \"ANN\", \"BDT\" or \"ANN_BDT\""
                            ,((rndOptTypes == "ANN") || (rndOptTypes == "BDT") || (rndOptTypes == "ANN_BDT") || (rndOptTypes == "BDT_ANN")) );
  }

  // number of randomly generated MLM values used to propagate the uncertainty on the input parameters to the MLM-estimator.
  // This is procedure is not used by default - instead we have the KNN uncertainty estimator.
  // nErrINP needs to be an even number larger than some reasonable minimal threshold
  // -----------------------------------------------------------------------------------------------------------
  int nErrINP(glob->GetOptI("nErrINP"));
  if(nErrINP < 1) { nErrINP = glob->GetOptI("nErrKNN"); } else if(nErrINP < 40) { nErrINP = 40; }
  if(nErrINP % 2 == 1)  nErrINP++;
  glob->SetOptI("nErrINP",nErrINP);

  // set the size of the vectors which holds the MLM types
  typeMLM.resize(glob->GetOptI("nMLMs"),TMVA::Types::kVariable);

  // set the size of the vector which holds the option for each MLM in single/randomized
  // regression to include the nominal MLM as an input parameter for the MLM bias correction
  // (for training, we initialize here from the global variable. otherwise, we load this
  // option per-MLM as part of loadOptsMLM())
  // -----------------------------------------------------------------------------------------------------------
  hasBiasCorMLMinp.resize(glob->GetOptI("nMLMs"), glob->GetOptB("biasCorMLMwithInp"));

  // internal base-names for PDFs generated by TMVA - These should not be changed unless some convention
  // changes in TMVA - These are not to be confused with the PDFs which are computed by ANNZ !!!
  // -----------------------------------------------------------------------------------------------------------
  glob->NewOptI("nTMVApdfBins",10);
  TString pdfStrReplace, pdfStrBase, pdfName, pdfStr;
  pdfStrReplace = (TString)"_REPLACE_";
  pdfStrBase    = (TString)"PDFInterpol"+pdfStrReplace+"=Spline2:Nbins"+pdfStrReplace
                           +"="+utils->intToStr(glob->GetOptI("nTMVApdfBins"))+":NSmooth"+pdfStrReplace+"=5";
  
  glob->NewOptC("pdfStr","");
  if(glob->GetOptB("doClassification")) {  //glob->GetOptB("doBinnedCls")
    pdfName = (TString)"MVAPdf"; // name used by TMVA
    pdfStr  = (TString)":CreateMVAPdfs:"+pdfStrBase;  pdfStr.ReplaceAll(pdfStrReplace,pdfName);
    
    glob->NewOptC("pdfStr",pdfStr);
  }

  // options for pdf used for calculation of separtion between sig/bck of classifications
  pdfName = (TString)"ANNZ_SepPDF";
  pdfStr  = (TString)pdfStrBase;  pdfStr.ReplaceAll(pdfStrReplace,"");
  glob->NewOptC("pdfSepName",pdfName);
  glob->NewOptC("pdfSepStr" ,pdfStr);

  // options for pdf used for calculation of separtion between sig/bck of classifications
  pdfName = (TString)"ANNZ_mcPDF";
  pdfStr  = (TString)pdfStrBase;  pdfStr.ReplaceAll(pdfStrReplace,"");
  glob->NewOptC("pdfMcName",pdfName);
  glob->NewOptC("pdfMcStr" ,pdfStr);

  // -----------------------------------------------------------------------------------------------------------
  // set MLM names and default titles
  // -----------------------------------------------------------------------------------------------------------
  setTags();

  // -----------------------------------------------------------------------------------------------------------
  // check if trained MLMs already exist (to set mlmSkip accordingly) and decide wether training is needed now
  // -----------------------------------------------------------------------------------------------------------
  int     nFoundMLMs(0);
  bool    trainingNeeded(true);
  TString allMLMsIn(""), allMLMsOut("");
  for(int nMLMnow=0; nMLMnow<glob->GetOptI("nMLMs"); nMLMnow++) {
    if(glob->GetOptB("doTrain") && nMLMnow != glob->GetOptI("nMLMnow")) continue;

    TString MLMname        = getTagName(nMLMnow);
    TString outXmlFileName = getKeyWord(MLMname,"trainXML","outXmlFileName");
    mlmSkip[MLMname]       = !(verifyXML(outXmlFileName));

    // for doBinnedCls, expect to find directories by a pattern like [./output/test_binCls_quick/binCls/train/ANNZ_39/*_nTries]
    if(glob->GetOptB("doBinnedCls") && !mlmSkip[MLMname]) {
      TString trainDirName = getKeyWord(MLMname,"trainXML","trainDirNameFullNow");
      TString nTriesTag    = getKeyWord(MLMname,"postTrain","nTriesTag");
      TString nTryDirPatt  = (TString)"ls "+trainDirName+"*"+nTriesTag;
      int     sysReturn    = 0; 
      TString sysOut       = utils->getShellCmndOutput(nTryDirPatt,NULL,false,false,&sysReturn);
      bool    missingDir   = (sysReturn != 0 || sysOut == "");

      if(missingDir) aLOG(Log::DEBUG_1)<<coutRed<<" ... No output for - "<<coutCyan<<nTryDirPatt<<coutDef<<endl;

      mlmSkip[MLMname] = missingDir;
    }

    if(mlmSkip[MLMname]) { allMLMsOut += (TString)coutRed  +MLMname+coutYellow+",";               }
    else                 { allMLMsIn  += (TString)coutGreen+MLMname+coutYellow+","; nFoundMLMs++; }

    if(!glob->GetOptB("doTrain")) {
      aLOG(Log::DEBUG)  <<coutGreen<<" ... "<<(TString)(mlmSkip[MLMname] ? (TString)coutRed+"REJECT " : (TString)coutBlue+"ACCEPT ")
                        <<coutGreen<<MLMname<<"  ("<<outXmlFileName<<")"<<coutDef<<endl;
    }

    // if doTrain and this MLM has already bee trained, only proceed is overwriteExistingTrain
    if(glob->GetOptB("doTrain") && !mlmSkip[MLMname] && !glob->GetOptB("overwriteExistingTrain")) {
      trainingNeeded = false;

      aLOG(Log::INFO) <<coutWhiteOnGreen<<" - found trained \""<<coutYellow<<MLMname<<coutWhiteOnGreen<<"\" ("<<outXmlFileName<<")"<<coutDef<<endl;
      aLOG(Log::INFO) <<coutGreen<<" -- Nothing to be done... (can force re-training with \"overwriteExistingTrain\")"<<coutDef<<endl;
    }
  }
  if(!glob->GetOptB("doTrain") && !glob->GetOptB("doOnlyKnnErr")) {
    aLOG(Log::INFO) <<coutPurple<<LINE_FILL('-',56)                   <<coutDef<<endl;
    aLOG(Log::INFO) <<coutBlue  <<" - All ACCEPTED MLMs: "<<allMLMsIn <<coutDef<<endl;
    aLOG(Log::INFO) <<coutBlue  <<" - All REJECTED MLMs: "<<allMLMsOut<<coutDef<<endl;
    aLOG(Log::INFO) <<coutPurple<<LINE_FILL('-',112)                  <<coutDef<<endl;

    if(glob->GetOptB("doBinnedCls")) {
      VERIFY(LOCATION,(TString)"Can not proceed if there are untrained MLMs ... Please re-run training !",(nFoundMLMs == glob->GetOptI("nMLMs")));
    }
    VERIFY(LOCATION,(TString)"No trained MLMs found ... Please re-run training !",(nFoundMLMs > 0));
  }

  if(glob->GetOptB("doOnlyKnnErr")) trainingNeeded = false;
  
  // initialize the outputs with the training directory and reset it
  if(trainingNeeded && !glob->GetOptB("isReadOnlySys")) {
    outputs->InitializeDir(glob->GetOptC("outDirNameFull"),glob->GetOptC("baseName"));
  }
  glob->NewOptB("trainingNeeded",trainingNeeded);

  // -----------------------------------------------------------------------------------------------------------
  // If training set the variables for the MLMs.
  // If not training, get the configuration options for each MLM from file.
  // -----------------------------------------------------------------------------------------------------------
  loadOptsMLM();

  // finally, lock the global variables so that they can not be changed by accident
  glob->setLock(true);

  // save the final configuration options to file
  // -----------------------------------------------------------------------------------------------------------
  bool saveConfig = ( ( (glob->GetOptB("doTrain") && trainingNeeded) || !glob->GetOptB("doTrain") ) && !glob->GetOptB("isReadOnlySys") );
  if(saveConfig) {
    TString saveFileName = getKeyWord("","baseConfig","current");
    aLOG(Log::INFO)<<coutYellow<<" - Saving run information in "<<coutGreen<<saveFileName<<coutYellow<<" ..."<<coutDef<<endl;

    vector <TString> optNames; glob->GetAllOptNames(optNames);
    utils->optToFromFile(&optNames,glob,saveFileName,"WRITE");
    optNames.clear();
  }

  // safe initialization for the eval manager
  // -----------------------------------------------------------------------------------------------------------
  aRegEval = NULL;

  return;
}

// ===========================================================================================================
/**
 * @brief  - Set internal name-tags for MLMs, corresponding weights and errors, PDF bins etc.
 */
// ===========================================================================================================
void ANNZ::setTags() {
// ===========================================================================================================
  int nMLMs    = glob->GetOptI("nMLMs");
  int nPDFs    = glob->GetOptI("nPDFs");
  int nPDFbins = glob->GetOptI("nPDFbins");

  // set the base tag names for the different MLM and PDF components
  TString baseTag_v      = "_val";    glob->NewOptC("baseTag_v",      baseTag_v);
  TString baseTag_e      = "_err";    glob->NewOptC("baseTag_e",      baseTag_e);
  TString baseTag_w      = "_wgt";    glob->NewOptC("baseTag_w",      baseTag_w);
  TString baseTag_MLM_a  = "_MLM_a";  glob->NewOptC("baseTag_MLM_avg",baseTag_MLM_a);
  TString baseTag_PDF_a  = "_PDF_a";  glob->NewOptC("baseTag_PDF_avg",baseTag_PDF_a);
  TString baseTag_PDF_m  = "_PDF_m";  glob->NewOptC("baseTag_PDF_max",baseTag_PDF_m);
  TString baseTag_errKNN = "_dzTrg";  glob->NewOptC("baseTag_errKNN", baseTag_errKNN);
  
  mlmBaseTag  .clear();
  mlmTagName  .resize(nMLMs); mlmTagErr   .resize(nMLMs); mlmTagErrKNN.resize(nMLMs);
  mlmTagWeight.resize(nMLMs); mlmTagClsVal.resize(nMLMs); mlmTagIndex .resize(nMLMs);
  mlmTagBias  .resize(nMLMs);

  inErrTag.resize(nMLMs,vector<TString>(0,"")); // the internal length is set in setNominalParams()
  
  for(int nMLMnow=0; nMLMnow<nMLMs; nMLMnow++) {
    TString MLMname = (TString)glob->GetOptC("basePrefix")+utils->intToStr(nMLMnow); // format of "ANNZ_0"

    mlmTagName  [nMLMnow]  = (TString)MLMname;                 mlmBaseTag[mlmTagName  [nMLMnow]]   = MLMname;
    mlmTagWeight[nMLMnow]  = (TString)MLMname+baseTag_w;       mlmBaseTag[mlmTagWeight[nMLMnow]]   = MLMname;
    mlmTagBias  [nMLMnow]  = (TString)MLMname+"_bias";         mlmBaseTag[mlmTagBias  [nMLMnow]]   = MLMname;
    mlmTagClsVal[nMLMnow]  = (TString)MLMname+"_clsVal";       mlmBaseTag[mlmTagClsVal[nMLMnow]]   = MLMname;
    mlmTagIndex [nMLMnow]  = (TString)MLMname+"_index";        mlmBaseTag[mlmTagIndex [nMLMnow]]   = MLMname;
    mlmTagErrKNN[nMLMnow]  = (TString)MLMname+baseTag_errKNN;  mlmBaseTag[mlmTagErrKNN[nMLMnow]]   = MLMname;

    mlmTagErr[nMLMnow]["N"] = (TString)MLMname+baseTag_e+"N";  mlmBaseTag[mlmTagErr[nMLMnow]["N"]] = MLMname;
    mlmTagErr[nMLMnow][""]  = (TString)MLMname+baseTag_e    ;  mlmBaseTag[mlmTagErr[nMLMnow][""] ] = MLMname;
    mlmTagErr[nMLMnow]["P"] = (TString)MLMname+baseTag_e+"P";  mlmBaseTag[mlmTagErr[nMLMnow]["P"]] = MLMname;
  }

  if(nPDFs > 0) {
    pdfBinNames.resize(nPDFs,vector<TString>(nPDFbins));
    pdfAvgNames.resize(nPDFs);

    for(int nPdfNow=0; nPdfNow<nPDFs; nPdfNow++) {
      for(int nPdfBinNow=0; nPdfBinNow<nPDFbins; nPdfBinNow++) {
        pdfBinNames[nPdfNow][nPdfBinNow] = (TString)glob->GetOptC("baseName_nPDF")+TString::Format("%d_%d",nPdfNow,nPdfBinNow); // format of ANNZ_PDF_0_0
      }

      // average unweighted and weighted pdf values and corresponding errors
      for(int nPdfTypeNow=0; nPdfTypeNow<3; nPdfTypeNow++) {
        TString tagName(""), baseName("");
        if     (nPdfTypeNow == 0) { tagName = baseTag_MLM_a; baseName = glob->GetOptC("baseName_regMLM_avg"); } // unweighted average
        else if(nPdfTypeNow == 1) { tagName = baseTag_PDF_m; baseName = glob->GetOptC("baseName_regPDF_max"); } // peak of pdf
        else if(nPdfTypeNow == 2) { tagName = baseTag_PDF_a; baseName = glob->GetOptC("baseName_regPDF_avg"); } // full pdf average

        pdfAvgNames[nPdfNow][baseTag_v+tagName] = (TString)baseName+utils->intToStr(nPdfNow);
        pdfAvgNames[nPdfNow][baseTag_e+tagName] = (TString)baseName+utils->intToStr(nPdfNow)+baseTag_e;
        pdfAvgNames[nPdfNow][baseTag_w+tagName] = (TString)baseName+utils->intToStr(nPdfNow)+baseTag_w;
      }
    }
  }

  bestMLMname[baseTag_v]     = (TString)glob->GetOptC("baseName_regBest");
  bestMLMname[baseTag_e]     = (TString)glob->GetOptC("baseName_regBest")+baseTag_e;
  bestMLMname[baseTag_e+"N"] = (TString)glob->GetOptC("baseName_regBest")+baseTag_e+"N";
  bestMLMname[baseTag_e+"P"] = (TString)glob->GetOptC("baseName_regBest")+baseTag_e+"P";
  bestMLMname[baseTag_w]     = (TString)glob->GetOptC("baseName_regBest")+baseTag_w;

  return;
}

// ===========================================================================================================
TString ANNZ::getBaseTagName(TString MLMname) {
// ===========================================================================================================
  VERIFY(LOCATION,(TString)"(MLMname = \""+MLMname+"\") has unsupported format",(mlmBaseTag.find(MLMname) != mlmBaseTag.end()));
  return mlmBaseTag[MLMname];
}
// ===========================================================================================================
TString ANNZ::getTagName(int nMLMnow) {
// ===========================================================================================================
  VERIFY(LOCATION,(TString)"(nMLMnow = "+utils->intToStr(nMLMnow)+") out of range ?!?",(nMLMnow >= 0 && nMLMnow < glob->GetOptI("nMLMs")));
  return mlmTagName[nMLMnow];
}
// ===========================================================================================================
TString ANNZ::getTagError(int nMLMnow, TString errType) {
// ===========================================================================================================
  VERIFY(LOCATION,(TString)"(nMLMnow = "+utils->intToStr(nMLMnow)+") out of range ?!?",(nMLMnow >= 0 && nMLMnow < glob->GetOptI("nMLMs")));
  VERIFY(LOCATION,(TString)"(Unknown error-type requested (\"errType\" = "+errType+")",(mlmTagErr[nMLMnow].find(errType) != mlmTagErr[nMLMnow].end()));
  return mlmTagErr[nMLMnow][errType];
}
// ===========================================================================================================
TString ANNZ::getTagWeight(int nMLMnow) {
// ===========================================================================================================
  VERIFY(LOCATION,(TString)"(nMLMnow = "+utils->intToStr(nMLMnow)+") out of range ?!?",(nMLMnow >= 0 && nMLMnow < glob->GetOptI("nMLMs")));
  return mlmTagWeight[nMLMnow];
}
// ===========================================================================================================
TString ANNZ::getTagBias(int nMLMnow) {
// ===========================================================================================================
  VERIFY(LOCATION,(TString)"(nMLMnow = "+utils->intToStr(nMLMnow)+") out of range ?!?",(nMLMnow >= 0 && nMLMnow < glob->GetOptI("nMLMs")));
  return mlmTagBias[nMLMnow];
}
// ===========================================================================================================
TString ANNZ::getTagClsVal(int nMLMnow) {
// ===========================================================================================================
  VERIFY(LOCATION,(TString)"(nMLMnow = "+utils->intToStr(nMLMnow)+") out of range ?!?",(nMLMnow >= 0 && nMLMnow < glob->GetOptI("nMLMs")));
  return mlmTagClsVal[nMLMnow];
}
// ===========================================================================================================
TString ANNZ::getTagIndex(int nMLMnow) {
// ===========================================================================================================
  VERIFY(LOCATION,(TString)"(nMLMnow = "+utils->intToStr(nMLMnow)+") out of range ?!?",(nMLMnow >= 0 && nMLMnow < glob->GetOptI("nMLMs")));
  return mlmTagIndex[nMLMnow];
}
// ===========================================================================================================
TString ANNZ::getTagInVarErr(int nMLMnow, int nInErrNow) {
// ===========================================================================================================
  VERIFY(LOCATION,(TString)"(nMLMnow = "  +utils->intToStr(nMLMnow)  +") out of range ?!?",(nMLMnow >= 0   && nMLMnow < glob->GetOptI("nMLMs")));
  VERIFY(LOCATION,(TString)"(nInErrNow = "+utils->intToStr(nInErrNow)+") out of range ?!?",(nInErrNow >= 0 && nInErrNow < (int)inErrTag[nMLMnow].size()));

  return inErrTag[nMLMnow][nInErrNow];
}
// ===========================================================================================================
TString ANNZ::getTagPdfBinName(int nPdfNow, int nBinNow) {
// ===========================================================================================================
  VERIFY(LOCATION,(TString)"(nPdfNow = "+utils->intToStr(nPdfNow)+") out of range ?!?",(nPdfNow >= 0 && nPdfNow < glob->GetOptI("nPDFs")));
  VERIFY(LOCATION,(TString)"(nBinNow = "+utils->intToStr(nBinNow)+") out of range ?!?",(nBinNow >= 0 && nBinNow < glob->GetOptI("nPDFbins")));

  return pdfBinNames[nPdfNow][nBinNow];
}
// ===========================================================================================================
TString ANNZ::getTagPdfAvgName(int nPdfNow, TString type) {
// ===========================================================================================================
  VERIFY(LOCATION,(TString)"(nPdfNow = "+utils->intToStr(nPdfNow)+") out of range ?!?",(nPdfNow >= 0 && nPdfNow < glob->GetOptI("nPDFs")));
  VERIFY(LOCATION,(TString)"(type = \""+type+"\") has unsupported format",(pdfAvgNames[nPdfNow].find(type) != pdfAvgNames[nPdfNow].end()));

  return pdfAvgNames[nPdfNow][type];
}
// ===========================================================================================================
TString ANNZ::getTagBestMLMname(TString type) {
// ===========================================================================================================
  VERIFY(LOCATION,(TString)"(type = \""+type+"\") has unsupported format",(bestMLMname.find(type) != bestMLMname.end()));

  return bestMLMname[type];
}
// ===========================================================================================================
int ANNZ::getTagNow(TString MLMname) {
// ===========================================================================================================
  TString MLMnamePost(MLMname); MLMnamePost.ReplaceAll(glob->GetOptC("basePrefix"),"");
  VERIFY(LOCATION,(TString)"(MLMname = \""+MLMname+"\") has unsupported format",(MLMnamePost.IsDigit()));

  int nMLMnow = static_cast<int>(MLMnamePost.Atoi());
  VERIFY(LOCATION,(TString)"(nMLMnow = "+utils->intToStr(nMLMnow)+") out of range ?!?",(nMLMnow >= 0 && nMLMnow < glob->GetOptI("nMLMs")));

  return nMLMnow;
}
// ===========================================================================================================
TString ANNZ::getErrKNNname(int nMLMnow) {
// ===========================================================================================================
  VERIFY(LOCATION,(TString)"(nMLMnow = "+utils->intToStr(nMLMnow)+") out of range ?!?",(nMLMnow >= 0 && nMLMnow < glob->GetOptI("nMLMs")));
  return mlmTagErrKNN[nMLMnow];
}
// ===========================================================================================================
int ANNZ::getErrKNNtagNow(TString errKNNname) {
// ===========================================================================================================
  TString errKNNnamePost(errKNNname); errKNNnamePost.ReplaceAll(glob->GetOptC("baseTag_errKNN"),"");

  return getTagNow(errKNNnamePost);
}

// ===========================================================================================================
/**
 * @brief          - Conventions for file names and paths for different categories, as a function of MLMname
 * 
 * @param MLMname  - Name of current MLM
 * @param sequence - The category (training directory, KNN error directory etc.)
 * @param key      - The requested element (file-name, directory-name etc.)
 * 
 * @return         - The requested string
 */
// ===========================================================================================================
TString ANNZ::getKeyWord(TString MLMname, TString sequence, TString key) {
// ===========================================================================================================

  // -----------------------------------------------------------------------------------------------------------
  if(sequence == "trainXML") {
    // -----------------------------------------------------------------------------------------------------------
    TString trainDirNameFullNow = glob->GetOptC("trainDirNameFull");
    if(!glob->GetOptB("doTrain")) trainDirNameFullNow += (TString)getBaseTagName(MLMname)+"/";

    TString outFileDirTrain     = trainDirNameFullNow+MLMname+"_weights/";
    TString outXmlFileName      = outFileDirTrain+glob->GetOptC("typeANNZ")+"_"+MLMname+".weights.xml";
    TString configSaveFileName  = outFileDirTrain+"saveTrainOpt_"+MLMname+".txt";
    TString outFileNameTrain    = trainDirNameFullNow+MLMname+".root";

    if     (key == "trainDirNameFullNow") return trainDirNameFullNow;
    else if(key == "outFileDirTrain")     return outFileDirTrain;
    else if(key == "outXmlFileName")      return outXmlFileName;
    else if(key == "configSaveFileName")  return configSaveFileName;
    else if(key == "outFileNameTrain")    return outFileNameTrain;
    else                                  VERIFY(LOCATION,(TString)"Unknown key (\""+key+"\") in ketKeyWord()",false);
  }
  // -----------------------------------------------------------------------------------------------------------
  else if(sequence == "knnErrXML") {
    // -----------------------------------------------------------------------------------------------------------
    TString outFileDirKnnErr  = glob->GetOptC("outDirNameFull")+glob->GetOptC("treeName")+glob->GetOptC("baseName_knnErr")+"_weights"+"_"+MLMname+"/";
    TString outFileNameKnnErr = glob->GetOptC("outDirNameFull")+glob->GetOptC("treeName")+glob->GetOptC("baseName_knnErr")+"_"+MLMname+".root";

    if     (key == "outFileDirKnnErr")   return outFileDirKnnErr;
    else if(key == "outFileNameKnnErr")  return outFileNameKnnErr;
    else                                 VERIFY(LOCATION,(TString)"Unknown key (\""+key+"\") in ketKeyWord()",false);
  }
  // -----------------------------------------------------------------------------------------------------------
  else if(sequence == "treeErrKNN") {
    // -----------------------------------------------------------------------------------------------------------
    TString treeErrKNNname = (TString)glob->GetOptC("treeName")+"_errKNN";

    if     (key == "treeErrKNNname")   return treeErrKNNname;
    // else if(key == "outFileNameKnnErr")  return outFileNameKnnErr;
    else                                 VERIFY(LOCATION,(TString)"Unknown key (\""+key+"\") in ketKeyWord()",false);
  }
  // -----------------------------------------------------------------------------------------------------------
  else if(sequence == "postTrain") {
    // -----------------------------------------------------------------------------------------------------------
    TString postTrainDirNameMLM("");
    if(glob->GetOptB("doTrain")) {
      bool checkName = glob->GetOptC("trainDirNameFull").EndsWith((TString)MLMname+"/");
      VERIFY(LOCATION,(TString)"Found un-supported form - trainDirNameFull = \""+glob->GetOptC("trainDirNameFull")+"\".",checkName);

      postTrainDirNameMLM      = (TString)glob->GetOptC("trainDirNameFull")+glob->GetOptC("postTrainName");
    }
    else postTrainDirNameMLM   = (TString)glob->GetOptC("trainDirNameFull")+MLMname+"/"+glob->GetOptC("postTrainName");

    TString nTriesTag          = (TString)"_nTries";
    TString configSaveFileName = (TString)postTrainDirNameMLM+"savePostTrainOpt.txt";
    TString hisClsPrbFile      = (TString)postTrainDirNameMLM+glob->GetOptC("hisName")+"_ClsPrb.root";
    TString hisClsPrbHis       = (TString)MLMname+"_prb";

    if     (key == "postTrainDirName")   return postTrainDirNameMLM;
    else if(key == "nTriesTag")          return nTriesTag;
    else if(key == "configSaveFileName") return configSaveFileName;
    else if(key == "hisClsPrbFile")      return hisClsPrbFile;
    else if(key == "hisClsPrbHis")       return hisClsPrbHis;
    else                                 VERIFY(LOCATION,(TString)"Unknown key (\""+key+"\") in ketKeyWord()",false);
  }
  // -----------------------------------------------------------------------------------------------------------
  else if(sequence == "optimResults" || sequence == "verifResults") {
    // -----------------------------------------------------------------------------------------------------------
    TString baseName            = (TString)((sequence == "optimResults") ? glob->GetOptC("optimDirNameFull") : glob->GetOptC("verifDirNameFull"));
    TString configSaveFileName  = (TString)baseName+"saveOptimOpt.txt";
    TString rootSaveFileName    = (TString)baseName+"saveOptimObj.root";
    TString biasCorHisTag       = (TString)"biasCorHis_";

    if     (key == "configSaveFileName") return configSaveFileName;
    else if(key == "rootSaveFileName")   return rootSaveFileName;
    else if(key == "biasCorHisTag")      return biasCorHisTag;
    else                                 VERIFY(LOCATION,(TString)"Unknown key (\""+key+"\") in ketKeyWord()",false);
  }
  // -----------------------------------------------------------------------------------------------------------
  else if(sequence == "baseConfig") {
    // -----------------------------------------------------------------------------------------------------------
    TString baseName = "saveOpt.txt";

    if     (key == "current") return glob->GetOptC("outDirNameFull")  +baseName;
    else if(key == "optim")   return glob->GetOptC("optimDirNameFull")+baseName;
    else if(key == "verif")   return glob->GetOptC("verifDirNameFull")+baseName;
    else                      VERIFY(LOCATION,(TString)"Unknown key (\""+key+"\") in ketKeyWord()",false);
  }
  
  VERIFY(LOCATION,(TString)"Unknown sequence (\""+sequence+"\") in ketKeyWord()",false);
  return "";
}

// ===========================================================================================================
/**
 * @brief          - get the weight expression from userWgtsM using either a VarMaps or a TChain
 * 
 * @param strIn   - the input string to be regularized
 * @param var     - The associated chain (needed for string variable identification)
 * @param aChain  - The associated chain (needed for string variable identification)
 * 
 * @return         - The requested string
 */
// ===========================================================================================================
TString ANNZ::getRegularStrForm(TString strIn, VarMaps * var, TChain * aChain) {
// ===========================================================================================================
  if(strIn == "") return strIn;

  bool hasVar   = (dynamic_cast<VarMaps*>(var)   != NULL);
  bool hasChain = (dynamic_cast<TChain*>(aChain) != NULL);

  VERIFY(LOCATION,(TString)"Must provide either a VarMaps or a TChain in order to get a safe weight expression",(hasVar || hasChain));

  // define a temporary VarMaps so as to regularize the weight expression
  if(!hasVar) {
    var = new VarMaps(glob,utils,"varRegularStrForm");
    var->connectTreeBranches(aChain);
  }
  
  TString strOut = var->regularizeStringForm(strIn);

  if(!hasVar) DELNULL(var);

  return strOut;
}

// ===========================================================================================================
/**
 * @brief    - Load options for different setups.
 *
 * @details  - For training: setup internal maps for cuts and weights (userCutsM, userWgtsM) based on user
 *           inputs, and fill the nominal parameters for training, using setNominalParams().
 *           - While not in training: Extract the options used from training for each MLM from file, perform
 *           consistencey checks, combine cuts/weights if needed, store signal/background
 *           cuts for binned-classification etc.
 */
// ===========================================================================================================
void ANNZ::loadOptsMLM() {
// ===========================================================================================================
  aLOG(Log::DEBUG_1) <<coutWhiteOnBlack<<coutYellow<<" - starting ANNZ::loadOptsMLM() ... "<<coutDef<<endl;

  int     nMLMs     = glob->GetOptI("nMLMs");
  TString weightKNN = glob->GetOptC("baseName_wgtKNN");

  // general initializations
  inNamesVar.resize(nMLMs); inNamesErr.resize(nMLMs); inVarsScaleFunc.resize(nMLMs);

  for(int nMLMnow=0; nMLMnow<nMLMs; nMLMnow++) {
    TString MLMname = getTagName(nMLMnow);

    userCutsM[        "_sig"]   = ""; userCutsM[        "_bck"]   = "";
    userCutsM[MLMname+"_sig"]   = ""; userCutsM[MLMname+"_bck"]   = "";
    userCutsM[MLMname+"_train"] = ""; userCutsM[MLMname+"_valid"] = "";
    userWgtsM[MLMname+"_train"] = ""; userWgtsM[MLMname+"_valid"] = "";
  }

  // -----------------------------------------------------------------------------------------------------------
  // training
  // -----------------------------------------------------------------------------------------------------------
  if(glob->GetOptB("doTrain")) {
    int     nMLMnow = glob->GetOptI("nMLMnow");
    TString MLMname = getTagName(nMLMnow);

    // cuts/weights:
    // -----------------------------------------------------------------------------------------------------------
    userCutsM[        "_sig"]   = (TCut)glob->GetOptC("userCuts_sig");
    userCutsM[        "_bck"]   = (TCut)glob->GetOptC("userCuts_bck");
    userCutsM[MLMname+"_train"] = (TCut)glob->GetOptC("userCuts_train");
    userCutsM[MLMname+"_valid"] = (TCut)glob->GetOptC("userCuts_valid");
    userWgtsM[MLMname+"_train"] = utils->cleanWeightExpr(glob->GetOptC("userWeights_train"));
    userWgtsM[MLMname+"_valid"] = utils->cleanWeightExpr(glob->GetOptC("userWeights_valid"));

    if(glob->GetOptB("useWgtKNN")) {
      for(int nTrainValidNow=0; nTrainValidNow<2; nTrainValidNow++) {
        TString MLM_trainValid = (TString)MLMname+((nTrainValidNow == 0) ? "_train" : "_valid");

        if(userWgtsM[MLM_trainValid] == "" || userWgtsM[MLM_trainValid] == "1") {
          userWgtsM[MLM_trainValid] = weightKNN;
        }
        else {
          userWgtsM[MLM_trainValid] = (TString)"("+weightKNN+")*("+userWgtsM[MLM_trainValid]+")";
        }
      }
    }

  }
  // -----------------------------------------------------------------------------------------------------------
  // optimization, validation and evaluation
  // -----------------------------------------------------------------------------------------------------------
  else {
    map <TString, map<TString,bool> > modCW;
    // -----------------------------------------------------------------------------------------------------------
    // consistency checks and warnings - cuts/weights
    // -----------------------------------------------------------------------------------------------------------
    for(int nCutWgtNow=0; nCutWgtNow<2; nCutWgtNow++) {
      TString cutWgtName = (TString)((nCutWgtNow == 0) ? "Cuts" : "Weights");

      bool hasNewWgt = (glob->GetOptC((TString)"user"+cutWgtName+"_train") != "");
      if(nCutWgtNow == 1 && hasNewWgt) hasNewWgt = (glob->GetOptC((TString)"user"+cutWgtName+"_train") != "1");
      if(hasNewWgt) {
        aLOG(Log::WARNING) <<coutRed<<(TString)" - found [user"+cutWgtName+"_train = \""<<coutYellow
                           <<glob->GetOptC((TString)"user"+cutWgtName+"_train")<<coutRed
                           <<"\"] which will be ignored during validation (this can only be set during training!) ..."<<coutDef<<endl;

        glob->SetOptC((TString)"user"+cutWgtName+"_train","");
      }

      // get modify_userCuts_valid, modify_userWeights_valid
      TString mod_user_valid   = (TString)"modify_user"+cutWgtName+"_valid";

      modCW[cutWgtName]["ADD"] = ((glob->GetOptC(mod_user_valid)).EqualTo("ADD"      ,TString::kIgnoreCase));
      modCW[cutWgtName]["OVR"] = ((glob->GetOptC(mod_user_valid)).EqualTo("OVERWRITE",TString::kIgnoreCase));
      modCW[cutWgtName]["IGN"] = ((glob->GetOptC(mod_user_valid)).EqualTo("IGNORE"   ,TString::kIgnoreCase));
      
        VERIFY(LOCATION,(TString)"Found unsupported "+mod_user_valid+" (\""+glob->GetOptC(mod_user_valid)+"\") ... "
                        +"Supported are: \"IGNORE\", \"ADD\" or \"OVERWRITE\"."
                        ,(modCW[cutWgtName]["ADD"] || modCW[cutWgtName]["OVR"] || modCW[cutWgtName]["IGN"]));

      hasNewWgt = (glob->GetOptC((TString)"user"+cutWgtName+"_valid") != "");
      if(nCutWgtNow == 1 && hasNewWgt) hasNewWgt = (glob->GetOptC((TString)"user"+cutWgtName+"_valid") != "1");
      if(hasNewWgt) {
        TString message  = (TString)coutGreen+" - found [user"+cutWgtName+"_valid = \""
                           +coutYellow+glob->GetOptC("user"+cutWgtName+"_valid")+coutGreen+"\"] "
                           +"and ["+mod_user_valid+" = \""+coutYellow+glob->GetOptC(mod_user_valid)+coutGreen+"\"]";

        if     (modCW[cutWgtName]["ADD"]) message += (TString)" -> Will combine these with the individual MLM "+cutWgtName+"."+coutDef;
        else if(modCW[cutWgtName]["OVR"]) message += (TString)" -> Will overwrite the individual MLM "         +cutWgtName+"."+coutDef;
        else                              message += (TString)" -> Will ignore user"+cutWgtName+"_valid "                 +"."+coutDef;
        
        aLOG(Log::WARNING) <<message<<endl;
      }
    }

    // -----------------------------------------------------------------------------------------------------------
    // 
    // -----------------------------------------------------------------------------------------------------------
    OptMaps          * optMap = new OptMaps("localOptMap");
    TString          saveName = "";
    vector <TString> optNames;
    saveName = glob->versionTag();  optNames.push_back(saveName); optMap->NewOptC(saveName);
    saveName = "configSave_name";   optNames.push_back(saveName); optMap->NewOptC(saveName);
    saveName = "configSave_type";   optNames.push_back(saveName); optMap->NewOptC(saveName);
    saveName = "inputVariables";    optNames.push_back(saveName); optMap->NewOptC(saveName);
    saveName = "inputVarErrors";    optNames.push_back(saveName); optMap->NewOptC(saveName);
    saveName = "userCuts_train";    optNames.push_back(saveName); optMap->NewOptC(saveName);
    saveName = "userCuts_valid";    optNames.push_back(saveName); optMap->NewOptC(saveName);
    saveName = "userCuts_sig";      optNames.push_back(saveName); optMap->NewOptC(saveName);
    saveName = "userCuts_bck";      optNames.push_back(saveName); optMap->NewOptC(saveName);
    saveName = "userWeights_train"; optNames.push_back(saveName); optMap->NewOptC(saveName);
    saveName = "userWeights_valid"; optNames.push_back(saveName); optMap->NewOptC(saveName);
    saveName = "userWeights_sig";   optNames.push_back(saveName); optMap->NewOptC(saveName);
    saveName = "userWeights_bck";   optNames.push_back(saveName); optMap->NewOptC(saveName);
    saveName = "zTrg";              optNames.push_back(saveName); optMap->NewOptC(saveName);
    saveName = "binCls_nBins";      optNames.push_back(saveName); optMap->NewOptI(saveName);
    saveName = "binCls_clsBins";    optNames.push_back(saveName); optMap->NewOptC(saveName);
    saveName = "binCls_maxBinW";    optNames.push_back(saveName); optMap->NewOptF(saveName);

    if(glob->GetOptB("doRegression") && !glob->GetOptB("doBinnedCls")) {
      saveName = "biasCorMLMwithInp"; optNames.push_back(saveName); optMap->NewOptB(saveName);
    }

    // store general information
    // -----------------------------------------------------------------------------------------------------------
    // for classification
    userCutsM["_sig"]         = (TCut)glob->GetOptC("userCuts_sig");
    userCutsM["_bck"]         = (TCut)glob->GetOptC("userCuts_bck");
    // for all setups
    TCut    userCuts_valid    = (TCut)glob->GetOptC("userCuts_valid");
    TString userWgts_valid    = utils->cleanWeightExpr(glob->GetOptC("userWeights_valid"));

    // 
    // -----------------------------------------------------------------------------------------------------------
    inputVariableV.resize(nMLMs,""); trainTimeM.resize(nMLMs,0);

    bool isFirstMLM(true);
    for(int nMLMnow=0; nMLMnow<nMLMs; nMLMnow++) {
      TString MLMname = getTagName(nMLMnow);  if(mlmSkip[MLMname]) continue;

      if(glob->GetOptB("doBinnedCls")) {
        saveName = MLMname+"_sig"; optNames.push_back(saveName); optMap->NewOptC(saveName);
        saveName = MLMname+"_bck"; optNames.push_back(saveName); optMap->NewOptC(saveName);
      }

      optMap->setDefaultOpts(); // for good measure if looking at debugging messages... not really important
      optMap->SetOptC("configSave_name",MLMname);

      // get the parameters stored during training (beforehand optMap is slmost empty, so expect lots of warning
      // messages in DEBUG mode, as the parameters from the file will override optMap)
      TString configSaveFileName = getKeyWord(MLMname,"trainXML","configSaveFileName");
      utils->optToFromFile(&optNames,optMap,configSaveFileName,"READ","SILENT_KeepFile",(inLOG(Log::DEBUG_3) || (inLOG(Log::DEBUG_2) && nMLMnow<5)));

      VERIFY(LOCATION,(TString)"Found inconsistent settings (configSave_name = \""+optMap->GetOptC("configSave_name")+
                               "\", but expected to be \""+MLMname+"\") from file ("+configSaveFileName+")",
                               (optMap->GetOptC("configSave_name") == MLMname));

      if(utils->getCodeVersionDiff(optMap->GetOptC(glob->versionTag())) != 0) {
        aLOG(Log::WARNING) <<coutWhiteOnBlack<<" - Got \"versionTag\" = "<<coutPurple<<optMap->GetOptC(glob->versionTag())
                           <<coutWhiteOnBlack<<" from "<<coutYellow<<configSaveFileName
                           <<coutWhiteOnBlack<<", while the current running version is "
                           <<coutPurple<<glob->GetOptC(glob->versionTag())<<coutWhiteOnBlack
                           <<" - we can go on, but please consider generating this directory from scratch ..." <<coutDef<<endl;
      }

      // for binned classification
      if(glob->GetOptB("doBinnedCls")) {
        userCutsM[MLMname+"_sig"] = (TCut)optMap->GetOptC(MLMname+"_sig");
        userCutsM[MLMname+"_bck"] = (TCut)optMap->GetOptC(MLMname+"_bck");

        optNames.erase(std::remove(optNames.begin(), optNames.end(), MLMname+"_sig"), optNames.end()); optMap->DelOptC(MLMname+"_sig");
        optNames.erase(std::remove(optNames.begin(), optNames.end(), MLMname+"_bck"), optNames.end()); optMap->DelOptC(MLMname+"_bck");
      }

      if(glob->GetOptB("doRegression") && !glob->GetOptB("doBinnedCls")) {
        hasBiasCorMLMinp[nMLMnow] = optMap->GetOptB("biasCorMLMwithInp");
      }

      // isDynBinCls - avoid direct comparison of the binCls_clsBins string, since different machines
      // may have slightly different quantile/floating point calculation, and so while the derived bins may
      // be the same, the string may be slightly different
      // -----------------------------------------------------------------------------------------------------------
      bool isDynBinCls = (glob->GetOptC("binCls_clsBins") == "");

      // fill the zBinCls_binE, zBinCls_binC global vectors which contain the bin information
      if(isFirstMLM) {
        binClsStrToV(optMap->GetOptC("binCls_clsBins"));

        // for bonned classification, make sure all the required MLMs are accepted (use zBinCls_binC which is set in binClsStrToV())
        if(glob->GetOptB("doBinnedCls")) {
          int nClsBins = (int)zBinCls_binC.size();
          VERIFY(LOCATION,
            (TString)"The derived number of classification bins ("+utils->intToStr(nClsBins)+") does not match the number "
            +"of accepted MLMs (has nMLMs = "+utils->intToStr(nMLMs)+"). Found \"binCls_nBins\" = "+utils->intToStr(nClsBins)
            +" ... Is this consistent with the value used for training ?!?",
            (nClsBins == nMLMs)
          );
        }
      }

      // consistancy checks
      // -----------------------------------------------------------------------------------------------------------
      if(glob->GetOptB("doRegression")) {
        for(int nCheckNow=0; nCheckNow<100; nCheckNow++) {
          if(nCheckNow > 0 && !glob->GetOptB("doBinnedCls")) continue;

          TString name(""), type("");
          if     (nCheckNow == 0) { type = "C"; name  = "zTrg";                                      }
          else if(nCheckNow == 1) { type = "C"; name  = "userCuts_train";                            }
          else if(nCheckNow == 2) { type = "C"; name  = "userWeights_train";                         }
          else if(nCheckNow == 3) { type = "I"; name  = "binCls_nBins";                              }
          else if(nCheckNow == 3) { type = "F"; name  = "minValZ";                                   }
          else if(nCheckNow == 3) { type = "F"; name  = "maxValZ";                                   }
          else if(nCheckNow == 4) { type = "C"; name  = "binCls_clsBins"; if( isDynBinCls) continue; }
          else if(nCheckNow == 5) { type = "F"; name  = "binCls_maxBinW"; if(!isDynBinCls) continue; }
          else break;

          aLOG(Log::DEBUG_2) <<coutRed<<" - Validating "<<coutPurple<<MLMname<<coutRed<<" option "<<coutYellow<<name<<coutDef<<endl;

          if(type == "C") {
            if(isFirstMLM) { glob->SetOptC(name,optMap->GetOptC(name)); continue; }
            if(glob->GetOptC(name) == optMap->GetOptC(name)) continue;
            name += (TString)" = \""+glob->GetOptC(name)+"\"";
          }
          if(type == "F") {
            if(isFirstMLM) { glob->SetOptF(name,optMap->GetOptF(name)); continue; }
            if(fabs(glob->GetOptF(name) - optMap->GetOptF(name)) < 1e-10) continue;
            
            name += (TString)" = \""+utils->floatToStr(glob->GetOptF(name))+"\"";
          }
          if(type == "I") {
            if(isFirstMLM) { glob->SetOptI(name,optMap->GetOptI(name)); continue; }
            if(glob->GetOptI(name) == optMap->GetOptI(name)) continue;
            
            name += (TString)" = \""+utils->intToStr(glob->GetOptI(name))+"\"";
          }
          if(type == "B") {
            if(isFirstMLM) { glob->SetOptB(name,optMap->GetOptB(name)); continue; }
            if(glob->GetOptB(name) == optMap->GetOptB(name)) continue;
            
            name += (TString)" = \""+utils->boolToStr(glob->GetOptB(name))+"\"";
          }

          VERIFY(LOCATION,(TString)"Found inconsistent settings for ["+name+"] with the value from file ("+configSaveFileName+")",false);
        }        
      }

      // -----------------------------------------------------------------------------------------------------------
      // input parameters
      // -----------------------------------------------------------------------------------------------------------
      inputVariableV[nMLMnow] = optMap->GetOptC("inputVariables");
      trainTimeM    [nMLMnow] = optMap->GetOptF(glob->GetOptC("aTimeName"));
      typeMLM       [nMLMnow] = nameToTypeMLM[optMap->GetOptC("configSave_type")];

      setNominalParams(nMLMnow,optMap->GetOptC("inputVariables"),optMap->GetOptC("inputVarErrors"));

      // -----------------------------------------------------------------------------------------------------------
      // cuts/weights:
      // -----------------------------------------------------------------------------------------------------------
      // check if need to add the original validation cuts/weights to the current cuts/weights or to overwrite the original
      userCutsM[MLMname+"_train"] = (TCut)optMap->GetOptC("userCuts_train");

      if     (modCW["Cuts"]["OVR"]) userCutsM[MLMname+"_valid"] = userCuts_valid;
      else if(modCW["Cuts"]["ADD"]) userCutsM[MLMname+"_valid"] = userCuts_valid+(TCut)optMap->GetOptC("userCuts_valid");
      else                          userCutsM[MLMname+"_valid"] = (TCut)optMap->GetOptC("userCuts_valid");

      // use only the original training weights
      TString wgtFinal_train = utils->cleanWeightExpr(optMap->GetOptC("userWeights_train"));
      if(glob->GetOptB("useWgtKNN")) {
        if((wgtFinal_train == "") || (wgtFinal_train == "1") || (wgtFinal_train == (TString)"("+weightKNN+")")) {
          wgtFinal_train = weightKNN;
        }
      }
      userWgtsM[MLMname+"_train"] = utils->cleanWeightExpr(wgtFinal_train);

      // may change the original validation weights
      TString addWgts = utils->cleanWeightExpr(optMap->GetOptC("userWeights_valid"));
      if(userWgts_valid != "1" && !utils->isSameWeightExpr(userWgts_valid,addWgts)) {
        addWgts = (TString)"("+userWgts_valid+")*("+addWgts+")";
      }

      TString wgtFinal_valid("");
      if     (modCW["Weights"]["OVR"]) wgtFinal_valid = userWgts_valid;
      else if(modCW["Weights"]["ADD"]) wgtFinal_valid = utils->cleanWeightExpr(addWgts);
      else                             wgtFinal_valid = utils->cleanWeightExpr(optMap->GetOptC("userWeights_valid"));

      if(glob->GetOptB("useWgtKNN")) {
        if((wgtFinal_valid == "1") || (wgtFinal_valid == (TString)"("+weightKNN+")")) {
          wgtFinal_valid = weightKNN;
        }
        else if(!(wgtFinal_valid.Contains(weightKNN))) {
          wgtFinal_valid = (TString)"("+weightKNN+")*("+wgtFinal_valid+")";
        }
      }
      userWgtsM[MLMname+"_valid"] = utils->cleanWeightExpr(wgtFinal_valid);

      // verify that the original sig/bck definitions are consistenet with each other and with the current cuts
      // -----------------------------------------------------------------------------------------------------------
      TCut userCuts_sig = (TCut)optMap->GetOptC("userCuts_sig");
      TCut userCuts_bck = (TCut)optMap->GetOptC("userCuts_bck");

      VERIFY(LOCATION,(TString)"Found inconsistent settings (userCuts_sig = \""+(TString)userCutsM["_sig"]+
                               "\", but expected to be \""+(TString)userCuts_sig+"\") from file ("+configSaveFileName+")",
                               (userCutsM["_sig"] == userCuts_sig));

      VERIFY(LOCATION,(TString)"Found inconsistent settings (userCuts_bck = \""+(TString)userCutsM["_bck"]+
                               "\", but expected to be \""+(TString)userCuts_bck+"\") from file ("+configSaveFileName+")",
                               (userCutsM["_bck"]== userCuts_bck));  

      aLOG(Log::DEBUG_2) <<coutBlue<<"Setting options from file: "<<coutRed<<MLMname<<coutBlue<<" ("<<optMap->GetOptC("configSave_type")
                         <<") from "<<utils->getdateDateTimeStr(trainTimeM[nMLMnow])<<coutDef<<endl;
      aLOG(Log::DEBUG_2) <<coutBlue<<" --> cuts_train: "<<coutYellow<<userCutsM[MLMname+"_train"]<<coutBlue
                         <<" cuts_valid: "<<coutYellow<<userCutsM[MLMname+"_valid"]<<coutBlue
                         <<" weights_train: "<<coutYellow<<userWgtsM[MLMname+"_train"]<<coutBlue
                         <<" weights_valid: "<<coutYellow<<userWgtsM[MLMname+"_valid"]<<coutDef<<endl;
      
      isFirstMLM = false;
    }
    
    modCW.clear(); optNames.clear();
    DELNULL(optMap);
  }


  return;
}

// ===========================================================================================================
/**
 * @brief                  - Set nominal parameters in the inNamesVar, inNamesErr vectors.
 * 
 * @param nMLMnow          - Index of current MLM
 * @param inputVariables   - List of input variables used for training, separated by ';', having format
 *                         like: "MAG_U;MAG_G;(MAG_G-MAG_R);(MAG_R-MAG_I);pow(MAG_I-MAG_Z,2)".
 *                         Functional expressions of input parameters from the dataset are allowed.
 * @param inputVarErrors   - Optional list of errors, corresponding in order to the list given in inputVariables.
 *                         This may be set to an empty string, if input errors are not to be used.
 */
// ===========================================================================================================
void ANNZ::setNominalParams(int nMLMnow, TString inputVariables, TString inputVarErrors) {
// ===========================================================================================================
  TString MLMname           = getTagName(nMLMnow);
  TString basePrefix        = glob->GetOptC("basePrefix");
  TString baseName_inVarErr = glob->GetOptC("baseName_inVarErr");

  aLOG(Log::DEBUG_2) <<coutBlue<<" - setNominalParams() - "<<coutRed<<nMLMnow<<coutBlue<<" - inputVariables = "<<coutGreen<<inputVariables
                     <<coutBlue<<" inputVarErrors = "<<coutYellow<<inputVarErrors<<coutDef<<endl;

  inNamesVar[nMLMnow].clear(); inNamesErr[nMLMnow].clear();

  inputVariables.ReplaceAll(" ",""); inNamesVar[nMLMnow] = utils->splitStringByChar(inputVariables,';');

  int nInVars = (int)inNamesVar[nMLMnow].size();
  VERIFY(LOCATION,(TString)"List of input variable names (\"inputVariables\") is empty !!!",(nInVars > 0));

  for(int nVarNow=0; nVarNow<nInVars; nVarNow++) {
    bool isAllowedName = !(inNamesVar[nMLMnow][nVarNow].BeginsWith(basePrefix));

    VERIFY(LOCATION,(TString)"Variable names in \"inputVariables\" can not begin with \""+basePrefix+"\"",isAllowedName);
  }

  if(inputVarErrors == "") {
    inNamesErr[nMLMnow].resize(nInVars,"");
  }
  else {
    inputVarErrors.ReplaceAll(" ",""); inNamesErr[nMLMnow] = utils->splitStringByChar(inputVarErrors,';');

    VERIFY(LOCATION,(TString)"The size of the input variable names,titles,errors lists are not compatible (titles and errors are "+
                             "each allowed to either be empty or of the same length ... "+"\"inputVariables\" = [ "+inputVariables
                             +" ] "+"\"inputVarErrors\" = [ "+inputVarErrors+" ] ... ",(inNamesVar[nMLMnow].size() == inNamesErr[nMLMnow].size()));

    inErrTag[nMLMnow].resize(nInVars);
    for(int nVarNow=0; nVarNow<nInVars; nVarNow++) {
      inErrTag[nMLMnow][nVarNow] = (TString)baseName_inVarErr+inNamesErr[nMLMnow][nVarNow];
    }
  }

  return;
}

// ===========================================================================================================
/**
 * @brief          - Log-in all nominal cut expressions in a VarMaps()
 * 
 * @param var      - The VarMaps() object to which the cuts are added
 * @param nMLMnow  - Current number of the MLM for which the cuts are defined
 * @param verbose  - Flag to force debugging output
 */
// ===========================================================================================================
void ANNZ::setMethodCuts(VarMaps * var, int nMLMnow, bool verbose) {
// ===========================================================================================================
  TString MLMname = getTagName(nMLMnow);
  VERIFY(LOCATION,(TString)"Memory leak ?! ",(dynamic_cast<VarMaps*>(var)));

  bool debug = inLOG(Log::DEBUG_2) || (inLOG(Log::DEBUG_1) && verbose);
  if(debug) aLOG(Log::DEBUG_1) <<coutYellow<<" - var->printCut("<<coutLightBlue<<MLMname<<coutYellow<<") ... : "<<coutDef<<endl;

  for(int nCutNow=0; nCutNow<100; nCutNow++) {
    TString cutName("");
    if     (nCutNow == 0) cutName = "_comn";  else if(nCutNow == 1) cutName = "_sig";
    else if(nCutNow == 2) cutName = "_bck";   else if(nCutNow == 3) cutName = MLMname+"_train";
    // else if(nCutNow == 4) cutName = "_train"; else if(nCutNow == 5) cutName = "_valid"; // deprecated
    else if(nCutNow == 4) cutName = MLMname+"_valid"; else break;

    var->setTreeCuts(cutName,getTrainTestCuts(cutName,nMLMnow));

    if(debug) { aLOG(Log::DEBUG_1) << ""; var->printCut(cutName,true); }
  }

  return;
}

// ===========================================================================================================
/**
 * @brief          - Get a given cut from the nominal options
 * 
 * @param cutType  - The name-tag of the requested cut
 * @param nMLMnow  - Current number of the MLM for which the cuts are defined
 * @param split0
 * @param split1   - Ratio of objects to accept using a "split" cut,
 *                 e.g., for [split0=2 , split1=3], accept two out of every three objects.
 * @param var      - The associated VarMaps() object
 * @param aChain   - The associated chain
 */
// ===========================================================================================================
TCut ANNZ::getTrainTestCuts(
  TString cutType, int nMLMnow, int split0, int split1, VarMaps * var, TChain * aChain
) {
// ===========================================================================================================
  vector <TString> cutTypeV = utils->splitStringByChar(cutType,';');
  int              nCuts    = (int)cutTypeV.size();
  TString          MLMname  = getTagName(nMLMnow);
  TCut             treeCuts = "";

  VERIFY(LOCATION,(TString)"Trying to use getTrainTestCuts() with no defined cut-type: \""+cutType+"\"",(nCuts > 0 && cutType != ""));

  for(int nCutNow=0; nCutNow<nCuts; nCutNow++) {
    TString cutTypeNow = cutTypeV[nCutNow];

    if(cutTypeNow == "_comn") {
      if(glob->GetOptB("hasTruth") && !glob->GetOptB("doClassification")) {
        if(glob->GetOptB("useCutsMinMaxZ")) {
          treeCuts += (TCut)(glob->GetOptC("zTrg")+TString::Format(" >= %f",glob->GetOptF("minValZ")));
          treeCuts += (TCut)(glob->GetOptC("zTrg")+TString::Format(" <= %f",glob->GetOptF("maxValZ")));
        }
      }
    }
    // deprecated
    // else if(cutTypeNow == "_train" || cutTypeNow == "_valid") {
    //   // if has separate sub-sample for training convergence and for testing -> "testValidType_train" or "testValidType_valid"
    //   if(glob->GetOptB("separateTestValid")) treeCuts += (TCut)glob->GetOptC((TString)"testValidType"+cutTypeNow);
    // }
    else if(cutTypeNow == "_train" || cutTypeNow == "_valid") {
      aLOG(Log::WARNING) <<coutWhiteOnBlack<<" - Somehow got \"cutTypeNow\" = "<<coutPurple<<cutTypeNow<<coutWhiteOnBlack
                         <<" in getTrainTestCuts(). Can go on, but please check where this comes from ..." <<coutDef<<endl;
    }
    else if(cutTypeNow == MLMname+"_train" || cutTypeNow == MLMname+"_valid" || cutTypeNow == "_sig" || cutTypeNow == "_bck") {
      VERIFY(LOCATION,(TString)"Trying to get from userCutsM element [\""+cutTypeNow+"\"] which doesnt exist ..."
                              ,(userCutsM.find(cutTypeNow) != userCutsM.end()));

      treeCuts += (TCut)userCutsM[cutTypeNow];
    }
    else if(cutTypeNow == "split") {
      VERIFY(LOCATION,(TString)"Must have (0 < split0 < split1)",(split1 > split0 && split0 > 0));

      treeCuts += (TCut)((TString)glob->GetOptC("indexName")+" % "+TString::Format("%d < %d",split1,split0));
    }
    else VERIFY(LOCATION,(TString)"Trying to use getTrainTestCuts() with unsupported cut-type (\""+cutTypeNow+"\")",false);
  }

  if(var || aChain) treeCuts = (TCut)getRegularStrForm((TString)treeCuts,var,aChain);

  cutTypeV.clear();
  return treeCuts;
}

// ===========================================================================================================
/**
 * @brief             - Compose a list of user-selected MLMs which will be added to output
 *                    trees and ascii files. These will be in addition to the nominal outputs (such
 *                    as the "best" MLM or the PDFs in case of randomized regression).
 * 
 * @details           - The "MLMsToStore" parameter controls the type of output. Possible options are:
 *                      - "ALL"           - write all trained MLMs
 *                      - "BEST"          - write only the best performing MLM
 *                      - "BEST;x"        - write only the x best performing MLMs (x = integer number)
 *                      - "LIST;0;1;3;55" - specific list of MLM indices (sepaated by ';';) to write out
 *                   
 * @param optimMLMv   - Vector which will be filled with the list of selected MLMs.
 * @param mlmSkipNow  - The results of the selection.
 */
// ===========================================================================================================
void  ANNZ::selectUserMLMlist(vector <TString> & optimMLMv, map <TString,bool> & mlmSkipNow) {
// ===========================================================================================================
  aLOG(Log::INFO) <<coutWhiteOnBlack<<coutPurple<<" - starting ANNZ::selectUserMLMlist() ... "<<coutDef<<endl;

  mlmSkipNow.clear();

  int     nMLMsIn     = (int)optimMLMv.size();
  int     nMLMs       = glob->GetOptI("nMLMs");
  TString MLMsToStore = glob->GetOptC("MLMsToStore");

  TString acceptFormat = (TString)( !glob->GetOptB("doRegression") ? "[\"ALL\"], [\"BEST\"], [\"BEST;2\"] or [\"LIST;0;1;3;55\"]"
                                                                   : "[\"ALL\"] or [\"LIST;0;1;3;55\"]" );
  VERIFY(LOCATION,(TString)"Found empty option MLMsToStore - must specify format such as: "+acceptFormat+" !!!" ,(MLMsToStore != ""));

  // -----------------------------------------------------------------------------------------------------------
  // first reset the rejection list to reject all MLMs
  // -----------------------------------------------------------------------------------------------------------
  for(int nMLMnow=0; nMLMnow<nMLMs; nMLMnow++) {
    TString MLMname = getTagName(nMLMnow); mlmSkipNow[MLMname] = true;
  }
  
  // accept all MLMs
  // -----------------------------------------------------------------------------------------------------------
  if(MLMsToStore == "ALL") {
    mlmSkipNow = mlmSkip;

    aLOG(Log::INFO)<<coutGreen<<" - Will use all MLMs ..."<<coutDef<<endl;
  }  
  // only the first few by order of performance
  // -----------------------------------------------------------------------------------------------------------
  else if(MLMsToStore.BeginsWith("BEST")) {
    int nBestToUse(1);
    if(MLMsToStore != "BEST") {
      TString MLMsToStorePost(MLMsToStore);  MLMsToStorePost.ReplaceAll("BEST;","");
      VERIFY(LOCATION,(TString)"Found unsupported option (MLMsToStore = \""+MLMsToStore+"\") !!!",(MLMsToStore.BeginsWith("BEST;")));
      VERIFY(LOCATION,(TString)"Found unsupported option (MLMsToStore = \""+MLMsToStore+"\") !!!",(MLMsToStorePost.IsDigit()));

      nBestToUse = MLMsToStorePost.Atoi();
    }

    for(int nMLMinNow=0; nMLMinNow<nMLMsIn; nMLMinNow++) {
      TString MLMname = optimMLMv[nMLMinNow]; 
      if(nMLMinNow < nBestToUse) mlmSkipNow[MLMname] = false;
    }
    aLOG(Log::INFO)<<coutGreen<<" - Will use only the first "<<coutBlue<<nBestToUse<<coutGreen<<" MLMs (ranked by performance) ..."<<coutDef<<endl;
  }
  // only the following list of MLMs. Expect format like:   0;2;6;3
  // -----------------------------------------------------------------------------------------------------------
  else if(MLMsToStore.BeginsWith("LIST;")) {
    TString MLMsToStorePost(MLMsToStore);  MLMsToStorePost.ReplaceAll("LIST;","");

    vector <TString> useMLMv  = utils->splitStringByChar(MLMsToStorePost,';');
    int              nUseMLMs = (int)useMLMv.size();

    // only accept the selected MLMs from useMLMv
    int nMLMsInNow(0);
    for(int nMLMinNow=0; nMLMinNow<nUseMLMs; nMLMinNow++) {
      TString useMLMnow = useMLMv[nMLMinNow];

      VERIFY(LOCATION,(TString)"Found unsupported option (MLMsToStore = \""+MLMsToStore+"\") !!!",(useMLMnow.IsDigit()));
      
      int     nMLMnow     = useMLMnow.Atoi();
      TString MLMname     = getTagName(nMLMnow);
      mlmSkipNow[MLMname] = false;
      nMLMsInNow++;

      VERIFY(LOCATION,(TString)"Found missing MLM ("+MLMname+") from list (MLMsToStore = \""+MLMsToStore+"\") !!!",(!mlmSkip[MLMname]));
    }
    VERIFY(LOCATION,(TString)"Found no accepted MLMs, extracted from (MLMsToStore = \""+MLMsToStore+"\") !!!",(nMLMsInNow > 0));

    useMLMv.clear();
  }
  else VERIFY(LOCATION,(TString)"Found unsupported option (MLMsToStore = \""+MLMsToStore+"\") !!!",false);

  return;
}

// ===========================================================================================================
/**
 * @brief    - Set vectors with bin edges and centers used for PDF bin definitions and for plotting.
 * 
 * @details  - Includes equal-width bin definitions for closure histograms, plotting and
 *           equal width PDFs.
 *           - Also includes optional user-defined bin edges, which need-not be of equal-width,
 *           using the "userPdfBins" parameter. The latter should have a format like: "0.05;0.3;0.5;0.6;0.8".
 *           The first and last numbers must be consistent with the definitions of the minimal and
 *           maximal value of the target variable. In addition, bin edges must be in acending order.
 */
// ===========================================================================================================
void ANNZ::setInfoBinsZ() {
// ===========================================================================================================
  TString userPdfBins    = glob->GetOptC("userPdfBins");
  double  minValZ        = glob->GetOptF("minValZ");
  double  maxValZ        = glob->GetOptF("maxValZ");
  double  underflowZ     = glob->GetOptF("underflowZ");
  double  overflowZ      = glob->GetOptF("overflowZ");
  int     nUnderflowBins = glob->GetOptI("nUnderflowBins");
  int     nOverflowBins  = glob->GetOptI("nOverflowBins");
  int     nDrawBins_zTrg = glob->GetOptI("nDrawBins_zTrg");
  double  zClosBinWidth  = glob->GetOptF("zClosBinWidth");
  double  zPlotBinWidth  = glob->GetOptF("zPlotBinWidth");
  bool    doTrain        = glob->GetOptB("doTrain");
  bool    doBinnedCls    = glob->GetOptB("doBinnedCls");
  int     nPDFs          = glob->GetOptI("nPDFs");
  int     nPDFbins       = glob->GetOptI("nPDFbins");
  TString zTrgName       = glob->GetOptC("zTrg");
  double  underflowW     = (nUnderflowBins > 0 ) ? glob->GetOptF("underflowZwidth") / double(nUnderflowBins) : 0;
  double  overflowW      = (nOverflowBins  > 0 ) ? glob->GetOptF("overflowZwidth")  / double(nOverflowBins)  : 0;
  
  TString name("");
  int     nBinsZ(0);
  double  minZ(0), maxZ(0), binW(0);
  bool    hasUserPdfBins(userPdfBins != "");

  for(int nBinTypwNow=0; nBinTypwNow<100; nBinTypwNow++) {
    if(nBinTypwNow == 0) {
      if(zClosBinWidth < EPS) continue;
      
      minZ   = minValZ;
      maxZ   = maxValZ;
      binW   = zClosBinWidth;
      nBinsZ = static_cast<int>(floor(0.1+(maxValZ - minValZ)/binW));
      name   = "closure";
    }
    else if(nBinTypwNow == 1) {
      minZ   = underflowZ;
      maxZ   = overflowZ;
      binW   = zPlotBinWidth;
      nBinsZ = static_cast<int>(floor(0.1+(maxValZ - minValZ)/binW)) + nUnderflowBins+nOverflowBins;
      name   = "plotting";
    }
    else if(nBinTypwNow == 2) {
      if(hasUserPdfBins) continue;

      minZ   = minValZ;
      maxZ   = maxValZ;
      nBinsZ = max(nPDFbins,1);
      binW   = (maxValZ - minValZ) / double(nBinsZ);
      name   = "PDF";
    }
    else if(nBinTypwNow == 3) {
      minZ   = underflowZ;
      maxZ   = overflowZ;
      binW   = (maxValZ - minValZ)/double(nDrawBins_zTrg);
      nBinsZ = nDrawBins_zTrg + nUnderflowBins+nOverflowBins;
      name   = (TString)"plotting "+zTrgName;
    }
    else break;

    aLOG(Log::DEBUG) <<coutWhiteOnBlack<<coutGreen<<" - setInfoBinsZ() - setting "<<coutPurple<<nBinsZ<<coutGreen<<" "<<name
                     <<" bins with width ("<<binW<<") within ["<<minZ<<","<<maxZ<<"]"<<coutDef<<endl;

    vector <double> bins_E(nBinsZ+1,0), bins_C(nBinsZ,0);
    
    for(int nBinZnow=0; nBinZnow<nBinsZ; nBinZnow++) {
      double  binEdgeL(0), binCenter(0);
      if(minZ < minValZ) {
        if(nBinZnow < nUnderflowBins) {
          binEdgeL  = underflowZ + underflowW * nBinZnow;
          binCenter = binEdgeL   + underflowW * 0.5;
        }
        else if(nBinZnow >= nBinsZ - nOverflowBins) {
          binEdgeL  = maxValZ  + overflowW * (nBinZnow - nBinsZ + nOverflowBins);
          binCenter = binEdgeL + overflowW * 0.5;
        }
        else {
          binEdgeL  = minValZ  + binW * (nBinZnow - nUnderflowBins);
          binCenter = binEdgeL + binW * 0.5;
        }
      }
      else {
        binEdgeL  = minValZ  + binW * nBinZnow;
        binCenter = binEdgeL + binW * 0.5;
      }
      
      bins_E[nBinZnow] = binEdgeL;
      bins_C[nBinZnow] = binCenter;
    }
    bins_E[nBinsZ] = maxZ;
  
    if     (nBinTypwNow == 0) { zClos_binE    = bins_E; zClos_binC    = bins_C; }
    else if(nBinTypwNow == 1) { zPlot_binE    = bins_E; zPlot_binC    = bins_C; }
    else if(nBinTypwNow == 2) { zPDF_binE     = bins_E; zPDF_binC     = bins_C; }
    else if(nBinTypwNow == 3) { zTrgPlot_binE = bins_E; zTrgPlot_binC = bins_C; }
  }
  
  // for(int nBinZnow=0; nBinZnow<nBinsZ+1; nBinZnow++) cout <<" - zTrgPlot_binE - "<<nBinZnow<<CT<<zTrgPlot_binE[nBinZnow]<<endl;

  if(hasUserPdfBins) {
    vector <TString> pdfBinV = utils->splitStringByChar(userPdfBins,';');

    nBinsZ   = (int)pdfBinV.size() - 1;
    nPDFbins = nBinsZ;

    glob->SetOptI("nPDFbins",nBinsZ);

    aLOG(Log::DEBUG) <<coutWhiteOnBlack<<coutGreen<<" - setInfoBinsZ() - setting "<<coutPurple<<nBinsZ<<coutGreen
                     <<" closure/PDF bins: "<<coutPurple<<userPdfBins<<coutDef<<endl;

    zPDF_binE .resize(nBinsZ+1,0); zPDF_binC .resize(nBinsZ,0);
    // zClos_binE.resize(nBinsZ+1,0); zClos_binC.resize(nBinsZ,0);

    for(int nPdfBinNow=0; nPdfBinNow<nBinsZ+1; nPdfBinNow++) {
      zPDF_binE [nPdfBinNow] = utils->strToDouble(pdfBinV[nPdfBinNow]);
      // zClos_binE[nPdfBinNow] = zPDF_binE[nPdfBinNow];

      if(nPdfBinNow > 0) {
        VERIFY(LOCATION,(TString)"PDF bins must be given in ascending order (\"userPdfBins\" = "+userPdfBins+")"
                                ,(zPDF_binE[nPdfBinNow] > zPDF_binE[nPdfBinNow-1]));
      }
    }
    for(int nPdfBinNow=0; nPdfBinNow<nBinsZ; nPdfBinNow++) {
      zPDF_binC [nPdfBinNow] = zPDF_binE[nPdfBinNow] + (zPDF_binE[nPdfBinNow+1] - zPDF_binE[nPdfBinNow])/2.;
      // zClos_binC[nPdfBinNow] = zPDF_binC[nPdfBinNow];
    }

    // for(int nBinZnow=0; nBinZnow<nBinsZ+1; nBinZnow++) cout <<" - zClos_binE - "<<nBinZnow<<CT<<zClos_binE[nBinZnow]<<endl;

    VERIFY(LOCATION,(TString)"Found bin edge in [\"userPdfBins\" = "+userPdfBins+"] which is smaller than [\"minValZ\" = "
                            +utils->floatToStr(minValZ)+"]... ",(zPDF_binE[0]      >= minValZ));

    VERIFY(LOCATION,(TString)"Found bin edge in [\"userPdfBins\" = "+userPdfBins+"] which is larger than [\"maxValZ\" = "
                            +utils->floatToStr(maxValZ)+"]... ",(zPDF_binE[nBinsZ] <= maxValZ));
  }

  bool isGoodSetup = (doTrain && doBinnedCls) || (nPDFs == 0) || (nPDFbins > 0);
  VERIFY(LOCATION,(TString)"Must either use userPdfBins, or set the number of PDF bins (\"nPDFbins\" > 0)",isGoodSetup);

  return;
}

// ===========================================================================================================
/**
 * @brief            - Get the bin bumber for a given floating-point value and a corresponding vector of bin-edges.
 *                   
 * @param valZ       - The floatin-point position for which we look for a bin-number.
 * @param binEdgesV  - Vector of bin-edges.
 * @param forceCheck - option to constrain value to be within the predefined range.
 * 
 * @return           - The requested bin-number
 */
// ===========================================================================================================
int ANNZ::getBinZ(double valZ, vector <double> & binEdgesV, bool forceCheck) {
// ===========================================================================================================
  int nBinsZ = (int)binEdgesV.size() - 1;
  
  VERIFY(LOCATION,(TString)"Trying to getBinZ() a vector of size "+utils->intToStr(nBinsZ),(nBinsZ > 0));

  if(valZ < binEdgesV[0]) {
    VERIFY(LOCATION,(TString)"Trying to getBinRegZ() with valZ = "+utils->floatToStr(valZ)
                            +" smaller than low bin-edge ("+utils->floatToStr(binEdgesV[0])+")",!forceCheck);
    return -1;
  }
  if(valZ > binEdgesV[nBinsZ]) {
    VERIFY(LOCATION,(TString)"Trying to getBinRegZ() with valZ = "+utils->floatToStr(valZ)
                            +" larger than high bin-edge ("+utils->floatToStr(binEdgesV[nBinsZ])+")",!forceCheck);
    return -1;
  }
  for(int nBinZnow=0; nBinZnow<nBinsZ; nBinZnow++) {
    if(valZ <= binEdgesV[nBinZnow+1]) return nBinZnow;
  }
  return -1;
}

// ===========================================================================================================
/**
 * @brief          - Extract the classification bin-edges from a string into a vector, used for binned-classification.
 *                    
 * @param clsBins  - The input string, which should have a format like: "0.05;0.1;0.2;0.3;0.4;0.5;0.6;0.8".
 *                 The first and last numbers must be consistent with the definitions of the minimal and
 *                 maximal value of the target variable. In addition, bin edges must be in acending order.
 */
// ===========================================================================================================
void ANNZ::binClsStrToV(TString clsBins) {
// ===========================================================================================================
  if(clsBins == "") return;

  double minValZ = glob->GetOptF("minValZ");
  double maxValZ = glob->GetOptF("maxValZ");

  zBinCls_binE.clear(); zBinCls_binC.clear();

  vector <TString> userBinV  = utils->splitStringByChar(clsBins,';');
  int              nUserBins = (int)userBinV.size() - 1;

  aLOG(Log::INFO) <<coutWhiteOnBlack<<coutGreen<<" - deriveBinClsBins() - setting "<<coutPurple<<nUserBins<<coutGreen
                  <<" binCls bins: "<<coutPurple<<clsBins<<coutDef<<endl;

  zBinCls_binE.resize(nUserBins+1,0); zBinCls_binC.resize(nUserBins,0);

  for(int nUserBinNow=0; nUserBinNow<nUserBins+1; nUserBinNow++) {
    zBinCls_binE[nUserBinNow] = utils->strToDouble(userBinV[nUserBinNow]);

    if(nUserBinNow > 0) {
      VERIFY(LOCATION,(TString)"binCls bins must be given in ascending order ("+clsBins+")"
                              ,(zBinCls_binE[nUserBinNow] > zBinCls_binE[nUserBinNow-1]));
      
      zBinCls_binC[nUserBinNow-1] = zBinCls_binE[nUserBinNow-1] + (zBinCls_binE[nUserBinNow] - zBinCls_binE[nUserBinNow-1])/2.;
    }
  }

  VERIFY(LOCATION,(TString)"Found bin edge in ["+clsBins+"] which is smaller than [\"minValZ\" = "
                          +utils->floatToStr(minValZ)+"]... ",(zBinCls_binE[0]         >= minValZ));

  VERIFY(LOCATION,(TString)"Found bin edge in ["+clsBins+"] which is larger than [\"maxValZ\" = "
                          +utils->floatToStr(maxValZ)+"]... ",(zBinCls_binE[nUserBins] <= maxValZ));

  return;
}

// ===========================================================================================================
/**
 * @brief         - Derive the classification bin-edges for a given setup, used for binned-classification.
 *            
 * @details       - Two options may be used to define the classification bins.
 *                  - If [binCls_maxBinW==0], then set e.g.,:
 *                      args["binCls_nBins"]   = 7,
 *                      args["binCls_clsBins"] = "0.05;0.1;0.2;0.3;0.4;0.5;0.6;0.8",
 *                    to use a specific set of bins. It is required to explicitly set binCls_nBins, the value of the number
 *                    of bins which are given in binCls_clsBins. This is to ensure that the training loop indeed
 *                    produces an MLM for each of the bins in range(binCls_nBins).
 *                    Make sure that the first and last bins in binCls_clsBins are within minValZ and maxValZ.
 *                  - If [binCls_clsBins==""], then set e.g.,:
 *                      args["binCls_nBins"]   = 10,
 *                      args["binCls_maxBinW"] = 0.1,
 *                    to divide the range between minValZ and maxValZ into binCls_nBins bins. The bin-width is
 *                    defined on the fly according, such that each bin has approximately the same number of (weighted) entries,
 *                    while constraining the ben-width to be no larger than binCls_maxBinW.
 *                    It is possible for the automatic bin calculation not to work (especially with sparse distributions),
 *                    so please check the output of the training, and confirm that the bins (and the bin content) are reasonable
 *                  
 * @param chainM  - Map of input chains. These are needed to get the distribution of the target variable (zTrg),
 *                which are used to derive the classification bins.
 * @param cutM    - Map of cuts used in conjunction with chainM to derive the classification bins. Also serve
 *                to store the final signal/background definition of the bin for the current value of nMLMnow.
 * 
 * @return        - A string, containing the final classification bins, with a format such as for "binCls_clsBins".
 */
// ===========================================================================================================
TString ANNZ::deriveBinClsBins(map < TString,TChain* > & chainM, map < TString,TCut > & cutM) {
// ===========================================================================================================
  aLOG(Log::DEBUG) <<coutWhiteOnBlack<<coutYellow<<" - starting ANNZ::deriveBinClsBins() ... "<<coutDef<<endl;

  int     closHisN        = glob->GetOptI("closHisN");
  TString zTrgName        = glob->GetOptC("zTrg");
  TString binCls_clsBins  = glob->GetOptC("binCls_clsBins");
  int     minObjTrainTest = glob->GetOptI("minObjTrainTest");
  int     nMLMnow         = glob->GetOptI("nMLMnow");
  double  minValZ         = glob->GetOptF("minValZ");
  double  maxValZ         = glob->GetOptF("maxValZ");
  double  maxBinW         = glob->GetOptF("binCls_maxBinW");
  int     nBinDivs        = glob->GetOptI("binCls_nBins");
  TString MLMname         = getTagName(nMLMnow);
  TString wgtTrain        = getRegularStrForm(userWgtsM[MLMname+"_train"],NULL,chainM["_train"]);

  Log::LOGtypes binLog(Log::DEBUG_2);


  // fill a histogram with the distribution of zTrgName (from the training chain, after cuts)
  // -----------------------------------------------------------------------------------------------------------
  vector < TH1 *> hisQ_orig(2,NULL);
  for(int nHisNow=0; nHisNow<2; nHisNow++) {
    TString hisQuantName = TString::Format("hisQuant_%d",nHisNow);
    hisQ_orig[nHisNow]   = new TH1F(hisQuantName,hisQuantName,closHisN,minValZ,maxValZ);
    
    TString drawExprs    = (TString)zTrgName+">>+"+hisQuantName;
    TString cutExprs     = (TString)+"("+(TString)((TCut)cutM["_comn"]+(TCut)cutM["_train"])+")";  cutExprs.ReplaceAll("()","");
    if(nHisNow == 1 && wgtTrain != "") {
      cutExprs          += (TString)" * ("+wgtTrain+")";
    }
    // TCanvas * tmpCnvs    = new TCanvas("tmpCnvs","tmpCnvs");
    // int     nEvtPass     = chainM["_train"]->Draw(drawExprs,cutExprs); DELNULL(tmpCnvs);
    int nEvtPass = utils->drawTree(chainM["_train"],drawExprs,cutExprs);

    if(nHisNow == 1) {
      aLOG(Log::INFO)  <<coutYellow<<" - Found "<<coutPurple<<nEvtPass<<coutYellow<<" objects (weighted integral = "<<coutPurple
                       <<hisQ_orig[nHisNow]->Integral()<<coutYellow<<") from "<<coutGreen<<chainM["_train"]->GetName()<<coutDef<<endl;
      aLOG(Log::INFO)  <<coutYellow<<"   using (cuts)*(weights) = "<<coutPurple<<cutExprs<<coutDef<<endl;
    }
    VERIFY(LOCATION,(TString)"Found no objects which passed the cuts ... Something is horribly wrong !?!?",(nEvtPass > 0));
  }


  if(binCls_clsBins != "") {
    binClsStrToV(binCls_clsBins);
  }
  // -----------------------------------------------------------------------------------------------------------
  // divide the range into bins with approximately equal numbers of entries, where bins widths are
  // restiricted to be no larger than approximately the value of binCls_maxBinW
  // (possibly resulting in much fewer entries in those restricted bins...)
  // -----------------------------------------------------------------------------------------------------------
  // the alg:
  //  1. divide entire range [zMin,zMax] into quantiles
  //  2. from low-edge, if any bin is wider than maxBinW, redifine the upper bound by lowEdge+maxBinW and
  //     compute quantiles in the range from lowEdge+maxBinW to zMax
  //     do this until reaching last bin with edge zMax
  //  3. if last bin has width>maxBinW, redifine its lower bound to zMax-maxBinW
  //     then repeat steps 1-3 for [zMin,zMax-maxBinW], that is, excluding the last bin
  //  4. end when all bins have width<maxBinW
  // -----------------------------------------------------------------------------------------------------------
  else {
    TH1 * hisQ_now = (TH1F*)hisQ_orig[1]->Clone((TString)hisQ_orig[1]->GetName()+"_cln");

    VERIFY(LOCATION,(TString)"Must set binCls_nBins >= (maxValZ-minValZ)/binCls_maxBinW ..."
                            ,(nBinDivs >= static_cast<int>(floor(0.1+ (maxValZ-minValZ)/maxBinW ))) );

    vector <double> fracV, quantV, binEdgesTry, binEdges;
    binEdges.push_back(maxValZ);

    // try to derive te desired bin scheme as many times as needed
    // -----------------------------------------------------------------------------------------------------------
    for(int nBinTry=0; nBinTry<nBinDivs; nBinTry++) {
      double minValZ_now  = minValZ;
      int    nBinDivs_now = nBinDivs - nBinTry;

      binEdgesTry.clear(); binEdgesTry.push_back(minValZ_now);

      // -----------------------------------------------------------------------------------------------------------
      // from low to high values, check the width of each bin. if it is higher than maxBinW, set the upper bin edge
      // to (the low value + maxBinW), then recalculate the remianing bins for higher values (quantiles of
      // one less division, to get the same number of bins which spanned this region before the change). Do this for
      // every bin, such that only the las bin (upper edge == maxValZ) can possibly have a width which is (> maxBinW)
      // -----------------------------------------------------------------------------------------------------------
      while(nBinDivs_now > 0) {
        fracV.resize(nBinDivs_now+1);
        for(int nBinDivNow=0; nBinDivNow<nBinDivs_now+1; nBinDivNow++) { fracV[nBinDivNow] = nBinDivNow/double(nBinDivs_now); }
        fracV[nBinDivs_now] -= .00001;

        utils->param->clearAll();
        int hasQuant = utils->getQuantileV(fracV,quantV,hisQ_now);
        VERIFY(LOCATION,(TString)"Could not compute quantiles for his("+(TString)hisQ_now->GetName()+") ... Something is horribly wrong ?!?",(hasQuant == 1));

        if(inLOG(binLog)) {
          aLOG(binLog) <<coutBlue  <<LINE_FILL('-',112)<<coutDef<<endl;
          aLOG(binLog) <<coutYellow<<" - bin-index, fraction, bin-edges, bin-width, #bin-entries (weighted) - "<<coutDef<<endl;
          aLOG(binLog) <<coutBlue  <<LINE_FILL('-',112)<<coutDef<<endl;

          for(int nBinDivNow=0; nBinDivNow<nBinDivs_now; nBinDivNow++) {
            hisQ_orig[1]->GetXaxis()->SetRangeUser(quantV[nBinDivNow],quantV[nBinDivNow+1]);
            aLOG(binLog)   <<std::left<<coutRed<<" - "<<std::setw(6)<<nBinDivNow<<" "<<coutGreen<<std::setw(14)<<fracV[nBinDivNow+1]<<" "
                           <<coutPurple<<std::setw(14)<<quantV[nBinDivNow]<<" "<<std::setw(14)<<quantV[nBinDivNow+1]<<" "
                           <<std::setw(14)<<quantV[nBinDivNow+1]-quantV[nBinDivNow]<<" "<<coutBlue
                           <<std::setw(14)<<hisQ_orig[1]->Integral()<<coutDef<<endl;
          }
          hisQ_orig[1]->GetXaxis()->UnZoom();
        }

        int     nBinsSet(0);
        double  newEdge(minValZ_now);
        for(int nBinDivNow=0; nBinDivNow<nBinDivs_now; nBinDivNow++) {
          nBinsSet++;
          if(nBinDivs_now > nBinsSet && quantV[nBinDivNow+1]-quantV[nBinDivNow] > maxBinW) {
            newEdge = quantV[nBinDivNow]+maxBinW;
            binEdgesTry.push_back(newEdge);
            break;
          }
          binEdgesTry.push_back(quantV[nBinDivNow+1]);
        }
        if(newEdge > minValZ_now) {
          for(int nHisBinNow=1;nHisBinNow<hisQ_now->GetNbinsX()+1; nHisBinNow++) {
            if(hisQ_now->GetXaxis()->GetBinLowEdge(nHisBinNow) > newEdge) break;
            hisQ_now->SetBinContent(nHisBinNow,0);
          }
        }
        nBinDivs_now -= nBinsSet;
        minValZ_now   = newEdge;
      }

      if(inLOG(binLog)) {
        aLOG(binLog) <<coutPurple<<LINE_FILL('-',112)<<coutDef<<endl;
        aLOG(binLog) <<coutPurple<<" - So far ..."<<coutDef<<endl;
        aLOG(binLog) <<coutPurple<<LINE_FILL('-',112)<<coutDef<<endl;

        for(int nBinDivNow=0; nBinDivNow<(int)binEdgesTry.size()-1; nBinDivNow++) {
          hisQ_orig[1]->GetXaxis()->SetRangeUser(binEdgesTry[nBinDivNow],binEdgesTry[nBinDivNow+1]);
          
          aLOG(binLog) <<std::left<<coutGreen<<" - "<<std::setw(6)<<nBinDivNow<<coutPurple<<""<<std::setw(14)<<binEdgesTry[nBinDivNow]<<" "
                       <<std::setw(14)<<binEdgesTry[nBinDivNow+1]<<" "<<coutBlue<<std::setw(14)<<
                       binEdgesTry[nBinDivNow+1]-binEdgesTry[nBinDivNow]<<" "<<coutYellow<<std::setw(14)<<hisQ_orig[1]->Integral()<<coutDef<<endl;
        }
        hisQ_orig[1]->GetXaxis()->UnZoom();
      }

      nBinDivs_now = nBinDivs - nBinTry;

      VERIFY(LOCATION,(TString)"Could not compute dynamic bins for binned-classification... Try setting \"binCls_clsBins\" instead."
                              ,((int)binEdgesTry.size() == nBinDivs_now+1));

      // -----------------------------------------------------------------------------------------------------------
      // check if the last bin is wider than maxBinW. If it is, set it to (its upper bound - maxBinW) and redo
      // the entire calculation from minValZ, excluding this last bin
      // -----------------------------------------------------------------------------------------------------------
      if(binEdgesTry[nBinDivs_now] - binEdgesTry[nBinDivs_now-1] > maxBinW) {
        double newEdge = binEdgesTry[nBinDivs_now] - maxBinW;
        binEdges.push_back(newEdge);

        hisQ_now->Reset();
        for(int nHisBinNow=1;nHisBinNow<hisQ_now->GetNbinsX()+1; nHisBinNow++) {
          hisQ_now->SetBinContent(nHisBinNow,hisQ_orig[1]->GetBinContent(nHisBinNow));
          if(hisQ_now->GetXaxis()->GetBinLowEdge(nHisBinNow) >= newEdge) break;
        }
        
        aLOG(binLog) <<coutRed<<LINE_FILL('-',112)<<coutDef<<endl;
        aLOG(binLog) <<coutRed<<" - Setting new upper edge = "<<coutGreen<<newEdge<<coutRed<<" , as the highest bin"
                                    <<" turned out to be wider than ("<<coutYellow<<maxBinW<<coutRed<<")"<<coutDef<<endl;
      }
      else break;
    }

    // -----------------------------------------------------------------------------------------------------------
    // add to binEdgesTry all the excluded bin edges (bins with width maxBinW at the upper edge, maxValZ).
    // These are ordered in reverse in binEdges
    // -----------------------------------------------------------------------------------------------------------
    for(int nBinDivNow=1; nBinDivNow<(int)binEdges.size(); nBinDivNow++) {
      binEdgesTry.push_back( binEdges[(int)binEdges.size()-1 - nBinDivNow] );
    }

    VERIFY(LOCATION,(TString)"Could not compute dynamic bins for binned-classification... Try setting \"binCls_clsBins\" instead."
                            ,((int)binEdgesTry.size() == nBinDivs+1));

    // -----------------------------------------------------------------------------------------------------------
    // write the derived bin edges/centers to the global vectors, zBinCls_binE and zBinCls_binC
    // -----------------------------------------------------------------------------------------------------------
    zBinCls_binE.clear(); zBinCls_binC.clear();
    
    for(int nBinDivNow=0; nBinDivNow<(int)binEdgesTry.size(); nBinDivNow++) {
      zBinCls_binE.push_back(binEdgesTry[nBinDivNow]);
    }
    for(int nBinDivNow=0; nBinDivNow<(int)binEdgesTry.size()-1; nBinDivNow++) {
      zBinCls_binC.push_back(zBinCls_binE[nBinDivNow] + (zBinCls_binE[nBinDivNow+1] - zBinCls_binE[nBinDivNow])/2.);
    }

    DELNULL(hisQ_now);
    fracV.clear(); quantV.clear(); binEdgesTry.clear();
  }

  // -----------------------------------------------------------------------------------------------------------
  // check that the selected/derived bins each has enough entries for training, and output som info
  // -----------------------------------------------------------------------------------------------------------
  aLOG(Log::INFO) <<coutBlue<<LINE_FILL('-',112)<<coutDef<<endl;
  aLOG(Log::INFO) <<coutCyan<<" - FINAL CLASSIFICATION BIN SCHEME: "<<coutGreen<<"bin-index , "<<coutPurple<<"bin-edges , "
                  <<coutBlue<<" bin-width , "<<coutYellow<<"#bin-entries (unweighted , weighted) "<<coutCyan<<"-"<<coutDef<<endl;
  aLOG(Log::INFO) <<coutBlue<<LINE_FILL('-',112)<<coutDef<<endl;

  for(int nClsBinNow=0; nClsBinNow<(int)zBinCls_binC.size(); nClsBinNow++) {
    hisQ_orig[0]->GetXaxis()->SetRangeUser(zBinCls_binE[nClsBinNow],zBinCls_binE[nClsBinNow+1]);
    hisQ_orig[1]->GetXaxis()->SetRangeUser(zBinCls_binE[nClsBinNow],zBinCls_binE[nClsBinNow+1]);
    
    double  intgr0(hisQ_orig[0]->Integral()), intgr1(hisQ_orig[1]->Integral());
     
    if(intgr0 < minObjTrainTest) {
      aLOG(Log::ERROR) <<coutWhiteOnRed<<"There are not enough objects in [bin = "<<nClsBinNow<<", nObj = "<<intgr0
                       <<"] - minimal (weighted) value is "<<coutYellow<<minObjTrainTest<<coutDef<<endl;
      aLOG(Log::ERROR) <<coutWhiteOnRed<<"Try increasing the value of binCls_maxBinW or decreasing the number of bins, binCls_nBins"<<coutDef<<endl;
    
      VERIFY(LOCATION,(TString)"Not enough objects in the derived classification bins... Something is horribly wrong !?!?",false);
    }

    aLOG(Log::INFO)  <<std::left<<coutGreen<<" - "<<std::setw(6)<<nClsBinNow<<coutPurple<<""<<std::setw(14)<<zBinCls_binE[nClsBinNow]<<" "
                     <<std::setw(14)<<zBinCls_binE[nClsBinNow+1]<<" "<<coutBlue<<std::setw(14)<<
                     zBinCls_binE[nClsBinNow+1]-zBinCls_binE[nClsBinNow]<<" "<<coutYellow
                     <<std::setw(14)<<intgr0<<" "<<std::setw(14)<<intgr1<<coutDef<<endl;
  }
  hisQ_orig[0]->GetXaxis()->UnZoom(); hisQ_orig[1]->GetXaxis()->UnZoom();


  // -----------------------------------------------------------------------------------------------------------
  // compose the final bin-string, and the signal/background definitions for this nMLMnow
  // -----------------------------------------------------------------------------------------------------------
  TString sig  = TString::Format((TString)"("+zTrgName+"  > %f && "+zTrgName+" <= %f)",zBinCls_binE[nMLMnow],zBinCls_binE[nMLMnow+1]);
  TString bck  = TString::Format((TString)"("+zTrgName+" <= %f || "+zTrgName+"  > %f)",zBinCls_binE[nMLMnow],zBinCls_binE[nMLMnow+1]);
 
  if(nMLMnow == 0) { sig.ReplaceAll(" >",">="); bck.ReplaceAll("<="," <"); }

  cutM["_sig"] = userCutsM["_sig"] = sig;
  cutM["_bck"] = userCutsM["_bck"] = bck;

  // compose the final bin choise as a string, for later storing in the run log
  TString clsBins(""), clsBinsPrint("");
  for(int nClsBinNow=0; nClsBinNow<(int)zBinCls_binE.size(); nClsBinNow++) {
    TString binE = utils->doubleToStr(zBinCls_binE[nClsBinNow]);

    clsBins += binE; clsBinsPrint += (TString)coutPurple+binE;
    if(nClsBinNow<(int)zBinCls_binC.size()) {
      clsBins += ";"; clsBinsPrint += (TString)coutGreen+";";
    }
  }

  // write out some info as output
  aLOG(Log::INFO) <<coutGreen <<LINE_FILL('-',112)<<coutDef<<endl;
  aLOG(Log::INFO) <<coutBlue  <<" - will use signal cuts:     "<<coutRed <<cutM["_sig"]<<coutDef<<endl;
  aLOG(Log::INFO) <<coutRed   <<" - will use background cuts: "<<coutBlue<<cutM["_bck"]<<coutDef<<endl;
  aLOG(Log::INFO) <<coutYellow<<" - final bin scheme:         "          <<clsBinsPrint<<coutDef<<endl;
  aLOG(Log::INFO) <<coutGreen <<LINE_FILL('-',112)<<coutDef<<endl;

  // cleanup
  DELNULL(hisQ_orig[0]); DELNULL(hisQ_orig[1]); hisQ_orig.clear();

  return clsBins;
}


// ===========================================================================================================
/**
 * @brief         - Create new trees based on inpt trees and a set of cuts.
 *
 * @param chainM  - Maps of input chains and cuts which contain the initial dataset which is cut, as well
 *                as the new chains created here.
 * @param cutM    - The  cuts used to define the new dataset.
 * @param optMap  - Used to store the final number of objects in the split trees.
 */
// ===========================================================================================================
void ANNZ::createCutTrainTrees(
  map < TString,TChain* > & chainM, map < TString,TCut > & cutM, OptMaps * optMap
) {
// ===========================================================================================================
  aLOG(Log::DEBUG) <<coutWhiteOnBlack<<coutYellow<<" - starting ANNZ::createCutTrainTrees() ... "<<coutDef<<endl;

  int     maxNobj         = glob->GetOptI("maxNobj");
  int     nObjectsToWrite = glob->GetOptI("nObjectsToWrite");
  TString outDirNameFull  = glob->GetOptC("outDirNameFull");

  // -----------------------------------------------------------------------------------------------------------
  // create the new signal/background trees for the train/test samples
  // -----------------------------------------------------------------------------------------------------------
  for(int trainValidType=0; trainValidType<2; trainValidType++) {
    TString trainValidName = (TString)((trainValidType == 0) ? "_train" : "_valid");

    // create a VarMaps and connect the chain which is read-in
    TString varName = (TString)"inputTreeVars_"+chainM[trainValidName]->GetName();
    VarMaps * var_0 = new VarMaps(glob,utils,varName);
    var_0->connectTreeBranches(chainM[trainValidName]); 

    TString varCutNameCmn = "comn";
    var_0->setTreeCuts(varCutNameCmn,cutM["_comn"]+cutM[trainValidName]);

    // create splitIndex variables for signal and background (countinous counters for each sub-sample that can later be used for cuts)
    VarMaps * var_1      = new VarMaps(glob,utils,varName+"_cut");

    vector < pair<TString,TString> > varTypeNameV;
    var_1->varStruct(var_0,NULL,NULL,&varTypeNameV);

    // create an output tree with branches according to var_0
    TString inTreeName = (TString)chainM[trainValidName]->GetName();
    TTree   * cutTree  = new TTree(inTreeName,inTreeName); cutTree->SetDirectory(0); outputs->TreeMap[inTreeName] = cutTree;

    var_1->createTreeBranches(cutTree); 
    var_1->setDefaultVals();

    bool  breakLoop(false), mayWriteObjects(false);
    var_0->clearCntr();
    for(Long64_t loopEntry=0; true; loopEntry++) {
      if(!var_0->getTreeEntry(loopEntry))  breakLoop = true;

      if((mayWriteObjects && var_0->GetCntr("nObj") % nObjectsToWrite == 0) || breakLoop) {
        outputs->WriteOutObjects(false,true); outputs->ResetObjects(); mayWriteObjects = false;
      }
      if(breakLoop) break;

      // skip if failed selection cuts = common for all methods and this train/test sample
      // -----------------------------------------------------------------------------------------------------------
      if(var_0->hasFailedTreeCuts(varCutNameCmn)) continue;

      var_1->copyVarData(var_0,&varTypeNameV);
      var_1->fillTree();

      var_0->IncCntr("nObj"); mayWriteObjects = true;
    }
    var_0->printCntr(inTreeName,Log::DEBUG); outputs->WriteOutObjects(false,true); outputs->ResetObjects();

    // store the final number of signal/background objects
    optMap->NewOptI((TString)"n"+trainValidName+"_cut",var_0->GetCntr("nObj"));

    TString outFileName = outDirNameFull+inTreeName+"*.root";

    aLOG(Log::DEBUG) <<coutRed<<" - switch to new chain("<<var_0->GetCntr("nObj")
                     <<") "<<coutGreen<<inTreeName<<coutRed<<"  from  "<<coutBlue<<outFileName<<coutDef<<endl;

    // signla/background chains for later use from the root trees just created
    chainM[trainValidName+"_cut"] = new TChain(inTreeName,inTreeName);
    chainM[trainValidName+"_cut"]->SetDirectory(0); chainM[trainValidName+"_cut"]->Add(outFileName);

    // cleanup
    DELNULL(var_0); DELNULL(var_1); varTypeNameV.clear();
  }

  int n_train_cut = (maxNobj > 0)?min(maxNobj,optMap->GetOptI("n_train_cut")):optMap->GetOptI("n_train_cut"); optMap->NewOptI("ANNZ_nTrain",n_train_cut);
  int n_valid_cut = (maxNobj > 0)?min(maxNobj,optMap->GetOptI("n_valid_cut")):optMap->GetOptI("n_valid_cut"); optMap->NewOptI("ANNZ_nValid",n_valid_cut);

  aLOG(Log::INFO) <<coutLightBlue<<" - Final available - nTrain,nTest = "<<optMap->GetOptI("n_train_cut")<<","<<optMap->GetOptI("n_valid_cut")<<coutDef<<endl;

  return;
}

// ===========================================================================================================
/**
 * @brief         - Create signal and background trees from "_train" and "_valid" chains, based on a
 *                set of predefined cuts.
 *
 * @param chainM  - Maps of input chains and cuts which contain the initial dataset which is split, as well
 *                as the new chains created here.
 * @param cutM    - The "_train" and "_valid" cuts and the signal/background cuts used to split the dataset.
 * @param optMap  - Used to store the final number of objects in the split trees.
 */
// ===========================================================================================================
void ANNZ::splitToSigBckTrees(
  map < TString,TChain* > & chainM, map < TString,TCut > & cutM, OptMaps * optMap
) {
// ===========================================================================================================
  aLOG(Log::DEBUG) <<coutWhiteOnBlack<<coutYellow<<" - starting ANNZ::splitToSigBckTrees() ... "<<coutDef<<endl;

  int     maxNobj         = glob->GetOptI("maxNobj");
  // TString splitName       = glob->GetOptC("splitName");  // deprecated
  TString outDirNameFull  = glob->GetOptC("outDirNameFull");
  int     nObjectsToWrite = glob->GetOptI("nObjectsToWrite");

  // -----------------------------------------------------------------------------------------------------------
  // create the new signal/background trees for the train/test samples
  // -----------------------------------------------------------------------------------------------------------
  for(int trainValidType=0; trainValidType<2; trainValidType++) {
    TString trainValidName  = (TString)((trainValidType == 0) ? "_train" : "_valid");

    // create a VarMaps and connect the chain which is read-in
    TString inTreeName = chainM[trainValidName]->GetName();
    TString varName    = (TString)"inputTreeVars_"+inTreeName;
    VarMaps * var      = new VarMaps(glob,utils,varName);
    var->connectTreeBranches(chainM[trainValidName]); 

    TString varCutNameCmn = "comn";  var->setTreeCuts(varCutNameCmn,cutM["_comn"]+cutM[trainValidName]);
    TString varCutNameSig = "_sig";  var->setTreeCuts(varCutNameSig,cutM["_sig"]                      );
    TString varCutNameBck = "_bck";  var->setTreeCuts(varCutNameBck,cutM["_bck"]                      );

    TString inTreeNameSig = (TString)inTreeName+"_sig";
    TString inTreeNameBck = (TString)inTreeName+"_bck";

    // create splitIndex variables for signal and background (countinous counters for each sub-sample that can later be used for cuts)
    vector < pair<TString,TString> > varTypeNameV;

    VarMaps * varSig = new VarMaps(glob,utils,varName+"_sig");  varSig->varStruct(var,NULL,NULL,&varTypeNameV);
    VarMaps * varBck = new VarMaps(glob,utils,varName+"_bck");  varBck->varStruct(var);

    // varSig->NewVarI(splitName+"_sigBck"); varBck->NewVarI(splitName+"_sigBck"); // deprecated

    // create an output tree with branches according to var
    TTree * mergedTreeSig = new TTree(inTreeNameSig,inTreeNameSig); mergedTreeSig->SetDirectory(0);  outputs->TreeMap[inTreeNameSig] = mergedTreeSig;
    TTree * mergedTreeBck = new TTree(inTreeNameBck,inTreeNameBck); mergedTreeBck->SetDirectory(0);  outputs->TreeMap[inTreeNameBck] = mergedTreeBck;

    varSig->createTreeBranches(mergedTreeSig); 
    varBck->createTreeBranches(mergedTreeBck);

    TString nObjNameSig((TString)"n"+trainValidName+"_sig"), nObjNameBck((TString)"n"+trainValidName+"_bck");

    bool  breakLoop(false), mayWriteObjects(false);
    var->clearCntr();
    var->NewCntr(nObjNameSig,0); var->NewCntr(nObjNameBck,0);
    for(Long64_t loopEntry=0; true; loopEntry++) {
      if(!var->getTreeEntry(loopEntry))  breakLoop = true;

      if((mayWriteObjects && var->GetCntr("nObj") % nObjectsToWrite == 0) || breakLoop) {
        outputs->WriteOutObjects(false,true); outputs->ResetObjects(); mayWriteObjects = false;
      }
      if(breakLoop) break;

      // skip if failed selection cuts = common for all methods and this train/test sample
      // -----------------------------------------------------------------------------------------------------------
      if(var->hasFailedTreeCuts(varCutNameCmn)) continue;

      // set to default before anything else
      varSig->setDefaultVals(&varTypeNameV); varBck->setDefaultVals(&varTypeNameV);

      if(!var->hasFailedTreeCuts(varCutNameSig)) {
        varSig->copyVarData(var,&varTypeNameV);
        // varSig->SetVarI(splitName+"_sigBck",var->GetCntr(nObjNameSig));  // deprecated
        var->IncCntr(nObjNameSig);

        varSig->fillTree();
      }
      else var->IncCntr((TString)"failedCut: "+var->getFailedCutType());

      if(!var->hasFailedTreeCuts(varCutNameBck)) {
        varBck->copyVarData(var,&varTypeNameV);
        // varBck->SetVarI(splitName+"_sigBck",var->GetCntr(nObjNameBck)); // deprecated
        var->IncCntr(nObjNameBck);

        varBck->fillTree();
      }
      else var->IncCntr((TString)"failedCut: "+var->getFailedCutType());

      var->IncCntr("nObj"); mayWriteObjects = true;
    }
    var->printCntr(inTreeName,Log::DEBUG); outputs->WriteOutObjects(false,true); outputs->ResetObjects();

    // store the final number of signal/background objects
    optMap->NewOptI((TString)"n"+trainValidName+"_sig",var->GetCntr((TString)"n"+trainValidName+"_sig"));
    optMap->NewOptI((TString)"n"+trainValidName+"_bck",var->GetCntr((TString)"n"+trainValidName+"_bck"));

    TString inFileNameSig = (TString)outDirNameFull+"/"+inTreeNameSig+"*.root";
    TString inFileNameBck = (TString)outDirNameFull+"/"+inTreeNameBck+"*.root";
    
    // optMap->NewOptC("trainTreeNameSig"+trainValidName,inFileNameSig);
    // optMap->NewOptC("trainTreeNameBck"+trainValidName,inFileNameBck);

    aLOG(Log::DEBUG) <<coutRed<<" - switch to new chain("<<var->GetCntr((TString)"n"+trainValidName+"_sig")
                     <<") "<<coutGreen<<inTreeNameSig<<coutRed<<"  from  "<<coutBlue<<inFileNameSig<<coutDef<<endl;
    aLOG(Log::DEBUG) <<coutRed<<" - switch to new chain("<<var->GetCntr((TString)"n"+trainValidName+"_bck")
                     <<") "<<coutGreen<<inTreeNameBck<<coutRed<<"  from  "<<coutBlue<<inFileNameBck<<coutDef<<endl;

    // signla/background chains for later use from the root trees just created
    chainM[trainValidName+"_sig"] = new TChain(inTreeNameSig,inTreeNameSig); chainM[trainValidName+"_bck"] = new TChain(inTreeNameBck,inTreeNameBck);
    chainM[trainValidName+"_sig"]->SetDirectory(0);                          chainM[trainValidName+"_bck"]->SetDirectory(0);
    chainM[trainValidName+"_sig"]->Add(inFileNameSig);                       chainM[trainValidName+"_bck"]->Add(inFileNameBck);

    // cleanup
    DELNULL(var); DELNULL(varSig); DELNULL(varBck); varTypeNameV.clear();
  }

  int n_train_sig = (maxNobj > 0)?min(maxNobj,optMap->GetOptI("n_train_sig")):optMap->GetOptI("n_train_sig"); optMap->NewOptI("ANNZ_nTrain_sig",n_train_sig);
  int n_train_bck = (maxNobj > 0)?min(maxNobj,optMap->GetOptI("n_train_bck")):optMap->GetOptI("n_train_bck"); optMap->NewOptI("ANNZ_nTrain_bck",n_train_bck);
  int n_valid_sig = (maxNobj > 0)?min(maxNobj,optMap->GetOptI("n_valid_sig")):optMap->GetOptI("n_valid_sig"); optMap->NewOptI("ANNZ_nValid_sig",n_valid_sig);
  int n_valid_bck = (maxNobj > 0)?min(maxNobj,optMap->GetOptI("n_valid_bck")):optMap->GetOptI("n_valid_bck"); optMap->NewOptI("ANNZ_nValid_bck",n_valid_bck);

  aLOG(Log::INFO) <<coutLightBlue<<" - Final available - nTrainSig,nTrainBck,nTestSig,nTestBck    = "<<optMap->GetOptI("n_train_sig")<<","
                  <<optMap->GetOptI("n_train_bck")<<","<<optMap->GetOptI("n_valid_sig")<<","<<optMap->GetOptI("n_valid_bck")<<coutDef<<endl;

  return;
}

