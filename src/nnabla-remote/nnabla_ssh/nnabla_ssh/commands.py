# Copyright 2020,2021 Sony Corporation.
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

import io
import os
import re
import sys


def parse_environment_variables(command):
    connector_info = {}
    processor_name = command.upper()
    if processor_name == 'PROFILE':
        processor_name = 'TRAIN'
    num = os.getenv(f'{command.upper()}_PROCESSOR_INDEX')
    if num:
        num = int(num)
    else:
        num = -1

    for name, value in os.environ.items():
        m = re.match(r'^REMOTE_PROCESSOR_([1-9])_([A-Z][0-9A-Z_]*[0-9A-Z])', name)
        if m:
            n = int(m.group(1))
            name = m.group(2).lower()
            if n not in connector_info:
                connector_info[n] = {}
                connector_info[n]['num'] = n

            connector_info[n][name] = value

    return num, connector_info


def call_remote_command(info, cmdname, args):
    import importlib
    import glob
    from os.path import basename, join, dirname, abspath, splitext

    connectors = [splitext(basename(x))[0] for x in glob.glob(
        join(dirname(abspath(__file__)), 'remote', '[A-Za-z]*.py'))]

    for n in connectors:
        print(f'[COMMAND]: Connector type [{n.upper()}] is available.', file=sys.stderr, flush=True)

    ret = None

    n = info['type'].lower()
    if n in connectors:
        connector = importlib.import_module(f'nnabla_ssh.remote.{n}')

        with connector.RemoteConnector(info) as c:
            ret = c.exec_cli_command(cmdname, args)

    return ret
