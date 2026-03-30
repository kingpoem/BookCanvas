#pragma once

#include <QString>

namespace BooksimPaths {

/// 查找 booksim 可执行文件（与运行时工作目录解析一致）。
[[nodiscard]] QString findBooksimExecutable();

/// BookSim 资源目录：优先为可执行文件所在目录；macOS 已安装 .app 则解析到源码树中的 `3rdpart/booksim2/src`（若存在）。
[[nodiscard]] QString booksimWorkingDirectory();

[[nodiscard]] QString defaultTopologyExportPath();
[[nodiscard]] QString defaultConfigExportPath();

/// 首次启动时写入 settings.ini 中的默认路径。
void ensureDefaultExportPathSettings();

[[nodiscard]] QString topologyExportPathFromSettings();
[[nodiscard]] QString configExportPathFromSettings();

/// 写入 JSON 的 `network_file`：与配置同目录时用文件名，否则用绝对路径，便于 booksim 在配置所在 cwd 下打开拓扑。
[[nodiscard]] QString networkFileFieldForJson(const QString& topologyFilePath,
                                              const QString& configFilePath);

} // namespace BooksimPaths
