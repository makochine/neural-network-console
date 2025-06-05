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
echo '# Test plot_series in common backend'

script_dir=$(dirname "$0")


echo '################################################################################'
echo '# Plot series data from MonitorSeries output text file'
rm -rf $script_dir/../output/plot_series
mkdir -p $script_dir/../output/plot_series
nn_cli plot_series $script_dir/../../sample_dataset/time-series/Test-error.series.txt -o $script_dir/../output/plot_series/Test-error-series.jpeg
echo Saved file: $script_dir/../output/plot_series/Test-error-series.jpeg

