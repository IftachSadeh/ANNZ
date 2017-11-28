# --------------------------------------------------------------------------------------------------
# Use this script to generate knn-error estimates for a given observable (evaluated object). 
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#   - We use a reference dataset for which the true value of the evaluated object is know.
#   - The errors are computed by finding the near neighbours of the evaluated object. We compute
#     the distribution of errors, defined for a given object as [eval_value - true_value].
#   - The final error is then derived as the width of the distribution of errors.
# --------------------------------------------------------------------------------------------------
# - The code is run in two stages (see details below):
#   1. genInputTrees: Here we do pre-processing of the reference dataset (done once).
#   2. evaluate:      Here we evaluate the errors on a given dataset. This can be repeated
#                     on different samples without the need to repeat the [genInputTrees] stage.
# --------------------------------------------------------------------------------------------------

from scripts.helperFuncs import *

# command line arguments and basic settings
# --------------------------------------------------------------------------------------------------
init()

# ==================================================================================================
# The main code -
# --------------------------------------------------------------------------------------------------
#   - run the following:
#     python annz_rndReg_knnErr.py --onlyKnnErr --genInputTrees
#     python annz_rndReg_knnErr.py --onlyKnnErr --evaluate
# --------------------------------------------------------------------------------------------------
log.info(whtOnBlck(" - "+time.strftime("%d/%m/%y %H:%M:%S")+" - starting ANNZ"))

if glob.annz["doOnlyKnnErr"]:
  glob.annz["logLevel"] = "DEBUG_1"

  # outDirName - set output directory name
  glob.annz["outDirName"] = "test_knnErr"

  # --------------------------------------------------------------------------------------------------
  # - zReg_onlyKnnErr, zTrg:
  #   The names of the evaluated object and of the true value of the latter.
  #   For instance, [zReg_onlyKnnErr] could be the photo-z of a galaxy, which we evaluate with some
  #   external code. [zTrg] would in this case be the corresponding true redshift of the galaxy.
  # --------------------------------------------------------------------------------------------------
  # the name of the evaluated objects
  glob.annz["zReg_onlyKnnErr"] = "myPhotoZ"
  # the name of the true value of the evaluated objects 
  glob.annz["zTrg"]            = "Z"

  # --------------------------------------------------------------------------------------------------
  # pre-processing of the input dataset
  # --------------------------------------------------------------------------------------------------
  if glob.annz["doGenInputTrees"]:
    # inDirName    - directory in which input files are stored
    glob.annz["inDirName"]    = "examples/data/photoZ/knnErr"

    # inAsciiVars  - list of parameter types and parameter names, corresponding to columns in the input
    #                file, e.g., [TYPE:NAME] may be [F:MAG_U], with 'F' standing for float. (see advanced example for detailed explanation)
    glob.annz["inAsciiVars"]  = "D:Z;F:myPhotoZ;F:MAG_U;F:MAG_G;F:MAG_R;F:MAG_I;F:MAG_Z"
    # input dataset
    glob.annz["inAsciiFiles"] = "boss_dr10_refForKnnErr.csv"

    # --------------------------------------------------------------------------------------------------
    # possibly also use a reference dataset to derive weights which will be used as part of the
    # knn error estimation
    # --------------------------------------------------------------------------------------------------
    #   - If [useWgtKNN==True] use the reference dataset to derive weights
    #   - see scripts/annz_rndReg_weights.py for detailed job options for [useWgtKNN==True].
    # --------------------------------------------------------------------------------------------------
    useWgtKNN = False
    if useWgtKNN:
      glob.annz["useWgtKNN"]             = True
      glob.annz["inAsciiFiles_wgtKNN"]   = "boss_dr10_refForReWeight.csv"
      glob.annz["inAsciiVars_wgtKNN"]    = "F:MAG_U;F:MAGERR_U;F:MAG_G;F:MAGERR_G;" + \
                                           "F:MAG_R;F:MAGERR_R;F:MAG_I;F:MAGERR_I;F:MAG_Z;F:MAGERR_Z"
      glob.annz["weightInp_wgtKNN"]      = "1/pow(MAG_G*MAG_U*MAG_R*MAG_I, 1.5)"
      glob.annz["weightRef_wgtKNN"]      = "1"
      glob.annz["weightVarNames_wgtKNN"] = "MAG_U;MAG_G;MAG_R;MAG_I;MAG_Z"

    # run ANNZ with the current settings
    runANNZ()


  # --------------------------------------------------------------------------------------------------
  # - generate the knn-errors for a given dataset
  # --------------------------------------------------------------------------------------------------
  if glob.annz["doEval"]:
    # inDirName,inAsciiFiles - directory with files to make the calculations from, and list of input files
    glob.annz["inDirName"]      = "examples/data/photoZ/knnErr"
    glob.annz["inAsciiFiles"]   = "boss_dr10_evalForKnnErr.csv"

    # inAsciiVars - list of parameters in the input files (doesnt need to be exactly the same as
    #               in doGenInputTrees, but must contain all of the parameters which were used for training)
    glob.annz["inAsciiVars"]    = "F:MAG_U;F:MAG_G;F:MAG_R;F:MAG_I;F:MAG_Z"

    # the variables used for the knn-search (can be functional expressions containing any of the input variables)
    # does not need to be the same observables which were used to derive the original [zReg_onlyKnnErr], though
    # that would certainly be recommended
    glob.annz["knnVars_onlyKnnErr"] = "MAG_U;MAG_G;(MAG_G-MAG_R);(MAG_R-MAG_I);(MAG_I-MAG_Z)"

    # set the number of near-neighbours used to compute the KNN error estimator
    glob.annz["nErrKNN"] = 100 # should be around ~100

    useMoreOpts = False
    if useMoreOpts:
      # optional cut on the reference sample for the knn-calculation
      glob.annz["cuts_onlyKnnErr"]    = "MAG_I<23"
      # optional weights applied on the reference sample for the knn-calculation
      glob.annz["weights_onlyKnnErr"] = "MAG_G"
      # add parameters included in the evaluated dataset to the output
      glob.annz["addOutputVars"]      = "MAG_U;MAG_G;"
      # add a postfix to the name of the output directory (so that we may e.g., evaluate
      # multiple input datasets without overriding the directories)
      glob.annz["evalDirPostfix"]     = "_myPostfix"

    # run ANNZ with the current settings
    runANNZ()

log.info(whtOnBlck(" - "+time.strftime("%d/%m/%y %H:%M:%S")+" - finished running ANNZ !"))

