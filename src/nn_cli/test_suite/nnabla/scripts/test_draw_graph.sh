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
echo '# Test draw_graph in nnabla backend'

script_dir=$(dirname "$0")


echo '################################################################################'
echo '# Draw all networks in a nntxt and save'
rm -rf $script_dir/../output/draw_graph/nntxt_all
mkdir -p $script_dir/../output/draw_graph/nntxt_all
nn_cli draw_graph \
-o $script_dir/../output/draw_graph/nntxt_all \
-f pdf \
$script_dir/../../sample_dataset/infer_data/01_logistic_regression.nntxt


echo '################################################################################'
echo '# Draw a certain network in a nntxt and save'
rm -rf $script_dir/../output/draw_graph/nntxt_main
mkdir -p $script_dir/../output/draw_graph/nntxt_main
nn_cli draw_graph \
-o $script_dir/../output/draw_graph/nntxt_main \
-f pdf \
-n Main \
$script_dir/../../sample_dataset/infer_data/01_logistic_regression.nntxt


echo '################################################################################'
echo '# Draw all networks in a nnp and save'
rm -rf $script_dir/../output/draw_graph/nnp_all
mkdir -p $script_dir/../output/draw_graph/nnp_all
nn_cli draw_graph \
-o $script_dir/../output/draw_graph/nnp_all \
-f pdf \
$script_dir/../../sample_dataset/infer_data/results.nnp


echo '################################################################################'
echo '# Draw a certain network in a nnp and save'
rm -rf $script_dir/../output/draw_graph/nnp_main
mkdir -p $script_dir/../output/draw_graph/nnp_main
nn_cli draw_graph \
-o $script_dir/../output/draw_graph/nnp_main \
-f pdf \
-n Main \
$script_dir/../../sample_dataset/infer_data/results.nnp


