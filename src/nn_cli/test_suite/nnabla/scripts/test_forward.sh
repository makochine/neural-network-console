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
echo '# Test forward in nnabla backend'

script_dir=$(dirname "$0")


echo '################################################################################'
echo '# Test forward with dataset'
rm -rf $script_dir/../output/forward/forward_with_dataset
mkdir -p $script_dir/../output/forward/forward_with_dataset
nn_cli forward \
-c $script_dir/../../sample_dataset/infer_data/01_logistic_regression.nntxt \
-p $script_dir/../../sample_dataset/infer_data/parameter.h5 \
-d $script_dir/../../sample_dataset/MNIST/small_mnist_4or9_test.csv \
-o $script_dir/../output/forward/forward_with_dataset


echo '################################################################################'
echo '# Test forward without dataset'
rm -rf $script_dir/../output/forward/forward_without_dataset
mkdir -p $script_dir/../output/forward/forward_without_dataset
nn_cli forward \
-c  $script_dir/../../sample_dataset/infer_data/network-executor-only.nntxt \
-p $script_dir/../../sample_dataset/infer_data/parameter.h5 \
-d $script_dir/../../sample_dataset/MNIST/small_mnist_4or9_test.csv \
-o $script_dir/../output/forward/forward_without_dataset
