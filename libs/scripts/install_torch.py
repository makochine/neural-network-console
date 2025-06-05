import sys
import subprocess
from nnabla import logger
from nnabla.ext_utils import list_extensions


def main():
    logger.critical('Installing torch...')

    packages = [
        'torch==2.5.1',
        'torchvision==0.20.1',
        'torchaudio==2.5.1'
    ]
    if 'cudnn' in list_extensions():
        index_url = ['--index-url', 'https://download.pytorch.org/whl/cu121']
    else:
        index_url = ['--index-url', 'https://download.pytorch.org/whl/cpu']

    try:
        subprocess.check_call([
            sys.executable,
            '-m',
            'pip',
            'install',
            *packages,
            *index_url
        ])
        with open('./settings/install_pkg_list.txt', 'w') as f:
            f.write('\n'.join(packages))
        logger.log(99, 'torch is installed successfully.')
    except subprocess.CalledProcessError as e:
        logger.critical('Error occurred while installing torch.')
        logger.critical(
            'Please run `python libs/scripts/install_torch.py` directly.')
        logger.log(99, e)


if __name__ == '__main__':
    main()
