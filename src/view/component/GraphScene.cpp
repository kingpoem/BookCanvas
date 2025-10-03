#include "GraphScene.h"
#include <QBrush>
#include <QFile>
#include <QGraphicsLineItem>
#include <QGraphicsSceneMouseEvent>
#include <QMap>
#include <QMimeData>
#include <QPen>
#include <QTextStream>

GraphScene::GraphScene(QObject* parent)
    : ElaGraphicsScene(parent) {
    setSceneRect(QRectF(0, 0, 1200, 600));
    // (x, y ,sceneX, sceneY) (x, y) 控制画布左上角点 (sceneX, sceneY) 控制宽高
}

GraphNode* GraphScene::createNode(const QString& id, const QPointF& pos, GraphNode::NodeType type) {
    auto* node = new GraphNode(id, type);
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

    // 检查连接是否合法
    if (!isConnectionValid(start, end)) {
        qDebug() << "非法连接：节点只能连接路由器";
        return nullptr;
    }

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

bool GraphScene::isConnectionValid(GraphNode* start, GraphNode* end) const {
    if (!start || !end)
        return false;

    // 路由器可以连接任何节点（路由器或普通节点）
    if (start->getType() == GraphNode::Router) {
        return true;
    }

    // 普通节点只能连接路由器
    if (start->getType() == GraphNode::Node) {
        return end->getType() == GraphNode::Router;
    }

    return false;
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
        QString id;
        GraphNode::NodeType type;

        if (m_pendingToolName == "Router") {
            int routerCount = 0;
            for (GraphNode* node : m_nodes) {
                if (node->getType() == GraphNode::Router) {
                    routerCount++;
                }
            }
            id = QString("Router_%1").arg(routerCount);
            type = GraphNode::Router;
        } else {
            int nodeCount = 0;
            for (GraphNode* node : m_nodes) {
                if (node->getType() == GraphNode::Node) {
                    nodeCount++;
                }
            }
            id = QString("Node_%1").arg(nodeCount);
            type = GraphNode::Node;
        }

        createNode(id, event->scenePos(), type);
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

// 辅助函数：从ID中提取数字部分
int GraphScene::extractNumberId(const QString& id) {
    // ID格式为 "类型_数字"，如 "Router_0", "Node_1"
    QStringList parts = id.split('_');
    if (parts.size() >= 2) {
        return parts[1].toInt();
    }
    return 0; // 默认值
}

// 导出图信息
void GraphScene::exportGraph(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return;

    QTextStream out(&file);

    // 存储每个路由器的连接信息
    QMap<int, QList<QPair<QString, double>>> routerConnections; // 路由器ID -> 连接列表(类型+ID, 权重)

    // 遍历所有边，构建连接关系
    for (GraphEdge* edge : m_edges) {
        GraphNode* start = edge->startNode();
        GraphNode* end = edge->endNode();
        double weight = edge->weight();

        // 提取数字ID
        int startNum = extractNumberId(start->getId());
        int endNum = extractNumberId(end->getId());

        // 确定连接类型和方向
        if (start->getType() == GraphNode::Router && end->getType() == GraphNode::Router) {
            // 只记录一次：按数字大小
            if (startNum < endNum) {
                QString conn = "router " + QString::number(endNum);
                routerConnections[startNum].append(qMakePair(conn, weight));
            } else if (endNum < startNum) {
                QString conn = "router " + QString::number(startNum);
                routerConnections[endNum].append(qMakePair(conn, weight));
            }
        } else if (start->getType() == GraphNode::Router && end->getType() == GraphNode::Node) {
            // 路由器到节点的连接
            QString conn = "node " + QString::number(endNum);
            routerConnections[startNum].append(qMakePair(conn, weight));
        } else if (start->getType() == GraphNode::Node && end->getType() == GraphNode::Router) {
            // 节点到路由器的连接
            QString conn = "node " + QString::number(startNum);
            routerConnections[endNum].append(qMakePair(conn, weight));
        }
    }

    // 按路由器ID排序输出
    QList<int> routerIds = routerConnections.keys();
    std::sort(routerIds.begin(), routerIds.end());

    for (int routerId : routerIds) {
        out << "router " << routerId;

        // 获取该路由器的所有连接
        const auto& connections = routerConnections[routerId];

        // 输出所有连接（可以按需要排序）
        for (const auto& connection : connections) {
            out << " " << connection.first;
            if (connection.second != 1.0) {
                out << " " << connection.second;
            } else {
                // do nothing
                // out << " "; // 或者如果权重为1可以省略，根据您的需求
            }
        }
        out << "\n";
    }

    file.close();
}
