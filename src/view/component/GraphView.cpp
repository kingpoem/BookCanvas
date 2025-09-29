#include "GraphView.h"
#include "GraphScene.h"
#include <QWheelEvent>

GraphView::GraphView(GraphScene* scene, QWidget* parent)
    : ElaGraphicsView(scene, parent) {
    setRenderHint(QPainter::Antialiasing);      // 开启抗锯齿，使画布线条更顺滑
    setDragMode(QGraphicsView::RubberBandDrag); // 使用鼠标拖出“橡皮筋”选择框来选择多个图形项
    setAcceptDrops(true); // 启用拖放功能，让 View 可以接收拖放事件（文件、图形元素等）
}

// 滚轮缩放
void GraphView::wheelEvent(QWheelEvent* event) {
    const double scaleFactor = 1.15;
    if (event->angleDelta().y() > 0)
        scale(scaleFactor, scaleFactor);
    else
        scale(1.0 / scaleFactor, 1.0 / scaleFactor);
}
