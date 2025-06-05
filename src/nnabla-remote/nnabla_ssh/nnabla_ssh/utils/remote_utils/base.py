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


import abc
import datetime
import os
import sys
import typing

class FileInfo(typing.NamedTuple):
    remote_filename: str
    md5sum: str
    num_of_lines: int


class RemoteBase(metaclass=abc.ABCMeta):

    def __init__(self):
        self.file_info = {}
        self.dataset_files = []

    @abc.abstractmethod
    def start_nnabla_cli(self, command, args):
        """nnabla_cli をバックラウンドで起動する機能を実装するための仮
        想メソッドです。
        """

        pass

    @abc.abstractmethod
    def get_nnabla_cli_log(self, *args, **kwargs):
        """バックラウンドで動作しているnnabla_cliのログを取得します。

        その他、出力ディレクトリの同期、バッックグラウンドで動作してい
        るnnabla_cliの終了判定も同時に実施します。
        """

        pass

    @abc.abstractmethod
    def terminate_nnabla_cli(self):
        """バックグラウンドで動作しているnnabla_cliを停止します。
        """

        pass

    @abc.abstractmethod
    def cleanup_nnabla_cli(self):
        """バックグラウンドで動作(ジョブ投入待ち等も含む)しているnnabla_cliをすべて停止します。

        """

        pass

    def date(self):
        return datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")

    def verbose(self, s):
        """ Log output for debugging
        """
        if self._verbose:
            print(f'{self.date()}  [{self._type}]: {s}', file=sys.stderr, flush=True)

    def error(self, s):
        """ Log output for debugging
        """
        print(f'{self.date()}  [{self._type}]: ERROR: {s}', file=sys.stderr, flush=True)
        sys.exit(-1)

    def signal_handler(self, signum, frame):
        """シグナルハンドラ

        CTRL-CやBREAKなどで終了された場合に後始末を行う
        """
        # 出力ファイルを取得
        self.sync_output_from_remote()
        self.cleanup()
        # バックグラウンドで実行されているnnabla_cliを強制終了
        self.terminate_nnabla_cli()
        # スレッドが生きているとうまく終了できないことがあるので自分自身をKILLして終了する
        os.kill(os.getpid(), 9)
