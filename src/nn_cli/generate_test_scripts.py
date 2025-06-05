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

import os
import yaml


def generate_test_script(backend, command):
    script_content = f'''#!/bin/bash

echo '################################################################################'
echo '# Test {command} in {backend} backend'

script_dir=$(dirname "$0")


'''
    backend_folder = f'test_suite/{backend}'
    os.makedirs(backend_folder, exist_ok=True)
    config_path = f'test_suite/test_config/{backend}/{command}.yaml'
    if not os.path.exists(config_path):
        print(f'test config {config_path} not found ')
        return

    with open(config_path, 'r') as config_file:
        config = yaml.safe_load(config_file)

    for test in config['tests']:
        script_content += f"echo '################################################################################'\n"
        script_content += f"echo '# {test['name']}'\n"
        script_content += f"{test['cmd']}"

        script_content += '\n\n'

    script_name = f'test_{command}.sh'
    scripts_folder = os.path.join(backend_folder, 'scripts')
    os.makedirs(scripts_folder, exist_ok=True)
    script_path = os.path.join(scripts_folder, script_name)

    if os.path.exists(script_path):
        os.remove(script_path)
    with open(script_path, 'w') as script_file:
        script_file.write(script_content)


def generate_folder_structure():
    backends = ['nnabla', 'torch', 'common']
    base_folder = 'test_suite'
    run_all_tests_content = '''#!/bin/bash

echo '################################################################################'
echo '# run all test'
        '''

    for backend in backends:
        commands_info = {
            'train': ['nnabla', 'torch'],
            'infer': ['nnabla', 'torch'],
            'forward': ['nnabla', 'torch'],
            'encode_param': ['nnabla'],
            'decode_param': ['nnabla'],
            'profile': ['nnabla', 'torch'],
            'conv_dataset': ['nnabla'],
            'compare_with_cpu': ['nnabla', 'torch'],
            'create_image_classification_dataset': ['nnabla'],
            'upload': ['nnabla', 'torch'],
            'create_tar': ['nnabla', 'torch'],
            'function_info': ['nnabla'],
            'optimize': ['common'],
            'dump': ['nnabla', 'torch'],
            'nnb_template': ['nnabla'],
            'convert': ['nnabla'],
            'plot_series': ['common'],
            'plot_timer': ['common'],
            'draw_graph': ['nnabla'],
            'version': ['common'],
        }

        for command, supported_backends in commands_info.items():
            if backend in supported_backends:
                generate_test_script(backend, command)
                run_all_tests_content += f'''
cd "$(dirname "$0")/{backend}/test_{command}"
bash test_{command}.sh
'''

    run_all_tests_path = os.path.join(base_folder, 'run_all_tests.sh')
    with open(run_all_tests_path, 'w') as run_all_tests_file:
        run_all_tests_file.write(run_all_tests_content)


if __name__ == "__main__":
    generate_folder_structure()
