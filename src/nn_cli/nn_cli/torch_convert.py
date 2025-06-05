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
import zipfile
import tempfile
import shutil

def convert_command(args):
    # Create a temporary directory
    with tempfile.TemporaryDirectory() as temp_dir:
        # Extract contents of nnpt file
        with zipfile.ZipFile(args.input, 'r') as zip_ref:
            zip_ref.extractall(temp_dir)

        # Find the extracted sdcproj file
        sdcproj_file = None
        for root, dirs, files in os.walk(temp_dir):
            for file in files:
                if file.endswith('.sdcproj'):
                    sdcproj_file = os.path.join(root, file)
                    break

        if not sdcproj_file:
            print("Error: No .sdcproj file found in the extracted content.")
            return

        # Move the sdcproj file to the final output location
        shutil.move(sdcproj_file, args.output)

        # Remove all other files
        for root, dirs, files in os.walk(temp_dir):
            for file in files:
                file_path = os.path.join(root, file)
                if file_path != args.output:
                    os.remove(file_path)

    return args.output


