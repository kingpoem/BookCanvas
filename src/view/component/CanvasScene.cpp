#include "CanvasScene.h"
#include <QGraphicsScene>
#include <QGraphicsSceneEvent>
#include <QMimeData>

CanvasScene::CanvasScene(QObject* parent)
    : ElaGraphicsScene(parent) {
    setBackgroundBrush(Qt::white); // 设置背景颜色为白色
    setSceneRect(0, 0, 1200, 800); // 默认画布大小
}

// 拖拽进入
void CanvasScene::dragEnterEvent(QGraphicsSceneDragDropEvent* event) {
    if (event->mimeData()->hasText())  // 如果拖拽的数据中包含文本
        event->acceptProposedAction(); // 接受拖拽操作
}

// 拖动中
void CanvasScene::dragMoveEvent(QGraphicsSceneDragDropEvent* event) {
    event->acceptProposedAction(); // 一律接受，允许在场景内移动
}

// 放下
void CanvasScene::dropEvent(QGraphicsSceneDragDropEvent* event) {
    if (!event->mimeData()->hasText()) // 如果拖拽的数据中没有文本
        return;                        // 什么也不做

    QString toolName = event->mimeData()->text(); // 读取拖拽进来的文本内容，作为工具名称
    QPointF pos = event->scenePos();              // 获取释放时的场景坐标

    addToolItem(toolName, pos);    // 根据工具名称在场景里添加对应图形
    event->acceptProposedAction(); // 确认拖放操作
}

// 根据工具名称生成图形
void CanvasScene::addToolItem(const QString& toolName, const QPointF& pos) {
    const int size = 50; // 图形默认大小

    if (toolName == "Circle") {
        addEllipse(pos.x() - size / 2,
                   pos.y() - size / 2,
                   size,
                   size,
                   QPen(Qt::black),
                   QBrush(Qt::red));
    } else if (toolName == "Square") {
        addRect(pos.x() - size / 2,
                pos.y() - size / 2,
                size,
                size,
                QPen(Qt::black),
                QBrush(Qt::green));
    } else if (toolName == "ArrowRight") {
        QPolygonF arrow;
        arrow << QPointF(pos.x(), pos.y()) << QPointF(pos.x() + size, pos.y() + size / 2)
              << QPointF(pos.x(), pos.y() + size);
        addPolygon(arrow, QPen(Qt::black), QBrush(Qt::blue));
    }
}
