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

import contextlib
import csv
import hashlib
import io
import json
import os
import platform
import re
import zipfile

from nnabla_ssh.utils import FileInfo


class RemoteConvert:

    ####################################################################################################################
    # public methods

    def csv_file_info(self, local_filename):
        """ローカルにあるCSVデータセットをリモートにコピーする時に必要となる情報を返します。

        返り値はリモート側のCSVファイルのファイル名、ローカルCSVファイルのmd5sum、
        行数、およびCSV内に含まれるファイルのリモートとローカルの対応辞書となります。
        """

        return (self._remote_csv_file_info(local_filename), self._remote_csv_dict_filename(local_filename))

    def convert_csv_file(self, local_csv_filename):
        """ローカルにあるCSVデータセットをリモートにコピーするために変換します。

        変換するだけでコピーは実施しません。

        """

        remote_csv_file_info = self._remote_csv_file_info(local_csv_filename)
        remote_csv_dict_filename = self._remote_csv_dict_filename(local_csv_filename)

        remote_data_directory = os.path.splitext(remote_csv_file_info.remote_filename)[0] + '/'
        remote_csv_name, _ = os.path.splitext(os.path.basename(remote_csv_file_info.remote_filename))

        filenames = {}
        items = {}
        file_info = {}
        local_csv_directory = os.path.dirname(os.path.abspath(local_csv_filename))

        with io.StringIO() as w:
            with open(local_csv_filename, "r", encoding="utf-8") as f:
                count = -1
                for row in csv.reader(f):
                    row_items = []
                    if count < 0:
                        print(','.join(row), file=w)
                    else:
                        new = []
                        for item in row:
                            new_item = item
                            if item in items:
                                new_item = filenames[items[item]]
                            else:
                                p1 = os.path.abspath(os.path.join(local_csv_directory, item))
                                if os.path.exists(p1):
                                    new_item = remote_data_directory + self._convert_item_filename(p1, item)
                                    items[item] = p1
                                    filenames[p1] = new_item
                                    file_info[p1] = FileInfo(new_item, None, None)
                                    row_items.append((p1, new_item))
                                else:
                                    p2 = os.path.abspath(item)
                                    if os.path.exists(p2):
                                        new_item = remote_data_directory + self._convert_item_filename(p2, item)
                                        items[item] = p2
                                        filenames[p2] = new_item
                                        file_info[p2] = FileInfo(new_item, None, None)
                                        row_items.append((p2, new_item))
                            new.append(new_item)
                        print(','.join(new), file=w)
                    yield row_items
                    count += 1
            w.seek(0)
            yield [(w, remote_csv_file_info.remote_filename)]

        # ローカルファイル名とリモートファイル名のリストを保存
        with io.StringIO() as w:
            json.dump(file_info, w)
            w.seek(0)
            yield [(w, remote_csv_dict_filename)]

        self.file_info.update(file_info)

    @contextlib.contextmanager
    def convert_config_file(self, local_config_filename):
        name, ext = os.path.splitext(os.path.basename(local_config_filename))
        ext = ext.lower()

        if ext == '.nntxt':
            with io.StringIO() as w:
                with open(local_config_filename) as f:
                    config_lines, csvs = self._convert_config_lines(f.readlines())
                    for l in config_lines:
                        print(l, file=w)
                w.seek(0)
                yield w, csvs
        elif ext == '.nnp':
            with io.BytesIO() as w:
                with zipfile.ZipFile(local_config_filename, 'r') as src_nnp:
                    with zipfile.ZipFile(w, 'w') as dst_nnp:
                        for n in src_nnp.namelist():
                            _, e = os.path.splitext(n)
                            e = e.lower()
                            if e == '.nntxt':
                                lines = src_nnp.read(n).decode().splitlines()
                                config_lines, csvs = self._convert_config_lines(lines)
                                with io.StringIO() as s:
                                    for l in config_lines:
                                        print(l, file=s)
                                    s.seek(0)
                                    dst_nnp.writestr(n, s.read())
                            else:
                                dst_nnp.writestr(n, src_nnp.read(n))
                w.seek(0)
                yield w, csvs
        else:
            self.error(f'Unsupported prefix of config file {local_config_filename}')

    ####################################################################################################################
    # internal methods

    def _convert_item_filename(self, name, origname):
        h = hashlib.md5(origname.encode('utf-8')).hexdigest()
        n, e = os.path.splitext(name)
        return f'{h}{e}'

    def _check_file(self, filename):
        md5 = hashlib.md5()
        num_of_lines = 0
        with open(filename, 'r') as f:
            for l in f.readlines():
                l = l.rstrip()
                md5.update(l.encode())
                num_of_lines += 1
        md5sum = md5.hexdigest()
        return md5sum, num_of_lines

    def _remote_csv_dict_filename(self, local_csv_filename):
        h = hashlib.md5(os.path.abspath(local_csv_filename).encode('utf-8')).hexdigest()
        return os.path.splitext(self.file_info[local_csv_filename].remote_filename)[0] + f'.{h}.json'

    def _remote_csv_file_info(self, local_filename):
        if local_filename in self.file_info:
            return self.file_info[local_filename]
        md5sum, num_of_lines = self._check_file(local_filename)
        name, ext = os.path.splitext(os.path.basename(local_filename))
        remote_filename = '/'.join([self.remote_base_directory, 'datasets', f'{name}_{md5sum}{ext}'])
        info = FileInfo(remote_filename, md5sum, num_of_lines)
        self.file_info[local_filename] = info
        return info

    def _convert_config_lines(self, lines):
        csvs = []
        converted = []
        in_global_config = False
        global_config = []
        last_csv_path = None
        for l in lines:
            l = l.rstrip()
            o = l
            if l.startswith('global_config'):
                in_global_config = True

            if l.startswith('}') and in_global_config:
                global_config.append(l)

                if 'CudaCachedArray' in global_config:
                    converted += global_config
                else:
                    self.verbose('Force using CUDA.')
                    converted.append('global_config {')
                    converted.append('  default_context {')
                    converted.append('    array_class: "CudaCachedArray"')
                    converted.append('    device_id: "0"')
                    converted.append('    backends: "cudnn:float"')
                    converted.append('    backends: "cuda:float"')
                    converted.append('    backends: "cpu:float"')
                    converted.append('  }')
                    converted.append('}')

                in_global_config = False
                continue

            if in_global_config:
                global_config.append(l)
                continue

            m = re.match(r'(^\s*)(uri|cache_dir):\s*(.*)$', l)
            if m:
                indent = m.group(1)
                datatype = m.group(2)
                value = m.group(3).replace('"', '').replace("'", '')
                if datatype == 'uri':
                    # uriをリモート側のPATHに変更
                    csv_path = value
                    csvs.append(csv_path)
                    new_csv_path = self._remote_csv_file_info(csv_path).remote_filename
                    last_csv_path = new_csv_path
                    l = f'{indent}{datatype}: "{new_csv_path}"'
                else:
                    # キャッシュはデータセットと同じ場所に決め
                    # 打ちで置く
                    cache = last_csv_path.replace('.csv', '.cache')
                    last_csv_path = None

                    self.file_info[value] = FileInfo(cache, None, None)

                    l = f'{indent}{datatype}: "{cache}"'
            converted.append(l)
        return converted, csvs

    def create_deconvert_dictionary(self):
        self._remote_files = {}
        for local_filename, info in self.file_info.items():
            self._remote_files[info.remote_filename] = local_filename

    def deconvert_csv_content(self, data):
        lines = []
        with io.StringIO(data.decode()) as s:
            for row in csv.reader(s):
                new = []
                for item in row:
                    if item in self._remote_files:
                        new.append(self._remote_files[item])
                    else:
                        new.append(item)
                lines.append(','.join(new))
        return lines

    def deconvert_nntxt_content(self, data):
        lines = []
        for l in data.decode().splitlines():
            l = l.rstrip()
            m = re.match(r'(^\s*)(uri|cache_dir):\s*(.*)$', l)
            if m:
                indent = m.group(1)
                datatype = m.group(2)
                value = m.group(3).replace('"', '').replace("'", '')
                if value in self._remote_files:
                    l = f'{indent}{datatype}: "{self._remote_files[value]}"'
            lines.append(l)
        return lines

    def deconvert_nnp_content(self, data):
        converted = None
        with io.BytesIO() as dst:
            with zipfile.ZipFile(dst, 'w') as dst_nnp:
                with io.BytesIO(data) as src:
                    with zipfile.ZipFile(src, 'r') as src_nnp:
                        for n in src_nnp.namelist():
                            _, e = os.path.splitext(n)
                            e = e.lower()
                            if e == '.nntxt':
                                config_lines = self.deconvert_nntxt_content(src_nnp.read(n))
                                with io.StringIO() as s:
                                    for l in config_lines:
                                        print(l.rstrip(), file=s)
                                    s.seek(0)
                                    dst_nnp.writestr(n, s.read())
                            else:
                                dst_nnp.writestr(n, src_nnp.read(n))
            dst.seek(0)
            converted = dst.read()
        return converted
