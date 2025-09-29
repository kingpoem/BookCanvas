#pragma once
#include "ElaGraphicsScene.h"
#include "GraphEdge.h"
#include "GraphNode.h"
#include <QList>
#include <QMap>
#include <QString>

// 提供了节点/边的创建、删除，以及与拖拽交互、鼠标事件相关的逻辑
class GraphScene : public ElaGraphicsScene {
    Q_OBJECT
public:
    explicit GraphScene(QObject* parent = nullptr);

    GraphNode* createNode(const QString& id,
                          const QPointF& pos); // 创建节点
    void removeNode(GraphNode* node);          // 删除节点和相关的边
    [[nodiscard]] QList<GraphNode*> nodes() const {
        return m_nodes;
    } // 返回当前场景中所有节点的列表

    // 边管理
    GraphEdge* createEdge(GraphNode* start, GraphNode* end, double weight = 1.0);
    void removeEdge(GraphEdge* edge);
    [[nodiscard]] QList<GraphEdge*> edges() const { return m_edges; }

    // 导出图信息
    void exportGraph(const QString& filePath);
    // drawBackground

protected:
    // 从工具栏拖动一个节点到场景
    void dragEnterEvent(QGraphicsSceneDragDropEvent* event) override;
    void dragMoveEvent(QGraphicsSceneDragDropEvent* event) override;
    void dropEvent(QGraphicsSceneDragDropEvent* event) override;
    // 鼠标交互事件（用于节点选择、拖动、连线等）
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;

private:
    QList<GraphNode*> m_nodes; // 存放所有节点
    QList<GraphEdge*> m_edges; // 存放所有边

    // 连线临时变量
    GraphNode* m_lineStartNode = nullptr; // 存放所有边
    GraphEdge* m_tempEdge = nullptr;      // 临时连线（随着鼠标移动）

    // 工具栏拖拽生成节点
    QString m_pendingToolName;
};
