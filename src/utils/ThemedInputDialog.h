#pragma once

#include <QString>

class QWidget;

namespace BookCanvasUi {

/// 与 Ela 主题一致的行文本输入框（用于替代 QInputDialog::getText，避免原生对话框在浅色主题下仍为深色）。
[[nodiscard]] QString promptLineText(QWidget* parent,
                                     const QString& windowTitle,
                                     const QString& labelText,
                                     const QString& defaultText,
                                     bool* ok);

/// 与 Ela 主题一致的提示框（替代 QMessageBox，避免浅色/深色下原生样式不一致）
void alertInformation(QWidget* parent, const QString& title, const QString& text);
void alertWarning(QWidget* parent, const QString& title, const QString& text);

/// 只读多行预览（与 Ela 主题一致），用于 JSON / 拓扑等文本查看。
/// @param absoluteExportPath 非空时在正文上方展示该绝对路径（可选中复制）；为空则不展示路径区。
void showReadOnlyTextPreview(QWidget* parent,
                             const QString& windowTitle,
                             const QString& bodyText,
                             const QString& absoluteExportPath = {});

} // namespace BookCanvasUi
