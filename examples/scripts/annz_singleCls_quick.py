from annz.helperFuncs import *

# command line arguments and basic settings
# --------------------------------------------------------------------------------------------------
init()

# just in case... (may comment this out)
if not glob.annz["doSingleCls"]:
  log.info(red(" - "+time.strftime("%d/%m/%y %H:%M:%S")+" - This scripts is only designed for singleClassification...")) ; sys.exit(0)

# ==================================================================================================
# The main code - single classification -
# --------------------------------------------------------------------------------------------------
#   - run the following:
#     python annz_singleCls.py --singleClassification --genInputTrees
#     python annz_singleCls.py --singleClassification --train
#     python annz_singleCls.py --singleClassification --optimize
#     python annz_singleCls.py --singleClassification --evaluate
# --------------------------------------------------------------------------------------------------
log.info(whtOnBlck(" - "+time.strftime("%d/%m/%y %H:%M:%S")+" - starting ANNZ"))

# --------------------------------------------------------------------------------------------------
# general options which are the same for all stages
#   - PLEASE ALSO INSPECT generalSettings(), WHICH HAS BEEN RUN AS PART OF init(), FOR MORE OPTIONS
# --------------------------------------------------------------------------------------------------
# outDirName - set output directory name
glob.annz["outDirName"]   = "test_singleCls_quick"

# nMLMs - the number of random MLMs to generate - for single classification, this must be [1]
glob.annz["nMLMs"]        = 1

# the definition of signal and background classes
glob.annz["userCuts_sig"] = "type == 3";
glob.annz["userCuts_bck"] = "type == 6";

# --------------------------------------------------------------------------------------------------
# pre-processing of the input dataset
# --------------------------------------------------------------------------------------------------
if glob.annz["doGenInputTrees"]:
  # inDirName    - directory in which input files are stored
  glob.annz["inDirName"]    = "examples/data/sgSeparation/train"

  # inAsciiVars  - list of parameter types and parameter names, corresponding to columns in the input
  #                file, e.g., [TYPE:NAME] may be [F:MAG_U], with 'F' standing for float. (see advanced example for detailed explanation)
  glob.annz["inAsciiVars"]  = "C:class; F:z; UL:objid; F:psfMag_r; F:fiberMag_r; F:modelMag_r; F:petroMag_r; F:petroRad_r; F:petroR50_r; " \
                            + " F:petroR90_r; F:lnLStar_r; F:lnLExp_r; F:lnLDeV_r; F:mE1_r; F:mE2_r; F:mRrCc_r; I:type_r; I:type"

  # --------------------------------------------------------------------------------------------------
  # - inAsciiFiles - list of files for training, testing and validation. objects are selected for each subsample from the entire
  #                  dataset.
  # - splitType    - deteermine the method for splitting the dataset into trainig testing and validation subsamples.
  # - see e.g., annz_rndReg_advanced.py for alternative ways to define input files and split datasets
  # --------------------------------------------------------------------------------------------------
  glob.annz["splitType"]    = "serial" # "serial", "blocks" or "random"
  glob.annz["inAsciiFiles"] = "sgCatalogue_galaxy_0.txt;sgCatalogue_galaxy_1.txt;sgCatalogue_star_0.txt;sgCatalogue_star_1.txt;sgCatalogue_star_3.txt"

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

    # rndOptTypes - generate these randomized MLM types (currently "ANN", "BDT" or "ANN_BDT" are supported).
    glob.annz["rndOptTypes"] = "BDT" # for this example, since BDTs are much faster to train, exclude ANNs...

    # inputVariables - semicolon-separated list of input variables for the MLMs. Can include math expressions of the variables
    # given in inAsciiVars (see https://root.cern.ch/root/html520/TFormula.html for examples of valid math expressions)
    glob.annz["inputVariables"]    = "psfMag_r; fiberMag_r;modelMag_r ;  petroMag_r; petroRad_r; petroR50_r;petroR90_r;" \
                                   + "lnLStar_r;lnLExp_r;lnLDeV_r ; TMath::Log(pow(mE1_r,2))"

    # can place here specific randomization settings, cuts and weights (see advanced example for details)
    # if this is left as is, then random job options are generated internally in ANNZ, using MLM types
    # given by rndOptTypes. see ANNZ::generateOptsMLM().
    # ....
    # --------------------------------------------------------------------------------------------------

    # run ANNZ with the current settings
    runANNZ()

# --------------------------------------------------------------------------------------------------
# optimization and evaluation
# --------------------------------------------------------------------------------------------------
if glob.annz["doOptim"] or glob.annz["doEval"]:

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

    # run ANNZ with the current settings
    runANNZ()

log.info(whtOnBlck(" - "+time.strftime("%d/%m/%y %H:%M:%S")+" - finished running ANNZ !"))

