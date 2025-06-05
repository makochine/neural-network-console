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
echo '# Test function_info in nnabla backend'

script_dir=$(dirname "$0")

rm -rf $script_dir/../output/function_info
mkdir -p $script_dir/../output/function_info

echo '################################################################################'
echo '# test function_info'
nn_cli function_info \
$script_dir/../output/train/mnist-vae/results.nnp \
-o $script_dir/../output/function_info/expanded.txt


echo '################################################################################'
echo '# test function_info with all_support ONNX'
nn_cli function_info \
$script_dir/../output/train/mnist-vae/results.nnp \
--all_support onnx > $script_dir/../output/function_info/all_support_onnx.txt


echo '################################################################################'
echo '# test function_info with all_support NNB'
nn_cli function_info \
$script_dir/../output/train/mnist-vae/results.nnp \
--all_support nnb  > $script_dir/../output/function_info/all_support_nnb.txt


echo '################################################################################'
echo '# test function_info with exclude function'
nn_cli function_info \
$script_dir/../output/train/mnist-vae/results.nnp \
-x Affine > $script_dir/../output/function_info/exclude.txt


