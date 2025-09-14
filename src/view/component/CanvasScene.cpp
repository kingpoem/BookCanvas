#include "CanvasScene.h"
#include <QGraphicsScene>
#include <QGraphicsSceneEvent>
#include <QMimeData>

CanvasScene::CanvasScene(QObject* parent)
    : ElaGraphicsScene(parent) {
    setBackgroundBrush(Qt::white);
    setSceneRect(0, 0, 1200, 800); // 默认画布大小
}

// 拖拽进入
void CanvasScene::dragEnterEvent(QGraphicsSceneDragDropEvent* event) {
    if (event->mimeData()->hasText())
        event->acceptProposedAction();
}

// 拖动中
void CanvasScene::dragMoveEvent(QGraphicsSceneDragDropEvent* event) {
    event->acceptProposedAction();
}

// 放下
void CanvasScene::dropEvent(QGraphicsSceneDragDropEvent* event) {
    if (!event->mimeData()->hasText())
        return;

    QString toolName = event->mimeData()->text();
    QPointF pos = event->scenePos();

    addToolItem(toolName, pos);
    event->acceptProposedAction();
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
