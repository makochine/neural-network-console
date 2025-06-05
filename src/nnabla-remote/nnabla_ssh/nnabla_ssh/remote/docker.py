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

import datetime
import time
import paramiko
import scp
import sys

from nnabla_ssh.utils import remote


class RemoteConnector(remote.Remote):
    """Dockerが使えるホストにSSHで接続して学習を実施する
    """

    def __init__(self, info):
        """Initialize
        """
        super(RemoteConnector, self).__init__()

        self._type = 'DOCKER'
        self._verbose = True  # make it true for debugging.

        self._docker_image = 'nnabla/nnabla-ext-cuda-multi-gpu:py38-cuda110-mpi3.1.6-v1.32.0'
        if 'image' in info and info['image'] is not None:
            self._docker_image = info['image']
        self._hostname = info['host']
        self.remote_base_directory = info['basedir']
        self._user = info['user']
        self._key = self.from_private_key(info['key'])
        self._gpus = 'all'
        self._gpu_list = []
        if 'gpus' in info and info['gpus'] is not None:
            self._gpu_list = [int(x) for x in info['gpus'].split(',')]
            self._gpus = f"\'\"device={info['gpus']}\"\'"

    def __enter__(self):
        """Connect to SSH
        """
        self.connect()
        return self

    def __exit__(self, execption_type, exception_value, traceback):
        """Close connection
        """
        self.disconnect()

    def connect(self):
        self.ssh = paramiko.SSHClient()
        self.ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
        self.ssh.connect(hostname=self._hostname, port=22,
                         username=self._user, pkey=self._key)
        self.update_pathnames()
        self.scp = scp.SCPClient(self.ssh.get_transport(), progress=self.scp_progress)

    def disconnect(self):
        self.ssh.close()

    def start_nnabla_cli(self, command, args):
        self.execute(f'docker pull {self._docker_image}')
        self.dockername = f'nnabla-ssh-' + datetime.datetime.now().strftime('%Y%m%d-%H%M%S')

        std, err, code = self.get_execute_output('nvidia-smi --query-gpu=index,name --format=csv ')
        gpu_names = {}
        if code == 0:
            for l in std.decode().splitlines()[1:]:
                index, name = [i.strip() for i in l.split(',')]
                gpu_names[int(index)] = name

        if self._gpus == 'all':
            self._gpu_list = sorted(gpu_names.keys())
        self._num_gpu = len(self._gpu_list)
        self.verbose(f'Using {self._num_gpu}GPU(s)')
        for i in self._gpu_list:
            self.verbose(f'     GPU{i} ({gpu_names[i]})')

        cmdline = f'docker run -d --gpus {self._gpus} -u $(id -u):$(id -g)'
        cmdline += f' --name {self.dockername}'
        cmdline += f' -v {self.remote_base_directory}:{self.remote_base_directory}'
        cmdline += f' -w {self.remote_base_directory}'
        cmdline += f' {self._docker_image}'

        if command == 'forward':
            cmdline += f' nnabla_cli '
            cmdline += command
            cmdline += self.parse_forward_options(args)

        elif command == 'train' or command == 'profile':
            if self._num_gpu == 1:
                cmdline += f' nnabla_cli '
            else:
                cmdline += f' mpiexec -q -n {self._num_gpu} nnabla_cli -m '

            cmdline += command
            cmdline += self.parse_train_options(args)

        else:
            return None

        cmdline += f' -c {args.config}'
        cmdline += f' -o {args.out}'

        std, err, code = self.get_execute_output(cmdline)
        self.log_position_std = 0
        self.log_position_err = 0

    def get_nnabla_cli_log(self):
        finished = False
        std, err, code = self.get_execute_output(f'docker ps -q -f name={self.dockername}')

        if len(std) == 0 and len(err) == 0:
            finished = True

        std, err, code = self.get_execute_output(f'docker logs {self.dockername}')
        log_std = std.decode()
        rlog_std = log_std[self.log_position_std:]
        self.log_position_std = len(log_std)
        log_err = err.decode()
        rlog_err = log_err[self.log_position_err:]
        self.log_position_err = len(log_err)
        return rlog_err + rlog_std, finished

    def terminate_nnabla_cli(self):
        self.get_execute_output(f'docker kill {self.dockername}')
        self.get_execute_output(f'docker rm {self.dockername}')

    def cleanup_nnabla_cli(self):
        self.get_execute_output(
            'docker ps -a -f name=nnabla-ssh-[0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9]-[0-9][0-9][0-9][0-9][0-9][0-9] --format {{.ID}} |xargs docker kill')
        self.get_execute_output(
            'docker ps -a -f name=nnabla-ssh-[0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9]-[0-9][0-9][0-9][0-9][0-9][0-9] --format {{.ID}} |xargs docker rm')
