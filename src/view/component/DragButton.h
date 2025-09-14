#pragma once
#include "ElaIconButton.h"
#include <QPoint>
#include <QMouseEvent>

class DragButton : public ElaIconButton {
    Q_OBJECT
public:
    explicit DragButton(ElaIconType::IconName awesome,
                        const QString& toolType,
                        QWidget* parent = nullptr);
    explicit DragButton(ElaIconType::IconName awesome,
                        int pixelSize,
                        const QString& toolType,
                        QWidget* parent = nullptr);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;

private:
    QPoint m_startPos;  // 鼠标按下起点
    QString m_toolType; // 工具类型，用于拖拽 MIME 数据
};
