# Copyright 2025 Sony Group Corporation.
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
import argparse
import csv
import configparser
from collections import OrderedDict
from collections.abc import Mapping

import numpy as np
import torch
import tensorflow as tf
import PIL

from transformers import pipeline
from nnabla.logger import logger
from nnabla.utils.progress import configure_progress, progress
from nnabla.utils.image_utils import imsave

def get_new_file_name(output_dir, key, i):
    dir = os.path.join(output_dir, f'{key}_{i // 1000:04}')
    if not os.path.exists(dir):
        os.makedirs(dir)
    return os.path.join(dir, f'{i % 1000:04}')

def flatten_dict_list(d, parent_key='', sep='_'):
    parent = parent_key + sep if parent_key else ''
    items = []
    if not isinstance(d, Mapping):
        if isinstance(d, list):
            if len(d) > 0:
                if isinstance(d[0], Mapping):
                    for i, d1 in enumerate(d):
                        items.extend(flatten_dict_list(d1, parent + str(i), sep=sep).items())
                    return OrderedDict(items)
                else:
                    d = torch.Tensor(d)
            else:
                return OrderedDict({})
        return flatten_dict_list(OrderedDict([('result', d)]), parent_key, sep)
    for k, v in d.items():
        if isinstance(v, Mapping):
            items.extend(flatten_dict_list(v, parent + k, sep=sep).items())
        elif isinstance(v, list):
            items.extend([(parent + k + sep + str(i), v2) for i, v2 in enumerate(v)])
        else:
            items.append((parent + k, v))
    return OrderedDict(items)

def sort_ordered_dict(d, keys_order):
    ordered = OrderedDict((k, d[k]) for k in keys_order if k in d)
    unordered = {k: d[k] for k in d if k not in ordered}
    ordered.update(unordered)
    return ordered


def PrepareDataset(args, config):
    if args.dataset is not None:
        dataset_name = ''
        dataset_uri = args.dataset
    else:
        num_dataset = int(config['Dataset']['Num'])
        dataset_names = [config[f'Dataset_{i}']['Name'] for i in range(num_dataset)]
        if 'Test' in dataset_names:
            dataset_index = dataset_names.index('Test')
        elif 'Validation' in dataset_names:
            dataset_index = dataset_names.index('Validation')
        else:
            dataset_index = -1
        
        if dataset_index >= 0:
            dataset_name = dataset_names[dataset_index] + ' '
            dataset_uri = config[f'Dataset_{dataset_index}']['URI']
        else:
            dataset_name= ''
            dataset_uri = ''

    if dataset_uri:
        logger.log(99, f"Loading {dataset_name}dataset from {dataset_uri}.") # Todo : change to info
        table = None
        if os.path.splitext(dataset_uri)[1].lower() == '.csv':
            with open(dataset_uri, encoding='utf-8-sig', newline='') as f:
                reader = csv.reader(f)
                table = [row for row in reader]

        if table is None:
            logger.critical(f"Error : Failed to read {dataset_uri}.")
    else:
        batch_size = int(config['Config']['BatchSize'])
        table = [[] for _ in range(batch_size + 1)]

    class Dataset:
        pass

    dataset = Dataset()
    dataset.filename = dataset_uri
    dataset.header = table[0]
    dataset.table = table[1:]
    dataset.variables = [col.split(':', 1)[0] for col in dataset.header]

    return dataset

def ReadProperty(layer_config, property_name, required=True):
    num_property = int(layer_config['NumProperty'])
    for ip in range(num_property):
        if property_name == layer_config[f'Property_{ip}_Name']:
            return layer_config[f'Property_{ip}_Value']
    if required:
        logger.critical(f'Error : Property "{property_name}" is not found in layer "{ReadProperty(layer_config, "Name")}."')
        exit(1)
    else:
        return None

class TransformerFunction:
    def __init__(self, layer_config, dataset):
        self.id = layer_config['ID']
        self.name = ReadProperty(layer_config, 'Name')
        self.dependencies = []
        self.dependents = []
        self.start_function = False
        self.function = lambda *args, **kwargs: (args, kwargs)
        self.args = {}
        self.result = None

        layer_type = layer_config['Type']
        if layer_type == 'InputString' or layer_type == 'InputFileName':
            self.start_function = True;
            dataset_variable = ReadProperty(layer_config, 'Dataset')
            arg_name = ReadProperty(layer_config, 'ArgName', False)
            dataset_variable_index = dataset.variables.index(dataset_variable)
            if layer_type == 'InputFileName':
                dataset_dir = os.path.dirname(dataset.filename)
                if arg_name:
                    self.function = lambda *args, **kwargs: {arg_name: os.path.join(dataset_dir, args[0][dataset_variable_index])}
                else:
                    self.function = lambda *args, **kwargs: (os.path.join(dataset_dir, args[0][dataset_variable_index]),)

                # replace path
                for i, row in enumerate(dataset.table):
                    if not os.path.isabs(dataset.table[i][dataset_variable_index]):
                        dataset.table[i][dataset_variable_index] = os.path.join(dataset_dir, dataset.table[i][dataset_variable_index])
            else:
                if arg_name:
                    self.function = lambda *args, **kwargs: {arg_name: args[0][dataset_variable_index]}
                else:
                    self.function = lambda *args, **kwargs: (args[0][dataset_variable_index],)
        elif layer_type == 'Constant':
            self.start_function = True;
            value = ReadProperty(layer_config, 'Value')
            value_type = ReadProperty(layer_config, 'ValueType')
            arg_name = ReadProperty(layer_config, 'ArgName', False)
            if value_type == 'Int':
                if arg_name:
                    self.function = lambda *args, **kwargs: {arg_name: int(value)}
                else:
                    self.function = lambda *args, **kwargs: (int(value),)
            elif value_type == 'Float':
                if arg_name:
                    self.function = lambda *args, **kwargs: {arg_name: float(value)}
                else:
                    self.function = lambda *args, **kwargs: (float(value),)
            else:
                if arg_name:
                    self.function = lambda *args, **kwargs: {arg_name: value}
                else:
                    self.function = lambda *args, **kwargs: (value,)
        elif layer_type == 'Identity' or layer_type == 'Concatenate':
            self.function = lambda *args, **kwargs: args if len(kwargs) == 0 else (kwargs if len(args) == 0 else (args, kwargs))
        elif layer_type == 'Extract':
            attribute = ReadProperty(layer_config, 'Attribute')
            arg_name = ReadProperty(layer_config, 'ArgName', False)
            if arg_name:
                self.function = lambda *args, **kwargs: {arg_name: kwargs[attribute]}
            else:
                self.function = lambda *args, **kwargs: (kwargs[attribute],)
        elif layer_type != 'Comment':
            pipeline_type = layer_type
            pipe_args = {}
            num_property = int(layer_config['NumProperty'])
            for ip in range(2, num_property):
                arg_value = layer_config[f'Property_{ip}_Value']
                if arg_value:
                    arg_name = layer_config[f'Property_{ip}_Name']
                    arg_type = layer_config[f'Property_{ip}_Type']
                    if arg_type in ['Boolean']:
                        arg_value = arg_value == 'True'
                    if arg_type in ['Int','PInt','UInt']:
                        arg_value = int(arg_value)
                    elif arg_type in ['Float']:
                        arg_value = float(arg_value)
                    elif 'Array' in arg_type:
                        arg_value = arg_value.split(',')
                        if 'Int' in arg_type:
                            arg_value = list(map(int, arg_value))
                        elif 'Float' in arg_type:
                            arg_value = list(map(float, arg_value))
                    pipe_args[arg_name] = arg_value
                model = pipe_args.pop('model', None)
                self.args = pipe_args
                pipe = pipeline(pipeline_type, model = model, device=0 if torch.cuda.is_available() else -1)
                self.function = lambda *args, **kwargs: flatten_dict_list(pipe(*args, **kwargs))

    def execute(self, *args, **kwargs):
        kwargs_ = self.args
        kwargs_.update(kwargs)
        
        self.result = self.function(*args, **kwargs_)
        return self.result

    def add_dependency(self, function):
        self.dependencies.append(function)

    def add_dependent(self, function):
        self.dependents.append(function)

    def get_result(self):
        return self.result

class TransformersWorkflow:
    def __init__(self):
        self.functions = {}
        self.start_function_ids = []

    def add_function(self, config, dataset):
        function = TransformerFunction(config, dataset)
        self.functions[function.id] = function
        if function.start_function:
            self.start_function_ids.append(function.id)
        return function

    def link_functions(self, from_id, to_id):
        from_function = self.functions.get(from_id)
        to_function = self.functions.get(to_id)
        from_function.add_dependent(to_function)
        to_function.add_dependency(from_function)

    def execute(self, dataset_row):
        completed_functions = set()
        for start_function_id in self.start_function_ids:
            start_function = self.functions.get(start_function_id)
            if start_function is None:
                logger.critical(f'No function idd {start_function_id} found.')
                continue
            self._execute_function(start_function, dataset_row, completed_functions)
        result = OrderedDict()
        for result_1 in [function.get_result() for function in self.functions.values() if not function.dependents and function.get_result() is not None]:
            result.update(result_1)
        return result

    def _execute_function(self, function, dataset_row, completed_functions):
        if function.id in completed_functions:
            return
        inputs = [dep.get_result() for dep in function.dependencies if dep.id in completed_functions]
        if len(inputs) < len(function.dependencies): # if not all dependencies are completed
            return
        args = ()
        kwargs = OrderedDict()
        for input in inputs:
            if isinstance(input, str):
                args = args + (input,)
            if isinstance(input, (tuple, list)):
                for input_1 in input:
                    if isinstance(input_1, Mapping):
                        kwargs.update(input_1)
                    else:
                        args = args + (input_1,)
            elif isinstance(input, Mapping):
                kwargs.update(input)
        if len(args) == 0 and not kwargs:
            args = (dataset_row,)
        function.execute(*args, **kwargs)
        completed_functions.add(function.id)
        for dep in function.dependents:
            self._execute_function(dep, dataset_row, completed_functions)

def inference(args):
    configure_progress(os.path.join(args.output, 'progress.txt'))
    logger.log(99, "Executing inference.")

    logger.log(99, f"Loading config from {args.config}.") # Todo : change to info
    config = configparser.ConfigParser()
    config.read(args.config)

    dataset = PrepareDataset(args, config)

    # prototype
    workflow = TransformersWorkflow()
    num_layer = int(config['Network_Global']['NumLayer'])
    for i in range(num_layer):
        layer_config = config[f'Layer_{i}']
        workflow.add_function(layer_config, dataset)

    num_link = int(config['Network_Global']['NumLink'])
    for i in range(num_link):
        link_config = config[f'Link_{i}']
        source_id = link_config['SourceLayerID']
        dest_id = link_config['DestLayerID']
        workflow.link_functions(source_id, dest_id)

    first = True
    num_data = len(dataset.table)
    for i, row in enumerate(dataset.table):
        progress(f'Processing... {i + 1} / {num_data} ', i / len(dataset.table))
        try:
            result = workflow.execute(row)
            for key in result:
                if isinstance(result[key], torch.Tensor) or isinstance(result[key], tf.Tensor):
                    arr = result[key].numpy()
                    result[key] = get_new_file_name(args.output, key, i) + '.npy'
                    try:
                        np.save(result[key], arr)
                    except:
                        logger.critical(f'Failed to save npy file with shape{arr.shape} to {result[key]}.')
                        raise
                elif isinstance(result[key], PIL.Image.Image):
                    image = result[key]
                    result[key] = get_new_file_name(args.output, key, i) + '.png'
                    try:
                        image.save(result[key])
                    except:
                        logger.critical(f'Failed to save PIL image with shape{image.shape} to {result[key]}.')
                        raise
            if first:
                header_order = result.keys()
                first = False
            result = sort_ordered_dict(result, header_order)
            if len(result.keys()) > len(header_order):
                header_order = result.keys()
            dataset.table[i].extend([str(col) for col in result.values()])
        except:
            raise
    progress(None)
    
    #save inference result
    output_result_file_name = os.path.join(args.output, 'output_result.csv')
    logger.log(99, f"Saving inference results in {output_result_file_name}") # Todo : change to info
    with open(output_result_file_name, 'w', newline='', encoding='utf-8-sig') as f:
        writer = csv.writer(f, quoting=csv.QUOTE_MINIMAL)
        dataset.header.extend(header_order)
        writer.writerow(dataset.header)    
        max_cols = max(len(row) for row in dataset.table)
        dataset.table = [row + [''] * (max_cols - len(row)) for row in dataset.table]
        writer.writerows(dataset.table)    
    
    logger.log(99, "Inference Completed.")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        '-c',
        '--config',
        help='path to Neural Network Console config file (ini)',
        required=True)
    parser.add_argument(
        '-o', '--output', help='output directory name', required=True)
    parser.add_argument(
        '-d', '--dataset', help='dataset file name dataset to be inferred')
    parser.set_defaults(func=inference)

    args = parser.parse_args()
    args.func(args)


if __name__ == '__main__':
    main()
