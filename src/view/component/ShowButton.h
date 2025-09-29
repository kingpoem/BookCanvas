#pragma once
#include "ElaIconButton.h"
#include <QMouseEvent>
#include <QPoint>

// 显示/隐藏权重按钮
class ShowButton : public ElaIconButton {
    Q_OBJECT
public:
    // clang-format off
    explicit ShowButton(ElaIconType::IconName awesome, QString toolType, QWidget* parent = nullptr);
    explicit ShowButton(ElaIconType::IconName awesome, int pixelSize, QString toolType, QWidget* parent = nullptr);
    // clang-format on

signals:
    void toggled(bool visible); // 点击切换显示状态信息

protected:
    void mousePressEvent(QMouseEvent* event) override;

private:
    QString m_toolType;    // 工具类型，用于拖拽 MIME 数据
    bool m_visible = true; // 当前显示状态
};
