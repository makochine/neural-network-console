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

import sys
from nnabla.logger import logger

from nnabla_ssh.commands import parse_environment_variables
from nnabla_ssh.commands import call_remote_command


def get_callback_version():
    return 'NNabla SSH callback module.'


def alternative_cli(args):
    if 'func' in args:
        for name in ['forward', 'profile', 'train']:
            if args.func.__name__ == f'{name}_command':
                num, infos = parse_environment_variables(name)
                if num not in infos:
                    logger.log(99, f'[CALLBACK]: Exec {name} on local')
                    return None
                info = infos[num]
                if f'{name}_on_local' in info and info[f'{name}_on_local'] == 'True':
                    logger.log(99, f'[CALLBACK]: Exec {name} on local')
                    return None
                logger.log(99, f'[CALLBACK]: Exec {name} on [{info["name"]}]')
                ret = call_remote_command(info, name, args)
                if ret is None:
                    logger.log(99, f'[CALLBACK]: Exec {name} fallback to local')
                    return ret
                else:
                    logger.log(99, f'[CALLBACK]: {name} finished.')
                    sys.exit(0)
    return None
