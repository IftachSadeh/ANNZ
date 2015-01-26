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

#ifndef CatFormat_h
#define CatFormat_h

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
 * @brief  - convert input ascii files into root trees
 */
// ===========================================================================================================
class CatFormat : public BaseClass {
// =================================

public:  
  CatFormat(TString aName = "CatFormat", Utils * aUtils = NULL, OptMaps * aMaps = NULL, OutMngr * anOutMngr = NULL);
  ~CatFormat();

  // member functions
  // -----------------------------------------------------------------------------------------------------------
  void    Init();
  void    asciiToSplitTree(TString inAsciiFiles, TString inAsciiVars);
  void    asciiToFullTree(TString inAsciiFiles, TString inAsciiVars, TString treeNamePostfix = "");
  void    asciiToSplitTree_wgtKNN(TString inAsciiFiles, TString inAsciiVars, TString inAsciiFiles_wgtKNN, TString inAsciiVars_wgtKNN);
  void    parseInputVars(VarMaps * var, TString inAsciiVars, vector <TString> & inVarNames, vector <TString> & inVarTypes);
  bool    inputLineToVars(TString line, VarMaps * var, vector <TString> & inVarNames, vector <TString> & inVarTypes);
  void    setSplitVars(VarMaps * var, TRandom * rnd, map <TString,int> & intMap);
  void    addWgtKNNtoTree(TChain * aChainInp = NULL, TChain * aChainRef = NULL, TString outTreeName = "");

};
#endif  // #ifndef CatFormat_h
