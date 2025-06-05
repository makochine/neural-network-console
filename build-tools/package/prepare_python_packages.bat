@REM Copyright 2022,2023,2024,2025 Sony Group Corporation.
@REM
@REM Licensed under the Apache License, Version 2.0 (the "License");
@REM you may not use this file except in compliance with the License.
@REM You may obtain a copy of the License at
@REM
@REM     http://www.apache.org/licenses/LICENSE-2.0
@REM
@REM Unless required by applicable law or agreed to in writing, software
@REM distributed under the License is distributed on an "AS IS" BASIS,
@REM WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
@REM See the License for the specific language governing permissions and
@REM limitations under the License.

@ECHO OFF

REM Input variables
ECHO "[%0]"
ECHO "PYTHON_ENV=%PYTHON_ENV%"
ECHO "PYTHON_VER=%PYTHON_VER%"
ECHO "CPU_ONLY=%CPU_ONLY%"
ECHO "NNABLA_VER=%NNABLA_VER%"

SETLOCAL

REM --------------------------------------------------------------------------
REM Set PATH
REM --------------------------------------------------------------------------
PATH %PYTHON_ENV%;%PATH%
where python
python -m pip install -U pip
IF ERRORLEVEL 1 GOTO :error

python -m pip install wheel
IF ERRORLEVEL 1 GOTO :error

REM --------------------------------------------------------------------------
REM Install nnabla
REM --------------------------------------------------------------------------
python -m pip install nnabla==%NNABLA_VER% nnabla_converter==%NNABLA_VER%
IF ERRORLEVEL 1 GOTO :error
IF NOT DEFINED CPU_ONLY (
    ECHO "downloading nnabla_ext_cuda_alllib-%NNABLA_VER%-cp%PYTHON_VER:.=%-cp%PYTHON_VER:.=%-win_amd64.whl..."
    python -m pip install https://nnabla.org/whl/nnabla_ext_cuda_alllib/nnabla_ext_cuda_alllib-%NNABLA_VER%-cp%PYTHON_VER:.=%-cp%PYTHON_VER:.=%-win_amd64.whl
    IF ERRORLEVEL 1 GOTO :error
)

REM --------------------------------------------------------------------------
REM Install packages
REM --------------------------------------------------------------------------
python -m pip install --only-binary :all: ^
       asn1crypto ^
       awscli ^
       beautifulsoup4 ^
       decord ^
       fastapi ^
       ftfy ^
       gdcm ^
       icrawler ^
       janome ^
       matplotlib ^
       nltk ^
       norbert ^
       opencv-python ^
       paramiko ^
       pillow ^
       plotly ^
       psutil ^
       pycocotools==2.0.8 ^
       pydicom==2.4.4 ^
       pydub ^
       pysocks ^
       scikit-image ^
       "scikit-learn!=1.7.0" ^
       scp ^
       soundfile
IF ERRORLEVEL 1 GOTO :error

REM --------------------------------------------------------------------------
REM Install packages
REM --------------------------------------------------------------------------
python -m pip install ^
       clip ^
       addict ^
       flask ^
       flask-cors ^
       librosa ^
       openai ^
       python-pptx ^
       pythonnet ^
       python-certifi-win32 ^
       retrying ^
       transformers ^
       uvicorn ^
       wrapt==1.12.1 ^
       resampy ^
       av ^
       huggingface-hub ^
       pyarrow ^
       pandas
IF ERRORLEVEL 1 GOTO :error

REM --------------------------------------------------------------------------
REM install dlib
REM --------------------------------------------------------------------------
python -m pip install dlib==19.24.5
IF ERRORLEVEL 1 GOTO :error

REM --------------------------------------------------------------------------
REM remove __pycache__ directories
REM --------------------------------------------------------------------------
PUSHD %PYTHON_ENV%
FOR /R %%d IN (__pycache__) DO (
    if EXIST %%d (
       RMDIR /s /q %%d > NUL
    )
)
POPD

ENDLOCAL
EXIT /b


REM --------------------------------------------------------------------------
REM error
REM --------------------------------------------------------------------------
:error
ECHO FAILD.
ENDLOCAL
EXIT /b 1
