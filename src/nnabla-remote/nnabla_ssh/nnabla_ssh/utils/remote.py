# Copyright 2022,2023,2024,2025 Sony Group Corporation.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

""" Remoteクラスを提供するモジュール

Remoteクラスはabc.abstractmethodを含む仮想クラスとなっており、このモ
ジュールをユーザーが直接importして利用することは想定していません。

"""

from nnabla_ssh.utils import RemoteBase
from nnabla_ssh.utils import RemoteSsh
from nnabla_ssh.utils import RemoteConvert
from nnabla_ssh.utils import RemoteCopy
from nnabla_ssh.utils import RemoteCommand


class Remote(RemoteBase, RemoteSsh, RemoteConvert, RemoteCopy, RemoteCommand):
    """nnabla_cliコマンドをリモートで実行するための機能を提供します。

    いくつかのメソッドをabstractとしており、nnabla_cliの実行方法、接続
    方法を柔軟にカスタマイズして利用することを想定しています。
    """

    def __init__(self):
        super(Remote, self).__init__()
