#pragma once
#include "ElaIconButton.h"
#include <QMouseEvent>
#include <QPoint>

// 带图标的按钮
class DragButton : public ElaIconButton {
    Q_OBJECT
public:
    // clang-format off
    explicit DragButton(ElaIconType::IconName awesome, QString toolType, QWidget* parent = nullptr);
    explicit DragButton(ElaIconType::IconName awesome, int pixelSize, QString toolType, QWidget* parent = nullptr);
    // clang-format on

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;

private:
    QPoint m_startPos;  // 鼠标按下起点
    QString m_toolType; // 工具类型，用于拖拽 MIME 数据
};
