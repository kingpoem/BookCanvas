#pragma once
#include "ElaGraphicsItem.h"
#include <QGraphicsSceneMouseEvent>
#include <QRectF>
#include <QString>

class GraphNode : public ElaGraphicsItem {
    Q_OBJECT
public:
    enum NodeState { Normal, Highlighted };
    enum NodeType { Node, Router }; // 添加节点类型枚举
    explicit GraphNode(QString id, NodeType type = Node, QGraphicsItem* parent = nullptr);

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
    [[nodiscard]] NodeType getType() const { return m_type; }; // 获取节点类型

signals:
    void posChanged(const QPointF& localPos, const QPointF& scenePos);
    void configureRequested(GraphNode* node); // 配置请求信号

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;
    void contextMenuEvent(QGraphicsSceneContextMenuEvent* event) override; // 右键菜单

private:
    QString m_id;
    NodeType m_type; // 节点类型
    QRectF m_rect = QRectF(0, 0, 50, 50);
    NodeState m_state = Normal;
};
