# Copyright 2018,2019,2020,2021 Sony Corporation.
# Copyright 2021,2022,2023,2024,2025 Sony Group Corporation.
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

import os
import argparse
import zipfile

import nnabla.utils.cli.convert
from nnabla.utils.cli.encode_decode_param import decode_param_command
from nnabla.logger import logger


def is_tf_v1_pb_file(pb_file_path):
    import tensorflow as tf
    try:
        with tf.io.gfile.GFile(pb_file_path, 'rb') as f:
            graph_def = tf.compat.v1.GraphDef()
            graph_def.ParseFromString(f.read())
        return True
    except Exception as e:
        return False


def import_file(args):
    ext = os.path.splitext(args.input)[1].lower()

    logger.log(99, 'Converting input file to .nnp file...')
    input_nnp = os.path.dirname(args.output) + os.sep + '~' + \
        os.path.splitext(os.path.basename(args.input))[0] + '.nnp'

    if ext == ".pb" and not is_tf_v1_pb_file(args.input):
        args.input = os.path.dirname(args.input)

    args.nnp_no_expand_network = True
    args.nnp_parameter_nntxt = False
    args.nnp_parameter_h5 = False
    args.nnp_exclude_parameter = False
    args.batch_size = 1
    args.export_format = 'NNP'
    args.settings = None
    args.split = None
    args.config = None
    args.nnp_import_executor_index = None
    args.default_variable_type = ['FLOAT32']
    args.import_format = 'NNP'
    args.outputs = None
    args.inputs = None
    args.nnp_version = None
    args.files = [args.input, input_nnp]

    try:
        nnabla.utils.cli.convert.convert_command(args)

        logger.log(99, 'Extracting files in nnp...')
        with zipfile.ZipFile(input_nnp) as zip:
            zip.extractall(args.output)

        class decode_args:
            pass
        decode_args.param = args.output + os.sep + 'parameter.protobuf'
        if not os.path.exists(decode_args.param):
            decode_args.param = args.output + os.sep + 'parameter.h5'
        decode_args.outdir = args.output + os.sep + 'net'
        decode_param_command(decode_args)
    except BaseException:
        try:
            if input_nnp != args.input:
                os.remove(input_nnp)
        except BaseException:
            pass
        raise

    logger.log(99, 'Import completed successfully.')
    if input_nnp != args.input:
        os.remove(input_nnp)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        '-i', '--input', help='path to .nnp or .onnx', required=True)
    parser.add_argument(
        '-o', '--output', help='output dir', required=True)
    parser.set_defaults(func=import_file)

    args = parser.parse_args()
    args.func(args)


if __name__ == '__main__':
    main()
