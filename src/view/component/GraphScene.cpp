#include "GraphScene.h"
#include "RouterConfigDialog.h"
#include "RouterGlobalConfigDialog.h"
#include "utils/CanvasDebugLog.h"
#include <QBrush>
#include <QFile>
#include <QGraphicsLineItem>
#include <QGraphicsSceneMouseEvent>
#include <QMap>
#include <QMimeData>
#include <QPen>
#include <QPointer>
#include <QTextStream>
#include <QTimer>
#include <QVector>

GraphScene::GraphScene(QObject* parent)
    : ElaGraphicsScene(parent) {
    setSceneRect(QRectF(0, 0, 1200, 600));
    // (x, y ,sceneX, sceneY) (x, y) 控制画布左上角点 (sceneX, sceneY) 控制宽高
}

void GraphScene::setPlaceTool(PlaceTool tool) {
    m_placeTool = tool;
}

QString GraphScene::allocateNextNodeId(GraphNode::NodeType type) const {
    if (type == GraphNode::Router) {
        int routerCount = 0;
        for (GraphNode* node : m_nodes) {
            if (node->getType() == GraphNode::Router) {
                ++routerCount;
            }
        }
        return QStringLiteral("Router_%1").arg(routerCount);
    }
    int nodeCount = 0;
    for (GraphNode* node : m_nodes) {
        if (node->getType() == GraphNode::Node) {
            ++nodeCount;
        }
    }
    return QStringLiteral("Node_%1").arg(nodeCount);
}

GraphNode* GraphScene::createNode(const QString& id, const QPointF& pos, GraphNode::NodeType type) {
    auto* node = new GraphNode(id, type);
    addItem(node);
    node->setPos(pos); // NOTE: this line must be after addItem(node); `addItem` has written `setPos`
    m_nodes.append(node);

    // 连接节点的配置请求信号到场景
    connect(node, &GraphNode::configureRequested, this, &GraphScene::nodeConfigureRequested);

    connect(node, &GraphNode::deleteRequested, this, [this](GraphNode* n) {
        if (!n) {
            return;
        }
        canvasDebugLog(QStringLiteral("GraphScene.cpp: deleteRequested slot id=%1 n=0x%2")
                           .arg(n->getId())
                           .arg(quintptr(n), 0, 16));
        const QPointer<GraphNode> guard(n);
        QTimer::singleShot(0, this, [this, guard]() {
            canvasDebugLog(QStringLiteral("GraphScene.cpp: singleShot before removeNode guard=0x%1")
                               .arg(quintptr(guard.data()), 0, 16));
            if (guard) {
                removeNode(guard.data());
            }
        });
    });

    return node;
}

void GraphScene::removeNode(GraphNode* node) {
    canvasDebugLog(QStringLiteral("GraphScene::removeNode [enter] ptr=0x%1 edges=%2")
                       .arg(quintptr(node), 0, 16)
                       .arg(m_edges.size()));
    if (!node) {
        return;
    }

    if (m_highlightNode == node) {
        m_highlightNode = nullptr;
    }
    m_routerConfigs.remove(node->getId());

    QVector<GraphEdge*> incident;
    incident.reserve(m_edges.size());
    for (GraphEdge* e : m_edges) {
        if (e && (e->startNode() == node || e->endNode() == node)) {
            incident.append(e);
        }
    }
    canvasDebugLog(QStringLiteral("GraphScene::removeNode [before incident loop] count=%1 id=%2")
                       .arg(incident.size())
                       .arg(node->getId()));
    for (GraphEdge* e : incident) {
        canvasDebugLog(QStringLiteral("GraphScene::removeNode [foreach] removeEdge e=0x%1")
                           .arg(quintptr(e), 0, 16));
        removeEdge(e);
    }
    // ElaGraphicsScene::removeItem(ElaGraphicsItem*) 内部已 delete item，禁止再 delete/deleteLater
    canvasDebugLog(QStringLiteral("GraphScene::removeNode [before m_nodes.removeAll]"));
    m_nodes.removeAll(node);
    canvasDebugLog(QStringLiteral("GraphScene::removeNode [call ElaGraphicsScene::removeItem node] "
                                  "(Ela 实现会 delete 节点)"));
    removeItem(node);
    canvasDebugLog(QStringLiteral("GraphScene::removeNode [return] ptr invalid"));
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

    edge->setWeight(weight);
    edge->updatePosition();

    // 让边随节点移动（节点发出 posChanged）
    connect(start, &GraphNode::posChanged, edge, &GraphEdge::updatePosition);
    connect(end, &GraphNode::posChanged, edge, &GraphEdge::updatePosition);

    m_edges.append(edge);
    return edge;
}

void GraphScene::removeEdge(GraphEdge* edge) {
    canvasDebugLog(QStringLiteral("GraphScene::removeEdge [enter] ptr=0x%1 listSize=%2")
                       .arg(quintptr(edge), 0, 16)
                       .arg(m_edges.size()));
    if (!edge) {
        return;
    }
    GraphNode* s = edge->startNode();
    GraphNode* eN = edge->endNode();
    if (s) {
        disconnect(s, &GraphNode::posChanged, edge, &GraphEdge::updatePosition);
    }
    if (eN) {
        disconnect(eN, &GraphNode::posChanged, edge, &GraphEdge::updatePosition);
    }
    canvasDebugLog(
        QStringLiteral("GraphScene::removeEdge [m_edges.removeAll before Ela removeItem]"));
    m_edges.removeAll(edge);
    canvasDebugLog(QStringLiteral(
        "GraphScene::removeEdge [call ElaGraphicsScene::removeItem edge] (Ela 实现会 delete 边)"));
    removeItem(edge);
    canvasDebugLog(QStringLiteral("GraphScene::removeEdge [return] ptr invalid"));
}

void GraphScene::setAllEdgeWeightsVisible(bool visible) {
    for (GraphEdge* edge : m_edges) {
        edge->setWeightVisible(visible);
    }
}

bool GraphScene::isConnectionValid(GraphNode* start, GraphNode* end) {
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
        const GraphNode::NodeType type = (m_pendingToolName == QStringLiteral("Router"))
                                             ? GraphNode::Router
                                             : GraphNode::Node;
        const QString id = allocateNextNodeId(type);
        createNode(id, event->scenePos(), type);
        m_pendingToolName.clear();
        event->acceptProposedAction();
    }
}

void GraphScene::mousePressEvent(QGraphicsSceneMouseEvent* event) {
    if (event->button() == Qt::LeftButton && m_placeTool != PlaceTool::None) {
        QGraphicsItem* top = itemAt(event->scenePos(), QTransform());
        if (top == nullptr) {
            if (m_highlightNode) {
                m_highlightNode->setNodeState(GraphNode::Normal);
                m_highlightNode = nullptr;
            }
            const GraphNode::NodeType nt = (m_placeTool == PlaceTool::Router) ? GraphNode::Router
                                                                              : GraphNode::Node;
            const QString nid = allocateNextNodeId(nt);
            createNode(nid, event->scenePos(), nt);
            event->accept();
            return;
        }
    }

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
            // ElaGraphicsScene::removeItem 会 delete 图形项
            removeItem(m_tempEdge);
        }
        m_tempEdge = nullptr;
        m_lineStartNode = nullptr;
    }
    ElaGraphicsScene::mouseReleaseEvent(event);
}

// 辅助函数：从ID中提取数字部分
int GraphScene::extractNumberId(const QString& id) {
    QStringList parts = id.split('_');
    if (parts.size() >= 2) {
        return parts[1].toInt();
    }
    return 0;
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

        int startNum = extractNumberId(start->getId());
        int endNum = extractNumberId(end->getId());

        if (start->getType() == GraphNode::Router && end->getType() == GraphNode::Router) {
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

        for (const auto& connection : connections) {
            out << " " << connection.first;
            if (connection.second != 1.0) {
                out << " " << connection.second;
            } else {
                // do nothing
                // out << " "; // 或者如果权重为1可以省略
            }
        }
        out << "\n";
    }

    file.close();
}

// 获取和设置路由器独立配置
QMap<QString, QString> GraphScene::getRouterConfig(const QString& routerId) const {
    if (m_routerConfigs.contains(routerId)) {
        return m_routerConfigs[routerId];
    }
    return {};
}

void GraphScene::setRouterConfig(const QString& routerId, const QMap<QString, QString>& config) {
    m_routerConfigs[routerId] = config;
}

namespace {

[[nodiscard]] QString escapeJsonString(const QString& value) {
    QString o;
    o.reserve(value.size() + 8);
    for (QChar c : value) {
        if (c == u'\\') {
            o += QStringLiteral("\\\\");
        } else if (c == u'"') {
            o += QStringLiteral("\\\"");
        } else {
            o += c;
        }
    }
    return o;
}

} // namespace

// 导出JSON配置
void GraphScene::exportJSONConfig(const QString& filePath, const QString& networkFileOverride) {
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return;

    QTextStream out(&file);

    out << "{\n";

    QMap<QString, QString> globalConfigToExport = m_globalConfig;
    if (globalConfigToExport.isEmpty()) {
        globalConfigToExport = RouterGlobalConfigDialog::getDefaultConfig();
    }
    if (!networkFileOverride.isEmpty()) {
        globalConfigToExport.insert(QStringLiteral("network_file"), networkFileOverride);
    }

    bool first = true;
    for (auto it = globalConfigToExport.begin(); it != globalConfigToExport.end(); ++it) {
        if (!first) {
            out << ",\n";
        }
        first = false;
        const QString& value = it.value();
        bool okInt = false;
        value.toInt(&okInt);
        bool okDouble = false;
        if (!okInt) {
            value.toDouble(&okDouble);
        }

        if (okInt || okDouble) {
            out << "  \"" << it.key() << "\": " << value;
        } else {
            out << "  \"" << it.key() << "\": \"" << escapeJsonString(value) << "\"";
        }
    }

    // 导出路由器特定配置
    // 收集所有路由器节点
    QMap<int, QString> routerIds; // 数字ID -> 完整ID
    for (GraphNode* node : m_nodes) {
        if (node->getType() == GraphNode::Router) {
            QString nodeId = node->getId();
            int numericId = extractNumberId(nodeId);
            routerIds[numericId] = nodeId;
        }
    }

    // 如果有路由器，导出配置
    if (!routerIds.isEmpty()) {
        out << ",\n\n  \"routers\": {\n";
        bool firstRouter = true;
        for (auto it = routerIds.begin(); it != routerIds.end(); ++it) {
            if (!firstRouter)
                out << ",\n";
            firstRouter = false;

            // 获取路由器配置（如果有），否则使用默认配置
            QMap<QString, QString> routerConfig = m_routerConfigs.value(it.value());
            if (routerConfig.isEmpty()) {
                routerConfig = RouterConfigDialog::getDefaultConfig();
            }

            out << "    \"" << it.key() << "\": {\n";

            bool firstParam = true;
            for (auto paramIt = routerConfig.begin(); paramIt != routerConfig.end(); ++paramIt) {
                if (!firstParam)
                    out << ",\n";
                firstParam = false;

                const QString& value = paramIt.value();
                bool isNumber = false;
                value.toInt(&isNumber);
                bool isDouble = false;
                if (!isNumber) {
                    value.toDouble(&isDouble);
                }

                if (isNumber || isDouble) {
                    out << "      \"" << paramIt.key() << "\": " << value;
                } else {
                    out << "      \"" << paramIt.key() << "\": \"" << escapeJsonString(value)
                        << "\"";
                }
            }

            out << "\n    }";
        }
        out << "\n  }\n";
    } else {
        out << "\n";
    }

    out << "}\n";
    file.close();
}
