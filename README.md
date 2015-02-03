# ANNZ 2.0.0

## Introduction
ANNZ uses both regression and classification techniques for estimation of single-value photo-z (or any regression problem) solutions and PDFs. In addition it is suitable for classification problems, such as star/galaxy classification.

ANZZ uses the [TMVA package](http://tmva.sourceforge.net/) which is based on [ROOT](https://root.cern.ch/). The current version is a completely new implementation of the original ANNZ package.

The different configurations for regression problems (such as photometric redshift estimation) are referred to as *single regression*, *randomized regression* and *binned classification*. In addition, it is possible to run ANNZ in *single classification* and *randomized classification* modes, used for general classification problems.

The algorithm is described in the dedicated paper (**COMING SOON**).

### Terminology
  - **ANN**: artificial neural network.
  - **BDT**: boosted decision tree(s).
  - **KNN**: k-nearest neighbors.
  - **MLM**: machine-learning method, may refer to ANN,BDT or any other of the algorithms available in TMVA.
  - **spec-z**: spectroscopic redshift (the *true* redshift or the *target* of the regression).
  - **photo-z**: the derived redshift value from an MLM estimator.

### Modes of operation
#### Single regression or classification
In the simplest configuration of ANNZ, a single regression or classification is performed.
While any of the MLMs available through TMVA may be used, ANN/BDT generally achieve the best performance. These two therefore have automated configuration-option generation, as part of the default setup of ANNZ.

#### Randomized regression
An ensemble of regression methods is automatically generated. The randomized MLMs differ from each other in several ways. This includes setting unique random seed initializations, as well as changing the configuration parameters of a given algorithm (e.g., number of hidden layers in an ANN), or the set of input parameters used for the training.

Once training is complete, optimization takes place. In this stage, a distribution of photo-z solutions for each galaxy is derived.  A selection procedure is then applied to the ensemble of answers, choosing the subset of methods which achieve optimal performance. The selected MLMs are then folded with their respective uncertainty estimates, which are derived using a KNN-uncertainty estimator (see [Oyaizu et al, 2007](http://arxiv.org/abs/0711.0962)). A set of PDF candidates is generated, where each candidate is constructed by a different set of relative weights associated with the various MLM components. Two  selection schemes which optimize the performance of the PDF candidates are currently implemented in ANNZ. In this way, the PDFs which best describe the target of the regression are chosen, resulting in two alternative PDF solutions.

The final products are the *best* solution out of all the randomized MLMs, the full binned PDF(s) and the weighted and un-weighted average of the PDF(s), each also having a corresponding uncertainty estimator. (More details below.)

#### Binned classification
ANNZ may also be run in binned classification mode, employing an algorithm similar to that used by [Gerdes et al, 2010](http://arxiv.org/abs/0908.4085). The first step of the calculation involves dividing the redshift range of the input samples into many small *classification bins*. Within the redshift bounds of a given bin, the *signal* sample is defined as the collection of galaxies for which the spec-z is within the bin. Similarly, the *background* sample includes all galaxies with spec-z outside the confines of the bin.

The algorithm proceeds by training a different classification MLM for each redshift bin. The output of a trained method in a given bin, is translated to the probability for a galaxy to have a redshift which falls inside that bin. The distribution of probabilities from all of the bins is normalized to unity, accounting for possible varying bin width. It then stands as the photo-z PDF of the galaxy.

The final products are the full binned PDF, and the weighted average of the PDF, together with the corresponding uncertainty estimators.
Binned classification generally requires a larger dataset for training, compared to randomized regression (all parts of the spec-z distribution need to have enough objects for training).

#### Randomized classification
Randomized classification may be used for general classification problems. In this case, an ensemble of MLMs are randomized during training (as done in randomized regression). The optimization stage includes calculation of the separation parameter between signal/background distributions, and produces a ranking of all solutions. Any or all ranked solutions may then be included in the final product. Classification uncertainties may additionally be computed.

## Installation 

### Requirements

  - python 2.7 or higher.
  - gcc 4.3 or higher.
  - ROOT 5.34/11 or higher.

### Download (and if needed install) ROOT

ROOT is available [here](https://root.cern.ch/drupal/content/downloading-root). Version 5.34/25 was used for development, though later versions should be backward compatible (please report any issues).

Lets assume we want to install ROOT at `/home/work/root`. We can get ROOT, in one of two ways (as explained on the ROOT site):

  1. It is possible to download a pre-compiled directory for modern operating systems. For instance, for OSX 10.10 x86-64 with clang 6.0 (ROOT version   5.34.25), do:
  ```bash
  cd /home/work
  wget http://root.cern.ch/download/root_v5.34.25.macosx64-10.10-i386.tar.gz
  tar xvfz root_v5.34.25.macosx64-10.10-i386.tar.gz
  ```

  2. Alternatively, one may download the source files and compile from scratch. This would require something like:
  ```bash
  cd /home/work
  wget http://root.cern.ch/download/root_v5.34.25.source.tar.gz
  tar xvfz root_v5.34.25.source.tar.gz
  cd root
  ./configure
  make
  ```

In order to check that ROOT is properly installed, do:
```bash
/home/work/root/bin/root -b -q
```

### Install ANNZ

For the following, let's assume the working directory is going to be `/home/work/annz`.

First download [ANNZ](https://github.com/IftachSadeh/ANNZ):
```bash
git clone https://github.com/IftachSadeh/ANNZ
```

Make a personal copy of the example-script directory:
```bash
cd /home/work/annz
cp -r examples/scripts .
```

ROOT requires definition of the environmental variables `$ROOTSYS` (set to the directory where ROOT is installed) and `$LD_LIBRARY_PATH` (should include the path, `${ROOTSYS}/lib`). If these are not defined in the session, it is possible to set the `rootHome` variable in `scripts/commonImports.py` instead. This would e.g., be:
```python
rootHome = "/home/work/root/"
```

Otherwise, define `$ROOTSYS` in the session, and set in `scripts/commonImports.py`:
```python
useDefinedROOTSYS = True
```

To install and/or clean ANNZ is possible with any of the `scripts/annz_*.py` scripts described in the following. For instance, do:
```bash
python scripts/annz_singleReg_quick.py --make
```

(This is also done automatically the first time any of the `scripts/annz_*.py` scripts are run, if the compilation directory, `lib/`, is not detected.)

To force a complete re-install, one may either safely delete the `lib/` directory, or do:
```bash
python scripts/annz_singleReg_quick.py --make --clean
```


## Examples

### Quickstart guide

The various example scripts includes comments about the different variables which the user needs to set. Each operational mode has a *quickstart* dedicated script as well as an *advanced* script. The latter include more job-options as well as more detailed documentation.

For each of the following, please use follow the four respective steps (generation, training, optimization/verification, evaluation) in sequence. For instance, for single regression, do:
```bash
python scripts/annz_singleReg_quick.py --singleRegression --genInputTrees
python scripts/annz_singleReg_quick.py --singleRegression --train
python scripts/annz_singleReg_quick.py --singleRegression --optimize
python scripts/annz_singleReg_quick.py --singleRegression --evaluate
```

#### Single regression

  1. **genInputTrees**: Convert the input ascii dataset into ROOT TTrees, which are later processed by ANNZ. Plots are also created, corresponding to distributions of each input parameter.
  ```bash
  python scripts/annz_singleReg_quick.py --singleRegression --genInputTrees
  ```

  2. **train**: Perform training of a single MLM.
  ```bash
  python scripts/annz_singleReg_quick.py --singleRegression --train
  ```
  
  3. **optimize**: Verify that the training was successful and create some performance plots.
  ```bash
  python scripts/annz_singleReg_quick.py --singleRegression --optimize
  ```
  
  4. **evaluate**: Evaluate an input dataset using the trained MLM.
  ```bash
  python scripts/annz_singleReg_quick.py --singleRegression --evaluate
  ```

#### Randomized regression

  1. **genInputTrees**: Convert the input ascii dataset into ROOT TTrees, which are later processed by ANNZ. Plots are also created, corresponding to distributions of each input parameter.
  ```bash
  python scripts/annz_rndReg_quick.py --randomRegression --genInputTrees
  ```

  2. **train**: Perform training of multiple MLMs, where each has a different configuration (type of MLM, algorithm-parameter choices, input-parameter sets).
  ```bash
  python scripts/annz_rndReg_quick.py --randomRegression --train
  ```

  3. **optimize**: Using all trained MLMs, rank the different solutions by their performance metrics (bias, scatter and outlier-fractions); generate up to two types of PDF solutions using the distribution of MLM solutions, convoluted with the corresponding error estimates. Performance plots are also created.
  ```bash
  python scripts/annz_rndReg_quick.py --randomRegression --optimize
  ```

  4. **evaluate**: Evaluate an input dataset using the derived estimators: the *best* MLM, the PDF(s) and the average of the weighted and the un-weighted PDF solutions.
  ```bash
  python scripts/annz_rndReg_quick.py --randomRegression --evaluate
  ```


#### Binned classification

  1. **genInputTrees**: Convert the input ascii dataset into ROOT TTrees, which are later processed by ANNZ. Plots are also created, corresponding to distributions of each input parameter.
  ```bash
  python scripts/annz_binCls_quick.py --binnedClassification --genInputTrees
  ```

  2. **train**: Perform training of an MLM for each one of the classification bins. In each bin multiple MLM candidates are trained, where each has a different configuration (type of MLM, algorithm-parameter choices, input-parameter sets); only the candidate with the *best* performance (highest separation between classification signal and background) is kept.
  ```bash
  python scripts/annz_binCls_quick.py --binnedClassification --train
  ```

  3. **verify**: Verification that each classification bin has a corresponding trained MLM, and that the training is self-consistent. Performance plots are also created.
  ```bash
  python scripts/annz_binCls_quick.py --binnedClassification --verify
  ```

  4. **evaluate**: Evaluate an input dataset using the derived estimators: the PDF and the average of the PDF solution.
  ```bash
  python scripts/annz_binCls_quick.py --binnedClassification --evaluate
  ```


#### Single classification

  1. **genInputTrees**: Convert the input ascii dataset into ROOT TTrees, which are later processed by ANNZ. Plots are also created, corresponding to distributions of each input parameter.
  ```bash
  python scripts/annz_singleCls_quick.py --singleClassification --genInputTrees
  ```

  2. **train**: Perform training of a single MLM.
  ```bash
  python scripts/annz_singleCls_quick.py --singleClassification --train
  ```

  3. **optimize**: Verify that the training was successful and create some performance plots.
  ```bash
  python scripts/annz_singleCls_quick.py --singleClassification --optimize
  ```

  4. **evaluate**: Evaluate an input dataset using the trained MLM.
  ```bash
  python scripts/annz_singleCls_quick.py --singleClassification --evaluate
  ```

#### Randomized classification

  1. **genInputTrees**: Convert the input ascii dataset into ROOT TTrees, which are later processed by ANNZ. Plots are also created, corresponding to distributions of each input parameter.
  ```bash
  python scripts/annz_rndCls_quick.py --randomClassification --genInputTrees
  ```

  2. **train**: Perform training of multiple MLMs, where each has a different configuration (type of MLM, algorithm-parameter choices, input-parameter sets).
  ```bash
  python scripts/annz_rndCls_quick.py --randomClassification --train
  ```

  3. **optimize**: Using all trained MLMs, rank the different solutions by separation parameter between classification signal and background. Performance plots are also created.
  ```bash
  python scripts/annz_rndCls_quick.py --randomClassification --optimize
  ```

  4. **evaluate**: Evaluate an input dataset using selected MLMs from the ranked list of estimators.
  ```bash
  python scripts/annz_rndCls_quick.py --randomClassification --evaluate
  ```

### Additional options

#### Advanced examples

In addition to the scripts described above, `scripts/` also includes *advanced* examples, which illustrate more sophisticated ways to use ANNZ. These may be run exactly like in the above descriptions. For instance, for randomized regression, do:
```bash
python scripts/annz_rndReg_advanced.py --randomRegression --genInputTrees
python scripts/annz_rndReg_advanced.py --randomRegression --train
python scripts/annz_rndReg_advanced.py --randomRegression --optimize
python scripts/annz_rndReg_advanced.py --randomRegression --evaluate
```


#### Global options (`generalSettings.py`)

The `scripts/generalSettings.py` script includes the following two functions:

  1. **generalSettings()**: set additional options, which are then used for all example scripts, for all operational modes (training, optimization etc.). In addition to the given examples, any of the options defined in `myMain::myMain()` (in `src/myMain.cpp`) may be used.
  
  2. **genRndOpts**: an example for setting specific randomization options for MLM during training. In general, during submission of training jobs, the variable, `userMLMopts`, holds the MLM settings. If left empty, either an ANN or a BDT is generated, using the randomization procedure defined in `ANNZ::generateOptsMLM()` (in `src/ANNZ_train.cpp`). See e.g., `scripts/annz_rndReg_advanced.py`, for a use-example.

#### MLM algorithm options

The syntax for defining MLM options is explained in the [TMVA wiki](http://tmva.sourceforge.net/optionRef.html) and in the [TMVA manuall](http://tmva.sourceforge.net/docu/TMVAUsersGuide.pdf) (in the chapter, *The TMVA Methods*). It may be specified by the user with the `glob.annz["userMLMopts"]` variable. The only requirement not defined nominally in TMVA is `ANNZ_MLM`. This is an internal variable in ANNZ which specifies the type of MLM requested by the user. 

Here are a couple of examples:

  - For instance we can define a BDT with 110 decision trees, using the AdaBoost (adaptive boost) algorithm:
  ```python
  glob.annz["userMLMopts"] = "ANNZ_MLM=BDT:VarTransform=N:NTrees=110:NormMode=NumEvents:BoostType=AdaBoost:"
  ```

  - Similarly, we can use an ANN with two hidden layers, having `N+4` and `N+9` neurons (where `N` is the number of input parameters); the neurons will be of type `tanh`, and the training method will be the Broyden-Fletcher-Goldfarb-Shannon (BFGS) method:
  ```python
  glob.annz["userMLMopts"] = "ANNZ_MLM=ANN:HiddenLayers=N+4,N+9:NeuronType=tanh:TrainingMethod=BFGS"
  ```

See the advanced scripts for additional details.

#### Running on a batch farm

It is advisable to run ANNZ on a batch farm, especially during the training phase. An example of how this may be done is given in `scripts/annz_qsub.py`. Please note that this only serves as a guideline, and should probably be customized for a particular cluster.


#### FITS format support

The nominal input/output of ANNZ is in the format of ascii files. In order to avoid additional dependencies, FITS format support is not incorporated directly into ANNZ. Instead, two functions, `fitsToAscii()` and `asciiToFits()`, are available in `scripts/fitsFunc.py` (and require the python package, `astropy.io`). These may be used for creating ascii files in the format accepted by ANNZ from a FITS input file, and vise versa. Examples are given in `scripts/annz_fits_quick.py`. They can be run with:
```bash
python scripts/annz_fits_quick.py --fitsToAscii
python scripts/annz_fits_quick.py --asciiToFits
```


## The outputs of ANNZ

### Randomized regression

Two PDFs may be generated, derived by choosing a weighting for the trained MLMs, which is *most compatible* with the target value of the regression, `zTrg`. This is determined in two ways (generating two alternative PDFs), using cumulative PDF distributions; for the first PDF (`PDF_0` in the output), the cumulative distribution is based on the *truth* (`zTrg`). For the second PDF (`PDF_1` in the output), the cumulative distribution is based on the *best* MLM. For the former, a set of templates, derived from `zTrg` is used to fit the dataset. For the later, a flat distribution of the cumulator serves as the baseline. (The PDF derivation procedure is described in the paper in greater detail.)

We define the following name-tags:

  1. **ANNZ best**: The *best* MLM solution, that is, the solution which achieved the best combination of performance metrics (bias, scatter and outlier fractions).
  
  2. **ANNZ MLM average 0**: The un-weighted average of all MLMs which are included in the first PDF. In general, the PDF is composed by setting relative weights for the various MLMs, and convolving the MLM solutions with the corresponding uncertainty estimators. In this context, *un-weighted* means that the PDF weights are not used and that the MLMs are not convolved with the uncertainty estimators. In essence, this solution corresponds to the average solution of all MLMs which have non-zero PDF weights (the subset of the MLMs that have good performance metrics).
  
  3. **ANNZ PDF average 0**: The average of the first PDF (using the full weighted set of MLMs, convolved with uncertainty estimators).
  
  4. **ANNZ PDF 0**: The full PDF solution.
  
  5. **ANNZ MLM average 1, ANNZ PDF average 1, ANNZ PDF 1**: The corresponding estimators for the second PDF.

#### Optimization

The following data-products are produced during optimization (using, for example, `scripts/annz_rndReg_quick.py`):

  1. **output/test_randReg_quick/regres/optim/eval/ANNZ_randomReg_0000.csv**: an ascii file with the derived estimators.
  
  2. **output/test_randReg_quick/regres/optim/eval/plots/**: A directory with root scripts and pictures of performance plots (picture-format defined by `printPlotExtension`, as e.g., in `scripts/generalSettings.py`).

The first line of the output ascii file contains a list of the column names in the file.
The output ascii file of optimization/evaluation includes the regression target, the *best* MLM estimator, the averages (weighted and un-weighted) of the PDF(s) and the full PDF solution(s). The format may include e.g.,
```bash
D:Z ; F:ANNZ_best ; F:ANNZ_best_err ; F:ANNZ_best_wgt ; F:ANNZ_MLM_avg_0 ; F:ANNZ_MLM_avg_0_err ; F:ANNZ_MLM_avg_0_wgt ; F:ANNZ_PDF_avg_0 ; F:ANNZ_PDF_avg_0_err ; F:ANNZ_PDF_avg_0_wgt ; F:ANNZ_PDF_0_0 ; F:ANNZ_PDF_0_1 ....
```
Here the pattern `varType:varName` corresponds to the variable type and the variable name. For example, if `varType` is `F` the variable is a floating point number (the list of possible types is given in the advanced example scripts, under the `doGenInputTrees` condition).

In this example, `scripts/annz_rndReg_quick.py` was used. Here `Z` is the regression target, `ANNZ_best` is the best MLM estimator, and `ANNZ_best_err` and `ANNZ_best_wgt` correspond to the error estimation and the weight of the latter. The `ANNZ_PDF_0_*` are the full PDF solution for the first PDF, with one entry per PDF bin. `ANNZ_PDF_avg_0_err` is the uncertainty estimator for the PDF solution (calculated as the width of the PDF), and `ANNZ_PDF_avg_0_wgt` is the corresponding weight. Similar solutions are also defined for the second PDF.

There are two ways to define the PDF bins:

  1. The PDF is defined within `nPDFbins` equal-width bins between `minValZ` and `maxValZ` (the minimal and maximal defined values of the regression target). The `nPDFbins`, `minValZ` and `maxValZ` variables are mandatory settings for ANNZ, as defined in the example scripts.
  
  2. A specific set of bins of arbitrary with may defined by setting the variable, `userPdfBins` (in which case `nPDFbins` is ignored). The only constrain is that the first and last bin edges be within the range defined by `minValZ` and `maxValZ`. This can e.g., be
  ```python
  glob.annz["userPdfBins"] = "0.05;0.1;0.2;0.24;0.3;0.52;0.6;0.7;0.8"
  ```

It is possible to add any of the variables from the input dataset to the ascii output (for instance, it can be used to keep track of an object-id). This is controlled by setting e.g.,:
```python
glob.annz["addOutputVars"] = "MAG_U;MAGERR_I"
```
which in this example, adds the U-band magnitude and the error on the I-band magnitude to the output ascii file. The performance of the different estimators is also plotted as a function of any of these added variables.


#### Evaluation

The directory, `output/test_randReg_quick/regres/eval/` (for the `scripts/annz_rndReg_quick.py` example), contains the output ascii and ROOT tree files, respectively, `ANNZ_randomReg_0000.csv` and `ANNZ_tree_randomReg_00002.root`. These have a similar format to that which is described above.

### Single regression

The outputs of single regression are similar to those of randomized regression. In this case, the *best* MLM is actually the only MLM, and no PDF solutions are created. For instance, using `scripts/annz_singleReg_quick.py`, the performance plots will be found at `output/test_singleReg_quick/regres/optim/eval/plots/` and the output ascii file would be found at `output/test_singleReg_quick/regres/optim/eval/ANNZ_singleReg_0000.csv`. The latter would nominally include the variables:
```bash
D:Z ; F:ANNZ_best ; F:ANNZ_best_err ; F:ANNZ_best_wgt
```
which in this case correspond to the regression target, the (only) MLM solution and the corresponding error and weight estimates.

### Binned classification

The outputs of binned classification are similar to those of randomized regression. In this case, there are no individual MLM solutions, only PDF(s). For instance, using `scripts/annz_binCls_quick.py`, the performance plots will be found at `output/test_binCls_quick/binCls/verif/eval/plots/` and the output ascii file would be found at `output/test_binCls_quick/binCls/verif/eval/ANNZ_binnedCls_0000.csv`.


### Single/randomized classification

The output of a classifier is the classification probability (defined within [0,1]). The nominal ascii output, e.g., the content of `./output/test_randCls_quick/clasif/eval/ANNZ_randomCls_0000.csv` (using `scripts/annz_rndCls_quick.py`) would be
```bash
F:ANNZ_8 ; F:ANNZ_8_wgt
```

This corresponds to the *best* MLM classifier and its the relevant object-weight, where the ranking is determined by the separation parameters. (In this case the best solution was found to be the MLM with index 8.)  Besides the best MLM, it is possible to also output other solutions (by rank or by MLM-id) using the `MLMsToStore` variable. An example is provided in `scripts/annz_rndCls_advanced.py`.

Variables from the input dataset may be added to the output, using the `addOutputVars` variable, as explained above.

It is possible to also generate an uncertainty estimator for the classification, based on the KNN error estimation method or by propagating the uncertainty on the input parameters to the MLM. This is an *experimental* feature of ANNZ, which may be used e.g., to define better cuts on the classification probability, and improve the completeness/purity of the classifier. To generate classification uncertainties (see `scripts/annz_rndCls_advanced.py`), set:
```python
glob.annz["addClsKNNerr"] = True
```

### Interpretation of estimator-weights

The weights for the different estimators which were mentioned above (`ANNZ_8_wgt`, `ANNZ_best_wgt`, `ANNZ_MLM_avg_0_wgt` etc.) serve two purposes:

  1. **numerical weights:** the numerical value of the weight is composed of the weight-definition provided by the user through the `userWeights_train` and `userWeights_valid` variables, combined with the reference dataset weight, which can be added using `useWgtKNN` (see the advanced example scripts).
  
  2. **binary cuts:** objects which do not pass the cuts end up with a zero weight. A trivial example of a cut, is an object which has a value of the regression target for which `zTrg < minValZ` or `zTrg > maxValZ`. Cut may also be defined by using the `userCuts_train` and `userCuts_valid` variables (see the advanced example scripts).

A few notes:

  - The estimator weights have nothing to do with the PDF-weights discussed above. The object weights represent per-object numbers which are derived from the overall properties of an object - they can even depend on variables which are not part of the training. For instance, in the examples, one may limit the impact on training, of objects with high uncertainty on the I-band magnitude:
  ```python
  glob.annz["userWeights_train"] = "(MAGERR_I > 1)/MAGERR_I + (MAGERR_I =< 1)*1"
  ```

  - The cut variables, `userCuts_train` and `userCuts_valid`, are binary, in the sense that they define the conditions for an object to be accepted for training or validation. On the other hand, the weight variables, `userWeights_train` and `userWeights_valid`, can serve as either cuts or weights; a zero-weight is equivalent to a cut, as it effectively excludes an objects. Therefore it is possible to compose weight expressions which have "boolean" components (such as `(MAGERR_I > 1)`) which are numerically equivalent to `0` or `1`. In principle, we can therefore do everything with `userWeights_train` and `userWeights_valid`. However, due to performance considerations, it is recommended to use `userCuts_train` and `userCuts_valid` for well defined rejection criteria; then use the weight variables for everything else.

  - If as part of the ascii or ROOT output an object has a weight which is zero, then the corresponding estimator for that object should not be used!

  - The plots provided following optimization or verification all take into account the respective weights of the different estimators.

## General comments

  - If ANNZ does not compile, check the definition of `$ROOTSYS` and `$LD_LIBRARY_PATH`. The values used by ANNZ are printed out before compilation starts.

  - During the `--genInputTrees` phase, a list of input parameters is specified, corresponding to the makeup of the input ascii files; here each column in the input file is registered. Note that the training of MLM does not necessarily need to correspond to the same set of parameters. In fact, any combination of input parameters, including mathematical expressions of the latter, may be used for training. The only constraint is that (for regression problems) the regression target should correspond to exactly one of the input parameters. See the various example scripts for details.

  - The training phase may be run multiple times, in order to make sure that all MLMs have completed training successfully. By default, if a trained MLM is detected in the output directory, then ANNZ does not overwrite it. In order to force re-training, one may delete the training directory for a particular MLM (for instance, using `scripts/annz_rndReg_quick.py`, this might be `output/test_randReg_quick/regres/train/ANNZ_3`). Alternatively, it's possible to force retraining by setting in the relevant python script the flag, 
  ```python
  glob.annz["overwriteExistingTrain"] = True
  ```

  - Single regression and single classification require the optimization phase, despite the fact that only one MLM is generated. Please make sure to have run that before evaluation.

  - Randomized regression and randomized classification use all successfully trained MLMs. If some of the MLMs failed, they are ignored, and optimization/evaluation may still take place. For binned classification, all trained MLMs must be present (as each defines a given classification bin). Therefore, if any MLM has failed in training during binned classification, optimization/evaluation will fail.

  - MLM error estimates are nominally derived using a KNN-uncertainty estimator. However, it is possible to directly propagate input variable uncertainties to an uncertainty on an MLM. This is done in a simplified manner in `ANNZ::getRegClsErrINP()` (in `src/ANNZ_err.cpp`), assuming that the input variable errors are Gaussian and uncorrelated. Please see the documentation of this function for a more detailed description.
  **An important note -** the choice to propagate the input variable uncertainties instead of using the KNN method must be made during the training phase of an MLM. This is done by setting the `inputVarErrors` parameter during training (see e.g., `scripts/annz_rndReg_advanced.py`). If set to a different value during optimization/verification or evaluation, it will have no affect.

  - It is possible to use ANNZ to generate object weights, based on a reference dataset. The weights are generated as part of the `--genInputTrees` phase, and are then used for training and optimization; they are also calculated during evaluation, and added as part of the per-object weight which is included in the output of the evaluation.
  This feature is useful, if e.g., the target dataset for evaluation has a different distribution of input parameters, compared to the training dataset. For instance, for photo-z derivation, it is possible for the spectroscopic  training sample to have a different color distribution, compared to the target photometric sample. The derived weights in this case are calculated as the ratio between the number of objects in a given color-box in the reference sample, compared to the training sample. The procedure is implemented in `CatFormat::addWgtKNNtoTree()` (in `src/CatFormat_wgtKNN.cpp`), where a more detailed explanation is also given. See `scripts/annz_rndReg_advanced.py` for a use-example.


  - It is possible to train/optimize MLMs using specific cuts and/or weights, based on any mathematical expression which uses the variables defined in the input dataset (not limited to the variables used for the training). The relevant variables are `userCuts_train`, `userCuts_valid`, `userWeights_train` and `userWeights_valid`. See the advanced scripts for use-examples.

  - The syntax for math expressions is defined using the ROOT conventions (see e.g., [TMath](https://root.cern.ch/root/html/TMath.html) and [TFormula](https://root.cern.ch/root/html/TFormula.html)). Acceptable expressions may for instance be the following ridiculous choice:
  ```python
  glob.annz["userCuts_train"]    = "(MAG_R > 22)/MAG_R + (MAG_R <= 22)*1"
  glob.annz["userCuts_valid"]    = "pow(MAG_G,3) + exp(MAG_R)*MAG_I/20. + abs(sin(MAG_Z))"
  ```

  - By default, a progress bar is drawn during training. If one is writing the output to a log file, the progress bar is important to avoid, as it will cause the size of the log file to become very large. One can either add `--isBatch` while running the example scripts, or set in `generalSettings.py` (or elsewhere),
  ```python
  glob.annz["isBatch"] = True
  ```

  - The output of ANNZ includes escape sequences for color. To avoid these, set 
  ```python
  glob.annz["useCoutCol"]  = False
  ```

  - To generate the class documentation, run
  ```bash
  doxygen Doxyfile
  ```


---

The example scripts use the data stored in `examples/data/` as the input for training, validation and evaluation. The data for the regression examples (`examples/data/photoZ/`) were derived from the [catalogues](http://www.sdss3.org/dr10/spectro/spectro_access.php). These include spectroscopic data, taken with the Baryon Oscillation Spectroscopic Survey (BOSS). The data for classification (`examples/data/sgSeparation/`) were derived from the Sloan Digital Sky Survey (SDSS) dataset, following the procedure used by [Vasconcellos et al., (2010)](http://arxiv.org/abs/1011.1951). (Please also see [SDSS](https://www.sdss3.org/collaboration/boiler-plate.php).)

---

The license for ANNZ is the GNU General Public License (v3), as given in LICENSE.txt and available [here](http://www.gnu.org/licenses).

---






