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
import io
import os
import re
import urllib.parse
import numpy as np
import timeit
import torch
import torch.nn as nn
import zipfile
import h5py
from contextlib import contextmanager
from collections import OrderedDict
import logging
import configparser

cpu_load_backend_ok = True
try:
    import psutil
except Exception:
    # measure cpu load only if psutil installed
    cpu_load_backend_ok = False

gpu_load_backend_ok = True
import nn_cli.nvml

gpu_load_backend_ok = False
_ANALYSE_GPU_STATUS_INTERVAL = 20
_GPU_SLOWING_WARNING_THRESHOLD = 1.4
_GPU_SLOWING_ERROR_THRESHOLD = 2

# load variable
# ============
gpu_m_count = 0
gpu_a_load = {}
if cpu_load_backend_ok:
    p_handler = psutil.Process()
    p_handler_avg = psutil.Process()
    p_handler_avg.cpu_percent()


def is_float(x):
    # x is string
    try:
        float(x)
        return True
    except ValueError:
        return False


def str_to_num(str):
    # Convert str to num.
    #   12345678
    #   123e123  123 ^ 123
    #   1234K    1234 * 1000
    #   1234k    1234 * 1024
    #   1234M    1234 * 1000^2
    #   1234m    1234 * 1024^2
    #   1234G    1234 * 1000^3
    #   1234g    1234 * 1024^3
    #   1234T    1234 * 1000^4
    #   1234t    1234 * 1024^4
    #   1234P    1234 * 1000^5
    #   1234p    1234 * 1024^5
    import re
    m = re.match(
        r'(^[1-9][0-9]*$|^([1-9][0-9]*(\.[0-9]+)?)((e)([0-9]+)|[KkMmGgTtPp]))$', str)
    print(m)
    if m:
        if m.group(4):
            if m.group(5):
                return int(float(m.group(2)) ** int(m.group(6)))
            else:
                return int(float(m.group(2)) *
                           {'K': 1000, 'k': 1024,
                            'M': 1000 ** 2, 'm': 1024 ** 2,
                            'G': 1000 ** 3, 'g': 1024 ** 3,
                            'T': 1000 ** 4, 't': 1024 ** 4,
                            'P': 1000 ** 5, 'p': 1024 ** 5}[m.group(4)])
        else:
            return int(m.group(1))
    else:
        return -1


def compute_full_path(root_path, file_path):
    full_path = os.path.join(root_path, file_path)
    full_path = full_path.replace('\\', os.path.sep)
    full_path = full_path.replace('/', os.path.sep)
    full_path = full_path.replace(os.path.sep + '.' + os.path.sep, os.path.sep)
    return full_path


def let_data_to_variable(variable, data, ctx=None, data_name=None, variable_name=None):
    pass


def collect_and_shape_result(c_load, g_load):
    # c_load : float e.g. 58.5
    # g_load : [[nvidia_device_id, gpu_load]]

    comm = current_communicator()
    if comm:
        res = [[comm.rank, c_load], *g_load[:1]]
        t_load_ndarray = np.array(res).reshape(-1)

        load_var = nn.Variable([len(t_load_ndarray), ])
        load_var.d = t_load_ndarray
        load_list_var = [nn.Variable([len(t_load_ndarray), ])
                         for _ in range(comm.size)]
        comm.all_gather(load_var.data, [a.data for a in load_list_var])
        result_arr = [[*np.round(a.d.astype(float), decimals=1)]
                      for a in load_list_var]
    else:
        res = [[0, c_load], *g_load[:1]]
        t_load_ndarray = np.round(np.array(res).reshape(-1), decimals=1)
        result_arr = [[*t_load_ndarray.astype(float)]]

    result_arr = sorted(result_arr, key=lambda x: x[0])

    return result_arr


def measure_cpu_gpu_instant_load():
    # Get current cpu gpu load, as
    # load = [rank, cpu_load, nvidia_device_id, gpu_load]
    # result_arr: [load, load, ...]
    pass


def get_cpu_gpu_average_load():
    # Get average cpu gpu load, as
    # load = [rank, cpu_load, nvidia_device_id, gpu_load]
    # result_arr: [load, load, ...]
    pass


def to_device(data, device):
    for key, value in data.items():
        if isinstance(value, torch.Tensor):
            data[key] = value.to(device)
        else:
            raise TypeError(f"Value associated with key '{key}' is not a PyTorch tensor.")

    return data


class NodeTimeInfoCollector:
    def __init__(self):
        self.timelist = []
        self._frequency = _ANALYSE_GPU_STATUS_INTERVAL
        self._warning_threshold = _GPU_SLOWING_WARNING_THRESHOLD
        self._error_threshold = _GPU_SLOWING_ERROR_THRESHOLD

    @contextmanager
    def collect_cost_time(self, comm, iter_n):
        if comm and iter_n % self._frequency == 0:
            t1 = timeit.default_timer()
            yield
            t2 = timeit.default_timer()
            self.timelist.append(t2 - t1)
            # self._get_analysis(comm)
        else:
            yield

    def _reap_data(self):
        result = self.timelist[:]
        self.timelist = []
        return result


def get_decorated_file_loader():
    '''get file loader for second stage loading
    Second stage means graph, optimizer, executor, ... and
    so on has already been loaded, the files to load are used
    to decorate existing nnabla representation.
    '''
    file_loaders = OrderedDict([
        ('.nnpt', _nnpt_file_loader),
        ('.h5', _h5_parameter_file_loader),
    ])
    return file_loaders


def _h5_parameter_file_loader(info, file_loader, nnpt, filename, ext='.h5', outpath='./'):
    with get_file_handle_load(nnpt, filename, ext) as hd:
        keys = []

        def _get_keys(name):
            ds = hd[name]
            if not isinstance(ds, h5py.Dataset):
                # Group
                return
            # To preserve order of parameters
            keys.append((ds.attrs.get('index', None), name))

        hd.visit(_get_keys)
        # Collect parameters in a dictionary before assigning them
        loaded_params = {}
        for _, key in sorted(keys):
            # key in format of /network_name/layer_index/layer_param_name
            ds = hd[key]
            # Store parameters in the dictionary
            loaded_params[key] = ds[()]

    return loaded_params


def assign_parameters(info, loaded_params):
    for key, param_value in loaded_params.items():
        network_name, layer_name, layer_param_name = key.split('/')
        network = info.networks.get(network_name)
        if network:
            layer = network.layers.get(layer_name)
            if layer:
                if layer.props['state_dict'] is None:
                    layer.props['state_dict'] = dict()
                layer.props['state_dict'][layer_param_name] = param_value


@contextmanager
def get_file_handle_load(nnpt, path, ext):
    if nnpt is None:
        if ext == '.nnpt':
            need_close = True
            f = zipfile.ZipFile(path, 'r')
        elif ext == '.h5':
            need_close = True
            if isinstance(path, str):
                f = h5py.File(path, 'r')
            else:
                f = h5py.File(io.BytesIO(path.read()), 'r')
        else:
            raise ValueError("Currently, ext == {} is not support".format(ext))
        try:
            yield f
        finally:
            if need_close:
                f.close()
    else:
        if ext in ['.h5']:
            # if nnpt is not None and extension is .h5
            # we assume to return a .h5 type file handler
            with nnpt.open(path, 'r') as n:
                f = h5py.File(io.BytesIO(n.read()), 'r')
                yield f
        else:
            f = nnpt.open(path, 'r')
            try:
                yield f
            finally:
                f.close()


def _nnpt_file_loader(info, file_loaders, nnpt, filename, ext, outpath=None):
    '''.nnpt
    nnpt file loader
    This loader only handles .nnpt file, which
    call other handlers to help to digest files.
    '''
    assert nnpt == None, ".nnpt in a .nnpt file seems impossible."
    with get_file_handle_load(nnpt, filename, ext) as n:
        for name in n.namelist():
            _, ext = os.path.splitext(name)
            for supported_extensions, file_loader in file_loaders.items():
                if ext in supported_extensions:
                    param_loader = file_loader(info, file_loaders, n, name, ext, outpath)
                elif ext == ".sdcproj" and outpath:
                    with open(os.path.join(outpath, os.path.basename(name)), 'wb') as f:
                        f.write(n.read(name))
        return param_loader


def load_files(info, file_loaders, filenames, extension=None, outpath=None):
    '''load_files
    Load files, if filesnames is not a list, we converted it to a list.
    If filenames is a list, we handle it one by one, with the handler tied with
    its postfix name.

    Args:
        ctx: A object that represents the context of sharing states with loaders.
        file_loaders (OrderedDict): List of handlers, tied with extension name.
        filenames (list): List of filenames.
        extension (str): File extension name, used to identify the file type
    Returns:
        None
    '''
    if isinstance(filenames, list) or isinstance(filenames, tuple):
        pass
    elif isinstance(filenames, str) or hasattr(filenames, 'read'):
        filenames = [filenames]

    for filename in filenames:
        if isinstance(filename, str):
            _, ext = os.path.splitext(filename)
        else:
            ext = extension
        handled = False
        for supported_extensions, file_loader in file_loaders.items():
            if ext in supported_extensions:
                return file_loader(info, file_loaders, None, filename, ext, outpath)
        else:
            if not handled:
                logging.warning('{} is omitted.'.format(filename))
            return None


def load_train_state(filename, info=None, outpath=None):
    file_loaders = get_decorated_file_loader()
    result = load_files(info, file_loaders, filename, outpath=outpath)
    logging.info("Load training resume states: {}".format(filename))
    return result


def extract_param_from_nnpt_h5(filename, outpath=None):
    result = load_train_state(filename, outpath=outpath)

    param_type_map = {'weight': 'W',
                      "bias": 'b'}

    def _save_param(param_name, variable, outpath):
        net_key, layer_key, param_key = param_name.split('/')
        net_key = net_key if sdc_proj.has_section(net_key + 'Network_Global') else 'Network_Global'
        op_type = ''
        op_name = ''
        param_type = param_type_map[param_key]
        if sdc_proj.has_section(net_key):
            properties = sdc_proj.items(f'Layer_{layer_key}')
            num_properties = len(properties)
            for key, value in properties:
                if key == 'type':
                    op_type = value.lower()
            if num_properties:
                for i in range(num_properties):
                    if properties[i][1] == 'Name':
                        op_name = next((prop_value for prop_name, prop_value in properties if
                                        prop_name == properties[i][0].replace('name', 'value')), None)
                        break  # exit loop once 'Name' property is found

            file_path = os.path.join(outpath, '~'.join([op_name, op_type, param_type]) + '.txt')
            with open(file_path, 'w') as output:
                print(variable.shape, file=output)
                np.array(variable).tofile(output, sep="\n")
            logging.info(f"Save {op_name} states: {file_path}")

    sdc_file = os.path.join(outpath, 'config.sdcproj')
    sdc_proj = configparser.ConfigParser() if os.path.isfile(sdc_file) else None
    if sdc_proj:
        with open(sdc_file, 'r') as f:
            sdc_proj.read_file(f)

    net_outpath = os.path.join(outpath, 'net')
    os.makedirs(net_outpath, exist_ok=True)

    for key, variable in result.items():
        logging.log(99, key)
        param_name = urllib.parse.quote(key, safe='/ ')

        if sdc_proj:
            _save_param(param_name, variable, net_outpath)


def save_param_txt_to_h5(indir, param):
    networks = OrderedDict()
    # Load Parameters
    in_files = [f for f in os.listdir(
        indir) if os.path.isfile(os.path.join(indir, f)) and f.endswith('.txt')]

    def load_param_in_txt(filepath):
        with open(filepath, 'r') as input:
            ds = input.read()
        ds = ds.split("\n")
        shape = eval(ds.pop(0))
        return np.fromstring("\n".join(ds),
                             dtype=np.float32,
                             sep="\n").reshape(shape)

    for file_path in in_files:
        network_name, layer_name, param_name = file_path.split('~')
        param_name = param_name.split('.')[0]  # Remove the extension
        # Create network dictionary if it doesn't exist
        if network_name not in networks:
            networks[network_name] = OrderedDict()
        # Create layer dictionary if it doesn't exist
        if layer_name not in networks[network_name]:
            networks[network_name][layer_name] = OrderedDict()
        networks[network_name][layer_name][param_name] = load_param_in_txt(os.path.join(indir, file_path))

    # Save parameter in h5
    logging.log(99, 'Saving parameters...')
    with h5py.File(param, 'w') as hf:
        for network_name, network_data in networks.items():
            network_group = hf.create_group(network_name)
            for layer_name, layer_data in network_data.items():
                layer_group = network_group.create_group(layer_name)
                for param_type, parameters in layer_data.items():
                    layer_group.create_dataset(param_type, data=parameters)
    logging.log(99, 'Encode Parameter Completed.')


def save_train_state(filename, network_dict):
    with h5py.File(filename, 'w') as file:
        for net_name, net in network_dict.items():
            net_state = OrderedDict()
            for layer_name, layer in net.layers.items():
                if layer.function_instance:
                    layer_state = OrderedDict()
                    for param_name, param in layer.function_instance.state_dict().items():
                        layer_state[param_name] = param.cpu().numpy()
                    net_state[layer_name] = layer_state
            # Save the network state in the HDF5 file
            net_group = file.create_group(net_name)
            for layer_name, layer_state in net_state.items():
                layer_group = net_group.create_group(layer_name)
                for param_name, param_value in layer_state.items():
                    layer_group.create_dataset(param_name, data=param_value)


def current_communicator():
    class Communicator:
        pass

    comm = Communicator()
    return comm


def single_or_rankzero():
    return True


def torch_version():
    return str(torch.__version__)
