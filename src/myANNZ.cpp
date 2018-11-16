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
#include <commonInclude.hpp>
#include "myANNZ.hpp"
#include "Utils.hpp"
#include "OptMaps.hpp"
#include "OutMngr.hpp"
#include "ANNZ.hpp"
#include "CatFormat.hpp"


// ===========================================================================================================
/**
 * @brief       - The main function
 * 
 * @details     - Parse input parameters from the user and run an instance of Manager.
 * 
 * @param argc  - Number of input parameters.
 * @param argv  - Input parameters of format [NAME=VAL], (e.g., [nPDFbins="30"]), with possible types of int, float, bool, string.
 *              The possibe values of NAME are listed in Manager::Manager(), along with the default correponsing values (VAL).
 * 
 * @return      - return [0] if all went well.
 */
// ===========================================================================================================
int main(int argc, char ** argv){
// ===========================================================================================================

  // create the main object. It comes with built-in glob->() options which may be modified by user inputs in the following
  // -----------------------------------------------------------------------------------------------------------
  Manager * aManager = new Manager();

  // initialization, taking-in the user options
  aManager->Init(argc, argv);

  // -----------------------------------------------------------------------------------------------------------
  // The various classes
  // -----------------------------------------------------------------------------------------------------------
  // create catalogue trees
  if     (aManager->glob->GetOptB("doGenInputTrees")) aManager->GenerateInputTrees();
  // inTrainFlag calculation, without the need for MLMs
  else if(aManager->glob->GetOptB("doInTrainFlag"))   aManager->doInTrainFlag();
  // training, validation and evaluation modes
  else if(!aManager->glob->GetOptB("doOnlyKnnErr"))   aManager->DoANNZ();

  // knn-error calculation, without the need for MLMs
  if(aManager->glob->GetOptB("doOnlyKnnErr")) aManager->doOnlyKnnErr();
  
  DELNULL(aManager);
  cout<<endl;

  return 0;
}



// ===========================================================================================================
/**
 * @brief    - Constructor of Manager.
 * @details  - Define all possible user input parameters (and default values) as members of the glob OptMaps.
 */
// ===========================================================================================================
Manager::Manager() {
// ===========================================================================================================

  // -----------------------------------------------------------------------------------------------------------
  // initial setup - all of these glob->() options may may be overwritten by user inputs
  // -----------------------------------------------------------------------------------------------------------
  glob = new OptMaps("global");

  // -----------------------------------------------------------------------------------------------------------
  // analysis types
  // -----------------------------------------------------------------------------------------------------------
  glob->NewOptB("doRegression"    ,false); // either single or randomized regression modes or binned classification mode
  glob->NewOptB("doSingleReg"     ,false); // single     regression     mode
  glob->NewOptB("doRandomReg"     ,false); // randomized regression     mode
  glob->NewOptB("doBinnedCls"     ,false); // binned     classification mode
  glob->NewOptB("doClassification",false); // either single or randomized classification modes
  glob->NewOptB("doSingleCls"     ,false); // single     classification mode
  glob->NewOptB("doRandomCls"     ,false); // randomized classification mode

  // -----------------------------------------------------------------------------------------------------------
  // operational modes
  // -----------------------------------------------------------------------------------------------------------
  glob->NewOptB("doGenInputTrees",false); // create input trees for ANNZ from ascii files
  glob->NewOptB("doTrain"        ,false); // run ANNZ in training mode
  glob->NewOptB("doOptim"        ,false); // run ANNZ in validation mode
  glob->NewOptB("doVerif"        ,false); // run ANNZ in binned classification mode and check that training is complete
  glob->NewOptB("doEval"         ,false); // run ANNZ in evaluation mode
  glob->NewOptB("doOnlyKnnErr"   ,false); // run only the KNN error estimator
  glob->NewOptB("doInTrainFlag"  ,false); // run only the KNN weight flag


  // -----------------------------------------------------------------------------------------------------------
  // variables used by CatFormat
  // -----------------------------------------------------------------------------------------------------------
  glob->NewOptC("inAsciiFiles" ,"");        // list of input files (if no seperate inputs for training,testing,validting)
  glob->NewOptC("inAsciiVars"  ,"");        // list of input variables and variable-types as they appear in inAsciiFiles
  glob->NewOptC("addOutputVars","");        // list of input variables which will be added to the ascii output
  glob->NewOptB("storeOrigFileName",false); // whether to store the name of the original file for each object
  // inpFiles_sig, inpFiles_bck -
  //   optional lists of input files defining if an object is of type signal or background
  glob->NewOptC("inpFiles_sig","");
  glob->NewOptC("inpFiles_bck","");
  // list of input variables for evaluation (used exclusively by the Wrapper)
  glob->NewOptC("inVars"  ,"");

  // nSplit - how to split into training/testing(/validation) - 
  //   nSplit = 2 -> split into 2 (training,testing) sub-sets - for trainig/optimization
  //   nSplit = 1 -> no splitting - for evaluation
  // -----------------------------------------------------------------------------------------------------------
  glob->NewOptI("nSplit"        ,2);
  glob->NewOptC("splitType"     ,"byInFiles"); // [serial,blocks,random,byInFiles] - methods for splitting the input dataset
  glob->NewOptC("splitTypeTrain","");          // in case of seperate input files - this is the list of training  input files
  glob->NewOptC("splitTypeTest" ,"");          // in case of seperate input files - this is the list of testing   input files
  glob->NewOptC("splitTypeValid","");          // deprecated (kept for backward compatibility only)
  glob->NewOptI("splitSeed"     ,19888687);    // seed for random number generator for one of the splitting methods
  glob->NewOptC("inputVariables","");          // list of input variables as they appear in the input ascii files
  glob->NewOptC("inputVarErrors","");          // (optional) list of input variable errors

  // KNN weights from a reference dataset
  // -----------------------------------------------------------------------------------------------------------
  glob->NewOptB("useWgtKNN"            ,false); // add KNN weights based on an input cataloge, defined in inAsciiFiles_wgtKNN
  glob->NewOptC("inAsciiFiles_wgtKNN"  ,"");    // list of input files for KNN weight computation
  glob->NewOptC("inAsciiVars_wgtKNN"   ,"");    // list of input variables and variable-types as they appear in inAsciiFiles_wgtKNN
  glob->NewOptC("weightVarNames_wgtKNN","");    // list of input variables for KNN weight computation
  glob->NewOptC("outAsciiVars_wgtKNN"  ,"");    // list of output variables to be written to the ascii output of the KNN weight computation
  glob->NewOptI("minNobjInVol_wgtKNN"  ,50);    // minimal number of objects to use per KNN-volume in the KNN weight computation
  glob->NewOptC("weightInp_wgtKNN"     ,"");    // weight expression for input     kd-tree (function of the variables used in weightVarNames_wgtKNN)
  glob->NewOptC("weightRef_wgtKNN"     ,"");    // weight expression for reference kd-tree (function of the variables used in weightVarNames_wgtKNN)
  glob->NewOptC("cutInp_wgtKNN"        ,"");    // cut expression for input     kd-tree (function of the variables used in weightVarNames_wgtKNN)
  glob->NewOptC("cutRef_wgtKNN"        ,"");    // cut expression for reference kd-tree (function of the variables used in weightVarNames_wgtKNN)
  glob->NewOptF("sampleFracInp_wgtKNN" ,1);     // fraction of the input sample to use for the kd-tree (positive number, smaller or equal to 1)
  glob->NewOptF("sampleFracRef_wgtKNN" ,1);     // fraction of the input sample to use for the kd-tree (positive number, smaller or equal to 1)
  glob->NewOptB("doWidthRescale_wgtKNN",true);  // transform the input parameters used for the kd-tree to the range [-1,1]
  // number of KNN modules to use for hierarchical searches (may limit if consumes too much memory, but must be >= 2
  glob->NewOptI("nKnnFracs_wgtKNN"     ,10);
  // factor to decrease fraction of accepted objects for each KNN module - e.g., for module 1 all objects are
  // in, for module 2, 1/knnFracFact_wgtKNN are in, for module 3 1/(knnFracFact_wgtKNN*knnFracFact_wgtKNN) are in ...
  glob->NewOptI("knnFracFact_wgtKNN"   ,3);
  // by default, the weights are computed for the entire sample. That is, the training and the testing samples
  // are used together - we calculate the difference between the distribution of input-variables between [train+test samples]
  // and [ref sample]. However, it is possible to decide to compute the weights for each separately. That is, to calculate
  // weights for [train sample] with regards to [ref sample], and to separately get [test sample] with regards to [ref sample]. The
  // latter is only recommended if the training and testing samples have different input-variable distributions.
  glob->NewOptB("trainTestTogether_wgtKNN",true);

  // input files (given by splitTypeTrain, splitTypeTest and inAsciiFiles) may also be root files, containing
  // root trees, instead of ascii files. In this case, the name of the tree in the input files is
  // defined in inTreeName. one may alternatively define separate tree names for training and
  // testing, using inTreeNameTrain and inTreeNameTest.
  glob->NewOptC("inTreeName"      ,"");
  glob->NewOptC("inTreeNameTrain" ,"");
  glob->NewOptC("inTreeNameTest"  ,"");

  // if root input is given in inAsciiFiles_wgtKNN, the corresponding tree name is defined in treeName_wgtKNN
  glob->NewOptC("inTreeName_wgtKNN"     ,"");

  // independent KNN error estimation for an external dataset
  // -----------------------------------------------------------------------------------------------------------
  glob->NewOptC("zReg_onlyKnnErr"    ,"");   // name of variable for which the error is estimated
  glob->NewOptC("knnVars_onlyKnnErr" ,"");   // name of variable which corresponds to the true value of "zReg_onlyKnnErr"
  glob->NewOptC("cuts_onlyKnnErr"    ,"");   // possible cut expression applied to the KNN error estimator
  glob->NewOptC("weights_onlyKnnErr" ,"");   // possible weight expression applied to the KNN error estimator
  glob->NewOptB("doPlots_onlyKnnErr" ,true); // a flag to choose if to use doMetricPlots() for the onlyKnnErr setting

  // addInTrainFlag, minNobjInVol_inTrain, maxRelRatioInRef_inTrain -
  // -----------------------------------------------------------------------------------------------------------
  //   addInTrainFlag           - calculate for each object which is evaluated, if it is "close" in the input-parameter space to the training dataset.
  //                              The result is written as part of the evaluation output, as an additional parameter (name defined by baseName_inTrain),
  //                              which is zero if the object is not "close" to the training objects (therefore has unreliable result).
  //                              The calculation is performed using a KNN approach, similar to the algorithm used for the "useWgtKNN" calculation.
  //   minNobjInVol_inTrain     - The number of reference objects in the reference dataset which are used in the calculation.
  //   maxRelRatioInRef_inTrain - A number in the range, [0,1] - The minimal threshold of the relative difference between distances 
  //                              in the inTrainFlag calculation for accepting an object. If set to a negative value, then the value of the
  //                              output parameter will be distributed within the range [0,1].
  //   ...._inTrain             - The rest of the parameters ending with "_inTrain" have a similar role as their "_wgtKNN" counterparts
  // -----------------------------------------------------------------------------------------------------------
  glob->NewOptB("addInTrainFlag"          ,false);
  glob->NewOptI("minNobjInVol_inTrain"    ,100);
  glob->NewOptF("maxRelRatioInRef_inTrain",-1);
  glob->NewOptC("weightVarNames_inTrain"  ,"");    // list of input variables for KNN in/out computation
  glob->NewOptC("outAsciiVars_inTrain"    ,"");    // list of output variables to be written to the ascii output of the KNN in/out computation
  glob->NewOptC("weightInp_inTrain"       ,"");    // weight expression for input     kd-tree (function of the variables used in weightVarNames_inTrain)
  glob->NewOptC("weightRef_inTrain"       ,"");    // weight expression for reference kd-tree (function of the variables used in weightVarNames_inTrain)
  glob->NewOptC("cutInp_inTrain"          ,"");    // cut expression for input     kd-tree (function of the variables used in weightVarNames_inTrain)
  glob->NewOptC("cutRef_inTrain"          ,"");    // cut expression for reference kd-tree (function of the variables used in weightVarNames_inTrain)
  glob->NewOptF("sampleFracInp_inTrain"   ,1);     // fraction of the input sample to use for the kd-tree (positive number, smaller or equal to 1)
  glob->NewOptF("sampleFracRef_inTrain"   ,1);     // fraction of the input sample to use for the kd-tree (positive number, smaller or equal to 1)
  glob->NewOptB("doWidthRescale_inTrain"  ,true);  // transform the input parameters used for the kd-tree to the range [-1,1]
  glob->NewOptB("testAndEvalTrainMethods" ,false); // perform a TMVA test of the results of the training

  // -----------------------------------------------------------------------------------------------------------
  // general options (regression, binned-classification and classification)
  // -----------------------------------------------------------------------------------------------------------
  glob->NewOptI("nMLMs",-1); // number of MLMs for randomized regression/classification

  // -----------------------------------------------------------------------------------------------------------
  // general options (regression, binned-classification)
  // -----------------------------------------------------------------------------------------------------------
  glob->NewOptC("zTrg"           ,""); // the name of regression target - it must correspond to one of the input variables
  glob->NewOptF("minValZ"        , 1); // minimal value of regression target (the "truth" value)
  glob->NewOptF("maxValZ"        ,-1); // maximal value of regression target
  glob->NewOptF("underflowZ"     ,-1); // underflow value of regression target
  glob->NewOptF("overflowZ"      ,-1); // overflow value of regression target
  glob->NewOptI("nUnderflowBins" , 3); // number of underflow bins
  glob->NewOptI("nOverflowBins"  , 3); // number of overflow bins
  glob->NewOptF("underflowZwidth",-1); // total width of all underflow bins
  glob->NewOptF("overflowZwidth" ,-1); // total width of all overflow bins

  // -----------------------------------------------------------------------------------------------------------
  // general options (binned-classification)
  // -----------------------------------------------------------------------------------------------------------

  // useBinClsPrior -
  //   impose a "prior" on the complete probability distribution, based on the distribution of the
  //   target-variable in the training sample -> scalle the probability by the number of signal objects in
  //   each classification bin, relative to the entire sample
  // -----------------------------------------------------------------------------------------------------------
  glob->NewOptB("useBinClsPrior",true);

  // binCls_nBins,binCls_maxBinW,binCls_clsBins -
  // -----------------------------------------------------------------------------------------------------------
  // - Two options to define the classification bins:
  //   1. If [binCls_maxBinW==0], then set e.g.,:
  //        glob.annz["binCls_nBins"]   = 7
  //        glob.annz["binCls_clsBins"] = "0.05;0.1;0.2;0.3;0.4;0.5;0.6;0.8"
  //      to use a specific set of bins. It is required to explicitly set binCls_nBins, the value of the number
  //      of bins which are given in binCls_clsBins. This is to ensure that the training loop indeed
  //      produces an MLM for each of the bins in range(binCls_nBins).
  //      Make sure that the first and last bins in binCls_clsBins are within minValZ and maxValZ.
  //   2. If [binCls_clsBins==""], then set e.g.,:
  //        glob.annz["binCls_nBins"]   = 10
  //        glob.annz["binCls_maxBinW"] = 0.1
  //      to divide the range between minValZ and maxValZ into binCls_nBins bins. The bin-width is
  //      defined on the fly according, such that each bin has approximately the same number of (weighted) entries,
  //      while constraining the ben-width to be no larger than binCls_maxBinW. (see description in ANNZ::deriveBinClsBins().)
  //      It is possible for the automatic bin calculation not to work (especially with sparse distributions),
  //      so please check the output of the training, and confirm that the bins (and the bin content) are reasonable
  // -----------------------------------------------------------------------------------------------------------
  glob->NewOptI("binCls_nBins"  ,-1);
  glob->NewOptF("binCls_maxBinW",-1);
  glob->NewOptC("binCls_clsBins","");

  // generate binCls_nTries different randomized MLMs for each bin, and use only the one which has the highest separation parameter
  // -----------------------------------------------------------------------------------------------------------
  glob->NewOptI("binCls_nTries",-1);

  // binCls_bckShiftMin,binCls_bckShiftMax (optional training setting) -
  //   setting binCls_bckShiftMin,binCls_bckShiftMax to some values with (binCls_bckShiftMax > binCls_bckShiftMin)
  //   will exclude a range of background values from the training.
  //   For a signal bin z0<zTrg<z1 the nominal bacgkround will be (zTrg<z0 || zTrg>z1). Then the background
  //   will be defined as (zTrg<z0-x || zTrg>z1+x), with x a random number between binCls_bckShiftMin and binCls_bckShiftMax
  // -----------------------------------------------------------------------------------------------------------
  glob->NewOptF("binCls_bckShiftMin", 1);
  glob->NewOptF("binCls_bckShiftMax",-1);

  // - binCls_bckSubsetRange (optional training setting) -
  //   - Setting binCls_bckSubsetRange to value of format "x_y_p" will reject background objects from training,
  //     such that the ratio of background to signal is in the range between x and y. This will happen
  //     for p% of the requested binCls_nTries.
  //     For instance:
  //       using [binCls_bckSubsetRange="5_10_20"], the training will include between 5 times
  //       to 10 times as many background objects as signal objects. this cut will be applied 20% of
  //       the time (out of binCls_nTries).
  //   - all values, (x,y,p) should be integers.
  //   - the excat ratio of bck/sig is randomely generated within the requested range.
  // --------------------------------------------------------------------------------------------------
  glob->NewOptC("binCls_bckSubsetRange","");

  glob->NewOptB("optimMLMprb",true); // optimize by probability rather than by clssification-responce (the direct MLM output)
  glob->NewOptC("MLMsToStore","");   // ("BEST", "ALL", "3" or "0;1;4") which optimized MLMs to store to file during doEval

  // doMultiCls -
  //   use the multiClass classification option, where multiple distinct background samples may be used. This
  //   may currently only be used for binned classification. 
  // -----------------------------------------------------------------------------------------------------------
  // - Using the MultiClass option of binned classification, multiple background samples can be trained
  //   simultaneously against the signal. This means that each classification bin acts as an independent sample during
  //   the training. The MultiClass option is only compatible with four MLM algorithms: BDT, ANN, FDA and PDEFoam.
  //   For BDT, only the gradient boosted decision trees are available. That is, one may set ":BoostType=Grad",
  //   but not ":BoostType=Bagging" or ":BoostType=AdaBoost", as part of the userMLMopts option.
  //   - examples:
  //     - glob.annz["userMLMopts_0"] = "ANNZ_MLM=FDA:Formula=(0)+(1)*x0+(2)*x1+(3)*x2+(4)*x3:"
  //                                   +"ParRanges=(-1,1);(-10,10);(-10,10);(-10,10);(-10,10):"
  //                                   +"FitMethod=GA:PopSize=300:Cycles=3:Steps=20:Trim=True:SaveBestGen=1"
  //     - glob.annz["userMLMopts_1"] = "ANNZ_MLM=PDEFoam:nActiveCells=500:nSampl=2000:nBin=5:Nmin=100:Kernel=None:Compress=T"
  // - Using the MultiClass option, the binCls_bckShiftMin,binCls_bckShiftMax,binCls_bckSubsetRange
  //   options are ignored.
  // - Using the MultiClass option, training is much slower, it is therefore recommended to set a low
  //   value (<3) of binCls_nTries.
  glob->NewOptB("doMultiCls",false);

  // -----------------------------------------------------------------------------------------------------------
  // general options (classification)
  // -----------------------------------------------------------------------------------------------------------
  glob->NewOptB("addClsKNNerr",false); // add (experimental) KNN-error estimation for classification

  // -----------------------------------------------------------------------------------------------------------
  // training (regression, binned-classification and classification)
  // -----------------------------------------------------------------------------------------------------------
  glob->NewOptI("nMLMnow"    ,-1); // current index of MLM for training randomized regression/classification
  glob->NewOptI("nBinNow"    ,-1); // place-holder for the bin-index in binned classification (content copied to nMLMnow in ANNZ::Init())
  glob->NewOptC("userMLMopts",""); // user-defined options, used instead of general randomization of MLM-options

  // factory normalization (IT IS RECOMMENDED TO ALWAYS NORMALIZE!) -
  //   by default, if (alwaysUseNormalization==true), we use (NormMode=EqualNumEvents)
  //   the user can specify explicitly (NormMode=NumEvents) or (NormMode=EqualNumEvents), or choose not to
  //   use normalization by setting (alwaysUseNormalization==false) and not setting NormMode
  // -----------------------------------------------------------------------------------------------------------
  glob->NewOptB("alwaysUseNormalization",true);

  // variable transformations - 
  //   the following are available for a given MLM by using e.g., "VarTransform=P" as part of userMLMopts:
  //     D_Background,P_Signal,G,N_AllClasses for: Decorrelation, PCA-transformation, Gaussianisation, Normalisation,
  //     each for the given class of events ('AllClasses' denotes all events of all classes,
  //     if no class indication is given, 'All' is assumed)
  //   the transformations are addetive (one may book several for a given MLM)
  //   it is also possible to set the global variable "transANNZ" for all MLMs, as e.g.,
  //     args["transANNZ"] = "P"
  //   (by default args["transANNZ"] = "I" for the iddentity trans -> has not affect)
  // -----------------------------------------------------------------------------------------------------------
  glob->NewOptC("transANNZ","I");

  // generate these randomized MLM types (currently "ANN", "BDT" or "ANN_BDT" are supported)
  glob->NewOptC("rndOptTypes","ANN_BDT"); 

  // if (overwriteExistingTrain==false) and had already trained an MLM, don't overwrite the directory and don't retrain
  glob->NewOptB("overwriteExistingTrain",false);

  // -----------------------------------------------------------------------------------------------------------
  // optimization (randomized regression)
  // -----------------------------------------------------------------------------------------------------------
  // exclude margin for fitting cumulative dist as part of PDF optimization (eg within [0,0.1])
  glob->NewOptF("excludeRangePdfModelFit",0);

  // optimCondReg -
  //   ["sig68" or "bias"] - used for deciding how to rank MLM performance. the named criteria represents
  //   the metric which is more significant in deciding which MLM performs "best".
  // -----------------------------------------------------------------------------------------------------------
  glob->NewOptC("optimCondReg","sig68");

  // force the MLM ranking alg to take the outlier fraction into account (not recommended)
  glob->NewOptB("optimWithFracSig68",false); 

  // use MAD (median absolute deviation) instead of sigma_86 for randomized regression optimization
  glob->NewOptB("optimWithMAD",false); 

  // use scaled bias ((zReg-zTrg)/(1+zTrg)) instead of delta for randomized regression optimization
  glob->NewOptB("optimWithScaledBias",false);

  // -----------------------------------------------------------------------------------------------------------
  // evaluation (regression, binned-classification and classification)
  // -----------------------------------------------------------------------------------------------------------
  // input ascii files for evaluation are first converted to trees. keepEvalTrees sets if these are kept or deleted
  glob->NewOptB("keepEvalTrees", false);

  // -----------------------------------------------------------------------------------------------------------
  // user-defined cuts and weights
  // -----------------------------------------------------------------------------------------------------------
  glob->NewOptB("useCutsMinMaxZ"         ,true);  // reject objects with regression target outside of range in training/optimization
  glob->NewOptC("userCuts_train"         ,"");    // cuts applied only on training   sample
  glob->NewOptC("userCuts_valid"         ,"");    // cuts applied only on validation sample
  glob->NewOptC("userCuts_sig"           ,"");    // expression defining the signal     sample for classification
  glob->NewOptC("userCuts_bck"           ,"");    // expression defining the background sample for classification
  glob->NewOptC("userWeights_train"      ,"1");   // weights applied on training   sample
  glob->NewOptC("userWeights_valid"      ,"1");   // weights applied on validation sample
  glob->NewOptC("userWeights_metricPlots","1");   // weights applied on plotting during doMetricPlots()

  // if userCuts_valid (userWeights_valid) is set to a different value during optimization compared to its
  // value during training, then modify_userCuts_valid (modify_userWeights_valid) will determine what is
  // done (new cuts/weights may be added, overwriten or ignored).
  // -----------------------------------------------------------------------------------------------------------
  glob->NewOptC("modify_userCuts_valid"   ,"ADD"); // set to "ADD", "OVERWRITE" or "IGNORE"
  glob->NewOptC("modify_userWeights_valid","ADD"); // set to "ADD", "OVERWRITE" or "IGNORE"

  // -----------------------------------------------------------------------------------------------------------
  // PDF settings (randomized regression and binned classification)  
  // -----------------------------------------------------------------------------------------------------------
  glob->NewOptI("nPDFs"             ,0);    // number of PDF types to generate (currently 2 are implemented in each of the operational modes)
  glob->NewOptC("userPdfBins"       ,"");   // define a specific set of PDF bins (instead of nPDFbins equal-width bins)
  glob->NewOptF("minPdfWeight"      ,0.01); // weights smaller than minPdfWeight will be discarded
  // number of PDF bins (equal distance bins in the range [minValZ,maxValZ]). If userPdfBins is defined, this value is ignored
  glob->NewOptI("nPDFbins"          ,0);
  // instead of nPDFbins, it is possibleto define the width of a pdf bin, using pdfBinWidth
  glob->NewOptF("pdfBinWidth"       ,0);
  // number of random MLM weighting schemes to generate as part of getRndMethodBestPDF()
  glob->NewOptI("nRndPdfWeightTries",30);
  // number of random smearing to perform when folding uncertainty estimates into MLM solutions for PDF generation
  glob->NewOptI("nSmearsRnd"        ,50);
  // a lower acceptance bound to check if too few MLMs are trained or if something went wrong with the optimization procedure
  // (e.g., not enough trained MLMs have 'good' combinations of scatter, bias and outlier-fraction metrics).
  glob->NewOptI("minAcptMLMsForPDFs",5);
  // number of random smearing to perform for the PDF bias-correction
  glob->NewOptI("nSmearUnf"         ,100);
  // add calculation of maximum of PDF to output
  glob->NewOptB("addMaxPDF"         ,false);
  // flag to allow the option to NOT store the full value of pdfs in the output of optimization/evaluation
  // (so only the average metrics of a pdf are included in the output)
  glob->NewOptB("doStorePdfBins"    ,true);

  // if max_sigma68_PDF,max_bias_PDF are positive, they put thresholds on the maximal value of the
  // scatter/bias/outlier-fraction of an MLM which may be included in the PDF created in randomized regression
  glob->NewOptF("max_sigma68_PDF"   ,-1);    // maximal value of the scatter          of an MLM included in the PDF
  glob->NewOptF("max_bias_PDF"      ,-1);    // maximal value of the bias             of an MLM included in the PDF
  glob->NewOptF("max_frac68_PDF"    ,-1);    // maximal value of the outlier-fraction of an MLM included in the PDF
  
  glob->NewOptI("max_optimObj_PDF"        ,1e4);   // maximal number of objects to use in order to optimize PDFs
  glob->NewOptI("nOptimLoops"             ,1e4);   // maximal number of tries to improve the PDF weights
  glob->NewOptB("addOldStylePDFs"         ,false); // whether to use the old-style PDFs (defined since v2.2.3)
  glob->NewOptI("max_staticOptimTries_PDF",250);   // maximal number of steps for the optimization random walk
  
  // bias-correction procedure on MLMs and/or PDFs
  //   doBiasCorPDF      - whether or not to perform the correction for PDFs (during optimization)
  //   doBiasCorMLM      - whether or not to perform the correction for MLMs (during training)
  //   biasCorMLMopt     - MLM configuration options for the bias-correction for MLMs
  //                       - can take the same format as userMLMopts (e.g., [biasCorMLMopt="ANNZ_MLM=BDT:VarTransform=N:NTrees=100"])
  //                       - can be empty (then the job options will be automatically generated, same as is setting [userMLMopts=""])
  //                       - can be set as [biasCorMLMopt="same"], then the same configuration options as for the nominal MLM
  //                         for which the bias-correction is applied are used
  //                       - simple MLMs are recommended, e.g.: 
  //                          - BDT with around 50-100 trees:
  //                            "ANNZ_MLM=BDT:VarTransform=N:NTrees=100:BoostType=AdaBoost"
  //                          - ANN with a simple layer structure, not too many NCycles etc.:
  //                            "ANNZ_MLM=ANN::HiddenLayers=N,N+5:VarTransform=N,P:TrainingMethod=BFGS:NCycles=500:UseRegulator=True"
  //   biasCorMLMwithInp - add the nominal MLM as an input variable for the new MLM of the bias-correction (not strictly necessary)
  //   alwaysKeepBiasCor - whether or not to not check the KS-test and N_poiss metrics for improvement in order to
  //                       possibly reject the bias correction (check performed if [alwaysKeepBiasCor] is set to true)
  // -----------------------------------------------------------------------------------------------------------
  glob->NewOptB("doBiasCorPDF"      ,true);
  glob->NewOptB("doBiasCorMLM"      ,true);
  glob->NewOptB("biasCorMLMwithInp" ,false);
  glob->NewOptB("alwaysKeepBiasCor" ,false);
  glob->NewOptC("biasCorMLMopt"     ,"ANNZ_MLM=BDT:VarTransform=N:NTrees=50:BoostType=AdaBoost");

  // -----------------------------------------------------------------------------------------------------------
  // general stuff
  // -----------------------------------------------------------------------------------------------------------
  // possible log levels: ("ERROR","WARNING","INFO","DEBUG","DEBUG_1","DEBUG_2","DEBUG_3","DEBUG_4")
  glob->NewOptC("logLevel"        ,"INFO");    // logging output level
  
  glob->NewOptB("isBatch"         ,false);     // reduce output (especially progress bar) for batch running
  glob->NewOptC("outDirName"      ,"output");  // working directory for a given analysis
  glob->NewOptC("inDirName"       ,"rootIn");  // input directory for source-ascii files - only used for doGenInputTrees()
  glob->NewOptI("maxNobj"         ,0);         // limit the number of objects to use (if zero -> use all)
  glob->NewOptC("evalDirPostfix"  ,"");        // add this to the name of the evaluation directory
  glob->NewOptI("nObjectsToWrite" ,(int)5e5);  // maximal number of objects in a tree/output ascii file
  glob->NewOptI("nObjectsToPrint" ,(int)1e4);  // frequency for printing loop counter status
  glob->NewOptB("doPlots"         ,true);      // whether or not to generate performance and control plots
  glob->NewOptC("zTrgTitle"       ,"Z_{trg}"); // title of regression target   (for plots)
  glob->NewOptC("zRegTitle"       ,"Z_{reg}"); // title of regression variable (for plots)
  glob->NewOptF("zPlotBinWidth"   ,-1);        // width of bins to perform the plotting
  glob->NewOptI("nDrawBins_zTrg"  ,-1);        // number of bins in zTrg for plotting
  // possible list of variables which will always be plotted (that is, no safety checks on variable type will be performed)
  glob->NewOptC("alwaysPlotVars"  ,"");
  // possible list of variables for which we do not use quantile bins for plotting - e.g., set as "inTrainFlag;MAG_U"
  glob->NewOptC("noQuantileBinsPlots"  ,"inTrainFlag");
  // add plots with the distribution of the knn error estimator
  glob->NewOptB("doKnnErrPlots"   ,false); 

  // zClosBinWidth - typical width in the regression variable for plotting, e.g., for 10 plotting
  // bins, set zClosBinWidth = (maxValZ-minValZ)/10. - if not set, a quantile division will be used of the zTrg range
  // nZclosBins - alternatively, dynamically derive nZclosBins bins from the quantile distribution of zTrg
  glob->NewOptF("zClosBinWidth"   ,-1);
  glob->NewOptI("nZclosBins"      ,-1);

  // format for plotting (in addition to generated root scripts, which may be run with [root -l script.C]
  // availabe formats (leave empty [glob->NewOptC("printPlotExtension","")] to prevent plotting):
  //   "ps" "eps" "pdf" "svg" "tex" "gif" "xpm" "png" "jpg" "tiff" "xml" 
  // see: http://root.cern.ch/root/html/TPad.html#TPad:SaveAs
  // -----------------------------------------------------------------------------------------------------------
  glob->NewOptC("printPlotExtension","pdf");

  // flag to store root scripts corresponding to generated plots
  glob->NewOptB("savePlotScripts",false); 

  // use scaled bias (delta/(1+zTrg)) instead of delta for plotting in ANNZ::doMetricPlots()
  glob->NewOptB("plotWithScaledBias",false); 

  // -----------------------------------------------------------------------------------------------------------
  // uncertainty estimators - either KNN (K-near-neighbours) entimation (used by default), or input-error propagation.
  // -----------------------------------------------------------------------------------------------------------
  // by defaul will use standard-deviation to compute KNN-errors (may switch to sigma68 -> smaller error estimates)
  glob->NewOptB("defErrBySigma68",false);

  // for plotting only - derive the scatter of the relative uncertainty estimator in as a Gaussian fit instead of as
  // the scatter (or 68th percentile scatter) of the distribution
  glob->NewOptB("doGausSigmaRelErr",true);

  // number of near-neighbours to use for the KNN error estimation, see setupKdTreeKNN().
  glob->NewOptI("nErrKNN",100);
  // whether or not to rescale the input parameters of the knn-err search to the range [-1,1]
  glob->NewOptB("doWidthRescale_errKNN",true);
  // fraction of the input sample to use for the kd-tree uncertainty calculation. takes values within [0,1]
  glob->NewOptF("sampleFrac_errKNN",1);

  // if propagating input-errors - nErrINP is the number of randomly generated MLM values used to propagate
  // the uncertainty on the input parameters to the MLM-estimator. See getRegClsErrINP()
  glob->NewOptI("nErrINP",-1);

  // add positive and negative error estimators for MLMs to the ascii output of regression
  glob->NewOptB("writePosNegErrs",false);
  
  // -----------------------------------------------------------------------------------------------------------
  // technical stuff  
  // -----------------------------------------------------------------------------------------------------------
  glob->NewOptB("copyCodeToOutDir",true);  // save the current source code and running scripts to the working dir

  // lower and upper bounds on closure-histograms, which have as input (estimator - trueValue). If unchanged by the user from
  // the default values (with closHisL==closHisH), they will be set to [(maxValZ-minValZ)/2. ± 4.5 * (maxValZ-minValZ)]
  glob->NewOptF("closHisL"  ,-1);
  glob->NewOptF("closHisH"  ,-1);
  glob->NewOptI("closHisN"  ,50000); // # of bins in histograms used for quantile calculations (needs to be large number)
  glob->NewOptI("hisBufSize",50000); // # of objects to include in histograms of variable bins before setting final binning
  
  // keep or delete temporary trees generated during trainin
  glob->NewOptB("keepTrainingTrees_factory"  ,false); // TMVA tree - generated during training (small and can be kept for cross-checks)
  glob->NewOptB("keepTrainingTrees_sigBckCut",false);
  glob->NewOptB("keepOptimTrees_randReg"     ,true);
  
  // copy these folders/files as backup from the current directory to the working directory
  glob->NewOptC("copyCodeCmnd","rsync -Rrtaz --include 'examples/' --include 'scripts/' --include 'py/' --include 'include/' --include 'src/' --include '*.py' --include '*.hpp' --include '*.cpp' --exclude '*' *");

  // set info-level for ROOT operations (messages like plot printing etc. will be written out)
  glob->NewOptB("set_kInfoROOT",false);

  // default types for tree-connected variables
  // -----------------------------------------------------------------------------------------------------------
  glob->NewOptC("defVarSIL"           ,"I");  // short (S), int (I) or long (L)
  glob->NewOptC("defVarUSUIUL"        ,"UI"); // unsigned short (US), unsigned int (UI) or unsigned long (UL)
  glob->NewOptC("defVarFD"            ,"F");  // float (F) or double (D)
  glob->NewOptI("clsResponseHisN"     ,100);  // number of initial bins for classification response histograms
  glob->NewOptI("clsResponseHisR"     ,4);    // rebin factor           for classification response histograms
  glob->NewOptB("getSeparationWithPDF",true); // calculate separation parameters with PDF-spline fits (==true) or with histogramed data (==false)
  glob->NewOptI("initSeedRnd"         ,1979); // some random number so that the same set of randoms are chosen each time the code is run
  glob->NewOptB("doStoreToAscii"      ,true); // store evaluation into ascii files
  glob->NewOptI("minObjTrainTest"     ,50);   // minimal number of objects to use for training (very dangerous to set this too low !!!!)
  glob->NewOptB("isReadOnlySys"      ,false); // allow evalWrapper to be run on a read-only system

  // number of times to divide the collection of MLMs needed for evaluation - in principle, no division is neccessary, however,
  // some MLMs (notabley BDTs) require a lot of memory. Therefore, it might be better to only evaluate a sub-sample of the MLMs
  // at a time, and then combine the results when the actuall pdfs are evaluated. A high value of nDivEvalLoops will reduce the
  // memory usage, but will also slow down the code
  // -----------------------------------------------------------------------------------------------------------
  glob->NewOptI("nDivEvalLoops",1);

  // merge all postTrain trees into one file - if there are too many separate inputs (more than maxTreesMerge)
  // then split the merging into several steps - this avoids situations in which too many input files
  // need to be opened at once
  // -----------------------------------------------------------------------------------------------------------
  glob->NewOptI("maxTreesMerge",50);

  return;
}

// ===========================================================================================================
/**
 * @brief    - Initialization of Manager inc. user options.
 * @details  - Make sanity checks, input variable reformatting (e.g., add '/' at the
 *           end of directory names), and initialize instances of Utils and OutMngr.
 */
// ===========================================================================================================
void Manager::Init(int argc, char ** argv) {
// ===========================================================================================================
  // -----------------------------------------------------------------------------------------------------------
  // current version-tag for the code
  // -----------------------------------------------------------------------------------------------------------
  TString basePrefix("ANNZ_"), versionTag("2.3.0");
  // sanity check on allowed root versions with corresponding supported TMVA versions
  VERIFY(LOCATION,(TString)" - Using unsupported ROOT version ... ?!?!?",(ROOT_TMVA_V0 || ROOT_TMVA_V1));
  // -----------------------------------------------------------------------------------------------------------

  // -----------------------------------------------------------------------------------------------------------
  // read-in user inputs and modify/add glob->() options
  // -----------------------------------------------------------------------------------------------------------
  vector <TString> optNames;
  glob->GetAllOptNames(optNames);

  int  optNow(0), strLength(0);
  while(optNow < argc) {
    std::string  optStrStart, optStrEnd, optStr( argv[optNow] ), optStrNow;
    TString optTStr((TString)optStr), optNameNow, typeNow;
    // remove spaces from string
    std::string::iterator end_pos = std::remove(optStr.begin(), optStr.end(), ' ');  optStr.erase(end_pos, optStr.end());

    // go over all options which were defined in the Manager constructor
    for(int i=0; i<(int)optNames.size(); i++) {
      optNameNow = optNames[i];
      if(!(((TString)optStr).Contains(optNameNow))) continue;

      typeNow   = glob->GetOptType(optNameNow);  optStrNow = (TString)optNameNow+"=";
      strLength = optStrNow.length();            optStrStart.assign(optStr,0,strLength);
      optStrEnd = optStr;                        optStrEnd.erase(0,strLength);

      if( optStrStart == optStrNow && (typeNow == "C" || (typeNow != "C" && optStrEnd != "")) ) {
        if     (typeNow == "I") glob->NewOptI(optNameNow , static_cast<int>   (atof((char*)optStrEnd.c_str())));
        else if(typeNow == "F") glob->NewOptF(optNameNow , static_cast<double>(atof((char*)optStrEnd.c_str())));
        else if(typeNow == "C") glob->NewOptC(optNameNow , (TString)optStrEnd);
        else if(typeNow == "B") {
          if     (optStrEnd == "0" || optStrEnd == "false" || optStrEnd == "False" || optStrEnd == "FALSE") glob->NewOptB(optNameNow , false);
          else if(optStrEnd == "1" || optStrEnd == "true"  || optStrEnd == "True"  || optStrEnd == "TRUE")  glob->NewOptB(optNameNow , true );
          else { cout <<"Unknown bool flag initializtion ("<<optNameNow<<" = "<<optStrEnd<<")... ABOTRING !!!"<<endl;  exit(1); }
        }
      }
      if(optStrEnd == "" && typeNow == "B") glob->NewOptB(optTStr , true);
    }

    // general types according to patterns
    if(optTStr.BeginsWith("debug")) glob->NewOptB(optTStr(0,optTStr.First("=")), true);  // debug,debugCutParser,...
    if((optTStr(0,4)) == "int_" ) {
      glob->NewOptI( optTStr(4,optTStr.First("=")-4) ,
                     static_cast<int>(floor(((TString)(optTStr(optTStr.First("=")+1,optTStr.Length()))).Atof())) );
    }
    if((optTStr(0,4)) == "str_" ) {
      // EXMAPLE: str_dir_0=rootIn/trainTest will do:   glob->optS["dir_0"] = "rootIn/trainTest";
      glob->NewOptC( optTStr(4,optTStr.First("=")-4) , (TString)(optTStr(optTStr.First("=")+1,optTStr.Length())) );
    }
    if(optTStr.BeginsWith("inputVariables_")) {
      glob->NewOptC( optTStr(0,optTStr.First("=")) , (TString)(optTStr(optTStr.First("=")+1,optTStr.Length())) );
    }
    if(optTStr.BeginsWith("inputVarErrors_")) {
      glob->NewOptC( optTStr(0,optTStr.First("=")) , (TString)(optTStr(optTStr.First("=")+1,optTStr.Length())) );
    }
    if(optTStr.BeginsWith("userMLMopts_")) {
      glob->NewOptC( optTStr(0,optTStr.First("=")) , (TString)(optTStr(optTStr.First("=")+1,optTStr.Length())) );
    }
    optNow++;  
  }
  optNames.clear();

  // -----------------------------------------------------------------------------------------------------------
  // sanity check of user string-input
  // -----------------------------------------------------------------------------------------------------------
  glob->GetAllOptNames(optNames,"C");
  for(int nOptNow=0; nOptNow<(int)optNames.size(); nOptNow++) {
    glob->SetOptC(optNames[nOptNow],(glob->GetOptC(optNames[nOptNow])).ReplaceAll("\n","").ReplaceAll("\r",""));
  }
  optNames.clear();

  // -----------------------------------------------------------------------------------------------------------
  // internal naming conventions, which should not be tampered with
  // -----------------------------------------------------------------------------------------------------------
  // names of index parameters in trees
  // -----------------------------------------------------------------------------------------------------------
  glob->NewOptC("basePrefix"          ,basePrefix);             // base tag for all MLM names
  glob->NewOptC(glob->versionTag()    ,basePrefix+versionTag);  // version-tag
  glob->NewOptC("baseName_inVarErr"   ,basePrefix+"inVarErr_"); // base tag for all PDF names
  glob->NewOptC("baseName_nPDF"       ,basePrefix+"PDF_");      // base tag for all PDF names
  glob->NewOptC("baseName_wgtKNN"     ,basePrefix+"KNN_w");     // KNN weight variable
  glob->NewOptC("treeName"            ,basePrefix+"tree");      // internal name prefix for input trees
  glob->NewOptC("hisName"             ,basePrefix+"his");       // internal name prefix for histograms
  glob->NewOptC("indexName"           ,basePrefix+"index");     // original index from input file
  glob->NewOptC("origFileName"        ,basePrefix+"inFile");    // name of original source file
  glob->NewOptC("baseName_regBest"    ,basePrefix+"best");      // the "best"-performing MLM in randomized regression
  glob->NewOptC("baseName_regMLM_avg" ,basePrefix+"MLM_avg_");  // base-name for the average MLM solution (and its error) in randomized regression
  glob->NewOptC("baseName_regPDF_max" ,basePrefix+"PDF_max_");  // base-name for the peak of the pdf solution (and its error) in randomized regression
  glob->NewOptC("baseName_regPDF_avg" ,basePrefix+"PDF_avg_");  // base-name for the average pdf solution (and its error) in randomized regression
  glob->NewOptC("baseName_knnErr"     ,"_knnErr");        // name-postfix of error variable derived with the KNN estimator 
  // internal name for the onlyKnnErr option
  glob->NewOptC("baseName_onlyKnnErr" ,"_onlyKnnErr");
  // name of an optional output parameter in evaluation (is it "safe" to use the result for an evaluated object)
  glob->NewOptC("baseName_inTrain"    ,"inTrainFlag");
  // optional parameter to mark if an object is of type signal (1), background (0) or undefined (-1), based on the name of the original input file
  glob->NewOptC("sigBckInpName","sigBckInp");
  // glob->NewOptC("splitName"           ,basePrefix+"split");  // deprecated
  // glob->NewOptC("testValidType"       ,basePrefix+"tvType"); // deprecated

  // -----------------------------------------------------------------------------------------------------------
  // working directory names
  // -----------------------------------------------------------------------------------------------------------
  glob->NewOptC("trainDirName"      ,"train");       // name of sub-dir for training
  glob->NewOptC("optimDirName"      ,"optim");       // name of sub-dir for optimization
  glob->NewOptC("verifDirName"      ,"verif");       // name of sub-dir for verification
  glob->NewOptC("evalDirName"       ,"eval");        // name of sub-dir for optimization
  glob->NewOptC("inTrainFlagDirName","inTrainFlag"); // name of sub-dir for the inTrainFlag
  glob->NewOptC("postTrainName"     ,"postTrain");   // name of sub-dir for MLM input trees
  
  TString evalTreePostfix((TString)"_"+glob->GetOptC("evalDirName"));
  glob->NewOptC("evalTreePostfix"       ,evalTreePostfix);                     // postfix for evaluation trees
  glob->NewOptC("evalTreeWrapperPostfix",(TString)evalTreePostfix+"_Wrapper"); // postfix for wrapper of eval trees
  
  // -----------------------------------------------------------------------------------------------------------
  // special setup for the doOnlyKnnErr mode so that we can safely run ANNZ
  // -----------------------------------------------------------------------------------------------------------
  if(glob->GetOptB("doOnlyKnnErr")) {
    glob->SetOptB("doRegression", true);
    glob->SetOptB("doSingleReg",  true);
    glob->SetOptI("nSplit",       1);
    glob->SetOptI("nMLMs",        1);
    glob->SetOptI("nMLMnow",      0);
  }

  // -----------------------------------------------------------------------------------------------------------
  // update/finalize glob-> paramters after user inputs and initialize utils and outputs
  // -----------------------------------------------------------------------------------------------------------
  
  // initialize the logger
  Log::theLog::ReportingLevel() = Log::theLog::FromString((std::string)glob->GetOptC("logLevel"));

  // gErrorIgnoreLevel, the global verbosity setting for ROOT, may be one of:
  //   kPrint, kInfo, kWarning, kError, kBreak, kSysError, kFatal
  gErrorIgnoreLevel = (glob->GetOptC("logLevel").BeginsWith("DEBUG")) ? kWarning : kFatal;

  aLOG(Log::INFO)<<coutGreen<<" -----------------------------------------------------"<<coutDef<<endl;
  aLOG(Log::INFO)<<coutGreen<<" - Welcome to ANNZ v"<<versionTag
                            <<" (using ROOT v"<<ROOT_RELEASE<<") -"                   <<coutDef<<endl;
  aLOG(Log::INFO)<<coutGreen<<" -----------------------------------------------------"
                            <<"------------------------------------------------------"<<coutDef<<endl;

  // -----------------------------------------------------------------------------------------------------------
  // analysis type (used for directory naming)
  // -----------------------------------------------------------------------------------------------------------
  TString analysisPrefix("");
  if     (glob->GetOptB("doOnlyKnnErr")) {
    analysisPrefix = "onlyKnnErr";
  }
  else if(glob->GetOptB("doRegression")) {
    if(glob->GetOptB("doBinnedCls")) analysisPrefix = "binCls";
    else                             analysisPrefix = "regres";
  }
  else analysisPrefix = "clasif";

  // -----------------------------------------------------------------------------------------------------------
  // working directories (definitions which must come BEFORE initializing utils and outputs)
  // -----------------------------------------------------------------------------------------------------------  
  // unique evaluation directory prefix (so that multiple evaluation of different input files will not overwrite each other)
  if(glob->GetOptC("evalDirPostfix") != "") {
    glob->SetOptC("evalDirName",(TString)glob->GetOptC("evalDirName")+"_"+glob->GetOptC("evalDirPostfix"));
  }

  // add trailing slash if needed
  if(!glob->GetOptC("inDirName")         .EndsWith("/")) glob->SetOptC("inDirName",         (TString)glob->GetOptC("inDirName")         +"/");
  if(!glob->GetOptC("outDirName")        .EndsWith("/")) glob->SetOptC("outDirName",        (TString)glob->GetOptC("outDirName")        +"/");
  if(!glob->GetOptC("trainDirName")      .EndsWith("/")) glob->SetOptC("trainDirName",      (TString)glob->GetOptC("trainDirName")      +"/");
  if(!glob->GetOptC("optimDirName")      .EndsWith("/")) glob->SetOptC("optimDirName",      (TString)glob->GetOptC("optimDirName")      +"/");
  if(!glob->GetOptC("verifDirName")      .EndsWith("/")) glob->SetOptC("verifDirName",      (TString)glob->GetOptC("verifDirName")      +"/");
  if(!glob->GetOptC("evalDirName")       .EndsWith("/")) glob->SetOptC("evalDirName",       (TString)glob->GetOptC("evalDirName")       +"/");
  if(!glob->GetOptC("inTrainFlagDirName").EndsWith("/")) glob->SetOptC("inTrainFlagDirName",(TString)glob->GetOptC("inTrainFlagDirName")+"/");
  if(!glob->GetOptC("postTrainName")     .EndsWith("/")) glob->SetOptC("postTrainName",     (TString)glob->GetOptC("postTrainName")     +"/");

  glob->SetOptC("trainDirName",      (TString)analysisPrefix+"/"           +glob->GetOptC("trainDirName"));
  glob->NewOptC("postTrainDirName",  (TString)glob->GetOptC("trainDirName")+glob->GetOptC("postTrainName"));
  glob->SetOptC("optimDirName",      (TString)analysisPrefix+"/"           +glob->GetOptC("optimDirName"));
  glob->SetOptC("verifDirName",      (TString)analysisPrefix+"/"           +glob->GetOptC("verifDirName"));
  glob->SetOptC("evalDirName",       (TString)analysisPrefix+"/"           +glob->GetOptC("evalDirName"));

  if(glob->GetOptB("doTrain")) {
    int nMLMnow = glob->GetOptB("doBinnedCls") ? glob->GetOptI("nBinNow") : glob->GetOptI("nMLMnow");
    glob->SetOptC("trainDirName",(TString)glob->GetOptC("trainDirName")+glob->GetOptC("basePrefix")+TString::Format("%d",nMLMnow)+"/");
  }

  glob->NewOptC("outDirNamePath",        (TString)glob->baseOutDirName()         +glob->GetOptC("outDirName"));
  glob->NewOptC("inputTreeDirName",      (TString)glob->GetOptC("outDirNamePath")+glob->baseInDirName());
  glob->NewOptC("trainDirNameFull",      (TString)glob->GetOptC("outDirNamePath")+glob->GetOptC("trainDirName"));
  glob->NewOptC("postTrainDirNameFull",  (TString)glob->GetOptC("outDirNamePath")+glob->GetOptC("postTrainDirName"));
  glob->NewOptC("optimDirNameFull",      (TString)glob->GetOptC("outDirNamePath")+glob->GetOptC("optimDirName"));
  glob->NewOptC("verifDirNameFull",      (TString)glob->GetOptC("outDirNamePath")+glob->GetOptC("verifDirName"));
  glob->NewOptC("evalDirNameFull",       (TString)glob->GetOptC("outDirNamePath")+glob->GetOptC("evalDirName"));
  glob->NewOptC("inTrainFlagDirNameFull",(TString)glob->GetOptC("outDirNamePath")+glob->GetOptC("inTrainFlagDirName"));

  if     (glob->GetOptB("doGenInputTrees")) glob->NewOptC("outDirNameFull", glob->GetOptC("inputTreeDirName"));
  else if(glob->GetOptB("doTrain"))         glob->NewOptC("outDirNameFull", glob->GetOptC("trainDirNameFull"));
  else if(glob->GetOptB("doOptim"))         glob->NewOptC("outDirNameFull", glob->GetOptC("optimDirNameFull"));
  else if(glob->GetOptB("doVerif"))         glob->NewOptC("outDirNameFull", glob->GetOptC("verifDirNameFull"));
  else if(glob->GetOptB("doEval"))          glob->NewOptC("outDirNameFull", glob->GetOptC("evalDirNameFull"));
  else if(glob->GetOptB("doInTrainFlag"))   glob->NewOptC("outDirNameFull", glob->GetOptC("inTrainFlagDirNameFull"));

  else VERIFY(LOCATION,(TString)"Unknown operational mode",false);

  if(!glob->GetOptC("inputTreeDirName").EndsWith("/")) glob->SetOptC("inputTreeDirName",(TString)glob->GetOptC("inputTreeDirName")+"/");
  if(!glob->GetOptC("outDirNameFull").EndsWith("/"))   glob->SetOptC("outDirNameFull",  (TString)glob->GetOptC("outDirNameFull")  +"/");

  glob->NewOptC("userOptsFile_genInputTrees",  (TString)glob->GetOptC("inputTreeDirName")+"userOpts.txt");

  // add the inTrainFlag to the output (of evaluation), if needed
  if(glob->GetOptB("addInTrainFlag") && (glob->GetOptB("doEval") || glob->GetOptB("doInTrainFlag"))) {
    TString addOutputVars = glob->GetOptC("addOutputVars");
    
    if(addOutputVars != "") addOutputVars += ";";
    addOutputVars += glob->GetOptC("baseName_inTrain");
    
    glob->SetOptC("addOutputVars",addOutputVars);
  }

  // internal variable which chooses oprational modes for CatFormat::addWgtKNNtoTree() - either we compute relative
  // weights (doRelWgts==true), or do we determine if an objects is in/out of the reference sample phasespace (doRelWgts==false)
  // -----------------------------------------------------------------------------------------------------------
  glob->NewOptB("doRelWgts",glob->GetOptB("doGenInputTrees"));

  // init Utils
  // -----------------------------------------------------------------------------------------------------------
  utils = new Utils(glob);

  // init OutMngr
  // -----------------------------------------------------------------------------------------------------------
  outputs = new OutMngr("MyOutMngr",utils,glob);

  // working directories (definitions which must come AFTER initializing utils and outputs)
  // -----------------------------------------------------------------------------------------------------------
  glob->NewOptC("baseName", utils->getShellCmndOutput("pwd | sed 's/.*\\///g'"));  // original cmnd in shell is "pwd | sed 's/.*\///g'"

  // general variables
  // -----------------------------------------------------------------------------------------------------------
  glob->NewOptC("loopTreeName","loopTree");

  // -----------------------------------------------------------------------------------------------------------
  // some sanity checks
  // -----------------------------------------------------------------------------------------------------------
  bool hasGoodOpt =   glob->GetOptB("doGenInputTrees") || 
                   ( !glob->GetOptB("doGenInputTrees")  && (   glob->GetOptB("doTrain")       || glob->GetOptB("doOptim")
                                                            || glob->GetOptB("doVerif")       || glob->GetOptB("doEval")  
                                                            || glob->GetOptB("doInTrainFlag") ) );
  VERIFY(LOCATION,(TString)"Configuration problem... Did not find \"doGenInputTrees\", \"doTrain\", "+
                           "\"doOptim\", \"doVerif\", \"doEval\" or \"doInTrainFlag\" options ..."
                           ,hasGoodOpt);

  if(glob->GetOptB("useWgtKNN") && glob->GetOptB("doGenInputTrees")) {
    VERIFY(LOCATION,(TString)"Configuration problem... Can not set \"useWgtKNN\""
                            +" without setting corresponding input files in \"inAsciiFiles_wgtKNN\"",(glob->GetOptC("inAsciiFiles_wgtKNN") != ""));
  }

  if(glob->GetOptB("doOnlyKnnErr")) {
    bool hasGoodOpt  = ( glob->GetOptB("doGenInputTrees") || glob->GetOptB("doEval") );

    VERIFY(LOCATION,(TString)"Configuration problem... Did not find \"doGenInputTrees\" or \"doEval\" options ...", hasGoodOpt);

    bool hasOtherOpt = (   glob->GetOptB("doTrain") || glob->GetOptB("doOptim")
                        || glob->GetOptB("doVerif") || glob->GetOptB("doInTrainFlag") );

    VERIFY(LOCATION,(TString)"Configuration problem... \"doOnlyKnnErr\" can not be run with any of the following options: \"doTrain\", "+
                             "\"doOptim\", \"doVerif\" or \"doInTrainFlag\" !"
                             ,!hasOtherOpt);
  }

  // check the tadaset division (full, or split into training/testing). make sure that the 
  // deprecated split to 3 samples (training/testing/validation) is not requested by mistake
  // -----------------------------------------------------------------------------------------------------------
  int nSplit = glob->GetOptI("nSplit");
  VERIFY(LOCATION,(TString)"Currently, only [\"nSplit\" = 1 or 2] is supported ...",(nSplit == 1 || nSplit == 2));

  // for backward compatibility, make sure that splitTypeValid and splitTypeTest are not noth set.
  // If splitTypeValid is set instead of splitTypeTest, give a warning set splitTypeTest and reset splitTypeValid.
  TString sptTrn(glob->GetOptC("splitTypeTrain")), sptTst(glob->GetOptC("splitTypeTest")), sptVld(glob->GetOptC("splitTypeValid"));
  if(sptTrn != "" && sptVld != "") {
    VERIFY(LOCATION,(TString)"Got [\"splitTypeTrain\"="+sptTrn+"], [\"splitTypeTest\"="+sptTst+"], [\"splitTypeValid\"="+sptVld+"] ..."
                            +" please set only \"splitTypeTest\", as \"splitTypeValid\" is deprecated.",(sptTst == ""));

    aLOG(Log::WARNING) <<coutRed<<" - Found [\"splitTypeValid\" = "<<coutPurple<<sptVld
                       <<coutRed<<"] which is deprecated ... Setting as \"splitTypeTest\" instead "<<coutDef<<endl;

    glob->SetOptC("splitTypeTest",sptVld); glob->SetOptC("splitTypeValid","");
  }

  TString sptTyp(glob->GetOptC("splitType")), inFiles(glob->GetOptC("inAsciiFiles"));
  if(glob->GetOptB("doGenInputTrees") && sptTyp != "byInFiles") {
    if(inFiles == "") {
      VERIFY(LOCATION,(TString)"Got [\"splitType\"="+sptTyp+"] but \"inAsciiFiles\" is not set ...",false);
    }
    else if(sptTrn != "" || sptTst != "" || sptVld != "") {
      aLOG(Log::WARNING) <<coutRed<<" - Found [\"splitType\" = "<<coutPurple<<sptTyp<<coutRed<<" and [\"inAsciiFiles\" = "<<coutPurple<<inFiles
                         <<coutRed<<"] but also non-empty \"splitTypeTrain\", \"splitTypeTest\" or \"splitTypeValid\"."
                         <<" Will ignore the latter three... "<<coutDef<<endl;
    }
  }


  if(glob->GetOptI("initSeedRnd") < 0) glob->SetOptI("initSeedRnd",0);
  if(glob->GetOptI("maxNobj")     < 0) glob->SetOptI("maxNobj"    ,0);

  if(glob->GetOptB("doClassification") && !glob->GetOptB("doEval")) {
    // -----------------------------------------------------------------------------------------------------------
    // make sure that at least one of the two pairs, either "userCuts_sig" and "userCuts_bck", or
    // "inpFiles_sig" and "inpFiles_bck" is defined if "inpFiles_sig" or "inpFiles_bck" are defined, parse
    // the corresoinding cut and add it to "userCuts_sig" and "userCuts_bck".
    // -----------------------------------------------------------------------------------------------------------
    bool has_userCuts_sig(glob->GetOptC("userCuts_sig") != ""), has_userCuts_bck(glob->GetOptC("userCuts_bck") != "");
    bool has_inpFiles_sig(glob->GetOptC("inpFiles_sig") != ""), has_inpFiles_bck(glob->GetOptC("inpFiles_bck") != "");

    if(!has_inpFiles_sig || !has_inpFiles_bck) {
      VERIFY(LOCATION,(TString)"Signal/background definitions are not complete - must define both [\"userCuts_sig\" and \"userCuts_bck\"]"
                      +" or use [\"has_inpFiles_sig\" and \"has_inpFiles_bck\"] instead ...",(has_userCuts_sig && has_userCuts_bck));
    }

    if(!has_userCuts_sig || !has_userCuts_bck) {
      VERIFY(LOCATION,(TString)"Signal/background definitions are not complete - if [\"userCuts_sig\" and \"userCuts_bck\"] are not used,"
                      +"  must define both [\"has_inpFiles_sig\" and \"has_inpFiles_bck\"] ...",(has_inpFiles_sig && has_inpFiles_bck));
    }

    if(has_inpFiles_sig) {
      TString userCuts_sig = (TString)+"("+glob->GetOptC("sigBckInpName")+" == 1)";
      if(has_userCuts_sig) userCuts_sig = (TString)"("+glob->GetOptC("userCuts_sig")+") && "+userCuts_sig;

      glob->SetOptC("userCuts_sig",userCuts_sig);
    }

    if(has_inpFiles_bck) {
      TString userCuts_bck = (TString)+"("+glob->GetOptC("sigBckInpName")+" == 0)";
      if(has_userCuts_bck) userCuts_bck = (TString)"("+glob->GetOptC("userCuts_bck")+") && "+userCuts_bck;

      glob->SetOptC("userCuts_bck",userCuts_bck);
    }
  }

  // verify that isReadOnlySys can be enabled only together with doEval
  if(glob->GetOptB("isReadOnlySys") && !glob->GetOptB("doEval")) {
    glob->SetOptB("isReadOnlySys" ,false);

    aLOG(Log::WARNING) <<coutRed<<" - Found [\"isReadOnlySys\" = "<<coutPurple<<"true"
                       <<coutRed<<"] which can only be used together with \"doEval\""<<coutDef<<endl;
  }


  // print out final init parameters
  if(inLOG(Log::DEBUG)) { glob->printOpts(2,30); cout<<endl; }

  return;
}

// ===========================================================================================================
/**
 * @brief    - Parse input dataset into ROOT trees for internal use by ANNZ.
 * 
 * @details  - Direct ascii to tree conversion, given a proper list of input parameters and types, using CatFormat.
 *           - May also include KNN weight calculation, based on a reference sample.
 */
// ===========================================================================================================
void Manager::GenerateInputTrees() {
// ===========================================================================================================
  // initialize the outputs with the rootIn directory and reset it
  outputs->InitializeDir(glob->GetOptC("outDirNameFull"),glob->GetOptC("baseName"));

  // init CatFormat
  CatFormat * aCatFormat = new CatFormat("aCatFormat",utils,glob,outputs);

  // -----------------------------------------------------------------------------------------------------------
  // create root trees from the input ascii files and add a weight branch, calculated with the KNN method
  // -----------------------------------------------------------------------------------------------------------
  if(glob->GetOptB("useWgtKNN")) {
    aCatFormat->inputToSplitTree_wgtKNN(glob->GetOptC("inAsciiFiles"),       glob->GetOptC("inAsciiVars"),
                                        glob->GetOptC("inAsciiFiles_wgtKNN"),glob->GetOptC("inAsciiVars_wgtKNN"));
  }  
  // -----------------------------------------------------------------------------------------------------------
  // create root trees from the input ascii files
  // -----------------------------------------------------------------------------------------------------------
  else {
    aCatFormat->inputToSplitTree(glob->GetOptC("inAsciiFiles"),glob->GetOptC("inAsciiVars"));
  }

  DELNULL(aCatFormat);

  return;
}



// ===========================================================================================================
/**
 * @brief    - Calculate the knn-error estimator for a general input dataset.
 */
// ===========================================================================================================
void Manager::doOnlyKnnErr() {
// ===========================================================================================================

  VERIFY(LOCATION,(TString)"Can not define both \"doGenInputTrees\" and \"doEval\" at the same time ... run \"doGenInputTrees\" "
                          +"separately first, then \"doEval\"", !(glob->GetOptB("doGenInputTrees") && glob->GetOptB("doEval")));

  // -----------------------------------------------------------------------------------------------------------
  // create trees from the evaluated dataset
  // -----------------------------------------------------------------------------------------------------------
  if(glob->GetOptB("doEval")) {
    // initialize the inTrainFlag directory
    outputs->InitializeDir(glob->GetOptC("outDirNameFull"),glob->GetOptC("baseName"));

    // create the trees
    CatFormat * aCatFormat = new CatFormat("aCatFormat",utils,glob,outputs);

    aCatFormat->inputToFullTree(glob->GetOptC("inAsciiFiles"),glob->GetOptC("inAsciiVars"),"_full");
    
    DELNULL(aCatFormat);
  }

  // -----------------------------------------------------------------------------------------------------------
  // calculate the knn errors
  // -----------------------------------------------------------------------------------------------------------
  ANNZ * aANNZ = new ANNZ("aANNZ",utils,glob,outputs);
  
  aANNZ->KnnErr();
  
  DELNULL(aANNZ);

  return;
}



// ===========================================================================================================
/**
 * @brief    - Calculate the inTrainFlag quality-flag, without the need to train/optimize/evaluate MLMs.
 */
// ===========================================================================================================
void Manager::doInTrainFlag() {
// ===========================================================================================================
  // initialize the inTrainFlag directory
  outputs->InitializeDir(glob->GetOptC("outDirNameFull"),glob->GetOptC("baseName"));

  CatFormat * aCatFormat = new CatFormat("aCatFormat",utils,glob,outputs);

  aCatFormat->inputToFullTree_wgtKNN(glob->GetOptC("inAsciiFiles"),glob->GetOptC("inAsciiVars"),glob->GetOptC("evalTreePostfix"));

  DELNULL(aCatFormat);

  return;
}


// ===========================================================================================================
/**
 * @brief  - Run selected ANNZ procedure (training, optimization, verification or evaluation).
 */
// ===========================================================================================================
void Manager::DoANNZ() {
// ===========================================================================================================
  ANNZ * aANNZ = new ANNZ("aANNZ",utils,glob,outputs);

  if     (glob->GetOptB("doTrain")) aANNZ->Train(); // training
  else if(glob->GetOptB("doOptim")) aANNZ->Optim(); // optimization and performance plots for doRegression
  else if(glob->GetOptB("doVerif")) aANNZ->Optim(); // verification and performance plots for doBinnedCls
  else if(glob->GetOptB("doEval")) {                // evaluation of an input dataset
    CatFormat * aCatFormat = new CatFormat("aCatFormat",utils,glob,outputs);

    if(glob->GetOptB("addInTrainFlag")) {
      // -----------------------------------------------------------------------------------------------------------
      // create root trees from the input ascii files and add a weight branch, calculated with the KNN method
      // -----------------------------------------------------------------------------------------------------------
      aCatFormat->inputToFullTree_wgtKNN(glob->GetOptC("inAsciiFiles"),glob->GetOptC("inAsciiVars"),glob->GetOptC("evalTreePostfix"));
    }
    else {
      // -----------------------------------------------------------------------------------------------------------
      // create root trees from the input dataset
      // -----------------------------------------------------------------------------------------------------------
      aCatFormat->inputToFullTree(glob->GetOptC("inAsciiFiles"),glob->GetOptC("inAsciiVars"),glob->GetOptC("evalTreePostfix"));
    }

    DELNULL(aCatFormat);

    // produce the solution
    aANNZ->Eval();
 }
 DELNULL(aANNZ);

  return;
}


// ===========================================================================================================
/**
 * @brief  - Destructor of Manager.
 */
// ===========================================================================================================
Manager::~Manager() {
// ===========================================================================================================
  aLOG(Log::DEBUG)<<coutWhiteOnBlack<<coutBlue<<" - starting Manager::~Manager() ... "<<coutDef<<endl;

  DELNULL(outputs);
  DELNULL(glob);
  DELNULL(utils);

  return;
}
