# BookCanvas
Design, customize, and simulate irregular NoC topologies with a drag-and-drop interface!

## Build

### Windows

部分先决条件:
- Qt = 6.7.0
- cmake >= 3.23
- msvc
- ninja
- windeployqt（注意对应 Qt 版本）

其他注意事项:
- 不考虑使用 Qt Creator
- 子模块版本固定，不需要更新

#### Qt 源码安装

1. 使用[Qt Online Installer for Windows](https://qt.io/download-qt-installer-oss)安装Qt源码，无需代理，需要注册 Qt 账号
2. 启动该安装程序时，为了解决后续安装网络问题，请从命令行启动该程序，并添加合适的镜像源，一个不行换另外一个
    - 可以选择的两个镜像源:
    - `—mirror https://mirrors.tuna.tsinghua.edu.cn/qt/`
    - `—mirror https://mirror.nju.edu.cn/qt`
3. 在`C`盘底下新建`Qt`目录，再在该目录下新建`qt6`目录，后续如果安装失败，将清除整个安装目录（如果选择`C:\Qt\qt6`，则将清除`qt6`目录，保留`Qt`目录） 
4. 选择自定义安装，取消勾选`Qt6.9`（写本文档时如此进行）
5. 在`自定义`步骤中，在右上角显示中勾选`Archive`项，下面的Qt版本才会有`6.7.0`
6. 如果你想更改自定义目录，可以更改顶层cmake文件：`set(CMAKE_PREFIX_PATH C:/Qt/qt6/6.7.0/msvc2019_64 CACHE PATH "QT6 DIR" FORCE)`，
或者在命令行构建时指定 `-DCMAKE_PREFIX_PATH=/to/your/path`（具有最高优先级）

> [!TIP]
> 打包时使用工具 windeployqt 路径为 `C:\Qt\qt6\6.7.0\msvc2019_64\bin\windeployqt.exe`
> 如果你此前安装过 Anaconda3，注意甄别使用的是否是其中的 windeployqt 工具
> 打包命令 `& "C:\Qt\qt6\6.7.0\msvc2019_64\bin\windeployqt.exe" --release .\BookCanvas.exe`

#### Msvc 安装

1. 使用[Visual Studio Installer](https://visualstudio.microsoft.com/zh-hans/downloads)安装 Visual Studio
2. 配置环境变量，在 `Path` 中添加 `C:\Program Files (x86)\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\<version>\bin\Hostx64\x64`，
版本号替换为自己的，视具体情况更改，在命令行中输入 `cl` 检验是否安装成功

从源码构建：

```shell
git clone --recurse-submodules git@github.com:kingpoem/BookCanvas.git
cmake -Bbuild
cmake --build build --config Release --parallel 4
```

### macOS

```shell
git clone --recurse-submodules git@github.com:kingpoem/BookCanvas.git
cmake -Bbuild -DQT_SDK_DIR=$(brew --prefix qt)
cmake --build build --config Release --parallel 4
```

### Archlinux

```shell
git clone --recurse-submodules git@github.com:kingpoem/BookCanvas.git
cmake -Bbuild
cmake --build build --config Release --parallel 4
```

