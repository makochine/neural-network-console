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
import io
import paramiko
import scp
import sys
import time

from nnabla_ssh.utils import remote


class RemoteConnector(remote.Remote):
    """nnabla_cliが使えるホストにSSHで接続して学習を実施する
    """

    def __init__(self, info):
        """Initialize
        """
        super(remote.Remote, self).__init__()

        self._type = 'DIRECT'
        self._verbose = True  # make it true for debugging.

        self._hostname = info['host']
        self.remote_base_directory = info['basedir']
        self._user = info['user']
        self._key = self.from_private_key(info['key'])

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
        self.log_filename = f'{args.out}/{command}_log.txt'

        if command == 'forward':
            cmdline = 'nohup bash -l -c "nnabla_cli '
            cmdline += command
            cmdline += self.parse_forward_options(args)

        elif command == 'train' or command == 'profile':

            std, err, code = self.get_execute_output('nvidia-smi --query-gpu=index,name --format=csv ')
            gpu_names = {}
            if code == 0:
                for l in std.decode().splitlines()[1:]:
                    index, name = [i.strip() for i in l.split(',')]
                    gpu_names[int(index)] = name

            self._gpu_list = sorted(gpu_names.keys())
            self._num_gpu = len(self._gpu_list)
            self.verbose(f'Using {self._num_gpu}GPU(s)')
            for i in self._gpu_list:
                self.verbose(f'     GPU{i} ({gpu_names[i]})')

            # Temporarily exec only with Single-GPU mode.
            # if self._num_gpu == 1:
            #    cmdline = f'nohup nnabla_cli '
            # else:
            #    cmdline = f'nohup mpiexec -q -n {self._num_gpu} nnabla_cli -m '
            cmdline = f'nohup bash -l -c "nnabla_cli '

            cmdline += command
            cmdline += self.parse_train_options(args)

        else:
            return None

        cmdline += f' -c {args.config}'
        cmdline += f' -o {args.out}'
        cmdline += f' &> {self.log_filename}" &\n'
        s, e, code = self.get_execute_output(f'cat >{args.out}/{command}_start.sh', io.StringIO(cmdline))
        s, e, code = self.get_execute_output(f'cat >>{args.out}/{command}_start.sh', io.StringIO('echo $!\n'))
        std, err, code = self.get_execute_output(f'bash {args.out}/{command}_start.sh')
        self.command_pid = std.strip().decode()

        self.log_position_std = 0
        self.log_position_err = 0
        return True

    def get_nnabla_cli_log(self):
        finished = False
        std, err, code = self.get_execute_output(f'ps {self.command_pid}')
        if code != 0:
            finished = True

        std, err, code = self.get_execute_output(f'cat {self.log_filename}')
        log_std = std.decode()
        rlog_std = log_std[self.log_position_std:]
        self.log_position_std = len(log_std)
        log_err = err.decode()
        rlog_err = log_err[self.log_position_err:]
        self.log_position_err = len(log_err)
        return rlog_err + rlog_std, finished

    def terminate_nnabla_cli(self):
        self.get_execute_output(f'kill {self.command_pid}')

    def cleanup_nnabla_cli(self):
        pass
