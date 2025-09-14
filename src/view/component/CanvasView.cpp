#include "CanvasView.h"
#include <QWheelEvent>

CanvasView::CanvasView(QWidget* parent)
    : ElaGraphicsView(parent) {
    auto* scene = new CanvasScene(this);
    setScene(scene);

    setRenderHint(QPainter::Antialiasing);
    setDragMode(QGraphicsView::RubberBandDrag); // 橡皮筋选择
    setAcceptDrops(true);                       // View 接收拖放事件
}

CanvasView::CanvasView(CanvasScene* scene, QWidget* parent)
    : ElaGraphicsView(scene, parent) {
    setRenderHint(QPainter::Antialiasing);
    setDragMode(QGraphicsView::RubberBandDrag);
    setAcceptDrops(true);
}

// 滚轮缩放
void CanvasView::wheelEvent(QWheelEvent* event) {
    const double scaleFactor = 1.15;
    if (event->angleDelta().y() > 0)
        scale(scaleFactor, scaleFactor);
    else
        scale(1.0 / scaleFactor, 1.0 / scaleFactor);
}
