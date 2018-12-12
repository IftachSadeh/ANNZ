# Changelog

## ANNZ v2.3.1 (12/12/2018)

- Updated `py/ANNZ.py` and `scripts/annz_evalWrapper.py` for `python-3.6` compatibility.

- Fixed bug in the `Makefile`; now ROOT shared libraries are linked *after* the local objects.

- Added `isReadOnlySys` option, usable for evaluation only. One may set `isReadOnlySys = Ture` while using the python wrapper, in order to avoid writing anything to disk during evaluation.

- Fixed issue of unnecessary excess memory consumption following validation of XML files.

- Added `minPdfWeight` functionality to the new version of PDF generation using the random walk alg.

## ANNZ v2.3.0 (03/04/2018)

### For users:
- Changed the optimization method for generating regression PDFs. The new default method (denoted in the output as `PDF_0`) is now generated based on a simple random walk alg. The previous versions of the PDF are now denoted as `PDF_1` and `PDF_2`. While currently available, the deprecated PDFs are not guaranteed to be supported in the future. In order to derive the deprecated PDFs, set:
  ```python
  glob.annz["nPDFs"] = 3
  glob.annz["addOldStylePDFs"] = True
  ```

- **(1)** Two new job options corresponding to `PDF_0` have been added: `max_optimObj_PDF` and `nOptimLoops` (see `README.md` and `scripts/annz_rndReg_advanced.py` for details). **(2)** The default value of `excludeRangePdfModelFit` has been changed from `0.1` to `0`. **(3)** Added several job options for plotting, to control the extent of underflow and overflow regions in the regression target: `underflowZ`, `overflowZ`, `underflowZwidth`, `overflowZwidth`, `nUnderflowBins`, `nOverflowBins`. (See `src/myANNZ.cpp` for details.) **(4)** Added a variable, `nZclosBins`, to control the number of bins used for optimization-metric calculations in regression. (See `src/myANNZ.cpp` for details.) **(5)** ROOT scripts are no longer stored by default for each plot. Set `savePlotScripts` to choose otherwise.


- Added a wrapper class, which allows calling the evaluation phase for regression/classification directly from python. This can be used to integrate ANNZ directly within pipelines. The python interface is defined in `py/ANNZ.py`, with a full example given in `scripts/annz_evalWrapper.py`. (See README.md for details.)

- Bug fix in a few python scripts, where the example for the `weightInp_wgtKNN` option had previously been set to numerically insignificant values.

- Changed the interface to turn off colour output (see `README.md`).

### For developers:
- Major revamp of the `Makefile`, including adding a step of precompilation of the shared `include/commonInclude.hpp` header.

- Reorganization of shared namespaces.

- Created a new `Manager` class as part of `include/myANNZ.hpp`, `src/myANNZ.cpp`.

- The new random walk alg for generating regression PDFs is implemented in `ANNZ::getRndMethodBestPDF()`, which has been completely revamped. The old version of this function has been renamed to `ANNZ::getOldStyleRndMethodBestPDF()`. It is now used in order to derive `PDF_1` and `PDF_2`.

- Added a wrapper class for e.g., python integration, implemented in `include/Wrapper.hpp`, `src/Wrapper.cpp` and `py/ANNZ.py`.

- Completely rewrote `ANNZ::doEvalReg()` to comply with pipeline integration. Added new interfaces for regression evaluation, as implemented in `src/ANNZ_regEval.cpp`.

## ANNZ v2.2.2 (04/03/2017)

- Added the option to to *not* store the full value of pdfs in the output of optimization/evaluation, by setting
  ```python
  glob.annz["doStorePdfBins"] = False
  ```
In this case, only the average metrics of a pdf are included in the output.

- Added the `sampleFrac_errKNN` option, to allow to sub-sample the input dataset for the knn uncertainty calculation (similar to e.g., `sampleFracInp_wgtKNN` and `sampleFracInp_inTrain`).

- Added metric plots of the distribution of the KNN error estimator vs. the true bias. The plots are added to the output by setting
  ```python
  glob.annz["doKnnErrPlots"] = True
  ```

- Added support for input ROOT files with different Tree names.

- Added support for ROOT version `6.8.*`.

- Other minor modifications and bug fixes.

## ANNZ v2.2.1 (01/11/2016)

- Fixed bug with using general math expressions for the `weightVarNames_wgtKNN` and `weightVarNames_inTrain` variables.

- Modified the `Makefile` to explicitly include `rpath` in `LDFLAGS`, which may be needed for pre-compiled versions of ROOT.

- Modified `subprocess.check_output()` in `examples/scripts/annz_qsub.py , fitsFuncs.py` for `Python 3.x`.

- Fixed bug which caused a segmentation fault in some cases during reweighting.

- Other minor modifications and bug fixes.

## ANNZ v2.2.0 (24/5/2016)

- Added a bias correction procedure for MLMs, which may be switched off using `glob.annz["doBiasCorMLM"] = False`. (See `README.md` and `scripts/annz_rndReg_advanced.py` for details.)

- Added the option to generate error estimations (using the KNN method) for a general input dataset. An example script is provided as `scripts/annz_rndReg_knnErr.py`. (A detailed description is given in `README.md`.)

- Added the `userWeights_metricPlots` job option, which can be used to set weight expressions for the performance plots of regression. (See `README.md` for details.)

- Changed the binning scheme for the performance plots of auxiliary variables (defined using `glob.annz["addOutputVars"]`). Instead of equal-width bins, the plots now include bins which are defined as each having the same number of objects (equal-quantile binning). This e.g., reduces statistical fluctuations in computations of the bias, scatter and other parameters, as a function of the variables used for the training.

- Changed the default number of training cycles for ANNs from `5000` to a (more reasonable) randomized choice in the range `[500,2000]` (`ANNZ::generateOptsMLM()`). The option may be set to any other value by the user, using the `NCycles` setting. E.g., during training, set: `glob.annz["userMLMopts"] = "ANNZ_MLM=ANN::HiddenLayers=N,N+3:NCycles=3500"`.

- Fixed minor bug in `ANNZ::Train_binnedCls()`, which caused a mismatch of job-options for some configuration of binned classification.

- Added a version-tag to all intermediate option files, with a format as e.g., `[versionTag]=ANNZ_2.1.3`.

- Minor change to the selection criteria for `ANNZ_best` in randomized regression.

- Other minor modifications and bug fixes.

## ANNZ v2.1.2 (15/3/2016)

- Improved selection criteria for `ANNZ_best` in randomized regression. The optimization is now based on `glob.annz["optimCondReg"]="sig68"` or `"bias"`. (The `"fracSig68"` option is deprecated.)

- **Significant speed improvement** for KNN weights and `inTrainFlag` calculations in `CatFormat::addWgtKNNtoTree()`.

- Modified `CatFormat::addWgtKNNtoTree()` and `CatFormat::inputToSplitTree_wgtKNN()` so that both training and testing objects are used together as the reference dataset, when deriving KNN weights. This new option is on by default, and may be turned off by setting:
  ```python
  glob.annz["trainTestTogether_wgtKNN"] = False
  ```
  - For developers: internal interface change (not backward compatible) - What used to be `CatFormat::addWgtKNNtoTree(TChain * aChainInp, TChain * aChainRef, TString outTreeName)` has been changed to `CatFormat::addWgtKNNtoTree(TChain * aChainInp, TChain * aChainRef, TChain * aChainEvl, TString outTreeName)`.

- Cancelled the `splitTypeValid` option, which was not very useful and confusing for users. From now on, input datasets may only be divided into two subsets, one for training and one for testing. The user may define the training/testing samples in one of two ways (see `scripts/annz_rndReg_advanced.py` for details):
  
  1. Automatic splitting:
    ```python
    glob.annz["splitType"]    = "random"
    glob.annz["inAsciiFiles"] = "boss_dr10_0.csv;boss_dr10_1.csv"
    ```
  Set a list of input files in `inAsciiFiles`, and use `splitType` to specify the method for splitting the sample. Allowed values for the latter are `serial`, `blocks` or `random`.
  
  2. Splitting by file:
    ```python
    glob.annz["splitType"]      = "byInFiles"
    glob.annz["splitTypeTrain"] = "boss_dr10_0.csv"
    glob.annz["splitTypeTest"]  = "boss_dr10_1.csv;boss_dr10_2.csv"
    ```
  Set a list of input files for training in `splitTypeTrain`, and a list of input files for testing in `splitTypeTest`. 

- Added plotting for the evaluation mode of regression (single regression, randomized regression and binned classification). If the regression target is detected as part of the evaluated dataset, the nominal performance plots are created. For instance, for the `scripts/annz_rndReg_quick.py` script, the plots will be created in `output/test_randReg_quick/regres/eval/plots/`.

- Fixed bug in plotting routine from `ANNZ::doMetricPlots()`, when adding user-defined cuts for variables not already present in the input trees.

- Simplified the interface for string variables in cut and weight expressions.
  - For example, given a set of input parameters,
    ```python
    glob.annz["inAsciiVars"] = "D:MAG_AUTO_G;D:MAG_AUTO_R;D:MAG_AUTO_I;D:Z_SPEC;C:FIELD"
    ```
    one can now use cuts and weights of the form:
    ```python
    glob.annz["userCuts_train"]    = "    (FIELD == \"FIELD_0\") ||     (FIELD == \"FIELD_1\")"
    glob.annz["userCuts_valid"]    = "    (FIELD == \"FIELD_1\") ||     (FIELD == \"FIELD_2\")"
    glob.annz["userWeights_train"] = "1.0*(FIELD == \"FIELD_0\") +  2.0*(FIELD == \"FIELD_1\")"
    glob.annz["userWeights_valid"] = "1.0*(FIELD == \"FIELD_1\") +  0.1*(FIELD == \"FIELD_2\")"
    ```
    Here, training is only done using `FIELD_0` and `FIELD_1`; validation is weighted such that galaxies from `FIELD_1` have ten times the weight compared to galaxies from `FIELD_2` etc.

  - The same rules also apply for the weight and cut options for the KNN re-weighting method: `cutInp_wgtKNN`, `cutRef_wgtKNN`, `weightRef_wgtKNN` and `weightInp_wgtKNN`, and for the corresponding variables for the evaluation compatibility test: `cutInp_inTrain`, `cutRef_inTrain`, `weightRef_inTrain` and `weightInp_inTrain`. (Examples for the re-weighting and for the compatibility test using these variables are given in `scripts/annz_rndReg_advanced.py`.)

- `ANNZ_PDF_max_0` no longer calculated by default. This may be turned back on by setting
```python
glob.annz["addMaxPDF"] = True
```

- Other minor modifications and bug fixes.

## ANNZ v2.1.1 (15/1/2016)

- Fixed bug in generating a name for an internal `TF1` function in `ANNZ::setupKdTreeKNN()`.

- Fixed bug in plotting routine from `ANNZ::doMetricPlots()`, when adding user-requested variables which are not floats.

- Added the option,
    ```python
    glob.annz["optimWithMAD"] = False
    ```
    If set to `True`, then the MAD (median absolute deviation) is used, instead of the 68th percentile of the bias (`sigma_68`). This affects only the selection of the "best" MLM and the PDF optimization procedure in randomized regression. See `scripts/generalSettings.py`.

- Added the option,
  ```python
  glob.annz["optimWithScaledBias"] = False
  ```
  If set to `True`, then instead of the bias, `delta == zReg-zTrg`, the expression `deltaScaled == delta/(1+zTrg)` is used, where `zReg` is the estimated result of the MLM/PDF and `zTrg` is the true (target) value. This affects only the selection of the "best" MLM and the PDF optimization procedure in randomized regression. E.g., one can set this parameter in order to minimize the value of `deltaScaled` instead of the value of `delta`, or correspondingly the value of the scatter of `deltaScaled` instead of that of `delta`. The selection criteria for prioritizing the bias or the scatter remains the parameter `glob.annz["optimCondReg"]`. The latter can take the value `bias` (for `delta` or `deltaScaled`), `sig68` (for the scatter of `delta` or of `deltaScaled`), and `fracSig68` (for the outlier fraction of `delta` or of `deltaScaled`). See `scripts/generalSettings.py`.

- Added the option,
  ```python
  glob.annz["plotWithScaledBias"] = False
  ```
  If set to `True`, then instead of the bias, `delta == zReg-zTrg`, the expression `delta/(1+zTrg)` is used. This affects only the figures generated with the plotting routine, `ANNZ::doMetricPlots()`, and does not change any of the optimization/output of the code. See `scripts/generalSettings.py`.

- Added option to set the PDF bins in randomized regression by the width of the bins, instead of by the number of the bins. That is, one can now set e.g.,
  ```python
  glob.annz["pdfBinWidth"] = 0.01
  ```
  instead of e.g.,
  ```python
  glob.annz["nPDFbins"] = 100
  ```
  Assuming the regression range is `[minValZ,maxValZ] = [0,1.5]`, the first option will lead to 150 PDF bins of width 0.01, while the second will result in 100 bins of width 0.015. The two options are mutually exclusive (the user should define only one or the other).

- *For developers:* Changed internal key-word interface in `Utils::getInterQuantileStats()` for requesting a MAD calculation: to add the calculation - changed from `medianAbsoluteDeviation` to `getMAD`; to retrieve the result of the calculation - from `quant_medianAbsoluteDeviation` to `quant_MAD`.

- Other minor modifications.

## ANNZ v2.1.0 (08/10/2015)

- Removed unnecessary dictionary generation from the `Makefile`.

- Changed `std::map` to `std::unordered_map` in main containers of the `OptMaps()` and `VarMaps()` classes (constitutes a slight performance boost).

- Nominally, no longer keeping track of the name of the original input file (stored in the ROOT trees with the name defined in `origFileName` in `myANNZ::Init()`). This may be switched back on by setting `glob.annz["storeOrigFileName"]  = True`.

- Added the option to use an entire input file as signal or background for single/randomized classification, in addition to (or instead of) defining a cut based on one of the input parameters. In order to use this option, one muse define the variables `inpFiles_sig` and `inpFiles_bck`. An example is given in `scripts/annz_rndCls_advanced.py`.

- Added a bias correction for randomized regression PDFs. This options is now active by default, and may be turned off by setting,
  ```python
  glob.annz["doBiasCorPDF"] = False
  ```

- Other minor modifications.

## ANNZ v2.0.6 (03/8/2015)

- Did some code optimization for tree-looping operations.

- **Added the script, `annz_rndReg_weights.py`:** This shows how one may derive the weights based on the KNN method (using `useWgtKNN`), and/or the `inTrainFlag` quality-flag, without training/evaluating any MLMs.

- Added a plot-reference guide (`thePlotsExplained.pdf`).

- Added the option `doGausSigmaRelErr` (now set to `True` by default) to estimate the scatter of the relative uncertainty of regression solutions by a Gaussian fit, instead of by the RMS or the 68th percentile of the distribution of the relative uncertainty. This only affects the plotting output of regression problems (`ANNZ::doMetricPlots()`).

- Added support for general math expressions for the `weightVarNames_wgtKNN` and `weightVarNames_inTrain` variables.

- Nominally, the `inTrainFlag` quality flag is a binary operator, and may only take values of either `0` or `1`. Have now added the option of setting `maxRelRatioInRef_inTrain < 0`. In this case, the `maxRelRatioInRef_inTrain` parameter is ignored. As a result the `inTrainFlag` may take floating-point values between zero and one.

- Added a transformation of the input parameters used for the kd-tree during the nominal uncertainty calculation in regression. The variables after the transformation span the range `[-1,1]`. The transformations are performed by default, and may be turned off by setting,
  ```python
  glob.annz["doWidthRescale_errKNN"] = False
  ```
Similarly, added the same transformations for the kd-tree during the `glob.annz["useWgtKNN"] = True` and `glob.annz["addInTrainFlag"] = True` setups. These may be turned off using the flags, `doWidthRescale_wgtKNN` and `doWidthRescale_inTrain`, respectively.

- Added support for ROOT file inputs, which may be used instead of ascii inputs (example given in `scripts/annz_rndReg_advanced.py`).

- Other minor modifications.

## ANNZ v2.0.5 (17/6/2015)

- Fixed bug in `CatFormat::addWgtKNNtoTree()`, where the weight expression for the KNN trees did not include the `ANNZ_KNN_w` weight in cases of `glob.annz["addInTrainFlag"] = True`.

- Modified the condition on the MLM error estimator which is propagated to PDFs in randomized regression. Previously if an error was undefined, indicating a problem, the code would stop. Now the MLM is ignored and the code continues. The change is needed as sometimes there is a valid reason for the error to be undefined.

- Other minor modifications.

## ANNZ v2.0.4 (19/3/2015)

- **Modified the function, `CatFormat::addWgtKNNtoTree()`, and added `CatFormat::asciiToFullTree_wgtKNN()`:** The purpose of the new features is to add an output variable, denoted as `inTrainFlag` to the output of evaluation. The new output indicates if the corresponding object is "compatible" with other objects from the training dataset. The compatibility is estimated by comparing the density of objects in the training dataset in the vicinity of the evaluated object. If the evaluated object belongs to an area of parameter-space which is not represented in the training dataset, we will get `inTrainFlag = 0`. In this case, the output of the training is probably unreliable.

- Other minor modifications.

## ANNZ v2.0.3 (25/2/2015)

- **Added *MultiClass* support to binned classification:** The new option is controlled by setting the `doMultiCls` flag. In this mode, multiple background samples can be trained simultaneously against the signal. In the context of binned classification, this means that each classification bin acts as an independent sample during the training.

- **Added the function, `ANNZ::deriveHisClsPrb()`:** Modified binned classification, such that all classification probabilities are calculated by hand, instead of using the `CreateMVAPdfs` option of `TMVA::Factory`. By default, the new calculation takes into account the relative size of the signal in each classification bin, compared to the number of objects in the entire training sample. The latter feature may be turned off, by setting:
  ```python
  glob.annz["useBinClsPrior"] = False
  ```

- Added `ANNZ_PDF_max`, the most likely value of a PDF (the peak of the PDF), to the outputs of regression.

- Fixed compatibility issues with ROOT v6.02.

- Fixed bug in `VarMaps::storeTreeToAscii()`, where variables of type `Long64_t` were treated as `Bool_t` by mistake, causing a crash.

- Other minor modifications.

## ANNZ v2.0.2 (10/2/2015)

Fixed bug in VarMaps::storeTreeToAscii(), where variables of type `Long64_t` were treated as `Bool_t` by mistake, causing a crash.

## ANNZ v2.0.1 (10/2/2015)

The following changes were made:

- **Modified the way in which the KNN error estimator works:**
In the previous version, the errors were generated by looping for each object over the *n* near-neighbors in the training dataset. For a given object, this was for all neighbors for each of the MLMs.
In the revised version, MLM response values for the entire training dataset are estimated once; this is done before the loop on the objects begins, with the results stored in a dedicated tree (see `ANNZ::createTreeErrKNN()`). This tree is then read-in during the loop over the  objects for which the errors are generated. In this implementation, the KNN neighbor search is done once for all MLMs, and the errors are estimated simultaneously for all. This prevents both the unnecessary repeated calculations of MLM outputs, and the redundant searches for the *n* near-neighbors for the same object.

- **Name of evaluation subdirectory:**
Added the variable `evalDirPostfix`, which allows to modify the name of the evaluation subdirectory. Different input files can now be evaluated simultaneously, without overwriting previous results. The example scripts have been modified accordingly.

- Various small modifications.

## ANNZ v2.0.0 (26/1/2015)

First version (v2.0.0) of the new implementation of the machine learning code, ANNz.
