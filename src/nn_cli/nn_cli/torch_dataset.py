# Copyright 2023,2024,2025 Sony Group Corporation.
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

import numpy
import contextlib
from torch.utils.data import Dataset
import os
import h5py
import csv
from six import BytesIO
from six import StringIO
import urllib.request as request
import tempfile
import scipy.io.wavfile
from shutil import rmtree
import binascii
from six.moves.urllib.parse import urljoin
from collections import OrderedDict
from nn_cli.image_utils import imresize, imread
from nn_cli.audio_utils import auresize, auread


class FileReader:
    '''FileReader

    Read dataset from several data sources.
    Supported data sources are,

    * Local file (file or directory name)
    * HTTP/HTTPS (URI)
    * S3         (URI with s3:// prefix)

    Currently HTTP/HTTPS source does not support CACHE input because
    there is no standard way to get directory entry with
    HTTP/HTTPS/protocol.


    To access S3 data, you must specify AWS access key credentials with environment
    variable.
    Or, you can specify AWS_DEFAULT_PROFILE for using the profile.
'''

    def __init__(self, base_uri):
        self._base_uri = base_uri
        self.base_ext = os.path.splitext(self._base_uri)[1].lower()
        if base_uri[0:5].lower() == 's3://':
            self._file_type = 's3'
            uri_header, uri_body = self._base_uri.split('://', 1)
            us = uri_body.split('/')
            bucketname = us.pop(0)
            self._s3_base_key = '/'.join(us)

            import boto3
            self._s3_bucket = boto3.session.Session().resource('s3').Bucket(bucketname)

        elif base_uri[0:7].lower() == 'http://' or base_uri[0:8].lower() == 'https://':
            self._file_type = 'http'
        else:
            self._file_type = 'file'

    def read_s3_object(self, key):
        retry = 1
        result = ''
        while True:
            if retry > 10:
                raise
            try:
                result = self._s3_bucket.Object(key).get()['Body'].read()
                break
            except:
                retry += 1

        return result

    @contextlib.contextmanager
    def open(self, filename=None, textmode=False, encoding='utf-8-sig'):
        if filename is None:
            filename = self._base_uri
        else:
            if self._file_type == 's3':
                filename = urljoin(self._base_uri.replace(
                    's3://', 'http://'), filename.replace('\\', '/')).replace('http://', 's3://')
            elif self._file_type == 'http':
                filename = urljoin(self._base_uri, filename.replace('\\', '/'))
            else:
                filename = os.path.abspath(os.path.join(os.path.dirname(
                    self._base_uri.replace('\\', '/')), filename.replace('\\', '/')))
        f = None
        if self._file_type == 's3':
            uri_header, uri_body = filename.split('://', 1)
            us = uri_body.split('/')
            bucketname = us.pop(0)
            key = '/'.join(us)
            if textmode:
                f = StringIO(self.read_s3_object(key).decode(encoding))
            else:
                f = BytesIO(self.read_s3_object(key))
        elif self._file_type == 'http':
            f = request.urlopen(filename)
        else:
            if textmode:
                f = open(filename, 'rt', encoding=encoding)
            else:
                f = open(filename, 'rb')
        f.ext = os.path.splitext(filename)[1].lower()
        yield f
        f.close()

    @contextlib.contextmanager
    def open_cache(self, cache_name):
        if self._file_type == 's3':
            tmpdir = tempfile.mkdtemp()
            filename = urljoin((self._base_uri + '/').replace('s3://', 'http://'),
                               cache_name.replace('\\', '/')).replace('http://', 's3://')
            key = '/'.join(filename.split('/')[3:])
            fn = '{}/{}'.format(tmpdir, os.path.basename(filename))
            with open(fn, 'wb') as f:
                f.write(self.read_s3_object(key))
            with h5py.File(fn, 'r') as h5:
                yield h5
            rmtree(tmpdir, ignore_errors=True)
        elif self._file_type == 'http':
            pass
        else:
            filename = os.path.abspath(os.path.join(os.path.dirname(
                (self._base_uri + '/').replace('\\', '/')), cache_name.replace('\\', '/')))
            with h5py.File(filename, 'r') as h5:
                yield h5

    def listdir(self):
        if self._file_type == 's3':
            list = []
            for fn in self._s3_bucket.objects.filter(Prefix=self._s3_base_key + '/', Delimiter='/'):
                list.append(os.path.basename(fn.key))
            return sorted(list)
        elif self._file_type == 'http':
            return None
        return [f for f in sorted(os.listdir(self._base_uri)) if os.path.splitext(f)[1].lower() == ".h5"]


class CSVDataset(Dataset):
    def _remove_comment_cols(self, header, rows):
        for col_index in reversed(range(len(header))):
            if header[col_index][0] == '#':
                del header[col_index]
                for row in rows:
                    del row[col_index]

    def _process_header(self, row):
        self._variables_dict = OrderedDict()
        self._columns = []
        for column, column_value in enumerate(row):

            # Analyze header "NAME[__INDEX][:LABELNAME]"
            # TODO: use regex instead of split....
            try:
                variable_with_index, label = column_value.split(':', 1)
            except:
                label = None
                variable_with_index = column_value
            try:
                variable, index = variable_with_index.split('__', 1)
            except:
                variable = variable_with_index
                index = None

            self._columns.append((variable, index, label))
            if index is None:
                self._variables_dict[variable] = {
                    'label': label, 'value': None}
            else:
                if variable not in self._variables_dict:
                    self._variables_dict[variable] = []
                self._variables_dict[variable].append(
                    {'label': label, 'value': None})

    def _process_row(self, row):
        values = OrderedDict()
        if len(row) == len(self._columns):
            for column, column_value in enumerate(row):
                variable, index, label = self._columns[column]
                if index is None:
                    values[variable] = self._get_value(
                        column_value, is_vector=False)
                else:
                    if variable not in values:
                        values[variable] = []
                    values[variable].append(self._get_value(column_value))
        for key, value in values.items():
            values[key] = numpy.array(value).astype(numpy.float32)
        return values.values()

    def _get_value(self, value, is_vector=False):
        try:
            if is_vector:
                value = numpy.array([float(value)])
            else:
                value = float(value)
            return value
        except ValueError:
            pass
        ext = (os.path.splitext(value)[1]).lower()
        with self._filereader.open(value) as f:
            value = load(ext)(f, normalize=self._normalize)
        return value

    def _get_data(self, position):
        return tuple(self._process_row(self._rows[position]))

    def __init__(self, filename, normalize=False):
        super(CSVDataset, self).__init__()
        self._filename = filename
        self._normalize = normalize

        # Store contents of CSV file into the self._rows list.
        self._generation = -1
        self._rows = []
        self._filereader = FileReader(self._filename)
        with self._filereader.open(textmode=True, encoding='utf-8-sig') as f:
            csvreader = csv.reader(f)
            header = next(csvreader)
            self._rows = list(csvreader)
            self._size = len(self._rows)
            self._remove_comment_cols(header, self._rows)
            self._process_header(header)
        self._original_source_uri = self._filename
        self._variables = tuple(self._variables_dict.keys())

    def __len__(self):
        return self._size

    def __getitem__(self, idx):
        columns = self._variables
        data = self._get_data(idx)
        result = {k: v for k, v in zip(columns, data)}
        return result


def get_file_extension(source):
    ext = ''
    file_signature = {
        '.bmp': (['424d'], 0x0),
        '.dib': (['424d'], 0x0),
        '.pgm': (['50350a'], 0x0),
        '.jpeg': (['ffd8ff'], 0x0),
        '.jpg': (['ffd8ff'], 0x0),
        '.png': (['89504e470d0a1a0a'], 0x0),
        '.tif': (['492049'], 0x0),
        '.tiff': (['492049'], 0x0),
        '.eps': (['c5d0d3c6'], 0x0),
        '.gif': (['474946383761', '474946383961'], 0x0),
        '.ico': (['00000100'], 0x0),
        '.dcm': (['4449434d'], 0x80),
        '.wav': (['52494646'], 0x0),
    }
    if hasattr(source, "read"):
        if hasattr(source, "name"):
            ext = os.path.splitext(source.name)[1].lower()
        else:
            for extension, (signature, offset) in file_signature.items():
                source.seek(offset)
                data = binascii.hexlify(source.read()).decode('utf-8')
                source.seek(0)
                for s in signature:
                    if data.startswith(s):
                        ext = extension
    elif isinstance(source, str):
        ext = os.path.splitext(source)[1].lower()
    return ext


def load_image_imread(file, shape=None, max_range=1.0):
    '''
    Load image from file like object.

    :param file: Image contents
    :type file: file like object.
    :param shape: shape of output array
        e.g. (3, 128, 192) : n_color, height, width.
    :type shape: tuple of int
    :param float max_range: the value of return array ranges from 0 to `max_range`.

    :return: numpy array

    '''
    orig_img = imread(
        file)  # return value is from zero to 255 (even if the image has 16-bitdepth.)

    if len(orig_img.shape) == 2:  # gray image
        height, width = orig_img.shape
        if shape is None:
            out_height, out_width, out_n_color = height, width, 1
        else:
            out_n_color, out_height, out_width = shape
        assert (out_n_color == 1)
        if out_height != height or out_width != width:
            # imresize returns 0 to 255 image.
            orig_img = imresize(orig_img, (out_height, out_width))
        orig_img = orig_img.reshape((out_n_color, out_height, out_width))
    elif len(orig_img.shape) == 3:  # RGB image
        height, width, n_color = orig_img.shape
        if shape is None:
            out_height, out_width, out_n_color = height, width, n_color
        else:
            out_n_color, out_height, out_width = shape
        assert (out_n_color == n_color)
        if out_height != height or out_width != width or out_n_color != n_color:
            # imresize returns 0 to 255 image.
            orig_img = imresize(orig_img, (out_height, out_width, out_n_color))
        orig_img = orig_img.transpose(2, 0, 1)

    if max_range < 0:
        return orig_img
    else:
        # 16bit depth
        if orig_img.dtype == 'uint16':
            if max_range == 65535.0:
                return orig_img
            return (orig_img * (max_range / 65535.0)).astype(numpy.float32)
        # 8bit depth (default)
        else:
            if max_range == 255.0:
                return orig_img
            return (orig_img * (max_range / 255.0)).astype(numpy.float32)


def load_image(file, shape=None, normalize=False):
    if normalize:
        max_range = 1.0
    else:
        max_range = -1

    return load_image_imread(file, shape, max_range)


def load_csv(file, shape=None, normalize=False):
    """
    Load CSV file.

    :param file: CSV file.
    :type file: file like object
    :param shape : data array is reshape to this shape.
    :type shape: tuple of int

    :return: numpy array
    """
    value_list = []
    for row in csv.reader([l.decode('utf-8') for l in file.readlines()]):
        if len(row):
            value_list.append(list(map(float, row)))
    try:
        if shape is None:
            return numpy.array(value_list)
        else:
            return numpy.array(value_list).reshape(shape)
    except:
        raise


def load_npy(path, shape=None, normalize=False):
    if shape is None:
        return numpy.load(path, allow_pickle=True)
    else:
        return numpy.load(path, allow_pickle=True).reshape(shape)


def load_wav(path, shape=None, dtype=numpy.float32, normalize=False):
    wav = scipy.io.wavfile.read(path)[1]
    if shape is None:
        if len(wav.shape) == 1:
            return wav.reshape(-1, 1).astype(dtype)
        else:
            return wav.astype(dtype)
    else:
        return wav.reshape(shape).astype(dtype)


def load_audio_pydub(path, shape=None, normalize=False):
    if shape:
        return auresize(auread(path), shape)
    return auread(path)


def load_audio(file, shape=None, dtype=numpy.float32, normalize=False):
    return load_wav(file, shape, dtype, normalize)


_load_functions = {
    '.bmp': load_image,
    '.jpg': load_image,
    '.jpeg': load_image,
    '.png': load_image,
    '.gif': load_image,
    '.tif': load_image,
    '.tiff': load_image,
    '.dcm': load_image,
    '.csv': load_csv,
    '.npy': load_npy,
    '.wav': load_audio}


def load(ext):
    if ext in _load_functions:
        return _load_functions[ext]
    raise ValueError(
        'File format with extension "{}" is not supported.'.format(ext))


def _download_hook(t):
    last_b = [0]

    def inner(b=1, bsize=1, tsize=None):
        if tsize is not None:
            t.total = tsize
        t.update((b - last_b[0]) * bsize)
        last_b[0] = b

    return inner
