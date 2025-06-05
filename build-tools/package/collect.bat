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

SETLOCAL

REM Input variables
ECHO "[%0]"
ECHO "NNCDIR=%NNCDIR%"


XCOPY "nnc-plugin\samples\xai\*.*" %NNCDIR%\samples\sample_project\xai\ /E /Y /Q

XCOPY "nnc-plugin\plugins\*.*" %NNCDIR%\libs\plugins\ /E /Y /Q
XCOPY "libs\*.*" %NNCDIR%\libs\ /E /Y /Q

XCOPY "settings\*.*" %NNCDIR%\settings\ /E /Y /Q

XCOPY "samples\sample_dataset\*.*" "%NNCDIR%\samples\sample_dataset\" /E /Y /Q
XCOPY "samples\sample_project\*.*" "%NNCDIR%\samples\sample_project\" /E /Y /Q

REM NNABLA REMOTE
XCOPY "src\nnabla-remote\nnabla_ssh\nnabla_ssh" %NNCDIR%\libs\Python\Lib\site-packages\nnabla_ssh\ /E /Y /Q

REM NNABLA NAS
XCOPY "nnabla-nas\nnabla_nas" %NNCDIR%\libs\Python\Lib\site-packages\nnabla_nas\ /E /Y /Q

REM NN CLI
XCOPY "src\nn_cli\nn_cli" %NNCDIR%\libs\Python\Lib\site-packages\nn_cli\ /E /Y /Q

COPY /Y %NNCDIR%\libs\Python\Lib\site-packages\sklearn\.libs\*.dll %NNCDIR%\libs\Python

REM install examples
MKDIR %NNCDIR%\libs\bin
COPY C:\ProgramData\chocolatey\lib\ffmpeg\tools\ffmpeg\bin\ffmpeg.exe %NNCDIR%\libs\bin\
COPY C:\ProgramData\chocolatey\lib\ffmpeg\tools\ffmpeg\bin\ffprobe.exe %NNCDIR%\libs\bin\
XCOPY ai-research-code\d3net %NNCDIR%\libs\plugins\_NNablaExamples\ai-research-code\d3net\ /E /Y /Q
XCOPY ai-research-code\x-umx %NNCDIR%\libs\plugins\_NNablaExamples\ai-research-code\x-umx\ /E /Y /Q
XCOPY nnabla-examples\utils %NNCDIR%\libs\plugins\_NNablaExamples\nnabla-examples\utils\ /E /Y /Q
XCOPY nnabla-examples\image-generation %NNCDIR%\libs\plugins\_NNablaExamples\nnabla-examples\image-generation\ /E /Y /Q
XCOPY nnabla-examples\image-superresolution %NNCDIR%\libs\plugins\_NNablaExamples\nnabla-examples\image-superresolution\ /E /Y /Q
XCOPY nnabla-examples\object-detection %NNCDIR%\libs\plugins\_NNablaExamples\nnabla-examples\object-detection\ /E /Y /Q
XCOPY nnabla-examples\facial-keypoint-detection %NNCDIR%\libs\plugins\_NNablaExamples\nnabla-examples\facial-keypoint-detection\ /E /Y /Q
XCOPY nnabla-examples\vision-and-language %NNCDIR%\libs\plugins\_NNablaExamples\nnabla-examples\vision_and_language\ /E /Y /Q

REM attention editor
REM SET DEST=%NNCDIR%\libs\plugins\_Post_Process\_XAI\_Image/attention_editor_binary
REM ROBOCOPY nnc-plugin/tools/AttentionEditor/dist_electron/win-unpacked %DEST% /MIR /PURGE
REM ATTRIB -R %DEST%

REM Patches
COPY build-tools\patch\download.py %NNCDIR%\libs\Python\Lib\site-packages\nnabla\utils\

REM Other setup
XCOPY "default_settings\*.*" %NNCDIR%\settings\ /E /Y /D /Q
COPY default_settings\nnabla.conf %NNCDIR%\libs\Python\Lib\site-packages\nnabla\
COPY doc_files\*.* %NNCDIR%\
COPY neural_network_console.exe %NNCDIR%\neural_network_console.exe

REM cleanup
DEL /S /Q %NNCDIR%\libs\Python\*.pyc > NUL
DEL /S /Q %NNCDIR%\libs\Python\*.h > NUL
DEL /S /Q %NNCDIR%\libs\Python\Scripts > NUL
RMDIR /S /Q %NNCDIR%\libs\Python\Scripts > NUL

ENDLOCAL
