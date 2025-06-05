# Copyright 2019,2020,2021 Sony Corporation.
# Copyright 2021,2022,2023,2024,2025 Sony Group Corporation.
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

SDEEP_CONSOLE_DIRECTORY ?= $(shell pwd)
BUILD_DIRECTORY_SDEEP_CONSOLE ?= $(SDEEP_CONSOLE_DIRECTORY)/build
DOCKER_IMAGE_SDEEP_CONSOLE_BUILD ?= sdeep_console_build
DOCKER_RUN_OPTS +=--rm
DOCKER_RUN_OPTS += -v $$(pwd):$$(pwd)
DOCKER_RUN_OPTS += -w $$(pwd)
DOCKER_RUN_OPTS += -u $$(id -u):$$(id -g)
DOCKER_RUN_OPTS += -e HOME=/tmp
DOCKER_RUN_OPTS += -e CMAKE_OPTS=$(CMAKE_OPTS)

NNC_PLUGIN=nnc-plugin


all: doc bwd-sdeep-console-cli

.PHONY: sdeep-console-cli
sdeep-console-cli:
	mkdir -p $(BUILD_DIRECTORY_SDEEP_CONSOLE)
	cd $(BUILD_DIRECTORY_SDEEP_CONSOLE) && cmake $(SDEEP_CONSOLE_DIRECTORY)
	make -C $(BUILD_DIRECTORY_SDEEP_CONSOLE)
	rm -rf $(BUILD_DIRECTORY_SDEEP_CONSOLE)/settings
	cp -rf $(SDEEP_CONSOLE_DIRECTORY)/settings $(BUILD_DIRECTORY_SDEEP_CONSOLE)
	echo artifacts:
	ls -l $(SDEEP_CONSOLE_DIRECTORY)/sdeep_console_cli_util
	ls -ld $(SDEEP_CONSOLE_DIRECTORY)/settings

.PHONY: bwd-sdeep-console-cli
bwd-sdeep-console-cli:
	docker build $(DOCKER_BUILD_ARGS) -t $(DOCKER_IMAGE_SDEEP_CONSOLE_BUILD) -f build-tools/docker/Dockerfile .
	docker run $(DOCKER_RUN_OPTS) $(DOCKER_IMAGE_SDEEP_CONSOLE_BUILD) $(MAKE) sdeep-console-cli


.PHONY: doc
doc: update-plugin-doc doc-ja doc-en

.PHONY: clean-doc
clean-doc:
	rm -rf document/en/source/Plugin
	rm -rf document/ja/source/Plugin
	rm -rf document/build
	rm -rf document/html
	rm -f document/manual-en.pdf document/manual.pdf

.PHONY: update-plugin-doc
update-plugin-doc: validate_directory_exists
	cp -r $(NNC_PLUGIN)/manuals ./document/en/source/Plugin
	mv ./document/en/source/Plugin/ja  ./document/ja/source/Plugin

.PHONY: doc-ja
doc-ja: doc-ja-html doc-ja-pdf

.PHONY: doc-ja-html
doc-ja-html:
	@mkdir -p document/build
	rsync -a --delete document/ja/ document/build/ja-html
	docker run --rm -u $$(id -u) -v $$(pwd)/document:/docs -w /docs/build/ja-html sphinxdoc/sphinx-latexpdf make html
	@mkdir -p document/html
	rm -rf document/html/ja
	mv document/build/ja-html/build/html document/html/ja

.PHONY: doc-ja-pdf
doc-ja-pdf:
	@mkdir -p document/build
	rsync -a --delete document/ja/ document/build/ja-pdf
	docker run --rm -u $$(id -u) -v $$(pwd)/document:/docs -w /docs/build/ja-pdf sphinxdoc/sphinx-latexpdf make latexpdf
	mv document/build/ja-pdf/build/latex/neuralnetworkconsole.pdf document/manual.pdf

.PHONY: doc-en
doc-en: doc-en-html doc-en-pdf

.PHONY: doc-en-html
doc-en-html:
	@mkdir -p document/build
	rsync -a --delete document/en/ document/build/en-html
	docker run --rm -u $$(id -u) -v $$(pwd)/document:/docs -w /docs/build/en-html sphinxdoc/sphinx-latexpdf make html
	@mkdir -p document/html
	rm -rf document/html/en
	mv document/build/en-html/build/html document/html/en

.PHONY: doc-en-pdf
doc-en-pdf:
	@mkdir -p document/build
	rsync -a --delete document/en/ document/build/en-pdf
	docker run --rm -u $$(id -u) -v $$(pwd)/document:/docs -w /docs/build/en-pdf sphinxdoc/sphinx-latexpdf make latexpdf
	mv document/build/en-pdf/build/latex/neuralnetworkconsole.pdf document/manual-en.pdf

.PHONY: doc-clean
doc-clean:
	rm -rf document/en/source/Plugin
	rm -rf document/ja/source/Plugin
	rm -rf document/build
	rm -rf document/html
	rm -rf document/*.pdf


.PHONY: attention-editor
attention-editor:
	cd nnc-plugin/tools/AttentionEditor
	make -C nnc-plugin/tools/AttentionEditor win


.PHONY: validate_directory_exists
validate_directory_exists:
ifeq ("$(wildcard $(NNC_PLUGIN))", "")
	$(error $(NNC_PLUGIN) directory is not found)
endif
