#include "DragButton.h"
#include <QApplication>
#include <QDrag>
#include <QMimeData>

DragButton::DragButton(ElaIconType::IconName awesome, QString toolType, QWidget* parent)
    : ElaIconButton(awesome, parent)
    , m_toolType(std::move(toolType)) {
    setMouseTracking(true); // 即使鼠标没按键，也能跟踪移动
}

// clang-format off
DragButton::DragButton(ElaIconType::IconName awesome, int pixelSize, QString toolType, QWidget* parent)
    : ElaIconButton(awesome, pixelSize, parent)
    , m_toolType(std::move(toolType)) {
    setMouseTracking(true);
}
// clang-format on

void DragButton::mousePressEvent(QMouseEvent* event) {
    ElaIconButton::mousePressEvent(event); // 调用基类
}

void DragButton::mouseMoveEvent(QMouseEvent* event) {
    // 检查左键是否按住
    if (!(event->buttons() & Qt::LeftButton)) {
        qDebug() << event->buttons() << Qt::LeftButton;
        return;
    }

    // 判断拖动距离是否足够 小于这个距离 → 视为点击，不触发拖拽
    if ((event->pos() - m_startPos).manhattanLength() < QApplication::startDragDistance())
        return;

    // 创建拖拽对象
    auto* drag = new QDrag(this);
    auto* mimeData = new QMimeData; // 存储拖拽的数据内容

    // 设置拖拽数据为工具类型
    mimeData->setText(m_toolType);
    drag->setMimeData(mimeData);

    // 可选：拖拽显示按钮本身的 pixmap
    drag->setPixmap(this->grab()); // 拖动时，光标旁边会显示按钮的截图

    // 执行拖拽，操作类型是 复制
    drag->exec(Qt::CopyAction);
}
