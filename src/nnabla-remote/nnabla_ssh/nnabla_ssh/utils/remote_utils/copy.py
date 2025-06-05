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

import csv
import gzip
import io
import json
import os
import paramiko
import pathlib
import scp
import tarfile
import time
import tqdm

from nnabla_ssh.utils import FileInfo


class RemoteCopy:

    ####################################################################################################################
    # public methods

    def copy_csv_dataset_to_remote(self, local_csv_filename, force=False):
        self.verbose(f'Sending CSV dataset [{local_csv_filename}]')

        # CSVデータセットの情報を取得
        csv_file_info, csv_dict_filename = self.csv_file_info(local_csv_filename)

        # Server上に既に存在するか確認する
        overwrite = False
        if force:
            # 強制的に全て上書き
            overwrite = True
        else:
            if self.check_remote_file_exists(csv_file_info.remote_filename):
                # CSVデータセットは既に存在する
                if self.check_remote_file_exists(csv_dict_filename):
                    # ローカルファイル名とリモートファイル名の変換辞書も存在するのでサーバから取得する
                    for filename, info in json.loads(self.get_remote_file_content(csv_dict_filename)).items():
                        self.file_info[filename] = FileInfo(*info)
                    self.verbose(f'CSV dataset [{local_csv_filename}] already exists on server.')
                    self.dataset_files.append(csv_file_info.remote_filename)
                    return csv_file_info.remote_filename
                else:
                    # 辞書のみ作成する
                    self.verbose(f'CSV dataset [{local_csv_filename}] already exists on server.')
                    self.verbose(f' but dataset file dictionary does not exist.')
            else:
                # サーバ上にはまだ存在しないのでローカルから全てコピーする
                overwrite = True

        if overwrite:
            desc = f'Sending {os.path.basename(local_csv_filename)}'
        else:
            desc = f'Parsing {os.path.basename(local_csv_filename)}'

        t = tqdm.tqdm(total=csv_file_info.num_of_lines, desc=desc)

        dirs = []
        files = []
        for ret in self.convert_csv_file(local_csv_filename):
            for local, remote in ret:
                d = os.path.dirname(remote)
                if d not in dirs:
                    self.remote_mkdir_p(d)
                    dirs.append(d)
                if isinstance(local, io.StringIO):
                    self.copyfo_to_remote(local, remote)
                elif isinstance(local, str):
                    if overwrite:
                        files.append((local, remote))
                    if len(files) >= 100:
                        self.copy_files_to_remote(files)
                        files = []
                t.update(1)

        if len(files) > 0:
            self.copy_files_to_remote(files)
        t.close()
        self.dataset_files.append(csv_file_info.remote_filename)
        return csv_file_info.remote_filename

    def copy_config_file_to_remote(self, local_config_filename, remote_config_filename, copy_dataset=True):
        with self.convert_config_file(local_config_filename) as (content, csvs):
            for csv_path in csvs:
                if copy_dataset:
                    self.copy_csv_dataset_to_remote(csv_path)
            self.copyfo_to_remote(content, remote_config_filename)

    def remote_mkdir_p(self, remote_dir):
        s, e, code = self.get_execute_output(f'mkdir -p {remote_dir}')
        if code != 0:
            return False
        return True

    def get_remote_file_content(self, remote_filename):
        s, e, code = self.get_execute_output(f'cat {remote_filename}')
        if code != 0:
            return None
        return s

    def check_remote_file_exists(self, fn):
        s, e, code = self.get_execute_output(f'test -f "{fn}"')
        return code == 0

    def copy_from_remote(self, src, dest, max_retry=3):
        self._exec_scp_func('get', src, dest, max_retry)

    def copy_to_remote(self, src, dest, max_retry=3):
        self._exec_scp_func('put', src, dest, max_retry)

    def copyfo_to_remote(self, src, dest, max_retry=3):
        self._exec_scp_func('putfo', src, dest, max_retry)

    def copy_files_to_remote(self, files):
        # オンメモリでgz圧縮したtarデータを作成する
        o = io.BytesIO()
        tar = tarfile.TarFile(fileobj=o, mode='w')
        for local, remote in files:
            if remote.startswith(self.remote_base_directory):
                n = remote[len(self.remote_base_directory) + 1:]
                info = tarfile.TarInfo(name=n)
                info.size = os.path.getsize(local)
                with open(local, 'rb') as f:
                    tar.addfile(info, f)
            else:
                self.error(f'Could not send [{remote}].')
        tar.close()
        gz = gzip.compress(o.getvalue())
        o.close()

        # データを標準出力経由でリモートに送信し、リモートではtar
        # コマンドで展開する
        s, e, code = self.get_execute_output(f'tar Cxvzfm {self.remote_base_directory} -', io.BytesIO(gz))
        if code != 0:
            self.verbose(f'Could not send data')
            return False
        return True

    def copy_progress_files(self, fn):
        self.copy_from_remote('/'.join([self.remote_outdir, fn]), os.path.join(self.outdir, fn), max_retry=1)

    def sync_output_from_remote(self):
        """出力結果をリモートからローカルにコピーする
        """
        self.verbose('COPY OUTPUT')
        s, e, code = self.get_execute_output(f'find {self.remote_outdir} -type f')
        if code >= 0:
            remote_files = [l.decode().rstrip() for l in s.splitlines()]
            list_remote = set([str(pathlib.Path(f).relative_to(self.remote_outdir)) for f in remote_files])
            list_local = []
            for curdir, dirs, files in os.walk(self.outdir):
                for fn in files:
                    fn = str(pathlib.Path(os.path.join(curdir, fn)).relative_to(self.outdir))
                    list_local.append(fn)
            list_local = set(list_local)
            for fn in remote_files:
                if os.path.basename(fn) in ['log.txt']:
                    self.verbose(f'SKIP {fn}')
                    continue
                self.verbose(f'COPYING {fn}')
                fn_out = os.path.join(self.outdir, os.path.relpath(fn, self.remote_outdir))
                fn_out_dir = os.path.dirname(fn_out)
                if not os.path.isdir(fn_out_dir):
                    os.makedirs(fn_out_dir)
                _, e = os.path.splitext(fn)
                e = e.lower()
                if e == '.csv':
                    self.verbose(f'Deconvert {fn}')
                    s, e, code = self.get_execute_output(f'cat {fn}')
                    converted = self.deconvert_csv_content(s)
                    with open(fn_out, 'w') as f:
                        for l in converted:
                            print(l, file=f)
                elif e == '.nntxt':
                    self.verbose(f'Deconvert {fn}')
                    s, e, code = self.get_execute_output(f'cat {fn}')
                    converted = self.deconvert_nntxt_content(s)
                    with open(fn_out, 'w') as f:
                        for l in converted:
                            print(l, file=f)
                elif e == '.nnp':
                    self.verbose(f'Deconvert {fn}')
                    s, e, code = self.get_execute_output(f'cat {fn}')
                    converted = self.deconvert_nnp_content(s)
                    with open(fn_out, 'wb') as f:
                        f.write(converted)
                else:
                    self.copy_from_remote(fn, fn_out)
            for fn in (list_local - list_remote):
                if fn not in ['log.txt', 'result.ini']:
                    self.verbose(f"REMOVE {fn}")
                    os.unlink(os.path.join(self.outdir, fn))
