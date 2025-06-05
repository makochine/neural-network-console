Other information
=================

Trademarks
----------

Microsoft and Windows are registered trademarks or trademarks of Microsoft Corporation in the United States and/or other countries.

NVIDIA and CUDA are registered trademarks or trademarks of NVIDIA Corporation in the United States and/or other countries.

Other system names and product names appearing in this document are either registered trademarks or trademarks of their respective development manufacturers. Note that ™ and ® are not indicated in the text.

Licensing information
---------------------

This software contains software that Sony uses under a license agreement granted by its copyright holder. According to the requirement by the copyright holder of the software, Sony is obligated to notify the customer of this information. Please read the information regarding the license provided in settings\\about.txt.

Version update history
----------------------

- 2017/08/17 Version 1.00

    - First release

- 2017/11/15 Version 1.10

    - Addition of the AbsoluteError and EpsilonInsensitiveLoss layers
    - Addition of the Create Training Result function to the right-click menu of the Run Training button
    - Bug fix

- 2018/06/15 Version 1.20

    - Addition of the Unit function (addition of the Unit, Argument, and StructureSearch layers)
    - Addition of the LeakyReLU, SELU, Swish, MatrixDiag, MatrixDiagPart, FixedPointQuantize, Pow2Quantize, and DepthwiseConvolution layers
    - Addition of the nntxt import function (beta)
    - Addition of the pptx report function
    - Addition of a search function using the Components string
    - Improvement to the training speed by changing the cache file format
    - Improvement to the compatibility of the caffe prototxt import/export function
    - Improvement to the ArrangeLayer function
    - Bug fix

- 2018/12/06 Version 1.30

    - Support for Mixed Precision training
    - Support for exporting training results to nnp, nnb, and ONNX formats
    - Support for importing nnp and ONNX
    - Addition of a dataset upload function to the Cloud version
    - Addition of the DepthwiseDeconvoluion, GlobalAveragePooling, BroadcastTo, ClipGradByNorm, ClipGradByValue, Round, Ceil, Floor, Sin, Cos, Tan, Sinh, Cosh, ASin, ACos, ATan, ASinh, ACosh, ATanh, Pad, TopKData, TopKGrad, FFT, and IFFT layers
    - Addition of the Learning Rate Scheduler (Poly, Exp, Step, Cosine)
    - Addition of the AMSGRAD Optimizer
    - Addition of the Command Prompt function
    - Improvement to the Unit function
    - Bug fix

- 2019/02/19 Version 1.40

    - Addition of the error (error classification) analysis function and inference result sort function
    - Addition of the html report function
    - Addition of the Range and Warmup settings to the Optimizer
    - Addition of the Interpolate, Sort, Prune, and NmsDetection2D layers
    - Improvement to operability
    - Bug fix

- 2019/07/30 Version 1.50

    - Addition of the Cross Tabulation, Grad-CAM, Tile Images, and ParameterStats (calculates parameter statistics) plugins
    - Addition of the GELU, ReLU6, HardSigmoid, HardTanh, LogSigmoid, SoftPlus, SoftSign, TanhShrink, Sinc, Tile, Atan2, and FusedBatchNormalization layers
    - Addition of the ChannelLast property to Convolution, MaxPooling and AveragePooling
    - Addition of the AdaBound and AMSBound OptimizersAdaBound, AMSBound Optimizer
    - Improvement to operability
    - Bug fix

- 2019/12/03 Version 1.60

    - Addition of the LIME and Inference plugins
    - Support for execution of training on the |Console| Cloud version (beta)
    - Addition of a function for exporting to TensorFlow format (*.pb) (beta)
    - Improvement to operability
    - Bug fix

- 2020/03/04 Version 1.70

    - Bug fix


- 2020/07/30 Version 1.80

    - Addition of the automatic evaluation execution function
    - Addition of the layer search and replacement functions
    - Addition of the duplication function by Ctrl+D and Alt+drag
    - Support for wav file input in datasets
    - Enhancement to enable the selection of a display type for the CSV and wav files included in a dataset out of four options
    - Enhancement to enable checking training settings in the overview
    - Addition of the comment function to jobs
    - Addition of the AddN, MulN, BatchInv, IsInf, IsNaN, ResetInf, ResetNaN, and TestNetwork layers
    - Addition of the AdamW, SgdW, and Lars Optimizer
    - Addition of the Scatter plot and t-SNE plugins
    - Improvement to the visibility in properties and datasets
    - Bug fix


- 2020/10/12 Version 1.90

    - Support multi-node training function and tenant function when executing neural network training on the |Console| Cloud version (beta)
    - Addition of the Clear all parameter files function
    - Addition of the preview playback function for audio file
    - Addition of the Grad-CAM(batch) plugin
    - Addition of the Mish layer
    - Bug fix


- 2020/12.21 Version 2.00

    - Supports display in Japanese
    - Addition of the custom unit function
    - Addition of the import as unit function
    - Addition of the SGD Influence, Face evaluation, LIME(batch) and LIME(tabular) plugin
    - Supports activation using tokens
    - Enhancement to set a default display type for the CSV and wav files included in a dataset
    - Bug fix


- 2021/05/13 Version 2.10

    - Addition of various pre-processing plugins to facilitate dataset creation
    - Addition of export function in TensorFlow 2.0 saved_model format and tflite format (beta)
    - Addition of function to resume suspended training
    - Reduce GPU memory required for training
    - Addition of Out of Core function to train a large model that exceeds the size of GPU memory
    - Supports multiple inputs for Unit
    - Additon of the InstanceNormalization, GroupNormalization, LayerNormalization, Norm, NornNormalization, WeightStandardization layers
    - Addition of AdaBelief optimizer
    - Addition of SHAP, SHAP(batch) and TracIn plugins
    - Addition of adult, waveform classification, binary segmentation and natural language processing sample projects
    - Plugin hierarchy and UI improvements
    - Supports UTF-8 encoding for dataset CSV files
    - Bug fix


- 2021/07/01 Version 2.20

    - Addition of pre-processing and post-processing plug-ins to support image and audio signal processing.
    - Addition of image super-resolution and audio noise reduction sample project and sample dataset
    - Addition of SGD-Influence (tabular), SmoothGrad, SmoothGrad (batch), LIME (tabular batch) plugins
    - Bug fix


- 2021/10/25 Version 2.30

    - Addition of CycleGAN sample project
    - Addition of SHAP(tabular) and Influenehce functions (image) plugins
    - Bug fix


- 2022/02/01 Version 2.40

    - Addition of the import function
    - Addition of XAI Visualization (GradCAM, SHAP, LIME SmoothGrad)
    - Support default argument for the Unit
    - Addition of AdaptiveSeparableConvolution, BatchDet, BatchLogDet, CumSum, CumProd, DeformableConvolution, LogSoftmax, WeightNormalization, SpectralNorm, RandomErase, MinMaxQuantization layers
    - Addition of German Credit sample project and sample dataset
    - Addition of CenterNet and RepMLP simple sample project
    - Supports training plugin and dataset download & generation plugin function
    - Improvement of SGD Influence (image)
    - Plugin UI improvements
    - Bug fix

- 2022/06/01 Version 2.50

    - Addition of CPU / GPU usage rate display function
    - Addition of the function to display values of specific properties at once
    - Addition of MLP-Mixer simple sample project
    - Addition of PaDiM and Transfer Learning for image classification plugins
    - Supports enlarged display of the network where the mouse is located during reduced display
    - Addition of Feature Visualization (batch) plugin
    - Addition of Representer Point Selection plugin
    - Addition of Attention Branch Network sample project
    - Addition of Attention Map plugin
    - Addition of Attention Editor
    - Addition of SGD influence (tabular) method of the mean value by calculation multiple times
    - Bug fix


- 2022/11/01 Version 2.60

    - Addition of interactive demo plugin and dataset plugin features
    - Addition of D3Net, X-UMX, SLE-GAN, StyleGAN2, StyleGAN2 (Style Mixing), DeepLabv3+, Face Alignment Network, CenterNet and ESRGAN interactive demo plugins
    - Addition of recently used post-processing plug-in functionality and improvement of plug-in UI
    - Addition of fairness evaluation plugins
    - Addition of bias mitigation methods (Reweighing, Prejudice Remover Regularizer)
    - Bug fix


- 2023/03/20 Version 2.70

    - Addition of Reweighing Weight plugin in dataset fairness plugins
    - Addition of Reject Option-Based Classification post-processing bias mitigation algorithm
    - Addition of ROC-AUC Curve & Precision-Recall Curve post-processing visualization plugins
    - Bug fix


- 2024/01/31 Version 3.10

    - Addition of inference feature using pre-trained models (Function to call the Transformers pipeline)
    - Addition of feature to create simple datasets from text or folders
    - Addition of data preview feature
    - Addition of clone UI of ChatGPT using OpenAI GhatGPT API
    - Bug fix


- 2024/12/23 Version 3.30

    - Addition of torch support
    - Addition of support for inference execution using pre-trained models (Transformers Pipeline invocation feature) in graph execution
    - Addition of Constant, question-answering, translation, visual-question-answering, Identity, Concatenate, Extract, Video Classification and Zeroshot Object Detection layers (Transformers)
    - Added fine-tuning feature to the cloned UI of ChatGPT using OpenAI GPT API
    - Addition of training plugins
    - Removal of cloud functionality
    - Bug fix


- 2025/01/10 Version 3.31

    - Bug fix where training could not be performed using GPU

