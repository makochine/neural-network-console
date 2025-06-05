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
ECHO "PYTHON_ENV=%PYTHON_ENV% (install directory)"
ECHO "PYTHON_VER=%PYTHON_VER% (python version)"

SETLOCAL

CALL pyenv.bat update
CALL pyenv.bat install --list |FINDSTR /r "%PYTHON_VER%.[0-9]*[0-9]$" >%~dp0__python_versions.txt
SET VER=
FOR /f %%l in (%~dp0__python_versions.txt) do (
    setlocal enabledelayedexpansion
    SET VER=%%l
)
DEL %~dp0__python_versions.txt

ECHO Latest version of Python%PYTHON_VER% == %VER%
CALL pyenv.bat install %VER%
CALL pyenv.bat uninstall nnc
CALL pyenv.bat duplicate %VER% nnc >NUL
CALL pyenv.bat local nnc
CALL pyenv.bat rehash

SET ENVPATH=
FOR /F "tokens=* USEBACKQ" %%I IN (`CALL pyenv.bat which python`) DO (
    SET ENVPATH=%%I
)
FOR %%i in (%ENVPATH%) DO SET ENVPATH=%%~dpi
ECHO ENVPATH: %ENVPATH%

IF EXIST %PYTHON_ENV% DEL /S /Q %PYTHON_ENV% >NUL
XCOPY %ENVPATH%\*.* %PYTHON_ENV%\ /E /Y /Q

ENDLOCAL
EXIT /b


REM --------------------------------------------------------------------------
REM error
REM --------------------------------------------------------------------------
:ERROR
ECHO FAILD.
ENDLOCAL
EXIT /b 1