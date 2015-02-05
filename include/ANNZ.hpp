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

#include <BaseClass.hpp>

#include "TMVA/Tools.h"
#include "TMVA/Config.h"
#include "TMVA/Factory.h"
#include "TMVA/Reader.h"
#include "TMVA/MethodBase.h"
#include "TMVA/PDF.h"

// -----------------------------------------------------------------------------------------------------------
// hack to make all the private elements of MethodKNN accecible
// -----------------------------------------------------------------------------------------------------------
#define private public
#include "TMVA/MethodKNN.h"
#undef  private
// -----------------------------------------------------------------------------------------------------------


// ===========================================================================================================
/**
 * @brief  - Machine learning methods for regression and classification problems, producing single-value
 *         estimators and full probability density functions (PDFs), based on ROOT, using the TMVA package.
 */
// ===========================================================================================================
class ANNZ : public BaseClass {
// ============================

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


private:  
  // ===========================================================================================================
  // private functions:
  // ===========================================================================================================
  // ANNZ_utils.cpp :
  // -----------------------------------------------------------------------------------------------------------
  void     Init();
  void     setTags();
  TString  getTagName(int nMLMnow = -1);
  TString  getTagError(int nMLMnow = -1, TString errType = "");
  TString  getTagWeight(int nMLMnow = -1);
  TString  getTagClsVal(int nMLMnow = -1);
  TString  getTagIndex(int nMLMnow = -1);
  TString  getTagInVarErr(int nMLMnow = -1, int nInErrNow = -1);
  TString  getTagPdfBinName(int nPdfNow = -1, int nBinNow = -1);
  TString  getTagPdfAvgName(int nPdfNow = -1, TString type = "");
  TString  getTagBestMLMname(TString MLMname = "");
  int      getTagNow(TString MLMname);
  TString  getKeyWord(TString MLMname, TString sequence, TString key);
  void     loadOptsMLM();
  void     setNominalParams(int nMLMnow, TString inputVariables, TString inputVarErrors);
  void     setMethodCuts(VarMaps * var, int nMLMnow, bool verbose = true);
  TCut     getTrainTestCuts(TString cutType = "", int nMLMnow = -1, int split0 = 0, int split1 = 0);
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
  void     generateOptsMLM(OptMaps * optMap, TString userMLMopts);
  void     verifTarget(TTree * aTree);

  // -----------------------------------------------------------------------------------------------------------
  // ANNZ_TMVA.cpp :
  // -----------------------------------------------------------------------------------------------------------
  void              prepFactory(int nANNZnow = -1, TMVA::Factory * factory = NULL);
  void              doFactoryTrain(TMVA::Factory * factory);
  void              clearReaders();
  void              loadReaders(map <TString,bool> & mlmSkipNow);
  double            getReader(VarMaps * var = NULL, ANNZ_readType readType = ANNZ_readType::NUN, bool forceUpdate = false, int nMLMnow = -1);
  void              setupTypesTMVA();
  TMVA::Types::EMVA getTypeMLMbyName(TString typeName);
  bool              verifyXML(TString outXmlFileName = "");

  // -----------------------------------------------------------------------------------------------------------
  // ANNZ_err.cpp :
  // -----------------------------------------------------------------------------------------------------------
  void     setupKdTreeKNN(TChain * aChain, TCut cutsAll, int nANNZnow, TFile *& knnErrOutFile, TMVA::Factory *& knnErrFactory,
                          TMVA::kNN::ModulekNN *& knnErrModule, TCut cutsSig = "", TCut cutsBck = "",
                          TString wgtReg = "1", TString wgtSig = "1", TString wgtBck = "1");
  void     cleanupKdTreeKNN(TFile *& knnErrOutFile, TMVA::Factory *& knnErrFactory, bool verb = false);
  double   getRegClsErrKNN(VarMaps * var = NULL, ANNZ_readType readType = ANNZ_readType::NUN,
                           int nMLMnow = -1, TMVA::kNN::ModulekNN * knnErrModule = NULL, vector <double> * zErrV = NULL);
  double   getRegClsErrINP(VarMaps * var = NULL, ANNZ_readType readType = ANNZ_readType::NUN,
                           int nMLMnow = -1, UInt_t * seedP = NULL, vector <double> * zErrV = NULL);
  
  // -----------------------------------------------------------------------------------------------------------
  // ANNZ_loopRegCls.cpp :
  // -----------------------------------------------------------------------------------------------------------
  void     makeTreeRegClsAllMLM();
  void     makeTreeRegClsOneMLM(int nMLMnow = -1);
  double   getSeparation(TH1 * hisSig, TH1 * hisBck);
  TChain   * mergeTreeFriends(TChain * aChain = NULL, TChain * aChainFriend = NULL, vector<TString> * chainFriendFileNameV = NULL,
                              vector <TString> * acceptV = NULL, vector <TString> * rejectV = NULL, TCut aCut = "");
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
  void     getRndMethodBestPDF(TTree * aChain, int bestANNZindex, vector<int>  & zRegQnt_nANNZ, vector<double> & zRegQnt_bias, 
                               vector<double> & zRegQnt_sigma68, vector<double> & zRegQnt_fracSig68, vector < vector<double> > & bestWeightsV);
  void     doEvalReg(TChain * inChain = NULL, TString outDirName = "", vector <TString> * selctVarV = NULL);
  void     doMetricPlots(TChain * inChain = NULL, vector <TString> * selctMLMv = NULL);

  // -----------------------------------------------------------------------------------------------------------
  // ANNZ_loopRegCls.cpp :
  // -----------------------------------------------------------------------------------------------------------
  void     optimCls();
  void     doEvalCls();

  // ===========================================================================================================
  // private variables
  // ===========================================================================================================
  vector < Double_t >                   zClos_binE, zClos_binC, zPlot_binE, zPlot_binC, zPDF_binE, zPDF_binC, zBinCls_binE, zBinCls_binC;
  vector < TString >                    mlmTagName, mlmTagWeight, mlmTagClsVal, mlmTagIndex, inputVariableV;
  vector < map <TString,TString> >      mlmTagErr;
  vector < vector <TString> >           pdfBinNames, inErrTag;
  vector < map < TString,TString> >     pdfAvgNames;
  map    < TString,bool >               mlmSkip;
  vector < vector <TString> >           inNamesVar, inNamesErr;
  map    < TString,TCut >               userCutsM;
  map    < TString,TString >            userWgtsM, bestMLMname;
  vector < time_t >                     trainTimeM;
  vector < pair<TString,Float_t> >      readerInptV;
  vector < vector<int> >                readerInptIndexV;
  vector < TMVA::Reader* >              regReaders;
  vector < TMVA::Types::EMVA >          typeMLM, allANNZtypes;
  map    < TMVA::Types::EMVA,TString >  typeToNameMLM;
  map    < TString,TMVA::Types::EMVA >  nameToTypeMLM;

};
#endif  // #define ANNZ_h
