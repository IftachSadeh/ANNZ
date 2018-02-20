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

#ifndef Wrapper_h
#define Wrapper_h

#include "commonInclude.hpp"
class OutMngr;
class Utils;
class OptMaps;
class Manager;
class ANNZ;

// ===========================================================================================================
class Wrapper {
// ============
  public:
    Wrapper(std::string nameIn);
    ~Wrapper();
    
    TString name;
    void    Init(int argc, char ** argv);
    char *  Eval(char * evalId, char * nObjsVars, char ** varNames, char ** varVals);
    void    Release(char * evalId);

    Utils    * utils;
    OptMaps  * glob;
    OutMngr  * outputs;
    Manager  * aManager;
    ANNZ     * aANNZ;

    TTree                  * loopTree;
    map <TString,TString*> registry;
};

// ===========================================================================================================
/**
 * @brief  - shared registry for Wrapper instances, which is used for bookkeeping for python calls
 */
// ===========================================================================================================
class WrapperRegistry {
  public:
    static map <TString, Wrapper*> wrapper;
};

#endif
