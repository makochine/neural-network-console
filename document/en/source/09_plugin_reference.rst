Plugin reference
================

This chapter describes the plugins and their properties that can be used with the |Console|.

Pre-Processing Plugins
----------------------

To run the pre-processing plugin for creating a dataset, click the Create Dataset button at the top left of the dataset management screen and select the name of the plugin from the Create Dataset of the pop-up menu.

.. toctree::
   :maxdepth: 1

   Plugin/Pre_Process/Create_Dataset/ObjectDetection.rst
   Plugin/Pre_Process/Create_Dataset/SimpleTextClassification.rst
   Plugin/Pre_Process/Create_Dataset/StringClassification.rst
   Plugin/Pre_Process/Create_Dataset/SimpleTabularDataset.rst
   Plugin/Pre_Process/Create_Dataset/SplitImage.rst
   Plugin/Pre_Process/Create_Dataset/SplitWav.rst


To download open dataset or generate artificial data, click the Create Dataset button at the top left of the dataset management screen and select the name of the dataset from the Download or Generate Dataset from the pop-up menu.


Post-processing Plugins
-----------------------

To execute various post-processing plugins, right-click the evaluation result on the Evaluation tab to open a shortcut menu and select the plugin.

.. toctree::
   :maxdepth: 1

   Plugin/Post_Process/Utils/CrossTabulation.rst
   Plugin/Post_Process/Utils/CSVtoWAV.rst
   Plugin/Post_Process/Utils/Inference.rst
   Plugin/Post_Process/Utils/ParameterStats.rst
   Plugin/Post_Process/Utils/RestoreSplitImageWav.rst
   Plugin/Post_Process/Utils/SimilarWords.rst
   Plugin/Post_Process/Utils/SimpleTextGeneration.rst
   Plugin/Post_Process/Utils/tSNE.rst
   Plugin/Post_Process/Visualization/ScatterPlot.rst
   Plugin/Post_Process/Visualization/TileImages.rst
   Plugin/Post_Process/XAI/AttentionEditor.rst
   Plugin/Post_Process/XAI/FaceEvaluation.rst
   Plugin/Post_Process/XAI/GradCAM.rst
   Plugin/Post_Process/XAI/LIME.rst
   Plugin/Post_Process/XAI/SHAP.rst
   Plugin/Post_Process/XAI/SGDInfluence.rst
   Plugin/Post_Process/XAI/InfluenceFunctions.rst
   Plugin/Post_Process/XAI/SmoothGrad.rst
   Plugin/Post_Process/XAI/TracIn.rst


Training Plugins
----------------


To run the training plugin, select the name of the plugin from the listbox at the left of the Run button at the top right of the EDIT tab and click the Run button.

Once the plug-in has finished training, the training result folder will contain the nnp file of the model trained by the plugin.

The model created by the training plug-in can be evaluated from the Training tab in the same way as a model created by normal training processes.



DARTS (image classification)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Search and train network architecture for image classification using DARTS, which is one method of neural architecture search.

By using DARTS, you can automatically search compact and high performance network architecture for the training dataset.


DARTS: Differentiable Architecture Search

Hanxiao Liu, Karen Simonyan, Yiming Yang

https://arxiv.org/abs/1806.09055


To run DARTS, open training and validation dataset composed of variable x with 32x32 RGB image and variable y with class label starting from 0 in advance in the DATASET tab.

Also, set the number of epoch and batch size in Global Config on the CONFIG tab, updater in Optimizer.

It is not necessary to design networks in the EDUT tab in advance to execute this plug-in. Leave the EDIT tab blank.


.. list-table::
   :widths: 30 70
   :class: longtable

   * - mode
     -
        specifiy the execution mode of DARTS
        
        Specify "both" to perform both architecture search and training, "search" to perform only search, or "train" to perform only training

   * - arch
     -
        specify an arch file that shows the searched architectures to be used for training if you specify "train" as mode.
        
        arch file is stored in the training results folder where you ran search

   * - train_portion
     -
        Specify the percentage of training data used to search the architecture
        
        0.5 to use 50% data

   * - epoch_search
     - specify the number of epochs to search the architecture

   * - init_channel_search
     - specify the number of channels in the first layer during architecture search

   * - cells_search
     - specifies the number of cells during architecture search

   * - init_channel_train
     - Specify the number of channels in the first layer during training

   * - cells_train
     - specify the number of cells during training

   * - auxiliary_train
     - specify whether to use auxiliary loss during training

   * - drop_path_train
     -
        specify the percentage of drop path during training
        
        0.1 to drop 10% of the time

   * - print_frequency
     - specify how often to update the progress of the process in iterations


**Reference**

If training fails due to insufficient memory, set a smaller batch size in the Global Config in the CONFIG tab.

If you do not want to reduce the actual batch size, specify a larger value to the update interval in the Optimizer in the CONFIG tab.

For example, if the batch size is 64 and the update interval is 4, the actual batch size is 256, which is 64 * 4.


.. include:: 09_training_plugin.rst


Interactive Demo Plugins
------------------------


Through interactive demonstrations, you can experience a variety of Deep Learning technologies enabled by the Neural Network Libraries.

To run various interactive plugins, click Neural Network Libraries - Examples from the list on the left side of the screen after launch and click on the demo you want to run from the list.


D3Net Music Source Separation
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


The waveform input by mp3, wav file, etc., is output separately into four parts: bass, drums, vocals, and others.

The following D3Net is used for sound source separation.


Densely connected multidilated convolutional networks for dense prediction tasks

Naoya Takahashi, Yuki Mitsufuji

https://arxiv.org/abs/2011.11844



This plug-in works only with NVIDIA GPUs.


.. list-table::
   :widths: 30 70
   :class: longtable

   * - input
     - specify the waveform file for source separation

   * - output
     -
        specify the file name of the CSV file to output the sound source separation result
        
        each waveform file of the sound source separation result is saved in a folder with the same name as the newly created CSV file.

        The file name of each waveform file of the sound source separation result is output to the CSV file.


**Reference**

If the input waveform file is long, the process may take a very long time to complete.


X-UMX Music Source Separation
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


The waveform input by mp3, wav file, etc., is output separately into four parts: bass, drums, vocals, and others.

The following X-UMX is used for sound source separation.


All for One and One for All: Improving Music Separation by Bridging Networks

Ryosuke Sawata, Stefan Uhlich, Shusuke Takahashi, Yuki Mitsufuji

https://arxiv.org/abs/2010.04228


This plug-in works only with NVIDIA GPUs.


.. list-table::
   :widths: 30 70
   :class: longtable

   * - input
     - specify the waveform file for source separation

   * - output
     -
        specify the file name of the CSV file to output the sound source separation result
        
        each waveform file of the sound source separation result is saved in a folder with the same name as the newly created CSV file.

        The file name of each waveform file of the sound source separation result is output to the CSV file.


**Reference**

If the input waveform file is long, the process may take a very long time to complete.


SLE-GAN
~~~~~~~


Generate general image using the following Stabilized GAN.


Towards Faster and Stabilized GAN Training for High-fidelity Few-shot Image Synthesis

Bingchen Liu, Yizhe Zhu, Kunpeng Song, Ahmed Elgammal

https://arxiv.org/abs/2101.04775


This plug-in works only with NVIDIA GPUs.


.. list-table::
   :widths: 30 70
   :class: longtable

   * - model
     - specify the model to use for image generation

   * - output
     - specify the file name of the output image file


StyleGAN2
~~~~~~~~~


Generate face images by using following StyleGAN2.


Analyzing and Improving the Image Quality of StyleGAN

Tero Karras, Samuli Laine, Miika Aittala, Janne Hellsten, Jaakko Lehtinen, Timo Aila

https://arxiv.org/abs/1912.04958


This plug-in works only with NVIDIA GPUs.


.. list-table::
   :widths: 30 70
   :class: longtable

   * - latent_seed
     -
        specify the random seed used to generate the noise input z

        this drastically changes the result

   * - truncation_phi
     - specify the value for truncation trick

   * - noise_seed
     -
        specify random seed used for stochasticity input

        this slightly changes the result


   * - batch_size
     - specify the number of images to generate at one time


   * - output
     -
        specify the file name of the CSV file to output the image generation result
        
        each image file of the image generation result is saved in a folder with the same name as the newly created CSV file.

        The file name of each generated image file is output to the CSV file.


StyleGAN2 (Style Mixing)
~~~~~~~~~~~~~~~~~~~~~~~~


Generate 2 types of face images A and B by using  StyleGAN2 below, and generate a mix of the rough style of A's face image and the fine style of B's face image.


Analyzing and Improving the Image Quality of StyleGAN

Tero Karras, Samuli Laine, Miika Aittala, Janne Hellsten, Jaakko Lehtinen, Timo Aila

https://arxiv.org/abs/1912.04958


This plug-in works only with NVIDIA GPUs.


.. list-table::
   :widths: 30 70
   :class: longtable

   * - latent_seed
     -
        specify the random seed used to generate the noise input z for face image A

        this drastically changes the result

   * - latent_seed2
     -
        specify the random seed used to generate the noise input z for face image B

        this drastically changes the result

   * - mix_after
     - specify from which layer to use the secondary latent code w2 in face image B with a value of 0 ~ 17

   * - noise_seed
     -
        specify random seed used for stochasticity input

        this slightly changes the result

   * - truncation_phi
     - specify the value for truncation trick

   * - batch_size_a
     - specify the number of images A to generate

   * - batch_size_b
     - specify the number of images B to generate

   * - output
     -
        specify the image file name to output the image mix result
        
        the first column of the output image is image A, the first row is image B, and other columns are the result of mixing the styles of the face images in the first cols and rows.


DeepLabv3plus
~~~~~~~~~~~~~


Do semantic segmentation by using following DeepLab v3+.


Encoder-Decoder with Atrous Separable Convolution for Semantic Image Segmentation

Liang-Chieh Chen, Yukun Zhu, George Papandreou, Florian Schroff, Hartwig Adam

https://arxiv.org/abs/1802.02611v3


This plug-in works only with NVIDIA GPUs.


.. list-table::
   :widths: 30 70
   :class: longtable

   * - input
     - specify an image file for semantic segmentation

   * - target_w
     - specify the width of the image when semantic segmentation processing is performed

   * - target_w
     - specify the height of the image when semantic segmentation processing is performed

   * - dataset
     -
        specify the dataset used to train the model for processing

        voc : Pascal VOC 2012 only
        voc-coco : Pascal VOC 2012 and COCO

   * - output_stride
     - select an Atlas convolution output stride from 8 or 16 for the model for processing

   * - output
     - specify the file name of the image file to output the semantic segmentation results



Face Alignment Network
~~~~~~~~~~~~~~~~~~~~~~


Detect facial landmark by using the following Face Alignment Network


How far are we from solving the 2D & 3D Face Alignment problem? (and a dataset of 230,000 3D facial landmarks)

Adrian Bulat, Georgios Tzimiropoulos

https://arxiv.org/abs/1703.07332


This plug-in works only with NVIDIA GPUs.


.. list-table::
   :widths: 30 70
   :class: longtable

   * - input
     - specify an image file containing face for facial landmark detection

   * - landmarks_type
     - specify the landmarks used to train the facial lamdmark detection model from 2D or 3D

   * - output
     - specify the file name of the image file to output the facial landmark detection results


Center Net
~~~~~~~~~~

Detect objects in the image by using following CenterNet.


Objects as Points

Xingyi Zhou, Dequan Wang, Philipp Krahenbuhl

https://arxiv.org/abs/1904.07850


This plug-in works only with NVIDIA GPUs.


.. list-table::
   :widths: 30 70
   :class: longtable

   * - input
     - specify an image file for object detection

   * - architecture
     - specify the architecture of the model used for object detection from ResNet or dlav0

   * - dataset
     -
        specify the dataset used to traing the model for processing

        coco : COCO

        pascal : Pascal VOC 2012

   * - output
     - specify the file name of the image file to output the object detection result


ESRGAN
~~~~~~

Perform super-resolution (4x each in height and width) by using following ESRGAN.


ESRGAN: Enhanced Super-Resolution Generative Adversarial Networks

Xintao Wang, Ke Yu, Shixiang Wu, Jinjin Gu, Yihao Liu, Chao Dong, Chen Change Loy, Yu Qiao, Xiaoou Tang

https://arxiv.org/abs/1809.00219


This plug-in works only with NVIDIA GPUs.


.. list-table::
   :widths: 30 70
   :class: longtable

   * - input
     - specify an image file for super-resolution

   * - output
     - specify the file name of the image file to output the super-resolution result


AI Solutions Plugin
---------------------

With the AI Solutions Plugin, you can easily try out AI-related technologies provided by various companies.

To run AI Solutions plugins, select AI Solutions from the list on the left side of the screen after starting up and click on the plug-in you want to execute from the list.


ChatGPT using the OpenAI API
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This is a clone of ChatGPT using the OpenAI API.

https://platform.openai.com/

**Note**

Please determine whether the use of the API provided by OpenAI is permissible for your organization.

**Reference**

This plugin uses /v1/chat/completions as an endpoint for the API.


Setup
^^^^^

To use this plugin, you need to issue and configure an OpenAI API Key in advance. 

1. Create an OpenAI API key from the following Open AI website:

https://platform.openai.com/account/api-keys

2. Click the Setup button on the top right of the window to open the Setup window.

3. In the ENGINE tab of the Setup window, enter the created API key in Environment Variable, at the end of the line that says OPENAI_API_KEY_NNC=.

4. Click the Apply button on the lower right of the Setup window, then close the Setup window.


Launching Chatbot using OpenAI API
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Select AI Solutions from the left side list on the screen after starting, then select Chatbot using OpenAI API from the list.

Command prompt and a tab for Chatbot using ChatGPT API will launch in the web browser.


Terminating Chatbot using OpenAI API
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Close the command prompt and Chatbot using ChatGPT API web browser tab that were launched during startup.


Changing the model used for chat or fine-tuning
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Select the model to be used in the chat from the list box in the upper right corner of the tab.

For details about the models, please refer to the OpenAI website.

https://platform.openai.com/docs/models/

If there are already fine-tuned models that have been created, you will also be able to select those models from here.


To chat
^^^^^^^

1. If you want to use the system prompt, enter the system prompt in the text box labeled "Type system message here."

If you are not using the system prompt, this step is not necessary. 

**Note**

By using the system prompt, you can specify the role and behavior of the chatbot in advance.

2. Enter the text in the text box labeled "Type your message here." 

3. Click "Send Message" button at the bottom right of the text box to send the entered text. 

Or, press Ctrl+Enter on your keyboard to send the text. 

After sending the message, the chatbot's response will be displayed after a short wait. To continue the chat, enter more text in the text box and send it.


Starting a new chat
^^^^^^^^^^^^^^^^^^^

Click the "+ New Chat" button in the upper left corner of the tab.


Loading the log of a chat previously held
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

From the list of past chat histories displayed on the left side of the tabl, click on the log to be loaded.


Opening the folder where the log of a chat previously held is saved
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Click the "Open log" button in the upper left corner of the tab.

The folder where the log is saved will be opened with Explorer.


Saving frequently used system prompts
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

1. Enter the system prompt or click the text box to select it.

2. Click the save button located in the lower right corner of the system prompt.


Recalling previously saved system prompts
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Click the list box above the system prompt text box, then select the system prompt to be loaded.


Editing previously saved system prompts
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

1. Click the list box above the system prompt text box, then select "[Edit a saved prompt list]". The saved text file of the system prompt will be opened in Notepad.

2. Edit and save the system prompt.

Preparing a Dataset for Fine-Tuning
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

To perform fine-tuning of ChatGPT, prepare a dataset file in jsonl file format, where one line of text is in JSON format, as shown below.

"content" whose "role" is "system" indicates the system prompt, "content" whose "role" is "user" indicates the user's input, and "content" whose "role" is "assistant" indicates the AI response.

    {"messages": [{"role": "system", "content": "You are a greeting AI. Please respond to the user's input with a greeting."}, {"role": "user", "content": "Good morning"}, {"role": "assistant", "content": "Good morning"}]}

    {"messages": [{"role": "system", "content": "You are a greeting AI. Please respond to the user's input with a greeting."}, {"role": "user", "content": "Hello"}, {"role": "assistant", "content": "Hello"}]}

    {"messages": [{"role": "system", "content": "You are a greeting AI. Please respond to the user's input with a greeting."}, {"role": "user", "content": "Good evening"}, {"role": "assistant", "content": "Good evening"}]}

    …

You can also prepare input text as a CSV file with the first row as "role" and the followning rows as text, and then convert that into a jsonl format.

    system,user,assistant

    You are a greeting AI. Please respond to the user's input with a greeting.,Good morning,Good morning

    You are a greeting AI. Please respond to the user's input with a greeting.,Hello,Hello

    You are a greeting AI. Please respond to the user's input with a greeting.,Good evening,Good evening

    …

To convert the prepared CSV file to a jsonl format

1. Click the Files button on the screen to display the Files tab.

2. Click the "+ Upload New Dataset" button at the top left of the screen.

3. Select the prepared CSV file from the "Select file" button.

4. Click the Convert button. This will start the download of the converted jsonl file.

**Note**

In preparation for a dataset that includes multi-byte characters, it is necessary to encode the prepared jsonl and CSV files in UTF-8.

At least 10 examples must be provided to run the fine-tuning.

**Reference**

By continuing the role with user and assistant, you can create training data that includes longer interaction sequences.

When using a CSV file, variable length dialogues can be given by leaving the cell blank after the dialogue ends.


Uploading Dataset for Fine-Tuning
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

1. Click the Files button on the screen to display the Files tab.

2. Click the "+ Upload New Dataset" button at the top left of the screen.

3. Select the prepared jsonl file from the Select file button.

4. Click the Upload button.


Checking the Uploaded Datasets
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

1. Click the Files button on the screen to display the Files tab.

2. From the list on the left side of the screen, select the uploaded dataset.

The status of the dataset immediately after upload is "uploaded".

The status changes to "processed" after a while and fine-tuning can be executed.


Running Fine-Tuning With Uploaded Datasets
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

1. Click the Files button on the screen to display the Files tab.

2. From the list on the left side of the screen, select the dataset to be used for fine-tuning.

3. From the drop-down box in the upper right corner of the screen, select the model to be used for fine-tuning.

4. Click the "Run fine tuning using this data" button.

5. Click the OK button in the dialog box confirming the creation of the fine-tuning job.


Deleting the Uploaded Datasets
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

1. Click the Files button on the screen to display the Files tab.

2. From the list on the left side of the screen, select the dataset to be deleted.

3. Click the Delete button.

4. Click the OK button in the dialog box asking for confirmation of the deletion.


Checking the Running Fine-Tuning Jobs
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

1. Click the "Fine Tuning Job" button on the screen to display the "Fine Tuning Job" tab.

2. From the list on the left side of the screen, select the job to be checked.

The status of the job immediately after the start of Fine Tuning is "created", changes to "running" when fine-tuning starts, and becomes "succeeded" when fine-tuning completes.

The fine-tuned model can be selected from the drop-down box in the upper right corner of the screen and used for chat or further fine-tuning.


Cancelling the Running Fine-Tuning Jobs
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

1. Click the "Fine Tuning Job" button on the screen to display the "Fine Tuning Job" tab.

2. From the list on the left side of the screen, select the job to be cancelled.

3. Click the Cancel button.

The status of the cancelled job changes to "cancelled".

**Reference**

Due to OpenAI's specifications, it is not possible to delete the history of jobs.
