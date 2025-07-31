その他
======

商標について
------------

Microsoft、Windowsは、米国Microsoft Corporationの米国およびその他の国における登録商標または商標です。

NVIDIA、CUDAは、米国NVIDIA Corporation の米国およびその国の国における登録商標または商標です。

その他、本書に記載されているシステム名、製品名は、一般に各開発メーカーの登録商標あるいは商標です。なお、本文中には™、®マークは明記していません。

ライセンスに関する注意
----------------------

本ソフトウェアには、弊社がその著作権者とのライセンス契約に基づき使用しているソフトウェアが搭載されております。当該ソフトウェアの著作権者様の要求に基づき、弊社はこれらの内容をお客様に通知する義務があります。ライセンス内容に関しては、settings￥about.txtに記載されていますので、内容をご一読くださいますよう、よろしくお願い申し上げます。

バージョンアップ履歴
--------------------

- 2017/08/17 Version 1.00

    - 最初のリリース

- 2017/11/15 Version 1.10

    - AbsoluteError、EpsilonInsensitiveLossレイヤーの追加
    - 学習実行ボタンの右クリックメニューに学習結果ファイルの作成機能を追加
    - バグ、不具合修正

- 2018/06/15 Version 1.20

    - Unit機能の追加（Unitレイヤー、Argumentレイヤー、StructureSearchレイヤーの追加）
    - LeakyReLU、SELU、Swish、MatrixDiag、MatrixDiagPart、FixedPointQuantize、Pow2Quantize、DepthwiseConvolutionレイヤーの追加
    - nntxtインポート機能の追加（beta）
    - pptxレポート作成機能の追加
    - Componentsの文字列による検索機能の追加
    - キャッシュファイルフォーマットの変更による学習速度の向上
    - caffeのprototxt import/export機能における互換性の向上
    - ArrangeLayer機能の改善
    - バグ、不具合修正

- 2018/12/06 Version 1.30

    - Mixed Precision学習に対応
    - 学習結果のnnp、nnb、ONNX形式でのexportに対応
    - nnp、ONNXのimportに対応
    - Cloud版へのデータセットUpload機能の追加
    - DepthwiseDeconvoluion, GlobalAveragePooling, BroadcastTo, ClipGradByNorm, ClipGradByValue, Round, Ceil, Floor, Sin, Cos, Tan, Sinh, Cosh, ASin, ACos, ATan, ASinh, ACosh, ATanh, Pad, TopKData, TopKGrad,FFT,IFFTレイヤーの追加
    - Learning Rate Schedulerの追加（Poly, Exp, Step, Cosine）
    - AMSGRAD Optimizerの追加
    - Command Prompt機能の追加
    - Unit機能の改善
    - バグ、不具合修正

- 2019/02/19 Version 1.40

    - エラー（誤分類）解析機能、推論結果のソート機能の追加
    - htmlレポート作成機能の追加
    - OptimizerにRange、Warmupの設定を追加
    - Interpolate、Sort、Prune、NmsDetection2Dレイヤーの追加
    - 操作性の改善
    - バグ、不具合修正

- 2019/07/30 Version 1.50

    - Cross Tabulation（クロス集計）、Grad-CAM、Tile Images（画像を並べて表示）、ParameterStats（パラメータの統計値を算出）プラグインの追加
    - GELU,ReLU6,HardSigmoid,HardTanh,LogSigmoid,SoftPlus,SoftSign,TanhShrink,Sinc,Tile、Atan2, FusedBatchNormalizationレイヤーの追加
    - Convolution、MaxPooling、AveragePoolingのChannelLastプロパティの追加
    - AdaBound、AMSBound Optimizerの追加
    - 操作性の改善
    - バグ、不具合修正

- 2019/12/03 Version 1.60

    - LIME、Inferenceプラグインの追加
    - Neural Network Consoleクラウド版での学習実行に対応（beta）
    - TensorFlowフォーマット（*.pb）でのexport機能の追加（beta）
    - 操作性の改善
    - バグ、不具合修正

- 2020/03/04 Version 1.70

    - バグ、不具合修正

- 2020/07/30 Version 1.80

    - 自動評価実行機能の追加
    - レイヤー検索・置換機能の追加
    - Ctrl+D、Alt+ドラッグによる複製機能の追加
    - データセットにおいてwavファイルの入力に対応
    - データセットに含まれるCSV、wavファイルの表示を4種類から選択可能に
    - Overviewにて学習設定の確認が可能に
    - ジョブへのコメント機能の追加
    - AddN、MulN、BatchInv、IsInf、IsNaN、ResetInf、ResetNaN、TestNetworkレイヤーの追加
    - AdamW、SgdW、Lars Optimizerの追加
    - Scatter plot、t-SNEプラグインの追加
    - プロパティ、データセットにおける視認性を向上
    - バグ、不具合修正

- 2020/10/12 Version 1.90

    - Neural Network Consoleクラウド版での学習実行において、マルチノード学習とテナントの利用に対応（beta）
    - Clear all parameter files機能の追加
    - 音声波形のプレビュー再生機能の追加
    - Grad-CAM(batch)プラグインの追加
    - Mishレイヤーの追加
    - バグ、不具合修正

- 2020/12/21 Version 2.00

    - 日本語に対応
    - カスタムユニット機能の追加
    - 学習済みモデルのユニットとしてインポート機能の追加
    - SGD Influence、Face evaluation、LIME(batch)、LIME(tabular)プラグインの追加
    - トークンを用いたアクティベーションに対応
    - データセットに含まれるCSV、wavファイルのデフォルトの表示様式が設定可能に
    - バグ、不具合修正

- 2021/05/13 Version 2.10

    - データセットCSVファイルの作成を容易にする各種前処理プラグインの追加
    - TensorFlow 2.0、saved_modelフォーマット、tfliteフォーマットでのexport機能の追加（beta）
    - 中断した学習を再開する機能の追加
    - 学習に要するGPUメモリの削減
    - GPUメモリに乗り切らない大きなモデル学習するOut-of-Core学習機能の追加
    - Unitを複数入力に対応
    - InstanceNormalization、GroupNormalization、LayerNormalization、Norm、
    - NornNormalization、WeightStandardizationレイヤーの追加
    - AdaBelief Optimizerの追加
    - SHAP、SHAP(batch)、TracInプラグインの追加
    - Adultデータセットサンプルプロジェクト、波形分類、2値セグメンテーション、自然言語処理系サンプルプロジェクトの追加
    - プラグインの階層化とUIの改善
    - データセットCSVファイルをUTF-8 encodingに対応
    - バグ、不具合修正

- 2021/07/01 Version 2.20

    - 画像、音声処理を容易にする前処理・後処理プラグインの追加
    - 画像超解像、音声ノイズリダクションサンプルプロジェクト・サンプルデータセットの追加
    - SGD-Influence (tabular)、SmoothGrad、SmoothGrad (batch)、LIME (tabular batch)プラグインの追加
    - バグ、不具合修正

- 2021/10/25 Version 2.30

    - CycleGANのサンプルプロジェクトを追加
    - SHAP(tabular)及びInfluenehce functions (image)プラグインを追加
    - バグ、不具合修正

- 2022/02/01 Version 2.40

    - Import機能（Importレイヤー）の追加
    - XAI Visualization (GradCAM, SHAP, LIME SmoothGrad)プラグインを追加
    - ユニットのデフォルト引数のサポート
    - AdaptiveSeparableConvolution、BatchDet、BatchLogDet、CumSum、CumProd、DeformableConvolution、LogSoftmax、WeightNormalization、SpectralNorm、RandomErase、MinMaxQuantizationレイヤーの追加
    - German Creditサンプルプロジェクトおよびサンプルデータセットの追加
    - CenterNet、RepMLPの簡易サンプルプロジェクトの追加
    - 学習プラグイン、データセットダウンロード＆生成プラグインのサポート
    - SGD Influence (image)の改善
    - プラグインUIの改良
    - バグ、不具合修正

- 2022/06/01 Version 2.50

    - CPU/GPU利用率表示機能の追加
    - 特定のプロパティーの値をまとめて表示・確認する機能の追加
    - MLP-Mixerサンプルプロジェクトの追加
    - PaDiM、Transfer Learning（imageclassification）学習プラグインの追加
    - ネットワークの縮小表示時にマウスのある箇所の拡大表示に対応
    - Feature Visualization (batch)プラグインの追加
    - Representer Point Selectionプラグインの追加
    - Attention Branch Networkサンプルプロジェクトの追加
    - Attention Map プラグインの追加
    - Attention Editorの追加
    - SGD influence (tabular)について、influenceを複数回計算して平均値する処理を追加
    - バグ、不具合修正

- 2022/11/01 Version 2.60

    - インタラクティブデモプラグイン、データセットプラグイン機能の追加
    - D3Net、X-UMX、SLE-GAN、StyleGAN2、StyleGAN2 (Style Mixing)、DeepLabv3+、Face Alignment Network、CenterNet、ESRGANインタラクティブデモプラグインの追加
    - 最近使った後処理プラグイン機能の追加とプラグインUIの改良
    - 公平性の評価指標プラグインの追加
    - バイアス緩和手法(Reweighing, Prejudice Remover Regularizer)の追加
    - バグ、不具合修正

- 2023/03/20 Version 2.70

    - 公平性の評価指標プラグインの追加(Bias Amplification, KL Divergence)
    - 公平性のプラグインの追加（Reweighing weight，Reject Option-Based Classification）
    - ROC曲線，PR曲線表示プラグインの追加
    - バグ、不具合修正

- 2024/01/31 Version 3.10

    - 学習済みモデルを用いた推論実行機能（TransformersのPipeline呼び出し機能）の追加
    - テキストやフォルダからの簡易データセット作成機能の追加
    - Data Preview機能の追加
    - OpenAI GhatGPT APIを用いたChatGPTのクローンUIの追加
    - バグ、不具合修正

- 2024/12/23 Version 3.30

    - Torchのサポートを追加
    - 学習済みモデルを用いた推論実行機能（TransformersのPipeline呼び出し機能）グラフ実行に対応
    - Constant、question-answering、translation、visual-question-answering、Identity、Concatenate、Extract、Video Classification、Zeroshot Object Detectionレイヤーの追加（Transformers）
    - OpenAI GhatGPT APIを用いたChatGPTのクローンUIにFine Tuning機能を追加
    - 学習プラグインの追加
    - クラウド連携機能の削除
    - バグ、不具合修正

- 2025/01/10 Version 3.31

    - 学習時にGPUが利用できなかった不具合を修正

- 2025/07/17 Version 3.40

    - オープンソース版としてリリース
    - バグ、不具合修正

