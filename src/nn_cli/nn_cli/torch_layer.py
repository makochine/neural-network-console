# Copyright 2024,2025 Sony Group Corporation.
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

import os.path
import torch
import torchvision
import random
import torch.nn as nn
import numpy as np
import nn_cli.torch_custom_layers as custom_layers


def to_shape(input_string):
    output_shape = tuple([int(eval(v)) for v in input_string.split(',')])
    return output_shape


def load_param_in_txt(filepath):
    if not os.path.exists(filepath):
        return None
    with open(filepath, 'r') as input:
        ds = input.read()
    ds = ds.split("\n")
    shape = eval(ds.pop(0))
    return torch.from_numpy(np.fromstring("\n".join(ds),
                                          dtype=np.float32,
                                          sep="\n").reshape(shape))


def get_network_name(layer_name):
    if layer_name.startswith("Layer_"):
        return "Main"
    else:
        return layer_name.split('_Layer_')[0]


class Layer:
    def Input_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "ID": (int, 0),
                "Name": None,
                "Dataset": None,
                "Size": (to_shape, (1, 28, 28))
            })
        elif command == "create_function":
            # do nothing for Input
            data = args[0]
            variable_name = self.props['Dataset']
            data[self.layer_output_name(0)] = data[variable_name].to(torch.float32)
            return True

    def Constant_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Value": None,
                "ValueType": None
            })
        elif command == "create_function":
            pass

    def Dropout_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None,
                "P": (float, 0.5),
                "SkipAtTest": (bool, True),
            })
        elif command == "create_function":
            input_tensor = args[0][self.layer_input_name(0)]
            if self.function_instance is None:
                self.function_instance = nn.Dropout(float(self.props['P'])).to(input_tensor.device)
            if not self.owner.training_mode and self.props["SkipAtTest"]:
                self.function_instance.eval()
            else:
                self.function_instance.train()
            args[0][self.layer_output_name(0)] = self.function_instance(input_tensor)
            self.clear_inputs(args[0])
            return True

    def Linear_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None,
                "OutFeatures": (int, 16),
                "WithBias": (bool, True),
                "W.File": None,
                "b.File": None
            })
        elif command == "create_function":
            input_tensor = args[0][self.layer_input_name(0)]
            for i in [1, 2]:
                if not self.check_inputs(self.layer_input_name(i), args[0]):
                    return False

            if self.props['W.File']:
                weight_tensor = load_param_in_txt(self.props['W.File'])
            else:
                weight_tensor = args[0].get(self.layer_input_name(1), None)

            if self.props['b.File']:
                bias_tensor = load_param_in_txt(self.props['b.File'])
            else:
                bias_tensor = args[0].get(self.layer_input_name(2), None)

            input_shape = input_tensor.shape
            if self.function_instance is None:
                self.function_instance = nn.Linear(input_shape[-1],
                                                   self.props['OutFeatures'],
                                                   bias=self.props['WithBias']
                                                   ).to(input_tensor.device)

            with torch.no_grad():
                if 'state_dict' in self.props and self.props['state_dict']:
                    self.load_state()
                    del self.props['state_dict']
                else:
                    if weight_tensor is not None:
                        self.function_instance.weight.copy_(weight_tensor)
                        self.props['W.File'] = None
                    if bias_tensor is not None:
                        self.function_instance.bias.copy_(bias_tensor)
                        self.props['b.File'] = None

            if self.owner.training_mode:
                self.function_instance.train()
            else:
                self.function_instance.eval()
            args[0][self.layer_output_name(0)] = self.function_instance(input_tensor)
            self.clear_inputs(args[0])
            return True

    def Parameter_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None,
                "File": None,
                "Size": (to_shape, (16,)),
                "Initializer": None,
                "InitializerMultiplier": (float, 1.0),
                "LRateMultiplier": (float, 1.0),
            })
        elif command == "create_function":
            if 'HasEval' not in self.props and self.owner.training_mode:
                args[0][self.layer_output_name(0)] = load_param_in_txt(self.props['File'])
                self.props['HasEval'] = True
            else:
                args[0][self.layer_output_name(0)] = None
            return True

    def Reshape_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None,
                "OutShape": (to_shape, (16,)),
            })
        elif command == "create_function":
            input_tensor = args[0][self.layer_input_name(0)]
            if self.function_instance is None:
                target_shape = (input_tensor.shape[0],) + self.props["OutShape"]
                self.function_instance = custom_layers.Reshape(target_shape)
            args[0][self.layer_output_name(0)] = self.function_instance(input_tensor)
            self.clear_inputs(args[0])
            return True

    def Identity_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None,
                "OutShape": (to_shape, (16,)),
            })
        elif command == "create_function":
            input_tensor = args[0][self.layer_input_name(0)]
            if self.function_instance is None:
                self.function_instance = nn.Identity()
            args[0][self.layer_output_name(0)] = self.function_instance(input_tensor)
            self.clear_inputs(args[0])
            return True

    def Constant_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None,
                "ValueType": (str, 'Float'),
                "Value": None,
            })
        elif command == "create_function":
            if self.props['ValueType'] == "Float":
                args[0][self.layer_output_name(0)] = float(self.props["Value"])
            elif self.props['ValueType'] == "Int":
                args[0][self.layer_output_name(0)] = int(self.props["Value"])
            else:
                args[0][self.layer_output_name(0)] = str(self.props["Value"])
            self.clear_inputs(args[0])
            return True

    def Unit_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None,
                "Input_name": None,
                "Network": (str, ''),
            })
        elif command == "create_function":
            input_tensor = args[0][self.layer_input_name(0)]
            # Create unit network instance
            if self.unit_instance is None:
                self.create_unit_instance(input_tensor)
            output_data = self.unit_instance.infer({self.props['Input_name']: input_tensor})
            args[0][self.layer_output_name(0)] = next(iter(output_data.values()))
            self.clear_inputs(args[0])
            return True

    def RepeatBlock_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None,
                "Input_name": None,
                "Network": (str, ''),
                'Times': (int, 2)
            })
        elif command == "create_function":
            input_tensor = args[0][self.layer_input_name(0)]
            # Create unit network instance
            if self.unit_instance is None:
                self.create_unit_instance(input_tensor)
            for i in range(self.props['Times']):
                output_data = self.unit_instance.infer({self.props['Input_name']: input_tensor})
                input_tensor = next(iter(output_data.values()))
            args[0][self.layer_output_name(0)] = next(iter(output_data.values()))
            self.clear_inputs(args[0])
            return True

    def AdaptiveSeparableConvolution_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None,
                "KernelShape": (to_shape, "3,3"),
                "V.Dataset": None,
                "V.Generator": None,
                "V.GeneratorMultiplier": (float, 1.0),
                "H.Dataset": None,
                "H.Generator": None,
                "H.GeneratorMultiplier": (float, 1.0),
            })
        elif command == "create_function":
            pass

    def Convolution_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None,
                "OutMaps": (int, 16),
                "KernelShape": (to_shape, "3,3"),
                "BorderMode": None,
                "Padding": (to_shape, "0,0"),
                "Strides": (to_shape, "1,1"),
                "Dilation": (to_shape, "1,1"),
                "Group": (int, 1),
                "BaseAxis": (int, 0),
                "WithBias": (bool, True),
                "W.Initializer": None,
                "W.InitializerMultiplier": (float, 1.0),
                "W.LRateMultiplier": (float, 1.0),
                "b.Initializer": None,
                "b.InitializerMultiplier": (float, 1.0),
                "b.LRateMultiplier": (float, 1.0),
                "W.File": None,
                "b.File": None
            })
        elif command == "create_function":
            input_tensor = args[0][self.layer_input_name(0)]
            for i in [1, 2]:
                if not self.check_inputs(self.layer_input_name(i), args[0]):
                    return False
            input_shape = input_tensor.shape

            if self.props['W.File']:
                weight_tensor = load_param_in_txt(self.props['W.File'])
            else:
                weight_tensor = args[0].get(self.layer_input_name(1), None)

            if self.props['b.File']:
                bias_tensor = load_param_in_txt(self.props['b.File'])
            else:
                bias_tensor = args[0].get(self.layer_input_name(2), None)

            if self.function_instance is None:
                stride = self.props['Strides']
                kernel_shape = self.props['KernelShape']
                dilation = self.props['Dilation']
                border_mode = self.props['BorderMode']

                if isinstance(self.props['Padding'], tuple):
                    padding_shape = self.props['Padding']
                else:
                    if border_mode == 'same':
                        # Calculate padding for "same" border mode
                        padding_shape = []
                        for dim in range(len(input_shape) - 2):
                            padding = ((input_shape[dim + 2] - 1) + (kernel_shape[dim] - 1) * dilation[
                                dim] + 1 - input_shape[dim + 2]) // 2
                            padding_shape.append(padding)
                        padding_shape = tuple(padding_shape)
                    elif border_mode == 'valid':
                        # No padding for "valid" border mode
                        padding_shape = tuple([0] * len(stride))
                    elif border_mode == 'full':
                        # Calculate padding for "full" border mode
                        padding_shape = []
                        for dim in range(len(input_shape) - 2):
                            padding = ((input_shape[dim + 2] - 1) + (kernel_shape[dim] - 1) * dilation[
                                dim] + 1 -
                                       input_shape[dim + 2])
                            padding_shape.append(padding)
                        padding_shape = tuple(padding_shape)
                    else:
                        raise ValueError("Unsupported border mode")

                # Decide the convolution class based on the input size
                conv_classes = {3: nn.Conv1d, 4: nn.Conv2d, 5: nn.Conv3d}
                conv_class = conv_classes[len(input_shape)]

                self.function_instance = conv_class(in_channels=input_shape[1],
                                                    out_channels=self.props['OutMaps'],
                                                    kernel_size=kernel_shape,
                                                    stride=stride,
                                                    padding=padding_shape,
                                                    groups=self.props['Group'],
                                                    dilation=dilation
                                                    ).to(input_tensor.device)
            with torch.no_grad():
                if 'state_dict' in self.props and self.props['state_dict']:
                    self.load_state()
                    del self.props['state_dict']
                else:
                    if weight_tensor is not None:
                        self.function_instance.weight.copy_(weight_tensor)
                        self.props['W.File'] = None
                    if bias_tensor is not None:
                        self.function_instance.bias.copy_(bias_tensor)
                        self.props['b.File'] = None

            if self.owner.training_mode:
                self.function_instance.train()
            else:
                self.function_instance.eval()
            args[0][self.layer_output_name(0)] = self.function_instance(input_tensor)
            self.clear_inputs(args[0])
            return True

    def Deconvolution_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None,
                "OutMaps": (int, 16),
                "KernelShape": (to_shape, "3,3"),
                "Padding": (to_shape, "0,0"),
                "Strides": (to_shape, "1,1"),
                "Dilation": (to_shape, "1,1"),
                "Group": (int, 1),
                "BaseAxis": (int, 0),
                "WithBias": (bool, True),
                "W.Initializer": None,
                "W.InitializerMultiplier": (float, 1.0),
                "W.LRateMultiplier": (float, 1.0),
                "b.Initializer": None,
                "b.InitializerMultiplier": (float, 1.0),
                "b.LRateMultiplier": (float, 1.0),
                "W.File": None,
                "b.File": None
            })
        elif command == "create_function":
            input_tensor = args[0][self.layer_input_name(0)]
            for i in [1, 2]:
                if not self.check_inputs(self.layer_input_name(i), args[0]):
                    return False
            input_shape = input_tensor.shape

            if self.props['W.File']:
                weight_tensor = load_param_in_txt(self.props['W.File'])
            else:
                weight_tensor = args[0].get(self.layer_input_name(1), None)

            if self.props['b.File']:
                bias_tensor = load_param_in_txt(self.props['b.File'])
            else:
                bias_tensor = args[0].get(self.layer_input_name(2), None)

            if self.function_instance is None:
                # Decide the transposed convolution class based on the input size
                conv_transpose_classes = {3: nn.ConvTranspose1d, 4: nn.ConvTranspose2d, 5: nn.ConvTranspose3d}
                conv_transpose_class = conv_transpose_classes[len(input_shape)]
                self.function_instance = conv_transpose_class(in_channels=input_shape[1],
                                                              out_channels=self.props['OutMaps'],
                                                              kernel_size=self.props['KernelShape'],
                                                              stride=self.props['Strides'],
                                                              padding=self.props['Padding'],
                                                              groups=self.props['Group'],
                                                              dilation=self.props['Dilation'],
                                                              ).to(input_tensor.device)

            with torch.no_grad():
                if 'state_dict' in self.props and self.props['state_dict']:
                    self.load_state()
                    del self.props['state_dict']
                else:
                    if weight_tensor is not None:
                        self.function_instance.weight.copy_(weight_tensor)
                        self.props['W.File'] = None
                    if bias_tensor is not None:
                        self.function_instance.bias.copy_(bias_tensor)
                        self.props['b.File'] = None

            if self.owner.training_mode:
                self.function_instance.train()
            else:
                self.function_instance.eval()
            args[0][self.layer_output_name(0)] = self.function_instance(input_tensor)
            self.clear_inputs(args[0])
            return True

    def DeformableConvolution_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": "name",
                "OutMaps": "outmaps",
                "KernelShape": "kernel_shape",
                "BorderMode": "border_mode",
                "Padding": "padding",
                "Strides": "strides",
                "Dilation": "dilation",
                "Group": "group",
                "DeformableGroup": "deformable_group",
                "ChannelLast": "channel_last",
                "BaseAxis": "base_axis",
                "WithBias": "with_bias",
                "ParameterScope": "parameter_scope",
                "W.File": "w_file",
                "W.Initializer": "w_initializer",
                "W.InitializerMultiplier": "w_m",
                "W.LRateMultiplier": "w_lr_m",
                "Offset.Dataset": "offset_dataset",
                "Offset.Generator": "offset_generator",
                "Offset.GeneratorMultiplier": "offset_generator_multiplier",
                "Mask.Dataset": "mask_dataset",
                "Mask.Generator": "mask_generator",
                "Mask.GeneratorMultiplier": "mask_generator_multiplier",
                "b.File": "b_file",
                "b.Initializer": "b_initializer",
                "b.InitializerMultiplier": "b_m",
                "b.LRateMultiplier": "b_lr_m"
            })
        elif command == "create_function":
            pass

    def DepthwiseConvolution_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None,
                "OutMaps": (int, 16),
                "KernelShape": (to_shape, "3,3"),
                "BorderMode": None,
                "Padding": (to_shape, "0,0"),
                "Strides": (to_shape, "1,1"),
                "Dilation": (to_shape, "1,1"),
                "Group": (int, 1),
                "BaseAxis": (int, 0),
                "Multiplier": (int, 1),
                "WithBias": (bool, True),
                "W.Initializer": None,
                "W.InitializerMultiplier": (float, 1.0),
                "W.LRateMultiplier": (float, 1.0),
                "b.Initializer": None,
                "b.InitializerMultiplier": (float, 1.0),
                "b.LRateMultiplier": (float, 1.0),
                "W.File": None,
                "b.File": None,
            })
        elif command == "create_function":
            input_tensor = args[0][self.layer_input_name(0)]
            for i in [1, 2]:
                if not self.check_inputs(self.layer_input_name(i), args[0]):
                    return False
            input_shape = input_tensor.shape

            if self.props['W.File']:
                weight_tensor = load_param_in_txt(self.props['W.File'])
            else:
                weight_tensor = args[0].get(self.layer_input_name(1), None)

            if self.props['b.File']:
                bias_tensor = load_param_in_txt(self.props['b.File'])
            else:
                bias_tensor = args[0].get(self.layer_input_name(2), None)

            if self.function_instance is None:
                stride = self.props['Strides']
                kernel_shape = self.props['KernelShape']
                dilation = self.props['Dilation']
                border_mode = self.props['BorderMode']
                if type(self.props['Padding']) == tuple:
                    padding_shape = self.props['Padding']
                else:
                    if border_mode == 'same':
                        # Calculate padding for "same" border mode
                        padding_height = ((input_shape[2] - 1) + (kernel_shape[0] - 1) * dilation[0] + 1 -
                                          input_shape[2]) // 2
                        padding_width = ((input_shape[3] - 1) * stride[1] + (kernel_shape[1] - 1) * dilation[1] + 1 -
                                         input_shape[3]) // 2
                        padding_shape = (padding_height, padding_width)
                    elif border_mode == 'valid':
                        # No padding for "valid" border mode
                        padding_shape = (0, 0)
                    elif border_mode == 'full':
                        # Calculate padding for "full" border mode
                        padding_height = (input_shape[2] - 1) + (kernel_shape[0] - 1) * dilation[0] + 1 - \
                                         input_shape[2]
                        padding_width = (input_shape[3] - 1) * stride[1] + (kernel_shape[1] - 1) * dilation[1] + 1 - \
                                        input_shape[3]
                        padding_shape = (padding_height, padding_width)
                    else:
                        raise ValueError("Unsupported border mode")

                self.function_instance = nn.Conv2d(in_channels=input_shape[1],
                                                   out_channels=input_shape[1] * self.props['Multiplier'],
                                                   kernel_size=kernel_shape,
                                                   stride=stride,
                                                   padding=padding_shape,
                                                   groups=input_shape[1],
                                                   dilation=dilation
                                                   ).to(input_tensor.device)
            with torch.no_grad():
                if 'state_dict' in self.props and self.props['state_dict']:
                    self.load_state()
                    del self.props['state_dict']
                else:
                    if weight_tensor is not None:
                        self.function_instance.weight.copy_(weight_tensor)
                        self.props['W.File'] = None
                    if bias_tensor is not None:
                        self.function_instance.bias.copy_(bias_tensor)
                        self.props['b.File'] = None

            if self.owner.training_mode:
                self.function_instance.train()
            else:
                self.function_instance.eval()
            args[0][self.layer_output_name(0)] = self.function_instance(input_tensor)
            self.clear_inputs(args[0])
            return True

    def DepthwiseDeconvolution_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None,
                "OutMaps": (int, 16),
                "KernelShape": (to_shape, "3,3"),
                "BorderMode": None,
                "Padding": (to_shape, "0,0"),
                "Strides": (to_shape, "1,1"),
                "Dilation": (to_shape, "1,1"),
                "Group": (int, 1),
                "BaseAxis": (int, 0),
                "Divisor": (int, 1),
                "WithBias": (bool, True),
                "W.Initializer": None,
                "W.InitializerMultiplier": (float, 1.0),
                "W.LRateMultiplier": (float, 1.0),
                "b.Initializer": None,
                "b.InitializerMultiplier": (float, 1.0),
                "b.LRateMultiplier": (float, 1.0),
                "W.File": None,
                "b.File": None,
            })
        elif command == "create_function":
            input_tensor = args[0][self.layer_input_name(0)]
            for i in [1, 2]:
                if not self.check_inputs(self.layer_input_name(i), args[0]):
                    return False
            input_shape = input_tensor.shape

            if self.props['W.File']:
                weight_tensor = load_param_in_txt(self.props['W.File'])
            else:
                weight_tensor = args[0].get(self.layer_input_name(1), None)

            if self.props['b.File']:
                bias_tensor = load_param_in_txt(self.props['b.File'])
            else:
                bias_tensor = args[0].get(self.layer_input_name(2), None)

            if self.function_instance is None:
                out_channels = int(input_shape[1] / self.props['Divisor'])
                conv_transpose_classes = {3: nn.ConvTranspose1d, 4: nn.ConvTranspose2d, 5: nn.ConvTranspose3d}
                self.function_instance = conv_transpose_classes[len(input_shape)](in_channels=input_shape[1],
                                                                                  out_channels=out_channels,
                                                                                  kernel_size=self.props['KernelShape'],
                                                                                  stride=self.props['Strides'],
                                                                                  padding=self.props['Padding'],
                                                                                  groups=out_channels,
                                                                                  dilation=self.props['Dilation'],
                                                                                  ).to(input_tensor.device)

            with torch.no_grad():
                if 'state_dict' in self.props and self.props['state_dict']:
                    self.load_state()
                    del self.props['state_dict']
                else:
                    if weight_tensor is not None:
                        self.function_instance.weight.copy_(weight_tensor)
                        self.props['W.File'] = None
                    if bias_tensor is not None:
                        self.function_instance.bias.copy_(bias_tensor)
                        self.props['b.File'] = None

            if self.owner.training_mode:
                self.function_instance.train()
            else:
                self.function_instance.eval()
            args[0][self.layer_output_name(0)] = self.function_instance(input_tensor)
            self.clear_inputs(args[0])
            return True

    def Embed_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None,
                "NumClass": (int, 100),
                "Shape": (int, 16),
                "W.File": None
            })
        elif command == "create_function":
            input_tensor = args[0][self.layer_input_name(0)]

            if self.props['W.File']:
                weight_tensor = load_param_in_txt(self.props['W.File'])
            else:
                weight_tensor = args[0].get(self.layer_input_name(1), None)

            if self.function_instance is None:
                self.function_instance = nn.Embedding(num_embeddings=self.props['NumClass'],
                                                      embedding_dim=self.props['Shape'],
                                                      ).to(input_tensor.device)

            with torch.no_grad():
                if 'state_dict' in self.props and self.props['state_dict']:
                    self.load_state()
                    del self.props['state_dict']
                if weight_tensor is not None:
                    self.function_instance.weight.copy_(weight_tensor)
                    self.props['W.File'] = None

            if self.owner.training_mode:
                self.function_instance.train()
            else:
                self.function_instance.eval()
            args[0][self.layer_output_name(0)] = self.function_instance(input_tensor)
            self.clear_inputs(args[0])
            return True

    def Abs_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None,
            }, )
        elif command == "create_function":
            input_tensor = args[0][self.layer_input_name(0)]
            args[0][self.layer_output_name(0)] = torch.abs(input_tensor).to(input_tensor.device)
            self.clear_inputs(args[0])
            return True

    def CELU_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None,
                "Alpha": (float, 0.1),
                "InPlace": (bool, False)
            }, )
        elif command == "create_function":
            input_tensor = args[0][self.layer_input_name(0)]
            if self.function_instance is None:
                self.function_instance = nn.CELU(
                    alpha=self.props['Alpha'],
                    inplace=self.props['InPlace']).to(input_tensor.device)
            if self.owner.training_mode:
                self.function_instance.train()
            else:
                self.function_instance.eval()
            args[0][self.layer_output_name(0)] = self.function_instance(input_tensor)
            self.clear_inputs(args[0])
            return True

    def CReLU_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None,
                "Axis": (int, -1),
            })
        elif command == "create_function":
            input_tensor = args[0][self.layer_input_name(0)]
            if self.function_instance is None:
                self.function_instance = custom_layers.CReLU(
                    axis=self.props['Axis']
                ).to(input_tensor.device)
            args[0]["output"] = self.function_instance(input_tensor)
            if self.owner.training_mode:
                self.function_instance.train()
            else:
                self.function_instance.eval()
            args[0][self.layer_output_name(0)] = self.function_instance(input_tensor)
            self.clear_inputs(args[0])
            return True

    def GELU_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None,
            }, )
        elif command == "create_function":
            input_tensor = args[0][self.layer_input_name(0)]
            if self.function_instance is None:
                self.function_instance = nn.GELU().to(input_tensor.device)
            if self.owner.training_mode:
                self.function_instance.train()
            else:
                self.function_instance.eval()
            args[0][self.layer_output_name(0)] = self.function_instance(input_tensor)
            self.clear_inputs(args[0])
            return True

    def SoftPlus_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None,
            }, )
        elif command == "create_function":
            input_tensor = args[0][self.layer_input_name(0)]
            if self.function_instance is None:
                self.function_instance = nn.Softplus().to(input_tensor.device)
            if self.owner.training_mode:
                self.function_instance.train()
            else:
                self.function_instance.eval()
            args[0][self.layer_output_name(0)] = self.function_instance(input_tensor)
            self.clear_inputs(args[0])
            return True

    def HardSigmoid_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None,
            })
        elif command == "create_function":
            input_tensor = args[0][self.layer_input_name(0)]
            if self.function_instance is None:
                self.function_instance = nn.Hardsigmoid()
            if self.owner.training_mode:
                self.function_instance.train()
            else:
                self.function_instance.eval()
            args[0][self.layer_output_name(0)] = self.function_instance(input_tensor)
            self.clear_inputs(args[0])
            return True

    def LeakyReLU_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None,
                "Alpha": (float, 0.1),
                "InPlace": (bool, False)
            }, )
        elif command == "create_function":
            input_tensor = args[0][self.layer_input_name(0)]
            if self.function_instance is None:
                self.function_instance = nn.LeakyReLU(
                    negative_slope=self.props['Alpha'],
                    inplace=self.props['InPlace']).to(input_tensor.device)
            if self.owner.training_mode:
                self.function_instance.train()
            else:
                self.function_instance.eval()
            args[0][self.layer_output_name(0)] = self.function_instance(input_tensor)
            self.clear_inputs(args[0])
            return True

    def ELU_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None,
                "Alpha": (float, 0.1),
                "InPlace": (bool, False)
            }, )
        elif command == "create_function":
            input_tensor = args[0][self.layer_input_name(0)]
            if self.function_instance is None:
                self.function_instance = nn.ELU(
                    alpha=self.props['Alpha'],
                    inplace=self.props['InPlace']).to(input_tensor.device)
            if self.owner.training_mode:
                self.function_instance.train()
            else:
                self.function_instance.eval()
            args[0][self.layer_output_name(0)] = self.function_instance(input_tensor)
            self.clear_inputs(args[0])
            return True

    def LogSigmoid_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None,
            })
        elif command == "create_function":
            input_tensor = args[0][self.layer_input_name(0)]
            if self.function_instance is None:
                self.function_instance = nn.LogSigmoid()
            if self.owner.training_mode:
                self.function_instance.train()
            else:
                self.function_instance.eval()
            args[0][self.layer_output_name(0)] = self.function_instance(input_tensor)
            self.clear_inputs(args[0])
            return True

    def LogSoftmax_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None,
                "Axis": (int, 0),
            }, )
        elif command == "create_function":
            input_tensor = args[0][self.layer_input_name(0)]
            if self.function_instance is None:
                self.function_instance = nn.LogSoftmax(dim=self.props['Axis']).to(input_tensor.device)
            if self.owner.training_mode:
                self.function_instance.train()
            else:
                self.function_instance.eval()
            args[0][self.layer_output_name(0)] = self.function_instance(input_tensor)
            self.clear_inputs(args[0])
            return True

    def FFT_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None,
                "SignalNdim": (int, 2),
                "Normalized": (bool, False),
            }, )
        elif command == "create_function":
            input_tensor = args[0][self.layer_input_name(0)]
            if self.props["Normalized"]:
                norm_mode = 'ortho'
            else:
                norm_mode = 'backward'
            args[0][self.layer_output_name(0)] = torch.fft.fftn(input=input_tensor,
                                                                s=self.props["SignalNdim"],
                                                                norm=norm_mode).to(input_tensor.device)
            self.clear_inputs(args[0])
            return True

    def IFFT_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None,
                "SignalNdim": (int, 2),
                "Normalized": (bool, False),
            }, )
        elif command == "create_function":
            input_tensor = args[0][self.layer_input_name(0)]
            if self.props["Normalized"]:
                norm_mode = 'ortho'
            else:
                norm_mode = 'backward'
            args[0][self.layer_output_name(0)] = torch.fft.ifftn(input=input_tensor,
                                                                 s=self.props["SignalNdim"],
                                                                 norm=norm_mode).to(input_tensor.device)
            self.clear_inputs(args[0])
            return True

    def Mish_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None
            }, )
        elif command == "create_function":
            input_tensor = args[0][self.layer_input_name(0)]
            if self.function_instance is None:
                # SiLU is also known as the swish.
                self.function_instance = nn.Mish().to(input_tensor.device)
            if self.owner.training_mode:
                self.function_instance.train()
            else:
                self.function_instance.eval()
            args[0][self.layer_output_name(0)] = self.function_instance(input_tensor)
            self.clear_inputs(args[0])
            return True

    def PReLU_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None,
                "Alpha": (float, 0.1),
                "W.File": None,
                "slope.Initializer": None,
                "slope.InitializerMultiplier": (float, 0.25),
                "slope.LRateMultiplier": (float, 1.0),
            }, )
        elif command == "create_function":
            input_tensor = args[0][self.layer_input_name(0)]
            if self.function_instance is None:
                self.function_instance = nn.PReLU(
                    init=self.props['slope.InitializerMultiplier']).to(input_tensor.device)

            if self.props['W.File']:
                param_tensor = load_param_in_txt(self.props['W.File'])
            else:
                param_tensor = args[0].get(self.layer_input_name(1), None)

            with torch.no_grad():
                if 'state_dict' in self.props and self.props['state_dict']:
                    self.load_state()
                    del self.props['state_dict']
                elif param_tensor is not None:
                    self.function_instance.weight.copy_(param_tensor)
                    self.props['W.File'] = None
            if self.owner.training_mode:
                self.function_instance.train()
            else:
                self.function_instance.eval()
            args[0][self.layer_output_name(0)] = self.function_instance(input_tensor)
            self.clear_inputs(args[0])
            return True

    def ReLU_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None,
                "InPlace": (bool, False),
            }, )
        elif command == "create_function":
            input_tensor = args[0][self.layer_input_name(0)]
            if self.function_instance is None:
                self.function_instance = nn.ReLU(inplace=self.props['InPlace']).to(input_tensor.device)
            if self.owner.training_mode:
                self.function_instance.train()
            else:
                self.function_instance.eval()
            args[0][self.layer_output_name(0)] = self.function_instance(input_tensor)
            self.clear_inputs(args[0])
            return True

    def ReLU6_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None,
                "InPlace": (bool, False),
            }, )
        elif command == "create_function":
            input_tensor = args[0][self.layer_input_name(0)]
            if self.function_instance is None:
                self.function_instance = nn.ReLU6(inplace=self.props['InPlace']).to(input_tensor.device)
            if self.owner.training_mode:
                self.function_instance.train()
            else:
                self.function_instance.eval()
            args[0][self.layer_output_name(0)] = self.function_instance(input_tensor)
            self.clear_inputs(args[0])
            return True

    def SELU_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None,
            }, )
        elif command == "create_function":
            input_tensor = args[0][self.layer_input_name(0)]
            if self.function_instance is None:
                self.function_instance = nn.SELU().to(input_tensor.device)
            if self.owner.training_mode:
                self.function_instance.train()
            else:
                self.function_instance.eval()
            args[0][self.layer_output_name(0)] = self.function_instance(input_tensor)
            self.clear_inputs(args[0])
            return True

    def Sigmoid_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None,
            }, )
        elif command == "create_function":
            input_tensor = args[0][self.layer_input_name(0)]
            if self.function_instance is None:
                self.function_instance = nn.Sigmoid()
            if self.owner.training_mode:
                self.function_instance.train()
            else:
                self.function_instance.eval()
            args[0][self.layer_output_name(0)] = self.function_instance(input_tensor)
            self.clear_inputs(args[0])
            return True

    def Softmax_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None,
                "Axis": (int, 1),
            }, )
        elif command == "create_function":
            input_tensor = args[0][self.layer_input_name(0)]
            if self.function_instance is None:
                self.function_instance = nn.Softmax(dim=self.props['Axis']).to(input_tensor.device)
            if self.owner.training_mode:
                self.function_instance.train()
            else:
                self.function_instance.eval()
            args[0][self.layer_output_name(0)] = self.function_instance(input_tensor)
            self.clear_inputs(args[0])
            return True

    def SoftSign_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None
            }, )
        elif command == "create_function":
            input_tensor = args[0][self.layer_input_name(0)]
            if self.function_instance is None:
                self.function_instance = nn.Softsign().to(input_tensor.device)
            if self.owner.training_mode:
                self.function_instance.train()
            else:
                self.function_instance.eval()
            args[0][self.layer_output_name(0)] = self.function_instance(input_tensor)
            self.clear_inputs(args[0])
            return True

    def Swish_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None
            }, )
        elif command == "create_function":
            input_tensor = args[0][self.layer_input_name(0)]
            if self.function_instance is None:
                # SiLU is also known as the swish.
                self.function_instance = nn.SiLU().to(input_tensor.device)
            if self.owner.training_mode:
                self.function_instance.train()
            else:
                self.function_instance.eval()
            args[0][self.layer_output_name(0)] = self.function_instance(input_tensor)
            self.clear_inputs(args[0])
            return True

    def ASin_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None
            }, )
        elif command == "create_function":
            input_tensor = args[0][self.layer_input_name(0)]
            args[0][self.layer_output_name(0)] = torch.asin(input_tensor).to(input_tensor.device)
            self.clear_inputs(args[0])
            return True

    def ASinh_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None
            }, )
        elif command == "create_function":
            input_tensor = args[0][self.layer_input_name(0)]
            args[0][self.layer_output_name(0)] = torch.asinh(input_tensor).to(input_tensor.device)
            self.clear_inputs(args[0])
            return True

    def Sin_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None
            }, )
        elif command == "create_function":
            input_tensor = args[0][self.layer_input_name(0)]
            args[0][self.layer_output_name(0)] = torch.sin(input_tensor).to(input_tensor.device)
            self.clear_inputs(args[0])
            return True

    def Sinh_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None
            }, )
        elif command == "create_function":
            input_tensor = args[0][self.layer_input_name(0)]
            args[0][self.layer_output_name(0)] = torch.sinh(input_tensor).to(input_tensor.device)
            self.clear_inputs(args[0])
            return True

    def ACos_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None
            }, )
        elif command == "create_function":
            input_tensor = args[0][self.layer_input_name(0)]
            args[0][self.layer_output_name(0)] = torch.acos(input_tensor).to(input_tensor.device)
            self.clear_inputs(args[0])
            return True

    def ACosh_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None
            }, )
        elif command == "create_function":
            input_tensor = args[0][self.layer_input_name(0)]
            args[0][self.layer_output_name(0)] = torch.acosh(input_tensor).to(input_tensor.device)
            self.clear_inputs(args[0])
            return True

    def Cos_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None
            }, )
        elif command == "create_function":
            input_tensor = args[0][self.layer_input_name(0)]
            args[0][self.layer_output_name(0)] = torch.cos(input_tensor).to(input_tensor.device)
            self.clear_inputs(args[0])
            return True

    def Cosh_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None
            }, )
        elif command == "create_function":
            input_tensor = args[0][self.layer_input_name(0)]
            args[0][self.layer_output_name(0)] = torch.cosh(input_tensor).to(input_tensor.device)
            self.clear_inputs(args[0])
            return True

    def ATan_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None
            }, )
        elif command == "create_function":
            input_tensor = args[0][self.layer_input_name(0)]
            args[0][self.layer_output_name(0)] = torch.atan(input_tensor).to(input_tensor.device)
            self.clear_inputs(args[0])
            return True

    def ATanh_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None
            }, )
        elif command == "create_function":
            input_tensor = args[0][self.layer_input_name(0)]
            args[0][self.layer_output_name(0)] = torch.atanh(input_tensor).to(input_tensor.device)
            self.clear_inputs(args[0])
            return True

    def ATan2_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None,
            }, )
        elif command == "create_function":
            input_tensor = args[0][self.layer_input_name(0)]
            side_connect_tensor = args[0].get(self.layer_input_name(1), None)
            if side_connect_tensor is not None:
                args[0][self.layer_output_name(0)] = torch.atan2(input_tensor, side_connect_tensor).to(
                    input_tensor.device)
                self.clear_inputs(args[0])
                return True

    def Tan_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None
            }, )
        elif command == "create_function":
            input_tensor = args[0][self.layer_input_name(0)]
            args[0][self.layer_output_name(0)] = torch.tan(input_tensor).to(input_tensor.device)
            self.clear_inputs(args[0])
            return True

    def Tanh_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None
            }, )
        elif command == "create_function":
            input_tensor = args[0][self.layer_input_name(0)]
            args[0][self.layer_output_name(0)] = torch.tanh(input_tensor).to(input_tensor.device)
            self.clear_inputs(args[0])
            return True

    def Sinc_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None
            }, )
        elif command == "create_function":
            input_tensor = args[0][self.layer_input_name(0)]
            args[0][self.layer_output_name(0)] = torch.sinc(input_tensor).to(input_tensor.device)
            self.clear_inputs(args[0])
            return True

    def Tanh_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None,
            }, )
        elif command == "create_function":
            input_tensor = args[0][self.layer_input_name(0)]
            if self.function_instance is None:
                self.function_instance = nn.Tanh().to(input_tensor.device)
            if self.owner.training_mode:
                self.function_instance.train()
            else:
                self.function_instance.eval()
            args[0][self.layer_output_name(0)] = self.function_instance(input_tensor)
            self.clear_inputs(args[0])
            return True

    def HardTanh_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None,
            }, )
        elif command == "create_function":
            input_tensor = args[0][self.layer_input_name(0)]
            if self.function_instance is None:
                self.function_instance = nn.Hardtanh().to(input_tensor.device)
            if self.owner.training_mode:
                self.function_instance.train()
            else:
                self.function_instance.eval()
            args[0][self.layer_output_name(0)] = self.function_instance(input_tensor)
            self.clear_inputs(args[0])
            return True

    def TanhShrink_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None,
            }, )
        elif command == "create_function":
            input_tensor = args[0][self.layer_input_name(0)]
            if self.function_instance is None:
                self.function_instance = nn.Tanhshrink().to(input_tensor.device)
            if self.owner.training_mode:
                self.function_instance.train()
            else:
                self.function_instance.eval()
            args[0][self.layer_output_name(0)] = self.function_instance(input_tensor)
            self.clear_inputs(args[0])
            return True

    def AddN_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None,
                "InputsNum": (int, 2),
            })
        elif command == "create_function":
            pined_connect_num = self.props["InputsNum"]
            input_name = self.layer_input_name(0)
            pined_connect_tensor = args[0].get(input_name, None)
            # collect pined_connect_tensor to args[0] with a unique key
            if pined_connect_tensor is not None:
                index = len([key for key in args[0] if key.startswith(f"{input_name}_pined_connect_")])
                args[0][f"{input_name}_pined_connect_{index + 1}"] = pined_connect_tensor

            connect_tensors = [args[0][key] for key in args[0] if key.startswith(f"{input_name}_pined_connect_")]
            # Check if all input tensors are ready
            if len(connect_tensors) == pined_connect_num:
                # Call torch.add() with all tensors
                args[0][self.layer_output_name(0)] = torch.add(*connect_tensors)
                self.clear_pined_connect(args[0], input_name, pined_connect_num)
                self.clear_inputs(args[0])
                self.muti_inputs_ready = True
                return True
            else:
                self.muti_inputs_ready = False
                return False

    def Concatenate_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None,
                "Axis": (int, 1),
                "InputsNum": (int, 2),
            })
        elif command == "create_function":
            pined_connect_num = self.props["InputsNum"]
            input_name = self.layer_input_name(0)
            pined_connect_tensor = args[0].get(input_name, None)
            # collect pined_connect_tensor to args[0] with a unique key
            if pined_connect_tensor is not None:
                index = len([key for key in args[0] if key.startswith(f"{input_name}_pined_connect_")])
                args[0][f"{input_name}_pined_connect_{index + 1}"] = pined_connect_tensor

            connect_tensors = [args[0][key] for key in args[0] if key.startswith(f"{input_name}_pined_connect_")]
            # Check if all input tensors are ready
            if len(connect_tensors) == pined_connect_num:
                # Call torch.add() with all tensors
                args[0][self.layer_output_name(0)] = torch.concat(tensors=connect_tensors, dim=self.props["Axis"] + 1)
                self.clear_pined_connect(args[0], input_name, pined_connect_num)
                self.clear_inputs(args[0])
                self.muti_inputs_ready = True
                return True
            else:
                self.muti_inputs_ready = False
                return False

    def Tile_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None,
                "Reps": (to_shape, (1,)),
            }, )
        elif command == "create_function":
            input_tensor = args[0][self.layer_input_name(0)]
            rep_size = self.props['Reps']
            args[0][self.layer_output_name(0)] = torch.tile(input_tensor, rep_size).to(input_tensor.device)
            self.clear_inputs(args[0])
            return True

    def Pad_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None,
                "PadWidth": (to_shape, (1,)),
                "Mode": (str, 'constant'),
                "ConstantValue": (float, 0.0)
            }, )
        elif command == "create_function":
            input_tensor = args[0][self.layer_input_name(0)]
            args[0][self.layer_output_name(0)] = torch.nn.functional.pad(input=input_tensor,
                                                                         pad=self.props["PadWidth"],
                                                                         mode=self.props["Mode"]).to(
                input_tensor.device)
            self.clear_inputs(args[0])
            return True

    def Flip_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None,
                "Axes": (to_shape, (1,)),
            }, )
        elif command == "create_function":
            input_tensor = args[0][self.layer_input_name(0)]
            dims = [axe + 1 for axe in self.props["Axes"]]
            args[0][self.layer_output_name(0)] = torch.flip(input=input_tensor,
                                                            dims=dims).to(input_tensor.device)
            self.clear_inputs(args[0])
            return True

    def Transpose_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None,
                "Axes": (to_shape, (1,)),
            }, )
        elif command == "create_function":
            input_tensor = args[0][self.layer_input_name(0)]
            dims = (0, *[axe + 1 for axe in self.props["Axes"]])
            args[0][self.layer_output_name(0)] = input_tensor.permute(dims).to(input_tensor.device)
            self.clear_inputs(args[0])
            return True

    def MatrixDiag_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None,
            }, )
        elif command == "create_function":
            input_tensor = args[0][self.layer_input_name(0)]
            shape = input_tensor.shape
            last_dim = shape[-1]
            diag_size = min(shape[-2], last_dim)
            output_shape = shape[:-2] + (shape[-2], last_dim, last_dim)
            output_tensor = torch.zeros(output_shape, dtype=input_tensor.dtype, device=input_tensor.device)

            diag_indices = torch.arange(diag_size, device=input_tensor.device)
            output_tensor[..., diag_indices, diag_indices] = input_tensor[..., :diag_size]
            args[0][self.layer_output_name(0)] = output_tensor.to(input_tensor.device)
            self.clear_inputs(args[0])
            return True

    def MatrixDiagPart_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None,
            }, )
        elif command == "create_function":
            input_tensor = args[0][self.layer_input_name(0)]
            diagonal_elements = torch.diagonal(input_tensor, dim1=-2, dim2=-1)
            args[0][self.layer_output_name(0)] = diagonal_elements.to(input_tensor.device)
            self.clear_inputs(args[0])
            return True

    def Stack_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None,
                "Axis": (int, 1),
                "InputsNum": (int, 2),
            })
        elif command == "create_function":
            pined_connect_num = self.props["InputsNum"]
            input_name = self.layer_input_name(0)
            pined_connect_tensor = args[0].get(input_name, None)
            # collect pined_connect_tensor to args[0] with a unique key
            if pined_connect_tensor is not None:
                index = len([key for key in args[0] if key.startswith(f"{input_name}_pined_connect_")])
                args[0][f"{input_name}_pined_connect_{index + 1}"] = pined_connect_tensor

            connect_tensors = [args[0][key] for key in args[0] if key.startswith(f"{input_name}_pined_connect_")]
            # Check if all input tensors are ready
            if len(connect_tensors) == pined_connect_num:
                # Call torch.add() with all tensors
                args[0][self.layer_output_name(0)] = torch.stack(tensors=connect_tensors,
                                                                 dim=self.props["Axis"] + 1)
                self.clear_pined_connect(args[0], input_name, pined_connect_num)
                self.clear_inputs(args[0])
                self.muti_inputs_ready = True
                return True
            else:
                self.muti_inputs_ready = False
                return False

    def Slice_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None,
                "Start": (to_shape, (0, 0, 0)),
                "Stop": (str, None),
                "Step": (to_shape, (1, 1, 1)),
            }, )
        elif command == "create_function":
            input_tensor = args[0][self.layer_input_name(0)]
            if self.props['Stop'] == "*Input":
                stop = input_tensor.shape
            else:
                stop = (input_tensor.shape[0], *to_shape(self.props['Stop']))
            start = (0, *self.props['Start'])
            step = (1, *self.props['Step'])
            slices = tuple(slice(s, e, st) for s, e, st in zip(start, stop, step))
            output = input_tensor[slices]
            args[0][self.layer_output_name(0)] = output.to(input_tensor.device)
            self.clear_inputs(args[0])
            return True

    def ClipByValue_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None,
            })
        elif command == "create_function":
            for i in [0, 1, 2]:
                if not self.check_inputs(self.layer_input_name(i), args[0]):
                    return False
            input_tensor = args[0].get(self.layer_input_name(0), None)
            side_connect_min = args[0].get(self.layer_input_name(1), None)
            side_connect_max = args[0].get(self.layer_input_name(2), None)
            if (input_tensor is not None) and (side_connect_min is not None) and (side_connect_max is not None):
                args[0][self.layer_output_name(0)] = torch.clip(input_tensor, side_connect_min, side_connect_max).to(
                    input_tensor.device)
                self.clear_inputs(args[0])
                return True

    def MulN_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None,
                "InputsNum": (int, 2),
            })
        elif command == "create_function":
            pined_connect_num = self.props["InputsNum"]
            input_name = self.layer_input_name(0)
            pined_connect_tensor = args[0].get(input_name, None)
            # collect pined_connect_tensor to args[0] with a unique key
            if pined_connect_tensor is not None:
                index = len([key for key in args[0] if key.startswith(f"{input_name}_pined_connect_")])
                args[0][f"{input_name}_pined_connect_{index + 1}"] = pined_connect_tensor

            connect_tensors = [args[0][key] for key in args[0] if key.startswith(f"{input_name}_pined_connect_")]
            # Check if all input tensors are ready
            if len(connect_tensors) == pined_connect_num:
                args[0][self.layer_output_name(0)] = torch.mul(*connect_tensors)
                self.clear_pined_connect(args[0], input_name, pined_connect_num)
                self.clear_inputs(args[0])
                self.muti_inputs_ready = True
                return True
            else:
                self.muti_inputs_ready = False
                return False

    def Sub2_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None,
            })
        elif command == "create_function":
            input_tensor = args[0][self.layer_input_name(0)]
            if not self.check_inputs(self.layer_input_name(1), args[0]):
                return False
            side_connect_tensor = args[0].get(self.layer_input_name(1), None)
            if side_connect_tensor is not None:
                args[0][self.layer_output_name(0)] = torch.sub(input_tensor, side_connect_tensor).to(
                    input_tensor.device)
                self.clear_inputs(args[0])
                return True

    def Div2_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None,
            })
        elif command == "create_function":
            input_tensor = args[0][self.layer_input_name(0)]
            if not self.check_inputs(self.layer_input_name(1), args[0]):
                return False
            side_connect_tensor = args[0].get(self.layer_input_name(1), None)
            if side_connect_tensor is not None:
                args[0][self.layer_output_name(0)] = torch.div(input_tensor, side_connect_tensor).to(
                    input_tensor.device)
                self.clear_inputs(args[0])
                return True

    def MaximumN_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None,
                "InputsNum": (int, 2),
            })
        elif command == "create_function":
            pined_connect_num = self.props["InputsNum"]
            input_name = self.layer_input_name(0)
            pined_connect_tensor = args[0].get(input_name, None)
            # collect pined_connect_tensor to args[0] with a unique key
            if pined_connect_tensor is not None:
                index = len([key for key in args[0] if key.startswith(f"{input_name}_pined_connect_")])
                args[0][f"{input_name}_pined_connect_{index + 1}"] = pined_connect_tensor

            connect_tensors = [args[0][key] for key in args[0] if key.startswith(f"{input_name}_pined_connect_")]
            # Check if all input tensors are ready
            if len(connect_tensors) == pined_connect_num:
                # Call torch.add() with all tensors
                args[0][self.layer_output_name(0)], _ = torch.max(torch.stack(connect_tensors), dim=0)
                self.clear_pined_connect(args[0], input_name, pined_connect_num)
                self.clear_inputs(args[0])
                self.muti_inputs_ready = True
                return True
            else:
                self.muti_inputs_ready = False
                return False

    def MinimumN_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None,
                "InputsNum": (int, 2),
            })
        elif command == "create_function":
            pined_connect_num = self.props["InputsNum"]
            input_name = self.layer_input_name(0)
            pined_connect_tensor = args[0].get(input_name, None)
            # collect pined_connect_tensor to args[0] with a unique key
            if pined_connect_tensor is not None:
                index = len([key for key in args[0] if key.startswith(f"{input_name}_pined_connect_")])
                args[0][f"{input_name}_pined_connect_{index + 1}"] = pined_connect_tensor

            connect_tensors = [args[0][key] for key in args[0] if key.startswith(f"{input_name}_pined_connect_")]
            # Check if all input tensors are ready
            if len(connect_tensors) == pined_connect_num:
                # Call torch.add() with all tensors
                args[0][self.layer_output_name(0)], _ = torch.min(torch.stack(connect_tensors), dim=0)
                self.clear_pined_connect(args[0], input_name, pined_connect_num)
                self.clear_inputs(args[0])
                self.muti_inputs_ready = True
                return True
            else:
                self.muti_inputs_ready = False
                return False

    def Pow2_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None,
            })
        elif command == "create_function":
            input_tensor = args[0][self.layer_input_name(0)]
            if not self.check_inputs(self.layer_input_name(1), args[0]):
                return False
            side_connect_tensor = args[0].get(self.layer_input_name(1), None)
            if side_connect_tensor is not None:
                args[0][self.layer_output_name(0)] = torch.pow(input_tensor, side_connect_tensor).to(
                    input_tensor.device)
                self.clear_inputs(args[0])
                return True

    def AddScalar_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None,
                "Value": (float, 0.0),
            })
        elif command == "create_function":
            input_tensor = args[0][self.layer_input_name(0)]
            args[0][self.layer_output_name(0)] = input_tensor + self.props["Value"]
            self.clear_inputs(args[0])
            return True

    def MulScalar_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None,
                "Value": (float, 0.0),
            })
        elif command == "create_function":
            input_tensor = args[0][self.layer_input_name(0)]
            args[0][self.layer_output_name(0)] = input_tensor * self.props["Value"]
            self.clear_inputs(args[0])
            return True

    def RSubScalar_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None,
                "Value": (float, 0.0),
            })
        elif command == "create_function":
            input_tensor = args[0][self.layer_input_name(0)]
            args[0][self.layer_output_name(0)] = torch.sub(self.props["Value"], input_tensor)
            self.clear_inputs(args[0])
            return True

    def RDivScalar_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None,
                "Value": (float, 1.0),
            })
        elif command == "create_function":
            input_tensor = args[0][self.layer_input_name(0)]
            args[0][self.layer_output_name(0)] = torch.div(self.props["Value"], input_tensor)
            self.clear_inputs(args[0])
            return True

    def PowScalar_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None,
                "Value": (float, 1.0),
            })
        elif command == "create_function":
            input_tensor = args[0][self.layer_input_name(0)]
            args[0][self.layer_output_name(0)] = input_tensor ** self.props["Value"]
            self.clear_inputs(args[0])
            return True

    def RPowScalar_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None,
                "Value": (float, 1.0),
            })
        elif command == "create_function":
            input_tensor = args[0][self.layer_input_name(0)]
            args[0][self.layer_output_name(0)] = self.props["Value"] ** input_tensor
            self.clear_inputs(args[0])
            return True

    def MaximumScalar_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None,
                "Value": (float, 1.0),
            })
        elif command == "create_function":
            input_tensor = args[0][self.layer_input_name(0)]
            args[0][self.layer_output_name(0)] = torch.maximum(input_tensor, torch.tensor(self.props["Value"]))
            self.clear_inputs(args[0])
            return True

    def MinimumScalar_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None,
                "Value": (float, 1.0),
            })
        elif command == "create_function":
            input_tensor = args[0][self.layer_input_name(0)]
            args[0][self.layer_output_name(0)] = torch.minimum(input_tensor, torch.tensor(self.props["Value"]))
            self.clear_inputs(args[0])
            return True

    def Equal_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
            })
        elif command == "create_function":
            pass

    def EqualScalar_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Value": "value",
            })
        elif command == "create_function":
            pass

    def Greater_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "R.Dataset": "r_dataset",
                "R.Generator": "r_generator",
                "R.GeneratorMultiplier": "r_generator_multiplier"
            })
        elif command == "create_function":
            pass

    def GreaterEqual_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "R.Dataset": "r_dataset",
                "R.Generator": "r_generator",
                "R.GeneratorMultiplier": "r_generator_multiplier"
            })
        elif command == "create_function":
            pass

    def GreaterEqualScalar_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Value": "value",
            })
        elif command == "create_function":
            pass

    def GreaterScalar_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Value": "value",
            })
        elif command == "create_function":
            pass

    def IsInf_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
            })
        elif command == "create_function":
            pass

    def IsNaN_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
            })
        elif command == "create_function":
            pass

    def Less_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "R.Dataset": "r_dataset",
                "R.Generator": "r_generator",
                "R.GeneratorMultiplier": "r_generator_multiplier"
            })
        elif command == "create_function":
            pass

    def LessEqual_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "R.Dataset": "r_dataset",
                "R.Generator": "r_generator",
                "R.GeneratorMultiplier": "r_generator_multiplier"
            })
        elif command == "create_function":
            pass

    def LessEqualScalar_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Value": "value",
            })
        elif command == "create_function":
            pass

    def LessScalar_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Value": "value",
            })
        elif command == "create_function":
            pass

    def LogicalAnd_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
            })
        elif command == "create_function":
            pass

    def LogicalAndScalar_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Value": "value",
            })
        elif command == "create_function":
            pass

    def LogicalNot_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
            })
        elif command == "create_function":
            pass

    def LogicalOr_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
            })
        elif command == "create_function":
            pass

    def LogicalOrScalar_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Value": "value",
            })
        elif command == "create_function":
            pass

    def LogicalXor_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
            })
        elif command == "create_function":
            pass

    def LogicalXorScalar_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Value": "value",
            })
        elif command == "create_function":
            pass

    def NotEqual_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
            })
        elif command == "create_function":
            pass

    def NotEqualScalar_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Value": "value",
            })
        elif command == "create_function":
            pass

    def Delay_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Size": "size",
                "Initial.Dataset": "initial_dataset",
                "Initial.Generator": "initial_generator",
                "Initial.GeneratorMultiplier": "initial_generator_multiplier",
                "ID": "id"
            })
        elif command == "create_function":
            pass

    def RecurrentInput_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Axis": "axis",
                "ID": "id"
            })
        elif command == "create_function":
            pass

    def RecurrentOutput_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Axis": "axis",
                "ID": "id",
                "Length": "length"
            })
        elif command == "create_function":
            pass

    def AbsoluteError_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None,
                "T.Dataset": None,
                "IsLossFunction": (bool, True)
            })
        elif command == "create_function":
            if self.owner.forward_without_loss:
                args[0][self.layer_output_name(0)] = args[0][self.layer_input_name(0)]
                if 'loss' in args[0]:
                    args[0]['loss'][self.layer_output_name(0)] = args[0][self.layer_output_name(0)]
                else:
                    args[0]['loss'] = {self.layer_output_name(0): args[0][self.layer_output_name(0)]}
                self.clear_inputs(args[0])
                return True
            else:
                input_tensor = args[0][self.layer_input_name(0)]
                target = args[0][self.props['T.Dataset']]
                if self.function_instance is None:
                    self.function_instance = nn.L1Loss().to(input_tensor.device)

                if self.owner.training_mode:
                    self.function_instance.train()
                else:
                    self.function_instance.eval()
                args[0][self.layer_output_name(0)] = self.function_instance(input_tensor, target)
                if 'loss' in args[0]:
                    args[0]['loss'][self.layer_output_name(0)] = args[0][self.layer_output_name(0)]
                else:
                    args[0]['loss'] = {self.layer_output_name(0): args[0][self.layer_output_name(0)]}
                self.clear_inputs(args[0])
                return True

    def BinaryCrossEntropy_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None,
                "T.Dataset": None,
                "IsLossFunction": (bool, True)
            })
        elif command == "create_function":
            if self.owner.forward_without_loss:
                args[0][self.layer_output_name(0)] = args[0][self.layer_input_name(0)]
                if 'loss' in args[0]:
                    args[0]['loss'][self.layer_output_name(0)] = args[0][self.layer_output_name(0)]
                else:
                    args[0]['loss'] = {self.layer_output_name(0): args[0][self.layer_output_name(0)]}
                self.clear_inputs(args[0])
                return True
            else:
                input_tensor = args[0][self.layer_input_name(0)].view(-1, 1)
                target = args[0][self.props['T.Dataset']].view(-1, 1).to(input_tensor.dtype)
                if self.function_instance is None:
                    self.function_instance = nn.BCELoss().to(input_tensor.device)

                if self.owner.training_mode:
                    self.function_instance.train()
                else:
                    self.function_instance.eval()
                args[0][self.layer_output_name(0)] = self.function_instance(input_tensor, target)
                if 'loss' in args[0]:
                    args[0]['loss'][self.layer_output_name(0)] = args[0][self.layer_output_name(0)]
                else:
                    args[0]['loss'] = {self.layer_output_name(0): args[0][self.layer_output_name(0)]}
                self.clear_inputs(args[0])
                return True

    def CrossEntropyLoss_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None,
                "T.Dataset": None,
                "IsLossFunction": (bool, True)
            })
        elif command == "create_function":
            if self.owner.forward_without_loss:
                args[0][self.layer_output_name(0)] = args[0][self.layer_input_name(0)]
                if 'loss' in args[0]:
                    args[0]['loss'][self.layer_output_name(0)] = args[0][self.layer_output_name(0)]
                else:
                    args[0]['loss'] = {self.layer_output_name(0): args[0][self.layer_output_name(0)]}
                self.clear_inputs(args[0])
                return True
            else:
                input_tensor = args[0][self.layer_input_name(0)]
                target = args[0][self.props['T.Dataset']]
                if self.function_instance is None:
                    self.function_instance = nn.CrossEntropyLoss().to(input_tensor.device)

                if self.owner.training_mode:
                    self.function_instance.train()
                else:
                    self.function_instance.eval()

                input_shape = input_tensor.shape
                target_shape = target.shape
                if input_shape == target_shape:
                    # include class probabilities
                    pass
                else:
                    # include class index
                    target = target.long().squeeze()
                args[0][self.layer_output_name(0)] = self.function_instance(input_tensor, target)
                if 'loss' in args[0]:
                    args[0]['loss'][self.layer_output_name(0)] = args[0][self.layer_output_name(0)]
                else:
                    args[0]['loss'] = {self.layer_output_name(0): args[0][self.layer_output_name(0)]}
                self.clear_inputs(args[0])
                return True

    def EpsilonInsensitiveLoss_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None,
                "T.Dataset": None,
                "IsLossFunction": (bool, True)
            })
        elif command == "create_function":
            if self.owner.forward_without_loss:
                args[0][self.layer_output_name(0)] = args[0][self.layer_input_name(0)]
                if 'loss' in args[0]:
                    args[0]['loss'][self.layer_output_name(0)] = args[0][self.layer_output_name(0)]
                else:
                    args[0]['loss'] = {self.layer_output_name(0): args[0][self.layer_output_name(0)]}
                self.clear_inputs(args[0])
                return True
            else:
                input_tensor = args[0][self.layer_input_name(0)]
                target = args[0][self.props['T.Dataset']]
                if self.function_instance is None:
                    self.function_instance = custom_layers.ElementWiseEpsilonInsensitiveLoss()

                if self.owner.training_mode:
                    self.function_instance.train()
                else:
                    self.function_instance.eval()
                args[0][self.layer_output_name(0)] = self.function_instance(input_tensor, target)
                if 'loss' in args[0]:
                    args[0]['loss'][self.layer_output_name(0)] = args[0][self.layer_output_name(0)]
                else:
                    args[0]['loss'] = {self.layer_output_name(0): args[0][self.layer_output_name(0)]}
                self.clear_inputs(args[0])
                return True

    def HuberLoss_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None,
                "T.Dataset": None,
                "IsLossFunction": (bool, True),
                "Delta": (float, 1.0)
            })
        elif command == "create_function":
            if self.owner.forward_without_loss:
                args[0][self.layer_output_name(0)] = args[0][self.layer_input_name(0)]
                if 'loss' in args[0]:
                    args[0]['loss'][self.layer_output_name(0)] = args[0][self.layer_output_name(0)]
                else:
                    args[0]['loss'] = {self.layer_output_name(0): args[0][self.layer_output_name(0)]}
                self.clear_inputs(args[0])
                return True
            else:
                input_tensor = args[0][self.layer_input_name(0)]
                target = args[0][self.props['T.Dataset']]
                if self.function_instance is None:
                    self.function_instance = nn.HuberLoss(delta=self.props['Delta'])

                if self.owner.training_mode:
                    self.function_instance.train()
                else:
                    self.function_instance.eval()
                args[0][self.layer_output_name(0)] = self.function_instance(input_tensor, target)
                if 'loss' in args[0]:
                    args[0]['loss'][self.layer_output_name(0)] = args[0][self.layer_output_name(0)]
                else:
                    args[0]['loss'] = {self.layer_output_name(0): args[0][self.layer_output_name(0)]}
                self.clear_inputs(args[0])
                return True

    def KLMultinomial_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "BaseAxis": "base_axis",
                "q.Dataset": "q_dataset",
                "q.Generator": "q_generator",
                "q.GeneratorMultiplier": "q_generator_multiplier",
            })
        elif command == "create_function":
            pass

    def KLMultinomial_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "BaseAxis": "base_axis",
                "q.Dataset": "q_dataset",
                "q.Generator": "q_generator",
                "q.GeneratorMultiplier": "q_generator_multiplier",
            })
        elif command == "create_function":
            pass

    def SigmoidCrossEntropy_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None,
                "T.Dataset": None,
                "IsLossFunction": (bool, True)
            })
        elif command == "create_function":
            if self.owner.forward_without_loss:
                args[0][self.layer_output_name(0)] = args[0][self.layer_input_name(0)]
                if 'loss' in args[0]:
                    args[0]['loss'][self.layer_output_name(0)] = args[0][self.layer_output_name(0)]
                else:
                    args[0]['loss'] = {self.layer_output_name(0): args[0][self.layer_output_name(0)]}
                self.clear_inputs(args[0])
                return True
            else:
                input_tensor = args[0][self.layer_input_name(0)]
                target = args[0][self.props['T.Dataset']]
                if self.function_instance is None:
                    self.function_instance = nn.BCEWithLogitsLoss()

                if self.owner.training_mode:
                    self.function_instance.train()
                else:
                    self.function_instance.eval()
                args[0][self.layer_output_name(0)] = self.function_instance(input_tensor, target)
                if 'loss' in args[0]:
                    args[0]['loss'][self.layer_output_name(0)] = args[0][self.layer_output_name(0)]
                else:
                    args[0]['loss'] = {self.layer_output_name(0): args[0][self.layer_output_name(0)]}
                self.clear_inputs(args[0])
                return True

    def SquaredError_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None,
                "T.Dataset": None,
                "IsLossFunction": (bool, True)
            })
        elif command == "create_function":
            if self.owner.forward_without_loss:
                args[0][self.layer_output_name(0)] = args[0][self.layer_input_name(0)]
                if 'loss' in args[0]:
                    args[0]['loss'][self.layer_output_name(0)] = args[0][self.layer_output_name(0)]
                else:
                    args[0]['loss'] = {self.layer_output_name(0): args[0][self.layer_output_name(0)]}
                self.clear_inputs(args[0])
                return True
            else:
                input_tensor = args[0][self.layer_input_name(0)]
                side_connect_tensor = args[0].get(self.layer_input_name(1), None)
                if side_connect_tensor is not None:  # the second batch of matrices to be multiplied
                    target = args[0][self.props['T.Dataset']]
                else:
                    target = args[0][self.props['T.Dataset']]
                if self.function_instance is None:
                    self.function_instance = nn.MSELoss()

                if self.owner.training_mode:
                    self.function_instance.train()
                else:
                    self.function_instance.eval()
                args[0][self.layer_output_name(0)] = self.function_instance(input_tensor, target)
                if 'loss' in args[0]:
                    args[0]['loss'][self.layer_output_name(0)] = args[0][self.layer_output_name(0)]
                else:
                    args[0]['loss'] = {self.layer_output_name(0): args[0][self.layer_output_name(0)]}
                self.clear_inputs(args[0])
                return True

    def BatchInv_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None,
            })
        elif command == "create_function":
            input_tensor = args[0][self.layer_input_name(0)]
            if self.function_instance is None:
                self.function_instance = custom_layers.BatchInv().to(input_tensor.device)
            args[0]["output"] = self.function_instance(input_tensor)
            if self.owner.training_mode:
                self.function_instance.train()
            else:
                self.function_instance.eval()
            args[0][self.layer_output_name(0)] = self.function_instance(input_tensor)
            self.clear_inputs(args[0])
            return True

    def BatchMatmul_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None,
            })
        elif command == "create_function":
            input_tensor = args[0][self.layer_input_name(0)]
            if not self.check_inputs(self.layer_input_name(1), args[0]):
                return False
            side_connect_tensor = args[0].get(self.layer_input_name(1), None)
            if side_connect_tensor is not None:  # the second batch of matrices to be multiplied
                self.function_instance = custom_layers.Bmm().to(
                    input_tensor.device)
                args[0][self.layer_output_name(0)] = self.function_instance(input_tensor, side_connect_tensor)
                self.clear_inputs(args[0])
                return True

    def Ceil_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None,
            }, )
        elif command == "create_function":
            input_tensor = args[0][self.layer_input_name(0)]
            args[0][self.layer_output_name(0)] = torch.ceil(input_tensor).to(input_tensor.device)
            self.clear_inputs(args[0])
            return True

    def CumProd_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None,
                "Exclusive": (bool, False),
                "Reverse": (bool, False),
                "Axis": (int, 0),
            })
        elif command == "create_function":
            input_tensor = args[0][self.layer_input_name(0)]
            if self.function_instance is None:
                self.function_instance = custom_layers.Cumprod(
                    dim=self.props["Axis"] + 1,  # exclude batch dim
                    exclusive=self.props["Exclusive"],
                    reverse=self.props["Reverse"]
                ).to(input_tensor.device)
            args[0]["output"] = self.function_instance(input_tensor)
            if self.owner.training_mode:
                self.function_instance.train()
            else:
                self.function_instance.eval()
            args[0][self.layer_output_name(0)] = self.function_instance(input_tensor)
            self.clear_inputs(args[0])
            return True

    def CumSum_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None,
                "Exclusive": (bool, False),
                "Reverse": (bool, False),
                "Axis": (int, 0),
            })
        elif command == "create_function":
            input_tensor = args[0][self.layer_input_name(0)]
            if self.function_instance is None:
                self.function_instance = custom_layers.Cumsum(
                    dim=self.props["Axis"] + 1,  # exclude batch dim
                    exclusive=self.props["Exclusive"],
                    reverse=self.props["Reverse"]
                ).to(input_tensor.device)
            args[0]["output"] = self.function_instance(input_tensor)
            if self.owner.training_mode:
                self.function_instance.train()
            else:
                self.function_instance.eval()
            args[0][self.layer_output_name(0)] = self.function_instance(input_tensor)
            self.clear_inputs(args[0])
            return True

    def Floor_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None,
            }, )
        elif command == "create_function":
            input_tensor = args[0][self.layer_input_name(0)]
            args[0][self.layer_output_name(0)] = torch.floor(input_tensor).to(input_tensor.device)
            self.clear_inputs(args[0])
            return True

    def Log_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None,
            })
        elif command == "create_function":
            input_tensor = args[0][self.layer_input_name(0)]
            args[0][self.layer_output_name(0)] = torch.log(input_tensor)
            self.clear_inputs(args[0])
            return True

    def Exp_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None,
            })
        elif command == "create_function":
            input_tensor = args[0][self.layer_input_name(0)]
            args[0][self.layer_output_name(0)] = torch.exp(input_tensor)
            self.clear_inputs(args[0])
            return True

    def Max_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None,
                "Axes": (int, 1),
                "KeepDims": (bool, False),
            })
        elif command == "create_function":
            input_tensor = args[0][self.layer_input_name(0)]
            args[0][self.layer_output_name(0)], max_indices = torch.max(input_tensor, dim=self.props['Axes'] + 1,
                                                                        keepdim=self.props['KeepDims'])
            self.clear_inputs(args[0])
            return True

    def Min_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None,
                "Axes": (int, 1),
                "KeepDims": (bool, False),
            })
        elif command == "create_function":
            input_tensor = args[0][self.layer_input_name(0)]
            args[0][self.layer_output_name(0)], min_indices = torch.min(input_tensor, dim=self.props['Axes'] + 1,
                                                                        keepdim=self.props['KeepDims'])
            self.clear_inputs(args[0])
            return True

    def Mean_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None,
                "Axes": (int, 1),
                "KeepDims": (bool, False),
            })
        elif command == "create_function":
            input_tensor = args[0][self.layer_input_name(0)]
            args[0][self.layer_output_name(0)] = torch.mean(input_tensor, dim=self.props['Axes'] + 1,
                                                            keepdim=self.props['KeepDims'])
            self.clear_inputs(args[0])
            return True

    def Norm_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None,
                "Axes": (int, 1),
                "KeepDims": (bool, False),
            })
        elif command == "create_function":
            input_tensor = args[0][self.layer_input_name(0)]
            args[0][self.layer_output_name(0)] = torch.norm(input_tensor, dim=self.props['Axes'] + 1,
                                                            keepdim=self.props['KeepDims'])
            self.clear_inputs(args[0])
            return True

    def Sort_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None,
                "Axes": (int, 0),
                "Reverse": (bool, False),
                "OnlyIndex": (bool, False),
            })
        elif command == "create_function":
            input_tensor = args[0][self.layer_input_name(0)]
            sort_output = torch.sort(input_tensor,
                                     dim=self.props['Axes'] + 1,
                                     descending=self.props['Reverse'],
                                     stable=True
                                     )
            if self.props['OnlyIndex']:
                args[0][self.layer_output_name(0)] = sort_output[0]
            else:
                args[0][self.layer_output_name(0)] = sort_output
            self.clear_inputs(args[0])
            return True

    def Prune_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None,
                "Rate": (float, 0.9),
            })
        elif command == "create_function":
            input_tensor = args[0][self.layer_input_name(0)]
            if self.function_instance is None:
                self.function_instance = custom_layers.Prune(self.props["Rate"])
            args[0][self.layer_output_name(0)] = self.function_instance(input_tensor)
            self.clear_inputs(args[0])
            return True

    def Argument_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None,
                "Value": (str, ""),
                "Type": (str, "")
            })
        elif command == "create_function":
            type = self.props["Type"]
            value = self.props["Value"]
            if type == "PInt":
                value = int(value)
            elif type == "PIntArray":
                value = to_shape(value)
            args[0][self.props["Name"]] = value
            return True

    def Interpolate_hanle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None,
                "OutputSize": (to_shape, (1,)),
                "Mode": (str, 'linear'),
                "AlignCorners": (bool, False)
            })
        elif command == "create_function":
            input_tensor = args[0][self.layer_input_name(0)]
            if self.props['Mode'] == 'linear':
                if input_tensor.dim() == 3:
                    interpolation_method = 'linear'
                elif input_tensor.dim() == 4:
                    interpolation_method = 'bilinear'
                elif input_tensor.dim() == 5:
                    interpolation_method = 'trilinear'
                else:
                    raise ValueError(
                        "Input tensor must be 3D , 4D, or 5D")
            else:
                raise ValueError(f"Unsupported interpolate method: {self.props['Mode']}!")
            args[0][self.layer_output_name(0)] = nn.functional.interpolate(input_tensor,
                                                                           size=self.props['OutputSize'],
                                                                           mode=interpolation_method,
                                                                           align_corners=self.props['AlignCorners']).to(
                input_tensor.device)
            self.clear_inputs(args[0])
            return True

    def ResetNaN_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None,
            })
        elif command == "create_function":
            input_tensor = args[0][self.layer_input_name(0)]
            nan_indices = torch.isnan(input_tensor)
            input_tensor[nan_indices] = 0
            args[0][self.layer_output_name(0)] = input_tensor
            self.clear_inputs(args[0])
            return True

    def ResetInf_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None,
            })
        elif command == "create_function":
            input_tensor = args[0][self.layer_input_name(0)]
            inf_indices = torch.isinf(input_tensor)
            input_tensor[inf_indices] = 0
            args[0][self.layer_output_name(0)] = input_tensor
            self.clear_inputs(args[0])
            return True

    def NmnDetection2d_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None,
                "Thresh": (float, 0.5),
                "NMS": (float, 0.5),
                "NMSPerClass": (bool, True),
            })
        elif command == "create_function":
            input_tensor = args[0][self.layer_input_name(0)]
            if self.function_instance is None:
                self.function_instance = custom_layers.NMSDetection2D(thresh=self.props['Thresh'],
                                                                      nms=self.props['NMS'],
                                                                      nms_per_class=self.props['NMSPerClass'])
            args[0][self.layer_output_name(0)] = self.function_instance(input_tensor)
            self.clear_inputs(args[0])
            return True

    def Comment_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None,
            })
        elif command == "create_function":
            return True

    def TopKData_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None,
                "K": (int, 1),
                "Abs": (bool, False),
                "BaseAxis": (int, 1),
            })
        elif command == "create_function":
            input_tensor = args[0][self.layer_input_name(0)]
            input_tensor = input_tensor.view(*list(input_tensor.shape[:self.props['BaseAxis'] + 1]), -1)
            if self.props['Abs']:
                topk_values, topk_indices = torch.topk(input=torch.abs(input_tensor),
                                                       dim=self.props['BaseAxis'] + 1,
                                                       k=self.props['K'])
                args[0][self.layer_output_name(0)] = torch.gather(input_tensor, 1, topk_indices)
            else:
                topk_values, topk_indices = torch.topk(input=input_tensor,
                                                       dim=self.props['BaseAxis'] + 1,
                                                       k=self.props['K'])
                args[0][self.layer_output_name(0)] = topk_values
            self.clear_inputs(args[0])
            return True

    def ClipByNorm_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None,
                "Axes": (int, 1),
                "ClipNorm": (float, 1.0),
            })
        elif command == "create_function":
            input_tensor = args[0][self.layer_input_name(0)]
            flatten_input_tensor = input_tensor.view(input_tensor.shape[0], -1)
            th = self.props['ClipNorm']
            norm = torch.norm(flatten_input_tensor, dim=1)
            exceed_threshold = norm > th
            if torch.any(exceed_threshold):
                mask = exceed_threshold.unsqueeze(1)
                clipped_input_tensor = torch.where(mask,
                                                   flatten_input_tensor * (th / norm.unsqueeze(1)),
                                                   flatten_input_tensor)
            else:
                clipped_input_tensor = flatten_input_tensor
            args[0][self.layer_output_name(0)] = clipped_input_tensor.view(input_tensor.shape)
            self.clear_inputs(args[0])
            return True

    def Prod_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None,
                "Axes": (int, 1),
                "KeepDims": (bool, False),
            })
        elif command == "create_function":
            input_tensor = args[0][self.layer_input_name(0)]
            args[0][self.layer_output_name(0)] = torch.prod(input_tensor, dim=self.props['Axes'] + 1,
                                                            keepdim=self.props['KeepDims'])
            self.clear_inputs(args[0])
            return True

    def Round_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None,
            }, )
        elif command == "create_function":
            input_tensor = args[0][self.layer_input_name(0)]
            args[0][self.layer_output_name(0)] = torch.round(input_tensor).to(input_tensor.device)
            self.clear_inputs(args[0])
            return True

    def Sign_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None,
            }, )
        elif command == "create_function":
            input_tensor = args[0][self.layer_input_name(0)]
            args[0][self.layer_output_name(0)] = torch.sign(input_tensor).to(input_tensor.device)
            self.clear_inputs(args[0])
            return True

    def Sum_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None,
                "Axes": (int, 1),
                "KeepDims": (bool, False),
            })
        elif command == "create_function":
            input_tensor = args[0][self.layer_input_name(0)]
            args[0][self.layer_output_name(0)] = torch.sum(input_tensor, dim=self.props['Axes'] + 1,
                                                           keepdim=self.props['KeepDims'])
            self.clear_inputs(args[0])
            return True

    def BatchNormalization_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None,
                "DecayRate": (float, 0.9),
                "Epsilon": (float, 1e-4),
                "batch_stat": (bool, True),
                "beta.File": None,
                "gamma.File": None,
                "mean.File": None,
                "var.File": None
            }, )
        elif command == "create_function":
            input_tensor = args[0][self.layer_input_name(0)]
            bias_tensor = load_param_in_txt(self.props['b.File']) if self.props['beta.File'] else None
            weight_tensor = load_param_in_txt(self.props['gamma.File']) if self.props['gamma.File'] else None
            running_mean_tensor = load_param_in_txt(self.props['mean.File']) if self.props['mean.File'] else None
            running_var_tensor = load_param_in_txt(self.props['var.File']) if self.props['var.File'] else None

            if not self.function_instance:
                batch_norm_classes = {3: nn.BatchNorm1d, 4: nn.BatchNorm2d, 5: nn.BatchNorm3d}
                num_features = input_tensor.shape[1]
                momentum = 1 - self.props["DecayRate"]
                track_running_stats = not self.props["batch_stat"]
                self.function_instance = batch_norm_classes[len(input_tensor.shape)](num_features=num_features,
                                                                                     eps=self.props["Epsilon"],
                                                                                     momentum=momentum,
                                                                                     track_running_stats=track_running_stats).to(
                    input_tensor.device)
            with torch.no_grad():
                if 'state_dict' in self.props and self.props['state_dict']:
                    self.load_state()
                    del self.props['state_dict']
                if weight_tensor and bias_tensor:
                    self.function_instance.weight.copy_(weight_tensor)
                    self.function_instance.bias.copy_(bias_tensor)
                    self.props['beta.File'] = None
                    self.props['gamma.File'] = None
                if running_mean_tensor and running_var_tensor:
                    self.function_instance.running_mean.copy_(running_mean_tensor)
                    self.function_instance.running_var.copy_(running_var_tensor)
                    self.props['mean.File'] = None
                    self.props['var.File'] = None
            if self.owner.training_mode:
                self.function_instance.train()
            else:
                self.function_instance.eval()
            args[0][self.layer_output_name(0)] = self.function_instance(input_tensor)
            self.clear_inputs(args[0])
            return True

    def GroupNormalization_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None,
                "NumGroups": (int, 4),
                "Epsilon": (float, 1e-4),
                "beta.File": None,
                "gamma.File": None,
            }, )
        elif command == "create_function":
            input_tensor = args[0][self.layer_input_name(0)]
            bias_tensor = load_param_in_txt(self.props['b.File']) if self.props['beta.File'] else None
            weight_tensor = load_param_in_txt(self.props['gamma.File']) if self.props['gamma.File'] else None

            if not self.function_instance:
                self.function_instance = nn.GroupNorm(num_groups=self.props["NumGroups"],
                                                      num_channels=input_tensor.shape[1],
                                                      eps=self.props["Epsilon"]).to(input_tensor.device)
            with torch.no_grad():
                if 'state_dict' in self.props and self.props['state_dict']:
                    self.load_state()
                    del self.props['state_dict']
                if weight_tensor and bias_tensor:
                    self.function_instance.weight.copy_(weight_tensor)
                    self.function_instance.bias.copy_(bias_tensor)
                    self.props['beta.File'] = None
                    self.props['gamma.File'] = None
            if self.owner.training_mode:
                self.function_instance.train()
            else:
                self.function_instance.eval()
            args[0][self.layer_output_name(0)] = self.function_instance(input_tensor)
            self.clear_inputs(args[0])
            return True

    def InstanceNormalization_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None,
                "Epsilon": (float, 1e-4),
                "beta.File": None,
                "gamma.File": None
            }, )
        elif command == "create_function":
            input_tensor = args[0][self.layer_input_name(0)]
            bias_tensor = load_param_in_txt(self.props['b.File']) if self.props['beta.File'] else None
            weight_tensor = load_param_in_txt(self.props['gamma.File']) if self.props['gamma.File'] else None
            if not self.function_instance:
                instance_norm_classes = {3: nn.InstanceNorm1d, 4: nn.InstanceNorm2d, 5: nn.InstanceNorm3d}
                num_features = input_tensor.shape[1]
                self.function_instance = instance_norm_classes[len(input_tensor.shape)](num_features=num_features,
                                                                                        eps=self.props["Epsilon"]).to(
                    input_tensor.device)
            with torch.no_grad():
                if 'state_dict' in self.props and self.props['state_dict']:
                    self.load_state()
                    del self.props['state_dict']
                if weight_tensor and bias_tensor:
                    self.function_instance.weight.copy_(weight_tensor)
                    self.function_instance.bias.copy_(bias_tensor)
                    self.props['beta.File'] = None
                    self.props['gamma.File'] = None
            if self.owner.training_mode:
                self.function_instance.train()
            else:
                self.function_instance.eval()
            args[0][self.layer_output_name(0)] = self.function_instance(input_tensor)
            self.clear_inputs(args[0])
            return True

    def LayerNormalization_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None,
                "Epsilon": (float, 1e-4),
                "beta.File": None,
                "gamma.File": None,
            }, )
        elif command == "create_function":
            input_tensor = args[0][self.layer_input_name(0)]
            bias_tensor = load_param_in_txt(self.props['b.File']) if self.props['beta.File'] else None
            weight_tensor = load_param_in_txt(self.props['gamma.File']) if self.props['gamma.File'] else None

            if not self.function_instance:
                self.function_instance = nn.LayerNorm(normalized_shape=input_tensor.shape,
                                                      eps=self.props["Epsilon"]).to(
                    input_tensor.device)
            with torch.no_grad():
                if 'state_dict' in self.props and self.props['state_dict']:
                    self.load_state()
                    del self.props['state_dict']
                if weight_tensor and bias_tensor:
                    self.function_instance.weight.copy_(weight_tensor)
                    self.function_instance.bias.copy_(bias_tensor)
                    self.props['beta.File'] = None
                    self.props['gamma.File'] = None
            if self.owner.training_mode:
                self.function_instance.train()
            else:
                self.function_instance.eval()
            args[0][self.layer_output_name(0)] = self.function_instance(input_tensor)
            self.clear_inputs(args[0])
            return True

    def NormNormalization_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None,
                "P": (int, 2),
                "Axes": (to_shape, (1,)),
                "Epsilon": (float, 1e-12),
            }, )
        elif command == "create_function":
            input_tensor = args[0][self.layer_input_name(0)]
            args[0][self.layer_output_name(0)] = nn.functional.normalize(input=input_tensor,
                                                                         p=self.props["P"],
                                                                         dim=self.props["Axes"],
                                                                         eps=self.props["Epsilon"]
                                                                         )
            self.clear_inputs(args[0])
            return True

    def MaxPooling_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None,
                "KernelShape": (to_shape, "2,2"),
                "Strides": (to_shape, "1,1"),
                "Padding": (to_shape, "0,0"),
            })
        elif command == "create_function":
            input_tensor = args[0][self.layer_input_name(0)]
            if self.function_instance is None:
                max_pooling_classes = {3: nn.AvgPool1d, 4: nn.AvgPool2d, 5: nn.AvgPool3d}
                self.function_instance = max_pooling_classes[len(input_tensor.shape)](
                    kernel_size=self.props['KernelShape'],
                    stride=self.props['Strides'],
                    padding=self.props['Padding']
                ).to(input_tensor.device)
            args[0]["output"] = self.function_instance(input_tensor)
            if self.owner.training_mode:
                self.function_instance.train()
            else:
                self.function_instance.eval()
            args[0][self.layer_output_name(0)] = self.function_instance(input_tensor)
            self.clear_inputs(args[0])
            return True

    def AveragePooling_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None,
                "KernelShape": (to_shape, "2,2"),
                "Strides": (to_shape, "1,1"),
                "Padding": (to_shape, "0,0"),
            })
        elif command == "create_function":
            input_tensor = args[0][self.layer_input_name(0)]
            if self.function_instance is None:
                avg_pooling_classes = {3: nn.AvgPool1d, 4: nn.AvgPool2d, 5: nn.AvgPool3d}
                self.function_instance = avg_pooling_classes[len(input_tensor.shape)](
                    kernel_size=self.props['KernelShape'],
                    stride=self.props['Strides'],
                    padding=self.props['Padding']
                ).to(input_tensor.device)
            args[0]["output"] = self.function_instance(input_tensor)
            if self.owner.training_mode:
                self.function_instance.train()
            else:
                self.function_instance.eval()
            args[0][self.layer_output_name(0)] = self.function_instance(input_tensor)
            self.clear_inputs(args[0])
            return True

    def GlobalAveragePooling_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None
            })
        elif command == "create_function":
            input_tensor = args[0][self.layer_input_name(0)]
            if self.function_instance is None:
                self.function_instance = custom_layers.GlobalAvgPoolNd(
                ).to(input_tensor.device)
            args[0]["output"] = self.function_instance(input_tensor)
            if self.owner.training_mode:
                self.function_instance.train()
            else:
                self.function_instance.eval()
            args[0][self.layer_output_name(0)] = self.function_instance(input_tensor)
            self.clear_inputs(args[0])
            return True

    def SumPooling_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None,
                "KernelShape": (to_shape, "2,2"),
                "Padding": (to_shape, "0,0"),
                "Strides": (to_shape, "1,1"),
            })
        elif command == "create_function":
            input_tensor = args[0][self.layer_input_name(0)]
            if self.function_instance is None:
                self.function_instance = custom_layers.SumPool(
                    kernel_size=self.props['KernelShape'],
                    stride=self.props['Strides'],
                    padding=self.props['Padding']
                ).to(input_tensor.device)
            args[0]["output"] = self.function_instance(input_tensor)
            if self.owner.training_mode:
                self.function_instance.train()
            else:
                self.function_instance.eval()
            args[0][self.layer_output_name(0)] = self.function_instance(input_tensor)
            self.clear_inputs(args[0])
            return True

    def UnPooling_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None,
                "KernelShape": (to_shape, "2,2")
            })
        elif command == "create_function":
            input_tensor = args[0][self.layer_input_name(0)]
            if self.function_instance is None:
                self.function_instance = custom_layers.Unpooling(
                    kernel_size=self.props['KernelShape']
                ).to(input_tensor.device)
            args[0]["output"] = self.function_instance(input_tensor)
            if self.owner.training_mode:
                self.function_instance.train()
            else:
                self.function_instance.eval()
            args[0][self.layer_output_name(0)] = self.function_instance(input_tensor)
            self.clear_inputs(args[0])
            return True

    def RandomCrop_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None,
                "Shape": (str, None),
            })
        elif command == "create_function":
            input_tensor = args[0][self.layer_input_name(0)]
            if self.props['Stop'] == "*Input":
                Shape = input_tensor.shape
            else:
                Shape = (input_tensor.shape[0], *to_shape(self.props['Shape']))
            if self.function_instance is None:
                self.function_instance = torchvision.transforms.RandomCrop(size=Shape).to(input_tensor.device)
            if self.owner.training_mode:
                self.function_instance.train()
            else:
                self.function_instance.eval()
            args[0][self.layer_output_name(0)] = self.function_instance(input_tensor)
            self.clear_inputs(args[0])
            return True

    def RandomFlip_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None,
                "Seed": (int, -1),
                "Axes": (int, 0),
                "SkipAtTest": (bool, True),
            })
        elif command == "create_function":
            input_tensor = args[0][self.layer_input_name(0)]
            seed = self.props['Seed']
            axes = self.props['Axes'] + 1
            if seed != -1:
                random.seed(seed)
                torch.manual_seed(seed)
            if random.random() > 0.5:
                args[0][self.layer_output_name(0)] = torch.flip(input_tensor, dims=(axes,))
            else:
                args[0][self.layer_output_name(0)] = input_tensor
            self.clear_inputs(args[0])
            return True

    def RandomShift_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None,
                "Seed": (int, -1),
                "border_mode": (str, 'nearest'),
                "Shift": (to_shape, (0, 0)),
                "SkipAtTest": (bool, True),
            })
        elif command == "create_function":
            input_tensor = args[0][self.layer_input_name(0)]
            seed = self.props['Seed']
            if seed != -1:
                random.seed(seed)
                torch.manual_seed(seed)
            random_shifts = [random.randint(-shift, shift) for shift in self.props['Shift']]
            border_mode = self.props['border_mode']
            if border_mode == 'nearest':
                pad_mode = 'replicate'
            elif border_mode == 'reflect':
                pad_mode = 'reflect'
            # Apply padding to the tensor
            pad = []
            for shift in random_shifts:
                if shift > 0:
                    pad.extend([shift, 0])
                else:
                    pad.extend([0, -shift])

            padded_x = torch.nn.functional.pad(input_tensor, pad, mode=pad_mode, value=0)

            # Generate slices to shift the tensor
            slices = [slice(None)] * input_tensor.dim()  # Start with no slicing
            for i, shift in enumerate(random_shifts):
                dim = i+1
                if shift > 0:
                    slices[dim] = slice(shift, shift + input_tensor.size(dim))
                elif shift < 0:
                    slices[dim] = slice(0, shift)

            shifted_x = padded_x[tuple(slices)]
            args[0][self.layer_output_name(0)] = shifted_x
            self.clear_inputs(args[0])
            return True

    def RandomErase_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None,
                "Seed": (int, -1),
                "Prob": (float, 0.5),
                "AreaRatios": (to_shape, (0, 0)),
                "AspectRatios": (to_shape, (0, 0)),
                "Replacements": (to_shape, (0, 0)),
                "N": (int, -1),
                "Share": (bool, True),
                "STEFineGrained": (bool, True),
                "SkipAtTest": (bool, True),
            })
        elif command == "create_function":
            input_tensor = args[0][self.layer_input_name(0)]
            seed = self.props['Seed']
            if seed != -1:
                random.seed(seed)
                torch.manual_seed(seed)
            if random.random() > self.props['Prob']:
                args[0][self.layer_output_name(0)] = input_tensor
                self.clear_inputs(args[0])
                return True

            n_samples, n_channels, height, width = input_tensor.shape
            total_area = height * width
            area_ratios = self.props['AreaRatios']
            aspect_ratios = self.props['AspectRatios']
            replacements = self.props['Replacements']
            share = self.props['Share']
            for _ in range(self.props['N']):
                for i in range(n_samples):
                    erase_area = total_area * random.uniform(*area_ratios)
                    aspect_ratio = random.uniform(*aspect_ratios)
                    h = int(round(np.sqrt(erase_area * aspect_ratio)))
                    w = int(round(np.sqrt(erase_area / aspect_ratio)))

                    if h > height or w > width:
                        continue

                    top = random.randint(0, height - h)
                    left = random.randint(0, width - w)
                    replacement_value = random.uniform(*replacements)

                    if share:
                        input_tensor[:, :, top:top + h, left:left + w] = replacement_value
                    else:
                        input_tensor[i, :, top:top + h, left:left + w] = replacement_value
            args[0][self.layer_output_name(0)] = input_tensor
            self.clear_inputs(args[0])
            return True

    def OneHot_handle(self, command, *args, **kwargs):
        if command == "collect":
            section = args[0]
            self.get_property_value(section, {
                "Name": None,
                "Shape": (int, 5),
            })
        elif command == "create_function":
            input_tensor = args[0][self.layer_input_name(0)]
            args[0][self.layer_output_name(0)] = torch.nn.functional.one_hot(input_tensor,
                                                                             num_classes=self.props['Shape'])
            self.clear_inputs(args[0])
            return True

    def default_handle(self, command, *args, **kwargs):
        print(f"Unsupported layer: {self.type}!")

    def __init__(self):
        self.owner = None
        self.unit_instance = None
        self.type = "unknown"
        self.props = {}
        self.function_instance = None
        self.muti_inputs_ready = True
        self.layer_handlers = {
            # Io
            "Input": self.Input_handle,
            "Constant": self.Constant_handle,
            # Basic
            "Linear": self.Linear_handle,
            "Convolution": self.Convolution_handle,
            "DepthwiseConvolution": self.DepthwiseConvolution_handle,
            "Deconvolution": self.Deconvolution_handle,
            "DepthwiseDeconvolution": self.DepthwiseDeconvolution_handle,
            "Embed": self.Embed_handle,
            # Activation
            "ReLU": self.ReLU_handle,
            "CReLU": self.CReLU_handle,
            'LeakyReLU': self.LeakyReLU_handle,
            "PReLU": self.PReLU_handle,
            "Tanh": self.Tanh_handle,
            "HardTanh": self.HardTanh_handle,
            'TanhShrink': self.TanhShrink_handle,
            "Softmax": self.Softmax_handle,
            "Sigmoid": self.Sigmoid_handle,
            "HardSigmoid": self.HardSigmoid_handle,
            "LogSigmoid": self.LogSigmoid_handle,
            "Abs": self.Abs_handle,
            "ReLU6": self.ReLU6_handle,
            "ELU": self.ELU_handle,
            "CELU": self.CELU_handle,
            "SELU": self.SELU_handle,
            "GELU": self.GELU_handle,
            "SoftPlus": self.SoftPlus_handle,
            "SoftSign": self.SoftSign_handle,
            'Swish': self.Swish_handle,
            "Sinc": self.Sinc_handle,
            "Mish": self.Mish_handle,
            "LogSoftmax": self.LogSoftmax_handle,
            # Loss
            "CrossEntropyLoss": self.CrossEntropyLoss_handle,
            "AbsoluteError": self.AbsoluteError_handle,
            "BinaryCrossEntropy": self.BinaryCrossEntropy_handle,
            "EpsilonInsensitiveLoss": self.EpsilonInsensitiveLoss_handle,
            "HuberLoss": self.HuberLoss_handle,
            "SigmoidCrossEntropy": self.SigmoidCrossEntropy_handle,
            "SquaredError": self.SquaredError_handle,
            # Parameter
            "Parameter": self.Parameter_handle,
            # Pooling
            "MaxPooling": self.MaxPooling_handle,
            "AveragePooling": self.AveragePooling_handle,
            "GlobalAveragePooling": self.GlobalAveragePooling_handle,
            "SumPooling": self.SumPooling_handle,
            "Unpooling": self.UnPooling_handle,
            # Unit
            "Unit": self.Unit_handle,
            # Spectral
            "FFT": self.FFT_handle,
            "IFFT": self.IFFT_handle,
            # Math
            "Sum": self.Sum_handle,
            "CumSum": self.CumSum_handle,
            "Mean": self.Mean_handle,
            "Prod": self.Prod_handle,
            "CumProd": self.CumProd_handle,
            'Max': self.Max_handle,
            "Min": self.Min_handle,
            "Norm": self.Norm_handle,
            "Log": self.Log_handle,
            "Exp": self.Exp_handle,
            "Sign": self.Sign_handle,
            "BatchMatmul": self.BatchMatmul_handle,
            "BatchInv": self.BatchInv_handle,
            "Round": self.Round_handle,
            "Ceil": self.Ceil_handle,
            "Floor": self.Floor_handle,
            # Trigonometric
            "Sin": self.Sin_handle,
            "Cos": self.Cos_handle,
            "Tan": self.Tan_handle,
            "Sinh": self.Sinh_handle,
            "Cosh": self.Cosh_handle,
            "ASin": self.ASin_handle,
            "ACos": self.ACos_handle,
            "ATan": self.ATan_handle,
            "ATan2": self.ATan2_handle,
            "ASinh": self.ASinh_handle,
            "ACosh": self.ACosh_handle,
            "ATanh": self.ATanh_handle,
            # Arithmetic (Scalar)
            "AddScalar": self.AddScalar_handle,
            "MulScalar": self.MulScalar_handle,
            "RSubScalar": self.RSubScalar_handle,
            "RDivScalar": self.RDivScalar_handle,
            "PowScalar": self.PowScalar_handle,
            "RPowScalar": self.RPowScalar_handle,
            "MaximumScalar": self.MaximumScalar_handle,
            "MinimumScalar": self.MinimumScalar_handle,
            # Arithmetic (2 inputs)
            "AddN": self.AddN_handle,
            "Sub2": self.Sub2_handle,
            "MulN": self.MulN_handle,
            "Div2": self.Div2_handle,
            "Pow2": self.Pow2_handle,
            "MinimumN": self.MinimumN_handle,
            "MaximumN": self.MaximumN_handle,
            # Normalization
            "BatchNormalization": self.BatchNormalization_handle,
            "NormNormalization": self.NormNormalization_handle,
            "LayerNormalization": self.LayerNormalization_handle,
            "InstanceNormalization": self.InstanceNormalization_handle,
            "GroupNormalization": self.GroupNormalization_handle,
            # Others
            "Reshape": self.Reshape_handle,
            "Identity": self.Identity_handle,
            "Dropout": self.Dropout_handle,
            "Concatenate": self.Concatenate_handle,
            "Tile": self.Tile_handle,
            "Pad": self.Pad_handle,
            "Flip": self.Flip_handle,
            "Transpose": self.Transpose_handle,
            "Slice": self.Slice_handle,
            "Stack": self.Stack_handle,
            "MatrixDiag": self.MatrixDiag_handle,
            "MatrixDiagPart": self.MatrixDiagPart_handle,
            "ClipByValue": self.ClipByValue_handle,
            "ClipByNorm": self.ClipByNorm_handle,
            "TopKData": self.TopKData_handle,
            "Sort": self.Sort_handle,
            "Prune": self.Prune_handle,
            "Interpolate": self.Interpolate_hanle,
            "ResetNaN": self.ResetNaN_handle,
            "ResetInf": self.ResetInf_handle,
            "NmsDetection2d": self.NmnDetection2d_handle,
            "Comment": self.Comment_handle,
            # loop control
            "RepeatBlock": self.RepeatBlock_handle,
            # others (Pre process)
            "OneHot": self.OneHot_handle,
            "RandomCrop": self.RandomCrop_handle,
            "RandomFlip": self.RandomFlip_handle,
            "RandomShift": self.RandomShift_handle,
            "RandomErase": self.RandomErase_handle,
            # Argument
            "Argument": self.Argument_handle,
        }

    def get_property_value(self, section, req_prop_dict):
        n = int(section['NumProperty'])
        for i in range(n):
            if section[f"Property_{i}_Name"] in req_prop_dict:
                prop_name = section[f"Property_{i}_Name"]
                if req_prop_dict[section[f"Property_{i}_Name"]] is not None:
                    conversion, default_value = req_prop_dict[section[f"Property_{i}_Name"]]
                    if section[f"Property_{i}_Value"].startswith('*'):
                        self.props[prop_name] = section[f"Property_{i}_Value"]
                    else:
                        self.props[prop_name] = conversion(section[f"Property_{i}_Value"])
                else:
                    self.props[prop_name] = section[f"Property_{i}_Value"]

        for k, v in req_prop_dict.items():
            if k not in self.props:
                if v is not None:
                    _, def_value = v
                    self.props[k] = def_value

        for k, v in self.props.items():
            if type(v) == str and v.startswith('*'):
                key = v[1:]
                if key in self.props:
                    self.props[k] = self.props[key]

        self.props['state_dict'] = None

        # set network.lookup_table[id] = layer
        if section['ID'] not in self.owner.lookup_table:
            self.owner.lookup_table[int(section['ID'])] = self

    def create_unit_instance(self, data):
        unit_net_name = self.props['Network'] + '_Network_Global'
        unit_network = self.owner.global_networks[unit_net_name]
        # Change input layers to Identity layer in unit, since the input is from Main net
        unit_input = dict()
        for layer_id, layer in unit_network.lookup_table.items():
            if layer.type == 'Input':
                input_name = layer.layer_input_name()
                layer.type = 'Identity'
                unit_input[input_name] = data
        unit_network.infer(unit_input)
        self.props['Input_name'] = input_name
        self.unit_instance = unit_network

    def layer_input_name(self, index=0):
        return f"{self.props['Name']}_input_{index}"

    def layer_output_name(self, index=0):
        return f"{self.props['Name']}_output_{index}"

    def clear_inputs(self, data, inputs=1):
        for i in range(inputs):
            if self.layer_input_name(i) in data:
                del data[self.layer_input_name(i)]

    def clear_pined_connect(self, data, input_name, connect_num):
        for i in range(connect_num):
            if f"{input_name}_pined_connect_{i}" in data:
                del data[f"{input_name}_pined_connect_{i}"]

    @staticmethod
    def create_from_type(owner, layer_type, section):
        layer = Layer()
        layer.owner = owner
        layer.type = layer_type
        layer.process_section(section)
        return layer

    def check_inputs(self, name, data):
        if name in self.props and name not in data:
            return False
        return True

    def process_section(self, section):
        process = self.layer_handlers.get(self.type, self.default_handle)
        process("collect", section)

    def create_and_infer(self, data):
        process = self.layer_handlers.get(self.type, self.default_handle)
        ret = process("create_function", data)
        # print(f"create_and_infer: {self.type}, ret={ret}")
        return ret

    def load_state(self):
        params = {key: torch.tensor(value) for key, value in self.props['state_dict'].items()}
        self.function_instance.load_state_dict(params)
