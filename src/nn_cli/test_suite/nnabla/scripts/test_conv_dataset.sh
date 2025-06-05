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
echo '# Test conv_dataset in nnabla backend'

script_dir=$(dirname "$0")


echo '################################################################################'
echo '# Test creating cache'
rm -rf $script_dir/../output/conv_dataset
mkdir -p $script_dir/../output/conv_dataset
nn_cli conv_dataset \
$script_dir/../../sample_dataset/MNIST_gif/small_mnist_4or9_test_gif.csv \
$script_dir/../output/conv_dataset/test_gif.cache


echo '################################################################################'
echo '# Test creating cache with suffle'
nn_cli conv_dataset -S \
$script_dir/../../sample_dataset/MNIST_gif/small_mnist_4or9_test_gif.csv \
$script_dir/../output/conv_dataset/test_gif_suffle.cache


echo '################################################################################'
echo '# Test creating cache and normalize data range'
nn_cli conv_dataset -N \
$script_dir/../../sample_dataset/MNIST_gif/small_mnist_4or9_test_gif.csv \
$script_dir/../output/conv_dataset/test_gif_normalize.cache


echo '################################################################################'
echo '# Test creating cache with suffle and normalize data range'
nn_cli conv_dataset -S -N \
$script_dir/../../sample_dataset/MNIST_gif/small_mnist_4or9_test_gif.csv \
$script_dir/../output/conv_dataset/test_gif_suffle_normalize.cache


