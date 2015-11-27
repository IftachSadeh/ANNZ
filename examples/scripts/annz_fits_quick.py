from annz.helperFuncs import *
from annz.fitsFuncs   import fitsToAscii,asciiToFits

# command line arguments and basic settings
# --------------------------------------------------------------------------------------------------
init()

# just in case... (may comment this out)
if not glob.annz["doFitsToAscii"] and not glob.annz["doAsciiToFits"]:
  log.info(red(" - "+time.strftime("%d/%m/%y %H:%M:%S")+" - This scripts is only designed for fitsToAscii or asciiToFits...")) ; sys.exit(0)

# ==================================================================================================
# The main code - fits/ascii conversions -
# --------------------------------------------------------------------------------------------------
#   - run the following:
#     python annz_fits_quick.py --fitsToAscii
#     python annz_fits_quick.py --asciiToFits
# --------------------------------------------------------------------------------------------------
log.info(whtOnBlck(" - "+time.strftime("%d/%m/%y %H:%M:%S")+" - starting ANNZ"))

# outDirName - set output directory name
glob.annz["outDirName"] = "test_fits_quick"

# --------------------------------------------------------------------------------------------------
# use fits input to write ascii output
# --------------------------------------------------------------------------------------------------
if glob.annz["doFitsToAscii"]:

  # directory where the input fits files are located
  glob.annz["inDirName"]    = "examples/data/photoZ/fits"

  # semicolon-separated list of input files
  glob.annz["inFitsFiles"]  = "boss_dr10_0.fits.gz"

  # --------------------------------------------------------------------------------------------------
  # inFitsCols -
  # - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  #   semicolon-separated list of variables which are to extracted from the fits file
  #    - names must match the fits column names, but not all columns need to be selected
  #    - vector columns are also supported
  #    - the variable types are extracted from the fits header information automatically
  # --------------------------------------------------------------------------------------------------
  glob.annz["inFitsCols"]   = "MAG_U;MAGERR_U;MAG_G;MAGERR_G;MAG_R;MAGERR_R;MAG_I;MAGERR_I;MAG_Z;MAGERR_Z;Z"

  fitsToAscii()

# --------------------------------------------------------------------------------------------------
# use ascii input to write fits output
# --------------------------------------------------------------------------------------------------
if glob.annz["doAsciiToFits"]:

  # directory where the input ascii files are located
  glob.annz["inDirName"]    = "examples/data/photoZ/train"

  # semicolon-separated list of input files
  glob.annz["inAsciiFiles"] = "boss_dr10_0_large.csv;boss_dr10_1_large.csv"

  # semicolon-separated list of parameter types and parameter names, corresponding to columns in the input
  # file, e.g., [TYPE:NAME] may be [F:MAG_U], with 'F' standing for float. (see advanced example for detailed explanation)
  glob.annz["inAsciiVars"]  = "F:MAG_U;F:MAGERR_U;F:MAG_G;F:MAGERR_G;F:MAG_R;F:MAGERR_R;F:MAG_I;F:MAGERR_I;F:MAG_Z;F:MAGERR_Z;D:Z"

  asciiToFits()

log.info(whtOnBlck(" - "+time.strftime("%d/%m/%y %H:%M:%S")+" - finished runing ANNZ !"))

