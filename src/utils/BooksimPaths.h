#pragma once

#include <QString>

namespace BooksimPaths {

/// 查找 booksim 可执行文件：优先应用安装目录（BookCanvas.exe 同目录）中的 booksim，
/// 其次使用设置项 `booksimEnginePath`（存在且为文件时），最后自动检测。
[[nodiscard]] QString findBooksimExecutable();

/// 不经设置的自动探测路径（应用目录、源码树 `3rdpart/booksim2/src` 等），与默认引擎路径一致。
[[nodiscard]] QString defaultBooksimEnginePath();

/// BookSim 资源目录：优先为可执行文件所在目录；macOS 已安装 .app 则解析到源码树中的 `3rdpart/booksim2/src`（若存在）。
[[nodiscard]] QString booksimWorkingDirectory();

/// 默认 anynet 导出路径：统一落在用户可写数据目录（如 Windows 的 AppData），避免安装目录不可写导致失败。
[[nodiscard]] QString defaultTopologyExportPath();
/// 默认 JSON 导出路径：统一落在用户可写数据目录（如 Windows 的 AppData），避免安装目录不可写导致失败。
[[nodiscard]] QString defaultConfigExportPath();

/// 首次启动时写入 settings.ini：默认引擎路径为 `defaultBooksimEnginePath()`（若设置缺失或为空则写入）。
void ensureDefaultEnginePathSettings();

/// 首次启动时写入 settings.ini 中的默认导出路径。
void ensureDefaultExportPathSettings();

[[nodiscard]] QString topologyExportPathFromSettings();
[[nodiscard]] QString configExportPathFromSettings();

/// 基于设置中的导出路径，拼接隔离标识（如 tab_1）形成独立文件名。
[[nodiscard]] QString scopedExportPath(const QString& basePath, const QString& scopeToken);

/// 写入 JSON 的 `network_file`：与配置同目录时用文件名，否则用绝对路径，便于 booksim 在配置所在 cwd 下打开拓扑。
[[nodiscard]] QString networkFileFieldForJson(const QString& topologyFilePath,
                                              const QString& configFilePath);

} // namespace BooksimPaths
