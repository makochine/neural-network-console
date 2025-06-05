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
ECHO "NNCDIR=%NNCDIR%"
ECHO "CPU_ONLY=%CPU_ONLY%"

SETLOCAL

FOR %%i IN (%NNCDIR%) DO SET DIRNAME=%%~dpi
FOR %%i IN (%NNCDIR%) DO SET BASENAME=%%~nxi

PUSHD %DIRNAME%
IF DEFINED CPU_ONLY (
    REN %BASENAME% %BASENAME%_cpu
    "c:\Program Files\7-Zip\7z.exe" a -mx9 %BASENAME%_cpu.7z %BASENAME%_cpu
    IF ERRORLEVEL 1 GOTO :error
) ELSE (
    "c:\Program Files\7-Zip\7z.exe" a -mx9 %BASENAME%.7z %BASENAME%
    IF ERRORLEVEL 1 GOTO :error
)
POPD

ENDLOCAL
exit /b


REM --------------------------------------------------------------------------
REM error
REM --------------------------------------------------------------------------
:error
ECHO FAILD.
ENDLOCAL
EXIT /b 1
