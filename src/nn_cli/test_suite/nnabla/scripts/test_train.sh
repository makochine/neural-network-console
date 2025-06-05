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
echo '# Test train in nnabla backend'

script_dir=$(dirname "$0")


echo '################################################################################'
echo '# Clear output path'
rm -rf  $script_dir/../output/train
mkdir -p $script_dir/../output/train/train-with-cpu


echo '################################################################################'
echo '# Train with CPU'
nn_cli train -c $script_dir/../../sample_project/train-with-cpu/01_logistic_regression.nntxt \
-o $script_dir/../output/train/train-with-cpu

echo '################################################################################'
echo '# Train with CPU AMSGRAD'
rm -rf $script_dir/../output/train/train-amsgrad
mkdir -p $script_dir/../output/train/train-amsgrad
nn_cli train -c $script_dir/../../sample_project/train-with-cpu/01_logistic_regression_amsgrad.nntxt \
-o $script_dir/../output/train/train-amsgrad


echo '################################################################################'
echo '# Evaluation'
nn_cli forward -c $script_dir/../../sample_project/train-with-cpu/01_logistic_regression.nntxt \
-p $script_dir/../output/train/train-with-cpu/results.nnp \
-d $script_dir/../../sample_dataset/MNIST/small_mnist_4or9_test.csv \
-o $script_dir/../output/train/train-with-cpu/


echo '################################################################################'
echo '# Clear output path'
rm -rf $script_dir/../output/train/output_resume
mkdir -p $script_dir/../output/train/output_resume


echo '################################################################################'
echo '# CPU (terminate)'
nn_cli train -c $script_dir/../../sample_project/train-with-cpu/01_logistic_regression.nntxt -o \
$script_dir/../output/train/output_resume >& $script_dir/../output/train/output_resume/train.log &
pid=$!
(while ! grep -q 'epoch 20 of 50' $script_dir/../output/train/output_resume/train.log ; do sleep 0.1; done; kill $!) &
wait $pid || true
cat $script_dir/../output/train/otuput_resume/train.log 


echo '################################################################################'
echo '# Resume (terminate)'
nn_cli train -r -c $script_dir/../../sample_project/train-with-cpu/01_logistic_regression.nntxt \
-o $script_dir/../output/train/output_resume


echo '################################################################################'
echo '# Zero epoch learning'
rm -rf $script_dir/../output/train/zero-epoch-learning/
mkdir -p $script_dir/../output/train/zero-epoch-learning/
nn_cli train -c $script_dir/../../sample_project/zero-epoch-learning/01_logistic_regression.nntxt \
-o $script_dir/../output/train/zero-epoch-learning/

echo '################################################################################'
echo '# MNIST vae learning'
rm -rf $script_dir/../output/train/mnist-vae/
mkdir -p $script_dir/../output/train/mnist-vae/
nn_cli train -c $script_dir/../../sample_project/train-with-gpu/mnist_vae.nntxt \
-o $script_dir/../output/train/mnist-vae
