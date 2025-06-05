PaDiM
~~~~~

Train the image anomaly detection model by using PaDiM, one of the image anomaly detection methods.

PaDiM enables fast training of high-performance anomaly detection models using only normal images.


PaDiM: a Patch Distribution Modeling Framework for Anomaly Detection and Localization

Thomas Defard, Aleksandr Setkov, Angelique Loesch, Romaric Audigier

https://arxiv.org/abs/2011.08785


To run PaDiM, load a training and validation dataset that contains an RGB image with 224 x 224 pixel as variable x in the Datasets tab.

The training dataset should contain only normal images.

And set the batch size in Global Config in the CONFIG tab.

You do not need to design a network in the EDIT tab to run this plug-in (leave the network definition in the EDIT tab blank).


.. list-table::
   :widths: 30 70
   :class: longtable

   * - model
     - specify a pre-trained model of ImageNet to use for anomaly detection

   * - feature_ratio
     -
        specify the percentage of hidden features of the pre-trained model used for anomaly detection
        
        specify 0.5 to use 50% features

   * - gaussian
     -
        specify the grain size for which the Gaussian distribution is to be used in the Mahalanobis distance calculation
        
        dense
            calculate the Gaussian distribution for each pixel in Layer 1 (the highest resolution feature map). It works exactly like the PaDiM paper.

        1/16
            calculate the Gaussian distribution for each pixel in Layer 3 (the lowest resolution feature map). It saves a bit of model size compared to "dense".

        single
            only one Gaussian distribution is used throughout the image. It saves a lot of model size compared to "dense".


**Reference**

If training fails due to insufficient memory, set a smaller batch size in the Global Config in the CONFIG tab.


Transfer Learning (image classification)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Do transfer learning that can train high-psrformance image classification models at high spped by using pre-trained model from ImageNet.


To run transfer Learning, load a training and validation dataset with a square RGB image as variable x and a zero-based class label as variable y in the DATASET tab beforehand.

Set the number of training epoch and batch size in Global Config in the CONFIG tab, and set the optimization algorithm in Optimizer.

You do not need to design a network in the EDIT tab to run this plug-in (leave the network definition in the EDIT tab blank).


.. list-table::
   :widths: 30 70
   :class: longtable

   * - model
     - Specify a pre-trained model of ImageNet to use for transfer learning

   * - train_param
     -
        Specify the parameters to be trained
        
        all
            train the parameters of all layers (fine tuning)

        last_fc_only
            most parameters are fixed and only the parameters of the last affine layers of the image classification model are trained.

   * - max_zoom_ratio
     -
        Specify the maximum magnification for image augmentation processing during training
        
        Specify 1.0 if you do not perform image augmentation by image magnification

   * - max_aspect_ratio
     -
        Specify the maximum aspect ratio change rate in image augmentation processing during training
        
        Specify 1.0 when not performing image augmentation by changing the aspect ratio

   * - lr_flip
     -
        Specify whether to perform random horizontal flip in image augmentation processing during training
        
        Uncheck to turn off image augmentation by horizontal flip

   * - crop_ratio
     -
        Specify the ratio to crop the center of the input image during inference
        
        Specify 0.875 to use the center 224x224 pixels of a 256x256 pixel image during inference
        
        Specify 1.0 to turn off the crop in the center of the image


**Reference**

If training fails due to insufficient memory, set a smaller batch size in the Global Config in the CONFIG tab.

If you do not want to reduce the actual batch size, specify a larger value to the update interval in the Optimizer in the CONFIG tab.

For example, if the batch size is 64 and the update interval is 4, the actual batch size is 256, which is 64 * 4.


Transfer Learning (ViT)
~~~~~~~~~~~~~~~~~~~~~~~

Do transfer learning that can train high-psrformance image classification models at high spped by using ViT pre-trained model.


To run transfer Learning, load a training and validation dataset with a square RGB image as variable x and a zero-based class label as variable y in the DATASET tab beforehand.

Set the number of training epoch and batch size in Global Config in the CONFIG tab, and set the optimization algorithm in Optimizer.

You do not need to design a network in the EDIT tab to run this plug-in (leave the network definition in the EDIT tab blank).


An Image is Worth 16x16 Words: Transformers for Image Recognition at Scale

Alexey Dosovitskiy, Lucas Beyer, Alexander Kolesnikov, Dirk Weissenborn, Xiaohua Zhai, Thomas Unterthiner, Mostafa Dehghani, Matthias Minderer, Georg Heigold, Sylvain Gelly, Jakob Uszkoreit, Neil Houlsby

https://arxiv.org/abs/2010.11929


This plug-in works only with NVIDIA GPUs.


.. list-table::
   :widths: 30 70
   :class: longtable

   * - model
     - Specify a pre-trained model of ImageNet to use for transfer learning

   * - train_param
     -
        Specify the parameters to be trained
        
        all
            train the parameters of all layers (fine tuning)

        last_fc_only
            most parameters are fixed and only the parameters of the last affine layers of the image classification model are trained.

   * - max_zoom_ratio
     -
        Specify the maximum magnification for image augmentation processing during training
        
        Specify 1.0 if you do not perform image augmentation by image magnification

   * - max_aspect_ratio
     -
        Specify the maximum aspect ratio change rate in image augmentation processing during training
        
        Specify 1.0 when not performing image augmentation by changing the aspect ratio

   * - lr_flip
     -
        Specify whether to perform random horizontal flip in image augmentation processing during training
        
        Uncheck to turn off image augmentation by horizontal flip

   * - crop_ratio
     -
        Specify the ratio to crop the center of the input image during inference
        
        Specify 0.875 to use the center 224x224 pixels of a 256x256 pixel image during inference
        
        Specify 1.0 to turn off the crop in the center of the image


**Reference**

If training fails due to insufficient GPU memory, set a smaller batch size in the Global Config in the CONFIG tab.

If you do not want to reduce the actual batch size, specify a larger value to the update interval in the Optimizer in the CONFIG tab.

For example, if the batch size is 64 and the update interval is 4, the actual batch size is 256, which is 64 * 4.


Zero-shot image classification (CLIP)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Using the trained model of CLIP, create an image classification model based on the strings described for the image.


To run the plugin, load a validation dataset with a square RGB image as variable x and a zero-based class label as variable y in the DATASET tab beforehand.

This plugin does not use a training dataset, so a training dataset is not required.

Also, you do not need to design a network in the EDIT tab to run this plug-in (leave the network definition in the EDIT tab blank).


Learning Transferable Visual Models From Natural Language Supervision

Alec Radford, Jong Wook Kim, Chris Hallacy, Aditya Ramesh, Gabriel Goh, Sandhini Agarwal, Girish Sastry, Amanda Askell, Pamela Mishkin, Jack Clark, Gretchen Krueger, Ilya Sutskever

https://arxiv.org/abs/2103.00020


This plug-in works only with NVIDIA GPUs.

.. list-table::
   :widths: 30 70
   :class: longtable

   * - model
     - Specity a ViT pre-trained model to use for creating a classification model

   * - query_text
     -
        Specify the text for each class that describes the image to be classified, separated by commas
        
        For example, to classify a dog photo and a cat photo, specify "a dog photo,a cat photo"

   * - query_text_csv_file
     -
        Specify a CSV file in which the text for each class describing the image to be classified is written on each line
        
        query_text_csv_file is ignored if query_text is specified


PatchCore
~~~~~~~~~

Train the image anomaly detection model by using PatchCore, one of the image anomaly detection methods.

PatchCore enables creating high-performance anomaly detection models using only normal images.


Towards Total Recall in Industrial Anomaly Detection

Karsten Roth, Latha Pemula, Joaquin Zepeda, Bernhard Schölkopf, Thomas Brox, Peter Gehler

https://arxiv.org/abs/2106.08265


To run PatchCore, load a training and validation dataset that contains an RGB image with 224 x 224 pixel as variable x in the Datasets tab.

The training dataset should contain only normal images.

And set the batch size in Global Config in the CONFIG tab.

You do not need to design a network in the EDIT tab to run this plug-in (leave the network definition in the EDIT tab blank).


.. list-table::
   :widths: 30 70
   :class: longtable

   * - model
     - specify a pre-trained model of ImageNet to use for anomaly detection

   * - feature_ratio
     -
        specify the percentage of hidden features of the pre-trained model used for anomaly detection
        
        specify 0.5 to use 50% features

   * - sample_ratio
     -
        Specify the percentage of data points to retain among the hidden features calculated from all training data
        
        To use 1% features, specify 0.01.


**Reference**

It takes a long time to complete execution of PatchCore.


**Reference**

If training fails due to insufficient memory, set a smaller batch size in the Global Config in the CONFIG tab.
