#pragma once
#include "GraphNode.h"
#include <ElaGraphicsItem.h>
#include <QBrush>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsTextItem>
#include <QObject>
#include <QPen>

class GraphEdge : public ElaGraphicsItem {
    Q_OBJECT
public:
    explicit GraphEdge(GraphNode* startNode, GraphNode* endNode, QGraphicsItem* parent = nullptr);

    [[nodiscard]] GraphNode* startNode() const { return m_startNode; }
    [[nodiscard]] GraphNode* endNode() const { return m_endNode; }

    // 获取和设置边的权重
    [[nodiscard]] double weight() const { return m_weight; }
    void setWeight(double w);

    // 控制权重文本的显示/隐藏
    void setWeightVisible(bool visible);

    void setLine(const QLineF& line);
    [[nodiscard]] const QLineF& line() const { return m_line; }

    [[nodiscard]] QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

public slots:
    // 更新边的几何位置：
    // 当节点被移动时，需要调用它来调整连线的起止点和权重文字的位置
    void updatePosition(); // 根据节点位置更新线段

protected:
    // 鼠标双击事件：
    // 例如双击边可以弹出对话框修改权重，或者实现其它交互功能
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) override;
    void contextMenuEvent(QGraphicsSceneContextMenuEvent* event) override; // 右键菜单

private:
    GraphNode* m_startNode;            // 起点节点指针
    GraphNode* m_endNode;              // 终点节点指针
    double m_weight = 1.0;             // 边的权重（默认 1.0）
    QLineF m_line;                     // 保存边的线段
    QGraphicsTextItem* m_weightText{}; // 显示权重的文本对象（通常放在线条中点）
};
