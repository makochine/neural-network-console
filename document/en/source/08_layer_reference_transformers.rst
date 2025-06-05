Layer Reference (Transformers)
==============================

This chapter describes the layers and their properties that can be used in the Transformers project with the |Console|.

IO Layers
---------

InputFileName
~~~~~~~~~~~~~

This layer is used to input a string that represents the file name in the dataset CSV file.

It is used when inputting image or audio file names into the graph.

.. list-table::
   :widths: 30 70
   :class: longtable

   * - Dataset
     - Specify the name of the variable to be input into this InputFileName layer among the variables included in the dataset.

   * - ArgName
     -
        Specity the argument name when the subsequent function takes this variable as an input argument.

        If you do not set an argument name, leave it blank.

InputString
~~~~~~~~~~~

This layer is used to input a string that is written in a cell of the dataset CSV file.

It is used when inputting text data into the graph.

.. list-table::
   :widths: 30 70
   :class: longtable

   * - Dataset
     - Specify the variable name of the variable to be input into this InputString layer among the variables included in the dataset.

   * - ArgName
     -
        Specity the argument name when the subsequent function takes this variable as an input argument.

        If you do not set an argument name, leave it blank.

Constant
~~~~~~~~

This is a layer used to input a constant string or value.

It is used when inputting a constant that is independent of the dataset into the graph.


.. list-table::
   :widths: 30 70
   :class: longtable

   * - Value
     - Specify the value of the constant.

   * - ValueType
     - Specify the type of the constant as Text (string), Int (integer), or Float (floating-point number).

   * - ArgName
     -
        Specity the argument name when the subsequent function takes this constant as an input argument.

        If you do not set an argument name, leave it blank.

Pipeline Layers
---------------

These are layers for performing inference using pretrained models from HuggingFace.

.. list-table::
   :widths: 30 70
   :class: longtable

   * - audio-classification
     - 
        Classifies audio files specified by the file name in the input string.
        
        Outputs the classified result as a "label" string and the probability of the label as a "score".

   * - automatic-speech-recognition
     - 
        Performs speech recognition on the audio file specified by the file name in the input string.
        
        Outputs the recognized speech as a "text" string.

   * - depth-estimation
     - 
        Performs depth estimation on the image file specified by the file name in the input string.
        
        Outputs the estimated depth matrix as a "depth", and the visualization of the estimated depth as a "predicted_depth".

   * - feature_extraction
     - 
        Performs feature extraction on files such as images specified by the file name in the input string.
        
        Outputs the extracted features as a "tensor".

   * - fill_mask
     - 
        Predicts the word for the "<mask>" placeholder in the input string.
        
        Outputs the token index of the predicted word as a "token", the predicted word as a "token_str", and the input string with the <mask> replaced by the predicted word as a "sequence".

   * - image_classification
     - 
        Classifies the image file specified by the file name in the input string.
        
        Outputs the classified result as a "label" string and the probability of the label as a "score".

   * - image_segmentation
     - 
        Performs semantic segmentation on the image file specified by the file name in the input string.
        
        Outputs the score of each label as a "\*_score", the label string as a "\*_label", and the binary image indicating the segmented regions as a "\*_mask".

   * - image_to_text
     - 
        Converts the image file specified by the file name in the input string into text (captioning).
        
        Outputs the generated text as a "0_generated_text".

   * - object_detection
     - 
        Performs object detection on the image file specified by the file name in the input string.
        
        Outputs the score of each label as a "\*_score", the label string of the detected object as a "\*_label", and the coordinates of the detected object's bounding box as "\*_box_xmin", "\*_box_ymin", "\*_box_xmax", "\*_box_ymax".

   * - question_answering
     - 
        For the input string with the argument name "context", it answers the question indicated by the input string with the argument name "question".

        Outputs the starting position of the answer and the ending position in the input text as "start" and "end", and the answer string as "answer".

   * - summarization
     - 
        Summarizes the input text string.
        
        Outputs the summarized text as a "0_summary_text".

   * - text2text-generation
     - 
        Generates text based on the input text string.
        
        Outputs the generated text as a "0_generated_text".

   * - text-classification
     - 
        Classifies the input text string.
        
        Outputs the classified result as a "label" string and the probability of the label as a "score".

   * - text-generation
     - 
        Generates the text following the input text string.
        
        Outputs the input text string with the predicted text added as a "0_generated_text".

   * - token-classification
     - 
        Classifies the input text string on a word-by-word basis.
        
        Outputs the classified result as a "\*_entity", the score as a "\*_score", the index of the word from the beginning of the sentence as a "\*_index", the word as a "\*_word", and the start and end positions of the word in the input sentence as "\*_start" and "\*_end".

   * - translation
     - 
        Translate the input string.

        Outputs the string that represents the translation result as a "\*_translation_text".

   * - visual_question_answering
     - 
        For an image file specified by the input string with the argument name "context", it answer the question indicated by the input string with the argument name "question".

        Outputs the score of the answer as "\*_score" and the string of the answer as "\*_answer".

   * - zeroshot-classification
     - 
        Classifies the input string according to the labels specified as text strings.
        
        Outputs the probability of each label as a "\*_score" and the classified result as a "\*_label".

   * - zeroshot-image-classification
     - 
        Classifies the image file specified by the file name in the input string according to the labels specified as text strings.
        
        Outputs the probability of each label as a "\*_score" and the classified result as a "\*_label".

   * - zeroshot-audio-classification
     - 
        Classifies the audio file specified by the file name in the input string according to the labels specified as text strings.
        
        Outputs the probability of each label as a "\*_score" and the classified result as a "\*_label".

Common Properties for Pipeline Layers
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. list-table::
   :widths: 30 70
   :class: longtable

   * - Name
     -
        Specify the layer name.
        
        The layer name must be unique within the graph.

   * - model
     - Specify the model name of the pretrained model from HuggingFace.

**Note** 

If you only want to test the inference, you can leave the model property blank. In this case, the default model for each task will be used for inference.

However, if no model is specified, the inference result will be undefined. Therefore, you need to input the model property when using it in actual deployment.

Common Properties for zeroshot Layers
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. list-table::
   :widths: 30 70
   :class: longtable

   * - candidate_labels
     -
        Specify the labels to be classified as a comma-separated list of text strings.
        
        For example, to classify images into "flower" and "food", specify "flower,food".

   * - hypothesis_template
     -
        Specify a template string to decorate the label strings specified in candidate_labels.
        
        Use {} to indicate the position to insert the label string.
        
        For example, with candidate_labels as "flower,food" and hypothesis_template as "A photo of {}", perform classification for "A photo of flower" and "A photo of food".


Other Layers
------------

Comment
~~~~~~~

This layer is used to insert comments into the network graph. The Comment layer does not affect training or evaluation.

.. list-table::
   :widths: 30 70

   * - Comment
     -
        Specify the comment text.
        
        The comment text will be displayed on the Comment layer.

Identity
~~~~~~~~

Outputs the input as it is. It is not usually necessary, but it can be used to assign a name for identification purposes at a specific location in the network or to output the value of a specific location in the network as an inference result.


Concatenate
~~~~~~~~~~~

Concatenates two or more inputs. This is used to combine multiple input data into one output.


Extract
~~~~~~~

Extracts a specific argument from an input with two or more arguments.

It is used to connect the outputs of a layer with multiple outputs and extract only specific arguments, or to rewrite argument names in order to input the argument names included in the output of input layer into another layer.

For example, by connecting the output of the image-to-text layer to the Extract layer, and specifying "0_generated_text" for Attribute and "context" for ArgName, you can convert the variable name "0_generated_text" from the image-to-text layer to "context".

.. list-table::
   :widths: 30 70

   * - Attribute
     - Specify the argument name to be extracted.

   * - ArgName
     -
        Specify the renamed argument name of the extracted argument.

        Leave it empty if you want to keep only the value without specifying an argument name.
