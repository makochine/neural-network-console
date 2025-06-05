# Copyright 2025 Sony Group Corporation.
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

.PHONY: all
all: nnabla torch common

NNABLA_DEPS_DATASET = train conv_dataset create_image_classification_dataset create_tar
NNABLA_DEPS_TRAINED = infer forward draw_graph function_info dump nnb_template
NNABLA_NO_DEPS = profile compare_with_cpu
NNABLA_ALL_TEST = $(NNABLA_DEPS_DATASET) $(NNABLA_DEPS_TRAINED) $(NNABLA_NO_DEPS)
NNABLA_ALL_SUBDIRS = $(patsubst %,nnabla/output/%,$(NNABLA_ALL_TEST))

COMMON_ALL_TEST = plot_series plot_timer version 
COMMON_ALL_SUBDIRS = $(patsubst %,common/output/%,$(COMMON_ALL_TEST))

.PHONY: nnabla torch common
nnabla: $(NNABLA_ALL_TEST)
torch:
common: plot_series plot_timer version

.PHONY: train conv_dataset create_image_classification_dataset create_tar
train:                               test-env sample_dataset/MNIST sample_dataset/MNIST_gif nnabla/output/train
conv_dataset:                        test-env sample_dataset/MNIST sample_dataset/MNIST_gif nnabla/output/conv_dataset
create_image_classification_dataset: test-env sample_dataset/MNIST sample_dataset/MNIST_gif nnabla/output/create_image_classification_dataset
create_tar:                          test-env sample_dataset/MNIST sample_dataset/MNIST_gif nnabla/output/create_tar

.PHONY: infer forward draw_graph function_info dump nnb_template
infer:         test-env nnabla/output/train sample_dataset/infer_data nnabla/output/infer
forward:       test-env nnabla/output/train sample_dataset/infer_data nnabla/output/forward
draw_graph:    test-env nnabla/output/train sample_dataset/infer_data nnabla/output/draw_graph
function_info: test-env nnabla/output/train sample_dataset/infer_data nnabla/output/function_info
dump:          test-env nnabla/output/train sample_dataset/infer_data nnabla/output/dump
nnb_template:  test-env nnabla/output/train sample_dataset/infer_data nnabla/output/nnb_template

.PHONY: profile compare_with_cpu
profile:          test-env nnabla/output/profile
compare_with_cpu: test-env nnabla/output/compare_with_cpu

.PHONY: plot_series plot_timer version
plot_series: test-env common/output/plot_series
plot_timer:  test-env common/output/plot_timer
version:     test-env common/output/version

.PHONY: clean
clean:
	rm -rf sample_dataset/MNIST
	rm -rf sample_dataset/MNIST_gif
	rm -rf sample_dataset/infer_data
	rm -rf nnabla/output
	rm -rf common/output
	rm -rf test-env


$(NNABLA_ALL_SUBDIRS):
	. test-env/bin/activate \
	&& cd nnabla/scripts \
	&& bash test_$(patsubst nnabla/output/%,%,$@).sh

$(COMMON_ALL_SUBDIRS):
	. test-env/bin/activate \
	&& cd common/scripts \
	&& bash test_$(patsubst common/output/%,%,$@).sh

test-env:
	python3 -m venv test-env
	. test-env/bin/activate \
	&& pip install -U pip \
	&& pip install nnabla nnabla-ext-cuda120 opencv-python graphviz matplotlib \
	&& pip install ../dist/nn_cli-*.whl

sample_dataset/MNIST:
	mkdir -p sample_dataset/MNIST
	curl -LO https://raw.githubusercontent.com/sony/nnc-plugin/refs/heads/main/plugins/_Pre_Process/_Download_or_Generate_Dataset/create_mnist_csv.py
	. test-env/bin/activate && python create_mnist_csv.py -o sample_dataset/MNIST
	rm -f create_mnist_csv.py

sample_dataset/MNIST_gif:
	. test-env/bin/activate \
	&& python scripts/change_format.py --maxnum 0 --srcdir sample_dataset/MNIST/training   --outdir sample_dataset/MNIST_gif/training \
	&& python scripts/change_format.py --maxnum 0 --srcdir sample_dataset/MNIST/validation --outdir sample_dataset/MNIST_gif/validation
	sed s/png/gif/ sample_dataset/MNIST/small_mnist_4or9_training.csv > sample_dataset/MNIST_gif/small_mnist_4or9_training_gif.csv
	sed s/png/gif/ sample_dataset/MNIST/small_mnist_4or9_test.csv     > sample_dataset/MNIST_gif/small_mnist_4or9_test_gif.csv

sample_dataset/infer_data:
	. test-env/bin/activate \
	&& python scripts/image2bin.py \
	   --srcdir ./sample_dataset/MNIST/validation \
	   --outdir ./sample_dataset/infer_data/bin --plane 1 --maxnum 2
	cp nnabla/output/train/train-with-cpu/results.nnp sample_dataset/infer_data/
	cd sample_dataset/infer_data \
	&& unzip results.nnp 01_logistic_regression.nntxt parameter.h5 \
	&& . ../../test-env/bin/activate \
	&& nnabla_cli convert -E0 --nnp-parameter-h5 results.nnp results-executor.nnp \
	&& unzip results-executor.nnp network.nntxt \
	&& sed '/dataset {/,/}/d' network.nntxt > network-executor-only.nntxt \
	&& rm -f results-executor.nnp network.nntxt \
	&& sed -e 's/backend: "cpu"/backends: "cudnn:float"\n    backends: "cuda:float"\n    backends: "cpu:float"/' -e 's/array_class: "CpuArray"/array_class: "CudaCachedArray"/' -e 's/compute_backend: "default"/compute_backend: "default|cudnn"/' 01_logistic_regression.nntxt > network-gpu.nntxt
