#include "GraphNode.h"
#include <QPainter>
#include <QStyleOptionGraphicsItem>

GraphNode::GraphNode(QString id, QGraphicsItem* parent)
    : ElaGraphicsItem(parent)
    , m_id(std::move(id)) {
    setFlags(ItemIsSelectable | ItemSendsGeometryChanges
             | ItemIsMovable); // 可移动、可选中，并能报告几何变化
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
    painter->drawEllipse(m_rect); // 局部坐标系
}
