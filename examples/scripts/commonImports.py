import os,logging

# --------------------------------------------------------------------------------------------------
# if [useDefinedROOTSYS == False], rootHome overrides the predefined environmental variable, $ROOTSYS,
# if [useDefinedROOTSYS == True] and $ROOTSYS is defined, then rootHome will be ignored - see initROOT() in helperFuncs.py
useDefinedROOTSYS = True
# if set, rootHome must be the absolute path to the directory where ROOT is installed
rootHome          = ""
# --------------------------------------------------------------------------------------------------

pars         = dict()
qsub         = dict()
annz         = dict()

log          = logging.getLogger('annzLog')
annzDir      = os.getcwd() if not "ANNZSYS" in os.environ else os.environ["ANNZSYS"]
if annzDir[-1] is not "/":
  annzDir += "/"
libDirName   = os.path.join(annzDir,"lib")
exeName      = os.path.join(libDirName,"myANNZ")

# color output
# --------------------------------------------------------------------------------------------------
ColBlue      = ColRed          = ColGreen      = ColDef          = ColLightBlue     = ColYellow = ColCyan = ""
ColUnderLine = ColWhiteOnBlack = ColWhiteOnRed = ColWhiteOnGreen = ColWhiteOnYellow = ColPurple           = ""

def blue       (message): return ColBlue                   +str(message)+ColDef
def red        (message): return ColRed                    +str(message)+ColDef
def green      (message): return ColGreen                  +str(message)+ColDef
def lBlue      (message): return ColLightBlue              +str(message)+ColDef
def yellow     (message): return ColYellow                 +str(message)+ColDef
def purple     (message): return ColPurple                 +str(message)+ColDef
def cyan       (message): return ColCyan                   +str(message)+ColDef
def whtOnBlck  (message): return ColWhiteOnBlack           +str(message)+ColDef
def redOnBlck  (message): return ColWhiteOnBlack+ColRed    +str(message)+ColDef
def bluOnBlck  (message): return ColWhiteOnBlack+ColBlue   +str(message)+ColDef
def yellOnBlck (message): return ColWhiteOnBlack+ColYellow +str(message)+ColDef
def whtOnRed   (message): return ColWhiteOnRed             +str(message)+ColDef
def yellowOnRed(message): return ColWhiteOnRed+ColYellow   +str(message)+ColDef
def whtOnYellow(message): return ColWhiteOnYellow          +str(message)+ColDef
def whtOnGreen (message): return ColWhiteOnGreen           +str(message)+ColDef

# assertion with a message
# --------------------------------------------------------------------------------------------------
def Assert(message,state):
  if not state:
    log.critical(whtOnRed(message)) ; log.critical(whtOnRed("Will terminate !!!!"))
    exit(1)
