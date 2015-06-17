from helperFuncs import *

# command line arguments and basic settings
# --------------------------------------------------------------------------------------------------
init()

# ==================================================================================================
# The main code - weight generation without training/optimization/evaluation -
# --------------------------------------------------------------------------------------------------
#   - run the following:
#     python annz_rndReg_weights.py --genInputTrees
#     python annz_rndReg_weights.py --inTrainFlag
# --------------------------------------------------------------------------------------------------
log.info(whtOnBlck(" - "+time.strftime("%d/%m/%y %H:%M:%S")+" - starting ANNZ"))


# --------------------------------------------------------------------------------------------------
# - We define two use-cases for this script:
# --------------------------------------------------------------------------------------------------
#   1. We are interested in deriving weights for unrepresentative datasets (using [useWgtKNN=True]:
#     - In general, splitTypeTrain is the training dataset and splitTypeTest is the evaluation dataset.
#       For each one, the weights will be computed with regards to thereference dataset (defined in inAsciiFiles_wgtKNN).
#     - Since there is no actuall training of MLMs here, just weight derivation, we don't really care about
#       the "training"/"testing" labels. We just need to make sure that at least one of these two variables
#       holds the dataset for which we want to derive the weights.
#     - The results will be stored in e.g.:
#       ./output/test_randReg_weights/rootIn/ANNZ_KNN_wANNZ_tree_train_0000.csv (for the content of splitTypeTrain)
#       ./output/test_randReg_weights/rootIn/ANNZ_KNN_wANNZ_tree_valid_0000.csv (for the content of splitTypeTest)
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#   2. We are interested in deriving the inTrainFlag quality-flag:
#     - In this case, we are specifically interested in the content of splitTypeTrain. This "training" dataset
#       will be used as the reference sample when we run --inTrainFlag. (The purpose of the --inTrainFlag
#       run is to calculate "compatibility" with the reference training sample defined in splitTypeTrain.)
#     - Since the content of splitTypeTest will be ignored, we just need to set a random input file 
#       with the correct variable structure. the easiest is to use the same input as for splitTypeTrain
#     - The results will be stored in e.g.:
#       ./output/test_randReg_weights/inTrainFlag/inTrainFlagANNZ_tree_wgtTree_0000.csv
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#   - For these two cases, setting
#       glob.annz["splitTypeTrain"] = glob.annz["splitTypeTest"]
#     is a good choice. This is not mandatory, but is the recommended option, in order to
#     try and minimize confuision ...
# --------------------------------------------------------------------------------------------------
# - IMPORTANT notes:
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#   - It is only possible to run --inTrainFlag after running --genInputTrees, since
#     the output of --genInputTrees is the "training" data which serves as the reference sample
#     for the --inTrainFlag calculation - the outcome of the calculation will be a flag which is equal
#     to either zero or one, where zero will implay an object which is "not compatible" with the
#     reference dataset (the training sample).
#   - One does not need to set [useWgtKNN=True] during --genInputTrees, but --genInputTrees needs
#     to be run beforehand regardless, as the output of --genInputTrees is used during --inTrainFlag.
#     If [useWgtKNN=True] is indeed set during --genInputTrees, then these weights are also taken
#     into account during the --inTrainFlag calculation.
# --------------------------------------------------------------------------------------------------


# --------------------------------------------------------------------------------------------------
# general options which are the same for all stages
#   - PLEASE ALSO INSPECT generalSettings(), WHICH HAS BEEN RUN AS PART OF init(), FOR MORE OPTIONS
# --------------------------------------------------------------------------------------------------
# outDirName - set output directory name
glob.annz["outDirName"] = "test_randReg_weights"

# --------------------------------------------------------------------------------------------------
# pre-processing of the input dataset
# --------------------------------------------------------------------------------------------------
if glob.annz["doGenInputTrees"]:
  # inDirName    - directory in which input files are stored
  glob.annz["inDirName"]      = "examples/data/photoZ/train"

  # inAsciiVars  - list of parameter types and parameter names, corresponding to columns in the input
  #                file, e.g., [TYPE:NAME] may be [F:MAG_U], with 'F' standing for float. (see advanced example for detailed explanation)
  glob.annz["inAsciiVars"]    = "F:MAG_U;F:MAGERR_U;F:MAG_G;F:MAGERR_G;F:MAG_R;F:MAGERR_R;F:MAG_I;F:MAGERR_I;F:MAG_Z;F:MAGERR_Z"

  # input datasets
  glob.annz["nSplit"]         = 2
  glob.annz["splitTypeTrain"] = glob.annz["splitTypeTest"] = "boss_dr10_0_large_magCut_noZ.csv"


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
  useWgtKNN = True
  if useWgtKNN:
    glob.annz["useWgtKNN"]             = True
    glob.annz["minNobjInVol_wgtKNN"]   = 50
    glob.annz["inAsciiFiles_wgtKNN"]   = "boss_dr10_colorCuts.csv"
    glob.annz["inAsciiVars_wgtKNN"]    = "F:MAG_U;F:MAGERR_U;F:MAG_G;F:MAGERR_G;F:MAG_R;F:MAGERR_R;F:MAG_I;F:MAGERR_I;F:MAG_Z;F:MAGERR_Z;D:Z"
    glob.annz["weightVarNames_wgtKNN"] = "MAG_U;MAG_G;MAG_R;MAG_I;MAG_Z"
    
    # optional parameters (may leave empty as default value):
    glob.annz["sampleFracInp_wgtKNN"]  = 0.10                                          # fraction of dataset to use (positive number, smaller or equal to 1)
    glob.annz["sampleFracRef_wgtKNN"]  = 0.95                                          # fraction of dataset to use (positive number, smaller or equal to 1)
    glob.annz["outAsciiVars_wgtKNN"]   = "MAG_U;MAG_G;MAGERR_U"                        # write out two additional variables to the output file
    glob.annz["weightRef_wgtKNN"]      = "(MAGERR_R<0.7)*1 + (MAGERR_R>=0.7)/MAGERR_R" # down-weight objects with high MAGERR_R
    glob.annz["cutRef_wgtKNN"]         = "MAGERR_U<200"                                # only use objects which have small MAGERR_U

  # run ANNZ with the current settings
  runANNZ()


# --------------------------------------------------------------------------------------------------
# - generate inTrainFlag quality-flags
# --------------------------------------------------------------------------------------------------
if glob.annz["doInTrainFlag"]:

  # inDirName,inAsciiFiles - directory with files to make the calculations from, and list of input files
  glob.annz["inDirName"]      = "examples/data/photoZ/eval/"
  glob.annz["inAsciiFiles"]   = "boss_dr10_eval1_noZ.csv"
  # inAsciiVars - list of parameters in the input files (doesnt need to be exactly the same as in doGenInputTrees, but must contain all
  #               of the parameers which were used for training)
  glob.annz["inAsciiVars"]    = "F:MAG_U;F:MAGERR_U;F:MAG_G;F:MAGERR_G;F:MAG_R;F:MAGERR_R;F:MAG_I;F:MAGERR_I;F:MAG_Z;F:MAGERR_Z"

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
  #   - maxRelRatioInRef_inTrain - A number in the range, [0,1] - The minimal threshold of the relative difference between
  #                              distances in the inTrainFlag calculation for accepting an object - Should be a (<0.5) positive number.
  #   - ...._inTrain             - The rest of the parameters ending with "_inTrain" have a similar role as
  #                              their "_wgtKNN" counterparts, which are used with [glob.annz["useWgtKNN"] = True]. These are:
  #                                - "outAsciiVars_inTrain", "weightInp_inTrain", "cutInp_inTrain",
  #                                  "cutRef_inTrain", "sampleFracInp_inTrain" and "sampleFracRef_inTrain"
  # --------------------------------------------------------------------------------------------------
  glob.annz["addInTrainFlag"]           = True
  glob.annz["minNobjInVol_inTrain"]     = 100
  glob.annz["maxRelRatioInRef_inTrain"] = 0.1
  glob.annz["weightVarNames_inTrain"]   = "MAG_U;MAG_G;MAG_R;MAG_I;MAG_Z"
  glob.annz["outAsciiVars_inTrain"]     = "MAG_G;MAG_R" 

  # run ANNZ with the current settings
  runANNZ()

log.info(whtOnBlck(" - "+time.strftime("%d/%m/%y %H:%M:%S")+" - finished runing ANNZ !"))

