#include "GraphView.h"
#include "GraphScene.h"
#include <QEvent>
#include <QKeyEvent>
#include <QScrollBar>
#include <QWheelEvent>
#include <algorithm>

namespace {

constexpr double kZoomStepFactor = 1.15;
constexpr qreal kKeyboardPanStepInViewPx = 56.0;
constexpr qreal kPanSceneMargin = 2000.0;

bool hasZoomModifier(Qt::KeyboardModifiers modifiers) {
    return modifiers.testFlag(Qt::ControlModifier) || modifiers.testFlag(Qt::MetaModifier);
}

} // namespace

GraphView::GraphView(GraphScene* scene, QWidget* parent)
    : ElaGraphicsView(scene, parent) {
    setRenderHint(QPainter::Antialiasing);      // 开启抗锯齿
    setDragMode(QGraphicsView::RubberBandDrag); // 使用鼠标拖出“橡皮筋”选择框来选择多个图形项
    setAcceptDrops(true); // 启用拖放功能，让 View 可以接收拖放事件（文件、图形元素等）
    setMouseTracking(true);
    setFrameShape(QFrame::NoFrame);

    if (scene) {
        scene->setBackgroundBrush(palette().color(QPalette::Base));
    }
}

void GraphView::ensureSceneRectHasPanMargin() {
    if (!scene()) {
        return;
    }
    QRectF rect = scene()->sceneRect();
    const QRect viewRect = viewport()->rect();
    const QPointF p1 = mapToScene(viewRect.topLeft());
    const QPointF p2 = mapToScene(viewRect.bottomRight());
    const QRectF viewInScene = QRectF(p1, p2).normalized();
    const QRectF needRect = viewInScene.adjusted(-kPanSceneMargin,
                                                 -kPanSceneMargin,
                                                 kPanSceneMargin,
                                                 kPanSceneMargin);
    if (!rect.contains(needRect)) {
        scene()->setSceneRect(rect.united(needRect));
    }
}

void GraphView::panViewportBy(qreal dx, qreal dy) {
    ensureSceneRectHasPanMargin();
    if (auto* hBar = horizontalScrollBar()) {
        hBar->setValue(hBar->value() + static_cast<int>(std::lround(dx)));
    }
    if (auto* vBar = verticalScrollBar()) {
        vBar->setValue(vBar->value() + static_cast<int>(std::lround(dy)));
    }
}

void GraphView::zoomInStep() {
    applyZoomFactor(kZoomStepFactor);
    ensureSceneRectHasPanMargin();
}

void GraphView::zoomOutStep() {
    applyZoomFactor(1.0 / kZoomStepFactor);
    ensureSceneRectHasPanMargin();
}

void GraphView::resetZoomToDefault() {
    resetTransform();
    m_scale = 1.0;
    ensureSceneRectHasPanMargin();
}

void GraphView::mouseMoveEvent(QMouseEvent* event) {
    QPoint viewPos = event->pos();          // 视图坐标
    QPointF scenePos = mapToScene(viewPos); // 场景坐标
    ElaGraphicsView::mouseMoveEvent(event);
}

void GraphView::applyZoomFactor(double factor) {
    if (factor <= 0.0 || m_scale <= 0.0) {
        return;
    }
    const double targetScale = std::clamp(m_scale * factor, m_minScale, m_maxScale);
    const double appliedFactor = targetScale / m_scale;
    if (qFuzzyCompare(appliedFactor, 1.0)) {
        return;
    }
    scale(appliedFactor, appliedFactor);
    m_scale = targetScale;
}

// 滚轮缩放 + 限制最大最小值
void GraphView::wheelEvent(QWheelEvent* event) {
    const int deltaY = event->angleDelta().y();
    if (deltaY != 0) {
        if (deltaY > 0) {
            zoomInStep();
        } else {
            zoomOutStep();
        }
    }
    event->accept();
}

// Ctrl/Cmd + 0 恢复默认缩放
void GraphView::keyPressEvent(QKeyEvent* event) {
    const Qt::KeyboardModifiers modifiers = event->modifiers();
    const int key = event->key();

    if (hasZoomModifier(modifiers) && key == Qt::Key_0) {
        resetZoomToDefault();
        event->accept();
        return;
    }

    if (event->matches(QKeySequence::ZoomIn)
        || (hasZoomModifier(modifiers) && (key == Qt::Key_Plus || key == Qt::Key_Equal))) {
        zoomInStep();
        event->accept();
        return;
    }

    if (event->matches(QKeySequence::ZoomOut)
        || (hasZoomModifier(modifiers) && (key == Qt::Key_Minus || key == Qt::Key_Underscore))) {
        zoomOutStep();
        event->accept();
        return;
    }

    // 用视图变换平移相机，不依赖场景边界，任意画布尺寸都可平移。
    if (key == Qt::Key_Left) {
        panViewportBy(-kKeyboardPanStepInViewPx, 0.0);
        event->accept();
        return;
    }
    if (key == Qt::Key_Right) {
        panViewportBy(kKeyboardPanStepInViewPx, 0.0);
        event->accept();
        return;
    }
    if (key == Qt::Key_Up) {
        panViewportBy(0.0, -kKeyboardPanStepInViewPx);
        event->accept();
        return;
    }
    if (key == Qt::Key_Down) {
        panViewportBy(0.0, kKeyboardPanStepInViewPx);
        event->accept();
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

void GraphView::changeEvent(QEvent* event) {
    if (event->type() == QEvent::PaletteChange) {
        if (GraphScene* gs = qobject_cast<GraphScene*>(this->scene())) {
            gs->setBackgroundBrush(palette().color(QPalette::Base));
        }
    }
    ElaGraphicsView::changeEvent(event);
}

// 网格：按 Base 亮度选黑/白细线，不依赖 WindowText（部分主题下 WindowText 与 Base 过近导致看不见）
void GraphView::drawBackground(QPainter* painter, const QRectF& rect) {
    painter->fillRect(rect, palette().color(QPalette::Base));

    const int gridSize = 20;
    const QColor base = palette().color(QPalette::Base);
    const bool lightBg = base.lightness() > 128;
    QColor grid = lightBg ? QColor(0, 0, 0) : QColor(255, 255, 255);
    grid.setAlpha(lightBg ? 48 : 56);
    painter->setPen(QPen(grid, 1, Qt::SolidLine));

    int left = static_cast<int>(rect.left()) - (static_cast<int>(rect.left()) % gridSize);
    int right = static_cast<int>(rect.right());
    for (int xi = left; xi <= right; xi += gridSize) {
        auto x = static_cast<qreal>(xi);
        painter->drawLine(QLineF(x, rect.top(), x, rect.bottom()));
    }

    int top = static_cast<int>(rect.top()) - (static_cast<int>(rect.top()) % gridSize);
    int bottom = static_cast<int>(rect.bottom());
    for (int yi = top; yi <= bottom; yi += gridSize) {
        auto y = static_cast<qreal>(yi);
        painter->drawLine(QLineF(rect.left(), y, rect.right(), y));
    }
}
