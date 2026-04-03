# BookCanvas
Design, customize, and simulate irregular NoC topologies with a drag-and-drop interface!

## 文档导航

- 快速上手与页面流程：[`docs/USAGE.md`](docs/USAGE.md)
- 构建与安装（多平台）：见本文档下方 `Build` / `Install`
- 应用内参数与指标解释：侧栏页面 `使用说明`

## 快速开始（推荐）

如果你只想先跑通一轮仿真，按下面顺序执行：

1. 按平台完成构建（Windows/macOS/Archlinux 见下文 `Build`）
2. 启动程序后进入 `Setting`，确认：
   - `拓扑文件（Canvas「导出拓扑」）` 路径
   - `JSON 配置（Canvas「导出配置」与仿真）` 路径
3. 在 `Canvas` 放置节点或拓扑块，检查参数
4. 在 `全局配置` 补齐关键参数（如 `topology`、`routing_function`、`traffic`、`injection_rate`）
5. 在 `Simulation` 点击 `执行仿真`
6. 在 `BookSim 结果` 查看 KPI 和延迟/吞吐结果

详细流程、快捷键和排障建议见：[`docs/USAGE.md`](docs/USAGE.md)

## Build

### Windows

部分先决条件:
- Qt = 6.7.0
- cmake >= 3.23
- msvc
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
版本号替换为自己的，视具体情况更改，在 `powershell` 中输入 `cl` 检验是否安装成功
3. 不使用`-G "Ninja"`，目前 Ninja 构建有问题，请指定`-G "Visual Studio 17 2022"`

从源码构建：

```shell
git clone --recurse-submodules --depth 1 --shallow-submodules git@github.com:kingpoem/BookCanvas.git
make clean
cmake -Bbuild -G "Visual Studio 17 2022" -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel 4
cmake --build build --target booksim2 --config Release
& "C:\Qt\qt6\6.7.0\msvc2019_64\bin\windeployqt.exe" --release .\build\Release\BookCanvas.exe
.\build\Release\BookCanvas.exe
```

### macOS

```shell
git clone --recurse-submodules git@github.com:kingpoem/BookCanvas.git
cmake -B build -DQT_SDK_DIR=$(brew --prefix qt)
cmake --build build --config Release --parallel 4
```

仅需安装到「应用程序」时，见下文 **[Install](#install)** 中 macOS 一节执行 **`sudo make install`**（默认前缀 **`/Applications`**）。

### Archlinux

```shell
git clone --recurse-submodules git@github.com:kingpoem/BookCanvas.git
cmake -Bbuild
cmake --build build --config Release --parallel 4
```

## Install

与 **GNU Make** 相关的命令在仓库根目录执行；Windows 上一般在 **Git Bash / MSYS2** 等提供 `make` 的环境中使用。

说明:

- **`make install`** 等价于对 **`build/`** 执行不带 `--prefix` 的 **`cmake --install`**，文件会安装到**配置工程时**写入 `CMakeCache.txt` 的 **`CMAKE_INSTALL_PREFIX`**。未在 `cmake -B` 阶段指定该变量时，**macOS** 上本仓库默认为 **`/Applications`**（生成 **`BookCanvas.app`**）；其他 Unix 一般为 **`/usr/local`**；Windows 上为 CMake 默认路径（多为 **`Program Files` 下的应用程序目录**），具体以 `build/CMakeCache.txt` 为准。
- **仓库下的 `Install/`** 不再由子模块或 Make 强行指定；仅当你在配置时**手动**传入例如 **`-DCMAKE_INSTALL_PREFIX="$PWD/Install"`**（路径请按本体与平台写绝对路径或你认可的相对解析结果）时，**`make install`** 才会把内容装进该目录。**`make clean`** 仍会删除本地的 **`Install/`** 目录，用于清理这类「固定落在源码树里」的安装前缀。
- **单次覆盖安装路径**（不改编译缓存中的 `CMAKE_INSTALL_PREFIX`）仍可使用：`cmake --install build ... --prefix <path>`（Windows 多配置需 **`--config Release`**），与 **`make install`** 行为不同，除非你明确传入与缓存一致的前缀。
- **`make uninstall`** 根据 **`build/install_manifest*.txt`** 删除**最近一次** **`cmake --install` / `make install`** 实际写入的文件；**若需卸载，请先 `make uninstall` 再 `make clean`**，否则会删掉清单，无法再按列表反安装。清单若指向系统目录，**`make uninstall` 会 `rm -rf` 相应路径**，务必确认最近一次安装前缀。

### Windows

在 [上文](#windows) 完成首次 **`cmake -B`** 时即可约定安装位置，例如装到系统区域（示例路径）：

```shell
cmake -B build -G "Visual Studio 17 2022" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="C:/Program Files/BookCanvas"
cmake --build build --config Release --parallel 4
make install
```

若希望安装到仓库下 **`Install/`**，仅在配置阶段指定前缀（Git Bash 下示例）：

```shell
cmake -B build -G "Visual Studio 17 2022" -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX="$(pwd)/Install"
cmake --build build --config Release --parallel 4
make install
```

本次安装不改编译缓存、直接指定前缀：

```shell
cmake --install build --config Release --prefix "C:/自定义路径"
```

卸载（删除最近一次安装对应的文件）：

```shell
make uninstall
```

### macOS

未指定前缀时，顶层 **`CMakeLists.txt`** 会把 **`CMAKE_INSTALL_PREFIX`** 设为 **`/Applications`**，安装结果为 **`/Applications/BookCanvas.app`**。若 **`build/`** 很早以前已生成且缓存里仍是其他前缀，请删除 **`build`** 后重新 **`cmake -B ...`**，或在配置时显式传入 **`-DCMAKE_INSTALL_PREFIX=/Applications`**。因系统目录需写权限，一般使用：

```shell
cmake -B build -DQT_SDK_DIR=$(brew --prefix qt)
cmake --build build --config Release --parallel 4
sudo make install
```

若希望安装到仓库下 **`Install/`** 或其他路径，在配置阶段显式指定 **`CMAKE_INSTALL_PREFIX`**：

```shell
cmake -B build -DQT_SDK_DIR=$(brew --prefix qt) -DCMAKE_INSTALL_PREFIX="$PWD/Install"
cmake --build build --config Release --parallel 4
make install
```

仍可通过 **`cmake --install`** 单次覆盖前缀（示例，按需加 **`sudo`**）：

```shell
sudo cmake --install build --prefix /Applications
```

卸载：

```shell
sudo make uninstall
```

### Archlinux

配置时指定（或省略前缀以使用 CMake 默认，一般为 **`/usr/local`**）：

```shell
cmake -B build -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build build --config Release --parallel "$(nproc)"
make install
```

安装到仓库下 **`Install/`**：

```shell
cmake -B build -DCMAKE_INSTALL_PREFIX="$PWD/Install"
cmake --build build --config Release --parallel "$(nproc)"
make install
```

单次覆盖前缀：

```shell
sudo cmake --install build --prefix /usr/local
```

卸载：

```shell
make uninstall
```

