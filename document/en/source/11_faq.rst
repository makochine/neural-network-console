FAQ
===

.. _setup-1:

Setup
-----

“Access denied” is displayed, and the program will not start.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The folder in which |Console| was unzipped may not be writable from |Console|. This occurs such as when the unzipped |Console| is copied into the Program Files folder (folders under the Program Files folder cannot be written to from applications). Install |Console| in a folder that can be written to freely (immediately under drive C, on the desktop, in the document folder, etc.).

“The ordinal 4540 could not be located in the dynamic link library LIBEAY32.dll” is displayed when the program is started.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The installed OpenSSL may be of an older version. Install the latest OpenSSL.

“DLL load failed: The specified module could not be found” is displayed, and training cannot be executed.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Visual Studio 2015 Visual C++ Redistribution Package may not be installed in your PC. Download it from the following page, and install it.

https://www.microsoft.com/ja-jp/download/details.aspx?id=52685

“CUDA driver version is insufficient for CUDA runtime version” is displayed, and training cannot be executed.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The graphic driver installed in your PC may not be the latest version. Down load the latest graphic driver from the NVIDIA page, and install it.

An error occurs when training is executed with a sample project.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

If |Console| is installed in an environment that already has Python or the like installed, training may not be performed correctly. Temporarily uninstall Python or the like, or remove the PATH to them in the environment variables to isolate the problem.

**Reference**

|Console| calls Python contained in the zip file to run Python code. The Python used in this instance is specified dynamically through a PATH environment variable specified on the setup window on the |Console| GUI, but when Python is available in a folder specified by the OS PATH, it may be used with higher priority.

I can’t perform training using a GPU.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Check that the GPU board driver is the latest version.

|Console| currently only supports NVIDIA (CUDA compatible) GPUs. GPUs in AMD and Intel chipsets cannot be used.

Training is slow. Is there a way to know whether the GPU is working properly?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Software for monitoring the GPU usage can be used.

If the GPU load is 0% most of the time, it may be that the GPU is not being used properly. Check the settings on the setup window and whether the GPU is supported by the layers that you are using.

If the GPU load is not maintained at 50% or higher, loading training data from the disk may be falling behind, the batch size may be too small, a network structure with small array size may be in use, many layers not suitable for parallel computing are in use, the data size may be too small for effective parallel computing to begin with (many of the included sample projects fall under this description), and so on.

Further, training speed has been found to slow down due to the GPU’s used memory size reaching 100% depending on the environment. For this phenomenon, the problem may be resolved by updating the display driver to the latest version.

The GPU environment cannot be set up or training using a GPU does not work on a remote desktop environment.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

By design, Windows may disable the GPU when remote desktop connection is in use depending on the environment. The problem may be resolved by updating the display driver to the latest version. Perform the operation on a real machine, or try a non-Windows standard remote desktop that does not disable the GPU.

Dataset
-------

Can image data with 16-bit brightness resolution be used?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Currently, only 16-bit images in png format are supported. As core library’s internal computation is performed in 32-bit floating point, using 16-bit png images for training and validation data allows training and evaluation in 16-bit resolution. Even when training is performed using 16-bit images, the evaluation images shown on the Evaluation tab are 8 bit.

Can images or data with different sizes be used?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Data with the same variable name (e.g., x) must be of the same size. Different variables (e.g., x and y) can use different sizes.


.. _#during-training-execution:

During training execution
-------------------------

“Creating cache data…” takes an extremely long amount of time.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Cache data is being created based on the dataset input in CSV format in order to perform high-speed training. If there are a lot of training data samples and the disk access speed is slow, creating this cache data may take a long time.

If you are using a slow-speed storage, such as a network drive or archive hard disk, to store datasets, consider using NVMe SSD or the like.

Cache data is created in a folder with the name CSV file name+”.cache”.

By opening the cache data folder, you can check the cache creation status (a file is created for every 100 data samples).

If you select the Enable Dataset Cache check box on the Dataset tab, cache data created in the past is used. This enables subsequent cache data recreation to be skipped. If you update the CSV file, you must clear the Enable Dataset Cache check box to execute training and recreate the cache data.

Loss and error on the validation data do not decrease monotonically.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This may be because the number of data samples is insufficient or the network structure is not appropriate. See also ":ref:`Training does not progress properly such as NaN being displayed for the evaluation values <#training-does-not-progress-properly-such-as-nan-being-displayed-for-the-evaluation-values>`" in the FAQ.

Another reason may be that similar data samples are being provided consecutively (not shuffled) for the training datasets. |Console|, while loading data in order from the beginning of the provided training dataset, performs parameter optimization based on the mini-batch gradient descent method that handles data in units of mini-batches whose size is specified by Batch Size on the Config tab. When this is done, it is ideal for the data contained in each mini-batch to be full of variations to perform optimization efficiently. The situation may improve by shuffling the rows in the dataset CSV file in advance or by selecting the Shuffle check box on the Config tab to randomly shuffle the dataset at the time of execution.

.. _#training-does-not-progress-properly-such-as-nan-being-displayed-for-the-evaluation-values:

Training does not progress properly such as NaN being displayed for the evaluation values.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The input data values must be adjusted so that they roughly fall between -1.0 and 1.0. If there are data samples that deviate greatly from this range, NaN may occur during the neural network computation process (for image input, the brightness is normalized in the range 0.0 ≤ x < 1.0 by default).

The initialization function (Initializer) for Weight of Affine or Convolution may not be set properly.

The combination of the Loss layer and its previous Activation layer may not be appropriate. To perform regression on continuous values, specify a SquaredError layer without an Activation layer. To perform binary classification of 0s and 1s, specify a Simoid + BinaryCrossEntropy layer. To solve a category classification problem, specify a Softmax + CategoricalCrossEntropy layer.

You may be able to avoid the phenomenon by replacing Tanh or other layers in which NaN occurs when the input value deviates from a given range (via inf) with a layer such as ReLU that NaN never occurs.

“MemoryError” or “Out of memory” appears in the log output.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

There is a shortage in memory. If you are using a GPU, consider employing a GPU with larger memory (replicating cutting-edge image classification problems requires at least 12 GB of GPU memory (8 GB bare minimum).

To resolve memory shortage without changing the GPU, you can make the size of the input data smaller (for example, if it is an image, downsample to a lower resolution), set Batch Size to a smaller value on the Config tab (change the default value of 100 to 50, 20, or even 1), edit the network structure so that it uses less memory (Output and CostParameter indicated in the network statistics at the right on the Edit tab).

After loading trained parameters and editing the network, an error occurs when training is started.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The parameter size of the layer after editing may not match the size of the trained parameters. Check that the size of the loaded trained parameters (parameter size indicated by the .txt file specified by the W.File property in the case of Affine) matches the layer parameter size (input size × output size in the case of Affine). If they do not match, the trained parameters cannot be used, so set the property that specifies the trained parameter to blank.

An error appears in the log output, but I don’t understand the meaning.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

If the error is caused by a dataset, you may be able to determine the error by selecting Check Consistency on the shortcut menu on the Dataset tab. For details, see ":ref:`Checking the consistency of datasets <#checking-the-consistency-of-a-dataset>`”.

During evaluation execution
---------------------------

Can test data other than training data or validation data be specified?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

You can specify separately the evaluation data for training and the test data to use when you click the Run Evaluation button by adding a new dataset from the Dataset tab, loading test data, and specifying the test dataset added to the dataset under Executor (settings for when the Run Evaluation button is clicked) on the Config tab.

When evaluation is executed, it appears that evaluation calculation is being performed to the end, but nothing appears on the Evaluation tab.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The I/O pattern may not be supported by the current evaluation function. Right-click the training result list to open a shortcut menu, click Open Result Location, and open the folder containing the training result file. By opening output_result.csv, you can directly view the evaluation results.

In Confusion Matrix, all the data are classified to a certain cell.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

It may be that optimization is not being performed properly to begin with, and the obtained results may be the actual values. View the learning curve, and check that the loss and error values are steadily decreasing as epochs are repeated. Consider expanding the number of data samples used in training or changing the network structure.

Other FAQs
----------

The display driver stops responding, and the GUI display becomes crooked.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

It has been found that when a memory shortage occurs on the GPU or an old display driver is in use, the display driver stops responding. In the former case, restart |Console|. In the latter case, update the display driver to the latest version.

Loading of trained parameters cannot be performed correctly.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

When |Console| or the project file is placed under a long path name, when a parameter file or C code is being generated, the 258 character limit for the path length may be exceeded. Move |Console| or the project file to a higher folder level.

