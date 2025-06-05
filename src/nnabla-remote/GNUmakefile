# Copyright 2020,2021,2022,2023,2024,2025 Sony Group Corporation.
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
all: wheel test

.PHONY: clean
clean:
	@rm -rf dist
	@rm -rf nnabla_ssh/build
	@rm -rf nnabla_ssh/nnabla_ssh.egg-info
	@rm -rf wheel-env
	@rm -rf test/work
	@rm -rf test/data/dataset

.PHONY: wheel
wheel: setup-venv
	. wheel-env/bin/activate && \
	python -m build --wheel --no-isolation --outdir dist nnabla_ssh

#export REMOTE_SSH_LOCAL_SERVER_HOST := ""
#export REMOTE_SSH_LOCAL_SERVER_PRIVATE_KEY := ""
#export REMOTE_SSH_LOCAL_SERVER_USER := ""
.PHONY: test
test: wheel setup-test setup-dataset
	@./test/scripts/ssh-test-exec.sh direct
	@./test/scripts/ssh-test-exec.sh docker # need rootful docker

.PHONY: format
format:
	@autopep8 -i -v -r --max-line-length 120 --ignore E402 nnabla_ssh/nnabla_ssh


.PHONY: setup-venv
setup-venv: ./wheel-env

./wheel-env:
	python3 -m venv wheel-env
	. wheel-env/bin/activate && \
	pip install -U pip setuptools && \
	pip install build wheel


.PHONY: setup-test
setup-test: ./test/work/env

./test/work/env:
	mkdir -p ./test/work
	python3 -m venv ./test/work/env
	. ./test/work/env/bin/activate && \
	pip install -U pip && \
	pip install ./dist/nnabla_ssh-*.whl && \
	pip install pysocks awscli ipython


.PHONY: setup-dataset
setup-dataset: ./test/data/dataset/small_mnist_4or9_test.csv

./test/data/dataset/small_mnist_4or9_test.csv:
	mkdir -p test/data/dataset
	curl -LO https://raw.githubusercontent.com/sony/nnc-plugin/refs/heads/main/plugins/_Pre_Process/_Download_or_Generate_Dataset/create_mnist_csv.py
	. ./test/work/env/bin/activate && python3 create_mnist_csv.py -o test/data/dataset
	rm -rf test/data/dataset/training/[01235678]
	rm -rf test/data/dataset/validation/[01235678]
	rm -f test/data/dataset/mnist*.csv test/data/dataset/small_mnist_4or9_test_w_initmemory.csv
	rm -f create_mnist_csv.py
