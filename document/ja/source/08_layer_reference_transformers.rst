レイヤーリファレンス (Transformers)
===================================

|Console| のTransformersプロジェクトで利用可能なレイヤーとそのプロパティについて解説します。


入出力レイヤー
--------------

InputFileName
~~~~~~~~~~~~~

データセットCSVファイルのセルに書かれたファイル名の文字列を入力するレイヤーです。

画像、音声ファイルのファイル名をグラフに入力する際に使用します。


.. list-table::
   :widths: 30 70
   :class: longtable

   * - Dataset
     - データセットに含まれる変数のうち、このInputFileNameレイヤーに入力する変数の変数名を指定します

   * - ArgName
     -
        後続の関数がこの変数を引数として受け取る際の引数名を指定します

        特に引数名を指定しない場合は空のままにします


InputString
~~~~~~~~~~~

データセットCSVファイルのセルに書かれた文字列を入力するレイヤーです。

テキストデータをグラフに入力する際に使用します。


.. list-table::
   :widths: 30 70
   :class: longtable

   * - Dataset
     - データセットに含まれる変数のうち、このInputStringレイヤーに入力する変数の変数名を指定します

   * - ArgName
     -
        後続の関数がこの変数を引数として受け取る際の引数名を指定します

        特に引数名を指定しない場合は空のままにします


Constant
~~~~~~~~

文字列、もしくは値の定数を入力するレイヤーです。

データセットによらない定数をグラフに入力する際に使用します。


.. list-table::
   :widths: 30 70
   :class: longtable

   * - Value
     - 定数の値を指定します

   * - ValueType
     - 定数の型をText（文字列）、Int（整数）、Float（浮動小数点数）から指定します

   * - ArgName
     -
        後続の関数がこの定数を引数として受け取る際の引数名を指定します

        特に引数名を指定しない場合は空のままにします


パイプラインレイヤー
--------------------

HuggingFaceの学習済みモデルを用いて推論を行うレイヤーです。

.. list-table::
   :widths: 30 70
   :class: longtable

   * - audio-classification
     - 
        入力文字列でファイル名の指定された音声ファイルを分類します

        labelとして分類結果を示す文字列、scoreとしてラベルの確率を出力します

   * - automatic-speech-recognition
     - 
        入力文字列でファイル名の指定された音声ファイルに対し音声認識を行います

        textとして音声認識結果の文字列を出力します

   * - depth-estimation
     - 
        入力文字列でファイル名の指定された画像ファイルに対し深度推定を行います

        predicted_depthとして推定した深度の行列、depthとしてpredicted_depthを画像化したものを出力します

   * - feature_extraction
     - 
        入力文字列でファイル名の指定された画像などのファイルに対し特徴抽出を行います

        resultとして抽出した特徴のテンソルを出力します

   * - fill_mask
     - 
        入力文字列中の<mask>とした箇所の単語を推定します

        tokenとして推定した単語のトークンIndex、token_strとして推定した単語、sequenceとして入力文字列の<maxk>を推定した文字列に置き換えた文字列を出力します

   * - image_classification
     - 
        入力文字列でファイル名の指定された画像ファイルを分類します

        labelとして分類結果を示す文字列、scoreとしてラベルの確率を出力します

   * - image_segmentation
     - 
        入力文字列でファイル名の指定された画像ファイルに対しセマンティックセグメンテーションを行います

        \*_scoreとしてラベルのスコアを、\*_labelとしてラベルの文字列を、\*_maskとしてセグメンテーション結果の領域を示す2値画像を出力します

   * - image_to_text
     - 
        入力文字列でファイル名の指定された画像ファイルを文字列に変換します（Captioning）

        0_generated_textとして変換結果のテキストを出力します

   * - object_detection
     - 
        入力文字列でファイル名の指定された画像ファイルに対し物体検出を行います

        \*_scoreとしてラベルのスコアを、\*_labelとして検出した物体のラベルを示す文字列を、\*_box_xmin、\*_box_ymin、\*_box_xmax、\*_box_ymaxとしてそれぞれ検出した物体の左端のx座標、上端のy座標、右端のx座標、下端のy座標を出力します

   * - question_answering
     - 
        引数名contextである入力文字列について、引数名questionである入力文字列の示す質問に答えます

        start、endとして入力文章における回答の開始位置、終了位置を、answerとして回答の文字列を出力します

   * - summarization
     - 
        入力文字列を要約します

        0_summary_textとして要約結果のテキストを出力します

   * - text2text-generation
     - 
        入力文字列を元にテキストを出力します

        0_generated_textとして生成結果のテキストを出力します

   * - text-classification
     - 
        入力文字列を分類します

        labelとして分類結果を示す文字列、scoreとしてラベルの確率を出力します

   * - text-generation
     - 
        入力文字列に続くテキストを生成します

        0_generated_textとして入力文字列に推定結果のテキストを追加したテキストを出力します

   * - token-classification
     - 
        入力文字列を単語ごとに分類します

        \*_entityとして分類結果を示す文字列、\*_scoreとしてスコア、\*_indexとして文章の先頭からの単語のインデックス、\*_wordとして単語の文字列、\*_start、\*_endとしてそれぞれ入力文章における単語の開始位置、終了位置を出力します

   * - translation
     - 
        入力文字列の言語を翻訳します

        \*_translation_textとして翻訳結果を示す文字列を出力します

   * - visual_question_answering
     - 
        引数名contextである入力文字列でファイル名の指定された画像ファイルについて、引数名questionである入力文字列の示す質問に答えます

        \*_scoreとして回答のスコアを、\*_answerとして回答の文字列を出力します

   * - zeroshot-classification
     - 
        入力文字列を、文字列で指定するラベルに従い分類します

        \*_scoreとしてラベルの確率を、\*_labelとして分類結果を示す文字列を出力します

   * - zeroshot-image-classification
     - 
        入力文字列でファイル名の指定された画像ファイルを、文字列で指定するラベルに従い分類します

        \*_scoreとしてラベルの確率を、\*_labelとして分類結果を示す文字列を出力します

   * - zeroshot-audio-classification
     - 
        入力文字列でファイル名の指定された音声ファイルを、文字列で指定するラベルに従い分類します

        \*_scoreとしてラベルの確率を、\*_labelとして分類結果を示す文字列を出力します



パイプラインレイヤー共通プロパティ
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. list-table::
   :widths: 30 70
   :class: longtable

   * - Name
     -
        レイヤー名を指定します
        
        レイヤー名はグラフの中でユニークである必要があります

   * - model
     - HuggingFaceの学習済みモデルのモデル名を指定します

**ご参考**

ひとまず推論の実行を試すだけであれば、modelプロパティーは空欄のままでも推論を実行することができます。この場合、推論には各タスクのデフォルトのモデルが使用されます。

ただしモデルが指定されていない状態では推論結果が不定となるため、実使用においてはmodelプロパティを入力して使用する必要があります。

zeroshopレイヤー共通プロパティ
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. list-table::
   :widths: 30 70
   :class: longtable

   * - candidate_labels
     -
        分類対象のラベルをカンマ区切りのテキストで指定します
        
        例えばzeroshot-image-classificationを用い画像を花と食べ物に分類するには「flower,food」を指定します

   * - hypothesis_template
     -
        candidate_labelsで指定したラベル文字列を装飾するテンプレート文字列を指定します

        ラベルの文字列を挿入する箇所には{}を指定します
        
        例えばcandidate_labelsに「flower,food」、hypothesis_templateに「A photo of {}」を指定し、 「A photo of flower」と「A photo of food」に対する分類を行います


その他のレイヤー
----------------

Comment
~~~~~~~

ネットワークグラフにコメントを挿入します。Commentレイヤーは学習や評価などの処理には影響しません。



.. list-table::
   :widths: 30 70

   * - Comment
     -
        コメントを入力します
        
        入力したコメントのテキストはCommentレイヤー上に表示されます


Identity
~~~~~~~~

入力をそのまま出力します。通常用いる必要はありませんが、ネットワークの特定の箇所に識別用の名前を付けるため、ネットワークの特定の箇所の値を推論結果として出力するためなどに使用します。


Concatenate
~~~~~~~~~~~

2つ以上の入力を結合します。複数の入力データを1つにまとめて出力する場合に使用します。


Extract
~~~~~~~

2つ以上の引数を持つ入力から、特定の引数を抽出します。

複数の出力を持つレイヤーの出力を接続し、複数の出力のうち特定の引数のみを取り出すために、あるいは入力レイヤーの出力に含まれる引数名を別のレイヤーに入力する目的で引数名を書き換えるために使用します。

例えばimage-to-textレイヤーの出力をExtractレイヤーに接続し、Attributeに0_generated_textを、ArgNameにcontextを指定することで、image-to-textレイヤーの出力変数0_generated_textの変数名をcontextに変換することができます。


.. list-table::
   :widths: 30 70

   * - Attribute
     - 抽出する引数名を指定します。

   * - ArgName
     -
        抽出した引数の変換後の引数名を指定します

        特に引数名を指定せず値のみとする場合は空のままにします