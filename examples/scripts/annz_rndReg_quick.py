from helperFuncs import *

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
#     python annz_rndReg_quick.py --randomRegression --genInputTrees
#     python annz_rndReg_quick.py --randomRegression --train
#     python annz_rndReg_quick.py --randomRegression --optimize
#     python annz_rndReg_quick.py --randomRegression --evaluate
# --------------------------------------------------------------------------------------------------
log.info(whtOnBlck(" - "+time.strftime("%d/%m/%y %H:%M:%S")+" - starting ANNZ"))

# --------------------------------------------------------------------------------------------------
# general options which are the same for all stages
#   - PLEASE ALSO INSPECT generalSettings(), WHICH HAS BEEN RUN AS PART OF init(), FOR MORE OPTIONS
# --------------------------------------------------------------------------------------------------
# outDirName - set output directory name
glob.annz["outDirName"] = "test_randReg_quick"

# nMLMs - the number of random MLMs to generate
glob.annz["nMLMs"]      = 10

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
  # inDirName    - directory in which input files are stored
  glob.annz["inDirName"]      = "examples/data/photoZ/train"

  # inAsciiVars  - list of parameter types and parameter names, corresponding to columns in the input
  #                file, e.g., [TYPE:NAME] may be [F:MAG_U], with 'F' standing for float. (see advanced example for detailed explanation)
  glob.annz["inAsciiVars"]    = "F:MAG_U;F:MAGERR_U;F:MAG_G;F:MAGERR_G;F:MAG_R;F:MAGERR_R;F:MAG_I;F:MAGERR_I;F:MAG_Z;F:MAGERR_Z;D:Z"

  # splitTypeTrain - list of files for training. splitTypeTest - list of files for testing. splitTypeValid - list of files for validation if
  #                  there is no dedicated validation sample, set [nSplit=2] and ignore splitTypeValid (see advanced example for more options).
  glob.annz["nSplit"]         = 3
  glob.annz["splitTypeTrain"] = "boss_dr10_0.csv"
  glob.annz["splitTypeTest"]  = "boss_dr10_1.csv;boss_dr10_2.csv"
  glob.annz["splitTypeValid"] = "boss_dr10_3.csv" 

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
    glob.annz["inputVariables"] = "MAG_U;MAG_G;(MAG_G-MAG_R);(MAG_R-MAG_I);(MAG_I-MAG_Z)"

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

  # nPDFs - number of PDFs (up to two PDF types are implemented) (see advanced example for a general description of the two PDFs)
  glob.annz["nPDFs"]      = 2
  # nPDFbins - number of PDF bins, with equal width bins between minValZ and maxValZ. (see advanced example for setting other bin configurations)
  glob.annz["nPDFbins"]   = 90

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
    glob.annz["inDirName"]      = "examples/data/photoZ/eval/"
    glob.annz["inAsciiFiles"]   = "boss_dr10_eval1_noZ.csv"
    # inAsciiVars - list of parameters in the input files (doesnt need to be exactly the same as in doGenInputTrees, but must contain all
    #               of the parameers which were used for training)
    glob.annz["inAsciiVars"]    = "F:MAG_U;F:MAGERR_U;F:MAG_G;F:MAGERR_G;F:MAG_R;F:MAGERR_R;F:MAG_I;F:MAGERR_I;F:MAG_Z;F:MAGERR_Z"
    # evalDirPostfix - if not empty, this string will be added to the name of the evaluation directory
    #                  (can be used to prevent multiple evaluation of different input files from overwriting each other)
    glob.annz["evalDirPostfix"] = ""

    # run ANNZ with the current settings
    runANNZ()

log.info(whtOnBlck(" - "+time.strftime("%d/%m/%y %H:%M:%S")+" - finished runing ANNZ !"))

