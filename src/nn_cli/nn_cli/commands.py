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

import argparse
import os
import sys
import warnings
import nn_cli.env as env

warnings.filterwarnings("ignore", message="numpy.dtype size changed")


def train_command(args):
    backend = env.select_backend_by_config_file(args.config)
    return backend.train_command(args)


def infer_command(args):
    backend = env.select_backend_by_config_file(args.config)
    return backend.infer_command(args)


def forward_command(args):
    backend = env.select_backend_by_config_file(args.config)
    return backend.forward_command(args)


def encode_param_command(args):
    backend = env.select_backend(args.env)
    return backend.encode_param_command(args)


def decode_param_command(args):
    backend = env.select_backend(args.env)
    return backend.decode_param_command(args)


def profile_command(args):
    backend = env.select_backend_by_config_file(args.config)
    return backend.profile_command(args)


def conv_dataset_command(args):
    backend = env.select_backend('NNabla')
    return backend.conv_dataset_command(args)


def compare_with_cpu_command(args):
    backend = env.select_backend('NNabla')
    return backend.compare_with_cpu_command(args)


def create_image_classification_dataset_command(args):
    backend = env.select_backend('NNabla')
    return backend.create_image_classification_dataset_command(args)


def upload_command(args):
    backend = env.select_backend_by_config_file(args.config)
    return backend.upload_command(args)


def create_tar_command(args):
    backend = env.select_backend('NNabla')
    return backend.create_tar_command(args)


def function_info_command(args):
    backend = env.select_backend('NNabla')
    return backend.function_info_command(args)


def optimization_command(args):
    backend = env.select_backend('NNabla')
    return backend.optimization_command(args)


def dump_command(args):
    backend = env.select_backend('NNabla')
    return backend.dump_command(args)


def nnb_template_command(args):
    backend = env.select_backend('NNabla')
    return backend.nnb_template_command(args)


def plot_series_command(args):
    backend = env.select_backend('NNabla')
    return backend.plot_series_command(args)


def plot_timer_command(args):
    backend = env.select_backend('NNabla')
    return backend.plot_timer_command(args)


def draw_graph_command(args):
    backend = env.select_backend('NNabla')
    return backend.draw_graph_command(args)

def convert_command(args):
    backend = env.select_backend('Torch')
    return backend.convert_command(args)

def add_train_command(subparsers):
    subparser = subparsers.add_parser('train', help='Training with NNP or PTZ.')
    subparser.add_argument(
        '-r', '--resume', help='Resume from last saved parameter', action='store_true')
    subparser.add_argument(
        '-c', '--config', help='Path to nntxt, sdcproj or ptz', required=True)
    subparser.add_argument(
        '-p', '--param', help='Path to parameter file, .h5 or .pt', required=False)
    subparser.add_argument(
        '-o', '--outdir', help='Output directory', required=True)
    subparser.add_argument(
        '-O', '--enable-ooc', help='Enable Out Of Core training', action='store_true')
    subparser.add_argument(
        '-m', '--ooc-gpu-memory-size', help='OOC gpu memory size (INTEGER or NUMeNUM or NUM[KkMmGgTtPp])', default=None)
    subparser.add_argument(
        '-C', '--context', help='Force exec context (cpu or cudnn[:DEVID])', default=None)
    subparser.add_argument(
        '-w', '--ooc-window-length', help='OOC window length (INTEGER or NUMeNUM or NUM[KkMmGgTtPp])', default=None)
    if env.nnabla_enabled:
        import nnabla.utils.callback as callback
        callback.add_train_command_arg(subparser)
    subparser.set_defaults(func=train_command)


def add_infer_command(subparsers):
    # Infer
    subparser = subparsers.add_parser(
        'infer', help='Do inference with NNP and binary data file input.')
    subparser.add_argument(
        '-c', '--config', help='path to nntxt', required=True)
    subparser.add_argument(
        '-o', '--output', help='output file prefix', required=False)
    subparser.add_argument(
        '-p', '--param', help='path to parameter file', required=False)
    subparser.add_argument(
        '-t', '--data_type', help='Parameter type (uint8, int32 or float32)', default='float32', required=False)
    subparser.add_argument(
        '-b', '--batch_size',
        help='Batch size to use batch size in nnp file set -1.',
        type=int, default=1)
    subparser.add_argument('inputs', nargs='+')
    subparser.set_defaults(func=infer_command)


def add_forward_command(subparsers):
    # Forward
    subparser = subparsers.add_parser(
        'forward', help='Do evaluation with NNP and test dataset.')
    subparser.add_argument(
        '-c', '--config', help='path to nntxt', required=True)
    subparser.add_argument(
        '-p', '--param', help='path to parameter file', required=False)
    subparser.add_argument(
        '-d', '--dataset', help='path to CSV dataset', required=False)
    subparser.add_argument(
        '-o', '--outdir', help='output directory', required=True)
    subparser.add_argument(
        '-f', '--outfile', help='output file name', default='output_result.csv')
    subparser.add_argument(
        '--replace_path', help='replace data path in the dataset with absolute path', action='store_true')
    subparser.add_argument(
        '--result_outdir', help='output result directory', type=str, default='')
    subparser.add_argument(
        '-C', '--context', help='Force exec context (cpu or cudnn[:DEVID])', default="cpu")
    subparser.add_argument(
        '-b', '--batch_size',
        help='Batch size to use batch size in nnp file set -1.',
        type=int, default=-1)
    subparser.set_defaults(func=forward_command)


def add_decode_param_command(subparsers):
    # Decode param
    subparser = subparsers.add_parser(
        'decode_param', help='Decode parameter to plain text.')
    subparser.add_argument(
        '-p', '--param', help='path to parameter file', required=False)
    subparser.add_argument(
        '-o', '--outdir', help='output directory', required=True)
    subparser.add_argument(
        '-e', '--env', help='Specify the backend environment (torch or nnabla).', required=True)
    subparser.set_defaults(func=decode_param_command)


def add_encode_param_command(subparsers):
    # Encode param
    subparser = subparsers.add_parser(
        'encode_param', help='Encode plain text to parameter format.')
    subparser.add_argument(
        '-i', '--indir', help='input directory', required=True)
    subparser.add_argument(
        '-p', '--param', help='path to parameter file', required=False)
    subparser.set_defaults(func=encode_param_command)
    subparser.add_argument(
        '-e', '--env', help='Specify the backend environment (torch or nnabla).', required=True)
    subparser.set_defaults(func=encode_param_command)


def add_profile_command(subparsers):
    # Profile
    subparser = subparsers.add_parser(
        'profile', help='Profiling performance with NNP.')
    subparser.add_argument(
        '-c', '--config', help='path to nntxt', required=True)
    subparser.add_argument(
        '-o', '--outdir', help='output directory', required=True)
    subparser.set_defaults(func=profile_command)


def add_conv_dataset_command(subparsers):
    # Convert dataset
    subparser = subparsers.add_parser(
        'conv_dataset', help='Convert CSV dataset to cache.')
    subparser.add_argument('-F', '--force', action='store_true',
                           help='force overwrite destination', required=False)
    subparser.add_argument(
        '-S', '--shuffle', action='store_true', help='shuffle data', required=False)
    subparser.add_argument('-N', '--normalize', action='store_true',
                           help='normalize data range', required=False)
    subparser.add_argument('-t', "--num_of_threads", type=int, required=False,
                           help='use multithreading to convert cache, default to 10')
    subparser.add_argument('source')
    subparser.add_argument('destination')
    subparser.set_defaults(func=conv_dataset_command)


def add_create_image_classification_dataset_command(subparsers):
    # Create image classification dataset
    subparser = subparsers.add_parser('create_image_classification_dataset',
                                      help='Create dataset from image files.')
    subparser.add_argument(
        '-i', '--sourcedir', help='source directory with directories for each class', required=True)
    subparser.add_argument(
        '-o', '--outdir', help='output directory', required=True)
    subparser.add_argument(
        '-c', '--channel', help='number of output color channels', required=True)
    subparser.add_argument(
        '-w', '--width', help='width of output image', required=True)
    subparser.add_argument(
        '-g', '--height', help='height of output image', required=True)
    subparser.add_argument(
        '-m', '--mode', help='shaping mode (trimming or padding)', required=True)
    subparser.add_argument(
        '-s', '--shuffle', help='shuffle mode (true or false)', required=True)
    subparser.add_argument(
        '-f1', '--file1', help='output file name 1', required=True)
    subparser.add_argument(
        '-r1', '--ratio1', help='output file ratio(%%) 1')
    subparser.add_argument(
        '-f2', '--file2', help='output file name 2')
    subparser.add_argument(
        '-r2', '--ratio2', help='output file ratio(%%) 2')
    subparser.set_defaults(func=create_image_classification_dataset_command)


def add_upload_command(subparsers):
    # Uploader
    subparser = subparsers.add_parser(
        'upload', help='Upload dataset to Neural Network Console.')
    subparser.add_argument(
        '-e', '--endpoint', help='set endpoint uri', type=str)
    subparser.add_argument('token', help='token for upload')
    subparser.add_argument('filename', help='filename to upload')
    subparser.set_defaults(func=upload_command)


def add_create_tar_command(subparsers):
    # Create TAR for uploader
    subparser = subparsers.add_parser(
        'create_tar', help='Create tar file for Neural Network Console.')
    subparser.add_argument('source', help='CSV dataset')
    subparser.add_argument('destination', help='TAR filename')
    subparser.set_defaults(func=create_tar_command)


def add_function_info_command(subparsers):
    # Function Info
    subparser = subparsers.add_parser(
        'function_info', help='Output function info.')
    subparser.add_argument('-o', '--output', default=None,
                           help='output filename')
    subparser.add_argument('-f', '--all_support', default=None,
                           help='select function set: NNB, ONNX')
    subparser.add_argument('files', metavar='FILE', type=str, nargs='*',
                           help='*.nnp files that the function sets want to be shown')
    subparser.add_argument('-c', '--config', default=None,
                           help='user config file for target constraint')
    subparser.add_argument('-x', '--exclude', action='append', default=[],
                           help='exclude specified functions')
    subparser.add_argument('-t', '--target', action='store_true',
                           help='specify function set is output for target format/device')
    subparser.add_argument('-q', '--query', default=None,
                           help='query the detail of a function')
    subparser.add_argument('--nnp-no-expand-network', action='store_true',
                           help='[import][NNP] expand network with repeat or recurrent.')
    subparser.add_argument('--api', type=int, default=0,
                           help='List up api levels')
    subparser.set_defaults(func=function_info_command)


def add_import_arg(parser):
    parser.add_argument('-I', '--import-format', type=str, default='NNP',
                        help='[import] import format.')
    parser.add_argument('-E', '--nnp-import-executor-index', type=int, default=None,
                        help='[import][NNP] import only specified executor.')
    parser.add_argument('--nnp-exclude-preprocess', action='store_true',
                        help='[import][NNP] EXPERIMENTAL exclude preprocess functions when import.')
    parser.add_argument('--nnp-no-expand-network', action='store_true',
                        help='[import][NNP] expand network with repeat or recurrent.')


def add_dump_command(subparsers):
    # Dump Network.
    subparser = subparsers.add_parser(
        'dump', help='Dump network with supported format.')
    subparser.add_argument('-v', '--dump-verbose', action='store_true',
                           help='[dump] verbose output.')
    subparser.add_argument('-F', '--dump-functions', action='store_true',
                           help='[dump] dump function list.')
    subparser.add_argument('-V', '--dump-variables', action='store_true',
                           help='[dump] dump variable list.')
    subparser.add_argument('--dump-limit', type=int, default=-1,
                           help='[dump] limit num of items.')
    subparser.add_argument('-n', '--dump-variable-name', type=str, default=None,
                           help='[dump] Specific variable name to display.')
    subparser.add_argument('files', metavar='FILE', type=str, nargs='+',
                           help='File or directory name(s) to convert.')
    # import option
    add_import_arg(subparser)
    subparser.set_defaults(func=dump_command)


def add_nnb_template_command(subparsers):
    ################################################################################
    # Generate NNB template
    subparser = subparsers.add_parser(
        'nnb_template', help='Generate NNB config file template.')
    subparser.add_argument('files', metavar='FILE', type=str, nargs='+',
                           help='File to generate NNB config file template. The last is setting yaml file.')
    # import option
    add_import_arg(subparser)

    subparser.add_argument('-b', '--batch-size', type=int, default=-1,
                           help='[export] overwrite batch size.')
    subparser.add_argument('-T', '--default-variable-type', type=str, nargs=1, default=['FLOAT32'],
                           help='Default type of variable')
    subparser.add_argument('--api', type=int, default=-1,
                           help='Set API Level to convert to, default is highest API Level.')

    subparser.set_defaults(func=nnb_template_command)


def add_optimize_command(subparsers):
    ################################################################################
    # Optimize pb model
    subparser = subparsers.add_parser(
        'optimize', help='Optimize pb model.')
    subparser.add_argument('input_file', nargs=1,
                           help='Input pre-optimized pb model.')
    subparser.add_argument('output_file', nargs=1,
                           help='Output optimized pb model.')
    subparser.set_defaults(func=optimization_command)


def add_plot_series_command(subparsers):
    desc = '''Plot *.series.txt files produced by nnabla.MonitorSeries class.

Example:

    nnabla_cli plot_series -x "Epochs" -y "Squared error loss" -T 10 -l "config A" -l "config B" result_a/Training-loss.series.txt result_b/Training-loss.series.txt'''

    add_plot_any_command(
        subparsers, 'plot_series', 'Plot *.series.txt files.', desc, plot_series_command)


def add_plot_any_command(subparsers, subcommand, help, description, plot_command, ylabel_default=''):
    from argparse import RawTextHelpFormatter
    # Train
    subparser = subparsers.add_parser(
        subcommand,
        formatter_class=RawTextHelpFormatter,
        description=description,
        help=help)

    subparser.add_argument('inputs', type=str, nargs='+',
                           metavar='infile', help='Path to input file.')
    subparser.add_argument(
        '-l', '--label', action='append', help='Label of each plot.')
    subparser.add_argument(
        '-o', '--outfile', help='Path to output file.', type=str, default='')
    subparser.add_argument(
        '-x', '--xlabel', help='X-axis label of plot.', default='')
    subparser.add_argument(
        '-y', '--ylabel', help='Y-axis label of plot.', default=ylabel_default)
    subparser.add_argument('-t', '--title', help='Title of plot.', default='')
    subparser.add_argument(
        '-T', '--ylim-max', help='Y-axis plot range max.', type=float, default=None)
    subparser.add_argument(
        '-B', '--ylim-min', help='Y-axis plot range min.', type=float, default=None)
    subparser.add_argument(
        '-R', '--xlim-max', help='X-axis plot range max.', type=float, default=None)
    subparser.add_argument(
        '-L', '--xlim-min', help='X-axis plot range min.', type=float, default=None)

    subparser.set_defaults(func=plot_command)
    return subparser


def add_plot_timer_command(subparsers):
    desc = '''Plot *.timer.txt files produced by nnabla.MonitorTimeElapsed class.

Example:

    nnabla_cli plot_timer -x "Epochs" -l "config A" -l "config B" result_a/Epoch-time.timer.txt result_b/Epoch-time.timer.txt'''

    subparser = add_plot_any_command(
        subparsers, 'plot_timer', 'Plot *.timer.txt files.', desc, plot_timer_command)
    subparser.add_argument(
        '-e', '--elapsed', help='Plot total elapsed time. By default, it plots elapsed time per iteration.',
        action='store_true')
    subparser.add_argument(
        '-u', '--time-unit', help='Time unit chosen from {s|m|h|d}.', default='s')


def add_draw_graph_command(subparsers):
    desc = '''Draw a graph in a NNP or nntxt file with graphviz.

Example:

    nnabla_cli draw_graph -o output-folder path-to-nnp.nnp'''

    from argparse import RawTextHelpFormatter
    # Train
    subparser = subparsers.add_parser(
        'draw_graph',
        formatter_class=RawTextHelpFormatter,
        description=desc,
        help='Draw a graph in a NNP or nntxt file with graphviz.')
    subparser.add_argument('input', type=str,
                           help='Path to input nnp or nntxt.')
    subparser.add_argument(
        '-o', '--output-dir', type=str, default='draw_out', help='Output directory.')
    subparser.add_argument('-n', '--network', action='append',
                           help='Network names to be drawn.')
    subparser.add_argument('-f', '--format', default='png',
                           help='Graph saving format compatible with graphviz (`pdf`, `png`, ...).')
    subparser.set_defaults(func=draw_graph_command)
    return subparser


def add_compare_with_cpu_command(subparsers):
    # Compare with CPU
    subparser = subparsers.add_parser(
        'compare_with_cpu', help='Compare performance between two nntxt.')
    subparser.add_argument(
        '-c', '--config', help='path to nntxt', required=True)
    subparser.add_argument(
        '-c2', '--config2', help='path to cpu nntxt', required=True)
    subparser.add_argument(
        '-o', '--outdir', help='output directory', required=True)
    subparser.set_defaults(func=compare_with_cpu_command)


def add_convert_command(subparsers):
    # Converter
    subparser = subparsers.add_parser('convert', help='File format converter.')
    # import option
    subparser.add_argument('input', type=str, default=None,
                           help='Path to input file')
    subparser.add_argument('output', help='Path to input file')
    subparser.set_defaults(func=convert_command)


def install_command_processor(subparsers):
    add_train_command(subparsers)
    add_infer_command(subparsers)
    add_forward_command(subparsers)
    add_encode_param_command(subparsers)
    add_decode_param_command(subparsers)
    add_profile_command(subparsers)
    add_conv_dataset_command(subparsers)
    add_create_image_classification_dataset_command(subparsers)
    add_upload_command(subparsers)
    add_create_tar_command(subparsers)
    add_function_info_command(subparsers)
    add_optimize_command(subparsers)
    add_dump_command(subparsers)
    add_nnb_template_command(subparsers)
    add_convert_command(subparsers)
    add_plot_series_command(subparsers)
    add_plot_timer_command(subparsers)
    add_draw_graph_command(subparsers)
    add_compare_with_cpu_command(subparsers)
