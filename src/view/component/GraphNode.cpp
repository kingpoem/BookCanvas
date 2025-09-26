#include "GraphNode.h"
#include <QPainter>
#include <QStyleOptionGraphicsItem>

GraphNode::GraphNode(const QString& id, QGraphicsItem* parent)
    : ElaGraphicsItem(parent)
    , m_id(id)
    , m_label(id) {
    setFlags(ItemIsMovable | ItemIsSelectable
             | ItemSendsGeometryChanges); // 可移动、可选中，并能报告几何变化
}

QRectF GraphNode::boundingRect() const {
    return m_rect;
}

// 绘制节点外观
void GraphNode::paint(QPainter* painter,
                      const QStyleOptionGraphicsItem* /*option*/,
                      QWidget* /*widget*/) {
    painter->setBrush(Qt::yellow);
    painter->setPen(QPen(Qt::black, 2));
    painter->drawRect(m_rect);
    painter->drawText(m_rect, Qt::AlignCenter, m_label);

    painter->setBrush(Qt::black);
    painter->drawEllipse(QPointF(m_rect.left(), m_rect.center().y()), 2, 2);
    painter->drawEllipse(QPointF(m_rect.right(), m_rect.center().y()), 2, 2);
}

QPointF GraphNode::getPortPosition(int portIndex) const {
    switch (portIndex) {
    case 0:
        return mapToScene(m_rect.left(), m_rect.center().y()); // 左中
    case 1:
        return mapToScene(m_rect.right(), m_rect.center().y()); // 右中
    default:
        return mapToScene(m_rect.center());
    }
}

void GraphNode::mousePressEvent(QGraphicsSceneMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        m_dragging = true;
        m_dragStartPos = event->pos();
    }
    QGraphicsItem::mousePressEvent(event);
}

void GraphNode::mouseMoveEvent(QGraphicsSceneMouseEvent* event) {
    if (m_dragging) {
        QGraphicsItem::mouseMoveEvent(event);
    }
}

void GraphNode::mouseReleaseEvent(QGraphicsSceneMouseEvent* event) {
    m_dragging = false;
    QGraphicsItem::mouseReleaseEvent(event);
}
