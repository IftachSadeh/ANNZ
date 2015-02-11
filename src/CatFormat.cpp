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

#include "CatFormat.hpp"
#include "CatFormat_asciiToTree.cpp"
#include "CatFormat_wgtKNN.cpp"

// ===========================================================================================================
CatFormat::CatFormat(TString aName, Utils * aUtils, OptMaps * aMaps, OutMngr * anOutMngr)
          :BaseClass(        aName,         aUtils,           aMaps,           anOutMngr) {
// ========================================================================================
  Init();
  return;
}
CatFormat::~CatFormat() {
// ======================
  return;
}
// ===========================================================================================================

