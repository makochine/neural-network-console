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
import os
import time
import glob
import numpy as np
from datetime import datetime, timedelta
import logging
import zipfile
from collections import OrderedDict
import nnabla.utils.callback as callback
import nn_cli.torch_load as torch_load
from nn_cli.torch_utility import torch_version
from nn_cli.torch_utility import to_device
from nn_cli.torch_utility import NodeTimeInfoCollector
from nn_cli.torch_utility import get_cpu_gpu_average_load
from nn_cli.torch_utility import measure_cpu_gpu_instant_load
from nn_cli.torch_utility import current_communicator, single_or_rankzero
from nn_cli.torch_utility import load_train_state, save_train_state, assign_parameters


nodeTimeCollector = NodeTimeInfoCollector()

cpu_load_backend_ok = True
try:
    import psutil
except Exception:
    # measure cpu load only if psutil installed
    cpu_load_backend_ok = False

_save_parameter_info = {}

# state output
# ============
state_file_name = ''
last_state_datetime = datetime.now()

# callback
# ============
state_callback = None


def configure_progress(file_name, cb=None):
    if file_name is not None:
        global state_file_name
        state_file_name = file_name
    if cb is not None:
        global state_callback
        state_callback = cb
    progress(None)


def progress(state, progress=0.0):
    if len(state_file_name):
        global last_state_datetime
        if last_state_datetime < datetime.now() + timedelta(milliseconds=-1000) or state is None:
            last_state_datetime = datetime.now()
            retry = 1
            while True:
                try:
                    with open(state_file_name, 'w') as f:
                        if state is not None:
                            f.write(f'{state} ({progress * 100:.2f}%)')
                    break
                except:
                    retry += 1
                    if retry > 100:
                        logging.critical(f'Failed to write to {state_file_name}.')
                        raise
                    time.sleep(0.1)
    callback.update_progress(f'{state} ({progress * 100:.2f}%)')
    if cpu_load_backend_ok:
        callback.update_status()
    if state_callback is not None:
        state_callback(state, progress)


def _get_current_parameter(args, config):
    best_error, best_epoch = callback.get_best_from_status(args)
    globname = os.path.join(args.outdir, 'results_current_*.nnpt')
    exists = glob.glob(globname)
    if len(exists) > 0:
        ex_list = {}

        for ex in exists:
            n = int(ex.rsplit('_', 1)[1].rsplit('.', 1)[0])
            ex_list[n] = ex

        last_epoch = sorted(ex_list.keys(), reverse=True)[0]

        last_parameter = ex_list[last_epoch]

        logging.log(99, f"Load parameter from [{os.path.basename(last_parameter)}]")
        loaded_params = load_train_state(last_parameter, config)

        return last_epoch, best_epoch, best_error, loaded_params

    return 0, best_epoch, best_error, None


def _calc_estimate_time(timeinfo, max_iter, last_iter, iter):
    timeinfo.past_time = time.time() - timeinfo.start_time
    timeinfo.estimate_time = timeinfo.past_time * \
                             (max_iter - last_iter) / (iter - last_iter)
    timeinfo.remain_time = timeinfo.estimate_time - timeinfo.past_time
    timeinfo.last_past_time = timeinfo.past_time
    return timeinfo


def _calc_epoch_span(timeinfo):
    cur_time = time.time()
    span = cur_time - timeinfo.last_epoch_start_time
    timeinfo.last_epoch_start_time = cur_time
    return span


def _format_cgload_log(cg_load):
    narr = np.array(cg_load).T
    if narr.shape[0] == 4:
        log_str = f'average load:{{cpu:{np.mean(narr[1]):.1f}%, gpu:{np.mean(narr[3]):.1f}%}}'
    else:
        log_str = f'average load:{{cpu:{np.mean(narr[1]):.1f}%}}'
    return log_str


def _save_parameters(args, suffix, epoch, train_config, force=False):
    global _save_parameter_info

    if suffix not in _save_parameter_info:
        _save_parameter_info[suffix] = {}
        _save_parameter_info[suffix]['epoch'] = 0
        _save_parameter_info[suffix]['time'] = 0

    current_time = time.time()
    timediff = current_time - _save_parameter_info[suffix]['time']
    epochdiff = epoch - _save_parameter_info[suffix]['epoch']

    globname = os.path.join(args.outdir, f'results_{suffix}_*.nnpt')
    exists = glob.glob(globname)

    base = os.path.join(args.outdir, f'results_{suffix}_{epoch}')
    base_candidate = callback.result_base(base, suffix, args.outdir)
    if base_candidate is None:
        if suffix is None or suffix == 'best':
            base = os.path.join(args.outdir, 'results')
    else:
        base = base_candidate

    filename = base + '.nnpt'

    if force or (not os.path.exists(filename) and (timediff > 180.0 or epochdiff > 10)):

        # Remove existing nnpt before saving new file.
        for exist in exists:
            if exist != args.config:
                os.unlink(exist)

        version_filename = base + '_version.txt'

        with open(version_filename, 'w') as file:
            file.write(torch_version())

        # Save parameter of whole graph
        param_filename = f'{base}_param.h5'
        save_train_state(param_filename, train_config.networks)

        _save_parameter_info[suffix]['epoch'] = epoch
        _save_parameter_info[suffix]['time'] = current_time

        with zipfile.ZipFile(filename, 'w') as zip_file:
            zip_file.write(version_filename, 'version.txt')
            zip_file.write(param_filename, 'param_torch.h5')
            zip_file.write(args.config, 'config.sdcproj')

        os.unlink(version_filename)
        os.unlink(param_filename)

        callback.save_train_snapshot()


def _evaluate(args, config, monitoring_report, best_error, epoch):
    error_str = ''
    valid_error = 0.0

    def _sum_error(sum, error):
        return sum + error

    for name, mon in config.monitors.items():
        error_sum_monitor = 0.0
        error_count = 0
        data_size = len(mon.dataset.dataset_instance)
        batch_size = config.training_config.batch_size

        for i in range(data_size // batch_size):
            data = next(iter(mon.data_loader))
            to_device(data, config.global_config.default_context.device)
            mon.network.infer_mode()
            mon.network.infer(data)

            # Sum error before forward to prepare input data while processing
            # on GPU
            if error_count > 0:
                error_sum = 0.0
                for v in mon.monitor_variables(data):
                    error_sum += np.mean(v.cpu().detach().numpy())
                    # v.variable_instance.data.zero()
                error_sum_monitor = _sum_error(error_sum_monitor, error_sum)
                progress(f'Evaluating "{name}" : error={error_sum_monitor / error_count:.6f}',
                         1.0 * i / (data_size // batch_size))
            error_count += 1

        # Sum error at the end of dataset
        error_sum = 0.0
        for v in mon.monitor_variables(data):
            error_sum += np.mean(v.cpu().detach().numpy())
            # v.variable_instance.data.zero()
        error_sum_monitor = _sum_error(error_sum_monitor, error_sum)

        if error_count == 0:
            error = 0
        else:
            error = error_sum_monitor / error_count

        if np.isnan(error) or np.isinf(error):
            logging.log(99, 'Validation error is Nan')
            error = 0.0

        monitoring_report.append(f'  {name}: {error}\n')

        callback.update_status((['monitoring_report', epoch, name], error))
        callback.update_status((['last', name], error))  # save last value

        if error_str != '':
            error_str += ', '
        else:
            error_str = f' {{{name}={error:.6f}'
        if name == 'valid_error':
            valid_error = error

    if error_str != '':
        error_str += '}'

    # Save Parameters
    if (not config.training_config.save_best) or \
            (not best_error) or \
            (best_error is not None and valid_error <= best_error):
        best_error = valid_error
        callback.update_status(('best.valid_error', best_error))
        callback.update_status(('best.epoch', epoch))
        _save_parameters(args, 'best', epoch, config, True)

    return best_error, error_str


def _update(iteration, config, cost):
    def _sum_cost():
        cost.sum_epoch += cost.sum_iteration
        cost.num_iteration += 1

    for opt_name, opt in config.optimizers.items():
        data = next(iter(opt.data_loader))
        to_device(data, config.global_config.default_context.device)
        opt.network.train_mode()
        loss = opt.network.infer(data)
        opt.make_ready()

        # Prepare the learning rate scheduler for the optimizer
        config.schedulers[opt_name].make_ready(opt, config.training_config.iter_per_epoch)

        loss.backward()
        # Update the parameters
        opt.step()

        # Update the learning rate scheduler to adjust learning rate
        config.schedulers[opt_name].step(iteration)

        cost.sum_iteration += loss.item()
        cost.variables = loss.item()
        _sum_cost()

    # Monitor loss at the end of epoch
    if iteration % config.training_config.iter_per_epoch == config.training_config.iter_per_epoch - 1:
        cost.sum_iteration += loss.item()
        _sum_cost()
        cost.variables = None
        cost.sum_iteration = 0.0

    return cost


def _train(args, config):
    global _save_parameter_info
    _CGLOAD_LOG_INTERVAL = 20

    best_error = None
    last_epoch = 0
    loaded_params = None
    if args.resume:
        last_epoch, best_epoch, best_error, loaded_params = _get_current_parameter(
            args, config)
        if best_epoch is not None:
            logging.log(
                99, f"Best error {best_error} recorded at epoch {best_epoch} in previous training.")
            if best_epoch > last_epoch:
                logging.log(
                    99, f"Resumed epoch is {last_epoch} but this training keeps this result.")
        logging.log(99, f"Resume from epoch {last_epoch + 1}")
    if args.param:
        loaded_params = load_train_state(args.param, config)
        logging.log(99, f"Load parameter from [{os.path.basename(args.param)}]")
        args.param = None
    if loaded_params:
        assign_parameters(config, loaded_params)

    callback.update_status(('epoch.max', config.training_config.max_epoch))
    callback.update_status(('epoch.current', last_epoch + 1
    if last_epoch < config.training_config.max_epoch
    else config.training_config.max_epoch))

    max_iteration = config.training_config.max_epoch * \
                    config.training_config.iter_per_epoch
    logging.log(99, f'Training epoch {last_epoch + 1} of {config.training_config.max_epoch} begin')

    class Cost:
        pass

    cost = Cost()
    cost.sum_epoch = 0.0
    cost.num_iteration = 0
    cost.sum_iteration = 0.0
    cost.variables = None

    class TimeInfo:
        pass

    timeinfo = TimeInfo()
    timeinfo.past_time = 0
    timeinfo.estimate_time = 0
    timeinfo.last_past_time = None

    config.timelimit = -1
    if max_iteration > 0:
        last_iteration = last_epoch * config.training_config.iter_per_epoch
        if last_iteration < max_iteration:

            timeinfo.start_time = time.time()
            timeinfo.last_epoch_start_time = timeinfo.start_time

            callback.update_status('processing', True, timeinfo.start_time)

            for iteration in range(last_iteration, max_iteration):

                # instant load measurement
                measure_cpu_gpu_instant_load()
                cost = _update(iteration, config, cost)

                if np.isnan(cost.sum_epoch) or np.isinf(cost.sum_epoch):
                    logging.log(99, 'Cost is Nan')
                    return False, False

                timeinfo = _calc_estimate_time(
                    timeinfo, max_iteration, last_iteration, iteration + 1)
                callback.update_time_train(prediction=timeinfo.estimate_time)

                if 0 < config.timelimit < timeinfo.estimate_time:
                    logging.log(99,
                                f'Expected training time ({timeinfo.estimate_time:.3f}s) will exceed time limit ({config.timelimit}s).')
                    return False, False

                if (iteration + 1) % config.training_config.iter_per_epoch == 0:
                    # End of epoch
                    epoch = iteration // config.training_config.iter_per_epoch + 1
                    cost_avg_epoch = cost.sum_epoch / cost.num_iteration if cost.num_iteration else 0
                    cost.sum_epoch = 0.0
                    cost.num_iteration = 0
                    monitoring_report = []

                    # Evaluation
                    error_str = ''
                    if epoch % config.training_config.monitor_interval == 0 or epoch <= 5:
                        best_error, error_str = _evaluate(
                            args, config, monitoring_report, best_error, epoch)

                    # Cpu/Gpu average load
                    cg_load_str = ''
                    cgload_log = ''
                    cg_load = get_cpu_gpu_average_load()
                    if cg_load:
                        cg_load_str = f'epoch {epoch} average_load_matrix: {cg_load}'
                        span = _calc_epoch_span(timeinfo)
                        if span > _CGLOAD_LOG_INTERVAL:
                            cgload_log = _format_cgload_log(cg_load)

                    # Write to monitoring_report.yml
                    with open(os.path.join(args.outdir, 'monitoring_report.yml'), 'a') as f:
                        f.write(f'{epoch - 1}:\n')
                        f.write(f'  cost: {cost_avg_epoch}\n')
                        for s in monitoring_report:
                            f.write(s)

                    callback.update_status((['monitoring_report', epoch, 'cost'], cost_avg_epoch))

                    _save_parameters(args, 'current', epoch, config)

                    callback.update_status(('epoch.current', epoch))
                    callback.update_status()

                    logging.log(99,
                                f'epoch {epoch} of {config.training_config.max_epoch} cost={cost_avg_epoch:.6f} {error_str} time=({timeinfo.past_time:.1f}s /{timeinfo.estimate_time:.1f}s) {cgload_log}')

                    if cg_load_str:
                        # cpu_gpu_average_load record at epoch level
                        callback.update_status(
                            (['cpu_gpu_epoch_load', epoch], cg_load))
                        progress(cg_load_str, 1)

                    if not callback.check_training_time(args, config, timeinfo, epoch, last_epoch):
                        _save_parameters(
                            args, 'current', epoch, config, True)
                        return False, True

            _save_parameters(args, 'current', epoch, config, True)
    return True, False


def train_command(args):
    _, extension = os.path.splitext(args.config)
    if extension == '.sdcproj':
        proj_config = torch_load.load_from_sdcproj(args.config, args.context)
        result, restart = _train(args, proj_config)
    elif extension == '.nnpt':
        args.resume = True
        proj_config = torch_load.load_from_nnpt(args.config, args.context)
        result, restart = _train(args, proj_config)
    else:
        raise ValueError(f"Unsupported extension: {extension}. Please choose either '.sdcproj' or '.nnpt'.")

    if single_or_rankzero() and not restart:
        if result:
            logging.log(99, 'Training Completed.')
            callback.update_status('finished')
        else:
            logging.log(99, 'Training Incompleted.')
            callback.update_status('failed')
    if single_or_rankzero():
        progress(None)
    return True
