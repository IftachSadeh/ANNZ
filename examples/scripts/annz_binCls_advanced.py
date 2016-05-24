from helperFuncs import *

# command line arguments and basic settings
# --------------------------------------------------------------------------------------------------
init()

# just in case... (may comment this out)
if not glob.annz["doBinnedCls"]:
  log.info(red(" - "+time.strftime("%d/%m/%y %H:%M:%S")+" - This scripts is only designed for binnedClassification...")) ; sys.exit(0)

# ==================================================================================================
# The main code - binned classification -
# --------------------------------------------------------------------------------------------------
#   - run the following:
#     python annz_binCls_advanced.py --binnedClassification --genInputTrees
#     python annz_binCls_advanced.py --binnedClassification --train
#     python annz_binCls_advanced.py --binnedClassification --verify
#     python annz_binCls_advanced.py --binnedClassification --evaluate
# --------------------------------------------------------------------------------------------------
log.info(whtOnBlck(" - "+time.strftime("%d/%m/%y %H:%M:%S")+" - starting ANNZ"))

# --------------------------------------------------------------------------------------------------
# general options which are the same for all stages
#   - PLEASE ALSO INSPECT generalSettings(), WHICH HAS BEEN RUN AS PART OF init(), FOR MORE OPTIONS
# --------------------------------------------------------------------------------------------------
# outDirName - set output directory name
glob.annz["outDirName"] = "test_binCls_advanced"

# zTrg            - the name of the target variable of the regression
# minValZ,maxValZ - the minimal and maximal values of the target variable (zTrg)
glob.annz["zTrg"]       = "Z"
glob.annz["minValZ"]    = 0.05
glob.annz["maxValZ"]    = 0.8

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
  inFileOpt = 0
  # splitTypeTrain - list of files for training. splitTypeTest - list of files for testing
  if   inFileOpt == 0:
    glob.annz["splitTypeTrain"] = "boss_dr10_0_large.csv"
    glob.annz["splitTypeTest"]  = "boss_dr10_1_large.csv"
  # inAsciiFiles - one list of input files for training and testing, where the the objects are assigned to a given
  # category based on the selection criteria defined by splitType
  elif inFileOpt == 1:
    glob.annz["splitType"]      = "serial" # "serial", "blocks" or "random"
    glob.annz["inAsciiFiles"]   = "boss_dr10_0_large.csv;boss_dr10_1_large.csv;boss_dr10_2_large.csv"
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
  # - example use:
  #   set useWgtKNN as [True] to generate and use the weights
  # --------------------------------------------------------------------------------------------------
  useWgtKNN = False
  if useWgtKNN:
    glob.annz["useWgtKNN"]             = True
    glob.annz["minNobjInVol_wgtKNN"]   = 50
    glob.annz["inAsciiFiles_wgtKNN"]   = "boss_dr10_colorCuts.csv"
    glob.annz["inAsciiVars_wgtKNN"]    = glob.annz["inAsciiVars"]
    glob.annz["weightVarNames_wgtKNN"] = "MAG_U;MAG_G;MAG_R;MAG_I;MAG_Z"
    
    # optional parameters (may leave empty as default value):
    glob.annz["sampleFracInp_wgtKNN"]  = 0.9                                           # fraction of dataset to use (positive number, smaller or equal to 1)
    glob.annz["sampleFracRef_wgtKNN"]  = 0.8                                           # fraction of dataset to use (positive number, smaller or equal to 1)
    glob.annz["outAsciiVars_wgtKNN"]   = "MAG_U;MAG_G;MAGERR_U"                        # write out two additional variables to the output file
    glob.annz["weightRef_wgtKNN"]      = "(MAGERR_R<0.7)*1 + (MAGERR_R>=0.7)/MAGERR_R" # down-weight objects with high MAGERR_R
    glob.annz["cutRef_wgtKNN"]         = "MAGERR_U<200"                                # only use objects which have small MAGERR_U

  # run ANNZ with the current settings
  runANNZ()


# --------------------------------------------------------------------------------------------------
# training, verification and evaluation
# --------------------------------------------------------------------------------------------------
if glob.annz["doTrain"] or glob.annz["doVerif"] or glob.annz["doEval"]:

  # --------------------------------------------------------------------------------------------------
  # binCls_nBins,binCls_maxBinW,binCls_clsBins -
  # - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  # - Two options to define the classification bins:
  #   1. If [binCls_maxBinW==0], then set e.g.,:
  #        glob.annz["binCls_nBins"]   = 7
  #        glob.annz["binCls_clsBins"] = "0.05;0.1;0.2;0.3;0.4;0.5;0.6;0.8"
  #      to use a specific set of bins. It is required to explicitly set binCls_nBins, the value of the number
  #      of bins which are given in binCls_clsBins. This is to ensure that the training loop indeed
  #      produces an MLM for each of the bins in range(binCls_nBins).
  #      Make sure that the first and last bins in binCls_clsBins are within minValZ and maxValZ.
  #   2. If [binCls_clsBins==""], then set e.g.,:
  #        glob.annz["binCls_nBins"]   = 10
  #        glob.annz["binCls_maxBinW"] = 0.1
  #      to divide the range between minValZ and maxValZ into binCls_nBins bins. The bin-width is
  #      defined on the fly according, such that each bin has approximately the same number of (weighted) entries,
  #      while constraining the ben-width to be no larger than binCls_maxBinW. (see description in ANNZ::deriveBinClsBins().)
  #      It is possible for the automatic bin calculation not to work (especially with sparse distributions),
  #      so please check the output of the training, and confirm that the bins (and the bin content) are reasonable
  # - example use:
  #   set clsBinType and choose one of the following options for classification-bin definition:
  # --------------------------------------------------------------------------------------------------
  clsBinType = 2
  if   clsBinType == 0:
    # derive the binning scheme on the fly, such that 60 bins, each no wider than 0.02, are chosen
    glob.annz["binCls_nBins"]   = 60
    glob.annz["binCls_maxBinW"] = 0.02
  elif clsBinType == 1:
    # an example of some specific bin-scheme (shown for illustration... probably not the best choice)
    glob.annz["binCls_nBins"]   = 15
    glob.annz["binCls_clsBins"] = "0.05;0.1;0.15;0.2;0.25;0.3;0.35;0.4;0.45;0.5;0.55;0.6;0.65;0.7;0.75;0.8"
  elif clsBinType == 2:
    # another example of a specific bin-scheme, here defining 40 equal-width bins between minValZ and maxValZ
    glob.annz["binCls_nBins"]   = 40
    glob.annz["binCls_clsBins"] = ""
    for nBinNow in range(glob.annz["binCls_nBins"]+1):
      binEdge = glob.annz["minValZ"] + nBinNow*(glob.annz["maxValZ"]-glob.annz["minValZ"])/float(glob.annz["binCls_nBins"])
      glob.annz["binCls_clsBins"] += str(binEdge)+";"


  # --------------------------------------------------------------------------------------------------
  # training
  # --------------------------------------------------------------------------------------------------
  if glob.annz["doTrain"]:

    # generate binCls_nTries different randomized MLMs for each bin, and use only the one which has the highest separation parameter
    # a ghigh value will slow down training significantly (for BDTs a high value is ok, since they train very fast...)
    glob.annz["binCls_nTries"] = 10

    # rndOptTypes - generate automatically these randomized MLM types for cases
    #               where [userMLMopts = ""] (currently "ANN", "BDT" or "ANN_BDT" are supported)
    # --------------------------------------------------------------------------------------------------
    glob.annz["rndOptTypes"]   = "BDT"

    # --------------------------------------------------------------------------------------------------
    # doMultiCls:
    # --------------------------------------------------------------------------------------------------
    #   - Using the MultiClass option of binned classification, multiple background samples can be trained
    #     simultaneously against the signal. This means that each classification bin acts as an independent sample during
    #     the training. The MultiClass option is only compatible with four MLM algorithms: BDT, ANN, FDA and PDEFoam.
    #     For BDT, only the gradient boosted decision trees are available. That is, one may set ":BoostType=Grad",
    #     but not ":BoostType=Bagging" or ":BoostType=AdaBoost", as part of the userMLMopts option.
    #     - examples:
    #       - glob.annz["userMLMopts_0"] = "ANNZ_MLM=FDA:Formula=(0)+(1)*x0+(2)*x1+(3)*x2+(4)*x3:" \
    #                                     +"ParRanges=(-1,1);(-10,10);(-10,10);(-10,10);(-10,10):" \
    #                                     +"FitMethod=GA:PopSize=300:Cycles=3:Steps=20:Trim=True:SaveBestGen=1"
    #       - glob.annz["userMLMopts_1"] = "ANNZ_MLM=PDEFoam:nActiveCells=500:nSampl=2000:nBin=5:Nmin=100:Kernel=None:Compress=T"
    #   - Using the MultiClass option, the binCls_bckShiftMin,binCls_bckShiftMax,binCls_bckSubsetRange
    #     options are ignored.
    #   - Using the MultiClass option, training is much slower, it is therefore recommended to set a low
    #     value (<3) of binCls_nTries.
    # --------------------------------------------------------------------------------------------------
    glob.annz["doMultiCls"] = False

    # --------------------------------------------------------------------------------------------------
    # - binCls_bckShiftMin,binCls_bckShiftMax (Optional training setting):
    # --------------------------------------------------------------------------------------------------
    #   - setting binCls_bckShiftMin,binCls_bckShiftMax to some values with (binCls_bckShiftMax > binCls_bckShiftMin)
    #     will exclude a range of background values from the training.
    #     For a signal bin z0<zTrg<z1 the nominal bacgkround will be (zTrg<z0 || zTrg>z1). Then the background
    #     will be defined as (zTrg<z0-x || zTrg>z1+x), with x a random number between binCls_bckShiftMin and binCls_bckShiftMax
    # --------------------------------------------------------------------------------------------------
    glob.annz["binCls_bckShiftMin"] = 0.06
    glob.annz["binCls_bckShiftMax"] = 0.1

    # --------------------------------------------------------------------------------------------------
    # - binCls_bckSubsetRange (Optional training setting):
    # --------------------------------------------------------------------------------------------------
    #   - Setting binCls_bckSubsetRange to value of format "x_y_p" will reject background objects from training,
    #     such that the ratio of background to signal is in the range between x and y. This will happen
    #     for p% of the requested binCls_nTries.
    #     For instance:
    #       using [binCls_bckSubsetRange="5_10_20"], the training will include between 5 times
    #       to 10 times as many background objects as signal objects. this cut will be applied 20% of
    #       the time (out of binCls_nTries).
    #   - all values, (x,y,p) should be integers.
    #   - the excat ratio of bck/sig is randomely generated within the requested range.
    # --------------------------------------------------------------------------------------------------
    glob.annz["binCls_bckSubsetRange"] = "5_10_100"

    for nBinNow in range(glob.annz["binCls_nBins"]):
      glob.annz["nBinNow"] = nBinNow;
      if glob.annz["trainIndex"] >= 0 and glob.annz["trainIndex"] != nBinNow: continue

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
      myRandType = 0
      # --------------------------------------------------------------------------------------------------
      # set specific user-options for the first two tries, and let all the rest be randomized internally in ANNZ
      # --------------------------------------------------------------------------------------------------
      if myRandType == 0:
        for nRndOptNow in range(glob.annz["binCls_nTries"]):
          if   nRndOptNow == 0: glob.annz["userMLMopts_"+str(nRndOptNow)] = "ANNZ_MLM=BDT:VarTransform=Norm:NTrees=111:NormMode=NumEvents:BoostType=AdaBoost:"
          elif nRndOptNow == 1: glob.annz["userMLMopts_"+str(nRndOptNow)] = "ANNZ_MLM=BDT:VarTransform=Norm:NTrees=190"
          else: break
      # --------------------------------------------------------------------------------------------------
      # set specific user-options for the first try, and let all the rest be randomized using the
      # python function, genRndOpts(), with optional MLM types defined by glob.annz["rndOptTypes"]
      # --------------------------------------------------------------------------------------------------
      elif myRandType == 1:
        for nRndOptNow in range(glob.annz["binCls_nTries"]):
          if   nRndOptNow == 0: glob.annz["userMLMopts_"+str(nRndOptNow)] = "ANNZ_MLM=BDT:VarTransform=Norm:NTrees=111:NormMode=NumEvents:BoostType=AdaBoost:"
          else:                 glob.annz["userMLMopts_"+str(nRndOptNow)] = genRndOpts(nRndOptNow)
      # --------------------------------------------------------------------------------------------------
      # explicitly define the first two tries, and let ANNZ randomize the rest internally
      # --------------------------------------------------------------------------------------------------
      elif myRandType == 2:
        for nRndOptNow in range(glob.annz["binCls_nTries"]):
          if   nRndOptNow == 0: glob.annz["userMLMopts_"+str(nRndOptNow)] = "ANNZ_MLM=SVM"
          elif nRndOptNow == 1: glob.annz["userMLMopts_"+str(nRndOptNow)] = "ANNZ_MLM=KNN"
          else:                 glob.annz["userMLMopts_"+str(nRndOptNow)] = ""
      # --------------------------------------------------------------------------------------------------
      # let all MLMs be randomized internally by ANNZ
      # --------------------------------------------------------------------------------------------------
      else:
        for nRndOptNow in range(glob.annz["binCls_nTries"]):
          glob.annz["userMLMopts_"+str(nRndOptNow)] = ""
      # --------------------------------------------------------------------------------------------------

      # --------------------------------------------------------------------------------------------------
      # userCuts_train,userCuts_valid,userWeights_train,userWeights_valid -
      # - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
      #   - define cuts and weights for training and/or validation.
      #     - cuts are logical expressions that define acceptance criterial (e.g., objects which fail
      #       userCuts_train are excluded from the training).
      #       For instance, 
      #         glob.annz["userCuts_train"] = "MAG_G < 24"
      #       means that only objects which have G-band magnitude lower than 24 are accepted for training.
      #     - weights are used to enhance or reduce the significance of objects by some numerical value.
      #       weights may include logical expressions, to affect different subsets of the dataset.
      #   - cuts and weights defined during training can not be changed in verification/evaluation. However,
      #     the validation cuts/weights can be different for training and for verification/evaluation,
      #     determined by modify_userCuts_valid and modify_userWeights_valid.
      #     validation weights are not used directly during training, however, this is the place to
      #     specify any MLM specific weights. During verification one can only set global weigts for all MLMs.
      #   - the expressions for userCuts_train and userWeights_train must be the same for all nBinNow !!!
      # - example use:
      # --------------------------------------------------------------------------------------------------
      useTrainCuts = False
      if useTrainCuts: # set some nonsensical cuts and weights for illustration
        glob.annz["userCuts_train"]    = "MAG_G < 24"
        glob.annz["userCuts_valid"]    = "(MAGERR_I < 0.45) && ((MAG_U > 19) || (MAG_Z > 17))"
        glob.annz["userWeights_train"] = "(MAG_R > 22)/MAG_R + (MAG_R <= 22)*1"
        glob.annz["userWeights_valid"] = "(MAG_G < 20)*MAG_G + (MAG_G >= 20)*1"


      # --------------------------------------------------------------------------------------------------
      # inputVariables - 
      # - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
      #   - semicolon-separated list of input variables for the MLMs. Can include math expressions of the variables
      #     given in inAsciiVars (see https://root.cern.ch/root/html520/TFormula.html for examples of valid math expressions).
      #     logical expressions are also valid, as e.g., (MAG_G*(MAGERR_G<1) + 27*(MAGERR_G>=1))
      #     inputVariables can be different for any given MLM (any value of nMLMnow).
      #   - it is possible to randomize the input parameter for different elements within binCls_nTries:
      #     - set as many as binCls_nTries different combinations of input parameters (ANNZ will cycle
      #       through the different combinations if fewer than binCls_nTries are defined)
      #     - The expected glob.annz[] parameter name is e.g., format "inputVariables_0".
      # - example use:
      #   set inpVarType to choose wether to use one set of input-variables, or a try several variations.
      # --------------------------------------------------------------------------------------------------
      inpVarType = 0
      if   inpVarType == 0:
        glob.annz["inputVariables"] = "MAG_U;(MAG_U-MAG_G);(MAG_G-MAG_R);(MAG_R-MAG_I);(MAG_I-MAG_Z)"
      elif inpVarType == 1:
        for nRndOptNow in range(glob.annz["binCls_nTries"]):
          if   nRndOptNow == 0: glob.annz["inputVariables_"+str(nRndOptNow)] = "MAG_U;MAG_G;(MAG_G-MAG_R);(MAG_R-MAG_I);(MAG_I-MAG_Z)"
          elif nRndOptNow == 1: glob.annz["inputVariables_"+str(nRndOptNow)] = "MAG_U;MAG_G;(MAG_G-MAG_R);(MAG_R-MAG_I);(MAG_I-MAG_Z);(MAG_G-MAG_Z)"
          elif nRndOptNow == 2: glob.annz["inputVariables_"+str(nRndOptNow)] = "MAG_G; MAG_U;(MAG_G-MAG_R);(MAG_I-MAG_Z)"
          else: break

      # --------------------------------------------------------------------------------------------------
      # inputVarErrors -
      # - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
      #   - in general, MLM errors are only used by the second (experimental) PDF, and are not required for the
      #     nominal solution. If this second PDF is requested (setting [nPDFs=2]) then they are by default
      #     computed using the KNN method (as for randomized regression). However, it is possible (not recomended) to
      #     propagate the uncertainty on the input parameters directly, using inputVarErrors.
      #     see ANNZ::getRegClsErrINP() for details on how this is done.
      # - example use:
      #   set useInpErrs to [True] to try this out...
      #   note that the numer of elements in inputVarErrors_XXX and in inputVariables_XXX must match
      # --------------------------------------------------------------------------------------------------
      useInpErrs = False
      if useInpErrs:
        if nBinNow == 0:
          if   inpVarType == 0:
            glob.annz["inputVarErrors"] = "MAGERR_U;MAGERR_G;MAGERR_R;MAGERR_I;MAGERR_Z"
          elif inpVarType == 1:
            for nRndOptNow in range(glob.annz["binCls_nTries"]):
              if   nRndOptNow == 0: glob.annz["inputVarErrors_"+str(nRndOptNow)] = "MAGERR_U;MAGERR_G;MAGERR_R;MAGERR_I;MAGERR_Z"
              elif nRndOptNow == 1: glob.annz["inputVarErrors_"+str(nRndOptNow)] = "MAGERR_U;MAGERR_G;MAGERR_R;MAGERR_I;MAGERR_Z;MAGERR_Z"
              elif nRndOptNow == 2: glob.annz["inputVarErrors_"+str(nRndOptNow)] = ""  # generate the errors on MLM using the KNN-error method if needed
              else: break
        else:
          glob.annz["inputVarErrors"] = "" # generate the errors on MLM using the KNN-error method if needed

      # run ANNZ with the current settings
      runANNZ()
    
  # --------------------------------------------------------------------------------------------------
  # verification and evaluation
  # --------------------------------------------------------------------------------------------------
  if glob.annz["doVerif"] or glob.annz["doEval"]:

    # --------------------------------------------------------------------------------------------------
    # nPDFs - 
    #   number of PDFs (up to two PDF types are implemented, the second one is experimental)
    # --------------------------------------------------------------------------------------------------
    glob.annz["nPDFs"]    = 1

    # --------------------------------------------------------------------------------------------------
    # nPDFbins,userPdfBins -
    #   - nPDFbins:    number of PDF bins, with equal width bins between minValZ and maxValZ.
    #                  this is not directly tied to binCls_nBins -> the results of 
    #                  the classification bins are cast into whatever final PDF bin scheme is defined by nPDFbins.
    #                  nPDFbins is ignored if [userPdfBins != ""].
    #   - userPdfBins: define a specific set of bins instead of having nPDFbins bins of euqal width.
    # --------------------------------------------------------------------------------------------------
    glob.annz["nPDFbins"] = 40
    # glob.annz["userPdfBins"] = "0.05;0.3;0.5;0.6;0.8"

    # --------------------------------------------------------------------------------------------------
    # modify_userCuts_valid,modify_userWeights_valid -
    # - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    #   - during verification, it is possible to change the cuts/weights for validation which were
    #     defined during training. It is possible to either "ADD" (combine a new expression with the
    #     existing one), to "OVERWRITE" (echange the new expression with the old one), or "IGNORE" (keep
    #     the original definition used during traiing and ignore e.g., the curent value of userCuts_valid).
    #     by default, both modify_userCuts_valid and modify_userWeights_valid are set to "ADD", but
    #     if userCuts_valid and userWeights_valid are lef empty, there will be no affect.
    # - example use (choose changeCutsWeights):
    #   (notice the use of TMath::Abs() - see: https://root.cern.ch/root/html/TMath.html for available functions.)
    # --------------------------------------------------------------------------------------------------
    changeCutsWeights = 0
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

    # --------------------------------------------------------------------------------------------------
    # addOutputVars
    #   - add the following variables (extracted from the input file) to the output
    #     in addition to the regression target (zTrg) (usefull e.g., for matched object-IDs)
    # --------------------------------------------------------------------------------------------------
    glob.annz["addOutputVars"] = "MAG_U;MAGERR_R"

    # --------------------------------------------------------------------------------------------------
    # verification
    # --------------------------------------------------------------------------------------------------
    if glob.annz["doVerif"]:
      # run ANNZ with the current settings
      runANNZ()

    # --------------------------------------------------------------------------------------------------
    # evaluation
    # --------------------------------------------------------------------------------------------------
    if glob.annz["doEval"]:

      # inDirName,inAsciiFiles - directory with files to make the calculations from, and list of input files
      glob.annz["inDirName"]      = "examples/data/photoZ/eval/"
      glob.annz["inAsciiFiles"]   = "boss_dr10_eval1.csv" #;boss_dr10_eval0.csv"
      # inAsciiVars - list of parameters in the input files (doesnt need to be exactly the same as in doGenInputTrees, but must contain all
      #               of the parameers which were used for training)
      glob.annz["inAsciiVars"]    = "F:MAG_U;F:MAGERR_U;F:MAG_G;F:MAGERR_G;F:MAG_R;F:MAGERR_R;F:MAG_I;F:MAGERR_I;F:MAG_Z;F:MAGERR_Z;D:Z"
      # evalDirPostfix - if not empty, this string will be added to the name of the evaluation directory
      #                  (can be used to prevent multiple evaluation of different input files from overwriting each other)
      glob.annz["evalDirPostfix"] = ""

      # run ANNZ with the current settings
      runANNZ()

log.info(whtOnBlck(" - "+time.strftime("%d/%m/%y %H:%M:%S")+" - finished running ANNZ !"))

