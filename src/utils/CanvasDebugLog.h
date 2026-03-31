#pragma once

#include <QString>

/// 将调试信息追加写入本地日志文件（带时间戳，每次写入后 flush，尽量在崩溃前落盘）。
void canvasDebugLog(const QString& message);

/// 日志路径：应用程序目录下的 logs/bookcanvas_canvas_debug.log
[[nodiscard]] QString canvasDebugLogFilePath();
