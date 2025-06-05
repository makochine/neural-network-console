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
import cv2
import numpy as np
from pathlib import Path


def image_convert(args, image):
    if len(image.shape) == 2:
        if args.plane == 1:  # mono -> mono
            pass
        else:                # mono -> RGB
            image = cv2.cvtColor(image, cv2.COLOR_GRAY2RGB)
    elif image.shape[-1] == 3:
        if args.plane == 1:  # BGR -> mono
            image = cv2.cvtColor(image, cv2.COLOR_BGR2GRAY)
        else:                # BGR -> RGB
            image = cv2.cvtColor(image, cv2.COLOR_BGR2RGB)
    elif image.shape[-1] == 4:
        if args.plane == 1:  # BGRA -> mono
            image = cv2.cvtColor(image, cv2.COLOR_BGRA2GRAY)
        else:                # BGRA -> RGB
            image = cv2.cvtColor(image, cv2.COLOR_BGRA2RGB)

    if args.normalize:
        if image.dtype == np.uint8:
            image = (image / 255.0).astype(np.float32)
        elif image.dtype == np.uint16:
            image = (image / 65535.0).astype(np.float32)
        else:
            Exception(f'Unknown image.dtype: {image.dtype}')

    return image


def convert_with_walkthrough(args, convert_func):
    srcdir = Path(args.srcdir)
    for dir in srcdir.glob('**'):
        num_converted = 0
        for file in dir.glob('*'):
            if file.is_dir():
                break

            image = cv2.imread(file, cv2.IMREAD_UNCHANGED)
            image = convert_func(args, image)

            outdir = Path(args.outdir).joinpath(file.relative_to(srcdir).parent)
            outdir.mkdir(parents=True, exist_ok=True)
            outfile = Path(outdir).joinpath(file.with_suffix('.bin').name)
            image.tofile(outfile)

            num_converted += 1
            if args.maxnum != 0 and args.maxnum <= num_converted:
                break


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--srcdir', type=str, help='source image directory', required=True)
    parser.add_argument('--outdir', type=str, help='output RGBplane directory', default='out')
    parser.add_argument('--plane', type=int, help='number of planes', choices=(1,3), default=1)
    parser.add_argument('--normalize', type=bool, help='image normalization', default=True)
    parser.add_argument('--maxnum', type=int, help='number of generate files for each directory', default=0)
    args = parser.parse_args()

    convert_with_walkthrough(args, image_convert)
