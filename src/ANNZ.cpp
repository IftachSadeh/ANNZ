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
#include "ANNZ_TMVA.cpp"

#include "ANNZ_train.cpp"

#include "ANNZ_loopRegCls.cpp"
#include "ANNZ_loopCls.cpp"
#include "ANNZ_loopReg.cpp"

// ===========================================================================================================
ANNZ::ANNZ   (TString aName, Utils * aUtils, OptMaps * aMaps, OutMngr * anOutMngr)
       :BaseClass(        aName,         aUtils,           aMaps,       anOutMngr) {
// =================================================================================
  Init();
  return;
}
ANNZ::~ANNZ() {
// ================
  aLOG(Log::DEBUG) <<coutBlue<<" - starting ANNZ::~ANNZ() ... "<<coutDef<<endl;

  mlmTagName.clear();   mlmTagErr.clear();       mlmTagWeight.clear();   mlmTagClsVal.clear();
  mlmTagIndex.clear();  mlmSkip.clear();         pdfBinNames.clear();    pdfAvgNames.clear();
  trainTimeM.clear();   inputVariableV.clear();  inErrTag.clear();       readerInptIndexV.clear();
  zPDF_binE.clear();    zPDF_binC.clear();       zPlot_binE.clear();     zPlot_binC.clear();
  inNamesVar.clear();   inNamesErr.clear();      userWgtsM.clear();      mlmTagErrKNN.clear();
  zClos_binE.clear();   zClos_binC.clear();      zBinCls_binE.clear();   zBinCls_binC.clear();
  typeMLM.clear();      allANNZtypes.clear();    typeToNameMLM.clear();  nameToTypeMLM.clear();
  bestMLMname.clear();

  return;
}
// ===========================================================================================================
