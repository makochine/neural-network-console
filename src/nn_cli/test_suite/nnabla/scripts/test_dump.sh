#!/bin/bash
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

echo '################################################################################'
echo '# Test dump in nnabla backend'

script_dir=$(dirname "$0")


echo '################################################################################'
echo '# dump a nnp file'
rm -rf $script_dir/../output/dump
mkdir -p $script_dir/../output/dump
nn_cli dump \
$script_dir/../output/train/mnist-vae/results.nnp | \
tee $script_dir/../output/dump/dump.log


echo '################################################################################'
echo '# dump a nnp file with funciton list'
nn_cli dump --dump-functions Affine Affine_2 \
$script_dir/../output/train/mnist-vae/results.nnp | \
tee $script_dir/../output/dump/dump_function_list.log


echo '################################################################################'
echo '# dump a nnp file with limit 1'
nn_cli dump --dump-limit 1 \
$script_dir/../output/train/mnist-vae/results.nnp | \
tee $script_dir/../output/dump/dump_limit_1.log


echo '################################################################################'
echo '# dump a nnp file with variable name'
nn_cli dump -n Input \
$script_dir/../output/train/mnist-vae/results.nnp | \
tee $script_dir/../output/dump/dump_Input.log


echo '################################################################################'
echo '# dump a nnp file with variable list'
nn_cli dump --dump-variables Input Input_2 \
$script_dir/../output/train/mnist-vae/results.nnp | \
tee $script_dir/../output/dump/dump_Input_list.log

