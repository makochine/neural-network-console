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

import nnabla.utils.converter
from nnabla.logger import logger


def export(args):
    if os.path.splitext(args.network)[1].lower() != ".nnp":
        tmp_file = os.path.dirname(args.output) + os.sep + '~' + \
            os.path.splitext(os.path.basename(args.output))[0] + '.nnp'
        nnp_file = tmp_file
    else:
        tmp_file = None
        nnp_file = args.network
    try:
        if tmp_file is not None:
            logger.log(99, 'Creating temporary nnp file...')
            with zipfile.ZipFile(tmp_file, 'w') as zip:
                zip.write(args.network, arcname='net.nntxt')
                zip.write(args.parameter, arcname='parameters.h5')

        args.nnp_no_expand_network = False
        args.nnp_parameter_nntxt = False
        args.nnp_parameter_h5 = False
        args.nnp_exclude_parameter = False
        args.batch_size = 1
        args.export_format = args.format
        args.settings = None
        args.nnp_import_executor_index = 0
        args.split = None
        args.config = None
        args.default_variable_type = ['FLOAT32']
        args.define_version = None
        args.import_format = 'NNP'
        args.api = 1
        args.enable_optimize_pb = None
        args.channel_last = False
        args.quantization = False
        args.dataset = None
        args.ir_version = -1
        args.nnp_version = None

        logger.log(99, 'Converting temporary .nnp file to ' +
                   os.path.splitext(args.output)[1] + ' file...')
        if not nnabla.utils.converter.convert_files(
                args, [nnp_file], args.output):
            return False

        if args.format == "NNB":
            logger.log(99, 'Exporting C source code...')
            dir = os.path.dirname(args.output) + os.sep + 'c_source'
            try:
                os.mkdir(dir)
            except BaseException:
                pass
            args.export_format = "CSRC"
            nnabla.utils.converter.convert_files(args, [nnp_file], dir)
    finally:
        if tmp_file is not None:
            os.remove(tmp_file)

    logger.log(99, 'Export completed successfully.')


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        '-n',
        '--network',
        help='path to net.nntxt or results.nnp',
        required=True)
    parser.add_argument(
        '-p', '--parameter', help='path to parameters.h5')
    parser.add_argument(
        '-o', '--output', help='output file name', required=True)
    parser.add_argument(
        '-f',
        '--format',
        help='output format. NNP, NNB, ONNX, TF_PB, TF_CKPT_V1 or TF_CKPT_V2',
        required=True)
    parser.set_defaults(func=export)

    args = parser.parse_args()
    args.func(args)


if __name__ == '__main__':
    main()
