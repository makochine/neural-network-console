# Copyright 2017,2018,2019,2020,2021 Sony Corporation.
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

import argparse
import os
import sys
import warnings
import nn_cli.commands as commands
warnings.filterwarnings("ignore", message="numpy.dtype size changed")

nnabla_enabled = True
torch_enabled = True


def _nncli_version():
    _version = '0.0.1'
    _build_number = '1'
    print(f"Version:{_version}, Build:{_build_number}")


def version_command(args):
    print(_nncli_version())


def main():
    global nnabla_enabled, torch_enabled

    parser = argparse.ArgumentParser(description='nnabla_cli is a command-line interface wrapper nnabla and pytorch backend')
    parser.add_argument(
        '-m', '--mpi', help='exec with mpi.', action='store_true')

    subparsers = parser.add_subparsers()

    commands.install_command_processor(subparsers)

    # Version
    subparser = subparsers.add_parser(
        'version', help='Print version and build number.')
    subparser.set_defaults(func=version_command)


    args = parser.parse_args()
    if nnabla_enabled:
        import nnabla.utils.callback as callback
        r = callback.alternative_cli(args)
        if r is not None:
            return r

    global return_value
    import six.moves._thread as thread
    import threading
    thread.stack_size(8 * 1024 * 1024)
    sys.setrecursionlimit(1024 * 1024)
    main_thread = threading.Thread(target=cli_main, args=(parser, args))
    main_thread.start()
    main_thread.join()
    if not return_value:
        sys.exit(-1)


def cli_main(parser, args):
    global return_value
    return_value = False

    if 'func' not in args:
        parser.print_help(sys.stderr)
        sys.exit(-1)

    try:
        return_value = args.func(args)  # TODO: Need to create MPI comm
    except:
        import traceback
        print(traceback.format_exc())

        print("ABORTED")
        if args.mpi:
            os.kill(os.getpid(), 9)
        else:
            return_value = False
            sys.exit(-1)


if __name__ == '__main__':
    main()
