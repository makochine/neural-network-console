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
from setuptools import setup, find_packages

if __name__ == '__main__':
    with open("README.md", "r", encoding="utf-8") as fh:
        long_description = fh.read()

    setup(
        name="nn_cli",
        version="0.0.1",
        author="DeveloperInSony",
        author_email="developer@sony.com",
        description="A command-line interface to wrapper basic operators of AI models",
        long_description=long_description,
        long_description_content_type="text/markdown",
        packages=find_packages(),
        package_data={'': ['nn_cli/*']},
        entry_points={"console_scripts":
                          ["nn_cli=nn_cli.cli:main"]},
        classifiers=[
            "Programming Language :: Python :: 3",
            "License :: OSI Approved :: Apache Software License",
            "Operating System :: OS Independent",
        ],
        python_requires='>=3.6',
    )

