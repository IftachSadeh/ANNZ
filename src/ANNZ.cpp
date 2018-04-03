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

#include "ANNZ.hpp"
#include "ANNZ_utils.cpp"
#include "ANNZ_err.cpp"
#include "ANNZ_onlyKnnErr.cpp"
#include "ANNZ_TMVA.cpp"
#include "ANNZ_train.cpp"
#include "ANNZ_loopRegCls.cpp"
#include "ANNZ_loopCls.cpp"
#include "ANNZ_loopReg.cpp"
#include "ANNZ_regEval.cpp"
#include "ANNZ_clsEval.cpp"

// ===========================================================================================================
ANNZ::ANNZ(TString aName, Utils * aUtils, OptMaps * aMaps, OutMngr * anOutMngr)
     :BaseClass(   aName,         aUtils,           aMaps,       anOutMngr) {
// ===========================================================================================================
  Init();
  return;
}

// ===========================================================================================================
ANNZ::~ANNZ() {
// ===========================================================================================================
  aLOG(Log::DEBUG) <<coutBlue<<" - starting ANNZ::~ANNZ() ... "<<coutDef<<endl;

  mlmTagName.clear();   mlmTagErr.clear();        mlmTagWeight.clear();   mlmTagClsVal.clear();
  mlmTagIndex.clear();  mlmSkip.clear();          pdfBinNames.clear();    pdfAvgNames.clear();
  trainTimeM.clear();   inputVariableV.clear();   inErrTag.clear();       readerInptIndexV.clear();
  zPDF_binE.clear();    zPDF_binC.clear();        zPlot_binE.clear();     zPlot_binC.clear();
  inNamesVar.clear();   inNamesErr.clear();       userWgtsM.clear();      mlmTagErrKNN.clear();
  zClos_binE.clear();   zClos_binC.clear();       zBinCls_binE.clear();   zBinCls_binC.clear();
  typeMLM.clear();      allANNZtypes.clear();     typeToNameMLM.clear();  nameToTypeMLM.clear();
  bestMLMname.clear();  anlysTypes.clear();       readerInptV.clear();    readerBiasInptV.clear();
  mlmBaseTag.clear();   hasBiasCorMLMinp.clear(); zTrgPlot_binE.clear();  zTrgPlot_binC.clear();

  for(int nMLMnow=0; nMLMnow<(int)inVarsScaleFunc.size(); nMLMnow++) {
    for(int nVarNow=0; nVarNow<(int)inVarsScaleFunc[nMLMnow].size(); nVarNow++) DELNULL(inVarsScaleFunc[nMLMnow][nVarNow]);
  }
  inVarsScaleFunc.clear();

  for(int nHisNow=0; nHisNow<(int)regReaders .size(); nHisNow++) DELNULL(regReaders [nHisNow]);
  for(int nHisNow=0; nHisNow<(int)biasReaders.size(); nHisNow++) DELNULL(biasReaders[nHisNow]);
  for(int nHisNow=0; nHisNow<(int)hisClsPrbV .size(); nHisNow++) DELNULL(hisClsPrbV [nHisNow]);
  regReaders.clear(); biasReaders.clear(); hisClsPrbV.clear();

  evalRegErrCleanup();
  DELNULL(aRegEval);

  return;
}
// ===========================================================================================================


// ===========================================================================================================
RegEval::RegEval(TString aName, Utils * aUtils, OptMaps * aMaps, OutMngr * anOutMngr)
        :BaseClass(      aName,         aUtils,           aMaps,       anOutMngr) {
// ===========================================================================================================
  
  outDirName = "";   inTreeName = "";   inFileName = "";
  loopChain  = NULL; inChain    = NULL; selctVarV  = NULL; varKNN = NULL;
  
  seed = glob->GetOptI("initSeedRnd"); if(seed > 0) seed += 11825;
  rnd  = new TRandom(seed);
  
  varWrapper = new VarMaps(glob,utils,"varWrapper");
  
  return;
}

// ===========================================================================================================
RegEval::~RegEval() {
// ==================
  aLOG(Log::DEBUG) <<coutBlue<<" - starting RegEval::~RegEval() ... "<<coutDef<<endl;

  addVarV   .clear(); tagNameV     .clear(); isErrKNNv .clear(); mlmInV.clear();
  isErrINPv .clear(); mlmAvg_val   .clear(); mlmAvg_err.clear();
  mlmAvg_wgt.clear(); pdfBinWgt    .clear(); nClsBinsIn.clear();
  pdfWeightV.clear(); addMLMv      .clear(); allMLMv   .clear();
  mlmSkip.clear();    mlmSkipDivded.clear(); mlmSkipPdf.clear();
  pdfWgtValV.clear(); pdfWgtNumV   .clear(); regErrV   .clear();

  for(int nPDFnow=0; nPDFnow<(int)hisBiasCorV.size(); nPDFnow++) {
    for(int nPDFbinNow=0; nPDFbinNow<(int)hisBiasCorV[nPDFnow].size(); nPDFbinNow++) {
      DELNULL(hisBiasCorV[nPDFnow][nPDFbinNow]);
    }
  }
  hisBiasCorV.clear();

  if(!hasMlmChain && dynamic_cast<TChain*>(loopChain)) {
    vector <TTree*> friendV = utils->getTreeFriends(loopChain);
    for(int nTreeNow=0; nTreeNow<(int)friendV.size(); nTreeNow++) { loopChain->RemoveFriend(friendV[nTreeNow]); }
    for(int nTreeNow=0; nTreeNow<(int)friendV.size(); nTreeNow++) { DELNULL(friendV[nTreeNow]);                 }
    friendV.clear();

    DELNULL(loopChain);
  }

  DELNULL(varWrapper);

  for(int nPDFnow=0; nPDFnow<(int)hisPDF_w.size(); nPDFnow++) {
    DELNULL(hisPDF_w[nPDFnow]);
  }
  hisPDF_w.clear();

  if(!glob->GetOptB("keepEvalTrees") && !hasMlmChain && (inFileName != "")) {
    utils->safeRM(inFileName,inLOG(Log::DEBUG));
  }

  DELNULL(rnd);

  return;
}
// ===========================================================================================================



