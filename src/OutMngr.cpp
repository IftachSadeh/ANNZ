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

#include <TColor.h>
#include <TLegend.h>
#include <TLegendEntry.h>
#include <TPostScript.h>
#include <TGaxis.h>
#include <TPaveText.h>
#include <TFrame.h>
 
#include "OutMngr.hpp"
#include "OutMngr_utils.cpp"
#include "OutMngr_draw.cpp"

// ===========================================================================================================
OutMngr::OutMngr(TString aName, Utils * aUtils, OptMaps * aMaps) {
// ===============================================================
  if(!dynamic_cast<Utils*>(aUtils) || !dynamic_cast<OptMaps*>(aMaps)) {
    cout  <<coutWhiteOnBlack<<coutBlue<<" - Bad initialization of baseClass(aName,aUtils,aMaps = "
          <<aName<<","<<dynamic_cast<Utils*>(aUtils)<<","<<dynamic_cast<OptMaps*>(aMaps)
          <<")"<<coutDef<<endl;
    assert(false);
  }

  VERIFY(LOCATION,(TString)"Can't create OutMngr without a valid OptMaps ...",(dynamic_cast<OptMaps*>(aMaps)));
  VERIFY(LOCATION,(TString)"Can't create OutMngr without a valid Utils ...",(dynamic_cast<Utils*>(aUtils)));

  glob  = aMaps;
  utils = aUtils;
  draw  = new OptMaps("draw");

  // set the local color variables
  CT                = glob->CT;               coutDef           = glob->coutDef;
  coutRed           = glob->coutRed;          coutGreen         = glob->coutGreen;
  coutBlue          = glob->coutBlue;         coutLightBlue     = glob->coutLightBlue;
  coutYellow        = glob->coutYellow;       coutPurple        = glob->coutPurple;
  coutCyan          = glob->coutCyan;         coutUnderLine     = glob->coutUnderLine;
  coutWhiteOnBlack  = glob->coutWhiteOnBlack; coutWhiteOnRed    = glob->coutWhiteOnRed;
  coutWhiteOnGreen  = glob->coutWhiteOnGreen; coutWhiteOnYellow = glob->coutWhiteOnYellow;

	SetMyStyle();
  TH1::SetDefaultSumw2(true); 
	BaseDir = gDirectory->CurrentDirectory();

  return;
}

// ===========================================================================================================
OutMngr::~OutMngr() {
// ==============================
  DELNULL(draw);
  titleV.clear(); nameMap.clear(); titleMap.clear(); fitParMap.clear();
}
