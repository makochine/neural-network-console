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

from __future__ import print_function

import os
import logging
from nn_cli import torch_utility


# subcommands
# ===========
def decode_param_command(args, **kwargs):
    os.makedirs(args.outdir, exist_ok=True)
    # Load parameter
    logging.log(99, 'Loading parameters...')
    torch_utility.extract_param_from_nnpt_h5(args.param, args.outdir)
    logging.log(99, 'Decode Parameter Completed.')
    return True


def encode_param_command(args, **kwargs):
    logging.log(99, 'Saving parameters...')
    torch_utility.save_param_txt_to_h5(args.indir, args.param)
    logging.log(99, 'Encode Parameter Completed.')
    return True
