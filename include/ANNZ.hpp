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

#ifndef ANNZ_h
#define ANNZ_h

#include "BaseClass.hpp"
class RegEval;

// ===========================================================================================================
/**
 * @brief  - Machine learning methods for regression and classification problems, producing single-value
 *         estimators and full probability density functions (PDFs), based on ROOT, using the TMVA package.
 */
// ===========================================================================================================
class ANNZ : public BaseClass {
// ===========================================================================================================
  public:  
    ANNZ(TString aName = "ANNZ", Utils * aUtils = NULL, OptMaps * aMaps = NULL, OutMngr * anOutMngr = NULL);
    ~ANNZ();

    enum    ANNZ_readType { REG, PRB, CLS, NUN };

  public:  
    // ===========================================================================================================
    // public functions:
    // ===========================================================================================================
    void    Train();
    void    Optim();
    void    Eval();
    void    KnnErr();

    // -----------------------------------------------------------------------------------------------------------
    // manager and methods for evaluation
    // -----------------------------------------------------------------------------------------------------------    
    RegEval * aRegEval;
    
    void    evalRegSetup();
    void    evalRegLoop();

    void    evalRegErrSetup();
    void    evalRegErrCleanup();
    
    void    evalRegWrapperSetup();
    TString evalRegWrapperLoop();
    void    evalRegWrapperCleanup();

    void    evalClsSetup();
    void    evalClsLoop();
    
    void    evalClsWrapperSetup();
    TString evalClsWrapperLoop();
    void    evalClsWrapperCleanup();
  
    vector < pair<TString,Float_t> > readerInptV;

  private:  
    // ===========================================================================================================
    // private functions:
    // ===========================================================================================================
    // ANNZ_utils.cpp :
    // -----------------------------------------------------------------------------------------------------------
    void     Init();
    void     setTags();
    TString  getBaseTagName(TString MLMname = DefOpts::NullC);
    TString  getTagName(int nMLMnow = -1);
    TString  getTagError(int nMLMnow = -1, TString errType = "");
    TString  getTagWeight(int nMLMnow = -1);
    TString  getTagBias(int nMLMnow = -1);
    TString  getTagClsVal(int nMLMnow = -1);
    TString  getTagIndex(int nMLMnow = -1);
    TString  getTagInVarErr(int nMLMnow = -1, int nInErrNow = -1);
    TString  getTagPdfBinName(int nPdfNow = -1, int nBinNow = -1);
    TString  getTagPdfAvgName(int nPdfNow = -1, TString type = "");
    TString  getTagBestMLMname(TString MLMname = "");
    int      getTagNow(TString MLMname);
    TString  getErrKNNname(int nMLMnow = -1);
    int      getErrKNNtagNow(TString errKNNname);
    TString  getKeyWord(TString MLMname, TString sequence, TString key);
    TString  getRegularStrForm(TString strIn = "", VarMaps * var = NULL, TChain * aChain = NULL);
    void     loadOptsMLM();
    void     setNominalParams(int nMLMnow, TString inputVariables, TString inputVarErrors);
    void     setMethodCuts(VarMaps * var, int nMLMnow, bool verbose = true);
    TCut     getTrainTestCuts(TString cutType = "", int nMLMnow = 0, int split0 = -1, int split1 = -1, VarMaps * var = NULL, TChain * aChain = NULL);
    void     selectUserMLMlist(vector <TString> & optimMLMv, map <TString,bool> & mlmSkipNow);
    void     setInfoBinsZ();
    int      getBinZ(double valZ, vector <double> & binEdgesV, bool forceCheck = false);
    void     binClsStrToV(TString clsBins);
    TString  deriveBinClsBins(map < TString,TChain* > & chainM, map < TString,TCut > & cutM);
    void     createCutTrainTrees(map < TString,TChain* > & chainM, map < TString,TCut > & cutM, OptMaps * optMap);
    void     splitToSigBckTrees(map < TString,TChain* > & chainM, map < TString,TCut > & cutM, OptMaps * optMap);

    // -----------------------------------------------------------------------------------------------------------
    // ANNZ_train.cpp :
    // -----------------------------------------------------------------------------------------------------------
    void     Train_singleCls();
    void     Train_singleReg();
    void     Train_binnedCls();
    void     Train_singleRegBiasCor();
    void     generateOptsMLM(OptMaps * optMap, TString userMLMopts);
    void     verifTarget(TTree * aTree);

    // -----------------------------------------------------------------------------------------------------------
    // ANNZ_TMVA.cpp :
    // -----------------------------------------------------------------------------------------------------------
    void              prepFactory(int nMLMnow = -1, TMVA::Configurable * configIn = NULL, bool isBiasMLM = false);
    void              doFactoryTrain(TMVA::Factory * factory);
    void              clearReaders(Log::LOGtypes logLevel = Log::DEBUG_1);
    void              loadReaders(map <TString,bool> & mlmSkipNow, bool needMcPRB = true);
    double            getReader(VarMaps * var = NULL, ANNZ_readType readType = ANNZ_readType::NUN, bool forceUpdate = false, int nMLMnow = -1);
    void              setupTypesTMVA();
    TMVA::Types::EMVA getTypeMLMbyName(TString typeName);
    bool              verifyXML(TString outXmlFileName = "");

    // -----------------------------------------------------------------------------------------------------------
    // ANNZ_err.cpp :
    // -----------------------------------------------------------------------------------------------------------
    void     createTreeErrKNN(int nMLMnow);
    void     setupKdTreeKNN(TChain * aChainKnn, TFile *& knnErrOutFile, TMVA::Factory *& knnErrFactory,
                            TMVA::Configurable *& knnErrDataLdr, TMVA::kNN::ModulekNN *& knnErrModule,
                            vector <int> & trgIndexV, int nMLMnow, TCut cutsAll, TString wgtAll);
    void     cleanupKdTreeKNN(TFile *& knnErrOutFile, TMVA::Factory *& knnErrFactory,
                              TMVA::Configurable *& knnErrDataLdr, bool verb = false);
    void     getRegClsErrKNN(VarMaps * var, TMVA::kNN::ModulekNN * knnErrModule, vector <int> & trgIndexV,
                             vector <int> & nMLMv, bool isREG, vector < vector <double> > & zErrV);

    double   getRegClsErrINP(VarMaps * var, bool isREG, int nMLMnow, UInt_t * seedP = NULL, vector <double> * zErrV = NULL);
    
    // -----------------------------------------------------------------------------------------------------------
    // ANNZ_onlyKnnErr.cpp :
    // -----------------------------------------------------------------------------------------------------------
    void     onlyKnnErr_createTreeErrKNN();
    void     onlyKnnErr_eval();

    // -----------------------------------------------------------------------------------------------------------
    // ANNZ_loopRegCls.cpp :
    // -----------------------------------------------------------------------------------------------------------
    void     makeTreeRegClsAllMLM();
    void     makeTreeRegClsOneMLM(int nMLMnow = -1);
    double   getSeparation(TH1 * hisSig, TH1 * hisBck);
    void     deriveHisClsPrb(int nMLMnow = -1);
    TChain   * mergeTreeFriends(TChain * aChain = NULL, TChain * aChainFriend = NULL, vector<TString> * chainFriendFileNameV = NULL,
                                vector <TString> * acceptV = NULL, vector <TString> * rejectV = NULL,
                                TCut aCut = "", vector< pair<TString,TString> > * addFormV = NULL);
    void     verifyIndicesMLM(TChain * aChain = NULL);
    
    // -----------------------------------------------------------------------------------------------------------
    // ANNZ_loopReg.cpp :
    // -----------------------------------------------------------------------------------------------------------
    void     optimReg();
    void     fillColosureV(map < int,vector<int> >    & zRegQnt_nANNZ,   map < int,vector<double> > & zRegQnt_bias,
                           map < int,vector<double> > & zRegQnt_sigma68, map < int,vector<double> > & zRegQnt_fracSig68, TChain * aChain = NULL);
    void     getBestANNZ(map < int,vector<int> >    & zRegQnt_nANNZ,   map < int,vector<double> > & zRegQnt_bias,
                         map < int,vector<double> > & zRegQnt_sigma68, map < int,vector<double> > & zRegQnt_fracSig68,
                         vector < int >             & bestMLMsV,       bool                       onlyInclusiveBin = true);
    void     setBinClsPdfBinWeights(vector < vector < pair<int,double> > > & pdfBinWgt, vector <int> & nClsBinsIn);
    void     getBinClsBiasCorPDF(TChain * aChain, vector <TH2*>  & hisPdfBiasCorV);
    void     doEvalReg(TChain * inChain = NULL, TString outDirName = "", vector <TString> * selctVarV = NULL);
    void     doMetricPlots(TChain * inChain = NULL, vector <TString> * addPlotVarV = NULL, TString addOutputVarsIn = "");

    void     getRndMethodBestPDF(TTree * aChain, int bestANNZindex, vector<int>  & zRegQnt_nANNZ, vector<double> & zRegQnt_bias, 
                                 vector<double> & zRegQnt_sigma68, vector<double> & zRegQnt_fracSig68,
                                 vector < vector<double> > & bestWeightsV, vector <TH2*> & hisPdfBiasCorV);
    
    void     getOldStyleRndMethodBestPDF(TTree * aChain, int bestANNZindex, vector<int>  & zRegQnt_nANNZ, vector<double> & zRegQnt_bias, 
                                 vector<double> & zRegQnt_sigma68, vector<double> & zRegQnt_fracSig68,
                                 vector < vector<double> > & bestWeightsV, vector <TH2*> & hisPdfBiasCorV);

    vector < double > clipWeightsPDF(vector < double > & weightsIn, Log::LOGtypes logLevel = Log::INFO);

    // -----------------------------------------------------------------------------------------------------------
    // ANNZ_loopRegCls.cpp :
    // -----------------------------------------------------------------------------------------------------------
    void     optimCls();
    void     doEvalCls();

    // ===========================================================================================================
    // private variables
    // ===========================================================================================================
    vector < bool >                       hasBiasCorMLMinp;
    vector < time_t >                     trainTimeM;
    vector < TH1* >                       hisClsPrbV;
    vector < Float_t >                    readerBiasInptV;
    vector < Double_t >                   zClos_binE, zClos_binC, zPlot_binE, zPlot_binC, zPDF_binE,
                                          zPDF_binC, zBinCls_binE, zBinCls_binC, zTrgPlot_binE, zTrgPlot_binC;
    vector < TString >                    mlmTagName, mlmTagWeight, mlmTagBias, mlmTagClsVal, mlmTagIndex,
                                          mlmTagErrKNN, inputVariableV;

    map    < TString,bool >               mlmSkip;
    map    < TString,TCut >               userCutsM;
    map    < TString,TString >            userWgtsM, bestMLMname, mlmBaseTag;

    vector < vector <int> >               readerInptIndexV;
    vector < vector <TF1*> >              inVarsScaleFunc;
    vector < vector <TString> >           pdfBinNames, inErrTag, inNamesVar, inNamesErr;
    vector < map <TString,TString> >      mlmTagErr, pdfAvgNames;

    vector < TMVA::Reader* >              regReaders, biasReaders;
    vector < TMVA::Types::EAnalysisType > anlysTypes;
    vector < TMVA::Types::EMVA >          typeMLM, allANNZtypes;
    map    < TMVA::Types::EMVA,TString >  typeToNameMLM;
    map    < TString,TMVA::Types::EMVA >  nameToTypeMLM;
};
#endif  // #define ANNZ_h


// ===========================================================================================================
/**
 * @brief  - manager for performing evaluation
 */
// ===========================================================================================================
class RegEval : public BaseClass {
// ===========================================================================================================
  public:  
    RegEval(TString aName = "RegEval", Utils * aUtils = NULL, OptMaps * aMaps = NULL, OutMngr * anOutMngr = NULL);
    ~RegEval();

    TString outDirName, inTreeName, inFileName;
    int     nPdfTypes, bestANNZindex;
    double  minWeight;
    bool    hasErrKNN, hasErrs, hasMlmChain;
    
    UInt_t  seed;
    TRandom * rnd;
    VarMaps * varWrapper;
    TChain  * inChain, * loopChain;

    vector <TH1*>    hisPDF_w;
    vector <int>     nClsBinsIn, addMLMv, allMLMv;
    vector <TString> addVarV, tagNameV, * selctVarV, mlmInV;
    vector <bool>    isErrKNNv, isErrINPv;

    vector < vector<double> >              mlmAvg_val, mlmAvg_err, mlmAvg_wgt, pdfWeightV;
    vector < vector<double> >              pdfWgtValV, pdfWgtNumV, regErrV;
    map    < TString, bool >               mlmSkip, mlmSkipDivded, mlmSkipPdf;
    vector < vector <TH1*> >               hisBiasCorV;
    vector < vector < pair<int,double> > > pdfBinWgt;

    VarMaps                                      * varKNN;
    vector <int>                                 trgIndexV;
    vector <TFile *>                             knnErrOutFile;
    vector <TChain *>                            aChainKnn;
    map    <TString,int>                         allInputCombos;            
    vector <TMVA::Factory *>                     knnErrFactory;
    vector <TMVA::Configurable *>                knnErrDataLdr;
    vector <TMVA::kNN::ModulekNN *>              knnErrModule;
    map    < TMVA::kNN::ModulekNN*,vector<int> > getErrKNN;
};
