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

nnabla_enabled = True
torch_enabled = True

try:
    import nnabla
except:
    nnabla_enabled = False

try:
    import torch
except:
    torch_enabled = False

if nnabla_enabled:
    import nn_cli.nnabla_backend as nnabla_backend

if torch_enabled:
    import nn_cli.torch_backend as torch_backend


def select_backend_by_config_file(config_file):
    ext = os.path.splitext(config_file)[1]
    if ext == '.nntxt':
        return nnabla_backend
    elif ext == ".nnp":
        return nnabla_backend
    elif ext == '.sdcproj':
        return torch_backend
    elif ext == '.nnpt':
        return torch_backend
    return None


def select_backend(backend_name):
    if backend_name == 'NNabla':
        return nnabla_backend
    elif backend_name == 'Torch':
        return torch_backend
    else:
        raise ValueError(f"Unsupported backend: {backend_name}. Please choose either 'NNabla' or 'Torch'.")
