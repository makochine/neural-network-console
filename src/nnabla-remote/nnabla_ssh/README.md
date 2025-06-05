# 概要

nnabla_sshはNNablaのプラグインとして実装されています。

nnabla_sshをPYTHONPATHが通った場所に置いて、`nnabla.conf`に以下の
記述を追記することで利用することができます。

    [CALLBACK]
    util_callback_module = nnabla_ssh.callback


# 環境変数リファレンス

`nnabla_ssh`は`nnabla_cli`を全く変更せずに利用するため、各種オプションは環境変数で指定します。


## Common

* `TRAIN_PROCESSOR_INDEX`
* `FORWARD_PROCESSOR_INDEX`
* `PROFILE_PROCESSOR_INDEX`
  *  1から9迄の整数で、利用する設定情報を選択します。

* `REMOTE_PROCESSOR_X_NAME`
 * 設定情報の名前。同時に同じ名前を設定しないようにしてください。

* `REMOTE_PROCESSOR_X_TYPE`
 * 設定情報の種類。現在サポートしているのは、
    - `DIRECT`
    - `DOCKER`
  の2種類となります。

* `REMOTE_PROCESSOR_X_BASEDIR`
  * リモート実行の際に利用するディレクトリ

* `REMOTE_PROCESSOR_X_USER`
  * ssh接続の際に使用するユーザー名

* `REMOTE_PROCESSOR_X_KEY`
  * ssh接続の際に使用する秘密鍵の内容、もしくはファイル名

## DIRECT

sshで接続でき、nnablaが設定済みのマシンを利用します。NNCから利用するNNablaのバージョンを指定することはできません。

* `REMOTE_PROCESSOR_X_HOST`
  * ホスト名


## DOCKER

sshで接続でき、dockerが利用できるマシンを利用します。

* `REMOTE_PROCESSOR_X_HOST`
  * ホスト名

* `REMOTE_PROCESSOR_X_IMAGE`
  * dockerイメージ名

* `REMOTE_PROCESSOR_X_GPUS_FOR_DOCKER`
  * 使用するGPUをカンマ区切りで指定します。指定しないと全てのGPUを使用します。
    - 例: 1
    - 例: 2,3
  DOCKERの場合には「NUM_GPU」は参照しないので注意してください。
