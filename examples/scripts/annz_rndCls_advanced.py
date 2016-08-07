from scripts.helperFuncs import *

# command line arguments and basic settings
# --------------------------------------------------------------------------------------------------
init()

# just in case... (may comment this out)
if not glob.annz["doRandomCls"]:
  log.info(red(" - "+time.strftime("%d/%m/%y %H:%M:%S")+" - This scripts is only designed for randomClassification...")) ; sys.exit(0)

# ==================================================================================================
# The main code - randomized classification -
# --------------------------------------------------------------------------------------------------
#   - run the following:
#     python annz_rndCls_advanced.py --randomClassification --genInputTrees
#     python annz_rndCls_advanced.py --randomClassification --train
#     python annz_rndCls_advanced.py --randomClassification --optimize
#     python annz_rndCls_advanced.py --randomClassification --evaluate
# --------------------------------------------------------------------------------------------------
log.info(whtOnBlck(" - "+time.strftime("%d/%m/%y %H:%M:%S")+" - starting ANNZ"))

# --------------------------------------------------------------------------------------------------
# general options which are the same for all stages
#   - PLEASE ALSO INSPECT generalSettings(), WHICH HAS BEEN RUN AS PART OF init(), FOR MORE OPTIONS
# --------------------------------------------------------------------------------------------------
# outDirName - set output directory name
glob.annz["outDirName"]   = "test_randCls_advanced"

# nMLMs - the number of random MLMs to generate - for running the example,
# we set nMLMs at a small value, but this should be >~ 50 for production
glob.annz["nMLMs"]        = 10  # 100

# --------------------------------------------------------------------------------------------------
# the definition of the signal and background classes - must define at least one of the
# pairs [userCuts_sig and userCuts_bck] or [inpFiles_sig and inpFiles_bck], though can
# optionally also use three or all four of the latter together
# --------------------------------------------------------------------------------------------------
defSigBckCuts = 0
if   defSigBckCuts == 0:
  # --------------------------------------------------------------------------------------------------
  # define cuts based on existing parameters in the input files (in this example, the parameter 'type')
  # --------------------------------------------------------------------------------------------------
  glob.annz["userCuts_sig"] = "type == 3" # in this example, these are galaxies
  glob.annz["userCuts_bck"] = "type == 6" # in this example, these are stars
elif defSigBckCuts == 1:
  # --------------------------------------------------------------------------------------------------
  # define a given input file as containing only signal or only background objects instead of
  # explicitly setting userCuts_sig and userCuts_bck.
  # --------------------------------------------------------------------------------------------------
  glob.annz["inpFiles_sig"]  = "sgCatalogue_galaxy_0.txt;sgCatalogue_galaxy_1.txt;sgCatalogue_galaxy_2.txt"
  glob.annz["inpFiles_bck"]  = "sgCatalogue_star_0.txt;sgCatalogue_star_1.txt;sgCatalogue_star_3.txt"
elif defSigBckCuts == 2:
  # --------------------------------------------------------------------------------------------------
  # define a given input file as containing only signal or only background objects and in addition
  # explicitly set a cut on (in this example) only background objects.
  # --------------------------------------------------------------------------------------------------
  glob.annz["inpFiles_sig"]  = "sgCatalogue_galaxy_0.txt;sgCatalogue_galaxy_1.txt;sgCatalogue_galaxy_2.txt"
  glob.annz["inpFiles_bck"]  = "sgCatalogue_star_0.txt;sgCatalogue_star_1.txt;sgCatalogue_star_3.txt"
  glob.annz["userCuts_bck"]  = "mE2_r < 0.6" # may also use "inpFiles_sig" instead or in addition

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
  glob.annz["inDirName"]    = "examples/data/sgSeparation/train"
  # inAsciiVars  - list of parameter types and parameter names, corresponding to columns in the input files
  glob.annz["inAsciiVars"]  = "C:class; F:z; UL:objid; F:psfMag_r; F:fiberMag_r; F:modelMag_r; F:petroMag_r; F:petroRad_r; F:petroR50_r; " \
                            + " F:petroR90_r; F:lnLStar_r; F:lnLExp_r; F:lnLDeV_r; F:mE1_r; F:mE2_r; F:mRrCc_r; I:type_r; I:type"

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
  inFileOpt = 0
  # splitTypeTrain - list of files for training. splitTypeTest - list of files for testing
  if   inFileOpt == 0:
    glob.annz["splitTypeTrain"] = "sgCatalogue_galaxy_0.txt;sgCatalogue_star_0.txt"
    glob.annz["splitTypeTest"]  = "sgCatalogue_galaxy_1.txt;sgCatalogue_star_1.txt"
  # inAsciiFiles - one list of input files for training and testing, where the the objects are assigned to a given
  # category based on the selection criteria defined by splitType
  elif inFileOpt == 1:
    glob.annz["splitType"]      = "random" # "serial", "blocks" or "random"
    glob.annz["inAsciiFiles"]   = "sgCatalogue_galaxy_0.txt;sgCatalogue_galaxy_1.txt;sgCatalogue_star_0.txt;sgCatalogue_star_1.txt"
  else:
    inFileOpt("Unsupported...",False)

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
    #     logical expressions are also valid, as e.g., (TMath::Log(pow(mE1_r,2))*(mE1_r>0) + mE1_r*(mE1_r <=0))
    #     inputVariables can be different for any given MLM (any value of nMLMnow).
    # - example use:
    # --------------------------------------------------------------------------------------------------
    if   nMLMnow%2 == 0:
      glob.annz["inputVariables"]  = "psfMag_r; fiberMag_r;modelMag_r ;  petroMag_r; petroRad_r; petroR50_r;petroR90_r;" \
                                   + "lnLStar_r;lnLExp_r;lnLDeV_r;(mE1_r * mE2_r)"
    elif nMLMnow%2 == 1:
      glob.annz["inputVariables"]  = "psfMag_r; fiberMag_r;(modelMag_r-petroMag_r);petroMag_r; petroRad_r; petroR50_r;petroR90_r;" \
                                   + "lnLStar_r;lnLExp_r;lnLDeV_r ; TMath::Log(pow(mE1_r,2))"

    # --------------------------------------------------------------------------------------------------
    # inputVarErrors -
    # - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    #   - errors are an optional feature of evaluation in classification, and are not generated
    #     by default. if errors are requested (set [addClsKNNerr=True] in evaluation),
    #     then by default they are computed using the KNN error estimator.
    # --------------------------------------------------------------------------------------------------
    #   - inputVarErrors are optional input parameter errors. By default [inputVarErrors=""], and the
    #     errors on MLMs are derived using the KNN-error method. However, its possible to set
    #     inputVarErrors and propagate the error directly - see ANNZ::getRegClsErrINP() for details.
    #   - the errors are not used for training/optimization, but are set during training, as here
    #     it is convenient to specify MLM-dependent error expressions (as different MLMs may have
    #     a different number of input-variables).
    # - example use: set some nonsensical error expressions for illustration for one of the MLMs:
    # --------------------------------------------------------------------------------------------------
    if nMLMnow == 4:
      glob.annz["inputVarErrors"]  = "psfMag_r/10.; fiberMag_r/10.;(modelMag_r-petroMag_r)/10.;petroMag_r/10.; petroRad_r/10.;" \
                                   + "petroR50_r/10.;petroR90_r/10.;lnLStar_r/10.;lnLExp_r/10.;lnLDeV_r/10.; TMath::Log(pow(mE1_r,2))/10."

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
    
    # rndOptTypes - generate automatically these randomized MLM types for cases
    #               where [userMLMopts = ""] (currently "ANN", "BDT" or "ANN_BDT" are supported)
    # --------------------------------------------------------------------------------------------------
    glob.annz["rndOptTypes"] = "BDT"

    if   nMLMnow == 0: glob.annz["userMLMopts"] = "ANNZ_MLM=BDT:VarTransform=N:NTrees=110:NormMode=NumEvents:BoostType=AdaBoost:"
    elif nMLMnow == 1: glob.annz["userMLMopts"] = "ANNZ_MLM=SVM:MaxIter=700"
    elif nMLMnow == 2: glob.annz["userMLMopts"] = "ANNZ_MLM=BDT:NTrees=190:VarTransform=N,P"
    elif nMLMnow == 3: glob.annz["userMLMopts"] = genRndOpts(nMLMnow)
    else:              glob.annz["userMLMopts"] = ""

    # --------------------------------------------------------------------------------------------------
    # userCuts_train,userCuts_valid,userWeights_train,userWeights_valid -
    # - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    #   - define cuts and weights for training and/or validation for any given nMLMnow.
    #     - cuts are logical expressions that define acceptance criterial (e.g., objects which fail
    #       userCuts_train are excluded from the training).
    #       For instance, 
    #         glob.annz["userCuts_train"] = "modelMag_r < 22"
    #       means that only objects which have R-band model-magnitude lower than 22 are accepted for training.
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
      glob.annz["userCuts_train"]    = "modelMag_r < 22.1"
      glob.annz["userCuts_valid"]    = "mE2_r < 0.5"
      glob.annz["userWeights_train"] = "psfMag_r/2."
      glob.annz["userWeights_valid"] = "modelMag_r"
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
  # modify_userCuts_valid,modify_userWeights_valid -
  # - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  #   - during optimization, it is possible to change the cuts/weights for validation which were
  #     defined during training. It is possible to either "ADD" (combine a new expression with the
  #     existing one), to "OVERWRITE" (echange the new expression with the old one), or "IGNORE" (keep
  #     the original definition used during traiing and ignore e.g., the curent value of userCuts_valid).
  #     by default, both modify_userCuts_valid and modify_userWeights_valid are set to "ADD", but
  #     if userCuts_valid and userWeights_valid are lef empty, there will be no affect.
  # - example use (choose changeCutsWeights):
  # --------------------------------------------------------------------------------------------------
  changeCutsWeights = 1
  if changeCutsWeights == 1:
    glob.annz["modify_userCuts_valid"]    = "ADD"
    glob.annz["modify_userWeights_valid"] = "ADD"
    glob.annz["userCuts_valid"]           = "(modelMag_r < 19)"
    glob.annz["userWeights_valid"]        = "fiberMag_r"
  elif changeCutsWeights == 2:
    glob.annz["modify_userCuts_valid"]    = "OVERWRITE"
    glob.annz["userCuts_valid"]           = "(modelMag_r > 14.5)"
    glob.annz["userWeights_valid"]        = "(pow(fiberMag_r,2)/modelMag_r)*(modelMag_r>0)"
  elif changeCutsWeights == 3:
    glob.annz["modify_userCuts_valid"]    = "IGNORE"
    glob.annz["userCuts_valid"]           = "(modelMag_r > 15)"


  # --------------------------------------------------------------------------------------------------
  # optimization
  # --------------------------------------------------------------------------------------------------
  if glob.annz["doOptim"]:
    # run ANNZ with the current settings
    runANNZ()

  # --------------------------------------------------------------------------------------------------
  # evaluation
  # --------------------------------------------------------------------------------------------------
  if glob.annz["doEval"]:

    # inDirName,inAsciiFiles - directory with files to make the calculations from, and list of input files
    glob.annz["inDirName"]    = "examples/data/sgSeparation/eval/"
    glob.annz["inAsciiFiles"] = "sgCatalogue_galaxy.txt;sgCatalogue_star.txt"
    # inAsciiVars - list of parameters in the input files (doesnt need to be exactly the same as in doGenInputTrees, but must contain all
    #               of the parameers which were used for training)
    glob.annz["inAsciiVars"]  = "C:class; F:z; UL:objid; F:psfMag_r; F:fiberMag_r; F:modelMag_r; F:petroMag_r; F:petroRad_r; F:petroR50_r; " \
                              + " F:petroR90_r; F:lnLStar_r; F:lnLExp_r; F:lnLDeV_r; F:mE1_r; F:mE2_r; F:mRrCc_r; I:type_r; I:type"
    # evalDirPostfix - if not empty, this string will be added to the name of the evaluation directory
    #                  (can be used to prevent multiple evaluation of different input files from overwriting each other)
    glob.annz["evalDirPostfix"] = ""

    # ==================================================================================================
    # MLMsToStore - 
    # - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    #   - which MLMs to write to output.
    #     - "ALL"           - write all trained MLMs
    #     - "BEST"          - write only the best performing MLM
    #     - "BEST;x"        - write only the x best performing MLMs (x = integer number)
    #     - "LIST;0;1;3;55" - specific list of MLM indices (sepaated by ';';) to write out
    # --------------------------------------------------------------------------------------------------
    glob.annz["MLMsToStore"] = "BEST;2" 
    
    # --------------------------------------------------------------------------------------------------
    # addOutputVars
    #   - add the following variables (extracted from the input file) to the output
    #     in addition to the regression target (zTrg) (usefull e.g., for matched object-IDs)
    # --------------------------------------------------------------------------------------------------
    glob.annz["addOutputVars"] = "lnLStar_r;lnLExp_r;mE2_r"

    # --------------------------------------------------------------------------------------------------
    # addClsKNNerr -
    # - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    #   - errors are an optional feature of classification, and are not generated by default. if errors
    #     are requested [addClsKNNerr=True], then they are computed by default using the KNN error
    #     estimation method. optionally, if inputVarErrors was set during training for a given MLM, the
    #     quoted errors are propageted to the MLM uncertainty by a simplified method, instead of using KNN.
    # --------------------------------------------------------------------------------------------------
    glob.annz["addClsKNNerr"] = False

    # run ANNZ with the current settings
    runANNZ()

log.info(whtOnBlck(" - "+time.strftime("%d/%m/%y %H:%M:%S")+" - finished running ANNZ !"))

