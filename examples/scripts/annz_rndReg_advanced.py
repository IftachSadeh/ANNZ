from annz.helperFuncs import *

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
  #   - For training and testing/validation the input is divided into two (test,train) or into three (test,train,valid)
  #     sub-samples.
  #   - The user needs to define the number of sub-samples (e.g., nSplit = 1,2 or 3) and the way to divide the
  #     inputs in one of 4 ways (e.g., splitType = "serial", "blocks", "random" or "byInFiles" (default)):
  #       - serial: -> test;train;valid;test;train;valid;test;train;valid;test;train;valid...
  #       - blocks: -> test;test;test;test;train;train;train;train;valid;valid;valid;valid...
  #       - random: -> valid;test;test;train;valid;test;valid;valid;test;train;valid;train...
  #       - separate input files. Must supplay at least one file in splitTypeTrain and one in splitTypeTest.
  #         In this case, [nSplit = 2]. Optionally can set [nSplit = 3] and provide a list of files in "splitTypeValid" as well.
  #   - It is possible to use root input files instead of ascii inputs. In this case, use the "splitTypeTrain", "splitTypeTest",
  #     "splitTypeValid" and "inAsciiFiles" variables in the same way as for ascii inputs, but in addition, specify the name of the
  #     tree inside the root files, as the variable "inTreeName". Make sure not to mix ascii and root input files!
  # - example use:
  #   set inFileOpt and choose one of the following options for input file configuration:
  # --------------------------------------------------------------------------------------------------
  inFileOpt = 1
  # splitTypeTrain - list of files for training. splitTypeTest - list of files for testing and validation
  if   inFileOpt == 0:
    glob.annz["nSplit"]         = 2
    glob.annz["splitTypeTrain"] = "boss_dr10_0.csv"
    glob.annz["splitTypeTest"]  = "boss_dr10_1.csv;boss_dr10_2.csv"
  # splitTypeTrain - list of files for training. splitTypeTest - list of files for testing. splitTypeValid - list of files for validation
  elif inFileOpt == 1:
    glob.annz["nSplit"]         = 3
    glob.annz["splitTypeTrain"] = "boss_dr10_0.csv"
    glob.annz["splitTypeTest"]  = "boss_dr10_1.csv;boss_dr10_2.csv"
    glob.annz["splitTypeValid"] = "boss_dr10_3.csv"
  # inAsciiFiles - one list of input files for training, testing and validation, where the the objects are assigned to a given
  # category based on the selection criteria defined by splitType
  elif inFileOpt == 2:
    glob.annz["nSplit"]         = 3
    glob.annz["splitType"]      = "serial" # "serial", "blocks" or "random"
    glob.annz["inAsciiFiles"]   = "boss_dr10_0.csv;boss_dr10_1.csv;boss_dr10_2.csv;boss_dr10_3.csv"
  # example ofr using a root tree input file, instead of an ascii input
  elif inFileOpt == 3:
    glob.annz["nSplit"]         = 2
    glob.annz["splitType"]      = "serial" # "serial", "blocks" or "random"
    glob.annz["inTreeName"]     = "ANNZ_tree_full"
    glob.annz["inAsciiFiles"]   = "ANNZ_tree_full_00000.root"
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
    glob.annz["minNobjInVol_wgtKNN"]   = 50
    glob.annz["inAsciiFiles_wgtKNN"]   = "boss_dr10_colorCuts.csv"
    glob.annz["inAsciiVars_wgtKNN"]    = glob.annz["inAsciiVars"]
    glob.annz["weightVarNames_wgtKNN"] = "MAG_U;MAG_G;MAG_R;MAG_I;MAG_Z"

    # optional parameters (may leave empty as default value):
    glob.annz["sampleFracInp_wgtKNN"]  = 0.15                                          # fraction of dataset to use (positive number, smaller or equal to 1)
    glob.annz["sampleFracRef_wgtKNN"]  = 0.95                                          # fraction of dataset to use (positive number, smaller or equal to 1)
    glob.annz["outAsciiVars_wgtKNN"]   = "MAG_U;MAG_G;MAGERR_U"                        # write out two additional variables to the output file
    glob.annz["weightRef_wgtKNN"]      = "(MAGERR_R<0.7)*1 + (MAGERR_R>=0.7)/MAGERR_R" # down-weight objects with high MAGERR_R
    glob.annz["cutRef_wgtKNN"]         = "MAGERR_U<200"                                # only use objects which have small MAGERR_U

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
    elif nMLMnow == 1: glob.annz["userMLMopts"] = "ANNZ_MLM=SVM:MaxIter=700"
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

    # run ANNZ with the current settings
    runANNZ()


# --------------------------------------------------------------------------------------------------
# optimization and evaluation
# --------------------------------------------------------------------------------------------------
if glob.annz["doOptim"] or glob.annz["doEval"]:

  # --------------------------------------------------------------------------------------------------
  # nPDFs - number of PDFs (up to two PDF types are implemented for randomized regression):
  #   - PDFs are selected by choosing the weighting scheme which is "most compatible" with the true value.
  #     This is determined in two ways (generating two alternative PDFs), using cumulative distributions; for the first
  #     PDF (PDF_0 in the output), the cumulative distribution is based on the "truth" (the target variable, zTrg). For the second
  #     PDF (PDF_1 in the output), the cumulative distribution is based on the "best" MLM.
  #     For the former, a set of templates, derived from zTrg is used to fit the dataset. For the later,
  #     a flat distribution of the cumulator serves as the baseline.
  # --------------------------------------------------------------------------------------------------
  glob.annz["nPDFs"]      = 2

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
    glob.annz["nPDFbins"]    = 90
  elif pdfBinsType == 2:
    # pdfBinWidth - width of each PDF bin (equal width bins between minValZ and maxValZ - automatically derive nPDFbins)
    glob.annz["pdfBinWidth"] = 0.1 

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
  #   - if max_sigma68_PDF,max_bias_PDF are positive, they put thresholds on the maximal value of
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
    #   - maxRelRatioInRef_inTrain - Nominally, a number in the range, [0,1] - The minimal threshold of the relative difference between
    #                                distances in the inTrainFlag calculation for accepting an object - Should be a (<0.5) positive number.
    #                                If [maxRelRatioInRef_inTrain < 0] then this number is ignored, and the "inTrainFlag" flag becomes
    #                                a floating-point number in the range [0,1], instead of a binary flag.
    #   - ...._inTrain             - The rest of the parameters ending with "_inTrain" have a similar role as
    #                                their "_wgtKNN" counterparts, which are used with [glob.annz["useWgtKNN"] = True]. These are:
    #                                - "outAsciiVars_inTrain", "weightInp_inTrain", "cutInp_inTrain",
    #                                  "cutRef_inTrain", "sampleFracInp_inTrain" and "sampleFracRef_inTrain"
    # --------------------------------------------------------------------------------------------------
    addInTrainFlag = False
    if addInTrainFlag:
      glob.annz["addInTrainFlag"]           = True
      glob.annz["minNobjInVol_inTrain"]     = 100
      glob.annz["maxRelRatioInRef_inTrain"] = 0.1
      glob.annz["weightVarNames_inTrain"]   = "MAG_U;MAG_G;MAG_R;MAG_I;MAG_Z"
      # glob.annz["weightRef_inTrain"]        = "(MAG_Z<20.5 && MAG_R<22 && MAG_U<24)" # cut the reference sample, just to have some difference...

    # run ANNZ with the current settings
    runANNZ()

log.info(whtOnBlck(" - "+time.strftime("%d/%m/%y %H:%M:%S")+" - finished runing ANNZ !"))

