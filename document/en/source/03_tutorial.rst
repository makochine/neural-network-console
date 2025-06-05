.. _#tutorial:

Tutorial
========

.. _#training-using-a-sample-project:

Training using a sample project
-------------------------------

Starting |Console|
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Double-click neural_network_console.exe to run it.

Opening the sample project
~~~~~~~~~~~~~~~~~~~~~~~~~~

From the list of projects that appears after the application starts, select 01_logistic_regression.sdcproj and open it.

.. image:: figures/image7.png
   :align: center

**Reference**

The MNIST dataset used in the sample project is downloaded from the following website, and the dataset file is prepared when the sample project is opened for the first time. Typically, it takes a few minutes for dataset preparation to complete.

http://yann.lecun.com/exdb/mnist/

If downloading of the dataset fails in an environment where a proxy is in use, on the ENGINE tab of the setup window, add the proxy setting in the exiting environment variables.

HTTP_PROXY=http://(HTTP proxy server address):(port number)

HTTPS_PROXY=https://(HTTPS proxy server address):(port number)

Viewing the contents of the opened sample project
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

On the EDIT tab, check that a single-layer network (logistic regression) has been loaded for performing binary classification, which determines whether each of the 1 by 28 by 28 monochrome handwritten character images (images of 4 and 9) from the MNIST handwritten character classification dataset is a 9.

.. image:: figures/image8.png
   :align: center

On the Dataset tab, check that small MNIST training and validation datasets (images of 4 and 9 and labels indicating whether each image is a 9 where “4” is category index 0 and “9” is category index 1) extracted from the full MNIST dataset have been loaded.

.. image:: figures/image9.png
   :align: center

Executing training
~~~~~~~~~~~~~~~~~~

On the EDIT tab, click the Run Training button (shortcut: F5) to start the training process.

.. image:: figures/image10.png
   :align: center

The TRAINING tab shows the progress of training and the learning curve.

.. image:: figures/image11.png
   :align: center

*Epoch*, the horizontal axis of the graph, indicates the number of repeated generations (epochs) of optimization. *Cost* indicates the output of the loss function at the optimization stage, and *errors* indicate the outputs of the loss functions of the training data and validation data at the end of each epoch. As shown in the figure, if the cost and error values decrease as the number of epochs increases, optimization can be considered to be running properly.

Executing evaluation
~~~~~~~~~~~~~~~~~~~~

On the TRAINING tab, click the Run Evaluation button (shortcut: F6) to start the evaluation process.

.. image:: figures/image12.png
   :align: center
  
The EVALUATION tab shows the progress of the evaluation and the evaluation results.

.. image:: figures/image13.png
   :align: center

The evaluation results are shown in a table consisting of the validation dataset and an additional column (y’) containing the estimated results produced by the trained neural network. Column y’ in 01_logistic_regression.sdcproj indicates the probability of y being a 1 (the input image being a 9).

Click Confusion Matrix to show the accuracy evaluation result.

.. image:: figures/image14.png
   :align: center

**Accuracy** indicates the classification accuracy. In this example, we can see that 4s and 9s in the validation dataset were classified with 95.2% accuracy.

Designing and training a two-layer neural network
-------------------------------------------------

Creating a new project
~~~~~~~~~~~~~~~~~~~~~~

Start |Console|, and click + New Project to create a new project.

.. image:: figures/image15.png
   :align: center
  
Assigning the datasets
~~~~~~~~~~~~~~~~~~~~~~

On the DATASET tab, click the dataset assignment button (immediately below the dataset tab button in the screen shot), and assign small_mnist_4or9_training.csv to the training dataset.

.. image:: figures/image16.png
   :align: center
  
Click Validation under Dataset, and select the validation dataset. Like the training dataset, select small_mnist_4or9_test.csv for the validation dataset.

.. image:: figures/image17.png
   :align: center
  
Designing the two-layer neural network
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

1. Under Components at the left on the EDIT tab, double-click, Input, Affine, Tanh, Affine, Sigmoid, and BinaryCrossEntropy one by one to add layers to the network graph.

.. image:: figures/image18.png
   :align: center

**Reference**

Affine is in the Basic category (fourth from the top), Tanh and Sigmoid is in the Activation category (sixth from the top), and BinaryCrossEntropy is in the Loss category (second from the top).

2. Click Input layer to select it. Input “1,28,28” (the default value), which is the size of variable *x* (input data), in the Size box (click and input) under Layer Property.

.. image:: figures/image19.png
   :align: center

3. Click the first Affine layer to select it. Set the number of intermediate layer nodes (e.g., 100) (the default value) in the OutShape box under Layer Property.

.. image:: figures/image20.png
   :align: center

4. Click the second Affine layer to select it. Set the number of output layer nodes (1, which is the size of variable *y* (output data) in this example) in the OutShape box under Layer Property.

.. image:: figures/image21.png
   :align: center

.. _executing-training-1:

Executing training
~~~~~~~~~~~~~~~~~~

Click the Run Training button (shortcut: F5). If the project is not saved, a message prompting you to save appears. When you enter a file name and save the project, training will start.

.. image:: figures/image22.png
   :align: center

