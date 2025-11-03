#include "GraphNode.h"
#include <QDebug>
#include <QGraphicsSceneContextMenuEvent>
#include <QMenu>
#include <QPainter>
#include <QStyleOptionGraphicsItem>

GraphNode::GraphNode(QString id, NodeType type, QGraphicsItem* parent)
    : ElaGraphicsItem(parent)
    , m_id(std::move(id))
    , m_type(type) {
    setFlags(ItemIsMovable | ItemIsSelectable
             | ItemSendsGeometryChanges); // 可移动 选中 报告几何变化

    setAcceptedMouseButtons(Qt::LeftButton | Qt::RightButton);
}

QRectF GraphNode::boundingRect() const {
    return m_rect;
}

void GraphNode::paint(QPainter* painter,
                      const QStyleOptionGraphicsItem* option,
                      QWidget* /*widget*/) {
    QPen pen(Qt::black, 2);
    QBrush brush;

    // 根据节点类型设置不同的颜色和形状
    if (m_type == Router) {
        brush.setColor(Qt::blue); // 路由器用蓝色
    } else {
        brush.setColor(Qt::yellow); // 普通节点用黄色
    }

    // 设置高亮状态的样式
    if (m_state == Highlighted) {
        pen.setColor(Qt::red);
        pen.setWidth(3);
        if (m_type == Router) {
            brush.setColor(Qt::cyan);
        } else {
            brush.setColor(Qt::lightGray);
        }
    } else if (option->state & QStyle::State_Selected) {
        pen.setColor(Qt::red);
        pen.setWidth(3);
        if (m_type == Router) {
            brush.setColor(QColor(173, 216, 230)); // lightBlue 的 RGB 值
        } else {
            brush.setColor(Qt::yellow);
        }
    }

    painter->setPen(pen);
    painter->setBrush(brush);

    // 根据类型绘制不同形状
    if (m_type == Router) {
        painter->drawRect(m_rect); // 路由器绘制方形
        // 在中心绘制路由器标识文本
        painter->setPen(Qt::white);
        QFont font = painter->font();
        font.setPointSize(8);
        painter->setFont(font);
        painter->drawText(m_rect, Qt::AlignCenter, "R");
    } else {
        painter->drawEllipse(m_rect); // 普通节点绘制圆形
        // 在中心绘制节点标识文本
        painter->setPen(Qt::black);
        QFont font = painter->font();
        font.setPointSize(8);
        painter->setFont(font);
        painter->drawText(m_rect, Qt::AlignCenter, "N");
    }
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

// 右键菜单事件
void GraphNode::contextMenuEvent(QGraphicsSceneContextMenuEvent* event) {
    if (m_type == Router) {
        QMenu menu;
        QAction* configAction = menu.addAction("配置参数");

        QAction* selected = menu.exec(event->screenPos());
        if (selected == configAction) {
            emit configureRequested(this);
        }
    }
}
