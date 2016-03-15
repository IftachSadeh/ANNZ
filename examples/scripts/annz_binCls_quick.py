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
#     python annz_binCls_quick.py --binnedClassification --genInputTrees
#     python annz_binCls_quick.py --binnedClassification --train
#     python annz_binCls_quick.py --binnedClassification --verify
#     python annz_binCls_quick.py --binnedClassification --evaluate
# --------------------------------------------------------------------------------------------------
log.info(whtOnBlck(" - "+time.strftime("%d/%m/%y %H:%M:%S")+" - starting ANNZ"))

# --------------------------------------------------------------------------------------------------
# general options which are the same for all stages
#   - PLEASE ALSO INSPECT generalSettings(), WHICH HAS BEEN RUN AS PART OF init(), FOR MORE OPTIONS
# --------------------------------------------------------------------------------------------------
# outDirName - set output directory name
glob.annz["outDirName"] = "test_binCls_quick"

# zTrg            - the name of the target variable of the regression
# minValZ,maxValZ - the minimal and maximal values of the target variable (zTrg)
glob.annz["zTrg"]       = "Z"
glob.annz["minValZ"]    = 0.05
glob.annz["maxValZ"]    = 0.8

# --------------------------------------------------------------------------------------------------
# pre-processing of the input dataset
# --------------------------------------------------------------------------------------------------
if glob.annz["doGenInputTrees"]:
  # inDirName    - directory in which input files are stored
  glob.annz["inDirName"]      = "examples/data/photoZ/train"

  # inAsciiVars  - list of parameter types and parameter names, corresponding to columns in the input
  #                file, e.g., [TYPE:NAME] may be [F:MAG_U], with 'F' standing for float. (see advanced example for detailed explanation)
  glob.annz["inAsciiVars"]    = "F:MAG_U;F:MAGERR_U;F:MAG_G;F:MAGERR_G;F:MAG_R;F:MAGERR_R;F:MAG_I;F:MAGERR_I;F:MAG_Z;F:MAGERR_Z;D:Z"

  # splitTypeTrain - list of files for training and testing. the entire dataset is split into two parts (one for
  # each subsample), where splitting is determined by the [splitType="serial"] criteria. (see advanced example for more options/detials.)
  glob.annz["splitType"]      = "serial" # "serial", "blocks" or "random"
  glob.annz["inAsciiFiles"]   = "boss_dr10_0_large.csv;boss_dr10_1_large.csv;boss_dr10_2_large.csv"
  # run ANNZ with the current settings
  runANNZ()

# --------------------------------------------------------------------------------------------------
# training, optimization and evaluation
# --------------------------------------------------------------------------------------------------
if glob.annz["doTrain"] or glob.annz["doVerif"] or glob.annz["doEval"]:

  # binCls_nBins,binCls_maxBinW - 
  #   the range, [minValZ,maxValZ] is divided into binCls_nBins classification bins, such that the number
  #   of objects is approximately equal in each bin, and bins are constrained to have a width
  #   smaller than binCls_maxBinW.
  # --------------------------------------------------------------------------------------------------
  glob.annz["binCls_nBins"]   = 60
  glob.annz["binCls_maxBinW"] = 0.02

  # --------------------------------------------------------------------------------------------------
  # training
  # --------------------------------------------------------------------------------------------------
  if glob.annz["doTrain"]:

    # generate binCls_nTries different randomized MLMs for each bin, and use only the one which has the highest separation parameter
    glob.annz["binCls_nTries"] = 30

    # inputVariables - semicolon-separated list of input variables for the MLMs. Can include math expressions of the variables
    # given in inAsciiVars (see https://root.cern.ch/root/html520/TFormula.html for examples of valid math expressions)
    glob.annz["inputVariables"] = "MAG_U;(MAG_U-MAG_G);(MAG_G-MAG_R);(MAG_R-MAG_I);(MAG_I-MAG_Z)"

    # rndOptTypes - generate these randomized MLM types (currently "ANN", "BDT" or "ANN_BDT" are supported).
    glob.annz["rndOptTypes"] = "BDT" # BDT is better at classification than ANN, and is also much faster to train

    # setting binCls_bckShiftMin,binCls_bckShiftMax to some values with (binCls_bckShiftMax > binCls_bckShiftMin)
    # will exclude a range of background values from the training. (see advanced example for details.)
    # --------------------------------------------------------------------------------------------------
    glob.annz["binCls_bckShiftMin"] = 0.06
    glob.annz["binCls_bckShiftMax"] = 0.1

    # - Setting binCls_bckSubsetRange to value of format "x_y_p" will reject background objects from training,
    #   such that the ratio of background to signal is in the range between x and y. This will happen
    #   for p% of the requested binCls_nTries. (see advanced example for details.)
    # - in this example, 99% of the time, the number of background objects will be between 5 to 10 times 
    #   the number of signal objects. the other 1% of cases, all background objects will be used
    # --------------------------------------------------------------------------------------------------
    glob.annz["binCls_bckSubsetRange"] = "5_10_99"

    # can place here specific randomization settings, cuts and weights (see advanced example for details)
    # if this is left as is, then random job options are generated internally in ANNZ, using MLM types
    # given by rndOptTypes. see ANNZ::generateOptsMLM().
    # ....
    # --------------------------------------------------------------------------------------------------

    for nBinNow in range(glob.annz["binCls_nBins"]):
      glob.annz["nBinNow"] = nBinNow;
      if glob.annz["trainIndex"] >= 0 and glob.annz["trainIndex"] != nBinNow: continue

      # run ANNZ with the current settings
      runANNZ()
    
  # --------------------------------------------------------------------------------------------------
  # optimization and evaluation
  # --------------------------------------------------------------------------------------------------
  if glob.annz["doVerif"] or glob.annz["doEval"]:

    # nPDFs - number of PDFs (up to two PDF types are implemented, the second one is experimental)
    glob.annz["nPDFs"]    = 1
    # nPDFbins - number of PDF bins, with equal width bins between minValZ and maxValZ. (see advanced example
    #            for setting other bin configurations) this is not directly tied to binCls_nBins -> the results of 
    #            the classification bins are cast into whatever final PDF bin scheme is defined by nPDFbins.
    glob.annz["nPDFbins"] = 30

    # --------------------------------------------------------------------------------------------------
    # optimization
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

log.info(whtOnBlck(" - "+time.strftime("%d/%m/%y %H:%M:%S")+" - finished runing ANNZ !"))

