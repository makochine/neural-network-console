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
echo '# Test create_image_classification_dataset in nnabla backend'

script_dir=$(dirname "$0")


echo '################################################################################'
echo '# create image classification dataset'
rm -rf $script_dir/../output/create_image_classification_dataset/no_shuffle
mkdir -p $script_dir/../output/create_image_classification_dataset/no_shuffle
nn_cli create_image_classification_dataset -i $script_dir/../../sample_dataset/MNIST/training \
-o $script_dir/../output/create_image_classification_dataset/no_shuffle \
-c 1 -w 128  -g 128 -m padding -s false \
-f1 small_mnist_4or9_training_gif.csv \
-f2 small_mnist_4or9_test_gif.csv  -r2 20

echo '################################################################################'
echo '# create image classification dataset with shuffle'
rm -rf $script_dir/../output/create_image_classification_dataset/shuffle
mkdir -p $script_dir/../output/create_image_classification_dataset/shuffle
nn_cli create_image_classification_dataset -i $script_dir/../../sample_dataset/MNIST/training \
-o $script_dir/../output/create_image_classification_dataset/shuffle \
-c 1 -w 128  -g 128 -m padding -s true \
-f1 small_mnist_4or9_training_gif_suffle.csv \
-f2 small_mnist_4or9_test_gif_suffle.csv  -r2 20

