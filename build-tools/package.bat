@REM Copyright 2020,2021 Sony Corporation.
@REM Copyright 2021,2022,2023,2024,2025 Sony Group Corporation.
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

IF NOT DEFINED NNCDIR     SET NNCDIR=build\neural_network_console
IF NOT DEFINED PYTHON_VER SET PYTHON_VER=3.10
IF NOT DEFINED PYTHON_ENV SET PYTHON_ENV=%NNCDIR%\libs\Python
IF NOT DEFINED NNABLA_VER SET NNABLA_VER=1.39.0

ECHO "Creating package:"
ECHO "  CPU_ONLY=%CPU_ONLY%"
ECHO "  NNCDIR=%NNCDIR%"
ECHO "  PYTHON_VER=%PYTHON_VER%"
ECHO "  NNABLA_VER=%NNABLA_VER%"

CALL %~dp0package\prepare_python_environment.bat
CALL %~dp0package\prepare_python_packages.bat
CALL %~dp0package\collect.bat
IF NOT DEFINED SKIP_ARCHIVE (
    CALL %~dp0package\archive.bat
)