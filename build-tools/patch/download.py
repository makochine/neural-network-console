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


import os
import urllib
import urllib.request as request
from nnabla.logger import logger
from tqdm import tqdm


################################################################################
# for NNC. skip verify ssl certification.
import ssl
ctx = None
cert_check = os.getenv("NNABLA_SKIP_VERIFY_CERT", "").lower()
if cert_check in ["true", "yes", "ok"]:
    ctx = ssl.create_default_context()
    ctx.check_hostname = False
    ctx.verify_mode = ssl.CERT_NONE
else:
    try:
        import certifi
        ctx = ssl.create_default_context(cafile=certifi.where())
    except:
        pass
################################################################################


def get_data_home():
    d = os.path.expanduser('~/nnabla_data')
    if d == '/nnabla_data':
        d = './nnabla_data'
    try:
        os.makedirs(d)
    except OSError:
        pass  # python2 does not support exists_ok arg
    return d


def download(url, output_file=None, open_file=True, allow_overwrite=False):
    '''Download a file from URL.

    Args:
        url (str): URL.
        output_file (str, optional): If given, the downloaded file is written to the given path.
        open_file (bool): If True, it returns an opened file stream of the downloaded file.
        allow_overwrite (bool): If True, it overwrites an existing file.

    Returns:
        Returns file object if open_file is True, otherwise None.

    '''
    filename = url.split('/')[-1]
    if output_file is None:
        cache = os.path.join(get_data_home(), filename)
    else:
        cache = output_file
    if os.path.exists(cache) and not allow_overwrite:
        logger.info("> {} already exists.".format(cache))
        logger.info("> If you have any issue when using this file, ")
        logger.info("> manually remove the file and try download again.")
    else:
        try:
            r = request.urlopen(url, context=None)
        except urllib.error.URLError as e:
            if ctx is not None and hasattr(e, 'reason') and isinstance(e.reason, ssl.SSLError):
                if 'CERTIFICATE_VERIFY_FAILED' in str(e.reason):
                    r = request.urlopen(url, context=ctx)
                else:
                    raise
            else:
                raise
        try:
            content_length = int(r.headers.get('Content-Length'))
        except:
            content_length = 0
        unit = 1000000
        content = b''
        with tqdm(total=content_length, desc=filename, unit='B', unit_scale=True, unit_divisor=1024) as t:
            while True:
                data = r.read(unit)
                l = len(data)
                t.update(l)
                if l == 0:
                    break
                content += data
        r.close()
        with open(cache, 'wb') as f:
            f.write(content)
    if not open_file:
        return
    return open(cache, 'rb')
