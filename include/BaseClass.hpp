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

#ifndef BaseClass_h
#define BaseClass_h

#include <commonInclude.hpp>
#include <OptMaps.hpp>
#include <Utils.hpp>
#include <VarMaps.hpp>
#include <OutMngr.hpp>

// ===========================================================================================================
class BaseClass {
// ==============

public:
  BaseClass(TString aName = "BaseClass", Utils * aUtils = NULL, OptMaps * aMaps = NULL, OutMngr * anOutMngr = NULL);
  virtual ~BaseClass();

  TString       name;
  Utils         * utils;
  OptMaps       * glob;
  OutMngr * outputs;
  
  TString       CT;
  TString       coutDef, coutRed, coutGreen, coutBlue, coutLightBlue, coutYellow, coutPurple, coutCyan,
                coutUnderLine, coutWhiteOnBlack, coutWhiteOnRed, coutWhiteOnGreen, coutWhiteOnYellow;
};

#endif
