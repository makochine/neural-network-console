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
