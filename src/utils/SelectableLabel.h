#pragma once

#include <QLabel>
#include <Qt>

/// 使 QLabel / ElaText 等可用鼠标与键盘选中文本以便复制。
inline void applySelectableLabelText(QLabel* label) {
    if (!label) {
        return;
    }
    label->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
    label->setFocusPolicy(Qt::ClickFocus);
}
