# Build Instructions

## Overview

This document describes how to build and package Neural Network Console
using Windows (with Command Prompt), WSL (Linux), Chocolatey,
and Embarcadero C++ Builder.

## Requirements

### Windows side:

You need following software for building and packaging software:
- Embarcadero C++Builder
- Chocolatey

After installing C++Builder, please install the following packages
from the "GetIt Package Manager" in Tools menu.
- boost 1.70 for win64 Toolchain
- CMake 3.30

In order to make the `cmake` command available,
the directory where `cmake.exe` is located must be added to the PATH environment variable.
Here is an example:

```cmd
SET PATH=%PATH%;%USERPROFILE%\Documents\Embarcadero\Studio\23.0\CatalogRepository\CMake-3.30\bin
```

After installing Chocolatey, please install the following packages:
- 7zip
- ffmpeg
- pyenv-win

### WSL-side (Ubuntu recommended)

You need following packages for building software:
- make
- docker
- Python 3.x

## Building the software

### Step 1: Clone repositories

First, please clone or download the following repositories:
- sony/neural_network_console
- sony/ai-research-code
- sony/nnabla-examples
- sony/nnc-plugin
- sony/nnabla-nas (release/console_v2.5)

```cmd copy
git clone https://github.com/sony/neural_network_console
cd neural_network_console
git clone https://github.com/sony/ai-research-code
git clone https://github.com/sony/nnabla-examples
git clone https://github.com/sony/nnc-plugin
git clone -b release/console_v2.5 https://github.com/sony/nnabla-nas
```

### Step 2: Build documents

Build documents on the WSL environment.

Before running the command, make sure Docker is running on your system.

```bash
make doc
```

If you have already run `make doc` once and it fails, run the following command first to clean the previous build:

```bash
make clean-doc
make doc
```

### Step 3: Build utilities and application

Build utils and neural network console on the Command Prompt.

```cmd
build-tools\build-cli-util.bat
build-tools\build-neural-network-console.bat
```

### Step 4: Package the software

The last step is packaging, which takes several tens of minutes
to download many python packages.

```cmd
build-tools\package.bat
```

There should be a package 7z file under the `./build` directory. 
There should be a package 7z file under the `./build` directory. 