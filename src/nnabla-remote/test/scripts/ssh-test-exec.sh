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

if [[ ! -v REMOTE_SSH_LOCAL_SERVER_HOST || ! -v REMOTE_SSH_LOCAL_SERVER_PRIVATE_KEY || ! -v REMOTE_SSH_LOCAL_SERVER_USER ]]; then
    echo Please set 3 environment variables for this test.
    echo "  REMOTE_SSH_LOCAL_SERVER_HOST"
    echo "  REMOTE_SSH_LOCAL_SERVER_PRIVATE_KEY"
    echo "  REMOTE_SSH_LOCAL_SERVER_USER"
    exit 1
fi

set -eux

export BASE=$(readlink -f $(dirname $0)/..)/work
mkdir -p ${BASE}
cd ${BASE}
cp -f ../settings/nnabla.conf .

. env/bin/activate

DATASET_DIR=$(readlink -f ../data/dataset)
mkdir -p cache
CACHE_DIR=$(readlink -f ./cache)
cat ../data/net.nntxt.tmpl |sed -e "s%XXX_DATASET_DIR_XXX%${DATASET_DIR}%g" -e "s%XXX_CACHE_DIR_XXX%${CACHE_DIR}%g" >net.nntxt

if [ ! -e ../settings/$1.txt ]
then
    echo Test $1 does not exist.
    exit -1
fi

. ../settings/$1.txt

for n in ${AVAILABLE_TESTS}
do
    set +x
    echo '################################################################################'
    echo '################################################################################'
    eval "echo \${REMOTE_PROCESSOR_${n}_NAME}"
    echo '################################################################################'
    echo '################################################################################'
    echo
    set -x
    mkdir -p out/$1/$n
    export TRAIN_PROCESSOR_INDEX=$n
    nnabla_cli train -c net.nntxt -o out/$1/$n

    export FORWARD_PROCESSOR_INDEX=$n
    nnabla_cli forward -c out/$1/$n/results.nnp -d ../data/dataset/small_mnist_4or9_test.csv --replace_path -o out/$1/$n

    export PROFILE_PROCESSOR_INDEX=$n
    nnabla_cli profile -c net.nntxt -o out/$1/$n
done
