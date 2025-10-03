#include "GraphNode.h"
#include <QDebug>
#include <QPainter>
#include <QStyleOptionGraphicsItem>

GraphNode::GraphNode(QString id, QGraphicsItem* parent)
    : ElaGraphicsItem(parent)
    , m_id(std::move(id)) {
    setFlags(ItemIsMovable | ItemIsSelectable
             | ItemSendsGeometryChanges); // 可移动 选中 报告几何变化
}

QRectF GraphNode::boundingRect() const {
    return m_rect;
}

void GraphNode::paint(QPainter* painter,
                      const QStyleOptionGraphicsItem* option,
                      QWidget* /*widget*/) {
    QPen pen(Qt::black, 2);
    QBrush brush(Qt::black);

    // 这里只做外观绘制，不加逻辑
    if (option->state & QStyle::State_Selected) {
        pen.setColor(Qt::red);
        pen.setWidth(3);
        brush.setColor(Qt::yellow);
    }

    painter->setPen(pen);
    painter->setBrush(brush);
    painter->drawEllipse(m_rect);
}

// 捕捉选中状态变化
QVariant GraphNode::itemChange(GraphicsItemChange change, const QVariant& value) {
    if (change == ItemSelectedChange) {
        emit posChanged(this->pos(), this->scenePos());
        bool selected = value.toBool();
        if (selected) {
            qDebug() << "节点被选中:" << m_id;
        } else {
            qDebug() << "节点取消选中:" << m_id;
        }
    }
    return ElaGraphicsItem::itemChange(change, value);
}
