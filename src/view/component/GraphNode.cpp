#include "GraphNode.h"
#include <QPainter>
#include <QStyleOptionGraphicsItem>

GraphNode::GraphNode(const QString& id, QGraphicsItem* parent)
    : ElaGraphicsItem(parent)
    , m_id(id)
    , m_label(id) {
    setFlags(ItemIsSelectable | ItemSendsGeometryChanges); // 可移动、可选中，并能报告几何变化
    // setFlags(ItemIsMovable); 设置节点拖拽属性
}

QRectF GraphNode::boundingRect() const {
    return m_rect;
}

// 绘制节点外观
void GraphNode::paint(QPainter* painter,
                      const QStyleOptionGraphicsItem* /*option*/,
                      QWidget* /*widget*/) {
    painter->setBrush(Qt::black);
    painter->setPen(QPen(Qt::black, 2));
    painter->drawEllipse(m_rect);
    painter->drawText(m_rect, Qt::AlignCenter, m_label);
}

void GraphNode::mousePressEvent(QGraphicsSceneMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        m_dragging = true;
        m_dragStartPos = event->pos(); // 保存按下位置
        emit posChanged(event->pos(), mapToScene(event->pos()));
    }
    QGraphicsItem::mousePressEvent(event);
}

void GraphNode::mouseMoveEvent(QGraphicsSceneMouseEvent* event) {
    if (m_dragging) {
        // 计算偏移量，更新节点位置
        QPointF delta = event->pos() - m_dragStartPos;
        setPos(pos() + delta);
    }
    QGraphicsItem::mouseMoveEvent(event);
}

void GraphNode::mouseReleaseEvent(QGraphicsSceneMouseEvent* event) {
    m_dragging = false;
    QGraphicsItem::mouseReleaseEvent(event);
}
