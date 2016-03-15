import  time,logging
from    math          import floor
import  random        as     rnd
import  commonImports as     glob
from    commonImports import Assert, log, blue, red, green, lBlue, yellow, purple, cyan, whtOnBlck, redOnBlck
from    commonImports import bluOnBlck, yellOnBlck, whtOnRed, yellowOnRed, whtOnYellow, whtOnGreen

# --------------------------------------------------------------------------------------------------
# general settings which may be modified by the user (see myMain::myMain() for
# the complete list of options)
# --------------------------------------------------------------------------------------------------
def generalSettings():  
  # set the output level (by default set to ["INFO"]) - ["ERROR","WARNING","INFO","DEBUG","DEBUG_1","DEBUG_2","DEBUG_3","DEBUG_4"]
  # glob.annz["logLevel"] = "DEBUG_1"

  # set to [False] in order to avoid color output
  # glob.annz["useCoutCol"]  = False

  # if isBatch is [True] then a progress bar is not drawn during training (if writing the output to a log file this is important to avoid)
  # while it may be set from the command line (--isBatch), we can also be override it globally here
  # glob.annz["isBatch"] = True

  # change the frequency in which counters are printed on screen while looping over datasets
  # glob.annz["nObjectsToPrint"] = 1e3

  # set to [False] to suppress plotting (not even root scripts generated)
  # glob.annz["doPlots"] = False
  
  # if empty, will suppress pdf/jpeg/... picture plotting (only root scripts generated).
  # Otherwise, should be set to e.g., ["pdf"] or [.jpg] to choose format
  # glob.annz["printPlotExtension"] = ""

  # width of bins to perform the plotting
  # glob.annz["zPlotBinWidth"] = -1

  # number of bins in zTrg for plotting
  # glob.annz["nDrawBins_zTrg"] = 20

  # set the initial seed for MLM random option generation etc. (set [0] for changing random seeds)
  # glob.annz["initSeedRnd"] = 1

  # by default, if training for a given nMLMnow was succesful, running training again will have
  # no effect. in order to force re-training, set [overwriteExistingTrain=True]
  # glob.annz["overwriteExistingTrain"] = True

  # if (overwriteExistingTrain==false) and had already trained an MLM, don't overwrite the directory and don't retrain
  # glob.annz["overwriteExistingTrain"] = True

  # optimCondReg -
  #   ["sig68" or "bias"] - used for deciding how to rank MLM performance. the named criteria represents
  #   the metric which is more significant in deciding which MLM performs "best" (correspondingly, the bias, the
  #   68th percentile scatter of bias distribution, or the outlier fraction of the latter). The default value is "sig68".
  # glob.annz["optimCondReg"] = "bias"

  # optimWithScaledBias -
  #   If set to `True`, then instead of the bias, `delta == zReg-zTrg`, the expression `deltaScaled == delta/(1+zTrg)`
  #   is used, where zReg is the estimated result of the MLM/PDF and zTrg is the true (target) value.
  #   This affects only the selection of the "best" MLM and the PDF optimization procedure in randomized regression.
  #   E.g., one can set this parameter in order to minimize the value of `deltaScaled` instead of the value of `delta`, or
  #   correspondingly the value of the scatter of `deltaScaled` instead of that of `delta`.
  #   The selection criteria for prioritizing the bias or the scatter remains the parameter `glob.annz["optimCondReg"]`.
  #   This means that optimCondReg can take the value `bias` (for `delta` or `deltaScaled`),
  #   or `sig68` (for the 68th percentile scatter of `delta` or of `deltaScaled`). The default value is False.
  # glob.annz["optimWithScaledBias"] = True

  # use the scaled bias `(zReg-zTrg)/(1+zTrg)` instead of the bias for the figures generated with the plotting
  # routine - does not change any of the optimization procedure or outputs of the code, only the figures.
  # The default value is False.
  # glob.annz["plotWithScaledBias"] = True

  # optimWithMAD -
  #   set to True, so that the MAD (median absolute deviation) is used, instead of the 68th percentile 
  #   of the bias (`sigma_68`). This affects only the selection of the "best" MLM and the PDF optimization 
  #   procedure in randomized regression. The default value is False.
  #   glob.annz["optimWithMAD"] = True

  # number of random MLM weighting schemes to generate as part of getRndMethodBestPDF()
  # glob.annz["nRndPdfWeightTries"] = 50

  # number of random smearing to perform when folding uncertainty estimates into MLM solutions for PDF generation
  # glob.annz["nSmearsRnd"] = 100

  # a lower acceptance bound to check if too few MLMs are trained or if something went wrong with the optimization procedure
  # (e.g., not enough trained MLMs have 'good' combinations of scatter, bias and outlier-fraction metrics).
  # glob.annz["minAcptMLMsForPDFs"] = 5

  # reduce output (especially progress bar) for batch running
  # glob.annz["isBatch"] = True

  # number of near-neighbours to use for the KNN error estimation, see setupKdTreeKNN() - should be around ~100
  # glob.annz["nErrKNN"] = 150

  # The KNN error and weight calculations are nominally performed for rescaled variable distributions; each input variable is
  # mapped by a linear transformation to the range `[-1,1]`, so that the distance in the input parameter space is not biased
  # by the scale (units) of the different parameters. It is possible to prevent the rescalling by setting the following flags;
  # these respectively relate to the KNN error calculation, the reference dataset reweighting, and the training quality-flag.
  # glob.annz["doWidthRescale_errKNN"]  = False
  # glob.annz["doWidthRescale_wgtKNN"]  = False
  # glob.annz["doWidthRescale_inTrain"] = False

  # wether or not to perform a bias-correction on PDFs (by default set to True)
  # glob.annz["doBiasCorPDF"] = False

  # if propagating input-errors - nErrINP is the number of randomly generated MLM values used to propagate
  # the uncertainty on the input parameters to the MLM-estimator. See getRegClsErrINP()
  # glob.annz["nErrINP"] = -1 # if set to [-1], the default value is used

  # maximal number of objects in a tree/output ascii file
  # glob.annz["nObjectsToWrite"] = 1e6

  return

# --------------------------------------------------------------------------------------------------
# example for random training option generation
# --------------------------------------------------------------------------------------------------
def genRndOpts(aSeed):
  rnd.seed(1982516+aSeed)
  rndAr = [rnd.random() for i in range(15)]

  if not "rndOptTypes" in glob.annz: glob.annz["rndOptTypes"] = "BDT"

  rndOptTypes = glob.annz["rndOptTypes"]
  isReg       = glob.annz["doRegression"] and (not glob.annz["doBinnedCls"])

  doANN = "ANN" in rndOptTypes ; doBDT = "BDT" in rndOptTypes
  Assert("Must allow either \"ANN\", \"BDT\" or both in \"rndOptTypes\"",(doANN or doBDT))

  if doANN and doBDT:
    doANN = (rndAr[0] < 0.5) ; doBDT = (not doANN)

  if doANN:
    mlmType  = "ANN"
    nLayer0  = int(floor(rndAr[1]*5)); nLayer1 = int(floor(rndAr[2]*10)); nLayer2 = int(floor(rndAr[3]*5));
    layerArc = "N+"+str(nLayer0)+",N+"+str(nLayer1);
    if(rndAr[1] < 0.3 and rndAr[4] < 0.3): layerArc += ",N+"+str(nLayer2);
    layerArc = layerArc.replace("N+0","N");

    neuronInput = "sum";
    neuronType  = "tanh" if(rndAr[5]  < 0.5) else "sigmoid";
    useReg      = "True" if(rndAr[6]  < 0.5) else "False";
    resetStep   = "100"  if(rndAr[7]  < 0.3) else ( "250" if(rndAr[7] < 0.65) else "500" )
    convTests   = "-1"   if(rndAr[8]  < 0.3) else ( "25"  if(rndAr[8] < 0.65) else "50"  )
    testRate    = "5"    if(rndAr[9]  < 0.3) else ( "25"  if(rndAr[9] < 0.65) else "50"  )
    
    trainMethod = "BFGS" if(rndAr[10] < 0.7 and isReg) else "BP"; # sometimes BFGS crashes for classification ... ?!

    RandomSeed  = ":RandomSeed="+str(int(floor(rndAr[11]*100000)))

    opt = ":HiddenLayers="+layerArc+":NeuronType="+neuronType+":NeuronInputType="+neuronInput  \
          +":TrainingMethod="+trainMethod+":TestRate="+testRate+":NCycles=5000"                \
          +":UseRegulator="+useReg+":ConvergenceTests="+convTests+":ConvergenceImprove=1e-30"  \
          +":SamplingTraining=False:SamplingTesting=False"+":ResetStep="+resetStep+RandomSeed;

  elif doBDT:
    mlmType = "BDT";

    nTreeFact = 3 if(rndAr[1] < 0.2) else 1
    nTreesAdd = int(floor(rndAr[2]*300/10.) * 10) * nTreeFact
    nTrees    = ":NTrees="+str(int(250+max(0,min(nTreesAdd,800))))

    boostType = ":BoostType="
    if  (rndAr[3] < 0.4): boostType += "Bagging";
    elif(rndAr[3] < 0.8): boostType += "AdaBoost";
    else:
      if isReg: boostType += "AdaBoostR2"  # only for regression
      else:     boostType += "Grad"        # only for calssification

    nEventsMin = ":nEventsMin="+str(3+int(floor(rndAr[4]*57)))
    nCuts      = ":nCuts="+str(10+int(floor(rndAr[5]*30/5.)*5)) if(rndAr[6] < 0.3) else ""

    opt = nTrees+boostType #+nEventsMin+nCuts;


  rndAr = [rnd.random() for i in range(2)]

  varTrans = "";
  if(rndAr[0] < 0.9): varTrans += "N";
  if(rndAr[1] < 0.5): varTrans += "P";
  varTrans = varTrans.replace("NP","N,P");
  if(varTrans != ""):  varTrans  = ":VarTransform="+varTrans;

  opt = "ANNZ_MLM="+mlmType+opt+varTrans

  return opt

