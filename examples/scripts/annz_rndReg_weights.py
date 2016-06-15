# --------------------------------------------------------------------------------------------------
# - We define two use-cases for this script:
# --------------------------------------------------------------------------------------------------
#   1. We are interested in deriving weights for unrepresentative datasets (using [useWgtKNN=True]):
#     - Since there is no actual training of MLMs here, just weight derivation, we don't really care about
#       the "training"/"testing" labels. We therefore set ["nSplit" = 1] and use the entire dataset defined in "inAsciiFiles".
#     - The results will be stored in e.g.:
#       ./output/test_randReg_weights/rootIn/ANNZ_KNN_wANNZ_tree_full_0000.csv
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#   2. We are interested in deriving the inTrainFlag quality-flag:
#       the outcome of the calculation will be a flag between zero and one,
#       where zero will implay an object which is "not compatible" with the reference dataset.
#     - In this case, we must first run --genInputTrees. The output of this
#       will serve as the reference dataset for the --inTrainFlag calculation.
#     - The results will be stored in e.g.:
#       ./output/test_randReg_weights/inTrainFlag/inTrainFlagANNZ_tree_wgtTree_0000.csv
# --------------------------------------------------------------------------------------------------
# - IMPORTANT notes:
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#   - For these two cases, please note that the "inAsciiFiles" variable serves different purpose !!!
#   - One does not need to set [useWgtKNN=True] during --genInputTrees, but --genInputTrees needs
#     to be run beforehand regardless, as the output of --genInputTrees is used during --inTrainFlag.
#     If [useWgtKNN=True] is indeed set during --genInputTrees, then these weights are also taken
#     into account during the --inTrainFlag calculation.
# --------------------------------------------------------------------------------------------------


from annz.helperFuncs import *

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

# glob.annz["logLevel"] = "DEBUG_2"

# --------------------------------------------------------------------------------------------------
# general options which are the same for all stages
#   - PLEASE ALSO INSPECT generalSettings(), WHICH HAS BEEN RUN AS PART OF init(), FOR MORE OPTIONS
# --------------------------------------------------------------------------------------------------
# outDirName - set output directory name
glob.annz["outDirName"] = "test_randReg_weights"

# no splitting of the dataset into training/validation is needed here
glob.annz["nSplit"] = 1

# --------------------------------------------------------------------------------------------------
# pre-processing of the input dataset
# --------------------------------------------------------------------------------------------------
if glob.annz["doGenInputTrees"]:
  # inDirName    - directory in which input files are stored
  glob.annz["inDirName"]      = "examples/data/photoZ/train"

  # inAsciiVars  - list of parameter types and parameter names, corresponding to columns in the input
  #                file, e.g., [TYPE:NAME] may be [F:MAG_U], with 'F' standing for float. (see advanced example for detailed explanation)
  # glob.annz["inAsciiVars"]    = "F:MAG_U;F:MAGERR_U;F:MAG_G;F:MAGERR_G;F:MAG_R;F:MAGERR_R;F:MAG_I;F:MAGERR_I;F:MAG_Z;F:MAGERR_Z;D:Z"

  # input dataset
  # glob.annz["inAsciiFiles"]   = "boss_dr10_0.csv;boss_dr10_1.csv;boss_dr10_2.csv;boss_dr10_3.csv"

  glob.annz["inAsciiVars"]  = "F:MAG_U;F:MAGERR_U;F:MAG_G;F:MAGERR_G;F:MAG_R;F:MAGERR_R;F:MAG_I;F:MAGERR_I;F:MAG_Z;F:MAGERR_Z"
  glob.annz["inAsciiFiles"] = "boss_dr10_0_large_magCut_noZ.csv"

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
    glob.annz["inAsciiFiles_wgtKNN"]   = glob.annz["inAsciiFiles"]
    glob.annz["inAsciiVars_wgtKNN"]    = glob.annz["inAsciiVars"]

    # some random weird choice for [weightInp_wgtKNN, weightRef_wgtKNN] in this example, just to get different
    # distributions for the input and reference samples, so that we have something to calculate weights for...
    glob.annz["weightInp_wgtKNN"]      = "1/pow(MAG_G*MAG_U*MAG_R*MAG_I, 5)"
    glob.annz["weightRef_wgtKNN"]      = "1/MAGERR_G"

    glob.annz["weightVarNames_wgtKNN"] = "MAG_U;MAG_G;MAG_R;MAG_I;MAG_Z"

    # optional parameters (may leave empty as default value):
    glob.annz["sampleFracInp_wgtKNN"]  = 0.385                                          # fraction of dataset to use (positive number, smaller or equal to 1)
    glob.annz["sampleFracRef_wgtKNN"]  = 0.390                                          # fraction of dataset to use (positive number, smaller or equal to 1)
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

  # run ANNZ with the current settings
  runANNZ()


# --------------------------------------------------------------------------------------------------
# - generate inTrainFlag quality-flags
# --------------------------------------------------------------------------------------------------
if glob.annz["doInTrainFlag"]:

  # inDirName,inAsciiFiles - directory with files to make the calculations from, and list of input files
  glob.annz["inDirName"]      = "examples/data/photoZ/eval/"
  glob.annz["inAsciiFiles"]   = "boss_dr10_eval0_noZ.csv"
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
  glob.annz["addInTrainFlag"]           = True
  glob.annz["minNobjInVol_inTrain"]     = 100
  glob.annz["maxRelRatioInRef_inTrain"] = -1
  glob.annz["weightVarNames_inTrain"]   = "MAG_U;MAG_G;MAG_R;MAG_I;MAG_Z"
  glob.annz["outAsciiVars_inTrain"]     = "MAG_G;MAG_R"
  glob.annz["doWidthRescale_inTrain"]   = True

  # run ANNZ with the current settings
  runANNZ()

log.info(whtOnBlck(" - "+time.strftime("%d/%m/%y %H:%M:%S")+" - finished running ANNZ !"))

