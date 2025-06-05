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

import io
import os
import paramiko
import scp
import sys
import threading
import time


class RemoteSsh:

    def scp_progress(self, filename, size, sent):
        current_progress_time = time.time()
        if not hasattr(self, 'scp_progress_sent'):
            self.scp_progress_sent = sent
        if not hasattr(self, 'scp_current_progress_time'):
            self.scp_current_progress_time = current_progress_time
        if self.scp_progress_sent > sent:
            self.scp_progress_sent = sent
            self.scp_current_progress_time = current_progress_time
        if current_progress_time - self.scp_current_progress_time > 10:
            self.verbose(f'Copying {str(os.path.basename(filename))} progress: {float(sent*100)/float(size):.2f}%')
            self.scp_current_progress_time = current_progress_time
        self.scp_progress_sent = sent

    def update_pathnames(self):
        """ Update PATH names.
        `~`を展開する
        """
        s, e, code = self.get_execute_output(f'echo {self.remote_base_directory}')
        if code == 0:
            self.remote_base_directory = s.decode().strip()

    def from_private_key(self, keystring):
        """様々なタイプの秘密鍵からParamikoで用いる秘密鍵を作成します。

        Args:
           key: 秘密鍵が格納されているFile-like object

        Returns:
           paramikoで用いる鍵オブジェクト
        """

        if os.path.exists(os.path.expanduser(keystring)):
            key = open(os.path.expanduser(keystring), 'r')
        else:
            key = io.StringIO('\n'.join(keystring.split('\\n')))

        pkey = None
        for kc in [paramiko.ECDSAKey, paramiko.DSSKey, paramiko.Ed25519Key, paramiko.RSAKey]:
            try:
                key.seek(0)
                pkey = kc.from_private_key(key)
                self.verbose(f'{kc.__name__}')
            except:
                pass

        return pkey

    def get_execute_output(self, command, indata=None, max_retry=3):
        """ssh経由でcommandを実行します

        標準入力をEOFまで一気に渡し、コマンドの出力も全て読み出して
        returnするため、時間がかかる処理には向いていません。

        Args:
           command: 実行したい文字列
           indata:  非Noneだった場合、標準入力としてcommandに送信する

        Returns:
           標準出力、標準エラー出力、終了コードのタプル

        """

        retry = 1
        while True:
            try:
                stdin, stdout, stderr = self.ssh.exec_command(command)
                break
            except paramiko.ssh_exception.SSHException:
                if retry < max_retry:
                    retry += 1
                    self.verbose(f'    SSH failed RETRY ({retry}/{max_retry})')
                    self.disconnect()
                    self.connect()
                    continue
                else:
                    break

        if indata:
            stdin.write(indata.read())
        stdin.close()
        std = stdout.read()
        err = stderr.read()
        return std, err, stdout.channel.recv_exit_status()

    def execute(self, command):
        """ssh経由でcommandを実行します


        sshコマンドと同じように、リモートの標準出力と標準エラー出力を
        随時ローカルに出力します。スレッドを用いて処理するため、
        Windowsではうまく途中終了ができない場合があります。


        Args:
           command: 実行したい文字列


        """

        stdin, stdout, stderr = self.ssh.exec_command(command)
        max_size = 100

        def output(remote, local):
            while True:
                remote.flush()
                local.flush()
                buf = remote.read(max_size).decode()
                if buf:
                    local.write(buf)
                else:
                    break
                remote.flush()
                local.flush()

        th_out = threading.Thread(target=output, args=(stdout, sys.stdout))
        th_err = threading.Thread(target=output, args=(stderr, sys.stderr))
        th_out.start()
        th_err.start()
        th_out.join()
        th_err.join()

    def _exec_scp_func(self, scp_func, src, dest, max_retry):
        retry = 1
        while True:
            try:
                getattr(self.scp, scp_func)(src, dest)
                break
            except paramiko.ssh_exception.SSHException:
                if retry < max_retry:
                    retry += 1
                    self.verbose(f'    SCP failed RETRY ({retry}/{max_retry})')
                    self.disconnect()
                    self.connect()
                    continue
                else:
                    break
            except scp.SCPException:
                if retry < max_retry:
                    retry += 1
                    self.verbose(f'    SCP failed RETRY ({retry}/{max_retry})')
                    self.disconnect()
                    self.connect()
                    continue
                else:
                    break
