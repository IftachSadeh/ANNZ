''' =======================================================================================================
Copyright (C) 2015, Iftach Sadeh

This file is part of ANNZ.
ANNZ is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
======================================================================================================= '''

import os, time
from time import sleep
import json
import ctypes
from ctypes import cdll
from threading import BoundedSemaphore

# --------------------------------------------------------------------------------------------------
# wrapper class to directly call C++ functions of ANNZ, using the Wrapper.so shared library
# --------------------------------------------------------------------------------------------------
class ANNZ():
  lib      = None
  c_charP  = ctypes.POINTER(ctypes.c_char)
  c_charPP = ctypes.POINTER(c_charP)
  lock     = BoundedSemaphore(1)

  # --------------------------------------------------------------------------------------------------
  # constructor
  # --------------------------------------------------------------------------------------------------
  def __init__(self, name = None, opts = None, log = None):
    if name is None:
      name = 'Wrapper_'+str(int(time.time()*1e6))
    self.name  = str(name)
    self.nameC = ctypes.c_char_p(name.encode('utf-8'))

    self.log = log
    if self.log:
      self.log.debug("\033[33m"+" - ANNZ("+"\033[31m"+self.name+"\033[33m"+") initializing ..."+"\033[0m")

    if opts is None:
      raise Exception(" - must init ANNZ("+self.name+") with opts ...")
    self.opts = opts

    # --------------------------------------------------------------------------------------------------
    # make sure we have the correct env to load the library
    # --------------------------------------------------------------------------------------------------
    with ANNZ.lock:
      self.setEnv()

      if ANNZ.lib is None:
        # --------------------------------------------------------------------------------------------------
        # load the library and define the input/output interfaces
        # --------------------------------------------------------------------------------------------------
        try:
          loadLib  = self.opts['libDirName']+'/Wrapper.so'
          ANNZ.lib = cdll.LoadLibrary(loadLib)
          ANNZ.lib.wrapperNew.argtypes     = [ ctypes.c_char_p, ctypes.c_int, self.c_charPP ]
          ANNZ.lib.wrapperNew.restype      =   ctypes.c_bool
          ANNZ.lib.wrapperDel.argtypes     = [ ctypes.c_char_p ]
          ANNZ.lib.wrapperExists.argtypes  = [ ctypes.c_char_p ]
          ANNZ.lib.wrapperExists.restype   =   ctypes.c_bool
          ANNZ.lib.wrapperEval.argtypes    = [ ctypes.c_char_p, ctypes.c_char_p, ctypes.c_char_p, self.c_charPP, self.c_charPP ]
          ANNZ.lib.wrapperEval.restype     =   ctypes.c_char_p
          ANNZ.lib.wrapperRelease.argtypes = [ ctypes.c_char_p, ctypes.c_char_p ]

          if self.log:
            self.log.info("\033[31m"+" - loaded: "+"\033[32m"+loadLib+"\033[0m")

        except:
          raise Exception(" - could not load the shared library ...")

    # --------------------------------------------------------------------------------------------------
    # initialize the c++ ANNZ class
    # --------------------------------------------------------------------------------------------------
    (argc, argv) = self.parseOpts()

    # --------------------------------------------------------------------------------------------------
    # set the list of input variables which will be used in the evaluation calls
    # expect a format such as:
    #   self.opts["inVars"] = "F:MAG_U;F:MAG_G;F:MAG_R"
    # --------------------------------------------------------------------------------------------------
    try:
      if not 'inVars' in self.opts:
        raise
      if len(self.opts["inVars"].replace(' ','')) == 0:
        raise
    except:
        raise Exception(" - inVars is not defined or is empty ...")

    try:
      # parse the list of input variables, and created a list with a fixed order of keys
      inVars = self.opts["inVars"].replace(' ','').split(';')
      self.inVars = [ x[x.find(':')+1:] for x in inVars]
      
      # book variable names for the evaluation parameters that will be passed to C++
      self.nVars    = len(self.inVars)
      self.varNames = (self.c_charP * (self.nVars + 1))()
      for nVarNow in range(self.nVars):
        self.varNames[nVarNow] = self.buff(self.inVars[nVarNow])

      if self.log:
        self.log.debug("\033[35m"+" - ANNZ("+"\033[31m"+self.name+"\033[35m"+") " \
                       +"initialized with inVars: "+"\033[34m"+str(self.inVars)+"\033[0m")
    except:
      raise Exception(" - could not parse inVars = "+str(self.opts["inVars"])+" ...")

    # --------------------------------------------------------------------------------------------------
    # initialize the C++ Wrapper
    # --------------------------------------------------------------------------------------------------
    with ANNZ.lock:
      try:
        gotNew = ANNZ.lib.wrapperNew(self.nameC, argc, argv)
        if not gotNew:
          raise
      except:
        raise Exception(
          " - could not initialize a new instance of ANNZ ..." \
          +" is name = \""+self.name+"\" already used ?"
        )

    return
  
  # --------------------------------------------------------------------------------------------------
  # cleanup (called upon delete, or explicitly by user)
  # --------------------------------------------------------------------------------------------------
  def cleanup(self):
    with ANNZ.lock:
      ANNZ.lib.wrapperDel(self.nameC)
    return

  def __del__(self):
    self.cleanup()
    return

  # --------------------------------------------------------------------------------------------------
  # evaluate one event / list of events
  # --------------------------------------------------------------------------------------------------
  def eval(self, evalEvtIn):
    if len(evalEvtIn) == 0:
      raise Exception(" - trying to call evaluation with no input objects ...")
    
    # --------------------------------------------------------------------------------------------------
    # parse the inputs and perform the evaluation
    # --------------------------------------------------------------------------------------------------
    isDictIn = isinstance(evalEvtIn, dict)
    evalEvtV = [evalEvtIn] if isDictIn else evalEvtIn

    # book arrays according to the number of objects and the number of variables per object
    # --------------------------------------------------------------------------------------------------
    nObjs   = len(evalEvtV)
    varVals = (self.c_charP * ((self.nVars + 1) * nObjs))()
    nVarsC  = ctypes.c_char_p((str(nObjs)+';'+str(self.nVars)).encode('utf-8'))

    # fill the arrays with the input values, converted to *char
    # --------------------------------------------------------------------------------------------------
    for nObjNow in range(len(evalEvtV)):
      try:
        for nVarNow in range(self.nVars):
          varVals [nVarNow + nObjNow * self.nVars] = self.buff(evalEvtV[nObjNow][self.inVars[nVarNow]])
      
      except:
        raise Exception(
          " - missing input key for inVars = "+str(self.inVars) \
          +"] and input variables(line="+str(nObjNow)+") = "+str(evalEvtV[nObjNow])
        )

    # --------------------------------------------------------------------------------------------------
    # perform the evaluation
    # --------------------------------------------------------------------------------------------------
    try:
      # each call to wrapperEval registers a unique evalId. this is later used to release
      # the dynamic memory allocated to the TString output variable which is passed to python
      # note that ROOT is not thread safe, so only call C++ once at a time...
      # --------------------------------------------------------------------------------------------------
      with ANNZ.lock:
        evalId   = ctypes.c_char_p(('Wrapper_'+str(int(time.time()*1e6))).encode('utf-8'))
        evalStr  = ANNZ.lib.wrapperEval(self.nameC, evalId, nVarsC, self.varNames, varVals)
        evalDict = json.loads(evalStr)
        
        # release dynamic memory allocated to the TString output variable, produced by a given
        # call to wrapperEval (only after translating the output to a python dict with json !!!)
        # --------------------------------------------------------------------------------------------------
        ANNZ.lib.wrapperRelease(self.nameC, evalId)
    
    except:
      raise Exception(" - Could not evaluate for: "+str(evalEvtIn))

    return evalDict[0] if isDictIn else evalDict

  # --------------------------------------------------------------------------------------------------
  # initialization of the C++ ANNZ
  # --------------------------------------------------------------------------------------------------
  def parseOpts(self):
    argc = len(self.opts)
    argv = (self.c_charP * (argc + 1))()

    nKey      = 0
    cmndPrint = ""
    for key in self.opts:
      if key == "":
        continue
      elif key == "generalOpt":
        argv[nKey]  = self.buff(self.opts[key])
        cmndPrint  += str(self.opts[key])+" , "
      else:
        argv[nKey]  = self.buff(str(key+"="+str(self.opts[key])))
        cmndPrint  += str(key)+str("=")+"\'"+str(self.opts[key])+"\' , "

      nKey += 1

    if self.log:
      self.log.debug(" - Will run ANNZ with the following user-options: ")
      self.log.debug("   "+cmndPrint)

    return (argc, argv)

  # --------------------------------------------------------------------------------------------------
  # parse into a string
  # --------------------------------------------------------------------------------------------------
  def buff(self, strIn):
    return ctypes.create_string_buffer(str(strIn).encode('utf-8'))

  # --------------------------------------------------------------------------------------------------
  # make sure we have the correct env to load the library
  # --------------------------------------------------------------------------------------------------
  def setEnv(self):
    annzDir = os.getcwd() if not "ANNZSYS" in os.environ else os.environ["ANNZSYS"]
    if annzDir[-1] is not "/":
      annzDir += "/"

    self.opts['libDirName'] = os.path.join(annzDir,"lib")

    # # --------------------------------------------------------------------------------------------------
    # # probably dont need to mess around with LD_LIBRARY_PATH, since Wrapper.so was
    # # compiled with:
    # #   -rpath,$ANNZSYS/lib
    # # and so should be able to find all its shared libraries on its own ...
    # # --------------------------------------------------------------------------------------------------
    # if not "LD_LIBRARY_PATH" in os.environ:
    #   os.environ["LD_LIBRARY_PATH"] = ''

    # if not self.opts['libDirName'] in os.environ["LD_LIBRARY_PATH"]:
    #   if os.environ["LD_LIBRARY_PATH"] == '':
    #     os.environ["LD_LIBRARY_PATH"] = self.opts['libDirName']
    #   else:
    #     os.environ["LD_LIBRARY_PATH"] = self.opts['libDirName']+":"+os.environ["LD_LIBRARY_PATH"]

    return


