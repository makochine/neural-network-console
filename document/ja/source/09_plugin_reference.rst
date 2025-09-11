プラグインリファレンス
======================

|Console| で利用可能なプラグインとそのプロパティについて解説します。

前処理プラグイン
----------------

データセットの作成を行うための前処理プラグインを実行するには、データセット管理画面左上のデータセットを作成ボタンをクリックして表示されるポップアップメニューのCreate Datasetからプラグインを選択します。

.. toctree::
   :maxdepth: 1

   Plugin/Pre_Process/Create_Dataset/ObjectDetection.rst
   Plugin/Pre_Process/Create_Dataset/SimpleTextClassification.rst
   Plugin/Pre_Process/Create_Dataset/StringClassification.rst
   Plugin/Pre_Process/Create_Dataset/SimpleTabularDataset.rst
   Plugin/Pre_Process/Create_Dataset/SplitImage.rst
   Plugin/Pre_Process/Create_Dataset/SplitWav.rst


オープンデータセットのダウンロードや人工データセットの生成を実行するには、データセット管理画面左上のデータセットを作成ボタンをクリックして表示されるポップアップメニューのDownload or Generate Datasetからデータセットを選択します。


後処理プラグイン
----------------


各種後処理プラグインを実行するには、評価タブの評価結果を右クリックして表示されるポップアップメニューからプラグインを選択します。

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


学習プラグイン
--------------


各種学習プラグインを実行するには、編集タブ右上のRunボタン左のリストボックスからプラグインを選択し、Runボタンをクリックします。

学習プラグインによる学習が完了すると、学習結果フォルダに学習プラグインにより学習されたモデルのnnpファイルが保存されます。

学習プラグインにより作成されたモデルは、通常の学習により作成されたモデル同様に学習タブからの評価実行等を行うことができます。


DARTS (image classification)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Neural Architecture Searchの1手法であるDARTSを用い、画像分類タスクのためのネットワークアーキテクチャの探索と学習を行います。

DARTSを用いることで、学習データに合わせたコンパクトかつ高性能なネットワークアーキテクチャを自動で探索することができます。


DARTS: Differentiable Architecture Search

Hanxiao Liu, Karen Simonyan, Yiming Yang

https://arxiv.org/abs/1806.09055


DARTSを実行するには、予めデータセットタブにて変数xが画像サイズ32x32のRGB画像、yが0から始まるクラスラベルで構成された学習用、評価用データセットを読み込みます。

また、CONFIGタブのGlobal Configにて学習反復世代数、バッチサイズの設定を、Optimizerにて最適化アルゴリズムの設定を行います。

本プラグイン実行のために編集タブでネットワークを設計する必要はありません（編集タブのネットワーク定義は空白のままにします）。


.. list-table::
   :widths: 30 70
   :class: longtable

   * - mode
     -
        DARTSの実行モードを指定します
        
        アーキテクチャの探索と学習の両方を実行する場合はbothを、探索のみを実行する場合はsearchを、学習のみを実行する場合はtrainを指定します

   * - arch
     -
        modeとしてtrainを指定した場合に、学習に用いる探索済みのアーキテクチャを示すarchファイルを指定します
        
        archファイルはsearchを実行した学習結果フォルダに保存されています

   * - train_portion
     -
        アーキテクチャの探索に用いる学習データの割合を指定します
        
        50%のデータを用いるには0.5を指定します

   * - epoch_search
     - アーキテクチャの探索を行うエポック数を指定します

   * - init_channel_search
     - アーキテクチャ探索時の最初のレイヤーのチャンネル数を指定します

   * - cells_search
     - アーキテクチャ探索時のセルの数を指定します

   * - init_channel_train
     - 学習時の最初のレイヤーのチャンネル数を指定します

   * - cells_train
     - 学習時のセルの数を指定します

   * - auxiliary_train
     - 学習時に補助ロスを用いるかどうかを指定します

   * - drop_path_train
     -
        学習時にDrop Pathを行う割合を指定します
        
        10%の確率でDropするには0.1を指定します

   * - print_frequency
     - 処理の進捗表示を更新する頻度をIteration単位で指定します


**ご参考**

メモリ不足で学習に失敗する場合はコンフィグタブ、Global Configにてより小さいバッチサイズを選択します。

実バッチサイズを小さくしたくない場合は、コンフィグタブ、Optimizerの更新間隔により大きい値を指定します。

例えばバッチサイズを64、更新間隔を4とした場合、実バッチサイズは64 * 4の256となります。


.. include:: 09_training_plugin.rst


AI Solutionsプラグイン
----------------------

AI Solutionsプラグインを通じて、各社から提供されているAI関連技術を手軽に試すことができます。

各種AI Solutionsプラグインを実行するには、起動後の画面左側のリストよりAI Solutionsを選択し、一覧より実行する機能をクリックします。


Chatbot using OpenAI API
~~~~~~~~~~~~~~~~~~~~~~~~


OpenAIのAPIを利用したChatGPTのクローンです。

https://platform.openai.com/

**ご注意**

OpenAIの提供するAPIの業務での利用可否の判断は所属する組織にてお願いいたします。


**ご参考**

本プラグインはAPIのエンドポイントとして、/v1/chat/completionsを用いています。


事前セットアップ
^^^^^^^^^^^^^^^^

本プラグインの利用のためには、事前にOpenAIのAPI Keyの発行と設定が必要です。

1. 以下のOpen AIのWebサイトより、OpenAIのAPIキーを作成します。

https://platform.openai.com/account/api-keys

2. ウインドウ右上の設定（セットアップ）ボタンをクリックして設定画面を開きます。

3. 設定画面のエンジンタブで、環境変数のOPENAI_API_KEY_NNC=の行に、作成したAPIキーを入力します。

4. 設定画面右下の適用ボタンを押して設定画面を閉じます。


Chatbot using OpenAI APIを起動するには
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

起動後の画面左側のリストよりAI Solutionsを選択し、一覧よりChatbot using OpenAI APIを選択します。

コマンドプロンプトとChatbot using ChatGPT APIのWebブラウザが起動します。


Chatbot using OpenAI APIを終了するには
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

起動時したコマンドプロンプト、およびWebブラウザのChatbot using ChatGPT APIのタブを閉じます。


チャットやFine Tuningに使用するモデルを変更するには
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

画面右上のコンボボックスより、チャットに用いるモデルを選択します。

モデルについての詳細はOpenAIのWebサイトをご参照ください。

https://platform.openai.com/docs/models/

既に行ったFine Tuning済みのモデルが存在する場合は、それらのモデルもここから選択可能になります。


チャットを行うには
^^^^^^^^^^^^^^^^^^

1. システムプロンプトを用いる場合は、Type system message here.と書かれたテキストボックスにシステムプロンプトを入力します。システムプロンプトを用いない場合はこのステップは必要ありません。

**ご参考**

システムプロンプトを用いることにより、チャットボットの役割や動作を事前に指定することができます。

2. Type your message here.と書かれたテキストボックスにテキストを入力します。

3. テキストボックス右下のSend Messageボタンを押して入力したテキストを送信します。

..

   もしくは、キーボードのCtrlキーを押しながらEnterキーを押してテキストを送信します。
   
メッセージを送信してしばらく待つと、チャットボットの応答が表示されます。

続けてチャットを行うには、さらにテキストボックスにテキストを入力してテキストを送信します。


新しいチャットを開始するには
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

画面左上の+ New Chatボタンをクリックします。


以前行ったチャットのログを読み込むには
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

画面左に表示される過去のチャット履歴のリストより、読み込むログをクリックします。


以前行ったチャットのログの保存されたフォルダを開くには
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

画面左上のOpen logボタンをクリックします。

ログの保存されたフォルダがExplorerで開かれます。


よく利用するシステムプロンプトを保存しておくには
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

1. システムプロンプトを入力するか、テキストボックスをクリックして選択します。

2. システムプロンプト右下に表示される保存ボタンをクリックします。


過去に保存したシステムプロンプトを呼び出すには
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

システムプロンプトのテキストボックス上のリストボックスをクリックし、読み込むシステムプロンプトを選択します。


過去に保存したシステムプロンプトを編集するには
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

1. システムプロンプトのテキストボックス上のリストボックスをクリックし、[Edit a saved prompt list]を選択します。システムプロンプトの保存されたテキストファイルがメモ帳で開かれます。

2. システムプロンプトを編集し、保存します。



Fine Tuningのためのデータセットを準備するには
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

ChatGPTのFine Tuningを行うためには、以下のような1行のテキストがJSON形式であるjsonlファイル形式でデータセットファイルを準備します。

ここでroleがsystemのcontentはシステムプロンプト、roleがuserのcontentはユーザの入力、roleがassistantのcontentはAIの応答を示します。

    {"messages": [{"role": "system", "content": "あなたは挨拶をするAIです。ユーザの入力に対して挨拶を行ってください。"}, {"role": "user", "content": "おはよう"}, {"role": "assistant", "content": "おはようございます"}]}

    {"messages": [{"role": "system", "content": "あなたは挨拶をするAIです。ユーザの入力に対して挨拶を行ってください。"}, {"role": "user", "content": "こんにちは"}, {"role": "assistant", "content": "こんにちは"}]}

    {"messages": [{"role": "system", "content": "あなたは挨拶をするAIです。ユーザの入力に対して挨拶を行ってください。"}, {"role": "user", "content": "こんばんは"}, {"role": "assistant", "content": "こんばんは"}]}

    …

また、以下のような1行目がrole、2行目以降がテキストであるCSVファイルとして入力テキストを準備し、jsonl形式に変換することもできます。

    system,user,assistant

    あなたは挨拶をするAIです。ユーザの入力に対して挨拶を行ってください。,おはよう,おはようございます

    あなたは挨拶をするAIです。ユーザの入力に対して挨拶を行ってください。,こんにちは,こんにちは

    あなたは挨拶をするAIです。ユーザの入力に対して挨拶を行ってください。,こんばんは,こんばんは

    …

準備したCSVファイルをjsonl形式に変換するには

1. 画面上のFilesボタンをクリックし、Filesタブを表示します。

2. 画面左上の+ Upload New Datasetボタンをクリックします。

3. ファイルを選択ボタンから準備したCSVファイルを選択します。

4. Convertボタンをクリックします。変換後のjsonlファイルのダウンロードがはじまります。


**ご注意**

マルチバイト文字を含むデータセットを準備する場合、準備するjsonl、CSVファイルのエンコーディングはUTF-8である必要があります。

Fine Tuningを実行するためには最低10のExampleを与える必要があります。

**ご参考**

roleをさらにuser、assistantと続けることで、より長い対話シーケンスを含む学習データを作成することができます。

CSVファイルを用いる場合、対話終了後のセルは空白とすることで可変長の対話を与えることができます。


Fine Tuningのためのデータセットをアップロードするには
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

1. 画面上のFilesボタンをクリックし、Filesタブを表示します。

2. 画面左上の+ Upload New Datasetボタンをクリックします。

3. ファイルを選択ボタンから準備したjsonlファイルを選択します。

4. Uploadボタンをクリックします。


アップロード済みのデータセットを確認するには
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

1. 画面上のFilesボタンをクリックし、Filesタブを表示します。

2. 画面左のリストから、アップロードしたデータセットを選択します。

アップロード直後のデータセットのstatusはuploadedとなります。

しばらくしてstatusがprocessedに変わるとFine Tuningの実行が可能になります。


アップロード済みのデータセットを用いたFine Tuningを実行するには
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

1. 画面上のFilesボタンをクリックし、Filesタブを表示します。

2. 画面左のリストから、Fine Tuningに用いるデータセットを選択します。
   
3. 画面右上のコンボボックスより、Fine Tuningに用いるモデルを選択します。
   
4. Run fine tuning using this dataボタンをクリックします。
   
5. Fine Tuningのジョブの作成を確認するダイアログボックスのOKボタンをクリックします。


アップロード済みのデータセットを削除するには
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

1. 画面上のFilesボタンをクリックし、Filesタブを表示します。

2. 画面左のリストから、削除対象のデータセットを選択します。
   
3. Deleteボタンをクリックします。

4. 削除の確認を求めるダイアログボックスのOKボタンをクリックします。


実行中のFine Tuningジョブを確認するには
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

1. 画面上のFine Tuning Jobボタンをクリックし、Fine Tuning Jobタブを表示します。

2. 画面左のリストから、確認対象のジョブを選択します。
   
ジョブのstatusはFine Tuning実行直後はcreatedに、Fine Tuningが開始になるとrunningに、Fine Tuningが完了するとsucceededになります。

Fine Tuning済みモデルは画面右上のコンボボックスより選択し、チャットやさらなるFine Tuningのために利用可能になります。


実行中のFine Tuningジョブをキャンセルするには
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

1. 画面上のFine Tuning Jobボタンをクリックし、Fine Tuning Jobタブを表示します。

2. 画面左のリストから、キャンセル対象のジョブを選択します。
   
3. Cancelボタンをクリックします。

キャンセルされたジョブのstatusはcancelledになります。

**ご参考**

OpenAIの仕様によりジョブの履歴を削除することはできません。
