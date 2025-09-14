#include "DragButton.h"
#include <QApplication>
#include <QDrag>
#include <QMimeData>

DragButton::DragButton(ElaIconType::IconName awesome, const QString& toolType, QWidget* parent)
    : ElaIconButton(awesome, parent)
    , m_toolType(toolType) {
    setMouseTracking(true);
}

DragButton::DragButton(ElaIconType::IconName awesome,
                       int pixelSize,
                       const QString& toolType,
                       QWidget* parent)
    : ElaIconButton(awesome, pixelSize, parent)
    , m_toolType(toolType) {
    setMouseTracking(true);
}

void DragButton::mousePressEvent(QMouseEvent* event) {
    m_startPos = event->pos();
    ElaIconButton::mousePressEvent(event);
}

void DragButton::mouseMoveEvent(QMouseEvent* event) {
    if (!(event->buttons() & Qt::LeftButton))
        return;

    if ((event->pos() - m_startPos).manhattanLength() < QApplication::startDragDistance())
        return;

    // 创建拖拽对象
    QDrag* drag = new QDrag(this);
    QMimeData* mimeData = new QMimeData;

    // 设置拖拽数据为工具类型
    mimeData->setText(m_toolType);
    drag->setMimeData(mimeData);

    // 可选：拖拽显示按钮本身的 pixmap
    drag->setPixmap(this->grab());

    drag->exec(Qt::CopyAction);
}
