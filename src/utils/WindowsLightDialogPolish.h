#pragma once

class QWidget;

namespace BookCanvasUi {

/// 在 Windows 上为顶层对话框修正浅色主题下的系统深色边框/调色板问题（白底黑字）。
/// 深色主题下不修改样式；非 Windows 为空操作。
void installWindowsLightTopLevelDialogPolish(QWidget* topLevel);

} // namespace BookCanvasUi
