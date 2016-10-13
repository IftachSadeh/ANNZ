from   scripts.helperFuncs import *
import numpy as np
import csv,fnmatch

# --------------------------------------------------------------------------------------------------
# general settings
# --------------------------------------------------------------------------------------------------
delimList           = ';'
delimData           = ','
commentTag          = '#'
dataExt             = 1
nAddCharToStr       = 5
printFreq           = 5000
stringColLen        = 250
postfix_csv         = ".csv"
postfix_fits        = ".fits.gz"
maxLinesOutFile     = int(2e6)
splitNamePattern    = "_%04d"

# --------------------------------------------------------------------------------------------------
# use fits input to write ascii output
# --------------------------------------------------------------------------------------------------
def fitsToAscii():
  from astropy.io import fits
  log.info(blue(" - "+time.strftime("%d/%m/%y %H:%M:%S"))+green(" - starting fitsToAscii() ..."))

  # global variables from the user
  # --------------------------------------------------------------------------------------------------
  outDirName  = glob.annz["outDirName"]
  inFitsFiles = glob.annz["inFitsFiles"]
  inFitsCols  = glob.annz["inFitsCols"]
  inDirName   = glob.annz["inDirName"]
  maxNobj     = glob.annz["maxNobj"]

  outDirNameFull = "./output/"+outDirName+"/asciiOut/"
  resetDir(outDirNameFull,False)

  fitsFiles   = inFitsFiles.split(delimList)
  writeColInc = inFitsCols .split(delimList)

  # write an ascii file for every input fits file
  # --------------------------------------------------------------------------------------------------
  for fitsFileNow in fitsFiles:
    fitsFileName  = inDirName+"/"+fitsFileNow
    asciiFileName = outDirNameFull+fitsFileNow+postfix_csv
    log.info(green(" - will read from file: ")+red(fitsFileName))

    hdulist = fits.open(fitsFileName,memmap=True)    
    scidata = hdulist[dataExt].data
    cols    = hdulist[dataExt].columns
    nCols   = len(cols)
    nLines  = len(scidata)

    if maxNobj > 0 and nLines > maxNobj: nLines = maxNobj

    colNames = dict() ; doCalib  = dict() ; isStrCol = dict()
    colTypes = dict() ; arrTypes = dict() ; brcTypes = dict()
    skipCol  = dict() ; colVsize = dict()

    # register all columns from the fits file and check if they are requested by the user
    for nColNow in range(nCols):
      colNames[nColNow]                   = str(cols[nColNow].name)       # cast explicitly to string, or else there will be trouble later on...
      colTypes[nColNow]                   = str(cols[nColNow].format)     # cast explicitly to string, or else there will be trouble later on...
      brcTypes[nColNow]                   = getBrcType(colTypes[nColNow]) # FITS format -> tree branch type
      isStrCol[nColNow]                   = (brcTypes[nColNow] == "C")
      skipCol [nColNow]                   = getSkipCol(colNames[nColNow],"",writeColInc)
      arrTypes[nColNow],colVsize[nColNow] = getArrType(colTypes[nColNow]) # FITS format -> numpy type

    # derive the variable list and compose the header-line for the output file
    # --------------------------------------------------------------------------------------------------
    outLine = []
    for nColNow in range(nCols):
      if skipCol[nColNow]: continue

      if colVsize[nColNow] == 1 or isStrCol[nColNow]:
        outLine += [ brcTypes[nColNow]+":"+colNames[nColNow] ]
      else:
        for nVnow in range(colVsize[nColNow]):
          outLine += [ brcTypes[nColNow]+":"+colNames[nColNow]+"_"+str(nVnow) ]

    varList = commentTag+" "+delimList.join(outLine)
    log.info(purple(" - will write the following variables: "))
    log.info(blue("   "+varList.replace(delimList," "+delimList+" ")))

    nOutFile = 0
    for line in range(nLines):
      # create an output file (or switch if has gone through too many lines in the current file)
      # --------------------------------------------------------------------------------------------------
      if line % maxLinesOutFile == 0:
        if line > 0:
          asciiFile.close()
          cmnd = "mv "+asciiFileName+" "+asciiFileName.replace(postfix_csv,str(splitNamePattern % nOutFile)+postfix_csv)
          log.info(purple(" - sys-cmnd: ")+green(cmnd)) ; os.system(cmnd)
          nOutFile += 1
        
        log.info(yellow(" - will write to new file: ")+purple(asciiFileName))
        asciiFile = open(asciiFileName,'w')
        asciiFile.write(varList+"\n")

      # some output along
      # --------------------------------------------------------------------------------------------------
      if line%printFreq == 0:
        just = len(str(nLines)) ; percent = yellow(str(" ("+str(int(100*line/float(nLines)))+"%) ").ljust(7))
        log.info(green(" -- Went through ")+yellow(str(line).rjust(just))+green(" / "+str(nLines))+percent+green("lines so far..."))

      # get the data from every accepted column and write a line to the output ascii file
      # --------------------------------------------------------------------------------------------------
      outLine = []
      for nColNow in range(nCols):
        if skipCol[nColNow]: continue

        if isStrCol[nColNow]:
          outLine += [ "\""+str(scidata[line][nColNow])+"\"" ]
        else:
          if colVsize[nColNow] == 1:
            outLine += [ arrTypes[nColNow](scidata[line][nColNow]) ]
          else:
            for nVnow in range(colVsize[nColNow]):
              outLine += [ arrTypes[nColNow](scidata[line][nColNow][nVnow]) ]
    
      asciiFile.write(delimData.join([str(ele) for ele in outLine])+"\n")

    log.info(green(" -- Went through ")+yellow(str(line+1).rjust(len(str(nLines))))+green(" / "+str(nLines)+" lines."))

    asciiFile.close()
    if nOutFile > 1:
      cmnd = "mv "+asciiFileName+" "+asciiFileName.replace(postfix_csv,str(splitNamePattern % nOutFile)+postfix_csv)
      log.info(yellow(" - sys-cmnd: ")+green(cmnd)) ; os.system(cmnd)

  return


# --------------------------------------------------------------------------------------------------
# use ascii input to write fits output
# --------------------------------------------------------------------------------------------------
def asciiToFits():
  from astropy.io import fits
  log.info(blue(" - "+time.strftime("%d/%m/%y %H:%M:%S"))+green(" - starting asciiToFits() ..."))

  # global variables from the user
  # --------------------------------------------------------------------------------------------------
  outDirName     = glob.annz["outDirName"]
  inAsciiFiles   = glob.annz["inAsciiFiles"]
  inAsciiVars    = glob.annz["inAsciiVars"]
  inDirName      = glob.annz["inDirName"]
  maxNobj        = glob.annz["maxNobj"]

  asciiFiles     = inAsciiFiles.split(delimList)
  outDirNameFull = "./output/"+outDirName+"/fitsOut/"
  resetDir(outDirNameFull,False)

  # write a fits file for every input ascii file
  # --------------------------------------------------------------------------------------------------
  for asciiFileNow in asciiFiles:
    asciiFileName = inDirName+"/"+asciiFileNow
    asciiFile     = open(asciiFileName,'r')
    fileReader    = csv.reader(asciiFile, delimiter=delimData)
    log.info(green(" - will read from file: ")+red(asciiFileName))

    # derive the number of lines in the file
    cmnd      = "wc -l "+asciiFileName
    outputStr = (subprocess.check_output(cmnd, shell=True)).decode('ascii').replace("\n","").replace("\r","")
    nLines = (outputStr.split())[0]
    Assert("Could not derive number of lines using command \""+cmnd+"\" ... Something is horribly wrong ?!?! ",(nLines.isdigit()))
    nLines = int(nLines)
    if maxNobj > 0 and nLines > maxNobj: nLines = maxNobj

    nRows = 0 ; rows = [] ; colTypeNamesV = []
    for row in fileReader:

      # derive the variable types/names and define the fits columns
      # --------------------------------------------------------------------------------------------------
      if nRows == 0:
        if inAsciiVars == "":
          # try to extract the variable types/names from the first row
          word        = (row[0]).replace(" ","")
          Assert("Expect the first row to begin with a \""+commentTag+"\" !!!",(word[0] == commentTag))
          inAsciiVars = word[1:]

        log.info(purple(" - will write the following columns: "))
        log.info(blue("   "+inAsciiVars.replace(delimList," "+delimList+" ")))

        varTypeNameV = (inAsciiVars).split(delimList)
        for varTypeNameNow in varTypeNameV:
          varTypeName = varTypeNameNow.split(':')
          Assert("Expect the first row to contain a list of elements like \"varType:varName\" !!!",(len(varTypeName) == 2))

          colFormat      = getColFormat(varTypeName[0])
          arrType        = getArrType(colFormat)
          colTypeNamesV += [ [colFormat,varTypeName[1],arrType] ]

      # if the row is not a comments (starts with #), wirte it out
      # --------------------------------------------------------------------------------------------------
      if not commentTag in row[0]: rows += [row]

      # some output along 
      # --------------------------------------------------------------------------------------------------
      if nRows%printFreq == 0:
        just = len(str(nLines)) ; percent = yellow(str(" ("+str(int(100*nRows/float(nLines)))+"%) ").ljust(7))
        log.info(green(" -- Went through ")+yellow(str(nRows).rjust(just))+green(" / "+str(nLines))+percent+green("lines so far..."))

      nRows += 1
      if nRows == maxNobj: break
  
    log.info(green(" -- Went through ")+yellow(str(nRows).rjust(len(str(nLines))))+green(" / "+str(nLines)+" lines."))

    # define the fits columns and fill them with the contents of the data-arrya
    cols = [] ; arr = np.array(rows)
    for nColNow in range(len(colTypeNamesV)):
      colFormat = colTypeNamesV[nColNow][0]
      colName   = colTypeNamesV[nColNow][1]
      colType   = colTypeNamesV[nColNow][2]

      arrNow  = np.asarray(arr[:,nColNow],dtype=colType)
      colNow  = fits.Column(name=colName, format=colFormat, array=arrNow)
      cols   += [ colNow ]

    # write the fits file
    # --------------------------------------------------------------------------------------------------
    fitsFileName = outDirNameFull+asciiFileNow+postfix_fits
    log.info(yellow(" - will write to file: ")+purple(fitsFileName))
    tbhdu = fits.new_table(fits.ColDefs(cols))
    tbhdu.writeto(fitsFileName,clobber=True)

  return


# ==================================================================================================
# dictionaries and access functions for formatting of FITS columns and root-tree branches
# --------------------------------------------------------------------------------------------------
dict_FitsFormatToNumpyType = dict()
dict_FitsFormatToRootTree  = dict()
dict_RootTreeToFitsFormat  = dict()
# --------------------------------------------------------------------------------------------------
dict_FitsFormatToNumpyType["L"] = np.bool_     # Boolean (True or False) stored as a byte
dict_FitsFormatToNumpyType["B"] = np.int16     # Integer (-32768 to 32767) 
dict_FitsFormatToNumpyType["I"] = np.int16     # Integer (-32768 to 32767) 
dict_FitsFormatToNumpyType["J"] = np.int32     # Integer (-2147483648 to 2147483647)
dict_FitsFormatToNumpyType["K"] = np.int64     # Integer (-9223372036854775808 to 9223372036854775807)
dict_FitsFormatToNumpyType["E"] = np.float32   # Single precision float: sign bit, 8 bits exponent, 23 bits mantissa
dict_FitsFormatToNumpyType["D"] = np.float64   # Double precision float: sign bit, 11 bits exponent, 52 bits mantissa
# --------------------------------------------------------------------------------------------------
# for FITS format, see: http://pythonhosted.org/pyfits/users_guide/users_table.html
#                                     # FITS format code                     # Root-tree branch type
#                                     ------------------                     -----------------------
dict_FitsFormatToRootTree ["L"] = "B"  # L : logical (Boolean)                # B : a boolean (Bool_t)
dict_FitsFormatToRootTree ["B"] = "S"  # B : 1-bit  integer                   # S : a 16 bit signed integer (Short_t)
dict_FitsFormatToRootTree ["I"] = "S"  # I : 16-bit integer                   # S : a 16 bit signed integer (Short_t)
dict_FitsFormatToRootTree ["J"] = "I"  # J : 32-bit integer                   # I : a 32 bit signed integer (Int_t)
dict_FitsFormatToRootTree ["K"] = "L"  # K : 64-bit integer                   # L : a 64 bit signed integer (Long64_t)
dict_FitsFormatToRootTree ["E"] = "F"  # F : single precision floating point  # F : a 32 bit floating point (Float_t)
dict_FitsFormatToRootTree ["D"] = "D"  # D : double precision floating point  # D : a 64 bit floating point (Double_t)
dict_FitsFormatToRootTree ["A"] = "C"  # A : character                        # C : a character string terminated by the 0 character
# --------------------------------------------------------------------------------------------------
dict_RootTreeToFitsFormat ["B"] = "L"  # L : logical (Boolean)                # B : a boolean (Bool_t)
dict_RootTreeToFitsFormat ["S"] = "I"  # I : 16-bit integer                   # S : a 16 bit signed integer (Short_t)
dict_RootTreeToFitsFormat ["I"] = "J"  # J : 32-bit integer                   # I : a 32 bit signed integer (Int_t)
dict_RootTreeToFitsFormat ["L"] = "K"  # K : 64-bit integer                   # L : a 64 bit signed integer (Long64_t)
dict_RootTreeToFitsFormat ["F"] = "E"  # F : single precision floating point  # F : a 32 bit floating point (Float_t)
dict_RootTreeToFitsFormat ["D"] = "D"  # D : double precision floating point  # D : a 64 bit floating point (Double_t)
dict_RootTreeToFitsFormat ["C"] = "A"  # A : character                        # C : a character string terminated by the 0 character
# ==================================================================================================

# --------------------------------------------------------------------------------------------------
# derive the branch-type of variable for a root tree from the column-format of a FITS file
# --------------------------------------------------------------------------------------------------
def getBrcType(strIn):
  lenStr = len(strIn) ; assert(lenStr > 0)
  
  if(strIn == "X" or strIn == "C" or strIn == "M" or strIn == "P"):
    log.critical(whtOnRed("Got un-supported input - getBrcType("+str(strIn)+"). ABORTING !!!"))
    assert(False)

  if lenStr == 1:
    str1 = strIn
  else:
    str0 = strIn[:-1] ; str1 = strIn[-1:] ; assert(str0.isdigit())
  
  return dict_FitsFormatToRootTree[str1]

# --------------------------------------------------------------------------------------------------
# [only write included cols] or [do not write the excluded] cols to the output file
# --------------------------------------------------------------------------------------------------
def getSkipCol(colName,writeColExc,writeColInc):
  if   len(writeColExc) > 0:
    return     any(len(fnmatch.filter([colName],ele)) > 0 for ele in writeColExc) #any(colName == ele for ele in writeColExc)
  elif len(writeColInc) > 0:
    return not any(len(fnmatch.filter([colName],ele)) > 0 for ele in writeColInc) #any(colName == ele for ele in writeColInc)
  else:
    return False

# --------------------------------------------------------------------------------------------------
# derive the column-format of a FITS file from the branch-type of a root tree
# --------------------------------------------------------------------------------------------------
def getColFormat(formatIn,strLenIn = 0):
  if(formatIn == "B" or formatIn == "b"): # or formatIn == "s" or formatIn == "i" or formatIn == "l"):
    log.critical(whtOnRed("Got un-supported input - getColFormat("+str(formatIn)+"). ABORTING !!!"))
    assert(False)

  strLen = strLenIn    if strLenIn  >   0 else stringColLen
  lenStr = str(strLen) if formatIn == "C" else ""
  return lenStr+dict_RootTreeToFitsFormat[formatIn]

  # strLen = strLenIn if strLenIn > 0 else stringColLen
  # if formatIn == "C": return (str(strLen)+"A")
  # else:               return dict_RootTreeToFitsFormat[formatIn]

# --------------------------------------------------------------------------------------------------
# derive the corresponding np array-variable from the column-format of a FITS file
# --------------------------------------------------------------------------------------------------
def getArrType(strIn):
  lenStr = len(strIn)
  assert(lenStr > 0)
  if(strIn == "X" or strIn == "C" or strIn == "M" or strIn == "P"):
    log.critical(whtOnRed("Got un-supported input - getArrType("+str(strIn)+"). ABORTING !!!"))
    assert(False)

  # scalar column (single-char string also possible)
  if lenStr == 1:
    if strIn == "A":
      str0 = str(1+nAddCharToStr)
      return np.dtype('a'+str0),int(str0)   # eg, return a15
    else:
      return dict_FitsFormatToNumpyType[strIn],1
  
  # vector (or string with more than one char) column
  else:
    str0 = strIn[:-1] ; str1 = strIn[-1:] ; assert(str0.isdigit()) ; lenV = int(str0) # eg, get 5 from 5A
    if str1 == "A":
      str0 = str(int(str0)+nAddCharToStr)
      return np.dtype('a'+str0),int(str0)
    else:
      return dict_FitsFormatToNumpyType[str1],lenV



