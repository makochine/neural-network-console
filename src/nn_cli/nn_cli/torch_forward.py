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
import csv
import torch
import logging
import numpy as np
from nnabla.utils.image_utils import imsave
import nn_cli.torch_load as torch_load
from nn_cli.torch_utility import to_device
from nnabla.utils.data_source_loader import FileReader
from nn_cli.torch_utility import load_train_state, assign_parameters
from nn_cli.nnabla_utility import is_float, compute_full_path
from nn_cli.torch_dataset import CSVDataset


def _set_initial_values(result, type_and_name, d):
    result.names.append(type_and_name[1])
    vtype = ''
    dim = 0
    if not type_and_name[0]:
        # Infer modal type from tensor shape
        if len(d.shape) == 2:
            # csv for matrix
            vtype = '.csv'
            dim = 1
        elif len(d.shape) == 3:
            # png for 1 map or 3 maps tensor
            vtype = '.png'
            dim = 1 if d.shape[0] == 1 or d.shape[0] == 3 else d.shape[0]
        else:
            # col for others
            vtype = 'col'
            dim = np.prod(d.shape)
    result.types.append(vtype)
    result.dims.append(dim)
    return result


def _update_result(args, index, result, values, output_index, type_end_names, output_image):
    outputs = []
    for o, type_and_name in zip(values, type_end_names):
        for data_index, d in enumerate(o):
            if len(result.dims) <= output_index:
                result = _set_initial_values(result, type_and_name, d)
            if len(outputs) <= data_index:
                outputs.append([])
            name = result.names[output_index]
            vtype = result.types[output_index]
            dim = result.dims[output_index]

            # Output data
            if vtype == 'col' or not output_image:
                # Vector type output
                if 'numpy.ndarray' != type(d):
                    tmp = np.array([d])
                    outputs[data_index].extend(np.ndarray.flatten(tmp))
                else:
                    outputs[data_index].extend(np.ndarray.flatten(d))
            else:
                for dim_index in range(dim):
                    file_index = index + data_index
                    file_name = '{}_{:04d}'.format(
                        output_index, file_index // 1000) + os.path.sep
                    if dim > 1:
                        file_name += str(dim_index) + '_'
                    file_name += '{}{}'.format(file_index, vtype)
                    full_path = os.path.join(
                        args.outdir, args.result_outdir, file_name)
                    directory = os.path.dirname(full_path)
                    try:
                        os.makedirs(directory)
                    except OSError:
                        pass  # python2 does not support exists_ok arg
                    if vtype in ['.bmp', '.jpeg', '.jpg', '.png', '.gif', '.tif']:
                        x = np.array(d, dtype=np.float32) * 255.
                        while len(x.shape) == 4:
                            x = x[0]
                        if x.shape[0] > 3 or x.shape[0] == 2:
                            x = x[dim_index]
                        elif x.shape[0] == 3:
                            x = x.transpose(1, 2, 0)
                        else:
                            x = x.reshape(x.shape[1], x.shape[2])
                        x = x.clip(0, 255).astype(np.uint8)
                        imsave(full_path, x)
                    else:
                        # CSV type
                        with open(full_path, 'w') as f:
                            writer = csv.writer(f, lineterminator='\n')
                            x = np.array(d)
                            writer.writerows(x)
                    outputs[data_index].append(
                        os.path.join('.', args.result_outdir, file_name))
        output_index += 1

    return result, outputs


def _forward(args, config, output_image=True):
    global _save_parameter_info
    _CGLOAD_LOG_INTERVAL = 20

    class ForwardResult:
        pass

    result = ForwardResult()
    result.dims = []
    result.types = []
    result.names = []

    index = 0
    output_index = 0
    loaded_params = load_train_state(args.config, config)
    if loaded_params:
        assign_parameters(config, loaded_params)
    outputs = list()

    for name, mon in config.monitors.items():
        if name == 'valid_error':
            forward_dataset_instance = CSVDataset(
                args.dataset, mon.dataset.props['image_norm'])
            batch_size = args.batch_size if args.batch_size != -1 else 1
            forward_dataset = torch.utils.data.dataloader.DataLoader(forward_dataset_instance,
                                                                     batch_size=batch_size,
                                                                     shuffle=False)
            for i, data in enumerate(forward_dataset):
                to_device(data, config.global_config.default_context.device)
                index += len(data['x'])
                mon.network.infer_mode()
                mon.network.forward_without_loss = True
                result_1, outputs_1 = _update_result(
                    args, index, result, [next(iter(mon.network.infer(data)['loss'].values())).detach().cpu().numpy()]
                    , output_index, [['', "y'"]], output_image)
                result = result_1
                outputs += outputs_1
                logging.log(99, f"data {i+1} / {len(forward_dataset)}")
    return result, outputs


def forward_command(args):
    model_config = args.config
    _, extension = os.path.splitext(model_config)
    index = 0
    if extension == '.nnpt':
        proj_config = torch_load.load_from_nnpt(model_config, args.context)
        orders = {}
        result_csv_filename = os.path.join(args.outdir, args.outfile)
        # With CSV
        if os.path.splitext(args.dataset)[1] == '.csv':
            # load dataset as csv
            filereader = FileReader(args.dataset)
            with filereader.open(textmode=True, encoding='utf-8-sig') as f:
                rows = [row for row in csv.reader(f)]
            row0 = rows.pop(0)
            if args.replace_path:
                root_path = os.path.dirname(args.dataset)
                root_path = os.path.abspath(root_path.replace('/|\\', os.path.sep))
            else:
                root_path = '.'
            rows = [row for row in rows if len(row)]
            rows = list(map(lambda row: list(map(lambda i, x: x if row0[i][0] == '#' or is_float(
                x) else compute_full_path(root_path, x), range(len(row)), row)), rows))
            for i in range(len(rows)):
                orders[i] = i
        with open(result_csv_filename, 'w', encoding='utf-8') as f:
            writer = csv.writer(f, lineterminator='\n')
            result, outputs = _forward(args, proj_config)
            for name, dim in zip(result.names, result.dims):
                if dim == 1:
                    row0.append(name)
                else:
                    for d in range(dim):
                        row0.append(name + '__' + str(d))
            writer.writerow(row0)
            for i, output in enumerate(outputs):
                if index + i < len(rows):
                    import copy
                    row = copy.deepcopy(rows[orders[index + i]])
                    row.extend(output)
                    writer.writerow(row)
    else:
        raise ValueError(f"Unsupported extension: {extension}. Please choose '.nnpt' file.")
