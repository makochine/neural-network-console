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

import datetime
import glob
import os
import re
import signal
import sys
import time


class RemoteCommand:

    def parse_forward_options(self, args):
        cmdline = ''
        if 'param' in args and args.param:
            cmdline += f' -p {args.param}'
        if 'dataset' in args and args.dataset:
            cmdline += f' -d {args.dataset}'
        if 'outfile' in args and args.outfile:
            cmdline += f' -f {args.outfile}'
        if 'batch_size' in args and args.batch_size:
            cmdline += f' -b {args.batch_size}'
        if 'replace_path' in args and args.replace_path:
            cmdline += f' --replace_path'
        if 'result_outdir' in args and args.result_outdir:
            cmdline += f' --result_outdir {args.result_outdir}'
        return cmdline

    def parse_train_options(self, args):
        cmdline = ''
        if 'param' in args and args.param:
            cmdline += f' -p {args.param}'
        if 'resume' in args and args.resume:
            cmdline += f' -r'
        if 'enable_ooc' in args and args.enable_ooc:
            cmdline += f' -O'
        if 'ooc_gpu_memory_size' in args and args.ooc_gpu_memory_size:
            cmdline += f' -m {args.ooc_gpu_memory_size}'
        if 'ooc_window_length' in args and args.ooc_window_length:
            cmdline += f' -w {args.ooc_window_length}'
        return cmdline

    def exec_cli_command(self, command, args):
        """学習を実施する

        リモートで学習を実施します。リモート側では Remote.basedir以下
        に決め打ちでディレクトリを作成して利用します。


        Args:
           config: nntxt
           param: パラメータ
           outdir: 出力ファイル

        """

        self.csv_basedirs = {}
        self.outdir = args.outdir

        # シグナルハンドラを設定して、中断された際に後始末ができるよう
        # にします。
        signal.signal(signal.SIGINT, self.signal_handler)

        # 今回の学習時に利用するディレクトリを作成します。
        self.remote_base = '/'.join([self.remote_base_directory,
                                     'train',
                                     datetime.datetime.now().strftime('%Y%m%d-%H%M%S')])

        remote_config = '/'.join([self.remote_base, os.path.basename(args.config)])
        self.remote_outdir = '/'.join([self.remote_base, 'out'])
        self.remote_mkdir_p(self.remote_base)
        self.remote_mkdir_p(self.remote_outdir)

        copy_dataset = True
        if command == 'forward':
            copy_dataset = False

        for fn in glob.glob(f'{self.outdir}/*'):
            if os.path.isfile(fn):
                self.verbose(f'COPY TO REMOTE {fn}')
                _, e = os.path.splitext(fn)
                e = e.lower()
                if e == '.nntxt' or e == '.nnp':
                    self.copy_config_file_to_remote(fn,
                                                    '/'.join([self.remote_outdir, os.path.basename(fn)]),
                                                    copy_dataset=copy_dataset)
                else:
                    self.copy_to_remote(fn, '/'.join([self.remote_outdir, os.path.basename(fn)]))

        # nntxtファイルを変換します。
        self.copy_config_file_to_remote(args.config, remote_config, copy_dataset=copy_dataset)

        args.config = remote_config
        args.out = self.remote_outdir
        args.base = self.remote_base
        self._jobname = f'nnabla-ssh-{command}-' + datetime.datetime.now().strftime('%Y%m%d-%H%M%S')
        self._local_datadir = None

        if 'dataset' in args and args.dataset is not None:
            self._local_datadir = os.path.normpath(os.path.dirname(args.dataset))
            args.dataset = self.copy_csv_dataset_to_remote(args.dataset)

        if 'param' in args and args.param is not None:
            remote_param = '/'.join([self.remote_outdir, os.path.basename(args.param)])
            self.copy_to_remote(args.param, remote_param)
            args.param = remote_param

        if command == 'forward':
            self.create_deconvert_dictionary()
            self.start_nnabla_cli(command, args)

            offset = None

            progress_interval = 1
            recent_progress_time = time.time()

            while True:

                # ログを取得
                log, finished = self.get_nnabla_cli_log()
                if log is not None:
                    for l in log.splitlines():
                        m = re.match('^(\d{4})-(\d{2})-(\d{2}) (\d{2}):(\d{2}):(\d{2}),(\d+)(.*$)', l)
                        if m:
                            t = datetime.datetime(*[int(i) for i in m.groups()[:-1]])
                            if offset is None:
                                offset = datetime.datetime.today() - t
                            t = t + offset
                            sys.stderr.write(t.strftime('%Y-%m-%d %H:%M:%S'))
                            sys.stderr.write(',{:03d}'.format(t.microsecond // 1000))
                            sys.stderr.write(m.groups()[-1])
                        else:
                            sys.stderr.write(l)
                        sys.stderr.write('\n')
                        sys.stderr.flush()

                # 終了処理
                if finished:
                    self.verbose("Finished")
                    sys.stderr.flush()
                    break

                # progress_interval毎に出力結果を同期
                current_progress_time = time.time()
                if current_progress_time - recent_progress_time > progress_interval:
                    recent_progress_time = current_progress_time
                    for fn in ['monitoring_report.yml', 'progress.txt']:
                        self.copy_progress_files(fn)

                sys.stderr.flush()
                time.sleep(0.5)

        elif command == 'train' or command == 'profile':
            self.create_deconvert_dictionary()
            self.start_nnabla_cli(command, args)

            # 学習状況を監視

            # 出力ディレクトリの更新頻度
            # TODO: 状況に応じてインターバルタイムを変更したい。
            # progress.txtなどの進行状況や、出力データのサイズに応じて頻度
            # を変更させる必要があるかもしれない
            current_epoch = None
            prev_epoch = None
            offset = None

            progress_interval = 1
            recent_progress_time = time.time()

            result_interval = 60
            recent_result_time = time.time()

            while True:

                # ログを取得
                log, finished = self.get_nnabla_cli_log()
                if log is not None:
                    for l in log.splitlines():
                        m = re.match('^(\d{4})-(\d{2})-(\d{2}) (\d{2}):(\d{2}):(\d{2}),(\d+)(.*$)', l)
                        if m:
                            t = datetime.datetime(*[int(i) for i in m.groups()[:-1]])
                            if offset is None:
                                offset = datetime.datetime.today() - t
                            t = t + offset
                            sys.stderr.write(t.strftime('%Y-%m-%d %H:%M:%S'))
                            sys.stderr.write(',{:03d}'.format(t.microsecond // 1000))
                            sys.stderr.write(m.groups()[-1])
                        else:
                            sys.stderr.write(l)
                        sys.stderr.write('\n')
                        sys.stderr.flush()
                        m = re.search('epoch\s+(\d+)\s+of\s+(\d+)\s+(\w+)', l)
                        if m:
                            if m.group(3) != 'begin':
                                current_epoch = int(m.group(1))

                # 終了処理
                if finished:
                    self.verbose("Finished")
                    sys.stderr.flush()
                    break

                # progress_interval毎に出力結果を同期
                current_progress_time = time.time()
                if current_progress_time - recent_progress_time > progress_interval:
                    recent_progress_time = current_progress_time
                    for fn in ['monitoring_report.yml', 'progress.txt']:
                        self.copy_progress_files(fn)

                # result_interval毎に出力結果を同期
                current_result_time = time.time()
                if current_result_time - recent_result_time > result_interval:
                    recent_result_time = current_result_time
                    if prev_epoch != current_epoch:
                        try:
                            self.sync_output_from_remote()
                        except:
                            pass
                        prev_epoch = current_epoch

                sys.stderr.flush()
                time.sleep(0.5)

        else:
            return None

        self.verbose("Sync output dir")
        self.sync_output_from_remote()

        self.verbose("Clean remote output dir")
        self.cleanup()

        # 念のためリモート側のプロセス終了処理を実施
        self.verbose(f'Cleanup {command} process')
        self.terminate_nnabla_cli()

        # シグナルハンドラを元に戻す
        signal.signal(signal.SIGINT, signal.SIG_DFL)
        self.verbose(f'Remote {command} finished.')
        return True

    def cleanup(self):
        """リモートの学習結果ディレクトリを削除
        """
        self.get_execute_output(f'rm -rf {self.remote_base}')

    def cleanup_dataset(self):
        """リモートのデータセットディレクトリを全て削除
        """

        datasetdir = '/'.join([self.remote_base_directory, 'dataset'])
        self.get_execute_output(f'rm -rvf {datasetdir}')

    def cleanup_all_train(self):
        """リモートの学習結果ディレクトリを全て削除
        """

        traindir = '/'.join([self.remote_base_directory, 'train'])
        self.get_execute_output(f'rm -rvf {traindir}')

    def cleanup_all(self):
        """リモートをクリーンアップ
        """

        self.cleanup_dataset()
        self.cleanup_all_train()
        self.cleanup_nnabla_cli()
