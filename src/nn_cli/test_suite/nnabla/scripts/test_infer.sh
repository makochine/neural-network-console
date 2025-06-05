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
echo '# Test infer in nnabla backend'

script_dir=$(readlink -f $(dirname "$0"))


echo '################################################################################'
echo '# Infer with CPU'
rm -rf $script_dir/../output/infer/infer_with_cpu
mkdir -p $script_dir/../output/infer/infer_with_cpu
for file in $script_dir/../../sample_dataset/infer_data/bin/*/*; do
    nn_cli infer \
    -c $script_dir/../../sample_dataset/infer_data/01_logistic_regression.nntxt  \
    -p $script_dir/../../sample_dataset/infer_data/parameter.h5 \
    -o $script_dir/../output/infer/infer_with_cpu/ \
    $file
done

echo '################################################################################'
echo '# Infer without dataset'
rm -rf $script_dir/../output/infer/infer_without_dataset
mkdir -p $script_dir/../output/infer/infer_without_dataset
for file in $script_dir/../../sample_dataset/infer_data/bin/*/*; do
    nn_cli infer \
    -c $script_dir/../../sample_dataset/infer_data/network-executor-only.nntxt \
    -p $script_dir/../../sample_dataset/infer_data/parameter.h5 \
    -o $script_dir/../output/infer/infer_without_dataset/ \
    $file
done


echo '################################################################################'
echo '# Infer (GPU)'
rm -rf $script_dir/../output/infer/infer_with_gpu
mkdir -p $script_dir/../output/infer/infer_with_gpu
for file in $script_dir/../../sample_dataset/infer_data/bin/*/*; do
    nn_cli infer \
    -c $script_dir/../../sample_dataset/infer_data/network-gpu.nntxt \
    -p $script_dir/../../sample_dataset/infer_data/parameter.h5 \
    -o $script_dir/../output/infer/infer_with_gpu/ \
    $file
done
