# ---------------------------------------------------------------------------------------------------
# an example scripts for submitting batch jobs on a cluster.
# this is very likely not going to work out of the box and would require customization.
# the following is supposed to happen:
#   In the first step we make sure the code is compied (stage="make"). The we generate the input dataset
#   (stage="genInp"). The next step is training (stage="train"). These jobs are put on hold, and are only executed after
#   the "genInp" job has finised. This is done using the "hold_jid" command (the particulars may depend on
#   the batch farm). After training, optimization takes place (stage="optim"), where again, the optimization
#   job is pu on hold until all training jobs have finished.
# ---------------------------------------------------------------------------------------------------
from helperFuncs import *

# ---------------------------------------------------------------------------------------------------
# set here the python file to run, as well as nTrainJobs, which should correspond to [nMLMs] (for
# regression or classifcation) or to [binCls_nBins] (for binned classification).
# ---------------------------------------------------------------------------------------------------
subType = 0
if subType == 0:
  cmndBase   = "python examples/scripts/annz_rndReg_quick.py --randomRegression"     ; optimType = "optimize" ; nTrainJobs = 10
elif subType == 1:
  cmndBase   = "python examples/scripts/annz_rndCls_quick.py --randomClassification" ; optimType = "optimize" ; nTrainJobs = 10
elif subType == 2:
  cmndBase   = "python examples/scripts/annz_binCls_quick.py --binnedClassification" ; optimType = "verify"   ; nTrainJobs = 60
else:
  Assert("Unknown subType...",False)

onlyDoOptimVerif = False
# ---------------------------------------------------------------------------------------------------

# command line arguments and basic settings
initParse()
setCols()

qsub               = dict()
qsub["qsubCmnd"]   = "qsub"
qsub["shell"]      =  "#!/bin/bash -x"
qsub["opts"]       = "-S /bin/bash"
# qsub["opts"]      += " -l h_rt=23:59:55 -l mem=2.5G "
qsub["opts"]      += " -l h_rt=11:59:55 -l mem=2G "
# qsub["opts"]      += " -l h_rt=00:19:55"
qsub["depJob"]     = ""
qsub["depJobIds"]  = ""
qsub["prefix"]     = time.strftime("_%d_%m_%y__%H_%M_%S_")

qsub["dir"] = glob.annzDir+"qsub/"
resetDir(qsub["dir"],False,False)

for stage in ["make","genInp","train","optim"]:
  # ---------------------------------------------------------------------------------------------------
  # first just make sure the code is compiled and up to date
  # ---------------------------------------------------------------------------------------------------
  if stage == "make":
    cmnd_annz = cmndBase+" --make" ; os.system(cmnd_annz)
    continue
  # ---------------------------------------------------------------------------------------------------

  # ---------------------------------------------------------------------------------------------------
  # now submit jobs to generate the input dataset, to do training of nTrainJobs and to optimize the results
  # ---------------------------------------------------------------------------------------------------
  if stage == "genInp":
    nJobs = 1          ; qsub["depJob"] = ""                             ; qsub["depJobIds"] = "" 
  if stage == "train":
    nJobs = nTrainJobs ; qsub["depJob"] = "-hold_jid "+qsub["depJobIds"] ; qsub["depJobIds"] = "" 
  if stage == "optim":
    nJobs = 1          ; qsub["depJob"] = "-hold_jid "+qsub["depJobIds"] ; qsub["depJobIds"] = "" 

  if onlyDoOptimVerif:
    if stage == "genInp" or stage == "train": continue
    else:                                     qsub["depJob"] = qsub["depJobIds"] = ""

  # ---------------------------------------------------------------------------------------------------
  # for each job, compose the respective job options and store the submitted job-IDs
  # ---------------------------------------------------------------------------------------------------
  for nJobNow in range(nJobs):
    if stage == "genInp":
      cmnd_annz = cmndBase+" --genInputTrees"                    ; qsub["jobName"] = "genInp"
    if stage == "train":
      cmnd_annz = cmndBase+" --train --trainIndex="+str(nJobNow) ; qsub["jobName"] = "train"+str(nJobNow)
    if stage == "optim":
      cmnd_annz = cmndBase+" --"+optimType                       ; qsub["jobName"] = optimType[0:5]

    # make sure this is set in order to avoid crazy output during training
    if not "--isBatch" in cmnd_annz: cmnd_annz += " --isBatch"

    # time-tags for log files
    qsub["fileBase"] = "qsub"+qsub["prefix"]+qsub["jobName"]+".log"
    qsub["file"]     = qsub["dir"]+"qsub"+qsub["prefix"]+qsub["jobName"]+".sh"
    qsub["log"]      = qsub["dir"]+qsub["fileBase"]
    qsub["logIO"]    = qsub["log"]+"_IO"

    outFile  = open(qsub["file"], "w")
    lines    = [ ]
    lines   += [ qsub["shell"]  ]
    lines   += [ "cd "+glob.annzDir ]

    # ---------------------------------------------------------------------------------------------------
    # some optional local settings
    lines   += [ "source ~/.bashrc" ]
    # ---------------------------------------------------------------------------------------------------

    lines   += [ "date ; hostname ; pwd > "+qsub["log"] ]
    lines   += [ cmnd_annz+" >> "+qsub["log"]+" 2>&1"   ]
    
    for lineNow in lines: outFile.write(lineNow+"\n")
    outFile.close()

    # compose the job-submision command (specific syntax for a given batch-farm)
    # ---------------------------------------------------------------------------------------------------
    cmnd_qsub = qsub["qsubCmnd"]+" -wd "+glob.annzDir+" -v jobLog="+qsub["logIO"]+" "+qsub["opts"]+" "+qsub["depJob"]+" " \
                +" -N "+qsub["jobName"]+" -m n -o "+qsub["logIO"]+" -e "+qsub["logIO"]+" "+qsub["file"]

    # submit the job
    outputStr = (subprocess.check_output(cmnd_qsub, shell=True)).replace("\n","").replace("\r","")
    
    # get the job-id and build the string for the dependency job (specific syntax for a given cluster)
    # ---------------------------------------------------------------------------------------------------
    outputLst = outputStr.split()
    nWords    = len(outputLst)

    for nWord in range(nWords-2):
      if outputLst[nWord] == "job" and outputLst[nWord+2][0] == "(":
        jobIdNow = outputLst[nWord+1]
        Assert("undefined job-id ("+str(jobIdNow)+") ... ?!?!",jobIdNow.isdigit())

    if qsub["depJobIds"] == "": qsub["depJobIds"] = str(jobIdNow)
    else:                       qsub["depJobIds"] = qsub["depJobIds"]+","+str(jobIdNow)

    if qsub["depJob"] == "": log.info(green(outputStr) +" ("                               +purple(qsub["log"])+")")
    else:                    log.info(yellow(outputStr)+" ("+red(str(qsub["depJob"]))+" , "+purple(qsub["log"])+")")


