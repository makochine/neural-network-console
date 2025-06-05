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

import configparser
import copy
import importlib
import io
import multiprocessing
import os
import sys
import threading

from nnabla import logger


def get_help(plugin):
    plugin_path = './libs/plugins/' + plugin
    plugin_dir = os.path.dirname(os.path.abspath(plugin_path))

    if plugin_dir not in sys.path:
        sys.path.insert(0, plugin_dir)

    plugin_filename = os.path.basename(plugin)
    plugin_name, _ = os.path.splitext(plugin_filename)
    logger.critical(f'Getting help message of {plugin_name}')
    try:
        plugin_module = importlib.import_module(plugin_name)
    except BaseException:
        logger.critical(f'Error getting help message from {plugin_name}')
        logger.critical(f' to check what is wrong, please run `python3 {plugin} -h` directly.')
        return []

    sys.argv = [plugin_name, '-h']
    sys.stdout = mystdout = io.StringIO()
    t = threading.Thread(target=plugin_module.main)
    t.start()
    t.join()
    sys.stdout = sys.__stdout__
    help_message = mystdout.getvalue().splitlines()
    mystdout.close()

    return help_message


def parse_args(help):
    while help is not None and len(help) > 0:
        if help[0] == 'options:':
            help.pop(0)
            break
        help.pop(0)

    arg_lines = []
    for line in help:
        line = line.strip()
        if len(line) > 0:
            if line[0] == '-':
                arg_lines.append(line)
            elif len(arg_lines) > 0:
                arg_lines[-1] += ' ' + line

    args = []
    for line in arg_lines:
        arg_desc, arg_default = '', ''
        for i, s in enumerate(line.replace(',', ' ').split(' ')):
            if len(s) > 0:
                if s[0:2] == '--':
                    arg_name = s[2:]
                elif s[0] == '-':
                    arg_option = s[1:]
                elif arg_desc == s:
                    arg_desc = ''
                else:
                    if arg_desc != '':
                        arg_desc += ' '
                    arg_desc += s
        if 'default=' in arg_desc:
            arg_desc, arg_default = arg_desc.split('default=')
        if arg_option != 'h':
            arg = {
                'ArgOption': arg_option,
                'ArgName': arg_name,
                'ArgDesc': arg_desc,
                'ArgDefault': arg_default}
            args.append(arg)
    return args


def main():

    def update(section, help_message):
        logger.log(99, 'Updating... ' + section)
        plugin_file_name = './libs/plugins/' + section
        result = False
        if os.path.exists(plugin_file_name):
            ini[section]['DateModified'] = str(
                os.stat(plugin_file_name).st_mtime)
            ini[section]['NeedUpdate'] = '0'
            name = ''
            description = ''

            # name and description
            empty_line = False
            for line in help_message:
                if line == 'options:':
                    break
                elif line != '':
                    if not empty_line:
                        continue
                    if name == '':
                        name = line
                    else:
                        if description != '':
                            description += '\\n'
                        description += line
                else:
                    empty_line = True

            ini[section]['Name'] = name
            ini[section]['Description'] = description

            # args
            args = parse_args(help_message)
            ini[section]['NumArgs'] = str(len(args))
            for i, arg in enumerate(args):
                ini[section]['Arg{}_Option'.format(i)] = arg['ArgOption']
                ini[section]['Arg{}_Name'.format(i)] = arg['ArgName']
                ini[section]['Arg{}_Desc'.format(i)] = arg['ArgDesc']
                ini[section]['Arg{}_Default'.format(i)] = arg['ArgDefault']
            if name == '' or len(args) < 1:
                logger.critical(
                    f'Failed to update plugin "{section}" in libs/plugin.')
            else:
                result = True
        else:
            logger.critical(f'{section} not found in libs/plugin.')
        return result

    ini = configparser.ConfigParser()
    ini.optionxform = str

    config_file_name = './settings/plugins.ini'
    ini.read(config_file_name)
    success = True

    sections = []
    for plugin_name in ini.keys():
        if 'NeedUpdate' in ini[plugin_name] and ini.getint(plugin_name, 'NeedUpdate') > 0:
            sections.append(plugin_name)

    help_messages = []

    with multiprocessing.Pool(processes=10) as pool:
        help_messages = pool.map(get_help, sections)

    for section, help_message in zip(sections, help_messages):
        result = update(section, help_message)
        if not result:
            success = False

    with open(config_file_name, 'w') as ini_file:
        ini.write(ini_file, space_around_delimiters=False)
    if success:
        logger.log(99, 'Plugin update completed successfully.')


if __name__ == '__main__':
    main()
