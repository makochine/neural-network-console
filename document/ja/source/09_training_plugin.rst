PaDiM
~~~~~

画像の異常検知手法の1つであるPaDiMを用い、画像異常検知モデルの学習を行います。

PaDiMを用いることで、高性能な異常検知モデルを正常画像のみを用い高速に学習することができます。


PaDiM: a Patch Distribution Modeling Framework for Anomaly Detection and Localization

Thomas Defard, Aleksandr Setkov, Angelique Loesch, Romaric Audigier

https://arxiv.org/abs/2011.08785


PaDiMを実行するには、予めデータセットタブにて変数xとして画像サイズ224x224のRGB画像を指定した学習用、評価用データセットを読み込みます。

ここで、学習用データセットには正常画像のみを含むようにします。

また、CONFIGタブのGlobal Configにてバッチサイズの設定を行います。

本プラグイン実行のために編集タブでネットワークを設計する必要はありません（編集タブのネットワーク定義は空白のままにします）。


.. list-table::
   :widths: 30 70
   :class: longtable

   * - model
     - 異常検知に用いるImageNetの学習済みモデルを選択します

   * - feature_ratio
     -
        異常検知に用いる学習済みモデルの中間特徴量の割合を指定します
        
        50%の特徴を用いるには、0.5を指定します

   * - gaussian
     -
        マハラノビス距離計算に用いるガウス分布を求める粒度を指定します
        
        dense
            Layer1（最も解像度の高い特徴マップ）のピクセル毎にガウス分布を求めます。PaDiMの論文通りの動作になります

        1/16
            Layer3（最も解像度の低い特徴マップ）のピクセル毎にガウス分布を求めます。denseと比較しモデルのサイズをやや節約することができます

        single
            画像全体で1つのガウス分布のみ用います。denseと比較しモデルのサイズを大きく節約することができます


**ご参考**

メモリ不足で学習に失敗する場合はコンフィグタブ、Global Configにてより小さいバッチサイズを選択します。


PaDiM (C1)
~~~~~~~~~~~

PaDiM (C1)は、商用利用可能なデータセットC1(Commercially available dataset version 1)を用い学習した学習済みモデルを利用するプラグインです。

画像の異常検知手法の1つであるPaDiMを用い、画像異常検知モデルの学習を行います。

PaDiMを用いることで、高性能な異常検知モデルを正常画像のみを用い高速に学習することができます。


PaDiM: a Patch Distribution Modeling Framework for Anomaly Detection and Localization

Thomas Defard, Aleksandr Setkov, Angelique Loesch, Romaric Audigier

https://arxiv.org/abs/2011.08785


PaDiMを実行するには、予めデータセットタブにて変数xとして画像サイズ224x224のRGB画像を指定した学習用、評価用データセットを読み込みます。

ここで、学習用データセットには正常画像のみを含むようにします。

また、CONFIGタブのGlobal Configにてバッチサイズの設定を行います。

本プラグイン実行のために編集タブでネットワークを設計する必要はありません（編集タブのネットワーク定義は空白のままにします）。


.. list-table::
   :widths: 30 70
   :class: longtable

   * - model
     - 異常検知に用いる学習済みモデルを選択します

   * - feature_ratio
     -
        異常検知に用いる学習済みモデルの中間特徴量の割合を指定します
        
        50%の特徴を用いるには、0.5を指定します

   * - gaussian
     -
        マハラノビス距離計算に用いるガウス分布を求める粒度を指定します
        
        dense
            Layer1（最も解像度の高い特徴マップ）のピクセル毎にガウス分布を求めます。PaDiMの論文通りの動作になります

        1/16
            Layer3（最も解像度の低い特徴マップ）のピクセル毎にガウス分布を求めます。denseと比較しモデルのサイズをやや節約することができます

        single
            画像全体で1つのガウス分布のみ用います。denseと比較しモデルのサイズを大きく節約することができます


**ご参考**

メモリ不足で学習に失敗する場合はコンフィグタブ、Global Configにてより小さいバッチサイズを選択します。


Transfer Learning (C1)
~~~~~~~~~~~~~~~~~~~~~~~

Transfer Learning (C1)は、商用利用可能なデータセットC1(Commercially available dataset version 1)を用い学習した学習済みモデルを利用するプラグインで、高性能な画像分類モデルを高速に学習する転移学習を行います。

Transfer Learningを実行するには、予めデータセットタブにて変数xとして正方形のRGB画像を、変数yとして0から始まるクラスラベルを指定した学習用、評価用データセットを読み込みます。

また、CONFIGタブのGlobal Configにて学習反復世代数、バッチサイズの設定を、Optimizerにて最適化アルゴリズムの設定を行います。

本プラグイン実行のために編集タブでネットワークを設計する必要はありません（編集タブのネットワーク定義は空白のままにします）。


.. list-table::
   :widths: 30 70
   :class: longtable

   * - model
     - 転移学習に用いる学習済みモデルを選択します

   * - train_param
     -
        学習するパラメータを指定します
        
        all
            全てのレイヤーのパラメータを学習します（Fine Tuning）

        last_fc_only
            ほとんどのレイヤーのパラメータは固定し、画像分類モデル最後の全結合層のパラメータのみを学習します

   * - max_zoom_ratio
     -
        学習時のImage Augmentation処理における最大の拡大率を指定します
        
        画像拡大によるImage Augmentationを行わない合は1.0を指定します

   * - max_aspect_ratio
     -
        学習時のImage Augmentation処理における最大のアスペクト比の変更率を指定します
        
        アスペクト比変更によるImage Augmentationを行わない場合は1.0を指定します

   * - lr_flip
     -
        学習時のImage Augmentation処理における左右反転を行うかどうかを指定します
        
        左右反転によるImage Augmentationを行わない場合はチェックを外します

   * - crop_ratio
     -
        推論時に入力画像の中央を切り出す比率を指定します
        
        例えば256x256ピクセルの画像の中央224x224ピクセルを用いた推論を行うには0.875を指定します
        
        画像中央の切り出しを行わない場合は1.0を指定します


**ご参考**

メモリ不足で学習に失敗する場合はコンフィグタブ、Global Configにてより小さいバッチサイズを選択します。

実バッチサイズを小さくしたくない場合は、コンフィグタブ、Optimizerの更新間隔により大きい値を指定します。

例えばバッチサイズを64、更新間隔を4とした場合、実バッチサイズは64 * 4の256となります。


Transfer Learning (image classification)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

ImageNetの学習済みモデルを利用し、高性能な画像分類モデルを高速に学習する転移学習を行います。


Transfer Learningを実行するには、予めデータセットタブにて変数xとして正方形のRGB画像を、変数yとして0から始まるクラスラベルを指定した学習用、評価用データセットを読み込みます。

また、CONFIGタブのGlobal Configにて学習反復世代数、バッチサイズの設定を、Optimizerにて最適化アルゴリズムの設定を行います。

本プラグイン実行のために編集タブでネットワークを設計する必要はありません（編集タブのネットワーク定義は空白のままにします）。


.. list-table::
   :widths: 30 70
   :class: longtable

   * - model
     - 転移学習に用いるImageNetの学習済みモデルを選択します

   * - train_param
     -
        学習するパラメータを指定します
        
        all
            全てのレイヤーのパラメータを学習します（Fine Tuning）

        last_fc_only
            ほとんどのレイヤーのパラメータは固定し、画像分類モデル最後の全結合層のパラメータのみを学習します

   * - max_zoom_ratio
     -
        学習時のImage Augmentation処理における最大の拡大率を指定します
        
        画像拡大によるImage Augmentationを行わない合は1.0を指定します

   * - max_aspect_ratio
     -
        学習時のImage Augmentation処理における最大のアスペクト比の変更率を指定します
        
        アスペクト比変更によるImage Augmentationを行わない場合は1.0を指定します

   * - lr_flip
     -
        学習時のImage Augmentation処理における左右反転を行うかどうかを指定します
        
        左右反転によるImage Augmentationを行わない場合はチェックを外します

   * - crop_ratio
     -
        推論時に入力画像の中央を切り出す比率を指定します
        
        例えば256x256ピクセルの画像の中央224x224ピクセルを用いた推論を行うには0.875を指定します
        
        画像中央の切り出しを行わない場合は1.0を指定します


**ご参考**

メモリ不足で学習に失敗する場合はコンフィグタブ、Global Configにてより小さいバッチサイズを選択します。

実バッチサイズを小さくしたくない場合は、コンフィグタブ、Optimizerの更新間隔により大きい値を指定します。

例えばバッチサイズを64、更新間隔を4とした場合、実バッチサイズは64 * 4の256となります。


Transfer Learning (ViT)
~~~~~~~~~~~~~~~~~~~~~~~

ViTの学習済みモデルを利用し、高性能な画像分類モデルを高速に学習する転移学習を行います。


Transfer Learningを実行するには、予めデータセットタブにて変数xとして正方形のRGB画像を、変数yとして0から始まるクラスラベルを指定した学習用、評価用データセットを読み込みます。

また、CONFIGタブのGlobal Configにて学習反復世代数、バッチサイズの設定を、Optimizerにて最適化アルゴリズムの設定を行います。

本プラグイン実行のために編集タブでネットワークを設計する必要はありません（編集タブのネットワーク定義は空白のままにします）。


An Image is Worth 16x16 Words: Transformers for Image Recognition at Scale

Alexey Dosovitskiy, Lucas Beyer, Alexander Kolesnikov, Dirk Weissenborn, Xiaohua Zhai, Thomas Unterthiner, Mostafa Dehghani, Matthias Minderer, Georg Heigold, Sylvain Gelly, Jakob Uszkoreit, Neil Houlsby

https://arxiv.org/abs/2010.11929


本プラグインはNVIDIA製GPU搭載環境でのみ動作します。

.. list-table::
   :widths: 30 70
   :class: longtable

   * - model
     - 転移学習に用いるViTの学習済みモデルを選択します

   * - train_param
     -
        学習するパラメータを指定します
        
        all
            全てのレイヤーのパラメータを学習します（Fine Tuning）

        last_fc_only
            ほとんどのレイヤーのパラメータは固定し、画像分類モデル最後の全結合層のパラメータのみを学習します

   * - max_zoom_ratio
     -
        学習時のImage Augmentation処理における最大の拡大率を指定します
        
        画像拡大によるImage Augmentationを行わない合は1.0を指定します

   * - max_aspect_ratio
     -
        学習時のImage Augmentation処理における最大のアスペクト比の変更率を指定します
        
        アスペクト比変更によるImage Augmentationを行わない場合は1.0を指定します

   * - lr_flip
     -
        学習時のImage Augmentation処理における左右反転を行うかどうかを指定します
        
        左右反転によるImage Augmentationを行わない場合はチェックを外します

   * - crop_ratio
     -
        推論時に入力画像の中央を切り出す比率を指定します
        
        例えば256x256ピクセルの画像の中央224x224ピクセルを用いた推論を行うには0.875を指定します
        
        画像中央の切り出しを行わない場合は1.0を指定します

**ご参考**

GPUメモリ不足で学習に失敗する場合はコンフィグタブ、Global Configにてより小さいバッチサイズを選択します。

実バッチサイズを小さくしたくない場合は、コンフィグタブ、Optimizerの更新間隔により大きい値を指定します。

例えばバッチサイズを64、更新間隔を4とした場合、実バッチサイズは64 * 4の256となります。


Zero-shot image classification (CLIP)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

CLIPの学習済みモデルを利用し、画像について記述された文字列を元に画像の分類モデルを作成します。


Zero-shot image classificationを実行するには、予めデータセットタブにて変数xとして正方形のRGB画像を、変数yとして0から始まるクラスラベルを指定した評価用データセットを読み込みます。

本プラグインは学習用データセットを用いないため、学習用データセットは不要です。

また、本プラグイン実行のために編集タブでネットワークを設計する必要はありません（編集タブのネットワーク定義は空白のままにします）。


Learning Transferable Visual Models From Natural Language Supervision

Alec Radford, Jong Wook Kim, Chris Hallacy, Aditya Ramesh, Gabriel Goh, Sandhini Agarwal, Girish Sastry, Amanda Askell, Pamela Mishkin, Jack Clark, Gretchen Krueger, Ilya Sutskever

https://arxiv.org/abs/2103.00020


本プラグインはNVIDIA製GPU搭載環境でのみ動作します。

.. list-table::
   :widths: 30 70
   :class: longtable

   * - model
     - 分類モデル作成に用いるViTの学習済みモデルを選択します

   * - query_text
     -
        分類する画像について記述した分類先クラス毎のテキストをカンマ区切りで指定します
        
        例えば犬の写真と猫の写真を分類するには、「a dog photo,a cat photo」と入力します

   * - query_text_csv_file
     -
        分類する画像について記述した分類先クラス毎のテキストが各行に書かれたcsvファイルを指定します
        
        query_textが指定された場合、query_text_csv_fileは無視されます


PatchCore
~~~~~~~~~

画像の異常検知手法の1つであるPatchCoreを用い、画像異常検知モデルの学習を行います。

PatchCoreを用いることで、高性能な異常検知モデルを正常画像のみを用い作成することができます。


Towards Total Recall in Industrial Anomaly Detection

Karsten Roth, Latha Pemula, Joaquin Zepeda, Bernhard Schölkopf, Thomas Brox, Peter Gehler

https://arxiv.org/abs/2106.08265


PatchCoreを実行するには、予めデータセットタブにて変数xとして画像サイズ224x224のRGB画像を指定した学習用、評価用データセットを読み込みます。

ここで、学習用データセットには正常画像のみを含むようにします。

また、CONFIGタブのGlobal Configにてバッチサイズの設定を行います。

本プラグイン実行のために編集タブでネットワークを設計する必要はありません（編集タブのネットワーク定義は空白のままにします）。


.. list-table::
   :widths: 30 70
   :class: longtable

   * - model
     - 異常検知に用いるImageNetの学習済みモデルを選択します

   * - feature_ratio
     -
        異常検知に用いる学習済みモデルの中間特徴量の割合を指定します
        
        50%の特徴を用いるには、0.5を指定します

   * - sample_ratio
     -
        全学習データから求めた中間特徴量のうち、保持するデータ点の割合を指定します
        
        1%の特徴を用いるには、0.01を指定します


**ご参考**

PatchCoreの実行には長い時間を要することがあります。


**ご参考**

メモリ不足で学習に失敗する場合はコンフィグタブ、Global Configにてより小さいバッチサイズを選択します。


