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
echo '# Test profile in nnabla backend'

script_dir=$(dirname "$0")


echo '################################################################################'
echo '# Test profile'
rm -rf $script_dir/../output/profile
mkdir -p $script_dir/../output/profile
nn_cli profile \
-c $script_dir/../../sample_project/train-with-cpu/01_logistic_regression.nntxt \
-o $script_dir/../output/profile
