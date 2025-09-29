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
    if (!start || !end)
        return nullptr;
    auto* edge = new GraphEdge(start, end);
    edge->setWeight(weight);
    addItem(edge);
    m_edges.append(edge);
    return edge;
}

void GraphScene::removeEdge(GraphEdge* edge) {
    if (!edge)
        return;
    removeItem(edge);
    m_edges.removeAll(edge);
    delete edge;
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

// 鼠标事件用于连线
void GraphScene::mousePressEvent(QGraphicsSceneMouseEvent* event) {
    QGraphicsItem* item = itemAt(event->scenePos(), QTransform());
    auto* node = dynamic_cast<GraphNode*>(item);
    if (node && event->button() == Qt::RightButton) {
        m_lineStartNode = node;
        m_tempEdge = new GraphEdge(node, node);
        addItem(m_tempEdge);
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
