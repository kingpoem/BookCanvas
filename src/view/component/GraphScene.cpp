#include "GraphScene.h"
#include <QBrush>
#include <QFile>
#include <QGraphicsLineItem>
#include <QGraphicsSceneMouseEvent>
#include <QMimeData>
#include <QPen>
#include <QTextStream>

GraphScene::GraphScene(QObject* parent)
    : ElaGraphicsScene(parent) {
    setSceneRect(QRectF(0, 0, 1200, 600));
    // (x, y ,sceneX, sceneY) (x, y) 控制画布左上角点 (sceneX, sceneY) 控制宽高
}

GraphNode* GraphScene::createNode(const QString& id, const QPointF& pos) {
    auto* node = new GraphNode(id);
    addItem(node);
    node->setPos(pos); // NOTE: this line must be after addItem(node); `addItem` has written `setPos`
    m_nodes.append(node);
    return node;
}

void GraphScene::removeNode(GraphNode* node) {
    if (!node)
        return;
    // 删除关联边
    for (int i = m_edges.size() - 1; i >= 0; --i) {
        if (m_edges[i]->startNode() == node || m_edges[i]->endNode() == node) {
            removeEdge(m_edges[i]);
        }
    }
    removeItem(node);
    m_nodes.removeAll(node);
    delete node;
}

GraphEdge* GraphScene::createEdge(GraphNode* start, GraphNode* end, double weight) {
    if (!start || !end) return nullptr;
    auto* edge = new GraphEdge(start, end);
    addItem(edge);

    // 现在 item 已在 scene 中，可以正确 mapFromScene
    edge->setWeight(weight);
    edge->updatePosition();

    // 让边随节点移动（节点发出 posChanged）
    connect(start, &GraphNode::posChanged, edge, &GraphEdge::updatePosition);
    connect(end, &GraphNode::posChanged, edge, &GraphEdge::updatePosition);

    m_edges.append(edge);
    return edge;
}

void GraphScene::removeEdge(GraphEdge* edge) {
    if (!edge)
        return;
    removeItem(edge);
    qDebug() << m_edges.size() << ' ';
    m_edges.removeAll(edge);
    // delete edge; NOTE: add this line will contribute to segmentation fault;
}

void GraphScene::setAllEdgeWeightsVisible(bool visible) {
    for (GraphEdge* edge : m_edges) {
        edge->setWeightVisible(visible);
    }
}

// 拖拽生成节点
void GraphScene::dragEnterEvent(QGraphicsSceneDragDropEvent* event) {
    if (event->mimeData()->hasText()) {
        m_pendingToolName = event->mimeData()->text();
        event->acceptProposedAction();
    }
}

void GraphScene::dragMoveEvent(QGraphicsSceneDragDropEvent* event) {
    event->acceptProposedAction();
}

// 放置节点的直接逻辑
void GraphScene::dropEvent(QGraphicsSceneDragDropEvent* event) {
    if (!m_pendingToolName.isEmpty()) {
        QString id = QString("Node_%1").arg(m_nodes.size());
        createNode(id, event->scenePos()); // event->scenePos() is true
        m_pendingToolName.clear();
        event->acceptProposedAction();
    }
}

void GraphScene::mousePressEvent(QGraphicsSceneMouseEvent* event) {
    QGraphicsItem* item = itemAt(event->scenePos(), QTransform());
    auto* node = dynamic_cast<GraphNode*>(item);

    if (event->button() == Qt::LeftButton) {
        if (node) {
            if (!m_highlightNode) {
                // 没有高亮 → 设置为高亮
                m_highlightNode = node;
                node->setNodeState(GraphNode::Highlighted);
            } else if (m_highlightNode == node) {
                // 再点自己 → 取消高亮
                m_highlightNode->setNodeState(GraphNode::Normal);
                m_highlightNode = nullptr;
            } else {
                // 有高亮节点且点了别的节点 → 创建连线
                createEdge(m_highlightNode, node, 1.0);
                // 两个节点恢复默认
                m_highlightNode->setNodeState(GraphNode::Normal);
                node->setNodeState(GraphNode::Normal);
                m_highlightNode = nullptr;
            }
        } else {
            // 点击空白 → 取消高亮
            if (m_highlightNode) {
                m_highlightNode->setNodeState(GraphNode::Normal);
                m_highlightNode = nullptr;
            }
        }
    }

    ElaGraphicsScene::mousePressEvent(event);
}

void GraphScene::mouseMoveEvent(QGraphicsSceneMouseEvent* event) {
    if (m_tempEdge && m_lineStartNode) {
        QLineF line(m_lineStartNode->sceneBoundingRect().center(), event->scenePos());
        m_tempEdge->setLine(line);
    }
    ElaGraphicsScene::mouseMoveEvent(event);
}

void GraphScene::mouseReleaseEvent(QGraphicsSceneMouseEvent* event) {
    if (m_tempEdge && m_lineStartNode) {
        QGraphicsItem* item = itemAt(event->scenePos(), QTransform());
        auto* endNode = dynamic_cast<GraphNode*>(item);
        if (endNode && endNode != m_lineStartNode) {
            m_tempEdge->setParentItem(nullptr);
            m_tempEdge->setWeight(1.0);
            m_tempEdge->updatePosition();
            m_edges.append(m_tempEdge);
        } else {
            removeItem(m_tempEdge);
            delete m_tempEdge;
        }
        m_tempEdge = nullptr;
        m_lineStartNode = nullptr;
    }
    ElaGraphicsScene::mouseReleaseEvent(event);
}

// 导出图信息
void GraphScene::exportGraph(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return;

    QTextStream out(&file);
    out << "Nodes:\n";
    for (GraphNode* node : m_nodes) {
        out << node->getId() << ", x=" << node->scenePos().x() << ", y=" << node->scenePos().y()
            << "\n";
    }

    out << "Edges:\n";
    for (GraphEdge* edge : m_edges) {
        out << edge->startNode()->getId() << " -> " << edge->endNode()->getId()
            << ", weight=" << edge->weight() << "\n";
    }
    file.close();
}
