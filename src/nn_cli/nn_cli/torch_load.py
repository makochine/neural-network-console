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

import configparser
import torch
import zipfile
import os
import logging
import platform
from nn_cli.torch_dataset import CSVDataset
from nn_cli.torch_layer import Layer
from collections import OrderedDict


class ModelConfig:
    def __init__(self):
        self.global_config = None
        self.training_config = None
        self.datasets = OrderedDict()
        self.monitors = OrderedDict()
        self.executors = OrderedDict()
        self.optimizers = OrderedDict()
        self.schedulers = OrderedDict()
        self.networks = OrderedDict()

    def uri_preprocess(self, uri):
        sdc_path = os.path.dirname(self.sdc_file)
        uri = uri.replace("{SDCPATH}", f"{sdc_path}/")
        if platform.system() == "Linux":
            uri = uri.replace("\\", "/").replace("C:\\", "/c/")
        return uri

    def get_optimizer_dataset_name(self):
        return [optimizer.props['dataset_name'] for optimizer in self.optimizers.values()]

    def set_device(self, backend):
        if "cuda" in backend:
            if torch.cuda.is_available():
                if ':' in backend and int(backend.split(":")[1]) >= torch.cuda.device_count():
                    logging.warning(
                        f'Requested CUDA device {backend} is not available. Using default CUDA device: cuda:0')
                    self.global_config.default_context.device = torch.device("cuda:0")
                else:
                    self.global_config.default_context.device = torch.device(backend)
            else:
                logging.warning("CUDA is not available. Using CPU instead.")
                self.global_config.default_context.device = torch.device("cpu")
        else:
            self.global_config.default_context.device = torch.device("cpu")

    def make_ready(self):
        main_num_data = 0

        dataset_names = self.get_optimizer_dataset_name()

        # get the max size of all optimizers' dataset and replace the keys
        datasets = OrderedDict()
        for dataset in self.datasets.values():
            dataset.dataset_instance = CSVDataset(
                self.uri_preprocess(dataset.props["uri"]), dataset.props['image_norm'])
            if dataset.props['name'] in dataset_names:
                if main_num_data < len(dataset.dataset_instance):
                    main_num_data = len(dataset.dataset_instance)
            datasets[dataset.props['name']] = dataset
        self.datasets = datasets

        self.training_config.iter_per_epoch = int(main_num_data / self.training_config.batch_size)
        backend = self.global_config.default_context.backends[0]

        self.set_device(backend)

        networks = OrderedDict()
        for net in self.networks.values():
            networks[net.name] = net
        self.networks = networks

        # create loader for each dataset and replace the keys
        optimizers = OrderedDict()
        for opti_name, opti in self.optimizers.items():
            opti.dataset = self.datasets[opti.props["dataset_name"]]
            shuffle = opti.dataset.props["shuffle"]
            batch_size = self.training_config.batch_size
            opti.data_loader = torch.utils.data.dataloader.DataLoader(opti.dataset.dataset_instance,
                                                                      batch_size=batch_size,
                                                                      shuffle=shuffle)
            opti.network = self.networks[opti.props['network_name']]
            optimizers[opti_name] = opti
        self.optimizers = optimizers

        monitors = OrderedDict()
        for mon in self.monitors.values():
            mon.dataset = self.datasets[mon.props["dataset_name"]]
            batch_size = self.training_config.batch_size
            mon.data_loader = torch.utils.data.dataloader.DataLoader(mon.dataset.dataset_instance,
                                                                     batch_size=batch_size,
                                                                     shuffle=False)
            if mon.props['network_name'] not in self.networks:
                if mon.props['network_name'] == 'MainValidation':
                    mon.network = self.networks['Main']
                else:
                    raise ValueError("the network referred by monitor is not found!")
            else:
                mon.network = self.networks[mon.props['network_name']]
            monitors[mon.props["name"]] = mon
        self.monitors = monitors


class Network:
    def __init__(self):
        self.layers = None
        self.training_mode = True
        self.forward_without_loss = False
        self.lookup_table = {}
        self.links = []
        self.global_networks = {}
        self.weights = []

    def infer(self, data):
        output_refers = {}
        inputs = {}
        link_dict = {}
        link_layer_dict = {}
        for link in self.links:
            link_dict[link.props['ID']] = link
            source_layer = self.lookup_table[link.props['SourceLayerID']]
            out_name = source_layer.layer_output_name(link.props['SourcePinIndex'])
            if out_name not in output_refers:
                output_refers[out_name] = 1
            else:
                output_refers[out_name] += 1
            inputs[link.props['SourceLayerID']] = link.props['ID']
            link_layer_dict[link.props['SourceLayerID']] = link

        for link in self.links:
            if link.props['DestLayerID'] in inputs:
                del inputs[link.props['DestLayerID']]

            # check if a layer has a parameter link or muti inputs
            source_layer = self.lookup_table[link.props['SourceLayerID']]
            dest_layer = self.lookup_table[link.props['DestLayerID']]
            if source_layer.type == 'Parameter' or 'InputsNum' in dest_layer.props:
                dest_layer.props[dest_layer.layer_input_name(link.props['DestPinIndex'])] = 'True'

        # handle all input layers
        for layer_id in inputs:
            layer = self.lookup_table[layer_id]
            layer.create_and_infer(data)

        def from_output_to_input(source_layer, dest_layer):
            if source_layer.layer_output_name(link.props['SourcePinIndex']) in data:
                data[dest_layer.layer_input_name(link.props['DestPinIndex'])] = data[
                    source_layer.layer_output_name(link.props['SourcePinIndex'])]
                ref_count = output_refers[source_layer.layer_output_name(link.props['SourcePinIndex'])]
                ref_count -= 1
                output_refers[source_layer.layer_output_name(link.props['SourcePinIndex'])] = ref_count
                if ref_count <= 0:
                    # print(f"delete {source_layer.layer_output_name(link.props['SourcePinIndex'])}")
                    del data[source_layer.layer_output_name(link.props['SourcePinIndex'])]

        source_links = list(inputs.values())
        while True:
            if len(source_links) == 0:
                break
            link_id = source_links.pop(0)
            link = link_dict[link_id]
            while True:
                # print(f"{source_links}, current: {link.props['ID']}")
                source_layer = self.lookup_table[link.props['SourceLayerID']]
                dest_layer = self.lookup_table[link.props['DestLayerID']]
                # print(f"link id: {link.props['ID']}, {source_layer.props['Name']} -> {dest_layer.props['Name']}")
                # print(
                #     f"link id: {link.props['ID']}, source name: {source_layer.layer_output_name(link.props['SourcePinIndex'])}, dest name: {dest_layer.layer_input_name(link.props['DestPinIndex'])}")
                from_output_to_input(source_layer, dest_layer)
                next_link = link_layer_dict.get(link.props['DestLayerID'], None)
                if not dest_layer.create_and_infer(data):
                    if not dest_layer.muti_inputs_ready:
                        next_links = [link_ for _, link_ in link_dict.items() if
                                     link_.props["DestLayerID"] == link.props['DestLayerID'] and link_!=link].pop(0)
                        link = next_links
                        continue
                    else:
                        break
                if next_link is None:
                    break
                link = next_link

        if 'loss' in data and not self.forward_without_loss:
            return torch.sum(torch.stack(list(data['loss'].values())))
        return data

    def get_weight(self):
        for layer in self.layers.values():
            if layer.function_instance is not None:
                self.weights += layer.function_instance.parameters()
            if layer.unit_instance is not None:
                layer.unit_instance.get_weight()
                self.weights += layer.unit_instance.weights


    def train_mode(self):
        self.training_mode = True

    def infer_mode(self):
        self.training_mode = False


class Dataset:
    def __init__(self):
        self.props = {}

    @staticmethod
    def create_from_section(section):
        dataset = Dataset()
        dataset.process_section(section)
        return dataset

    def process_section(self, section):
        self.props = {
            "name": section['Name'],
            "uri": section['URI'],
            "shuffle": bool(section['Shuffle']),
            "enable_cache": bool(section["EnableCache"]),
            "image_norm": bool(section["ImageNormalization"]),
            "num_data": int(section["NumData"])
        }


class Executor:
    def __init__(self):
        self.props = {}

    @staticmethod
    def create_from_section(section):
        executor = Executor()
        executor.process_section(section)
        return executor

    def process_section(self, section):
        self.props = {
            "name": section['Executor_Name'],
            "network_name": section['Executor_NetworkName'],
            "dataset_name": section['Executor_DatasetName'],
            "num_eval": int(section["NumEvaluations"]),
            "repeat_eval_type": int(section["RepeatEvaluationType"]),
            "need_bp": bool(section["NeedBackPropagation"])
        }


class TrainConfig:
    def __init__(self):
        self.max_epoch = 0
        self.save_best = False
        self.batch_size = 1
        self.iter_per_epoch = 0
        self.monitor_interval = 0

    @staticmethod
    def create_from_section(section):
        train_config = TrainConfig()
        train_config.process_section(section)
        return train_config

    def process_section(self, section):
        self.max_epoch = int(section["MaxEpoch"])
        self.save_best = bool(section["SaveBest"])
        self.batch_size = int(section["BatchSize"])
        self.monitor_interval = int(section["MonitorInterval"])
        self.iter_per_epoch = 0


class Monitor:
    def __init__(self):
        self.props = {}
        self.network = None

    @staticmethod
    def create_from_section(section):
        monitor = Monitor()
        monitor.process_section(section)
        return monitor

    def process_section(self, section):
        self.props = {
            "name": section['Monitor_Name'],
            "network_name": section['Monitor_NetworkName'],
            "dataset_name": section['Monitor_DatasetName']
        }

    def monitor_variables(self, data):
        for k, v in data.items():
            if '_output_' in k:
                yield v


class OptimizerObject:
    def __init__(self):
        self.start_iter = 0
        self.end_iter = 0


class Optimizer:
    def __init__(self):
        self.props = {}
        self.solve_params = {}
        self.optimizer = OptimizerObject()
        self.opti_instance = None
        self.create_instance = None
        self.network = None

    @staticmethod
    def create_from_section(section):
        opti = Optimizer()
        opti.process_section(section)
        return opti

    def make_ready(self):
        if self.opti_instance is None:
            # weights = []
            # for layer in self.network.layers.values():
            #     if layer.function_instance is not None:
            #         weights += layer.function_instance.parameters()
            self.network.get_weight()
            self.opti_instance = self.create_instance(self.network.weights)

    def step(self):
        if self.opti_instance is not None:
            self.opti_instance.step()
            # current_lr = self.opti_instance.param_groups[0]['lr']
            # print(f'current learnning rate : {current_lr}')

    def get_solve_params(self, solve_params, section):
        for i in range(self.props['solve_param_num']):
            if section[f"SolverParameterName_{i}"] in solve_params:
                prop_name = section[f"SolverParameterName_{i}"]
                type_cls = solve_params[prop_name]
                if type_cls is not None:
                    self.solve_params[prop_name] = type_cls(section[f"SolverParameter_{i}"])

    def process_section(self, section):
        self.props = {
            "name": section['Optimizer_Name'],
            "network_name": section['Optimizer_NetworkName'],
            "dataset_name": section['Optimizer_DatasetName'],
            "solver_name": section['SolverName'],
            "update_interval": int(section['UpdateInterval']),
            "solve_param_num": int(section['SolverParameterNum']),
            "weight_decay": float(section['WeightDecay']),
            "lr_m": float(section['LearningRateMultiplier']),
            "lr_update_interval": int(section['LearningRateUpdateInterval'])
        }

        if self.props['solver_name'] == "Adam":
            solve_params = {
                "Alpha": float,
                "Beta1": float,
                "Beta2": float,
                "Epsilon": float,
                "Weight Decay": float,
            }
            self.get_solve_params(solve_params, section)

            def create_instance(weights):
                opti = torch.optim.Adam(weights,
                                        lr=self.solve_params['Alpha'],
                                        betas=(self.solve_params['Beta1'], self.solve_params['Beta2']),
                                        eps=self.solve_params['Epsilon'],
                                        weight_decay=self.solve_params['Weight Decay']
                                        # Others set as default
                                        )
                return opti

            self.create_instance = create_instance
        elif self.props['solver_name'] == "AdamW":
            solve_params = {
                "Alpha": float,
                "Beta1": float,
                "Beta2": float,
                "Epsilon": float,
                "Weight Decay": float,
            }
            self.get_solve_params(solve_params, section)

            def create_instance(weights):
                opti = torch.optim.AdamW(weights,
                                         lr=self.solve_params['Alpha'],
                                         betas=(self.solve_params['Beta1'], self.solve_params['Beta2']),
                                         eps=self.solve_params['Epsilon'],
                                         weight_decay=self.solve_params['Weight Decay']
                                         # Others set as default
                                         )
                return opti

            self.create_instance = create_instance
        elif self.props['solver_name'] == "Adamax":
            solve_params = {
                "LearningRate": float,
                "Beta1": float,
                "Beta2": float,
                "Epsilon": float,
                "Weight Decay": float
            }
            self.get_solve_params(solve_params, section)

            def create_instance(weights):
                opti = torch.optim.Adamax(weights,
                                          lr=self.solve_params['LearningRate'],
                                          betas=(self.solve_params['Beta1'], self.solve_params['Beta2']),
                                          eps=self.solve_params['Epsilon'],
                                          weight_decay=self.solve_params['Weight Decay']
                                          )
                return opti

            self.create_instance = create_instance
        elif self.props['solver_name'] == "AdaBound":
            solve_params = {
                "Alpha": float,
                "Beta1": float,
                "Beta2": float,
                "Epsilon": float,
                "FinalLR": float,
                "Gamma": float,
            }
            self.get_solve_params(solve_params, section)
        elif self.props['solver_name'] == "AMSGRAD":
            solve_params = {
                "Alpha": float,
                "Beta1": float,
                "Beta2": float,
                "Epsilon": float,
            }
            self.get_solve_params(solve_params, section)
        elif self.props['solver_name'] == "AMSBound":
            solve_params = {
                "Alpha": float,
                "Beta1": float,
                "Beta2": float,
                "Epsilon": float,
                "FinalLR": float,
                "Gamma": float,
            }
            self.get_solve_params(solve_params, section)
        elif self.props['solver_name'] == "Lars":
            solve_params = {
                "LearningRate": float,
                "Momentum": float,
                "Coefficient": float,
                "Epsilon": float,
            }
            self.get_solve_params(solve_params, section)
        elif self.props['solver_name'] == "Momentum":
            solve_params = {
                "LearningRate": float,
                "Momentum": float,
            }
            self.get_solve_params(solve_params, section)
        elif self.props['solver_name'] == "Nesterov":
            solve_params = {
                "LearningRate": float,
                "Momentum": float,
            }
            self.get_solve_params(solve_params, section)
        elif self.props['solver_name'] == "RMSprop":
            solve_params = {
                "LearningRate": float,
                "Decay": float,
            }
            self.get_solve_params(solve_params, section)
        elif self.props['solver_name'] == "Sgd":
            solve_params = {
                "LearningRate": float,
            }
            self.get_solve_params(solve_params, section)
        elif self.props['solver_name'] == "SgdW":
            solve_params = {
                "LearningRate": float,
                "Momentum": float,
                "Weight Decay": float,
            }
            self.get_solve_params(solve_params, section)
        elif self.props['solver_name'] == "AdaBelief":
            solve_params = {
                "Alpha": float,
                "Beta1": float,
                "Beta2": float,
                "Epsilon": float,
                "Weight Decay": float,
                "AMSGrad": float,
                "WD Decouple": float,
                "Fixed Decay": float,
                "Rectify": float,
            }
            self.get_solve_params(solve_params, section)
        elif self.props['solver_name'] == "Adadelta":
            solve_params = {
                "LearningRate": float,
                "Rho": float,
                "Epsilon": float,
                "Weight Decay": float,
            }
            self.get_solve_params(solve_params, section)

            def create_instance(weights):
                opti = torch.optim.Adadelta(weights,
                                            lr=self.solve_params['LearningRate'],
                                            rho=self.solve_params['Rho'],
                                            eps=self.solve_params['Epsilon'],
                                            weight_decay=self.solve_params['Weight Decay'],
                                            )
                return opti

            self.create_instance = create_instance
        elif self.props['solver_name'] == "Adagrad":
            solve_params = {
                "LearningRate": float,
                "Epsilon": float,
            }
            self.get_solve_params(solve_params, section)
        else:
            raise ValueError(f"solver {self.props['solver_name']}is not supported.")


class Scheduler:
    def __init__(self):
        self.props = {}
        self.optimizer = None
        self.scheduler_instance = None
        self.iter_per_epoch = 1
        self.warmup_scheduler_instance = None
        self.warmup_iters = 0
        self.inital_lr = 0
        self.scheduler_step = 0

    @staticmethod
    def create_from_section(section):
        scheduler = Scheduler()
        scheduler.process_section(section)
        return scheduler

    def make_ready(self, optimizer, iter_per_epoch):
        self.optimizer = optimizer.opti_instance
        if self.scheduler_instance is None:
            self.scheduler_instance = self.create_instance()
        self.iter_per_epoch = iter_per_epoch
        if self.props["LearningRateWarmupLength"] > 0:
            self.inital_lr = self.optimizer.defaults['lr']
            if self.props['LearningRateWarmupLengthUnit'] == 0:
                # Warmup length is in epochs
                self.warmup_iters = self.props['LearningRateWarmupLength'] * self.iter_per_epoch
            else:
                # Warmup length is in iterations
                self.warmup_iters = self.props['LearningRateWarmupLength']

    def step(self, iteration):
        # Warmup phase
        if iteration < self.warmup_iters:
            for param_group in self.optimizer.param_groups:
                param_group['lr'] = self.inital_lr * (iteration + 1) / self.warmup_iters
        # Main scheduler phase
        elif self.scheduler_instance:
            self.scheduler_step += 1
            if self.props['LearningRateUpdateIntervalUnit'] == 0:
                # Update LR per epoch
                if self.scheduler_step % self.iter_per_epoch == 0:
                    self.scheduler_instance.step(self.scheduler_step // self.iter_per_epoch)
            else:
                # Update LR per iteration
                self.scheduler_instance.step(self.scheduler_step)

    def get_scheduler_params(self, solve_params, section):
        for i in range(self.props['Solve_param_num']):
            if section[f"SolverParameterName_{i}"] in solve_params:
                prop_name = section[f"SolverParameterName_{i}"]
                type_cls = solve_params[prop_name]
                if type_cls is not None:
                    self.props[prop_name] = type_cls(section[f"SolverParameter_{i}"])

    def process_section(self, section):
        self.props = {
            "Scheduler_name": section['LearningRateScheduler'],
            "Solve_param_num": int(section['SolverParameterNum']),
            "LearningRateMultiplier": float(section['LearningRateMultiplier']),
            "UpdateInterval": int(section['LearningRateUpdateInterval']),
            "LearningRateUpdateIntervalUnit": int(section['LearningRateUpdateIntervalUnit']),
            "LearningRatePower": section['LearningRatePower'],
            "LearningRateSteps": section['LearningRateSteps'],
            "LearningRateWarmupScheduler": section['LearningRateWarmupScheduler'],
            "LearningRateWarmupLength": int(section['LearningRateWarmupLength']),
            "LearningRateWarmupLengthUnit": int(section['LearningRateWarmupLengthUnit']),
        }

        if self.props['Scheduler_name'] == "Exponential":
            scheduler_params = {
                "LearningRateMultiplier": float,
                "UpdateInterval": int,
                "LearningRateUpdateIntervalUnit": int,  # 0:epoch ; 1:iteration,
                "LearningRateWarmupLength": int,
                "LearningRateWarmupLengthUnit": int  # 0:epoch ; 1:iteration,
            }
            self.get_scheduler_params(scheduler_params, section)

            def create_instance():
                scheduler = torch.optim.lr_scheduler.ExponentialLR(self.optimizer,
                                                                   gamma=self.props[
                                                                       'LearningRateMultiplier']
                                                                   )
                return scheduler

            self.create_instance = create_instance
        elif self.props['Scheduler_name'] == "":
            # Define a dummy scheduler that does nothing (keep solid learning rate)
            class DummyScheduler(torch.optim.lr_scheduler._LRScheduler):
                def __init__(self, optimizer):
                    super(DummyScheduler, self).__init__(optimizer)

                def step(self, epoch=None):
                    pass

            def create_instance(optimizer):
                scheduler = DummyScheduler(optimizer)
                return scheduler

            self.create_instance = create_instance
        else:
            raise ValueError(f"scheduler {self.props['Scheduler_name']}is not supported.")


class Link:
    keynames = [
        "ID",
        "SourceLayerID",
        "SourcePinIndex",
        "DestLayerID",
        "DestPinIndex"
    ]

    def __init__(self):
        self.props = {}


class SDCLoader:
    def __init__(self, sdc_file):
        self.sdc_proj = configparser.ConfigParser()
        self.sdc_file = sdc_file
        with open(sdc_file, 'r') as f:
            self.sdc_proj.read_file(f)

    def build_proto_network(self, network_name):
        network = Network()
        network.layers = OrderedDict()
        network.name = network_name if network_name != '' else "Main"
        sect_name = f"{network_name}_Layer_" if network_name != '' else "Layer_"
        network_global = f"{network_name}_Network_Global" if network_name != '' else "Network_Global"
        network.num_layers = int(self.sdc_proj[network_global]['NumLayer'])
        network.num_links = int(self.sdc_proj[network_global]['NumLink'])
        for section_name, layer_name in self.find_section_startswith(sect_name):
            network.layers[layer_name] = Layer.create_from_type(network,
                                                                self.sdc_proj[section_name]["type"],
                                                                self.sdc_proj[section_name])
        link_name = f"{network_name}_Link_" if network_name != '' else "Link_"
        network.links = []
        for section_name, link_name in self.find_section_startswith(link_name):
            link = Link()
            for keyname in Link.keynames:
                link.props[keyname] = int(self.sdc_proj[section_name][keyname])
            network.links.append(link)
        return network

    def find_similar_section(self, pattern):
        for s in self.sdc_proj.sections():
            if pattern in s:
                clear_name = s.replace(f"{pattern}", '')
                if clear_name == '':
                    pass
                elif clear_name[-1] == '_':
                    clear_name = clear_name[:-1]
                yield s, clear_name
            else:
                continue

    def find_section_startswith(self, pattern):
        for s in self.sdc_proj.sections():
            if s.startswith(pattern):
                clear_name = s.replace(f"{pattern}", '')
                if clear_name == '':
                    pass
                elif clear_name[-1] == '_':
                    clear_name = clear_name[:-1]
                yield s, clear_name
            else:
                continue

    def build_network(self, model_config):
        model_config.networks = OrderedDict()
        for section_name, network_name in self.find_similar_section("Network_Global"):
            model_config.networks[section_name] = self.build_proto_network(network_name)
        for network in model_config.networks.values():
            network.global_networks = model_config.networks

    def build_optimizer(self, model_config):
        model_config.optimizers = OrderedDict()
        for section_name, optimizer_name in self.find_similar_section("Optimizer"):
            model_config.optimizers[section_name] = Optimizer.create_from_section(self.sdc_proj[section_name])

    def build_scheduler(self, model_config):
        model_config.schedulers = OrderedDict()
        for section_name, scheduler_name in self.find_similar_section("Optimizer"):
            model_config.schedulers[section_name] = Scheduler.create_from_section(self.sdc_proj[section_name])

    def build_monitor(self, model_config):
        model_config.monitors = OrderedDict()
        for section_name, monitor_name in self.find_similar_section("Monitor"):
            model_config.monitors[section_name] = Monitor.create_from_section(self.sdc_proj[section_name])

    def build_executor(self, model_config):
        model_config.executors = OrderedDict()
        for section_name, executor_name in self.find_similar_section("Executor"):
            model_config.executors[section_name] = Executor.create_from_section(self.sdc_proj[section_name])

    def build_dataset(self, model_config):
        model_config.datasets = OrderedDict()
        for section_name, executor_name in self.find_similar_section("Dataset_"):
            model_config.datasets[section_name] = Dataset.create_from_section(self.sdc_proj[section_name])

    def build_config(self, model_config):
        model_config.training_config = TrainConfig.create_from_section(self.sdc_proj["Config"])

    def build_global_config(self, model_config, context):
        def get_context(context):
            if "cpu" in context:
                return "cpu", 0
            else:
                s = context.split(":")
                dev_id = 0 if len(s) == 1 else int(s[1])
                if dev_id == -1:
                    dev_id = 0
                return "cuda", dev_id

        class GlobalConfig:
            pass

        class DefaultContext:
            pass

        global_config = GlobalConfig()
        global_config.default_context = DefaultContext()

        cpu_or_cuda, device_id = get_context(context)
        global_config.default_context.device_id = device_id
        if cpu_or_cuda == "cpu":
            global_config.default_context.backends = ["cpu:float"]
        else:
            global_config.default_context.backends = [f"cuda:{device_id}"]
        model_config.global_config = global_config

    def load(self, context):
        model_config = ModelConfig()
        model_config.sdc_file = self.sdc_file
        self.build_network(model_config)
        self.build_optimizer(model_config)
        self.build_scheduler(model_config)
        self.build_monitor(model_config)
        self.build_dataset(model_config)
        self.build_executor(model_config)
        self.build_config(model_config)
        self.build_global_config(model_config, context)

        return model_config


def load_from_sdcproj(sdc_file, context):
    conf = SDCLoader(sdc_file).load(context)
    conf.make_ready()
    return conf


def load_from_nnpt(nnpt_file, context):
    proj_dir = os.path.dirname(nnpt_file)
    tmp_dir = os.path.join(proj_dir, "nnpt.tmp")
    if not os.path.exists(tmp_dir):
        os.makedirs(tmp_dir)
    with zipfile.ZipFile(nnpt_file, 'r') as zip_ref:
        sdcproj_file = [file for file in zip_ref.namelist() if file.endswith('.sdcproj')][0]
        # Extract the sdcproj and h5 files to a temporary directory
        zip_ref.extractall(tmp_dir)
    # Load configuration from the extracted sdcproj file
    sdc_file = os.path.join(tmp_dir, sdcproj_file)
    conf = SDCLoader(sdc_file).load(context)
    conf.make_ready()
    return conf
