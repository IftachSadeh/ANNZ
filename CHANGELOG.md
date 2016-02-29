# Changelog

## Master version

- Fixed bug in plotting routine from `ANNZ::doMetricPlots()`, when adding user-defined cuts for variables not already present in the input trees.

## ANNZ 2.1.1 (15/1/2016)

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

## ANNZ 2.1.0 (08/10/2015)

- Removed unnecessary dictionary generation from Makefile.

- Changed `std::map` to `std::unordered_map` in main containers of the `OptMaps()` and `VarMaps()` classes (constitutes a slight performance boost).

- Nominally, no longer keeping track of the name of the original input file (stored in the ROOT trees with the name defined in `origFileName` in `myANNZ::Init()`). This may be switched back on by setting `glob.annz["storeOrigFileName"]  = True`.

- Added the option to use an entire input file as signal or background for single/randomized classification, in addition to (or instead of) defining a cut based on one of the input parameters. In order to use this option, one muse define the variables `inpFiles_sig` and `inpFiles_bck`. An example is given in `scripts/annz_rndCls_advanced.py`.

- Added a bias-correction for randomized regression PDFs. This options is now active by default, and may be turned off by setting,
  ```python
  glob.annz["doBiasCorPDF"] = False
  ```

- Other minor modifications.

## ANNZ 2.0.6 (03/8/2015)

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

## ANNZ 2.0.5 (17/6/2015)

- Fixed bug in `CatFormat::addWgtKNNtoTree()`, where the weight expression for the KNN trees did not include the `ANNZ_KNN_w` weight in cases of `glob.annz["addInTrainFlag"] = True`.

- Modified the condition on the MLM error estimator which is propagated to PDFs in randomized regression. Previously if an error was undefined, indicating a problem, the code would stop. Now the MLM is ignored and the code continues. The change is needed as sometimes there is a valid reason for the error to be undefined.

- Other minor modifications.

## ANNZ 2.0.4 (19/3/2015)

- **Modified the function, `CatFormat::addWgtKNNtoTree()`, and added `CatFormat::asciiToFullTree_wgtKNN()`:** The purpose of the new features is to add an output variable, denoted as `inTrainFlag` to the output of evaluation. The new output indicates if the corresponding object is "compatible" with other objects from the training dataset. The compatibility is estimated by comparing the density of objects in the training dataset in the vicinity of the evaluated object. If the evaluated object belongs to an area of parameter-space which is not represented in the training dataset, we will get `inTrainFlag = 0`. In this case, the output of the training is probably unreliable.

- Other minor modifications.

## ANNZ 2.0.3 (25/2/2015)

- **Added *MultiClass* support to binned classification:** The new option is controlled by setting the `doMultiCls` flag. In this mode, multiple background samples can be trained simultaneously against the signal. In the context of binned classification, this means that each classification bin acts as an independent sample during the training.

- **Added the function, `ANNZ::deriveHisClsPrb()`:** Modified binned classification, such that all classification probabilities are calculated by hand, instead of using the `CreateMVAPdfs` option of `TMVA::Factory`. By default, the new calculation takes into account the relative size of the signal in each classification bin, compared to the number of objects in the entire training sample. The latter feature may be turned off, by setting:
  ```python
  glob.annz["useBinClsPrior"] = False
  ```

- Added `ANNZ_PDF_max`, the most likely value of a PDF (the peak of the PDF), to the outputs of regression.

- Fixed compatibility issues with ROOT v6.02.

- Fixed bug in `VarMaps::storeTreeToAscii()`, where variables of type `Long64_t` were treated as `Bool_t` by mistake, causing a crash.

- Other minor modifications.

## ANNZ 2.0.2 (10/2/2015)

Fixed bug in VarMaps::storeTreeToAscii(), where variables of type `Long64_t` were treated as `Bool_t` by mistake, causing a crash.

## ANNZ 2.0.1 (10/2/2015)

The following changes were made:

- **Modified the way in which the KNN error estimator works:**
In the previous version, the errors were generated by looping for each object over the *n* near-neighbors in the training dataset. For a given object, this was for all neighbors for each of the MLMs.
In the revised version, MLM response values for the entire training dataset are estimated once; this is done before the loop on the objects begins, with the results stored in a dedicated tree (see `ANNZ::createTreeErrKNN()`). This tree is then read-in during the loop over the  objects for which the errors are generated. In this implementation, the KNN neighbor search is done once for all MLMs, and the errors are estimated simultaneously for all. This prevents both the unnecessary repeated calculations of MLM outputs, and the redundant searches for the *n* near-neighbors for the same object.

- **Name of evaluation subdirectory:**
Added the variable `evalDirPostfix`, which allows to modify the name of the evaluation subdirectory. Different input files can now be evaluated simultaneously, without overwriting previous results. The example scripts have been modified accordingly.

- Various small modifications.

## ANNZ 2.0.0 (26/1/2015)

First version (v2.0.0) of the new implementation of the machine learning code, ANNz.
