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

#include <BaseClass.hpp>

// ===========================================================================================================
BaseClass::BaseClass(TString aName, Utils * aUtils, OptMaps * aMaps, OutMngr * anOutMngr) {
// ========================================================================================
  if(!dynamic_cast<Utils*>(aUtils) || !dynamic_cast<OptMaps*>(aMaps) || !dynamic_cast<OutMngr*>(anOutMngr)) {
    cout  <<coutWhiteOnBlack<<coutBlue<<" - Bad initialization of BaseClass(aName,aUtils,aMaps,anOutMngr = "
          <<aName<<","<<dynamic_cast<Utils*>(aUtils)<<","<<dynamic_cast<OptMaps*>(aMaps)<<","<<dynamic_cast<OutMngr*>(anOutMngr)
          <<")"<<coutDef<<endl;
    assert(false);
  }

  name    = aName; 
  glob    = aMaps;
  utils   = aUtils;
  outputs = anOutMngr;
}
BaseClass::~BaseClass() { }
// ===========================================================================================================

