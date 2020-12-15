# ANNZ v2.3.2

## Introduction
ANNZ uses both regression and classification techniques for estimation of single-value photo-z (or any regression problem) solutions and PDFs. In addition it is suitable for classification problems, such as star/galaxy classification.

ANNZ uses the [TMVA package](http://tmva.sourceforge.net/) which is based on [ROOT](https://root.cern.ch/). The current version is a completely new implementation of the original ANNZ package.

The different configurations for regression problems (such as photometric redshift estimation) are referred to as *single regression*, *randomized regression* and *binned classification*. In addition, it is possible to run ANNZ in *single classification* and *randomized classification* modes, used for general classification problems.

The algorithm is described in the dedicated paper, [Sadeh et al., PASP, Volume 128, Number 968 (2016)](http://arxiv.org/abs/1507.00490).

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

Once training is complete, optimization takes place. In this stage, a distribution of photo-z solutions for each galaxy is derived.  A selection procedure is then applied to the ensemble of answers, choosing the subset of methods which achieve optimal performance. The selected MLMs are then folded with their respective uncertainty estimates, which are derived using a KNN-uncertainty estimator (see [Oyaizu et al, 2007](http://arxiv.org/abs/0711.0962)). A set of PDF candidates is generated, where each candidate is constructed by a different set of relative weights associated with the various MLM components.

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

ROOT is available [here](https://root.cern.ch/downloading-root). ROOT v5.34/25 was used for development, and v6.12 for testing. Later ROOT versions should also be compatible (please report any issues).

For installation instructions, go [here](https://root.cern.ch/building-root). Notice the pre-compiled versions available for common operating systems.

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

Otherwise, define `$ROOTSYS` and `$LD_LIBRARY_PATH` in the session, and set in `scripts/commonImports.py`:
```python
useDefinedROOTSYS = True
```

It is also recommended to add the installation directory to the `PYTHONPATH` system variable:
```bash
export PYTHONPATH=/home/work/annz:/home/work/annz/examples:$PYTHONPATH
```
This may be needed for `python` to recognize the sub-directory package structure, where scripts are imported with e.g., `from scripts.helperFuncs import *` statements.

To install and/or clean ANNZ is possible with any of the `scripts/annz_*.py` scripts described in the following. For instance, do:
```bash
python scripts/annz_singleReg_quick.py --make
```

(This is also done automatically the first time any of the `scripts/annz_*.py` scripts are run, if the compilation directory, `lib/`, is not detected.) To force a complete re-install, one may either safely delete the `lib/` directory, or do:
```bash
python scripts/annz_singleReg_quick.py --make --clean
```


## Examples

### Quickstart guide

The various example scripts includes comments about the different variables which the user needs to set. Each operational mode has a *quickstart* dedicated script as well as an *advanced* script. The latter include more job-options as well as more detailed documentation.

For each of the following, please follow the four respective steps (generation, training, optimization/verification, evaluation) in sequence. For instance, for single regression, do:
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
  
  4. **evaluate**: Evaluate an input dataset using the trained MLM. If the regression target is detected as part of the evaluated dataset, performance plots are created as well.
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

  3. **optimize**: Using all trained MLMs, rank the different solutions by their performance metrics (bias, scatter and outlier-fractions); generate PDF solutions using the distribution of MLM solutions, convoluted with the corresponding error estimates. Performance plots are also created.
  ```bash
  python scripts/annz_rndReg_quick.py --randomRegression --optimize
  ```

  4. **evaluate**: Evaluate an input dataset using the derived estimators: the *best* MLM, the PDF(s) and the average of the weighted PDF solutions. If the regression target is detected as part of the evaluated dataset, performance plots are created as well.
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

  4. **evaluate**: Evaluate an input dataset using the derived estimators: the PDF and the average of the PDF solution. If the regression target is detected as part of the evaluated dataset, performance plots are created as well.
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

The following is the list of all available MLM algorithms:

  - **`CUTS`:** Rectangular cut optimization.
  - **`Likelihood`:** Projective likelihood estimator (PDE approach).
  - **`PDERS`:** Multidimensional likelihood estimator (PDE range-search approach).
  - **`PDEFoam`:** Likelihood estimator using self-adapting phase-space binning.
  - **`KNN`:** k-Nearest Neighbors.
  - **`HMatrix`:** H-Matrix discriminant.
  - **`Fisher`:** Fisher discriminants (linear discriminant analysis).
  - **`LD`:** Linear discriminant analysis.
  - **`FDA`:** Function discriminant analysis.
  - **`ANN`:** Artificial neural networks (nonlinear discriminant analysis) using an MLP neural network (recommended type of neural network). Also available are the Clermont-Ferrand neural network (**`CFMlpANN`**) and the original ROOT implementation  of a neural network (**`TMlpANN`**).
  - **`SVM`:** Support vector machine.
  - **`BDT`:** Boosted decision and regression trees.
  - **`RuleFit`:** Predictive learning via rule ensembles.


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

#### Definition of input samples

- Machine learning methods require two input samples for the training process. The first of these is the *training sample*, which is used explicitly for the training process - we'll refer to this sample as `S-1`. The second is sometimes called the *validation sample* and sometimes the *testing sample*. It is used for evaluating the result of the trained MLM in each step of the training - we'll refer to this sample is `S-2`. The `S-2` sample should have the same properties as `S-1`, but should be an independent collection of objects.

- The terminology between *validation* and *testing* is not always consistent. Some authors refer to a third independent sample of objects which is used to check the performance of the trained MLM after training is complete; this third sample may be referred to as a *validation* or as a *testing* sample, thus creating potential confusion with regards to `S-2`. For the purposes of using this software package, the distinction between *validation* and *testing* is not relevant. We define only two samples, `S-1` for training, and `S-2` for checking the performance during each step of the training process. The user may use the evaluation stage (after optimization/verification is complete) in order to derive the solution for any other sample of objects.

- The user may define the training/testing samples in one of two ways (see `scripts/annz_rndReg_advanced.py` for details):
  
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

- Most of the examples show the use of ascii input files. However, one may also use ROOT files as input (see `scripts/annz_rndReg_advanced.py`). For instance, one may define separate input files with corresponding ROOT Tree names, as
  ```python
  glob.annz["inTreeNameTrain"]  = "tree0"  
  glob.annz["splitTypeTrain"]   = "file_0.root;file_1.root"
  glob.annz["inTreeNameTest"]   = "tree1"  
  glob.annz["splitTypeTest"]    = "file_2.root"
  ```
  Notice that in this case, all files defined in `splitTypeTrain` must contain a ROOT Tree as defined in `inTreeNameTrain`, and the same goes for `splitTypeTest` and `inTreeNameTest`. Since the Tree names for training and testing can be different, one may e.g., use two separate Trees from a single input file for training and testing:
  ```python
  glob.annz["splitTypeTrain"]   = "file_0.root"
  glob.annz["splitTypeTest"]    = "file_0.root"
  glob.annz["inTreeNameTrain"]  = "tree0"  
  glob.annz["inTreeNameTest"]   = "tree1"  
  ```

#### Definition of signal and background objects in single/randomized classification

Signal and background objects may be defined by using at least one pair of variables, either `userCuts_sig` and `userCuts_bck` or `inpFiles_sig` and `inpFiles_bck`. Alternatively, it is also possible to use three of the latter or all four in tandem. For example:

  - Using existing parameters from the input files (e.g., the variable `type`) to define logical expressions, such as
    ```python
    glob.annz["userCuts_sig"] = "type == 3" # all objects which pass this condition are signal objects
    glob.annz["userCuts_bck"] = "type == 6" # all objects which pass this condition are background objects
    ```

  - Using an entire input file for signal or for background objects, as e.g.,
    ```python
    glob.annz["inpFiles_sig"] = "sgCatalogue_galaxy_0.txt;sgCatalogue_galaxy_1.txt" # all objects from these files are signal objects
    glob.annz["inpFiles_bck"] = "sgCatalogue_star_0.txt" # all objects from this file are background objects
    ```
  
  - A combination of variables - input file selection together with additional cuts, as e.g.,
    ```python
    # all objects from this file which pass this cut are signal objects
    glob.annz["inpFiles_sig"] = "sgCatalogue_galaxy_0.txt"
    glob.annz["userCuts_sig"] = "mE2_r < 0.5"

    # all objects from this file are background objects
    glob.annz["inpFiles_bck"] = "sgCatalogue_star_0.txt"
    ```

#### Bias correction for regression

There are two bias correction options for regression, one for MLMs (applied during the training stage) and one for PDFs (applied during the optimization/verification phase). The former may be used for single/randomized regression, and the latter for single/randomized regression and for binned classification.
The MLM correction may be turned on/off by setting `glob.annz["doBiasCorMLM"]`, while the PDF correction my be used with `glob.annz["doBiasCorPDF"]`. An example for the relevant job options is given in `scripts/annz_rndReg_advanced.py`.

##### MLM bias correction

Following training of the *nominal MLM*, a second one is trained, the *bias-MLM*.
For the following, let's assume we train a single MLM, using `python scripts/annz_singleReg_quick.py --singleRegression --train` or using `python scripts/annz_rndReg_quick.py --randomRegression --train --trainIndex=0`. The output of this is a regression estimator, designated `ANNZ_0`.

The settings of the bias-MLM are defined using the `biasCorMLMopt` parameter, which uses the same syntax as `userMLMopts`.
The inputs for the bias-MLM are the same as for the nominal, where it is also possible to add the output of the nominal MLM as an input. For instance, if the nominal MLM is trained with `glob.annz["inputVariables"] = "MAG_U;MAG_G;MAG_R"`, then the bias-MLM can be trained with either `MAG_U;MAG_G;MAG_R` or `MAG_U;MAG_G;MAG_R;ANNZ_0` as inputs. 
The regression target of the bias-MLM is the difference between the target value and the regression estimator of the nominal MLM. For our example, the target of the nominal may be `glob.annz["zTrg"] = "Z"`. In this case, the regression target for training for the bias-MLM would be `(ANNZ_0 - Z)`.

The bias correction is used by evaluating for each object the difference between the nominal MLM estimator, and the estimator of the bias correction MLM. That is, for each object we will have `de-biased estimator := MLM(nominal) - MLM(bias)`. This is applied internally in the code. The user therefore only gets the de-biased estimator as an output - this output replaces the nominal MLM solution, and has the same designation. For our example, the designation `ANNZ_0` for the output of the regression will therefore refer to the de-biased estimator. 

The MLM bias correction must be enabled during training. If using randomized regression, it is possible to use it for only a subset of the trained MLMs, by changing the value of `glob.annz["doBiasCorMLM"]` in the training loop. One should use simple MLM structures to avoid slowing down training and evaluation. For instance: `glob.annz["biasCorMLMopt"] = "ANNZ_MLM=BDT:NTrees=50"` or `glob.annz["biasCorMLMopt"] = "ANNZ_MLM=ANN::HiddenLayers=N,N+5:VarTransform=N,P:TrainingMethod=BFGS:NCycles=500:UseRegulator=True"` are reasonable baseline choices, which will not result in significant overhead - BDTs are recommended, as they are usually faster to train.

Following derivation of the bias-MLM, a simple Kolmogorov-Smirnov test is performed, comparing the performance of the nominal MLM with the de-biased MLM. If the bias correction procedure resulted in degraded performance, it is rejected; the nominal MLM is then used as the estimator instead.


##### PDF bias correction

The bias correction for PDFs may be chained after the MLM bias correction, or just used on it's own. The PDF correction is computed as part of the optimization/verification phase, and does not need to be defined during training.

The correction is applied as a simplified unfolding of the regression solution with regards to the target. This is done by calculating the correlation for each bin of the PDF, between the target value and the position of the bin. The correlation is computed for each object in the training sample, so that the average per-bin correlation may be derived for the entire training sample. The correction is applied on the evaluated PDF. The is done by re-weighting each bin of the PDF according to the average estimated value of the regression target. The procedure is equivalent to multiplying the PDF (bin-by-bin) by the relation between the derived regression estimator (photo-z) and the target value (true redshift), as derived using the training sample.


#### Independent derivation of KNN error estimates

There is the option to generate error estimation (using the KNN method) for a general input dataset.
The evaluated dataset does not need to be created using `ANNZ`. That is, one can estimate photo-zs (or any other type of regression problem) using external software. `ANNZ` can then be used to derive errors, so long as one provides the corresponding reference dataset.

Some specifics:

  - The reference dataset must include the true value of the evaluated object, as well as a list of variables which are used for the KNN-search.
  
  - The errors are computed by finding the near neighbours of the evaluated object. We compute
    the distribution of errors, defined for a given object as `ERR == (eval_value - true_value)`.
  
  - The final error is derived as the width of the distribution of `ERR`. `ANNZ` provides both a symmetric error, and error estimates in the negative and in the positive directions (towards lower and higher values of the regression variable).
  
See `scripts/annz_rndReg_knnErr.py` for a complete example script. For illustration, the following job-options:
  
  - We define the evaluated variable and it's respective true value as e.g.,
  ```python
  glob.annz["zReg_onlyKnnErr"] = "myPhotoZ"
  glob.annz["zTrg"]            = "Z"
  ```
  where in this case, `myPhotoZ` is a photo-z estimator and `Z` is the true redshift.
  
  - The reference dataset may include the following variables:
  ```python
  glob.annz["inAsciiVars"] = "D:Z;F:myPhotoZ;F:MAG_U;F:MAG_G;F:MAG_R;F:MAG_I;F:MAG_Z"
  ```
  where the rest of the variables, besides `myPhotoZ` and `Z`, are auxiliary variables.
  
  - The KNN errors will be conducted using e.g., 
  ```python
  glob.annz["knnVars_onlyKnnErr"] = "MAG_U;MAG_G;(MAG_G-MAG_R);(MAG_R-MAG_I);(MAG_I-MAG_Z)"
  ```
  or any other functional combinations of the auxiliary variables.
  
  - An evaluated dataset, for which we derive the KNN errors, should then, in this example, include the following parameters:
  ```python
  glob.annz["inAsciiVars"] = "F:MAG_U;F:MAG_G;F:MAG_R;F:MAG_I;F:MAG_Z"
  ```
  That is, at the very least, the evaluated dataset must include all of the parameters listed in `glob.annz["knnVars_onlyKnnErr"]`, but it does not need to include either `myPhotoZ` or `Z`. (It is also allowed to include here other variables which are not used at all; these can then be copied over to the output catalogue using `glob.annz["addOutputVars"]`.)
  
  - The output catalogue will be stored in this example at `./output/test_knnErr/onlyKnnErr/eval/ANNZ_onlyKnnErr_0000.csv`. It will contain the variables, `F:myPhotoZ_err;F:myPhotoZ_errN;F:myPhotoZ_errP`, as well as any requested observer variables from the original dataset. Here `myPhotoZ_err` is the symmetric error estimated for `myPhotoZ`, and `myPhotoZ_errN`/`myPhotoZ_errP` the estimates for errors in the negative/positive directions, respectively.


### FITS format support

The nominal input/output of ANNZ is in the format of ascii files. In order to avoid additional dependencies, FITS format support is not incorporated directly into ANNZ. Instead, two functions, `fitsToAscii()` and `asciiToFits()`, are available in `scripts/fitsFunc.py` (and require the python package, `astropy.io`). These may be used for creating ascii files in the format accepted by ANNZ from a FITS input file, and vise versa. Examples are given in `scripts/annz_fits_quick.py`. They can be run with:
```bash
python scripts/annz_fits_quick.py --fitsToAscii
python scripts/annz_fits_quick.py --asciiToFits
```

### Running on a batch farm

It is advisable to run ANNZ on a batch farm, especially during the training phase. An example of how this may be done is given in `scripts/annz_qsub.py`. Please note that this only serves as a guideline, and should probably be customized for a particular cluster.


### Python pipeline integration

- Nominally, the input data to ANNZ are ingested from a source file (an ascii or ROOT file). For evaluation, there is also the option to call ANNZ on an object-by-object basis, directly from python. This can be done using a wrapper class defined in `py/ANNZ.py`.

- A full example is given in `scripts/annz_evalWrapper.py`. Schematically, the steps to use the wrapper are as follows:
  
  1. Setup the wrapper class with some user options (the same as would be used during nominal evaluation). Add a dedicated list of input parameters, defined in `inVars` (using the same syntax as for the `inAsciiVars` parameter):
  ```python
  from py.ANNZ import ANNZ
  opts = dict()
  opts['doRegression'] = True
  opts["inVars"] = "F:MAG_U;F:MAGERR_U;F:MAG_G;F:MAGERR_G;F:MAG_R;F:MAGERR_R;F:MAG_I;F:MAGERR_I;F:MAG_Z;F:MAGERR_Z"
  ...
  annz = ANNZ(opts)
  ```
  
  2. Call the evaluation function of the wrapper, providing values for the predefined set of inputs:
  ```python
  input = {
    'MAG_U':23.242401, 'MAGERR_U':1.231664, 'MAG_G':22.895664, 'MAGERR_G':0.675091, 'MAG_R':21.431746, 'MAGERR_R':0.225735, 'MAG_I':20.430061, 'MAGERR_I':0.111847, 'MAG_Z':20.008024, 'MAGERR_Z':0.108993 
  }
  output = annz.eval(input)
  ```
  where `output` is a dict containing the evaluation results.

  3. Call the cleanup function of the wrapper once done (for a graceful release of resources):
  ```python
  annz.cleanup()
  ```

- Step 1. (initialize) may take a bit of time, as MLM estimators and ROOT trees are being loaded on the `C++` side; it should be done once at startup. Step 2. (evaluate) is quick and may be called with little overhead. It can e.g., be integrated as part of a python loop. The wrapper object should remain valid throughout the life cycle of the pipeline, in order to keep the `C++` resources booked.
- `py/ANNZ.py` is implemented with a thread lock, which allows multiple instances to be run concurrently (e.g., for different types of estimators or for different inputs).

## The outputs of ANNZ

### Randomized regression

A PDF may be generated, derived by choosing a weighting for the trained MLMs, which is *most compatible* with the target value of the regression, `zTrg`. This is determined using cumulative PDF distributions. Nominally, the cumulative distribution is based on the *truth* (`zTrg`); it is derived using a simple random walk alg, and is denoted as `PDF_0` in the output. Prior to `ANNZ v2.3.0`, two other derivations of PDFs were used, the first also based on `zTrg`, and the second on the *best* MLM. The two deprecated PDFs are not guaranteed to be supported in the future; they may currently still be generated, by setting `glob.annz["addOldStylePDFs"] = True` and `glob.annz["nPDFs"] = 2` or `3`. They are respectively denoted as `PDF_1` and `PDF_2` in the output. (The metric for deriving the various PDFs is described in the paper in greater detail.)

We define the following name-tags:

  1. **ANNZ best (`ANNZ_best`)**: The *best* MLM solution, that is, the solution which achieved the best combination of performance metrics (bias, scatter and outlier fractions).
  
  2. **ANNZ MLM average 0 (`ANNZ_MLM_avg_0`)**: The un-weighted average of all MLMs which are included in the first PDF. In general, the PDF is composed by setting relative weights for the various MLMs, and convolving the MLM solutions with the corresponding uncertainty estimators. In this context, *un-weighted* means that the PDF weights are not used and that the MLMs are not convolved with the uncertainty estimators. In essence, this solution corresponds to the average solution of all MLMs which have non-zero PDF weights (the subset of the MLMs that have good performance metrics).
  
  3. **ANNZ PDF average 0 (`ANNZ_PDF_avg_0`)**: The average of the first PDF (using the full weighted set of MLMs, convolved with uncertainty estimators).

  4. **ANNZ PDF maximum 0 (`ANNZ_PDF_max_0`)**: The maximal value (the peak) of the first PDF.
  
  5. **ANNZ PDF 0 (`PDF_0_*`)**: The full PDF solution.
  
  6. **`ANNZ_MLM_avg_1`, `ANNZ_PDF_avg_1`, `ANNZ_PDF_max_1` and `PDF_1_*`**: The corresponding estimators for the second PDF.

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
  
  2. A specific set of bins of arbitrary width may defined by setting the variable, `userPdfBins` (in which case `nPDFbins` is ignored). The only constrain is that the first and last bin edges be within the range defined by `minValZ` and `maxValZ`. This can e.g., be
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

In addition to the above-mentioned variables, the parameter `inTrainFlag` is included in the output, provided the user sets:
```python
glob.annz["addInTrainFlag"] = True
```
(See `scripts/annz_rndReg_advanced.py` or `scripts/annz_rndReg_weights.py`.)

  - This output indicates if the an evaluated object is "compatible" with corresponding objects from the training dataset. The compatibility is estimated by comparing the density of objects in the training dataset in the vicinity of the evaluated object. If the evaluated object belongs to an area of parameter-space which is not represented in the training dataset, we will get `inTrainFlag = 0`. In this case, the output of the training is probably unreliable.

  - The calculation is performed using a KNN approach, similar to the algorithm used for the `useWgtKNN` calculation. It is possible to generate either binary flags (i.e., `inTrainFlag = 0` or `1`) or to get a floating-point value with in the range, `[0,1]`. The binary decision is based on the `maxRelRatioInRef_inTrain` parameter; the latter represents a threshold for the relative density of objects from the training sample in the area of the evaluated object. By default, `maxRelRatioInRef_inTrain = -1`. In this case, the user is expected to study the distribution of the `inTrainFlag` parameter, and decide on an appropriate cut value as a quality criteria. If the user sets ` 0 < maxRelRatioInRef_inTrain < 1`, this is equivalent to choosing the cut parameter in advance. In this case the quality flag is binary (i.e., all objects for which `inTrainFlag < maxRelRatioInRef_inTrain` will get a flag rounded down to `0`, the rest will get `1` ).

  - It is recommended to first generate a floating-point estimate of `inTrainFlag` and to study the distribution. One could e.g., decide to discard objects with a low value of `inTrainFlag`, based on how the bias or scatter increase as `inTrainFlag` decreases. Them, once a proper cut value for `maxRelRatioInRef_inTrain` is determined, `inTrainFlag` may be set to produce a binary decision.


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

### Performance plots for regression

Performance plots are created during optimization/validation (possibly also during evaluation).

The plots can take into account weights, if the `userWeights_metricPlots` is set. Numerical and logical expressions are allowed. As an example, let's assume we set the following during evaluation:
```python
glob.annz["userWeights_metricPlots"] = "((MAGERR_U <= 0.2) && (MAGERR_R < 1)) * ( (MAGERR_G<0.5)*6 + (MAGERR_G>=0.5)*1 )"
```
The result would be that all objects with `(MAGERR_U > 0.2)` or `(MAGERR_R >= 1)` will not be included in the plots (zero weight), and that all objects with `(MAGERR_G<0.5)` will have 6 times the weight compared to objects with `(MAGERR_G>=0.5)`.
The weights thus derived will also be included in the output of the evaluation (i.e., as part of `output/test_singleReg_quick/regres/optim/eval/ANNZ_singleReg_0000.csv` for single regression), under the corresponding variable-weight name (`ANNZ_best_wgt` for single regression, or the other `*_wgt` variables for MLMs and PDFs).
The content of `userWeights_metricPlots` can only include mathematical expressions using the variables defined in `inAsciiVars` for the corresponding evaluated sample. In addition, one may use the `inTrainFlag` parameter as a weight (if the user chooses to generate it as part of the pipeline). 

For optimization and validation, the `userWeights_train` and/or `userWeights_valid` weight expressions are also included by default (assuming these were defined during training). In addition, the KNN-weights (if `useWgtKNN` was set) are also automatically taken into account. (The combination of all of these are used in addition to `userWeights_metricPlots`.)

If the regression target is detected as part of the evaluated dataset, plots are created. These are found e.g., at `output/test_randReg_quick/regres/eval/plots/`. This is useful if, for instance, one wants to easily check the performance on a dataset which was not used for training/testing.

The plots will also include the dependence of the performance on any added variable in the evaluated dataset. For instance, if one sets `glob.annz["addOutputVars"] = "MAG_Z"`, then the dependence of the bias, scatter etc. on `MAG_Z` will be plotted. This is particularly useful for assessing the dependence of the performance on the `inTrainFlag` parameter, in order to decide on a cut value for the latter.

### Interpretation of estimator-weights

The weights for the different estimators which were mentioned above (`ANNZ_8_wgt`, `ANNZ_best_wgt`, `ANNZ_MLM_avg_0_wgt` etc.) serve two purposes:

  1. **numerical weights:** the numerical value of the weight is composed of the weight-definition provided by the user through the `userWeights_train` and `userWeights_valid` variables, combined with the reference dataset weight, which can be added using `useWgtKNN` (see the advanced example scripts). 
  
  2. **binary cuts:** objects which do not pass the cuts end up with a zero weight. A trivial example of a cut, is an object which has a value of the regression target for which `zTrg < minValZ` or `zTrg > maxValZ`. Cut may also be defined by using the `userCuts_train` and `userCuts_valid` variables (see the advanced example scripts).

A few notes:

  - For instance, lets assume that `userWeights_train`, `userWeights_valid`, `userCuts_train` and `userCuts_valid` are not set, while `useWgtKNN = True`. In this case, the weight variables (i.e., `ANNZ_best_wgt`) would correspond exactly to the reference dataset correction factors, which are stored e.g., in `output/test_randReg_advanced/rootIn/ANNZ_KNN_wANNZ_tree_valid_0000.csv`. This naturally only holds for the training/validation dataset. For a general evaluation sample, weight variables such as `ANNZ_best_wgt` would be derived by `userWeights_train`, `userWeights_valid`, `userCuts_train` and `userCuts_valid` alone.

  - The estimator weights have nothing to do with the PDF-weights discussed above. The object weights represent per-object numbers which are derived from the overall properties of an object - they can even depend on variables which are not part of the training. For instance, in the examples, one may limit the impact on training of objects with high uncertainty on the I-band magnitude:
  ```python
  glob.annz["userWeights_train"] = "(MAGERR_I > 1)/MAGERR_I + (MAGERR_I =< 1)*1"
  ```

  - The cut variables, `userCuts_train` and `userCuts_valid`, are binary, in the sense that they define the conditions for an object to be accepted for training or validation. On the other hand, the weight variables, `userWeights_train` and `userWeights_valid`, can serve as either cuts or weights; a zero-weight is equivalent to a cut, as it effectively excludes an objects. Therefore it is possible to compose weight expressions which have "boolean" components (such as `(MAGERR_I > 1)`) which are numerically equivalent to `0` or `1`. In principle, we can therefore do everything with `userWeights_train` and `userWeights_valid`. However, due to performance considerations, it is recommended to use `userCuts_train` and `userCuts_valid` for well defined rejection criteria; then use the weight variables for everything else.

  - The plots provided following optimization or verification all take into account the respective weights of the different estimators.


## General comments

  - If ANNZ does not compile, check the definition of `$ROOTSYS` and `$LD_LIBRARY_PATH`. The values used by ANNZ are printed out before compilation starts.

  - Mac users may need to install the `Xcode Command Line Tools` before compiling against ROOT.

  - During the `--genInputTrees` phase, a list of input parameters is specified, corresponding to the makeup of the input ascii files; here each column in the input file is registered. Note that the training of MLM does not necessarily need to correspond to the same set of parameters. In fact, any combination of input parameters, including mathematical expressions of the latter, may be used for training. The only constraint is that (for regression problems) the regression target should correspond to exactly one of the input parameters. See the various example scripts for details.
  An example for a set of input parameters is
  ```python
 glob.annz["inputVariables"] = "MAG_U*(MAG_U < 99)+28*(MAG_U >= 99) ; MAG_G*(MAG_G < 99)+25*(MAG_G >= 99) ; MAG_R*(MAG_R < 99)+23*(MAG_R >= 99) ; MAG_I*(MAG_I < 99)+22*(MAG_I >= 99) ; MAG_Z*(MAG_Z < 99)+22*(MAG_Z >= 99)"
  ```
  where non-detection of magnitudes (usually indicated by setting a magnitude to `100`) are mapped to the magnitude limits in the different bands. This avoids training/evaluating with nonsensical numerical values; it also does not require any special pre-processing of the input dataset, as the conditions are set on the fly during the training and evaluation stages.

  - The training phase may be run multiple times, in order to make sure that all MLMs have completed training successfully. By default, if a trained MLM is detected in the output directory, then ANNZ does not overwrite it. In order to force re-training, one may delete the training directory for a particular MLM (for instance, using `scripts/annz_rndReg_quick.py`, this might be `output/test_randReg_quick/regres/train/ANNZ_3`). Alternatively, it's possible to force retraining by setting in the relevant python script the flag, 
  ```python
  glob.annz["overwriteExistingTrain"] = True
  ```

  - Single regression and single classification require the optimization phase, despite the fact that only one MLM is generated. Please make sure to have run that before evaluation.

  - Randomized regression and randomized classification use all successfully trained MLMs. If some of the MLMs failed, they are ignored, and optimization/evaluation may still take place. For binned classification, all trained MLMs must be present (as each defines a given classification bin). Therefore, if any MLM has failed in training during binned classification, optimization/evaluation will fail.

  - MLM error estimates are nominally derived using a KNN-uncertainty estimator. However, it is possible to directly propagate input variable uncertainties to an uncertainty on an MLM. This is done in a simplified manner in `ANNZ::getRegClsErrINP()` (in `src/ANNZ_err.cpp`), assuming that the input variable errors are Gaussian and uncorrelated. Please see the documentation of this function for a more detailed description.
  **An important note -** the choice to propagate the input variable uncertainties instead of using the KNN method must be made during the training phase of an MLM. This is done by setting the `inputVarErrors` parameter during training (see e.g., `scripts/annz_rndReg_advanced.py`). If set to a different value during optimization/verification or evaluation, it will have no affect.

  - It is possible to use ANNZ to generate object weights, based on a reference dataset. The weights are generated as part of the `--genInputTrees` phase using the `useWgtKNN` option, and are then used for training and optimization; they are also calculated during evaluation, and added as part of the per-object weight which is included in the output of the evaluation.
  This feature is useful, if e.g., the target dataset for evaluation has a different distribution of input parameters, compared to the training dataset. For instance, for photo-z derivation, it is possible for the spectroscopic  training sample to have a different color distribution, compared to the target photometric sample. The derived weights in this case are calculated as the ratio between the number of objects in a given color-box in the reference sample, compared to the training sample. The procedure is implemented in `CatFormat::addWgtKNNtoTree()` (in `src/CatFormat_wgtKNN.cpp`), where a more detailed explanation is also given. See `scripts/annz_rndReg_advanced.py` for a use-example.

  - Using the script, `scripts/annz_rndReg_weights.py`, it is possible to generate the weights based on the KNN method (`useWgtKNN`), and/or the `inTrainFlag` quality-flag, without training/evaluating any MLMs. That is, no machine-learning or photo-z training is needed. Instead, this script may be used to simple generate training weights, or conversely derive the `inTrainFlag` quality-flag, for a given evaluated sample, with respect to a specific reference sample. The former are stored to e.g., `output/test_randReg_weights/rootIn/ANNZ_KNN_wANNZ_tree_valid_0000.csv`, and the latter to `output/test_randReg_weights/inTrainFlag/inTrainFlagANNZ_tree_wgtTree_0000.csv`.

  - The KNN error, weight and quality-flag calculations are nominally performed for rescaled variable distributions; each input variable is mapped by a linear transformation to the range `[-1,1]`, so that the distance in the input parameter space is not biased by the scale (units) of the different parameters. It is possible to prevent the rescalling by setting the following flags to `False`: `doWidthRescale_errKNN`, `doWidthRescale_wgtKNN` and `doWidthRescale_inTrain`.
  These respectively relate to the KNN error calculation, the reference dataset reweighting, and the training quality-flag.

  - It is possible to train/optimize MLMs using specific cuts and/or weights, based on any mathematical expression which uses the variables defined in the input dataset (not limited to the variables used for the training). The relevant variables are `userCuts_train`, `userCuts_valid`, `userWeights_train` and `userWeights_valid`. See the advanced scripts for use-examples.

  - The syntax for math expressions is defined using the ROOT conventions (see e.g., [TMath](https://root.cern.ch/root/html524/TMath.html) and [TFormula](https://root.cern.ch/root/html/TFormula.html)). Acceptable expressions may for instance include the following ridiculous choice:
  ```python
  glob.annz["userCuts_train"]    = "(MAG_R > 22)/MAG_R + (MAG_R <= 22)*1"
  glob.annz["userCuts_valid"]    = "pow(MAG_G,3) + exp(MAG_R)*MAG_I/20. + abs(sin(MAG_Z))"
  ```

  - Note that training variables (defined in `inputVariables`) can only include expressions containing integer or floating-point variables. However, for the cut and weight variables, it is possible to also use string variables.
    
    - For instance, let's assume that `inAsciiVars` included the variable `FIELD`, which gives the name of the field for each galaxy in the training and validation datasets. Then, one may e.g., set cuts and weights of the form:
    ```python
    glob.annz["userCuts_train"]    = "    (FIELD == \"FIELD_0\") ||     (FIELD == \"FIELD_1\")"
    glob.annz["userCuts_valid"]    = "    (FIELD == \"FIELD_1\") ||     (FIELD == \"FIELD_2\")"
    glob.annz["userWeights_train"] = "1.0*(FIELD == \"FIELD_0\") +  2.0*(FIELD == \"FIELD_1\")"
    glob.annz["userWeights_valid"] = "1.0*(FIELD == \"FIELD_1\") +  0.1*(FIELD == \"FIELD_2\")"
    ```
    Here, training is only done using `FIELD_0` and `FIELD_1`; validation is weighted such that galaxies from `FIELD_1` have 10 times the weight compared to galaxies from `FIELD_2` etc.
    
    - The same rules also apply for the weight and cut options for the KNN re-weighting method: `cutInp_wgtKNN`, `cutRef_wgtKNN`, `weightRef_wgtKNN` and `weightInp_wgtKNN`, and for the corresponding variables for the evaluation compatibility test (the `inTrainFlag` parameter): `cutInp_inTrain`, `cutRef_inTrain`, `weightRef_inTrain` and `weightInp_inTrain`. (Examples for the re-weighting and for the compatibility test using these variables are given in `scripts/annz_rndReg_advanced.py`.)

  - By default, the output of evaluation is written to a subdirectory named `eval` in the output directory. An output file may e.g., be `./output/test_randReg_quick/regres/eval/ANNZ_randomReg_0000.csv`. It is possible to set the the `evalDirPostfix` variable in order to change this. For instance, setting
  ```python
  glob.annz["evalDirPostfix"] = "cat0"
  ```
  will produce the same output file at `./output/test_randReg_quick/regres/eval_cat0/ANNZ_randomReg_0000.csv`. This may be used in order to run the evaluation on multiple input files simultaneously, without overwriting previous results.

  - There are several parameters used to tune PDFs in randomized regression. Here are a couple of principle examples:

    - **`optimWithScaledBias` -** may be used to set the preferred criteria for optimizing the *best* MLM and the PDFs. By default this is `False`, so that the bias, `delta == zReg-zTrg`, is used for the optimization, where `zReg` is the estimated result of the MLM/PDF and `zTrg` is the true (target) value. If set to `True`, then the scaled bias, `deltaScaled == delta/(1+zTrg)`, is used instead. In this case, also the corresponding scatter, and outlier fractions are derived from the distribution of `deltaScaled`.
    
    - **`optimCondReg` -** may be used to set the preferred criteria for optimizing the *best* MLM and the PDFs. The options are `bias`, or `sig68`, or `fracSig68`. These respectively stand for prioritizing the minimization of the absolute value of the bias; or of the scatter of the bias distribution; or of the corresponding outlier fraction. If `glob.annz["optimWithScaledBias"] = True`, the options for `optimCondReg` do not change, but the scaled bias, `deltaScaled`, is used instead of the bias.

    - **`optimWithMAD` -** if set to `True`, we calculate the optimizing of the *best* MLM and that of the PDFs using the MAD (median absolute deviation), instead of the 68th percentile, of the bias distribution. By default `annz["optimWithMAD"] = False`.

    - **`max_sigma68_PDF`, `max_bias_PDF`, `max_frac68_PDF` -** may be set to put a threshold on the maximal value of the scatter (`max_sigma68_PDF`), bias (`max_bias_PDF`) or outlier-fraction (`max_frac68_PDF`) of an MLM, which may be included in the PDF. For instance, setting
      ```python
      glob.annz["max_sigma68_PDF"] = 0.04
      ```
    will insure that any MLM which has scatter higher than `0.04` will not be included in the PDF.

    - **`minPdfWeight` -** may be used to set a minimal weights for an MLM in the PDF. For instance, setting
      ```python
      glob.annz["minPdfWeight"] = 0.05
      ```
    will insure that each MLM will have at least 5% relative significance in the PDF. That is, in this case, no more than 20 MLMs will be used for the PDF. (This option is only used for the two deprecated PDFs, and my be removed in the future.)

    - **`max_optimObj_PDF` -** may be used to limit the number of objects from the training sample to use as part of the random walk alg for deriving `PDF_0`. This should only be set if the optimization stage takes too long to run.

    - **`nOptimLoops` -** may be used to change the maximal number of steps taken by the random walk alg deriving `PDF_0`. Note that the random walk alg will likely end before `nOptimLoops` steps in any case; this will happen after a pre-set number of steps, during which the solution does not improve.

  - **`doMultiCls`:** Using the *MultiClass* option of binned classification, multiple background samples can be trained simultaneously against the signal. This means that each classification bin acts as an independent sample during the training. The MultiClass option is only compatible with four MLM algorithms: `BDT`, `ANN`, `FDA` and `PDEFoam`. For `BDT`, only the gradient boosted decision trees are available. That is, one may set `:BoostType=Grad`, but not `:BoostType=Bagging` or `:BoostType=AdaBoost`, as part of the `userMLMopts` option.

  - By default, a progress bar is drawn during training. If one is writing the output to a log file, the progress bar is important to avoid, as it will cause the size of the log file to become very large. One can either add `--isBatch` while running the example scripts, or set in `generalSettings.py` (or elsewhere),
  ```python
  glob.annz["isBatch"] = True
  ```

  - It is possible to use root input files instead of ascii inputs. In this case, use the `splitTypeTrain`, `splitTypeTest`, `inAsciiFiles` and `inAsciiFiles_wgtKNN` variables in the same way as for ascii inputs; in addition, specify the name of the tree inside the root files. The latter is done using the variable `inTreeName` (for the nominal set) or `inTreeName_wgtKNN` (for the `inAsciiFiles_wgtKNN` variable). An example is given in `scripts/annz_rndReg_advanced.py`.

  - *Slow/hanging evaluation:* For large numbers of MLMs or for complex MLM structures (e.g., very large BDTs), it is possible that evaluation will become very slow. This may happen if too much memory is required to load all of the different estimators at once. In case of hanging evaluation, one may try to set
  ```python
  nDivs = 2
  glob.annz["nDivEvalLoops"] = nDivs
  ```
  The `nDivEvalLoops` variable splits the evaluation phase into two steps. First sub-samples of the ensemble of MLMs are evaluated for each object. Then, the entire evaluated dataset is reprocessed as for the nominal evaluation mode. (This is an internal mechanism, transparent to the user.)
  In this example, `nDivs = 2`, results in the ensemble of MLMs being split in two sub-samples. Higher values of `nDivs` are allowed, but this may incur a computing overhead. (Nominally, we have `glob.annz["nDivEvalLoops"] = 1`, for which evaluation is not split at all, and the overhead is avoided.)

  - The output of ANNZ includes escape sequences for color. To avoid these, set the corresponding `UseCoutCol` variable in `include/commonInclude.hpp`:
  ```c++
  #define UseCoutCol true
  ```
  to `false`, and recompile the code.

  - To generate the class documentation, run
  ```bash
  doxygen Doxyfile
  ```


---

The example scripts use the data stored in `examples/data/` as the input for training, validation and evaluation. The data for the regression examples (`examples/data/photoZ/`) were derived from [these catalogues](http://www.sdss3.org/dr10/spectro/spectro_access.php). The latter include spectroscopic data taken with the Baryon Oscillation Spectroscopic Survey (BOSS). The data for classification (`examples/data/sgSeparation/`) were derived from the Sloan Digital Sky Survey (SDSS) dataset, following the procedure used by [Vasconcellos et al., (2010)](http://arxiv.org/abs/1011.1951). (Please also see [SDSS](https://www.sdss3.org/collaboration/boiler-plate.php).)

---

The license for ANNZ is the GNU General Public License (v3), as given in LICENSE.txt and available [here](http://www.gnu.org/licenses).

---






