#include "ShowButton.h"
#include <QMouseEvent>

ShowButton::ShowButton(ElaIconType::IconName awesome, QString toolType, QWidget* parent)
    : ElaIconButton(awesome, parent)
    , m_toolType(toolType) {
    setCursor(Qt::PointingHandCursor); // 鼠标悬停显示手型
}

ShowButton::ShowButton(ElaIconType::IconName awesome,
                       int pixelSize,
                       QString toolType,
                       QWidget* parent)
    : ElaIconButton(awesome, pixelSize, parent)
    , m_toolType(toolType) {
    setCursor(Qt::PointingHandCursor);
}

void ShowButton::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        // 切换显示状态
        m_visible = !m_visible;

        // 发射信号通知外部
        emit toggled(m_visible);

        // 可选：改变按钮样式，比如高亮或图标变化
        setChecked(m_visible);
    }

    // 调用父类处理（保留按钮本身功能）
    ElaIconButton::mousePressEvent(event);
}
