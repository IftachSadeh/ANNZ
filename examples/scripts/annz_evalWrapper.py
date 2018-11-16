# --------------------------------------------------------------------------------------------------
# simple example, illustrating how the evaluation python ANNZ should be setup.
# instead of providing an input ascii catalogue, it can be used for
# pipeline integration of ANNZ.
# the generation/training/optimization(verification) stages must be
# run first, as for the nominal evaluation examples (eg scripts/annz_singleReg_quick.py).
# --------------------------------------------------------------------------------------------------
# - the run this script, execute the following:
#   python annz_evalWrapper.py
# --------------------------------------------------------------------------------------------------
# - an example is given for several modes of operation (uncomment one...):
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
testType = 'doSingleReg'
# testType = 'doRandomReg'
# testType = 'doBinnedCls'
# testType = 'doRandomCls' # similarly for singleCls
# --------------------------------------------------------------------------------------------------

from py.ANNZ import ANNZ

# logging module
useLog = True
import logging
logging.basicConfig(format="(%(asctime)s %(levelname)s) %(message)s",datefmt='%H:%M')
log = logging.getLogger('annzLog')

def main():
  opts = dict()

  # ==================================================================================================
  # 1. initial setup:
  #   - do this once before anything else (could take a few seconds, as resources are loaded)
  # ==================================================================================================
  # define some user options
  init(testType, opts)
  
  # instantiate the evaluation manager (with a unique name, and a possible logger)
  name = 'myWrapper'
  annz = ANNZ(name=name, opts=opts, log=log)

  # ==================================================================================================
  # 2. run the evaluation:
  #   - can be called within a loop or in a seperate thread (after the initialization
  #     has completed).
  #   - calls to annz.eval(evalEvt) would be fast, where it is better to group multiple
  #     evaluations in terms of performance
  # ==================================================================================================
  
  # --------------------------------------------------------------------------------------------------
  # example call for a single evaluation (one input object)
  #   - input object is a single dict of input variables
  #     each of the variables listed in opts["inVars"] must be included !!!
  #   - output object is a single dict with the evaluation results
  # --------------------------------------------------------------------------------------------------
  if opts['doRegression']:
    evalIn = {
      'MAG_U':23.242401, 'MAGERR_U':1.231664, 'MAG_G':22.895664, 'MAGERR_G':0.675091, 'MAG_R':21.431746, 'MAGERR_R':0.225735, 'MAG_I':20.430061, 'MAGERR_I':0.111847, 'MAG_Z':20.008024, 'MAGERR_Z':0.108993 
    }
  else:
    evalIn = {
      'class':'GALAXY', 'z':0.03303384, 'objid':1237657633260830851, 'psfMag_r':19.77284, 'fiberMag_r':19.0557, 'modelMag_r':16.70067, 'petroMag_r':16.65162, 'petroRad_r':10.34484, 'petroR50_r':4.416367, 'petroR90_r':10.22645, 'lnLStar_r':-10820.06, 'lnLExp_r':-110.5183, 'lnLDeV_r':-928.4106, 'mE1_r':-0.4171576, 'mE2_r':-0.1005798, 'mRrCc_r':103.0461, 'type_r':3, 'type':3, 
    }

  evalOut = annz.eval(evalIn)
  
  # output is a single dict with the evaluation results
  log.info(
    "\033[32m"+" - single evaluation("+name+")    - ANNZ.lib.wrapperEval: " \
    + str('ANNZ_0:'+str(evalOut['ANNZ_0'])+", " if 'ANNZ_0' in evalOut else "") \
    +''.join([str(k)+':'+str(v)+', ' for n,(k,v) in enumerate(evalOut.items(), 0) if n < 3]) +' ...'+"\033[0m"
  )

  # --------------------------------------------------------------------------------------------------
  # example call for a multiple evaluations (single call with multiple objs as input)
  #   - input object is list of dicts, each one corresponding to one object to be evaluated
  #   - output object is a list of dicts with the corresponding evaluation results
  # --------------------------------------------------------------------------------------------------
  evalInV = loadObjV(opts)

  evalOutV = annz.eval(evalInV)
  
  # output is a list of dicts with the evaluation results
  for evalNow in evalOutV:
    log.info(
      "\033[33m"+" - multiple evaluations("+name+") - ANNZ.lib.wrapperEval: " \
      + str('ANNZ_0:'+str(evalNow['ANNZ_0'])+", " if 'ANNZ_0' in evalNow else "") \
      +''.join([str(k)+':'+str(v)+', ' for n,(k,v) in enumerate(evalNow.items(), 0) if n < 3]) \
      +' ...'+"\033[0m"
    )
  

  # ==================================================================================================
  # 3. cleanup resources when done (recommended but not mandatory):
  #   - do this once - a graceful exit, after the Wrapper object is no longer needed 
  # ==================================================================================================  
  annz.cleanup()

  return

# --------------------------------------------------------------------------------------------------
# initialization of user options
# (the following is the minimal set of options required for evaluation!)
# --------------------------------------------------------------------------------------------------
def init(testType, opts):
  opts['doEval'] = True

  # define the entries in the options dict (one will be overriden next)
  opts['doRegression']     = False
  opts['doClassification'] = False

  # can run evaluation within a read-only system, by setting isReadOnlySys
  # opts['isReadOnlySys'] = True

  # --------------------------------------------------------------------------------------------------
  # can work with single-regression, eg:
  # --------------------------------------------------------------------------------------------------
  if testType == 'doSingleReg':
    opts['doRegression'] = True
    opts['doSingleReg']  = True
    # this assumes that this outDirName was previsoulsy used to train/optimize ...
    opts["outDirName"]   = "test_singleReg_quick"

  # --------------------------------------------------------------------------------------------------
  # can work with randomized-regression, eg:
  # --------------------------------------------------------------------------------------------------
  if testType == 'doRandomReg':
    opts['doRegression'] = True
    opts['doRandomReg']  = True
    opts["outDirName"]   = "test_randReg_quick"

    # numer of PDFs and number of PDF bins 
    #   - nPDFs must be lower/equal to the number used for optimization (can be zero to speed things up...)
    #   - nPDFbins can be anything (not necessarily the same as for optimization)
    opts["nPDFs"]    = 1
    opts["nPDFbins"] = 120

  # --------------------------------------------------------------------------------------------------
  # can work with sbinned-classification, eg:
  # --------------------------------------------------------------------------------------------------
  if testType == 'doBinnedCls':
    opts['doRegression'] = True
    opts['doBinnedCls']  = True
    opts["outDirName"]   = "test_binCls_quick"

    # nPDFbins -
    #   - number of PDF bins, with equal width bins between minValZ and maxValZ. (see advanced example
    #     for setting other bin configurations) this is not directly tied to binCls_nBins -> the results of 
    #     the classification bins are cast into whatever final PDF bin scheme is defined by nPDFbins.
    #     (doesnt have to be the same as for training/validation)
    opts["nPDFbins"] = 160

  if testType == 'doRandomCls':
    opts['doClassification'] = True
    opts['doRandomCls']      = True
    opts["outDirName"]       = "test_randCls_quick"

  # --------------------------------------------------------------------------------------------------
  # list of parameter types and parameter names - THIS MUST INCLUDE any parameter
  # used for training/optimization, including weight/cut expressions
  # the same syntax fo rcomposing this list is used as for the nominal training for "inAsciiVars"
  # e.g., for a given entry, [TYPE:NAME] may be [F:MAG_U], with 'F' standing for float.
  # --------------------------------------------------------------------------------------------------
  if opts['doRegression']:
    opts["inVars"] = "F:MAG_U;F:MAGERR_U;F:MAG_G;F:MAGERR_G;F:MAG_R;F:MAGERR_R;F:MAG_I;F:MAGERR_I;F:MAG_Z;F:MAGERR_Z"
  else:
    opts["inVars"]  = "C:class; F:z; UL:objid; F:psfMag_r; F:fiberMag_r; F:modelMag_r; " \
                      + "F:petroMag_r; F:petroRad_r; F:petroR50_r;  F:petroR90_r; F:lnLStar_r; F:lnLExp_r; " \
                      + "F:lnLDeV_r; F:mE1_r; F:mE2_r; F:mRrCc_r; I:type_r; I:type"

  # --------------------------------------------------------------------------------------------------
  # (increase the logging level to get more info as stuff gets loaded...)
  # --------------------------------------------------------------------------------------------------
  # opts['logLevel'] = 'DEBUG_1'
  if 'logLevel' not in opts:
    logLevel = 'INFO'
  elif opts["logLevel"].startswith("DEBUG_"):
    logLevel = 'DEBUG'
  log.setLevel(logLevel.upper())

  return

# --------------------------------------------------------------------------------------------------
# helper function to set data for some objects to be evaluated
# --------------------------------------------------------------------------------------------------
def loadObjV(opts):
  evalObjV = []
  if opts['doRegression']:
    evalObjV += [{
      'MAG_U':23.242401, 'MAGERR_U':1.231664, 'MAG_G':22.895664, 'MAGERR_G':0.675091, 'MAG_R':21.431746, 'MAGERR_R':0.225735, 'MAG_I':20.430061, 'MAGERR_I':0.111847, 'MAG_Z':20.008024, 'MAGERR_Z':0.108993 
    }]
    evalObjV += [{
      'MAG_U':27.269310, 'MAGERR_U':50.263824, 'MAG_G':23.438055, 'MAGERR_G':1.112548, 'MAG_R':21.721695, 'MAGERR_R':0.294834, 'MAG_I':20.726177, 'MAGERR_I':0.146916, 'MAG_Z':20.221432, 'MAGERR_Z':0.132667
    }]
    evalObjV += [{
      'MAG_U':23.089993, 'MAGERR_U':1.070355, 'MAG_G':21.347996, 'MAGERR_G':0.162291, 'MAG_R':19.574608, 'MAGERR_R':0.040808, 'MAG_I':18.993824, 'MAGERR_I':0.029794, 'MAG_Z':18.625031, 'MAGERR_Z':0.030493
    }]
    evalObjV += [{
      'MAG_U':25.656546, 'MAGERR_U':11.380192, 'MAG_G':22.774303, 'MAGERR_G':0.603695, 'MAG_R':20.914745, 'MAGERR_R':0.140216, 'MAG_I':19.971313, 'MAGERR_I':0.073303, 'MAG_Z':19.469351, 'MAGERR_Z':0.066364
    }]
    evalObjV += [{
      'MAG_U':22.602184, 'MAGERR_U':0.682975, 'MAG_G':22.517540, 'MAGERR_G':0.476555, 'MAG_R':21.284885, 'MAGERR_R':0.197176, 'MAG_I':19.931887, 'MAGERR_I':0.070689, 'MAG_Z':19.524708, 'MAGERR_Z':0.069835
    }]
    evalObjV += [{
      'MAG_U':24.791594, 'MAGERR_U':5.130606, 'MAG_G':24.431559, 'MAGERR_G':2.777920, 'MAG_R':22.353245, 'MAGERR_R':0.527471, 'MAG_I':20.834749, 'MAGERR_I':0.162367, 'MAG_Z':20.455927, 'MAGERR_Z':0.164650
    }]
    evalObjV += [{
      'MAG_U':24.208023, 'MAGERR_U':2.997373, 'MAG_G':22.879683, 'MAGERR_G':0.665226, 'MAG_R':21.355974, 'MAGERR_R':0.210518, 'MAG_I':20.203375, 'MAGERR_I':0.090771, 'MAG_Z':19.677320, 'MAGERR_Z':0.080375
    }]
    evalObjV += [{
      'MAG_U':22.970003, 'MAGERR_U':0.958369, 'MAG_G':22.563587, 'MAGERR_G':0.497200, 'MAG_R':20.818642, 'MAGERR_R':0.128338, 'MAG_I':20.025190, 'MAGERR_I':0.077033, 'MAG_Z':19.577642, 'MAGERR_Z':0.073324
    }]
    evalObjV += [{
      'MAG_U':22.993204, 'MAGERR_U':0.979068, 'MAG_G':21.417919, 'MAGERR_G':0.173087, 'MAG_R':19.571741, 'MAGERR_R':0.040700, 'MAG_I':18.955645, 'MAGERR_I':0.028764, 'MAG_Z':18.565395, 'MAGERR_Z':0.028864
    }]
    evalObjV += [{
      'MAG_U':24.510849, 'MAGERR_U':3.961605, 'MAG_G':23.524729, 'MAGERR_G':1.205003, 'MAG_R':21.257866, 'MAGERR_R':0.192330, 'MAG_I':20.351210, 'MAGERR_I':0.104012, 'MAG_Z':19.945398, 'MAGERR_Z':0.102885
    }]
    evalObjV += [{
      'MAG_U':22.702892, 'MAGERR_U':0.749356, 'MAG_G':22.516407, 'MAGERR_G':0.476057, 'MAG_R':20.714643, 'MAGERR_R':0.116616, 'MAG_I':19.615702, 'MAGERR_I':0.052830, 'MAG_Z':19.130394, 'MAGERR_Z':0.048568
    }]
    evalObjV += [{
      'MAG_U':23.854374, 'MAGERR_U':2.164118, 'MAG_G':23.893440, 'MAGERR_G':1.692281, 'MAG_R':21.612862, 'MAGERR_R':0.266713, 'MAG_I':20.209888, 'MAGERR_I':0.091318, 'MAG_Z':19.779684, 'MAGERR_Z':0.088321
    }]
    evalObjV += [{
      'MAG_U':23.077784, 'MAGERR_U':1.058388, 'MAG_G':23.043072, 'MAGERR_G':0.773259, 'MAG_R':21.724533, 'MAGERR_R':0.295606, 'MAG_I':20.630796, 'MAGERR_I':0.134560, 'MAG_Z':20.069298, 'MAGERR_Z':0.115321
    }]
    evalObjV += [{
      'MAG_U':22.493427, 'MAGERR_U':0.617878, 'MAG_G':20.385187, 'MAGERR_G':0.066861, 'MAG_R':18.860731, 'MAGERR_R':0.021144, 'MAG_I':18.316992, 'MAGERR_I':0.015973, 'MAG_Z':17.976097, 'MAGERR_Z':0.016774
    }]
    evalObjV += [{
      'MAG_U':22.633821, 'MAGERR_U':0.703169, 'MAG_G':22.643038, 'MAGERR_G':0.534948, 'MAG_R':21.291964, 'MAGERR_R':0.198466, 'MAG_I':20.263699, 'MAGERR_I':0.095957, 'MAG_Z':19.681189, 'MAGERR_Z':0.080661
    }]

  else:
    evalObjV += [{
      'class':'GALAXY', 'z':0.03303384, 'objid':1237657633260830851, 'psfMag_r':19.77284, 'fiberMag_r':19.0557, 'modelMag_r':16.70067, 'petroMag_r':16.65162, 'petroRad_r':10.34484, 'petroR50_r':4.416367, 'petroR90_r':10.22645, 'lnLStar_r':-10820.06, 'lnLExp_r':-110.5183, 'lnLDeV_r':-928.4106, 'mE1_r':-0.4171576, 'mE2_r':-0.1005798, 'mRrCc_r':103.0461, 'type_r':3, 'type':3, 
    }]
    evalObjV += [{
     'class':'GALAXY',  'z':0.005773766,  'objid':1237658204497510547,  'psfMag_r':22.06691,  'fiberMag_r':21.01414,  'modelMag_r':21.87498,  'petroMag_r':21.94661,  'petroRad_r':1.318108,  'petroR50_r':0.6486785,  'petroR90_r':1.244854,  'lnLStar_r':-1.552897E-09,  'lnLExp_r':0,  'lnLDeV_r':0,  'mE1_r':0.06221469,  'mE2_r':0.1122612,  'mRrCc_r':3.602013,  'type_r':3,  'type':6, 
    }]
    evalObjV += [{
     'class':'GALAXY',  'z':0.006665626,  'objid':1237657633798029411,  'psfMag_r':20.08825,  'fiberMag_r':19.19922,  'modelMag_r':14.3057,  'petroMag_r':14.37642,  'petroRad_r':47.41063,  'petroR50_r':19.36942,  'petroR90_r':41.54013,  'lnLStar_r':-11142.03,  'lnLExp_r':-271.805,  'lnLDeV_r':-2866.475,  'mE1_r':-0.639175,  'mE2_r':-0.1294328,  'mRrCc_r':3220.916,  'type_r':3,  'type':3, 
    }]
    evalObjV += [{
     'class':'GALAXY',  'z':0.03321056,  'objid':1237657587098583341,  'psfMag_r':20.5661,  'fiberMag_r':19.98482,  'modelMag_r':18.50321,  'petroMag_r':18.45575,  'petroRad_r':4.994434,  'petroR50_r':2.342355,  'petroR90_r':5.130303,  'lnLStar_r':-131.3283,  'lnLExp_r':-0.002132118,  'lnLDeV_r':-18.9038,  'mE1_r':-0.7127562,  'mE2_r':0.3379882,  'mRrCc_r':53.78413,  'type_r':3,  'type':3, 
    }]
    evalObjV += [{
     'class':'GALAXY',  'z':0.04683965,  'objid':1237658204499345625,  'psfMag_r':18.58671,  'fiberMag_r':18.39965,  'modelMag_r':15.55714,  'petroMag_r':15.88162,  'petroRad_r':14.01934,  'petroR50_r':6.122344,  'petroR90_r':15.43166,  'lnLStar_r':-29937.61,  'lnLExp_r':-4371.197,  'lnLDeV_r':-2043.866,  'mE1_r':0.6341462,  'mE2_r':0.164976,  'mRrCc_r':79.98653,  'type_r':3,  'type':3, 
    }]
    evalObjV += [{
     'class':'GALAXY',  'z':0.04713015,  'objid':1237657634336800992,  'psfMag_r':19.99623,  'fiberMag_r':19.26587,  'modelMag_r':17.52621,  'petroMag_r':17.51039,  'petroRad_r':5.968755,  'petroR50_r':2.728045,  'petroR90_r':6.15565,  'lnLStar_r':-1703.759,  'lnLExp_r':-6.497786,  'lnLDeV_r':-333.1309,  'mE1_r':0.7158026,  'mE2_r':0.04406798,  'mRrCc_r':69.30363,  'type_r':3,  'type':3, 
    }]
    evalObjV += [{
     'class':'GALAXY',  'z':0.007376629,  'objid':1237656495644344517,  'psfMag_r':18.90098,  'fiberMag_r':18.15701,  'modelMag_r':14.01287,  'petroMag_r':14.03227,  'petroRad_r':36.93092,  'petroR50_r':16.06482,  'petroR90_r':43.81779,  'lnLStar_r':-40680.95,  'lnLExp_r':-1187.774,  'lnLDeV_r':-7225.69,  'mE1_r':-0.3491322,  'mE2_r':-0.7903082,  'mRrCc_r':2081.673,  'type_r':3,  'type':3, 
    }]
    evalObjV += [{
     'class':'GALAXY',  'z':0.0327869,  'objid':1237657634337325275,  'psfMag_r':17.72622,  'fiberMag_r':17.47259,  'modelMag_r':15.66522,  'petroMag_r':15.83927,  'petroRad_r':6.647578,  'petroR50_r':3.033248,  'petroR90_r':8.015042,  'lnLStar_r':-37558.7,  'lnLExp_r':-5409.944,  'lnLDeV_r':-1871.12,  'mE1_r':0.05587614,  'mE2_r':-0.2343361,  'mRrCc_r':11.3626,  'type_r':3,  'type':3, 
    }]
    evalObjV += [{
     'class':'STAR',  'z':0.0002064934,  'objid':1237648721764220976,  'psfMag_r':16.77034,  'fiberMag_r':17.08717,  'modelMag_r':16.77277,  'petroMag_r':16.82192,  'petroRad_r':1.051109,  'petroR50_r':0.540996,  'petroR90_r':1.183637,  'lnLStar_r':-10.98614,  'lnLExp_r':-11.89835,  'lnLDeV_r':-11.90376,  'mE1_r':-0.156887,  'mE2_r':-0.009762129,  'mRrCc_r':2.206376,  'type_r':6,  'type':6, 
    }]
    evalObjV += [{
     'class':'STAR',  'z':2.885607E-05,  'objid':1237648721764286799,  'psfMag_r':19.91815,  'fiberMag_r':20.24136,  'modelMag_r':19.91276,  'petroMag_r':19.97802,  'petroRad_r':1.104685,  'petroR50_r':0.568235,  'petroR90_r':1.229363,  'lnLStar_r':-0.4271297,  'lnLExp_r':-0.6142446,  'lnLDeV_r':-0.6140305,  'mE1_r':-0.1693342,  'mE2_r':-0.08308215,  'mRrCc_r':2.516321,  'type_r':6,  'type':6, 
    }]
    evalObjV += [{
     'class':'STAR',  'z':0.000127408,  'objid':1237648721764745241,  'psfMag_r':16.90057,  'fiberMag_r':17.22888,  'modelMag_r':16.89169,  'petroMag_r':16.9471,  'petroRad_r':1.196535,  'petroR50_r':0.6148058,  'petroR90_r':1.350501,  'lnLStar_r':-0.03991538,  'lnLExp_r':-0.01215911,  'lnLDeV_r':-0.01194226,  'mE1_r':-0.1434815,  'mE2_r':-0.0548901,  'mRrCc_r':2.907704,  'type_r':6,  'type':6, 
    }]
    evalObjV += [{
     'class':'STAR',  'z':-8.534989E-06,  'objid':1237648721764745300,  'psfMag_r':14.98866,  'fiberMag_r':15.31146,  'modelMag_r':14.97074,  'petroMag_r':15.03168,  'petroRad_r':1.159345,  'petroR50_r':0.6008313,  'petroR90_r':1.310971,  'lnLStar_r':-79.05621,  'lnLExp_r':-67.17168,  'lnLDeV_r':-66.8935,  'mE1_r':-0.1495259,  'mE2_r':-0.02673536,  'mRrCc_r':2.731805,  'type_r':6,  'type':6, 
    }]
    evalObjV += [{
     'class':'STAR',  'z':2.838524E-05,  'objid':1237648721765007429,  'psfMag_r':17.03375,  'fiberMag_r':17.38084,  'modelMag_r':17.04352,  'petroMag_r':17.10255,  'petroRad_r':1.164766,  'petroR50_r':0.5998273,  'petroR90_r':1.331746,  'lnLStar_r':-2.345536,  'lnLExp_r':-2.994244,  'lnLDeV_r':-2.997238,  'mE1_r':-0.1172282,  'mE2_r':-0.02568189,  'mRrCc_r':2.697967,  'type_r':6,  'type':6, 
    }]
    evalObjV += [{
     'class':'STAR',  'z':0.0004641676,  'objid':1237648721765138565,  'psfMag_r':18.70201,  'fiberMag_r':19.01781,  'modelMag_r':18.70161,  'petroMag_r':18.76492,  'petroRad_r':1.138641,  'petroR50_r':0.5829043,  'petroR90_r':1.276404,  'lnLStar_r':-0.008633397,  'lnLExp_r':-0.03469351,  'lnLDeV_r':-0.03458754,  'mE1_r':-0.09502023,  'mE2_r':-0.01182979,  'mRrCc_r':2.577004,  'type_r':6,  'type':6, 
    }]
    evalObjV += [{
     'class':'STAR',  'z':-0.0001639134,  'objid':1237648721765269635,  'psfMag_r':18.02512,  'fiberMag_r':18.34324,  'modelMag_r':18.01134,  'petroMag_r':18.06234,  'petroRad_r':1.153936,  'petroR50_r':0.5936277,  'petroR90_r':1.302546,  'lnLStar_r':-2.642519,  'lnLExp_r':-1.801068,  'lnLDeV_r':-1.794953,  'mE1_r':-0.1487014,  'mE2_r':-0.01129547,  'mRrCc_r':2.637369,  'type_r':6,  'type':6, 
    }]
    evalObjV += [{
      'class':'STAR', 'z':-3.684801E-05, 'objid':1237648721765400664, 'psfMag_r':16.28859, 'fiberMag_r':16.60587, 'modelMag_r':16.28181, 'petroMag_r':16.33114, 'petroRad_r':1.136989, 'petroR50_r':0.5890708, 'petroR90_r':1.278394, 'lnLStar_r':-3.305526, 'lnLExp_r':-1.510656, 'lnLDeV_r':-1.545843, 'mE1_r':-0.1760481, 'mE2_r':0.002540808, 'mRrCc_r':2.633093, 'type_r':6, 'type':6, 
    }]

  return evalObjV

# --------------------------------------------------------------------------------------------------
# run the main function on execution of this script
# --------------------------------------------------------------------------------------------------
if __name__ == '__main__':
  main()
# --------------------------------------------------------------------------------------------------
