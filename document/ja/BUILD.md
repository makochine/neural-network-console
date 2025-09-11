# ビルド手順

## 概要

このドキュメントでは、Windows（コマンドプロンプト）、WSL（Linux）、Chocolatey、
およびEmbarcadero C++ Builderを使用して、Neural Network Consoleをビルドおよびパッケージ化する方法について説明します。

## 要件

### Windows側:

ソフトウェアのビルドとパッケージ化には、以下のソフトウェアが必要です：
- Embarcadero C++Builder
- Chocolatey

C++Builderをインストール後、ツールメニューの「GetIt Package Manager」から以下のパッケージをインストールしてください。
- boost 1.70 for win64 Toolchain
- CMake 3.30

`cmake`コマンドを使用可能にするために、
`cmake.exe`が配置されているディレクトリをPATH環境変数に追加する必要があります。
以下は例です：

```cmd
SET PATH=%PATH%;%USERPROFILE%\Documents\Embarcadero\Studio\23.0\CatalogRepository\CMake-3.30\bin
```

Chocolateyをインストール後、以下のパッケージをインストールしてください：
- 7zip
- ffmpeg
- pyenv-win

### WSL側（Ubuntu推奨）

ソフトウェアのビルドには、以下のパッケージが必要です：
- make
- docker
- Python 3.x

## ソフトウェアのビルド

### ステップ1: リポジトリのクローン

まず、以下のリポジトリをクローンまたはダウンロードしてください：
- sony/neural_network_console
- sony/nnabla-examples
- sony/nnc-plugin
- sony/nnabla-nas (release/console_v2.5)

```cmd copy
git clone https://github.com/sony/neural_network_console
cd neural_network_console
git clone https://github.com/sony/nnabla-examples
git clone https://github.com/sony/nnc-plugin
git clone -b release/console_v2.5 https://github.com/sony/nnabla-nas
```

### ステップ2: ドキュメントのビルド

WSL環境でドキュメントをビルドします。

コマンドを実行する前に、システム上でDockerが起動していることを確認してください。

```bash
make doc
```

既に一度`make doc`を実行済みで失敗する場合は、以下のコマンドを先に実行して前回のビルドをクリーンアップしてください：

```bash
make clean-doc
make doc
```

### ステップ3: ユーティリティとアプリのビルド

コマンドプロンプトでユーティリティとneural network consoleをビルドします。

```cmd
build-tools\build-cli-util.bat
build-tools\build-neural-network-console.bat
```

### ステップ4: ソフトウェアのパッケージ化

最後のステップはパッケージ化で、多数のPythonパッケージをダウンロードするため
数十分かかります。

```cmd
build-tools\package.bat
```

`./build`ディレクトリ下にパッケージの7zファイルが作成されます。 
`./build`ディレクトリ下にパッケージの7zファイルが作成されます。 