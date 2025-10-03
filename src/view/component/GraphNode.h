#pragma once
#include "ElaGraphicsItem.h"
#include <QGraphicsSceneMouseEvent>
#include <QRectF>
#include <QString>

class GraphNode : public ElaGraphicsItem {
    Q_OBJECT
public:
    enum NodeState { Normal, Highlighted };
    explicit GraphNode(QString id, QGraphicsItem* parent = nullptr);

    void setNodeState(NodeState state) {
        m_state = state;
        update();
    }
    NodeState nodestate() { return m_state; }

    [[nodiscard]] QRectF boundingRect() const override; // 定义节点边界矩形
    void paint(QPainter* painter,
               const QStyleOptionGraphicsItem* option,
               QWidget* widget) override; // 绘制节点外观 QGraphicsItem 虚函数 刷新界面时自动调用

    [[nodiscard]] QString getId() const { return m_id; };

signals:
    void posChanged(const QPointF& localPos, const QPointF& scenePos);

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;

private:
    QString m_id;
    QRectF m_rect = QRectF(0, 0, 50, 50);
    NodeState m_state = Normal;
};
