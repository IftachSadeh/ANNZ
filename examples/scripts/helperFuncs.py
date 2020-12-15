import  sys,os,time,logging,argparse,subprocess
from    math                    import floor
# import  ast
# import  ctypes
# from    ctypes                  import cdll
from    scripts.generalSettings import *
import  scripts.commonImports   as     glob
from    scripts.commonImports   import Assert, log, blue, red, green, lBlue, yellow, purple, cyan, whtOnBlck, redOnBlck
from    scripts.commonImports   import bluOnBlck, yellOnBlck, whtOnRed, yellowOnRed, whtOnYellow, whtOnGreen

# --------------------------------------------------------------------------------------------------
# command line arguments and basic settings
# --------------------------------------------------------------------------------------------------
def init():
  initParse()
  setCols()
  generalSettings()
  initROOT()
  doMake()


# --------------------------------------------------------------------------------------------------
# parse user-input options:
# (bool inputs: if set as store_true, the default is False, and the variable is set as true if given in the command line)
# --------------------------------------------------------------------------------------------------
def initParse():
  parser = argparse.ArgumentParser(description="Command line parser:")
  parser.add_argument("--make",                 action='store_true')
  parser.add_argument("--clean",                action='store_true')
  parser.add_argument("--train",                action='store_true')
  parser.add_argument("--optimize",             action='store_true')
  parser.add_argument("--verify",               action='store_true')
  parser.add_argument("--evaluate",             action='store_true')  
  parser.add_argument("--qsub",                 action='store_true')
  parser.add_argument("--genInputTrees",        action='store_true')
  parser.add_argument("--singleRegression",     action='store_true')
  parser.add_argument("--randomRegression",     action='store_true')
  parser.add_argument("--binnedClassification", action='store_true')
  parser.add_argument("--singleClassification", action='store_true')
  parser.add_argument("--randomClassification", action='store_true')
  parser.add_argument("--onlyKnnErr",           action='store_true')
  parser.add_argument("--inTrainFlag",          action='store_true')
  parser.add_argument("--truncateLog",          action='store_true')
  parser.add_argument("--isBatch",              action='store_true')
  parser.add_argument("--fitsToAscii",          action='store_true')
  parser.add_argument("--asciiToFits",          action='store_true')
  parser.add_argument("--logFileName",          type=str,   default="")
  parser.add_argument("--logLevel",             type=str,   default="INFO")
  parser.add_argument("--generalOptS",          type=str,   default="NULL")
  parser.add_argument("--makeOpt",              type=str,   default="NULL")
  parser.add_argument("--maxNobj",              type=float, default=0)
  parser.add_argument("--trainIndex",           type=int,   default=-1)
  parser.add_argument("--generalOptI",          type=int,   default=-1)

  glob.pars = vars(parser.parse_args())

  initLogger()

  # sanity check of user options
  # --------------------------------------------------------------------------------------------------
  hasMake = (glob.pars["make"] or glob.pars["clean"])
  nModes = 0 ; nSetups = 0

  # if glob.pars["genInputTrees"]:        nSetups += 1
  if glob.pars["singleClassification"]: nSetups += 1
  if glob.pars["randomClassification"]: nSetups += 1
  if glob.pars["singleRegression"]:     nSetups += 1
  if glob.pars["randomRegression"]:     nSetups += 1
  if glob.pars["binnedClassification"]: nSetups += 1
  if glob.pars["onlyKnnErr"]:           nSetups += 1
  if glob.pars["inTrainFlag"]:          nSetups += 1
  if glob.pars["fitsToAscii"]:          nSetups += 1
  if glob.pars["asciiToFits"]:          nSetups += 1

  if not ((nSetups == 1) or (nSetups == 0 and (glob.pars["genInputTrees"] or hasMake or glob.pars["qsub"]))):
    log.warning("Possibly missing/conflicting job-options... Did you define: --singleClassification" \
                + " --randomClassification , --singleRegression --randomRegression, --binnedClassification, " \
                +"--onlyKnnErr, --inTrainFlag, --fitsToAscii, --asciiToFits .... ?")

  if nSetups == 1:
    if glob.pars["genInputTrees"]: nModes += 1
    if glob.pars["train"]:         nModes += 1
    if glob.pars["optimize"]:      nModes += 1
    if glob.pars["verify"]:        nModes += 1
    if glob.pars["evaluate"]:      nModes += 1
    if glob.pars["onlyKnnErr"]:    nModes += 1
    if glob.pars["inTrainFlag"]:   nModes += 1
    if glob.pars["fitsToAscii"]:   nModes += 1
    if glob.pars["asciiToFits"]:   nModes += 1

    if not (nModes == 1 or hasMake):
      log.warning("Should define exactly one of --genInputTrees --train , --optimize --verify, --evaluate, --onlyKnnErr, --inTrainFlag, --fitsToAscii, --asciiToFits !")

  glob.pars["onlyMake"] = (((nSetups == 0) or (nModes == 0)) and hasMake and (not glob.pars["genInputTrees"]))

  # add make option, e.g., "-j4"
  glob.makeOpt = glob.pars["makeOpt"] if glob.pars["makeOpt"] is not "NULL" else ""

  # set basic values for operational flags
  # --------------------------------------------------------------------------------------------------
  glob.annz["doGenInputTrees"]  = glob.pars["genInputTrees"]

  glob.annz["doSingleReg"]      = glob.pars["singleRegression"]
  glob.annz["doRandomReg"]      = glob.pars["randomRegression"]
  glob.annz["doBinnedCls"]      = glob.pars["binnedClassification"]

  glob.annz["doSingleCls"]      = glob.pars["singleClassification"]
  glob.annz["doRandomCls"]      = glob.pars["randomClassification"]

  glob.annz["doTrain"]          = glob.pars["train"]
  glob.annz["doOptim"]          = glob.pars["optimize"]
  glob.annz["doVerif"]          = glob.pars["verify"]
  glob.annz["doEval"]           = glob.pars["evaluate"]
  glob.annz["doOnlyKnnErr"]     = glob.pars["onlyKnnErr"]
  glob.annz["doInTrainFlag"]    = glob.pars["inTrainFlag"]

  glob.annz["doRegression"]     = glob.annz["doSingleReg"] or glob.annz["doRandomReg"] or glob.annz["doBinnedCls"]
  glob.annz["doClassification"] = glob.annz["doSingleCls"] or glob.annz["doRandomCls"]

  glob.annz["maxNobj"]          = int(floor(glob.pars["maxNobj"])) # limit number of used objects - used for debugging
  glob.annz["trainIndex"]       = glob.pars["trainIndex"]          # used for python batch-job submision

  glob.annz["doFitsToAscii"]    = glob.pars["fitsToAscii"]
  glob.annz["doAsciiToFits"]    = glob.pars["asciiToFits"]

  # general-use options for developers
  glob.annz["generalOptS"]      = glob.pars["generalOptS"]
  glob.annz["generalOptI"]      = glob.pars["generalOptI"]

  # default values for options which should be overridden in generalSettings()
  # --------------------------------------------------------------------------------------------------
  glob.annz["isBatch"] = (glob.pars["logFileName"] != "" or glob.pars["isBatch"])
  glob.annz["doPlots"] = True
  glob.annz["printPlotExtension"] = "pdf"

# setup the logger
# --------------------------------------------------------------------------------------------------
def initLogger():
  fileMode = "a" if glob.pars["truncateLog"] else "w"
  
  if glob.pars["logFileName"] != "":
    resetDir("./log",False)
    glob.pars["logFileName"]  = "./log/"+glob.pars["logFileName"]
    # glob.pars["logFileName"] += time.strftime("_%d_%m_%y__%H_%M_%S")

    logFileName = glob.pars["logFileName"]+"_python"
    
    logging.basicConfig(filename=logFileName,format="(%(asctime)s %(levelname)s) %(message)s",datefmt='%H:%M',filemode=fileMode)
  else:
    logging.basicConfig(format="(%(asctime)s %(levelname)s) %(message)s",datefmt='%H:%M',filemode=fileMode)
  
  log.setLevel(glob.pars["logLevel"].upper())

# output colors
# --------------------------------------------------------------------------------------------------
def setCols():
  useCoutCol = True
  if useCoutCol:
    glob.ColBlue="\033[34m"              ; glob.ColRed="\033[31m"               ; glob.ColGreen="\033[32m"           ; glob.ColDef="\033[0m"
    glob.ColLightBlue="\033[94m"         ; glob.ColYellow="\033[33m"            ; glob.ColPurple="\033[35m"          ; glob.ColCyan="\033[36m"
    glob.ColUnderLine="\033[4;30m"       ; glob.ColWhiteOnBlack="\33[40;37;1m"  ; glob.ColWhiteOnRed="\33[41;37;1m"
    glob.ColWhiteOnGreen="\33[42;37;1m"  ; glob.ColWhiteOnYellow="\33[43;37;1m"

# --------------------------------------------------------------------------------------------------
# create the directory dirName (or clean it of all content if it already exists and resetOutDir)
# --------------------------------------------------------------------------------------------------
def resetDir(dirName, resetOutDir, verb = True):
  Assert("Tried to resetDir with empty directory name",dirName != "")
  if verb: log.info(blue(" - Resetting directory(")+yellow(dirName)+blue(",")+red(resetOutDir)+blue(")"))
  
  if os.path.isdir(dirName):
    if resetOutDir:
      filesInDir = os.listdir(dirName)
      for fileNow in filesInDir:
        os.system("rm -v "+dirName+"/"+fileNow)
  else:
    os.system("mkdir -vp  "+dirName)


# --------------------------------------------------------------------------------------------------
# add env variables if needed
# --------------------------------------------------------------------------------------------------
def initROOT():

  if "ROOTSYS" in os.environ and glob.useDefinedROOTSYS:
    log.info(blue(" - Found defined ")+green("ROOTSYS")+blue(" = \"")+yellow(os.environ["ROOTSYS"])+blue("\". Setting glob.rootHome to match"))
    glob.rootHome = os.environ["ROOTSYS"]

  rootExeDir  = glob.rootHome+"/bin/"
  rootExe     = rootExeDir+"/root"
  rootHomeLib = glob.rootHome+"/lib/"

  Assert("Found rootHome = "+glob.rootHome+" which does not exist... Please set this in commonImports.py to the" \
         +" ROOT installation directory !",os.path.isdir(glob.rootHome))

  Assert("Did not find ROOT executable, expected as "+rootExe+"  ..." \
         +" Is ROOT properly installed ? Is glob.rootHome set to match ?",os.path.isfile(rootExe))

  # add the bin directory of ROOT as first in the PATH, to make sure that the correct bin/root-config is used in the Makefile
  os.environ["PATH"] = rootExeDir+":"+os.environ["PATH"]

  if not "ROOTSYS" in os.environ:
    os.environ["ROOTSYS"] = glob.rootHome
    log.info(blue(" - Setting ")+green("ROOTSYS")+blue(" = \"")+yellow(os.environ["ROOTSYS"])+blue("\""))

  elif glob.rootHome != os.environ["ROOTSYS"]:
    os.environ["ROOTSYS"] = glob.rootHome
    log.info(blue(" - Setting ")+green("ROOTSYS")+blue(" = \"")+yellow(os.environ["ROOTSYS"])+blue("\""))

  else:
    log.info(blue(" - Will use ")+green("ROOTSYS")+blue(" = \"")+yellow(os.environ["ROOTSYS"])+blue("\""))

  if not "LD_LIBRARY_PATH" in os.environ:
    os.environ["LD_LIBRARY_PATH"] = rootHomeLib
    log.info(blue(" - Setting ")+green("LD_LIBRARY_PATH")+blue(" = \"")+yellow(os.environ["LD_LIBRARY_PATH"])+blue("\""))

  elif not (rootHomeLib) in os.environ["LD_LIBRARY_PATH"]:
    os.environ["LD_LIBRARY_PATH"] = rootHomeLib+":"+os.environ["LD_LIBRARY_PATH"]
    log.info(blue(" - Adding to ")+green("LD_LIBRARY_PATH")+blue(" \"")+yellow(rootHomeLib)+blue("\""))

  else:
    log.info(blue(" - Found ")+green("LD_LIBRARY_PATH")+blue(" = \"")+yellow(os.environ["LD_LIBRARY_PATH"])+blue("\""))

  return

# --------------------------------------------------------------------------------------------------
# compile/clean if needed
# --------------------------------------------------------------------------------------------------
def doMake():
  hasLib  = os.path.isdir(glob.libDirName)
  hasExe  = os.path.isfile(glob.exeName)
  isClean = glob.pars["clean"]
  isMake  = glob.pars["make"] or (not hasLib) or (not hasExe)

  if isClean or isMake:
    resetDir(glob.libDirName,isClean)
  if isMake:
    log.info(blue(" - Moving to ")+red(glob.libDirName)+blue(" and compiling ANNZ... "))
    mkfl = os.path.join(glob.annzDir,'Makefile')
    cmnd = "cd "+glob.libDirName+" ; make "+glob.makeOpt+" -f "+mkfl
    cmkdStatus = os.system(cmnd) ; Assert("compilation failed",(cmkdStatus == 0))

    if os.path.isfile(glob.exeName): log.info(blue(" - Found ")+red(glob.exeName)+blue(" - compilation seems to have succeded... "))

  # check that the executable exists before moving on and add the lib dir to LD_LIBRARY_PATH (needed on some systems)
  if not isClean: Assert("Did not find ANNZ executable ("+glob.exeName+")",os.path.isfile(glob.exeName))

  if not glob.libDirName in os.environ["LD_LIBRARY_PATH"]:
    if os.environ["LD_LIBRARY_PATH"] == "": os.environ["LD_LIBRARY_PATH"] = glob.libDirName
    else:                                   os.environ["LD_LIBRARY_PATH"] = glob.libDirName+":"+os.environ["LD_LIBRARY_PATH"]

  if glob.pars["onlyMake"]: exit(0)

# --------------------------------------------------------------------------------------------------
# run the code
# --------------------------------------------------------------------------------------------------
def runOneANNZ():
  cmnd = glob.exeName+" " ; cmndPrint = ""
  for key in glob.annz:
    if key == "":
      continue
    elif key == "generalOpt":
      cmnd      += str(glob.annz[key])+" "
      cmndPrint += blue(str(glob.annz[key]))+" , "
    else:
      cmnd      += key+"="+"\'"+str(glob.annz[key])+"\' "
      cmndPrint += blue(str(key))+red(str("="))+"\'"+green(str(glob.annz[key]))+"\' , "

  if glob.pars["logFileName"] != "":
    cmnd      += " > "+glob.pars["logFileName"]+"_annz 2>&1"
    cmndPrint += purple(" > "+glob.pars["logFileName"]+"_annz 2>&1")

  log.info(yellow(" - Will run "+glob.exeName+" with the following user-options: "))
  log.info("   "+cmndPrint)
  log.info("")

  return os.system(cmnd)

# --------------------------------------------------------------------------------------------------
def runANNZ():
  exitStatus = runOneANNZ()
  if exitStatus == 0: return

  # if ANNZ failed during training, try again with a different random seed and randomized MLM options
  # --------------------------------------------------------------------------------------------------
  if glob.annz["doTrain"]:
    # set random seed if not already set
    setSeed = "initSeedRnd" not in glob.annz.keys()
    if not setSeed: setSeed = (glob.annz["initSeedRnd"] < 1)
    if setSeed: glob.annz["initSeedRnd"]  = 198876

    # remove all userMLMopts so that a random set is generated
    glob.annz["userMLMopts"] = ""
    if glob.annz["doBinnedCls"]:
      for nRndOptNow in range(glob.annz["binCls_nTries"]): glob.annz.pop("userMLMopts_"+str(nRndOptNow),None)

    nTries = 0
    for nTryNow in range(nTries):
      glob.annz["initSeedRnd"] += 1
      log.warning(whtOnRed(" - runANNZ failed !!! Will try again ("+str(nTryNow+1)+"/"+str(nTries)+") with initSeedRnd = ")+yellowOnRed(str(glob.annz["initSeedRnd"])))

      exitStatus = runOneANNZ()
      if exitStatus == 0: break

  Assert("runANNZ failed !!!",(exitStatus == 0))
  return


# ---------------------------------------------------------------------------------------------------
# 
# ---------------------------------------------------------------------------------------------------
def addArg(args,key,val):
  if key in args: args[key] += " "+str(val);
  else:           args[key]  = val

def getArg(args,key):
  if key in args: return args[key]
  else:           return ""

