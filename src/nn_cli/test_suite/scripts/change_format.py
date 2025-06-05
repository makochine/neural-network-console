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

import argparse
from pathlib import Path
from PIL import Image

def convert(args):
    srcdir = Path(args.srcdir)
    for dir in srcdir.glob('**'):
        num_converted = 0
        for file in dir.glob('*'):
            if file.is_dir():
                break

            image = Image.open(file)
            outdir = Path(args.outdir).joinpath(file.relative_to(srcdir).parent)
            outdir.mkdir(parents=True, exist_ok=True)
            outfile = Path(outdir).joinpath(file.with_suffix(f'.{args.type}').name)
            image.save(outfile)

            num_converted += 1
            if args.maxnum != 0 and args.maxnum <= num_converted:
                break


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--srcdir', type=str, help='image directory', required=True)
    parser.add_argument('--outdir', type=str, help='output directory', required=True)
    parser.add_argument('--type', type=str,   help='image format', choices=['png', 'gif', 'jpg'], default='gif')
    parser.add_argument('--maxnum', type=int, help='number of converted files', default=10)
    args = parser.parse_args()
    
    convert(args)
