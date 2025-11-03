#include "ShowButton.h"
#include <QMouseEvent>

ShowButton::ShowButton(ElaIconType::IconName awesome, QString toolType, QWidget* parent)
    : ElaIconButton(awesome, parent)
    , m_toolType(std::move(toolType)) {
    setCursor(Qt::PointingHandCursor); // 鼠标悬停显示手型
}

ShowButton::ShowButton(ElaIconType::IconName awesome,
                       int pixelSize,
                       QString toolType,
                       QWidget* parent)
    : ElaIconButton(awesome, pixelSize, parent)
    , m_toolType(std::move(toolType)) {
    setCursor(Qt::PointingHandCursor);
}

void ShowButton::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        // 切换显示状态
        m_visible = !m_visible;

        // 发射信号通知外部
        emit toggled(m_visible);

        setChecked(m_visible);
    }

    ElaIconButton::mousePressEvent(event);
}
