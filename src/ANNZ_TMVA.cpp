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

// ===========================================================================================================
/**
 * @brief          - Setup a TMVA::Factory (add variables and optionally a target for regression).
 *       
 * @param nMLMnow  - The index of the current MLM.
 * @param factory  - pointer to the TMVA::Factory which is set-up.
 */
// ===========================================================================================================
void ANNZ::prepFactory(int nMLMnow, TMVA::Factory * factory) {
// ===========================================================
  // TString MLMname = getTagName(nMLMnow);

  // if(!dynamic_cast<TMVA::Factory*>(factory)) return;
  VERIFY(LOCATION,(TString)"Memory leak ?! ",(dynamic_cast<TMVA::Factory*>(factory)));

  // since all variables are read-in from TTreeFormula, we define them as floats ("F") in the factory
  for(int nVarNow=0; nVarNow<(int)inNamesVar[nMLMnow].size(); nVarNow++) {
    factory->AddVariable(inNamesVar[nMLMnow][nVarNow],inNamesVar[nMLMnow][nVarNow],"",'F');
  }
  // add regression target if needed
  if(glob->GetOptB("doRegression") && !glob->GetOptB("doBinnedCls")) {
    factory->AddTarget(glob->GetOptC("zTrg"), glob->GetOptC("zTrgTitle")); 
  }

  // let TMVA know the name of the XML file
  // (the directory must be re-set each time a new factory is defined, sometime before the training)
  TString MLMname = getTagName(nMLMnow);
  (TMVA::gConfig().GetIONames()).fWeightFileDir = getKeyWord(MLMname,"trainXML","outFileDirTrain");

  return;
}


// ===========================================================================================================
/**
 * @brief          - Train a TMVA::Factory.
 * 
 * @param factory  - pointer to the TMVA::Factory which is trained.
 */
// ===========================================================================================================
void ANNZ::doFactoryTrain(TMVA::Factory * factory) {
// =================================================
  aLOG(Log::INFO) <<coutWhiteOnBlack<<coutPurple<<" - starting ANNZ::doFactoryTrain() - "<<coutYellow<<"This may take a while ..."<<coutDef<<endl;

  VERIFY(LOCATION,(TString)"Memory leak ?! ",(dynamic_cast<TMVA::Factory*>(factory)));

  // Train MVAs using the set of training events
  factory->TrainAllMethods();

  // if(glob->GetOptB("testAndEvalTrainMethods")) {
  //   // Evaluate all MVAs using the set of test events
  //   cout <<coutRed<<"Begin factory->TestAllMethods() ... "    <<coutDef<<endl;  factory->TestAllMethods();
  //   // Evaluate and compare performance of all configured MVAs
  //   cout <<coutRed<<"Begin factory->EvaluateAllMethods() ... "<<coutDef<<endl;  factory->EvaluateAllMethods();    
  // }

  return;
}


// ===========================================================================================================
/**
 * @brief  - Clear all registered TMVA::Reader objects
 */
// ===========================================================================================================
void ANNZ::clearReaders(Log::LOGtypes logLevel) {
// ==============================================
  aLOG(logLevel) <<coutWhiteOnBlack<<coutCyan<<" - starting ANNZ::clearReaders() ..."<<coutDef<<endl;

  for(int nMLMnow=0; nMLMnow<(int)regReaders.size(); nMLMnow++) {
    bool verb = (regReaders[nMLMnow] && inLOG(Log::DEBUG_1));

    DELNULL_(LOCATION,regReaders[nMLMnow],(TString)"regReaders["+utils->intToStr(nMLMnow)+"]",verb);
  }
  regReaders.clear(); readerInptV.clear(); readerInptIndexV.clear();

  return;
}

// ===========================================================================================================
/**
 * @brief             - Load TMVA::Reader objects for all accepted MLMs.
 * 
 * @details           - For every accepted MLM, first register all input-variable names in readerInptV.
 *                    Then create TMVA::Reader objects with the corresponding input-variables for each MLM.
 *                    The content of readerInptV will be updated by a VarMaps object in the loop later one,
 *                    so that the readers may be evaluated.
 *                    
 * @param mlmSkipNow  - Map for determining which MLM is accepted.
 */
// ===========================================================================================================
void ANNZ::loadReaders(map <TString,bool> & mlmSkipNow) {
// ======================================================
  aLOG(Log::DEBUG_1) <<coutWhiteOnBlack<<coutYellow<<" - starting ANNZ::loadReaders() ... "<<coutDef<<endl;
  
  int nMLMs = glob->GetOptI("nMLMs");

  // cleanup containers before initializing new readers
  clearReaders(Log::DEBUG_2);

  readerInptV.clear(); regReaders.resize(nMLMs,NULL); readerInptIndexV.resize(nMLMs);

  // -----------------------------------------------------------------------------------------------------------
  // initialize readerInptV and add all required variables by input variables (formulae) - using
  // a vector here so as to avoid possible bugs due to vaiable address changes which maps are susceptible to.
  // we do this before creating the readers, as some MLMs will share input variables and some will need their
  // own. This way we avoid duplicate variables and avoid wasting time when they are evaluated in the loop later on.
  // -----------------------------------------------------------------------------------------------------------
  for(int nMLMnow=0; nMLMnow<nMLMs; nMLMnow++) {
    TString MLMname = getTagName(nMLMnow);  if(mlmSkipNow[MLMname]) continue;
    
    int nInVar = (int)inNamesVar[nMLMnow].size();
    VERIFY(LOCATION,(TString)"No input variables defined for "+MLMname+" . Something is horribly wrong ?!?",(nInVar > 0));

    for(int nVarNow=0; nVarNow<nInVar; nVarNow++) {
      TString inVarNameNow = inNamesVar[nMLMnow][nVarNow];
      VERIFY(LOCATION,(TString)"Should not have empty input-variable. Something is horribly wrong... ?!?",(inVarNameNow != ""));        

      bool hasVar(false);
      for(int nVarNow=0; nVarNow<(int)readerInptV.size(); nVarNow++) {
        if(readerInptV[nVarNow].first == inVarNameNow) { hasVar = true; break; }
      }
      if(hasVar) continue;

      readerInptV.push_back(pair<TString,Float_t>(inVarNameNow,0));

      aLOG(Log::DEBUG) <<coutPurple<<" -- Adding input variable "<<coutGreen<<readerInptV.size()-1<<coutPurple
                       <<" (from "<<coutGreen<<MLMname<<coutPurple<<") - \""<<coutRed<<inVarNameNow<<coutPurple<<"\""<<coutDef<<endl;
    }
  }

  // -----------------------------------------------------------------------------------------------------------
  // create a reader for each accepted MLM
  // -----------------------------------------------------------------------------------------------------------
  int nReadIn(0);
  for(int nMLMnow=0; nMLMnow<nMLMs; nMLMnow++) {
    TString      MLMname   = getTagName(nMLMnow);    if(mlmSkipNow[MLMname]) continue;
    TString      verb      = "!Color";               if(inLOG(Log::DEBUG_2)) verb += ":!Silent"; else verb += ":Silent";
    regReaders[nMLMnow]    = new TMVA::Reader(verb);    

    int nInVar = (int)inNamesVar[nMLMnow].size();
    readerInptIndexV[nMLMnow].resize(nInVar,0);

    for(int nReaderInputNow=0; nReaderInputNow<nInVar; nReaderInputNow++) {
      int     readerInptIndex = -1;
      TString inVarNameNow    = inNamesVar[nMLMnow][nReaderInputNow];

      // find the position in readerInptV of the current variable and add it
      for(int nVarNow=0; nVarNow<(int)readerInptV.size(); nVarNow++) {
        if(readerInptV[nVarNow].first == inVarNameNow) { readerInptIndex = nVarNow; break; }
      }
      VERIFY(LOCATION,(TString)"Adding reader-var which does not exist in readerInptV... Something is horribly wrong... ?!?",(readerInptIndex >= 0));

      regReaders[nMLMnow]->AddVariable(inVarNameNow,&(readerInptV[readerInptIndex].second));
      readerInptIndexV[nMLMnow][nReaderInputNow] = readerInptIndex;
    }

    // book the reader
    TString outXmlFileName = getKeyWord(MLMname,"trainXML","outXmlFileName");
    cout << coutPurple; regReaders[nMLMnow]->BookMVA(MLMname,outXmlFileName); cout << coutDef;

    bool foundReader = (dynamic_cast<TMVA::MethodBase*>(regReaders[nMLMnow]->FindMVA(MLMname)));

    if(foundReader) {
      TString           methodName = (dynamic_cast<TMVA::MethodBase*>(regReaders[nMLMnow] ->FindMVA(MLMname)))->GetMethodTypeName();
      TMVA::Types::EMVA methodType = (dynamic_cast<TMVA::MethodBase*>(regReaders[nMLMnow] ->FindMVA(MLMname)))->GetMethodType();

      VERIFY(LOCATION,(TString)"Found inconsistent settings (configSave_type = \""+typeToNameMLM[typeMLM[nMLMnow]]
                              +"\" from the settings file, but "+MLMname+" is of type \""+typeToNameMLM[methodType]+"\""
                              ,(typeMLM[nMLMnow] == methodType));

      if(nReadIn  < 5) aLOG(Log::DEBUG) <<coutYellow<<" - Found   "<<methodName<<" Reader("<<coutRed<<nMLMnow<<coutYellow<<") ... "<<coutDef<<endl;
      if(nReadIn == 5) aLOG(Log::DEBUG) <<coutYellow<<" - Suppressing further messages ... "<<coutDef<<endl;
      nReadIn++;
    }
    else {
      mlmSkipNow[MLMname] = true;
      aLOG(Log::WARNING) <<coutYellow<<" - Missing     Reader("<<coutRed<<nMLMnow<<coutYellow<<") skipping MLM ... "<<coutDef<<endl;
    }
  }

  return;
}


// ===========================================================================================================
/**
 * @brief               - Get the output of the TMVA::Reader object.
 *                    
 * @param var           - A VarMaps object which may update the values of the input-variables which are
 *                      linked to the TMVA::Reader object.
 * @param readType      - The type of MLM used (regression, or one of two classification estimators).
 * @param forceUpdate   - A flag, indicating if the content of the input-variables to the TMVA::Reader object
 *                      should be updated from the current content of var.
 * @param nMLMnow       - The index of the current MLM.
 */
// ===========================================================================================================
double ANNZ::getReader(VarMaps * var, ANNZ_readType readType, bool forceUpdate, int nMLMnow) {
// ===========================================================================================
  VERIFY(LOCATION,(TString)"Memory leak ?! ",(dynamic_cast<VarMaps*>(var)));
  VERIFY(LOCATION,(TString)"Memory leak for regReaders[nMLMnow = "+utils->intToStr(nMLMnow)+"] ?! ",(dynamic_cast<TMVA::Reader*>(regReaders[nMLMnow])));
  VERIFY(LOCATION,(TString)"unknown readType (\""+utils->intToStr((int)readType)+"\") ...",(nMLMnow < glob->GetOptI("nMLMs")));

  TString MLMname = getTagName(nMLMnow);
  
  var->updateReaderFormulae(readerInptV,forceUpdate);
  
  if     (readType == ANNZ_readType::REG) return (regReaders[nMLMnow]->EvaluateRegression(MLMname))[0];
  else if(readType == ANNZ_readType::PRB) return max(min(regReaders[nMLMnow]->GetProba(MLMname),1.),0.);
  else if(readType == ANNZ_readType::CLS) return regReaders[nMLMnow]->EvaluateMVA(MLMname);
  else VERIFY(LOCATION,(TString)"un-supported readType (\""+utils->intToStr((int)readType)+"\") ...",false);
}

// ===========================================================================================================
/**
 * @brief  - Setup maps which convert between TMVA::Types and simple string-tags.
 */
// ===========================================================================================================
void  ANNZ::setupTypesTMVA() {
// ===========================
  TString           nameNow("");
  TMVA::Types::EMVA typeNow(TMVA::Types::kVariable);
  
  allANNZtypes.clear();  typeToNameMLM.clear();  nameToTypeMLM.clear();

  for(int nTypeNow=0; nTypeNow<100; nTypeNow++) {
    if     (nTypeNow ==  0) { typeNow = TMVA::Types::kCuts;            nameNow = "CUTS";            }
    else if(nTypeNow ==  1) { typeNow = TMVA::Types::kLikelihood;      nameNow = "Likelihood";      }
    else if(nTypeNow ==  2) { typeNow = TMVA::Types::kPDERS;           nameNow = "PDERS";           }
    else if(nTypeNow ==  3) { typeNow = TMVA::Types::kHMatrix;         nameNow = "HMatrix";         }
    else if(nTypeNow ==  4) { typeNow = TMVA::Types::kFisher;          nameNow = "Fisher";          }
    else if(nTypeNow ==  5) { typeNow = TMVA::Types::kKNN;             nameNow = "KNN";             }
    else if(nTypeNow ==  6) { typeNow = TMVA::Types::kCFMlpANN;        nameNow = "CFMlpANN";        }
    else if(nTypeNow ==  7) { typeNow = TMVA::Types::kTMlpANN;         nameNow = "TMlpANN";         }
    else if(nTypeNow ==  8) { typeNow = TMVA::Types::kBDT;             nameNow = "BDT";             }
    else if(nTypeNow ==  9) { typeNow = TMVA::Types::kDT;              nameNow = "DT";              }
    else if(nTypeNow == 10) { typeNow = TMVA::Types::kRuleFit;         nameNow = "RuleFit";         }
    else if(nTypeNow == 11) { typeNow = TMVA::Types::kSVM;             nameNow = "SVM";             }
    else if(nTypeNow == 12) { typeNow = TMVA::Types::kMLP;             nameNow = "ANN";             }
    else if(nTypeNow == 13) { typeNow = TMVA::Types::kBayesClassifier; nameNow = "BayesClassifier"; }
    else if(nTypeNow == 14) { typeNow = TMVA::Types::kFDA;             nameNow = "FDA";             }
    else if(nTypeNow == 15) { typeNow = TMVA::Types::kBoost;           nameNow = "Boost";           }
    else if(nTypeNow == 16) { typeNow = TMVA::Types::kPDEFoam;         nameNow = "PDEFoam";         }
    else if(nTypeNow == 17) { typeNow = TMVA::Types::kLD;              nameNow = "LD";              }
    else if(nTypeNow == 18) { typeNow = TMVA::Types::kPlugins;         nameNow = "Plugins";         }
    else if(nTypeNow == 19) { typeNow = TMVA::Types::kCategory;        nameNow = "Category";        }
    else if(nTypeNow == 20) { typeNow = TMVA::Types::kMaxMethod;       nameNow = "MaxMethod";       }
    else break;

    allANNZtypes.push_back(typeNow);  typeToNameMLM[typeNow] = nameNow;  nameToTypeMLM[nameNow] = typeNow;
  }

  return;
}

// ===========================================================================================================
/**
 * @brief           - Get the TMVA::Types associated with a given string-tag.
 * 
 * @param typeName  - The current string-tag.
 */
// ===========================================================================================================
TMVA::Types::EMVA  ANNZ::getTypeMLMbyName(TString typeName) {
// ==========================================================
  bool isRealType = (nameToTypeMLM.find(typeName) != nameToTypeMLM .end());
  if(isRealType) return nameToTypeMLM[typeName];

  TString message = (TString)"Trying to run with bad MLM type (\""+typeName+"\"). Acceptable types are: ";
  for(map <TMVA::Types::EMVA,TString>::iterator Itr = typeToNameMLM.begin(); Itr!=typeToNameMLM.end(); ++Itr)
    message += (TString)"\""+Itr->second+"\" ; ";

  VERIFY(LOCATION,(TString)message,false);

  return TMVA::Types::kVariable;
}

// ===========================================================================================================
/**
 * @brief                 - Check if an XML file (the output of a trained TMVA::Factory) esists and is valid.
 * 
 * @param outXmlFileName  - The path to the file which is being checked.
 */
// ===========================================================================================================
bool ANNZ::verifyXML(TString outXmlFileName) {
// ===========================================
  ifstream * testFile = new ifstream(outXmlFileName);
  bool isGoodXML = testFile->good();
  if(isGoodXML) {
    // minimal verification that the XML is good and has a definition of a "Method"
    // see: http://root.cern.ch/root/html/TMVA__Reader.html#TMVA__Reader:GetMethodTypeFromFile
    void  * doc      = TMVA::gTools().xmlengine().ParseFile(outXmlFileName,TMVA::gTools().xmlenginebuffersize());
    void  * rootnode = TMVA::gTools().xmlengine().DocGetRootElement(doc);
    isGoodXML = TMVA::gTools().HasAttr(rootnode, "Method");

    if(!isGoodXML) aLOG(Log::DEBUG_1)<<coutRed<<" ... Found bad XML file - "<<coutCyan<<outXmlFileName<<coutDef<<endl;
  }
  else aLOG(Log::DEBUG_1)<<coutRed<<" ... Did not find the XML file - "<<coutCyan<<outXmlFileName<<coutDef<<endl;

  DELNULL(testFile);
  return isGoodXML;
}

