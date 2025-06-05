# Copyright 2020,2021 Sony Corporation.
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

import awscli.clidriver
import io
import json
import sys


class capture(list):
    def __enter__(self):
        self._stdout = sys.stdout
        sys.stdout = self._stringio = io.StringIO()
        return self

    def __exit__(self, *args):
        self.extend(self._stringio.getvalue().splitlines())
        del self._stringio    # free up some memory
        sys.stdout = self._stdout


def check_awscli_output(args, raw_output=False):
    driver = awscli.clidriver.create_clidriver()
    with capture() as output:
        rc = driver.main(args)
    awscli.clidriver.HISTORY_RECORDER.record('CLI_RC', rc, 'CLI')
    if rc != 0:
        return None
    if raw_output:
        return output
    return json.loads('\n'.join(output))


def run_awscli(args):
    driver = awscli.clidriver.create_clidriver()
    rc = driver.main(args)
    awscli.clidriver.HISTORY_RECORDER.record('CLI_RC', rc, 'CLI')
    return rc
