from scripts.helperFuncs import *

# command line arguments and basic settings
# --------------------------------------------------------------------------------------------------
init()

# just in case... (may comment this out)
if not glob.annz["doRandomReg"]:
  log.info(red(" - "+time.strftime("%d/%m/%y %H:%M:%S")+" - This scripts is only designed for randomRegression...")) ; sys.exit(0)

# ==================================================================================================
# The main code - randomized regression -
# --------------------------------------------------------------------------------------------------
#   - run the following:
#     python annz_rndReg_advanced.py --randomRegression --genInputTrees
#     python annz_rndReg_advanced.py --randomRegression --train
#     python annz_rndReg_advanced.py --randomRegression --optimize
#     python annz_rndReg_advanced.py --randomRegression --evaluate
# --------------------------------------------------------------------------------------------------
log.info(whtOnBlck(" - "+time.strftime("%d/%m/%y %H:%M:%S")+" - starting ANNZ"))

# --------------------------------------------------------------------------------------------------
# general options which are the same for all stages
#   - PLEASE ALSO INSPECT generalSettings(), WHICH HAS BEEN RUN AS PART OF init(), FOR MORE OPTIONS
# --------------------------------------------------------------------------------------------------
# outDirName - set output directory name
glob.annz["outDirName"] = "test_randReg_advanced"

# nMLMs - the number of random MLMs to generate - for running the example,
# we set nMLMs at a small value, but this should be >~ 50 for production
glob.annz["nMLMs"]        = 10  # 100

# zTrg            - the name of the target variable of the regression
# minValZ,maxValZ - the minimal and maximal values of the target variable (zTrg)
glob.annz["zTrg"]       = "Z"
glob.annz["minValZ"]    = 0.0
glob.annz["maxValZ"]    = 0.8

# set the number of near-neighbours used to compute the KNN error estimator
glob.annz["nErrKNN"]    = 90 # should be around ~100

# the fraction of objects from the training dataset to use for the KNN error estimator. Allowed values are
# fractions within the range [0,1]. Unless the KNN error estimator is slow (the training sample very large),
# it is recommended to use the entire sample for the calculation (i.e., a value of 1), as in:
glob.annz["sampleFrac_errKNN"] = 1

# --------------------------------------------------------------------------------------------------
# pre-processing of the input dataset
# --------------------------------------------------------------------------------------------------
if glob.annz["doGenInputTrees"]:
  # --------------------------------------------------------------------------------------------------
  # - create root input files that ANNZ can run on.
  # - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  #   - inAsciiVars is a paired list with format "TYPE:NAME,TYPE:NAME,TYPE:NAME,TYPE:NAME", where TYPE can take values:
  #       (B-> [boolean]),                        (F-> [32 bit loating point]),           (D-> [64 bit floating point]),          (C->[string])
  #       (S-> [signed   16 bit signed integer]), (I-> [signed   32 bit signed integer]), (L-> [signed   64 bit signed integer]),
  #       (US->[unsigned 16 bit signed integer]), (UI->[unsigned 32 bit signed integer]), (UL->[unsigned 64 bit signed integer]),
  #     and NAME can be any string without spaces (special characters will be replaces...)
  #   - Only double,float,short and int (D,F,S,I) may be used as input variables for the training (but in case of double,
  #     the value is always cast down to float precision...). Other types of inputs may be used as cuts, sig/bck definitions etc.
  #   - Input variable names are not allowed to begin with "ANNZ_" (any permutations of cases)
  #   - If ANNZ is already trained, and just runs on new data using doEval, this format may be different. The only important
  #     point is that the variables used for the training are still available (and have the same TYPE).
  #   - lines from the input files begining with '#' are ignored.
  #   - variables in each line from the input files may be separated by either a white space " " or a comma ",".
  # --------------------------------------------------------------------------------------------------
  # inDirName    - directory in which input files are stored
  glob.annz["inDirName"]    = "examples/data/photoZ/train"
  # inAsciiVars  - list of parameter types and parameter names, corresponding to columns in the input files
  glob.annz["inAsciiVars"]  = "F:MAG_U;F:MAGERR_U;F:MAG_G;F:MAGERR_G;F:MAG_R;F:MAGERR_R;F:MAG_I;F:MAGERR_I;F:MAG_Z;F:MAGERR_Z;D:Z"

  # --------------------------------------------------------------------------------------------------
  #   - For training and testing/validation the input is divided into two (test,train) sub-samples.
  #   - The user needs to define the way to divide the samples
  #     inputs in one of 4 ways (e.g., splitType = "serial", "blocks", "random" or "byInFiles" (default)):
  #       - serial: -> test;train;valid;test;train;valid;test;train;valid;test;train;valid...
  #       - blocks: -> test;test;test;test;train;train;train;train;valid;valid;valid;valid...
  #       - random: -> valid;test;test;train;valid;test;valid;valid;test;train;valid;train...
  #       - separate input files. Must supplay at least one file in splitTypeTrain and one in splitTypeTest.
  # - example use:
  #   set inFileOpt and choose one of the following options for input file configuration:
  # --------------------------------------------------------------------------------------------------
  inFileOpt = 1
  # splitTypeTrain - list of files for training. splitTypeTest - list of files for testing
  if   inFileOpt == 0:
    glob.annz["splitTypeTrain"]  = "boss_dr10_0.csv"
    glob.annz["splitTypeTest"]   = "boss_dr10_1.csv;boss_dr10_2.csv"
  # inAsciiFiles - one list of input files for training and testing, where the the objects are assigned to a given
  # category based on the selection criteria defined by splitType
  elif inFileOpt == 1:
    glob.annz["splitType"]       = "serial" # "serial", "blocks" or "random"
    glob.annz["inAsciiFiles"]    = "boss_dr10_0.csv;boss_dr10_1.csv;boss_dr10_2.csv;boss_dr10_3.csv"
  # example for using a root tree input file, instead of an ascii input (objects split automatically for
  # training and teting)
  elif inFileOpt == 2:
    glob.annz["splitType"]       = "serial" # "serial", "blocks" or "random"
    glob.annz["inTreeName"]      = "ANNZ_tree_full"  
    glob.annz["inAsciiFiles"]    = "ANNZ_tree_full_00000.root"
  # --------------------------------------------------------------------------------------------------
  # examples for using a root tree input file, instead of an ascii input. in this case, we specify the
  # names of the files for training and testing separately, where each has the corresponding name of
  # the input tree. (in the following, the names of input files/trees are just for illustration -
  # they are not included in the example directory...)
  # --------------------------------------------------------------------------------------------------
  # two input files which each contain a root tree of the same name.
  elif False:
    # name of input root file and corresponding tree-name for training
    glob.annz["inTreeNameTrain"] = "tree0"
    glob.annz["splitTypeTrain"]  = "file0.root"
    # name of input root file and corresponding tree-name for testing
    glob.annz["inTreeNameTest"]  = "tree0"
    glob.annz["splitTypeTest"]   = "file1.root"
  # two input files with two distinct corresponding tree names
  elif False:
    # name of input root file and corresponding tree-name for training
    glob.annz["inTreeNameTrain"] = "tree0"
    glob.annz["splitTypeTrain"]  = "file0.root"
    # name of input root file and corresponding tree-name for testing
    glob.annz["inTreeNameTest"]  = "tree1"
    glob.annz["splitTypeTest"]   = "file1.root;file2.root"
  # a single input file list, where each file contains two distinct trees, one for training and one for testing
  elif False:
    glob.annz["inAsciiFiles"]    = "file0.root;file1.root"
    glob.annz["inTreeNameTrain"] = "tree0"
    glob.annz["inTreeNameTest"]  = "tree1"


  else:
    inFileOpt("Unsupported...",False)

  # --------------------------------------------------------------------------------------------------
  # add weights based on the KNN method (see: Cunha et al. (2008), http://arxiv.org/abs/0810.2991v4)
  # --------------------------------------------------------------------------------------------------
  #   - If [useWgtKNN==True] during doGenInputTrees, then these weights will be used for all
  #     subsequent calculations, added to whatever is defined in userWeights_train and userWeights_valid
  # - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  #   - an input file (or file list) is required (inAsciiFiles_wgtKNN), which has the reference
  #     sample from which the weights are derived. The ref' sample has the same variable
  #     structure defined in inAsciiVars_wgtKNN (similar rules as for inAsciiVars), but does not
  #     have to be equal to inAsciiVars. However, both inAsciiVars for the main sample (used for training etc.) 
  #     and inAsciiVars_wgtKNN used for the reference sample must include all variables needed to the weight calculation!
  #     The latter variables are defined in weightVarNames_wgtKNN, these do not necessarily need to correspond to the
  #     variables which are later used for the training (defined in inputVariables).
  # --------------------------------------------------------------------------------------------------
  #   - additional options:
  # - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  #     minNobjInVol_wgtKNN   - the minimal number of neighbours used for the KNN weight calc.
  #     outAsciiVars_wgtKNN   - is a list of variables (subset of inAsciiVars_wgtKNN) which will be written
  #                             out to the output ascii file of the weights, along with the actual weight. This ascii file
  #                             is not used by ANNZ, it is just for the user to inspect.
  #     sampleFracInp_wgtKNN,
  #     sampleFracRef_wgtKNN  - instead of using the entire input/reference sample for the KNN search, a fraction
  #                             of the sample may be used. This is important for large datasets, as the algorithm
  #                             may become very slow if the kd-tree has too many objects. The subsample used
  #                             after this cut is randomely selected from the entire input/reference sample.
  #     weightInp_wgtKNN,
  #     weightRef_wgtKNN      - a possible weight expression used for the kd-tree (each near-neighbour
  #                             will have a weight according to this, where the sum of these weights stands as the
  #                             effective number of near-neighbours in the calculated volume).
  #     cutInp_wgtKNN,
  #     cutRef_wgtKNN         - a possible cut expression used for the kd-tree (defines which entries
  #                             from the reference sample are excluded from the calculation).
  #     inTreeName_wgtKNN     - If inAsciiFiles_wgtKNN deines a root input file instead of an ascii input, use inTreeName_wgtKNN
  #                             to set the name of the tree inside the root input files. If inTreeName_wgtKNN is not set, then
  #                             inTreeName is used. (This may cause a conflict, if inTreeName is set to a different value for
  #                             some input which may be defined in inAsciiFiles for the nominal sample.)
  #     doWidthRescale_wgtKNN - Transform the input variables to the kd-tree to the range [-1,1] (set to True by default)
  # - example use:
  #   set useWgtKNN as [True] to generate and use the weights
  # --------------------------------------------------------------------------------------------------
  useWgtKNN = True
  if useWgtKNN:
    glob.annz["useWgtKNN"]             = True
    glob.annz["minNobjInVol_wgtKNN"]   = 100
    # glob.annz["inAsciiFiles_wgtKNN"]   = "boss_dr10_colorCuts.csv"
    # glob.annz["inAsciiVars_wgtKNN"]    = "F:MAG_U;F:MAGERR_U;F:MAG_G;F:MAGERR_G;F:MAG_R;F:MAGERR_R;F:MAG_I;F:MAGERR_I;F:MAG_Z;F:MAGERR_Z;D:Z"
    glob.annz["inAsciiFiles_wgtKNN"]   = "boss_dr10_0_large_magCut_noZ.csv"
    glob.annz["inAsciiVars_wgtKNN"]    = "F:MAG_U;F:MAGERR_U;F:MAG_G;F:MAGERR_G;F:MAG_R;F:MAGERR_R;F:MAG_I;F:MAGERR_I;F:MAG_Z;F:MAGERR_Z"
    
    # some random weird choice for [weightInp_wgtKNN, weightRef_wgtKNN] in this example, just to get different
    # distributions for the input and reference samples, so that we have something to calculate weights for...
    glob.annz["weightInp_wgtKNN"]      = "1/pow(MAG_G*MAG_U*MAG_R*MAG_I, 1.5)"
    glob.annz["weightRef_wgtKNN"]      = "1/MAGERR_G"

    glob.annz["weightVarNames_wgtKNN"] = "MAG_U;MAG_G;MAG_R;MAG_I;MAG_Z"

    # it is possible to use general mathematical expressions which include any of the input
    # variables defined in "inAsciiVars_wgtKNN", as for example, the following silly choice:
    # glob.annz["weightVarNames_wgtKNN"] = "(MAG_U-MAG_G)*MAG_I/MAG_Z;MAG_U;MAG_G;MAG_R;MAG_I;MAG_Z"

    # optional parameters (may leave empty as default value):
    glob.annz["sampleFracInp_wgtKNN"]  = 0.99                                          # fraction of dataset to use (positive number, smaller or equal to 1)
    glob.annz["sampleFracRef_wgtKNN"]  = 0.95                                          # fraction of dataset to use (positive number, smaller or equal to 1)
    glob.annz["outAsciiVars_wgtKNN"]   = "MAG_U;MAG_G;MAGERR_U"                        # write out two additional variables to the output file
    glob.annz["weightRef_wgtKNN"]      = "(MAGERR_R<0.7)*1 + (MAGERR_R>=0.7)/MAGERR_R" # down-weight objects with high MAGERR_R
    glob.annz["cutRef_wgtKNN"]         = "MAGERR_U<200"                                # only use objects which have small MAGERR_U
    glob.annz["doWidthRescale_wgtKNN"] = True

    # - trainTestTogether_wgtKNN
    #   by default, the weights are computed for the entire sample [trainTestTogether_wgtKNN = True].
    #   That is, the training and the testing samples are used together - we calculate the difference between the
    #   distribution of input-variables between [train+test samples] and [ref sample]. However, it is possible to
    #   decide to comput the weights for each separately. That is, to calculate wegiths for [train sample]
    #   with regards to [ref sample], and to separately get [test sample] with regards to [ref sample]. The latter
    #   is only recommended if the training and testing samples have different inpput-variable distributions.
    glob.annz["trainTestTogether_wgtKNN"] = False

    # example for using a root file as input, instead of an ascii input:
    useRootInputFile = False
    if useRootInputFile:
      glob.annz["inAsciiFiles_wgtKNN"]   = "ANNZ_tree_full_00000.root"
      glob.annz["inTreeName_wgtKNN"]     = "ANNZ_tree_full"  


  # run ANNZ with the current settings
  runANNZ()

# --------------------------------------------------------------------------------------------------
# training
# --------------------------------------------------------------------------------------------------
if glob.annz["doTrain"]:
  # for each MLM, run ANNZ
  for nMLMnow in range(glob.annz["nMLMs"]): 
    glob.annz["nMLMnow"] = nMLMnow
    if glob.annz["trainIndex"] >= 0 and glob.annz["trainIndex"] != nMLMnow: continue

    # --------------------------------------------------------------------------------------------------
    # inputVariables - 
    # - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    #   - semicolon-separated list of input variables for the MLMs. Can include math expressions of the variables
    #     given in inAsciiVars (see https://root.cern.ch/root/html520/TFormula.html for examples of valid math expressions).
    #     logical expressions are also valid, as e.g., (MAG_G*(MAGERR_G<1) + 27*(MAGERR_G>=1))
    #     inputVariables can be different for any given MLM (any value of nMLMnow).
    # - example use:
    # --------------------------------------------------------------------------------------------------
    if   nMLMnow%3 == 0: glob.annz["inputVariables"] = "MAG_U;MAG_G;MAG_R;MAG_I;MAG_Z"
    elif nMLMnow%3 == 1: glob.annz["inputVariables"] = "MAG_U;MAG_G;(MAG_G-MAG_R);(MAG_R-MAG_I);(MAG_I-MAG_Z)"
    elif nMLMnow%3 == 2: glob.annz["inputVariables"] = "MAG_U;MAG_G;(MAG_G-MAG_R);(MAG_R-MAG_I);(MAG_I-MAG_Z);pow(MAG_G-MAG_Z,2);"

    # --------------------------------------------------------------------------------------------------
    # inputVarErrors -
    # - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    #   - optional input parameter errors. By default [inputVarErrors=""], and the errors on MLMs are derived
    #     in-situ from the training dataset using the KNN-error method. However, it is possible (not recomended)
    #     to set inputVarErrors and propagate the error directly - see ANNZ::getRegClsErrINP() for details.
    # - example use:
    # --------------------------------------------------------------------------------------------------
    if nMLMnow == 0: glob.annz["inputVarErrors"] = "MAGERR_U;MAGERR_G;MAGERR_R;MAGERR_I;MAGERR_Z"
    else:            glob.annz["inputVarErrors"] = "" # generate the errors on MLM using the KNN-error method

    # --------------------------------------------------------------------------------------------------
    # userMLMopts,rndOptTypes -
    # - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    #   - the actual type and options of the MLMs may be randomized internally by ANNZ (for BDTs and/or ANNs),
    #     if we set [userMLMopts=""]. allowed types are given by rndOptTypes.
    #   - Optionally, it is possible to set for a given nMLMnow specific options instead.
    #     this can be done for by hand (see nMLMnow=0,1,2 below), or using some randmonization
    #     scheme. the latter is implemented for example in the genRndOpts() python routine (nMLMnow=3 below).
    # --------------------------------------------------------------------------------------------------
    #   - MLM types:
    # - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    #     - the available types (as listed in ANNZ::setupTypesTMVA()) are:
    #         CUTS,Likelihood,PDERS,HMatrix,Fisher,KNN,CFMlpANN,TMlpANN,BDT,DT,RuleFit,SVM,
    #         ANN,BayesClassifier,FDA,Boost,PDEFoam,LD,Plugins,Category,MaxMethod
    # --------------------------------------------------------------------------------------------------
    #   - configuration oftions for a given MLM type:
    # - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    #     - The list of available options is given in http://tmva.sourceforge.net/optionRef.html and in the TMVA manual.
    #       all TMVA options for the various methods are allowed.
    #       The only ANNZ requirements are that the option string contains
    #       the type of MLM [ANNZ_MLM=???]. Also, it is allowed to specify a global normalization, which normaly in
    #       TMVA is given as part of the TMVA::Factory options.
    #     - for example one may set:
    #         glob.annz["userMLMopts"] = "ANNZ_MLM=BDT:VarTransform=Norm:NTrees=110:NormMode=NumEvents:BoostType=AdaBoost:" 
    #       where "ANNZ_MLM=BDT" and "NormMode=NumEvents" are the two options which are not nominally part of TMVA
    # --------------------------------------------------------------------------------------------------
    #   - transformations (numerical value of inputs to training):
    # - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    #     - transformations are available for a given MLM by using e.g., "VarTransform=P". For instance:
    #         D_Background, P_Signal, G, N_AllClasses
    #       stand for Decorrelation, PCA-transformation, Gaussianisation, Normalisation,
    #       each are for some given class of events; 'AllClasses' , 'Background' denotes all events of all classes,
    #       or just for background objects (in cases of classification, where signal and background are defined).
    #       if no class indication is given, 'All' is assumed.
    #     - the transformations are addetive (one may book several for a given MLM).
    #     - if the user of ANNZ does not include "VarTransform=" in userMLMopts, a transformation is chosen randomely.
    #       to prevent any transformation, use "VarTransform=None"
    #     - it is also possible to set the global variable "transANNZ" for all MLMs, as e.g.,
    #         glob.annz["transANNZ"] = "P"
    #       (by default glob.annz["transANNZ"] = "I" for the iddentity trans -> has not affect).
    # --------------------------------------------------------------------------------------------------
    #   - factory normalization (number of objects):
    # - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    #     factory normalization is the number of input objects in different sub-samples -> global reweighting of samples.
    #     by default, if (alwaysUseNormalization==true), we use (NormMode=EqualNumEvents)
    #     the user can specify explicitly (NormMode=NumEvents) or (NormMode=EqualNumEvents), or choose not to
    #     use normalization by setting (alwaysUseNormalization==false) and not setting NormMode.
    #     it is recomended to always use some kind of normalization, so best to leave this at the default setting.
    # --------------------------------------------------------------------------------------------------
    if   nMLMnow == 0: glob.annz["userMLMopts"] = "ANNZ_MLM=BDT:VarTransform=N:NTrees=110:NormMode=NumEvents:BoostType=AdaBoost:"
    elif nMLMnow == 1: glob.annz["userMLMopts"] = "ANNZ_MLM=KNN"
    elif nMLMnow == 2: glob.annz["userMLMopts"] = "ANNZ_MLM=BDT:NTrees=190:VarTransform=N,P"
    elif nMLMnow == 3: glob.annz["userMLMopts"] = genRndOpts(nMLMnow)
    else:              glob.annz["userMLMopts"] = ""

    # rndOptTypes - generate automatically these randomized MLM types for cases
    #               where [userMLMopts = ""] (currently "ANN", "BDT" or "ANN_BDT" are supported)
    # --------------------------------------------------------------------------------------------------
    glob.annz["rndOptTypes"] = "BDT"

    # --------------------------------------------------------------------------------------------------
    # userCuts_train,userCuts_valid,userWeights_train,userWeights_valid -
    # - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    #   - define cuts and weights for training and/or validation for any given nMLMnow.
    #     - cuts are logical expressions that define acceptance criterial (e.g., objects which fail
    #       userCuts_train are excluded from the training).
    #       For instance, 
    #         glob.annz["userCuts_train"] = "MAG_G < 24"
    #       means that only objects which have G-band magnitude lower than 24 are accepted for training.
    #     - weights are used to enhance or reduce the significance of objects by some numerical value.
    #       weights may include logical expressions, to affect different subsets of the dataset.
    #   - cuts and weights defined during training can not be changed in optimization/evaluation. However,
    #     the validation cuts/weights can be different for training and for optimization/evaluation,
    #     determined by modify_userCuts_valid and modify_userWeights_valid.
    #     validation weights are not used directly during training, however, this is the place to
    #     specify any MLM specific weights. During optimization one can only set global weigts for all MLMs.
    # - example use:
    # --------------------------------------------------------------------------------------------------
    if nMLMnow == 0:
      # set some nonsensical cuts and weights for illustration for one of the MLMs:
      glob.annz["userCuts_train"]    = "MAG_G < 24"
      glob.annz["userCuts_valid"]    = "(MAGERR_I < 0.45) && ((MAG_U > 19) || (MAG_Z > 17))"
      glob.annz["userWeights_train"] = "(MAG_R > 22)/MAG_R + (MAG_R <= 22)*1"
      glob.annz["userWeights_valid"] = "(MAG_G < 20)*MAG_G + (MAG_G >= 20)*1"
    else:
      glob.annz["userCuts_train"]    = glob.annz["userCuts_valid"]    = ""
      glob.annz["userWeights_train"] = glob.annz["userWeights_valid"] = ""

    # --------------------------------------------------------------------------------------------------
    # bias-correction procedure on MLMs -
    # - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    #   - doBiasCorMLM      - whether or not to perform the correction for MLMs (during training)
    #   - biasCorMLMopt     - MLM configuration options for the bias-correction for MLMs - simple structures are recommended !!!
    #                         - can take the same format as userMLMopts (e.g., [biasCorMLMopt="ANNZ_MLM=BDT:VarTransform=N:NTrees=100"])
    #                         - can be empty (then the job options will be automatically generated, same as is setting [userMLMopts=""])
    #                         - can be set as [biasCorMLMopt="same"], then the same configuration options as for the nominal MLM
    #                           for which the bias-correction is applied are used
    #                       - simple MLMs are recommended, e.g.: 
    #                         - BDT with around 50-100 trees:
    #                           "ANNZ_MLM=BDT:VarTransform=N:NTrees=100:BoostType=AdaBoost"
    #                         - ANN with a simple layer structure, not too many NCycles etc.:
    #                           "ANNZ_MLM=ANN::HiddenLayers=N,N+5:VarTransform=N,P:TrainingMethod=BFGS:TestRate=5:NCycles=500:UseRegulator=True"
    #   - biasCorMLMwithInp - add the nominal MLM as an input variable for the new MLM of the bias-correction (not necessary, as it
    #                         may just add noise)
    #   - alwaysKeepBiasCor - whether or not to not check the KS-test and N_poiss metrics for improvement in order to
    #                         possibly reject the bias correction (check performed if [alwaysKeepBiasCor] is set to True)
    # - example use:
    # -----------------------------------------------------------------------------------------------------------
    doBiasCorMLM = True
    if doBiasCorMLM:
      glob.annz["doBiasCorMLM"]      = True
      glob.annz["biasCorMLMwithInp"] = False
      glob.annz["alwaysKeepBiasCor"] = False
      # as an example, a couple of choices of MLM options (this shouldn't matter much though...)
      if nMLMnow % 2 == 0:
        glob.annz["biasCorMLMopt"]   = "ANNZ_MLM=BDT:VarTransform=N:NTrees=50:BoostType=AdaBoost"
      else:
        glob.annz["biasCorMLMopt"]   = "ANNZ_MLM=ANN:HiddenLayers=N+5:VarTransform=N,P:TrainingMethod=BFGS:NCycles=500:UseRegulator=True"

    # run ANNZ with the current settings
    runANNZ()


# --------------------------------------------------------------------------------------------------
# optimization and evaluation
# --------------------------------------------------------------------------------------------------
if glob.annz["doOptim"] or glob.annz["doEval"]:

  # --------------------------------------------------------------------------------------------------
  # nPDFs - number of PDFs (up to three PDF types are implemented for randomized regression):
  #   - PDFs are selected by choosing the weighting scheme which is "most compatible" with the true value.
  #     This is determined in several ways (generating alternative PDFs), using cumulative distributions; for the first/second
  #     PDFs (PDF_0 and PDF_1 in the output), the cumulative distribution is based on the "truth" (the target variable, zTrg).
  #     For the third PDF (PDF_2 in the output), the cumulative distribution is based on the "best" MLM.
  #     For the former, a set of templates, derived from zTrg is used to fit the dataset. For the later,
  #     a flat distribution of the cumulator serves as the baseline.
  #   - nominally, only PDF_0 should be used. PDF_1/PDF_2 are depracated, and are not guaranteed to
  #     be supported in the future. They may currently still be generated by setting addOldStylePDFs to True
  # --------------------------------------------------------------------------------------------------
  glob.annz["nPDFs"] = 1

  speedUpMCMC = False
  if speedUpMCMC:
    # max_optimObj_PDF - can be set to some number (smaller than the size of the training sample). the
    # result will be to limit the number of objects used in each step of the MCMC used to derive the PDF
    glob.annz["max_optimObj_PDF"] = 2500
    # nOptimLoops - set the maximal number of steps taken by the MCMC
    # note that the MCMC will likely end before `nOptimLoops` steps in any case. this will happen
    # after a pre-set number of steps, during which the solution does not improve.
    glob.annz["nOptimLoops"] = 5000

  # add the depricated PDFs to the output
  getOldStylePdfs = False
  if getOldStylePdfs:
    # addOldStylePDFs - set to get the old-style PDFs (which were the default up to v2.2.2) as PDF_1 and PDF_2
    glob.annz["addOldStylePDFs"] = True
    # set to 2 or 3, respectively to add [PDF_1] or [PDF_1 and PDF_2] to the output
    glob.annz["nPDFs"]           = 3

  # --------------------------------------------------------------------------------------------------
  # definition of the PDF bins -
  #   - in optimization: the PDF bins are used only for the performance plots (not for the actual PDF
  #                      derivation), and determine the format of the output ascii file for the validation dataset.
  #   - in evaluation:   the PDF bins are used to generate the final PDF in the output ascii file for the
  #                      evaluation dataset.
  # - example use:
  #   set pdfBinsType as [1,2] or [0] to choose fixed- or user-defined- PDF bins, respectively.
  #   For fixed-width bins, it is possible to define either the number of bins (nPDFbins) or the
  #   width of each bin (pdfBinWidth). (Only one of [nPDFbins,pdfBinWidth] can be defined at a time.)
  # --------------------------------------------------------------------------------------------------
  pdfBinsType = 1
  if pdfBinsType == 0:
    # use a pre-defined set of PDF bins
    glob.annz["userPdfBins"] = "0.0;0.1;0.2;0.24;0.3;0.52;0.6;0.7;0.8"
  elif pdfBinsType == 1:
    # nPDFbins - number of PDF bins (equal width bins between minValZ and maxValZ- automatically derive pdfBinWidth)
    glob.annz["nPDFbins"]    = 120
  elif pdfBinsType == 2:
    # pdfBinWidth - width of each PDF bin (equal width bins between minValZ and maxValZ - automatically derive nPDFbins)
    glob.annz["pdfBinWidth"] = 0.01

  # --------------------------------------------------------------------------------------------------
  # modify_userCuts_valid,modify_userWeights_valid -
  # - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  #   - during optimization, it is possible to change the cuts/weights for validation which were
  #     defined during training. It is possible to either "ADD" (combine a new expression with the
  #     existing one), to "OVERWRITE" (echange the new expression with the old one), or "IGNORE" (keep
  #     the original definition used during traiing and ignore e.g., the curent value of userCuts_valid).
  #     by default, both modify_userCuts_valid and modify_userWeights_valid are set to "ADD", but
  #     if userCuts_valid and userWeights_valid are lef empty, there will be no affect.
  # - example use (choose changeCutsWeights):
  #   (notice the use of TMath::Abs() - see: https://root.cern.ch/root/html/TMath.html for available functions.)
  # --------------------------------------------------------------------------------------------------
  changeCutsWeights = 1
  if changeCutsWeights == 1:
    glob.annz["modify_userCuts_valid"]    = "ADD"
    glob.annz["modify_userWeights_valid"] = "ADD"
    glob.annz["userCuts_valid"]           = "(MAGERR_R < 1)"
    glob.annz["userWeights_valid"]        = "(MAG_G < 22)*1 + (MAG_G >= 22)*(TMath::Abs(sin(MAG_R)))"
  elif changeCutsWeights == 2:
    glob.annz["modify_userCuts_valid"]    = "OVERWRITE"
    glob.annz["userCuts_valid"]           = "(MAGERR_R < 1)"
    glob.annz["userWeights_valid"]        = "(MAG_G < 22)"
  elif changeCutsWeights == 3:
    glob.annz["modify_userCuts_valid"]    = "IGNORE"
    glob.annz["userCuts_valid"]           = "(MAGERR_R < 1)"

  # ==================================================================================================
  # MLMsToStore - 
  # - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  #   - which MLMs to write to output (in addition to the "best MLM" and the PDFs).
  #     - ""              - non
  #     - "ALL"           - write all trained MLMs
  #     - "LIST;0;1;3;55" - specific list of MLM indices (sepaated by ';') to write out
  # --------------------------------------------------------------------------------------------------
  glob.annz["MLMsToStore"] = "" 
  
  # --------------------------------------------------------------------------------------------------
  # addOutputVars
  #   - add the following variables (extracted from the input file) to the output
  #     in addition to the regression target (zTrg) (usefull e.g., for matched object-IDs)
  # --------------------------------------------------------------------------------------------------
  glob.annz["addOutputVars"] = "MAG_U;MAGERR_I"

  # --------------------------------------------------------------------------------------------------
  # max_sigma68_PDF, max_bias_PDF, max_frac68_PDF
  #   - if max_sigma68_PDF, max_bias_PDF are positive, they put thresholds on the maximal value of
  #     the scatter (max_sigma68_PDF), bias (max_bias_PDF) or outlier-fraction (max_frac68_PDF) of an
  #     MLM which may be included in the PDF created in randomized regression
  # --------------------------------------------------------------------------------------------------
  glob.annz["max_sigma68_PDF"] = 0.044
  glob.annz["max_bias_PDF"]    = 0.01
  glob.annz["max_frac68_PDF"]  = 0.10


  # --------------------------------------------------------------------------------------------------
  # optimization
  # --------------------------------------------------------------------------------------------------
  if glob.annz["doOptim"]:
    # run ANNZ with the current settings
    runANNZ()

  # --------------------------------------------------------------------------------------------------
  # evaluation
  # --------------------------------------------------------------------------------------------------
  elif glob.annz["doEval"]:

    # inDirName,inAsciiFiles - directory with files to make the calculations from, and list of input files
    glob.annz["inDirName"]      = "examples/data/photoZ/eval/"
    glob.annz["inAsciiFiles"]   = "boss_dr10_eval1_noZ.csv"
    # inAsciiVars - list of parameters in the input files (doesnt need to be exactly the same as in doGenInputTrees, but must contain all
    #               of the parameers which were used for training)
    glob.annz["inAsciiVars"]    = "F:MAG_U;F:MAGERR_U;F:MAG_G;F:MAGERR_G;F:MAG_R;F:MAGERR_R;F:MAG_I;F:MAGERR_I;F:MAG_Z;F:MAGERR_Z"
    # evalDirPostfix - if not empty, this string will be added to the name of the evaluation directory
    #                  (can be used to prevent multiple evaluation of different input files from overwriting each other)
    glob.annz["evalDirPostfix"] = "nFile0"

    # --------------------------------------------------------------------------------------------------
    # addInTrainFlag, minNobjInVol_inTrain, maxRelRatioInRef_inTrain -
    # - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    #   - addInTrainFlag           - calculate for each object which is evaluated, if it is "close" in the
    #                                input-parameter space to the training dataset. The result is written as part of the evaluation
    #                                output, as an additional parameter named "inTrainFlag". The value of "inTrainFlag"
    #                                is zero if the object is not "close" to the training objects (therefore probably has unreliable result).
    #                                The calculation is performed using a KNN approach, similar to the algorithm used for
    #                                the [glob.annz["useWgtKNN"] = True] calculation.
    #   - minNobjInVol_inTrain     - The number of reference objects in the reference dataset which are used in the calculation.
    #   - maxRelRatioInRef_inTrain - Nominally [maxRelRatioInRef_inTrain = -1], but can also be
    #                                a number in the range, [0,1] - This is the minimal threshold of the relative
    #                                difference between distances in the inTrainFlag calculation for accepting an object.
    #                                If positive, it should probably be a (<0.5) positive number. If [maxRelRatioInRef_inTrain < 0],
    #                                then this number is ignored, and the "inTrainFlag" flag becomes
    #                                a floating-point number in the range [0,1], instead of a binary flag.
    #   - ...._inTrain             - The rest of the parameters ending with "_inTrain" have a similar role as
    #                                their "_wgtKNN" counterparts, which are used with [glob.annz["useWgtKNN"] = True]. These are:
    #                                - "outAsciiVars_inTrain", "weightInp_inTrain", "cutInp_inTrain",
    #                                  "cutRef_inTrain", "sampleFracInp_inTrain" and "sampleFracRef_inTrain"
    # --------------------------------------------------------------------------------------------------
    addInTrainFlag = False
    if addInTrainFlag:
      glob.annz["inAsciiFiles"]             = "boss_dr10_eval0_noZ.csv" # in this case, choose a larger input file
      glob.annz["addInTrainFlag"]           = True
      glob.annz["minNobjInVol_inTrain"]     = 100
      glob.annz["maxRelRatioInRef_inTrain"] = -1
      glob.annz["weightVarNames_inTrain"]   = "MAG_U;MAG_G;MAG_R;MAG_I;MAG_Z"
      # glob.annz["weightRef_inTrain"]        = "(MAG_Z<20.5 && MAG_R<22 && MAG_U<24)" # cut the reference sample, just to have some difference...

    # run ANNZ with the current settings
    runANNZ()

log.info(whtOnBlck(" - "+time.strftime("%d/%m/%y %H:%M:%S")+" - finished running ANNZ !"))

