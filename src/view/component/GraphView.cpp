#include "GraphView.h"
#include "GraphScene.h"
#include <QWheelEvent>

GraphView::GraphView(GraphScene* scene, QWidget* parent)
    : ElaGraphicsView(scene, parent) {
    setRenderHint(QPainter::Antialiasing);      // 开启抗锯齿
    setDragMode(QGraphicsView::RubberBandDrag); // 使用鼠标拖出“橡皮筋”选择框来选择多个图形项
    setAcceptDrops(true); // 启用拖放功能，让 View 可以接收拖放事件（文件、图形元素等）
}

// 滚轮缩放 + 限制最大最小值
void GraphView::wheelEvent(QWheelEvent* event) {
    constexpr double scaleFactor = 1.15;

    if (event->angleDelta().y() > 0 && m_scale < m_maxScale) {
        scale(scaleFactor, scaleFactor);
        m_scale *= scaleFactor;
    } else if (event->angleDelta().y() < 0 && m_scale > m_minScale) {
        scale(1.0 / scaleFactor, 1.0 / scaleFactor);
        m_scale /= scaleFactor;
    }
}

// Ctrl + 0 恢复默认缩放
// macos is command + 0
void GraphView::keyPressEvent(QKeyEvent* event) {
    if (event->modifiers() & Qt::ControlModifier && event->key() == Qt::Key_0) {
        resetTransform();
        m_scale = 1.0;
        return;
    }
    ElaGraphicsView::keyPressEvent(event);
}

// 鼠标中键平移画布
void GraphView::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::MiddleButton) {
        setDragMode(QGraphicsView::ScrollHandDrag);
        QMouseEvent fakeEvent(QEvent::MouseButtonPress,
                              event->pos(),
                              event->globalPosition(),
                              Qt::LeftButton,
                              Qt::LeftButton,
                              Qt::NoModifier);
        QGraphicsView::mousePressEvent(&fakeEvent);
    } else {
        QGraphicsView::mousePressEvent(event);
    }
}

void GraphView::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::MiddleButton) {
        setDragMode(QGraphicsView::RubberBandDrag);
        QMouseEvent fakeEvent(QEvent::MouseButtonRelease,
                              event->pos(),
                              event->globalPosition(),
                              Qt::LeftButton,
                              Qt::LeftButton,
                              Qt::NoModifier);
        QGraphicsView::mouseReleaseEvent(&fakeEvent);
    } else {
        QGraphicsView::mouseReleaseEvent(event);
    }
}

// 网格绘制
void GraphView::drawBackground(QPainter* painter, const QRectF& rect) {
    const int gridSize = 20;
    QPen pen(QColor(230, 230, 230));
    painter->setPen(pen);

    // 画竖线
    int left = static_cast<int>(rect.left()) - (static_cast<int>(rect.left()) % gridSize);
    int right = static_cast<int>(rect.right());
    for (int xi = left; xi <= right; xi += gridSize) {
        auto x = static_cast<qreal>(xi);
        painter->drawLine(QLineF(x, rect.top(), x, rect.bottom()));
    }

    // 画横线
    int top = static_cast<int>(rect.top()) - (static_cast<int>(rect.top()) % gridSize);
    int bottom = static_cast<int>(rect.bottom());
    for (int yi = top; yi <= bottom; yi += gridSize) {
        auto y = static_cast<qreal>(yi);
        painter->drawLine(QLineF(rect.left(), y, rect.right(), y));
    }
}
