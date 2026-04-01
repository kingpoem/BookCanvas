#pragma once
#include "GraphNode.h"
#include <ElaGraphicsItem.h>
#include <QBrush>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsTextItem>
#include <QObject>
#include <QPainterPath>
#include <QPen>
#include <QVector>

class GraphEdgeBendHandle;

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
    [[nodiscard]] QPainterPath shape() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

public slots:
    // 更新边的几何位置：
    // 当节点被移动时，需要调用它来调整连线的起止点和权重文字的位置
    void updatePosition(); // 根据节点位置更新线段

protected:
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) override;
    void contextMenuEvent(QGraphicsSceneContextMenuEvent* event) override;
    [[nodiscard]] QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;

private:
    friend class GraphEdgeBendHandle;
    void rebuildPolylineFromAnchors();
    void applyPathThroughBendScene(const QPointF& sceneA, const QPointF& sceneB);
    void placeWeightLabel();
    void syncBendHandlePos();
    void onBendHandleMoved(const QPointF& handleTopLeft);

    GraphNode* m_startNode;
    GraphNode* m_endNode;
    double m_weight = 1.0;
    QLineF m_line;
    QVector<QPointF> m_polylineLocal;
    QGraphicsTextItem* m_weightText{};
    GraphEdgeBendHandle* m_bendHandle = nullptr;
    QPointF m_bendScene;
    bool m_bendUserEdited = false;
    bool m_syncingBendHandle = false;
};
