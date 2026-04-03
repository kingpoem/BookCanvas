#include "GraphScene.h"
#include "GraphTopologyBlock.h"
#include "RouterConfigDialog.h"
#include "RouterGlobalConfigDialog.h"
#include "utils/CanvasDebugLog.h"
#include <QBrush>
#include <QFile>
#include <QGraphicsLineItem>
#include <QGraphicsSceneMouseEvent>
#include <QInputDialog>
#include <QMap>
#include <QMessageBox>
#include <QMimeData>
#include <QPen>
#include <QPointer>
#include <QSet>
#include <QTextStream>
#include <QTimer>
#include <QVector>
#include <cmath>

namespace {

constexpr int kMeshMaxRouters = 400;
constexpr int kMeshMaxTerminals = 4000;

[[nodiscard]] bool safePowInt(int base, int exp, int maxValue, int& out) {
    if (exp < 0 || base < 0) {
        return false;
    }
    long long v = 1;
    for (int i = 0; i < exp; ++i) {
        v *= base;
        if (v > maxValue) {
            return false;
        }
    }
    out = static_cast<int>(v);
    return true;
}

[[nodiscard]] QVector<int> decodeRouterCoords(int routerIndex, int k, int n) {
    QVector<int> coords;
    coords.resize(n);
    int x = routerIndex;
    for (int d = 0; d < n; ++d) {
        coords[d] = x % k;
        x /= k;
    }
    return coords;
}

[[nodiscard]] int encodeRouterCoords(const QVector<int>& coords, int k) {
    int idx = 0;
    int mul = 1;
    for (int d = 0; d < coords.size(); ++d) {
        idx += coords[d] * mul;
        mul *= k;
    }
    return idx;
}

[[nodiscard]] int planeIndexForCoords(const QVector<int>& coords, int k) {
    if (coords.size() <= 2) {
        return 0;
    }
    int idx = 0;
    int mul = 1;
    for (int d = 2; d < coords.size(); ++d) {
        idx += coords[d] * mul;
        mul *= k;
    }
    return idx;
}

[[nodiscard]] quint64 undirectedEdgeKey(int a, int b) {
    const int lo = qMin(a, b);
    const int hi = qMax(a, b);
    return (static_cast<quint64>(static_cast<quint32>(lo)) << 32) | static_cast<quint32>(hi);
}

} // namespace

GraphScene::GraphScene(QObject* parent)
    : ElaGraphicsScene(parent) {
    setSceneRect(QRectF(0, 0, 1200, 600));
    // (x, y ,sceneX, sceneY) (x, y) 控制画布左上角点 (sceneX, sceneY) 控制宽高
}

void GraphScene::setPlaceTool(PlaceTool tool) {
    m_placeTool = tool;
    if (tool != PlaceTool::TopologyBlock) {
        m_pendingBooksimTopologyActive = false;
    }
}

void GraphScene::beginPlaceBooksimTopology(const BooksimTopologyParams& params) {
    m_pendingBooksimTopology = params;
    m_pendingBooksimTopologyActive = true;
    setPlaceTool(PlaceTool::TopologyBlock);
}

void GraphScene::clearBooksimTopologyPlacePending() {
    m_pendingBooksimTopologyActive = false;
    if (m_placeTool == PlaceTool::TopologyBlock) {
        m_placeTool = PlaceTool::None;
    }
}

void GraphScene::updateTopologyBlockParams(GraphTopologyBlock* block,
                                           const BooksimTopologyParams& params) {
    if (!block) {
        return;
    }
    block->setParams(params);
    if (params.topologyId == QLatin1String("mesh") || params.topologyId == QLatin1String("torus")
        || params.topologyId == QLatin1String("cmesh") || params.topologyId == QLatin1String("fly")
        || params.topologyId == QLatin1String("qtree")
        || params.topologyId == QLatin1String("tree4")
        || params.topologyId == QLatin1String("fattree")
        || params.topologyId == QLatin1String("flatfly")
        || params.topologyId == QLatin1String("dragonflynew")) {
        rebuildManagedTopology(block);
    }
}

QString GraphScene::allocateNextTopologyBlockId() const {
    return QStringLiteral("TopoBlock_%1").arg(m_topologyBlocks.size());
}

void GraphScene::createTopologyBlockAt(const QPointF& pos) {
    const QString bid = allocateNextTopologyBlockId();
    auto* block = new GraphTopologyBlock(bid, m_pendingBooksimTopology);
    addItem(block);
    block->setPos(pos);
    m_topologyBlocks.append(block);

    if (m_pendingBooksimTopology.topologyId == QLatin1String("mesh")
        || m_pendingBooksimTopology.topologyId == QLatin1String("torus")
        || m_pendingBooksimTopology.topologyId == QLatin1String("cmesh")
        || m_pendingBooksimTopology.topologyId == QLatin1String("fly")
        || m_pendingBooksimTopology.topologyId == QLatin1String("qtree")
        || m_pendingBooksimTopology.topologyId == QLatin1String("tree4")
        || m_pendingBooksimTopology.topologyId == QLatin1String("fattree")
        || m_pendingBooksimTopology.topologyId == QLatin1String("flatfly")
        || m_pendingBooksimTopology.topologyId == QLatin1String("dragonflynew")) {
        ManagedTopologyState st;
        st.params = m_pendingBooksimTopology;
        m_managedTopologies.insert(block, st);
        rebuildManagedTopology(block);
    }

    connect(block, &GraphTopologyBlock::configureRequested, this, [this](GraphTopologyBlock* b) {
        emit topologyBlockConfigureRequested(b);
    });
    connect(block, &GraphTopologyBlock::deleteRequested, this, [this](GraphTopologyBlock* b) {
        if (!b) {
            return;
        }
        const QPointer<GraphTopologyBlock> guard(b);
        QTimer::singleShot(0, this, [this, guard]() {
            if (guard) {
                removeTopologyBlock(guard.data());
            }
        });
    });
}

void GraphScene::removeTopologyBlock(GraphTopologyBlock* block) {
    if (!block) {
        return;
    }
    clearManagedTopology(block);
    m_managedTopologies.remove(block);
    m_topologyBlocks.removeAll(block);
    removeItem(block);
}

void GraphScene::clearManagedTopology(GraphTopologyBlock* block) {
    auto it = m_managedTopologies.find(block);
    if (it == m_managedTopologies.end()) {
        return;
    }
    ManagedTopologyState& st = it.value();
    const QList<QPointer<GraphEdge>> edges = st.edges;
    for (const QPointer<GraphEdge>& e : edges) {
        if (e) {
            removeEdge(e.data());
        }
    }
    st.edges.clear();

    const QList<QPointer<GraphNode>> terminals = st.terminals;
    for (const QPointer<GraphNode>& n : terminals) {
        if (n) {
            removeNode(n.data());
        }
    }
    st.terminals.clear();

    const QList<QPointer<GraphNode>> routers = st.routers;
    for (const QPointer<GraphNode>& r : routers) {
        if (r) {
            removeNode(r.data());
        }
    }
    st.routers.clear();
}

void GraphScene::rebuildManagedTopology(GraphTopologyBlock* block) {
    auto it = m_managedTopologies.find(block);
    if (it == m_managedTopologies.end()) {
        return;
    }
    ManagedTopologyState& st = it.value();
    st.params = block->params();
    clearManagedTopology(block);

    const QString topologyId = st.params.topologyId.trimmed().toLower();
    const int k = qMax(2, st.params.k);
    const int n = qMax(1, st.params.n);
    const int c = qMax(1, st.params.c);
    const bool isTorus = (topologyId == QLatin1String("torus"));
    const bool isCMesh = (topologyId == QLatin1String("cmesh"));
    const bool isFly = (topologyId == QLatin1String("fly"));
    const bool isQTree = (topologyId == QLatin1String("qtree"));
    const bool isTree4 = (topologyId == QLatin1String("tree4"));
    const bool isFatTree = (topologyId == QLatin1String("fattree"));
    const bool isFlatFly = (topologyId == QLatin1String("flatfly"));
    const bool isDragonfly = (topologyId == QLatin1String("dragonflynew"));
    const QPointF base = block->pos() + QPointF(0.0, 120.0);

    auto addTerminalAt = [this, &st](GraphNode* router, const QPointF& pos) {
        if (!router) {
            return;
        }
        GraphNode* node = createNode(allocateNextNodeId(GraphNode::Node), pos, GraphNode::Node);
        st.terminals.append(node);
        GraphEdge* e = createEdge(node, router, 1.0);
        if (e) {
            st.edges.append(e);
        }
    };

    if (isFly) {
        const int flyK = qMax(2, k);
        const int flyN = qMax(1, n);
        int perStage = 0;
        if (!safePowInt(flyK, qMax(0, flyN - 1), kMeshMaxRouters, perStage)) {
            QMessageBox::warning(nullptr,
                                 tr("Fly 规模过大"),
                                 tr("当前参数生成的 fly 规模过大，请减小 k / n 后重试。"));
            return;
        }
        const int routerCount = flyN * perStage;
        if (routerCount > kMeshMaxRouters) {
            QMessageBox::warning(nullptr,
                                 tr("Fly 规模过大"),
                                 tr("当前参数生成的路由器数量过大，请减小 k / n 后重试。"));
            return;
        }
        int terminalCount = 0;
        if (!safePowInt(flyK, flyN, kMeshMaxTerminals, terminalCount)) {
            QMessageBox::warning(nullptr,
                                 tr("Fly 规模过大"),
                                 tr("当前参数生成的终端数量过大，请减小 k / n 后重试。"));
            return;
        }
        QVector<GraphNode*> routers;
        routers.reserve(routerCount);
        const qreal stageStepX = 180.0;
        const qreal rowStepY = 80.0;
        for (int stage = 0; stage < flyN; ++stage) {
            for (int addr = 0; addr < perStage; ++addr) {
                const QPointF p(base.x() + stage * stageStepX, base.y() + addr * rowStepY);
                GraphNode* router = createNode(allocateNextNodeId(GraphNode::Router),
                                               p,
                                               GraphNode::Router);
                routers.append(router);
                st.routers.append(router);
            }
        }

        QSet<quint64> createdRouterEdges;
        for (int stage = 0; stage < flyN - 1; ++stage) {
            int shift = 1;
            if (!safePowInt(flyK, qMax(0, flyN - stage - 2), kMeshMaxRouters, shift)) {
                shift = 1;
            }
            for (int addr = 0; addr < perStage; ++addr) {
                const int zeroDigit = (addr / shift) % flyK;
                for (int port = 0; port < flyK; ++port) {
                    const int inAddr = addr - zeroDigit * shift + port * shift;
                    const int lhs = stage * perStage + inAddr;
                    const int rhs = (stage + 1) * perStage + addr;
                    const quint64 edgeKey = undirectedEdgeKey(lhs, rhs);
                    if (createdRouterEdges.contains(edgeKey)) {
                        continue;
                    }
                    GraphEdge* e = createEdge(routers[lhs], routers[rhs], 1.0);
                    if (e) {
                        createdRouterEdges.insert(edgeKey);
                        st.edges.append(e);
                    }
                }
            }
        }

        for (int addr = 0; addr < perStage; ++addr) {
            GraphNode* router = routers[(flyN - 1) * perStage + addr];
            const QPointF rPos = router->pos();
            for (int port = 0; port < flyK; ++port) {
                const qreal x = rPos.x() - ((flyK - 1) * 14.0 / 2.0) + port * 14.0;
                const qreal y = rPos.y() + 66.0;
                addTerminalAt(router, QPointF(x, y));
            }
        }
        return;
    }

    if (isQTree || isTree4) {
        const int treeK = 4;
        const int treeN = 3;
        QVector<QVector<GraphNode*>> levels;
        if (isQTree) {
            levels.resize(treeN);
            for (int h = 0; h < treeN; ++h) {
                int count = 1;
                if (!safePowInt(treeK, h, kMeshMaxRouters, count)) {
                    count = 1;
                }
                levels[h].reserve(count);
                const qreal step = 78.0;
                const qreal y = base.y() + h * 120.0;
                const qreal x0 = base.x() - ((count - 1) * step / 2.0) + 220.0;
                for (int i = 0; i < count; ++i) {
                    GraphNode* router = createNode(allocateNextNodeId(GraphNode::Router),
                                                   QPointF(x0 + i * step, y),
                                                   GraphNode::Router);
                    levels[h].append(router);
                    st.routers.append(router);
                }
            }
            QSet<quint64> createdRouterEdges;
            for (int h = 0; h < treeN - 1; ++h) {
                for (int p = 0; p < levels[h].size(); ++p) {
                    for (int child = 0; child < treeK; ++child) {
                        const int childPos = p * treeK + child;
                        if (childPos < 0 || childPos >= levels[h + 1].size()) {
                            continue;
                        }
                        const int a = static_cast<int>(st.routers.indexOf(levels[h][p]));
                        const int b = static_cast<int>(st.routers.indexOf(levels[h + 1][childPos]));
                        const quint64 edgeKey = undirectedEdgeKey(a, b);
                        if (createdRouterEdges.contains(edgeKey)) {
                            continue;
                        }
                        GraphEdge* e = createEdge(levels[h][p], levels[h + 1][childPos], 1.0);
                        if (e) {
                            createdRouterEdges.insert(edgeKey);
                            st.edges.append(e);
                        }
                    }
                }
            }
            for (GraphNode* leafRouter : levels[2]) {
                const QPointF rPos = leafRouter->pos();
                for (int i = 0; i < treeK; ++i) {
                    addTerminalAt(leafRouter, QPointF(rPos.x() - 22.0 + i * 14.0, rPos.y() + 66.0));
                }
            }
        } else {
            levels = QVector<QVector<GraphNode*>>{QVector<GraphNode*>(),
                                                  QVector<GraphNode*>(),
                                                  QVector<GraphNode*>()};
            const QVector<int> counts{4, 8, 16};
            for (int h = 0; h < counts.size(); ++h) {
                const int count = counts[h];
                const qreal step = (h == 0) ? 120.0 : ((h == 1) ? 86.0 : 66.0);
                const qreal y = base.y() + h * 120.0;
                const qreal x0 = base.x() - ((count - 1) * step / 2.0) + 220.0;
                for (int i = 0; i < count; ++i) {
                    GraphNode* router = createNode(allocateNextNodeId(GraphNode::Router),
                                                   QPointF(x0 + i * step, y),
                                                   GraphNode::Router);
                    levels[h].append(router);
                    st.routers.append(router);
                }
            }
            QSet<quint64> createdRouterEdges;
            for (int pos = 0; pos < 8; ++pos) {
                for (int port = 0; port < 4; ++port) {
                    const int child = 4 * (pos / 2) + port;
                    const int a = static_cast<int>(st.routers.indexOf(levels[1][pos]));
                    const int b = static_cast<int>(st.routers.indexOf(levels[2][child]));
                    const quint64 edgeKey = undirectedEdgeKey(a, b);
                    if (createdRouterEdges.contains(edgeKey)) {
                        continue;
                    }
                    GraphEdge* e = createEdge(levels[1][pos], levels[2][child], 1.0);
                    if (e) {
                        createdRouterEdges.insert(edgeKey);
                        st.edges.append(e);
                    }
                }
            }
            for (int p0 = 0; p0 < 4; ++p0) {
                for (int p1 = 0; p1 < 8; ++p1) {
                    const int a = static_cast<int>(st.routers.indexOf(levels[0][p0]));
                    const int b = static_cast<int>(st.routers.indexOf(levels[1][p1]));
                    const quint64 edgeKey = undirectedEdgeKey(a, b);
                    if (createdRouterEdges.contains(edgeKey)) {
                        continue;
                    }
                    GraphEdge* e = createEdge(levels[0][p0], levels[1][p1], 1.0);
                    if (e) {
                        createdRouterEdges.insert(edgeKey);
                        st.edges.append(e);
                    }
                }
            }
            for (GraphNode* leafRouter : levels[2]) {
                const QPointF rPos = leafRouter->pos();
                for (int i = 0; i < treeK; ++i) {
                    addTerminalAt(leafRouter, QPointF(rPos.x() - 22.0 + i * 14.0, rPos.y() + 66.0));
                }
            }
        }
        return;
    }

    if (isFatTree) {
        const int fatK = qMax(2, k);
        const int fatN = qMax(2, n);
        int nPos = 0;
        if (!safePowInt(fatK, qMax(0, fatN - 1), kMeshMaxRouters, nPos)) {
            QMessageBox::warning(nullptr,
                                 tr("FatTree 规模过大"),
                                 tr("当前参数生成的 FatTree 规模过大，请减小 k / n 后重试。"));
            return;
        }
        const int routerCount = fatN * nPos;
        if (routerCount > kMeshMaxRouters) {
            QMessageBox::warning(nullptr,
                                 tr("FatTree 规模过大"),
                                 tr("当前参数生成的路由器数量过大，请减小 k / n 后重试。"));
            return;
        }
        int terminalCount = 0;
        if (!safePowInt(fatK, fatN, kMeshMaxTerminals, terminalCount)) {
            QMessageBox::warning(nullptr,
                                 tr("FatTree 规模过大"),
                                 tr("当前参数生成的终端数量过大，请减小 k / n 后重试。"));
            return;
        }

        QVector<QVector<GraphNode*>> levels(fatN);
        const int cols = qMax(1, static_cast<int>(std::ceil(std::sqrt(static_cast<double>(nPos)))));
        const qreal colStep = 88.0;
        const qreal rowStep = 78.0;
        const qreal levelGapX = 220.0;
        for (int level = 0; level < fatN; ++level) {
            levels[level].reserve(nPos);
            const qreal x = base.x() + level * levelGapX;
            for (int pos = 0; pos < nPos; ++pos) {
                const int row = pos / cols;
                const int col = pos % cols;
                const qreal y = base.y() + row * rowStep + col * 0.2;
                GraphNode* router = createNode(allocateNextNodeId(GraphNode::Router),
                                               QPointF(x + col * colStep, y),
                                               GraphNode::Router);
                levels[level].append(router);
                st.routers.append(router);
            }
        }

        QSet<quint64> createdRouterEdges;
        int routersPerNeighborhood = nPos;
        for (int level = 0; level < fatN - 1; ++level) {
            const int routersPerBranch = qMax(1, routersPerNeighborhood / fatK);
            for (int pos = 0; pos < nPos; ++pos) {
                const int neighborhood = pos / routersPerNeighborhood;
                const int neighborhoodPos = pos % routersPerNeighborhood;
                for (int port = 0; port < fatK; ++port) {
                    const int childPos = neighborhood * routersPerNeighborhood
                                         + port * routersPerBranch
                                         + (neighborhoodPos % routersPerBranch);
                    if (childPos < 0 || childPos >= nPos) {
                        continue;
                    }
                    const int a = level * nPos + pos;
                    const int b = (level + 1) * nPos + childPos;
                    const quint64 edgeKey = undirectedEdgeKey(a, b);
                    if (createdRouterEdges.contains(edgeKey)) {
                        continue;
                    }
                    GraphEdge* e = createEdge(levels[level][pos], levels[level + 1][childPos], 1.0);
                    if (e) {
                        createdRouterEdges.insert(edgeKey);
                        st.edges.append(e);
                    }
                }
            }
            routersPerNeighborhood = routersPerBranch;
        }

        const int leafLevel = fatN - 1;
        for (int pos = 0; pos < levels[leafLevel].size(); ++pos) {
            GraphNode* leafRouter = levels[leafLevel][pos];
            const QPointF rPos = leafRouter->pos();
            for (int i = 0; i < fatK; ++i) {
                addTerminalAt(leafRouter,
                              QPointF(rPos.x() - ((fatK - 1) * 14.0 / 2.0) + i * 14.0,
                                      rPos.y() + 66.0));
            }
        }
        return;
    }

    if (isFlatFly) {
        const int ffK = qMax(2, k);
        const int ffN = qMax(1, n);
        const int ffC = qMax(1, c);
        int routerCount = 0;
        if (!safePowInt(ffK, ffN, kMeshMaxRouters, routerCount)) {
            QMessageBox::warning(nullptr,
                                 tr("FlatFly 规模过大"),
                                 tr("当前参数生成的 FlatFly 规模过大，请减小 k / n 后重试。"));
            return;
        }
        if (routerCount * ffC > kMeshMaxTerminals) {
            QMessageBox::warning(nullptr,
                                 tr("FlatFly 规模过大"),
                                 tr("当前参数会生成 %1 个终端，超过当前支持上限 %2。")
                                     .arg(routerCount * ffC)
                                     .arg(kMeshMaxTerminals));
            return;
        }

        int planeCount = 1;
        if (!safePowInt(ffK, qMax(0, ffN - 2), kMeshMaxRouters, planeCount)) {
            planeCount = 1;
        }
        const int planeCols = qMax(1,
                                   static_cast<int>(
                                       std::ceil(std::sqrt(static_cast<double>(planeCount)))));
        const qreal routerStepX = 105.0;
        const qreal routerStepY = 85.0;
        const qreal planeGapX = 130.0;
        const qreal planeGapY = 120.0;
        const qreal layerSpanX = (ffK - 1) * routerStepX + planeGapX;
        const qreal layerSpanY = ((ffN >= 2 ? (ffK - 1) : 0) * routerStepY) + planeGapY;

        QVector<GraphNode*> routers;
        routers.resize(routerCount);
        for (int idx = 0; idx < routerCount; ++idx) {
            const QVector<int> coords = decodeRouterCoords(idx, ffK, ffN);
            const int plane = planeIndexForCoords(coords, ffK);
            const int planeCol = plane % planeCols;
            const int planeRow = plane / planeCols;
            const qreal x = base.x() + planeCol * layerSpanX + coords[0] * routerStepX;
            const qreal y = base.y() + planeRow * layerSpanY
                            + ((ffN >= 2 ? coords[1] : 0) * routerStepY);
            GraphNode* router = createNode(allocateNextNodeId(GraphNode::Router),
                                           QPointF(x, y),
                                           GraphNode::Router);
            routers[idx] = router;
            st.routers.append(router);
        }

        QSet<quint64> createdRouterEdges;
        for (int idx = 0; idx < routerCount; ++idx) {
            const QVector<int> coords = decodeRouterCoords(idx, ffK, ffN);
            for (int d = 0; d < ffN; ++d) {
                for (int v = 0; v < ffK; ++v) {
                    if (v == coords[d]) {
                        continue;
                    }
                    QVector<int> next = coords;
                    next[d] = v;
                    const int nb = encodeRouterCoords(next, ffK);
                    const quint64 edgeKey = undirectedEdgeKey(idx, nb);
                    if (createdRouterEdges.contains(edgeKey)) {
                        continue;
                    }
                    GraphEdge* e = createEdge(routers[idx], routers[nb], 1.0);
                    if (e) {
                        createdRouterEdges.insert(edgeKey);
                        st.edges.append(e);
                    }
                }
            }
        }

        for (int idx = 0; idx < routerCount; ++idx) {
            GraphNode* router = routers[idx];
            const QPointF rPos = router->pos();
            for (int i = 0; i < ffC; ++i) {
                addTerminalAt(router,
                              QPointF(rPos.x() - ((ffC - 1) * 14.0 / 2.0) + i * 14.0,
                                      rPos.y() + 64.0));
            }
        }
        return;
    }

    if (isDragonfly) {
        const int p = qMax(2, k);
        const int a = 2 * p;
        const int g = a * p + 1;
        const int routerCount = a * g;
        if (routerCount > kMeshMaxRouters) {
            QMessageBox::warning(nullptr,
                                 tr("Dragonfly 规模过大"),
                                 tr("当前参数会生成 %1 个路由器，超过当前支持上限 %2。\n"
                                    "请减小 k 后重试。")
                                     .arg(routerCount)
                                     .arg(kMeshMaxRouters));
            return;
        }
        if (routerCount * p > kMeshMaxTerminals) {
            QMessageBox::warning(nullptr,
                                 tr("Dragonfly 规模过大"),
                                 tr("当前参数会生成 %1 个终端，超过当前支持上限 %2。\n"
                                    "请减小 k 后重试。")
                                     .arg(routerCount * p)
                                     .arg(kMeshMaxTerminals));
            return;
        }

        const int groupCols = qMax(1,
                                   static_cast<int>(std::ceil(std::sqrt(static_cast<double>(g)))));
        const int aCols = qMax(1, static_cast<int>(std::ceil(std::sqrt(static_cast<double>(a)))));
        const qreal groupStepX = 310.0;
        const qreal groupStepY = 240.0;
        const qreal routerStepX = 52.0;
        const qreal routerStepY = 52.0;

        QVector<GraphNode*> routers;
        routers.resize(routerCount);
        for (int grp = 0; grp < g; ++grp) {
            const int grpCol = grp % groupCols;
            const int grpRow = grp / groupCols;
            const QPointF gBase(base.x() + grpCol * groupStepX, base.y() + grpRow * groupStepY);
            for (int r = 0; r < a; ++r) {
                const int idx = grp * a + r;
                const int rc = r % aCols;
                const int rr = r / aCols;
                GraphNode* router = createNode(allocateNextNodeId(GraphNode::Router),
                                               QPointF(gBase.x() + rc * routerStepX,
                                                       gBase.y() + rr * routerStepY),
                                               GraphNode::Router);
                routers[idx] = router;
                st.routers.append(router);
            }
        }

        QSet<quint64> createdRouterEdges;
        for (int grp = 0; grp < g; ++grp) {
            const int begin = grp * a;
            for (int i = 0; i < a; ++i) {
                for (int j = i + 1; j < a; ++j) {
                    const int lhs = begin + i;
                    const int rhs = begin + j;
                    const quint64 edgeKey = undirectedEdgeKey(lhs, rhs);
                    if (createdRouterEdges.contains(edgeKey)) {
                        continue;
                    }
                    GraphEdge* e = createEdge(routers[lhs], routers[rhs], 1.0);
                    if (e) {
                        createdRouterEdges.insert(edgeKey);
                        st.edges.append(e);
                    }
                }
            }
        }

        for (int srcGrp = 0; srcGrp < g; ++srcGrp) {
            for (int dstGrp = srcGrp + 1; dstGrp < g; ++dstGrp) {
                const int srcLocal = (dstGrp > srcGrp) ? ((dstGrp - 1) / p) : (dstGrp / p);
                const int dstLocal = (srcGrp > dstGrp) ? ((srcGrp - 1) / p) : (srcGrp / p);
                const int lhs = srcGrp * a + qBound(0, srcLocal, a - 1);
                const int rhs = dstGrp * a + qBound(0, dstLocal, a - 1);
                const quint64 edgeKey = undirectedEdgeKey(lhs, rhs);
                if (createdRouterEdges.contains(edgeKey)) {
                    continue;
                }
                GraphEdge* e = createEdge(routers[lhs], routers[rhs], 1.0);
                if (e) {
                    createdRouterEdges.insert(edgeKey);
                    st.edges.append(e);
                }
            }
        }

        for (int idx = 0; idx < routerCount; ++idx) {
            GraphNode* router = routers[idx];
            const QPointF rPos = router->pos();
            for (int i = 0; i < p; ++i) {
                addTerminalAt(router,
                              QPointF(rPos.x() - ((p - 1) * 13.0 / 2.0) + i * 13.0,
                                      rPos.y() + 64.0));
            }
        }
        return;
    }

    const QString baseTopoName = isCMesh ? tr("CMesh") : (isTorus ? tr("Torus") : tr("Mesh"));
    const int layoutN = isCMesh ? 2 : n;

    int routerCount = 0;
    if (!safePowInt(k, layoutN, kMeshMaxRouters, routerCount)) {
        QMessageBox::warning(nullptr,
                             tr("%1 规模过大").arg(baseTopoName),
                             tr("当前参数 k=%1, n=%2 生成的路由器数量过大。\n"
                                "为保证画布可用性，当前最多支持 %3 个路由器。")
                                 .arg(k)
                                 .arg(layoutN)
                                 .arg(kMeshMaxRouters));
        return;
    }
    if (routerCount * c > kMeshMaxTerminals) {
        QMessageBox::warning(nullptr,
                             tr("%1 规模过大").arg(baseTopoName),
                             tr("当前参数会生成 %1 个终端，超过当前支持上限 %2。\n"
                                "请减小 k / n / c 后重试。")
                                 .arg(routerCount * c)
                                 .arg(kMeshMaxTerminals));
        return;
    }

    int planeCount = 1;
    if (!safePowInt(k, qMax(0, layoutN - 2), kMeshMaxRouters, planeCount)) {
        planeCount = 1;
    }
    const int planeCols = qMax(1,
                               static_cast<int>(
                                   std::ceil(std::sqrt(static_cast<double>(planeCount)))));

    const qreal routerStepX = 110.0;
    const qreal routerStepY = 90.0;
    const qreal planeGapX = 120.0;
    const qreal planeGapY = 120.0;
    const qreal layerSpanX = (k - 1) * routerStepX + planeGapX;
    const qreal layerSpanY = ((layoutN >= 2 ? (k - 1) : 0) * routerStepY) + planeGapY;

    QVector<GraphNode*> routers;
    routers.resize(routerCount);

    for (int idx = 0; idx < routerCount; ++idx) {
        const QVector<int> coords = decodeRouterCoords(idx, k, layoutN);
        const int plane = planeIndexForCoords(coords, k);
        const int planeCol = plane % planeCols;
        const int planeRow = plane / planeCols;
        const qreal x = base.x() + planeCol * layerSpanX + coords[0] * routerStepX;
        const qreal y = base.y() + planeRow * layerSpanY
                        + ((layoutN >= 2 ? coords[1] : 0) * routerStepY);
        GraphNode* router = createNode(allocateNextNodeId(GraphNode::Router),
                                       QPointF(x, y),
                                       GraphNode::Router);
        routers[idx] = router;
        st.routers.append(router);
    }

    QSet<quint64> createdRouterEdges;
    for (int idx = 0; idx < routerCount; ++idx) {
        const QVector<int> coords = decodeRouterCoords(idx, k, layoutN);
        for (int d = 0; d < layoutN; ++d) {
            QVector<int> next = coords;
            if (isTorus) {
                next[d] = (coords[d] + 1) % k;
            } else {
                if (coords[d] + 1 >= k) {
                    continue;
                }
                next[d] += 1;
            }
            const int nb = encodeRouterCoords(next, k);
            const quint64 edgeKey = undirectedEdgeKey(idx, nb);
            if (createdRouterEdges.contains(edgeKey)) {
                continue;
            }
            GraphEdge* e = createEdge(routers[idx], routers[nb], 1.0);
            if (e) {
                createdRouterEdges.insert(edgeKey);
                st.edges.append(e);
            }
        }
    }
    if (isCMesh && layoutN == 2) {
        // cmesh 在边界上存在 express 连接，这里用可视化连边表达该结构特性。
        const int half = qMax(1, k / 2);
        for (int idx = 0; idx < routerCount; ++idx) {
            const QVector<int> coords = decodeRouterCoords(idx, k, layoutN);
            const int x = coords[0];
            const int y = coords[1];
            QVector<int> dst = coords;
            bool hasExpress = false;
            if (x == 0 || x == (k - 1)) {
                dst[1] = (y + half) % k;
                hasExpress = true;
            } else if (y == 0 || y == (k - 1)) {
                dst[0] = (x + half) % k;
                hasExpress = true;
            }
            if (!hasExpress) {
                continue;
            }
            const int nb = encodeRouterCoords(dst, k);
            const quint64 edgeKey = undirectedEdgeKey(idx, nb);
            if (createdRouterEdges.contains(edgeKey)) {
                continue;
            }
            GraphEdge* e = createEdge(routers[idx], routers[nb], 1.0);
            if (e) {
                createdRouterEdges.insert(edgeKey);
                st.edges.append(e);
            }
        }
    }

    for (int idx = 0; idx < routerCount; ++idx) {
        GraphNode* router = routers[idx];
        const QPointF rPos = router->pos();
        for (int i = 0; i < c; ++i) {
            const qreal x = rPos.x() - ((c - 1) * 18.0 / 2.0) + i * 18.0;
            const qreal y = rPos.y() + 68.0;
            GraphNode* node = createNode(allocateNextNodeId(GraphNode::Node),
                                         QPointF(x, y),
                                         GraphNode::Node);
            st.terminals.append(node);
            GraphEdge* e = createEdge(node, router, 1.0);
            if (e) {
                st.edges.append(e);
            }
        }
    }
}

QString GraphScene::allocateNextNodeId(GraphNode::NodeType type) const {
    QSet<int> used;
    for (GraphNode* node : m_nodes) {
        if (node->getType() == type) {
            used.insert(extractNumberId(node->getId()));
        }
    }
    int k = 0;
    while (used.contains(k)) {
        ++k;
    }
    if (type == GraphNode::Router) {
        return QStringLiteral("Router_%1").arg(k);
    }
    return QStringLiteral("Node_%1").arg(k);
}

QString GraphScene::canonicalIdFromUserInput(GraphNode::NodeType type, const QString& raw) {
    QString s = raw.trimmed();
    if (s.isEmpty()) {
        return {};
    }
    if (type == GraphNode::Node) {
        if (s.startsWith(QLatin1String("Node_"), Qt::CaseInsensitive)) {
            s = s.mid(5);
        } else if (s.startsWith(u'T', Qt::CaseInsensitive)) {
            s = s.mid(1);
        }
        bool ok = false;
        const int n = s.toInt(&ok);
        if (!ok || n < 0) {
            return {};
        }
        return QStringLiteral("Node_%1").arg(n);
    }
    if (s.startsWith(QLatin1String("Router_"), Qt::CaseInsensitive)) {
        s = s.mid(7);
    } else if (s.startsWith(u'R', Qt::CaseInsensitive)) {
        s = s.mid(1);
    }
    bool ok = false;
    const int n = s.toInt(&ok);
    if (!ok || n < 0) {
        return {};
    }
    return QStringLiteral("Router_%1").arg(n);
}

bool GraphScene::renameNodeToId(GraphNode* node, const QString& newId) {
    if (!node || newId.isEmpty()) {
        return false;
    }
    for (GraphNode* n : m_nodes) {
        if (n != node && n->getId() == newId) {
            return false;
        }
    }
    const QString oldId = node->getId();
    if (oldId == newId) {
        return true;
    }
    if (node->getType() == GraphNode::Router && m_routerConfigs.contains(oldId)) {
        const auto cfg = m_routerConfigs.take(oldId);
        m_routerConfigs.insert(newId, cfg);
    }
    node->setGraphId(newId);
    return true;
}

void GraphScene::promptRenameNode(GraphNode* node) {
    if (!node) {
        return;
    }
    const bool isRouter = node->getType() == GraphNode::Router;
    QString suggest;
    const QString id = node->getId();
    if (isRouter && id.startsWith(QLatin1String("Router_"))) {
        suggest = id.mid(7);
    } else if (!isRouter && id.startsWith(QLatin1String("Node_"))) {
        suggest = id.mid(5);
    } else {
        suggest = id;
    }

    bool ok = false;
    const QString text
        = QInputDialog::getText(nullptr,
                                tr("重命名节点"),
                                isRouter ? tr("路由器编号（非负整数，或 R0 / Router_0）：")
                                         : tr("终端编号（非负整数，或 T0 / Node_0）："),
                                QLineEdit::Normal,
                                suggest,
                                &ok);
    if (!ok) {
        return;
    }
    const QString newId = canonicalIdFromUserInput(node->getType(), text);
    if (newId.isEmpty()) {
        QMessageBox::warning(nullptr,
                             tr("无效编号"),
                             tr("请输入非负整数，或使用 T0、Node_1、R0、Router_1 等形式。"));
        return;
    }
    if (!renameNodeToId(node, newId)) {
        QMessageBox::warning(nullptr, tr("重命名失败"), tr("该编号已被其他节点使用。"));
    }
}

GraphNode* GraphScene::createNode(const QString& id, const QPointF& pos, GraphNode::NodeType type) {
    auto* node = new GraphNode(id, type);
    addItem(node);
    node->setPos(pos); // NOTE: this line must be after addItem(node); `addItem` has written `setPos`
    m_nodes.append(node);

    // 连接节点的配置请求信号到场景
    connect(node, &GraphNode::configureRequested, this, &GraphScene::nodeConfigureRequested);

    connect(node, &GraphNode::renameRequested, this, [this](GraphNode* n) { promptRenameNode(n); });

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
    for (auto it = m_managedTopologies.begin(); it != m_managedTopologies.end(); ++it) {
        auto& routers = it.value().routers;
        for (qsizetype i = routers.size() - 1; i >= 0; --i) {
            if (!routers[i] || routers[i].data() == node) {
                routers.removeAt(i);
            }
        }
        auto& terminals = it.value().terminals;
        for (qsizetype i = terminals.size() - 1; i >= 0; --i) {
            if (!terminals[i] || terminals[i].data() == node) {
                terminals.removeAt(i);
            }
        }
    }
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
    for (auto it = m_managedTopologies.begin(); it != m_managedTopologies.end(); ++it) {
        auto& edges = it.value().edges;
        for (qsizetype i = edges.size() - 1; i >= 0; --i) {
            if (!edges[i] || edges[i].data() == edge) {
                edges.removeAt(i);
            }
        }
    }
    canvasDebugLog(QStringLiteral("GraphScene::removeEdge [return] ptr invalid"));
}

void GraphScene::setAllEdgeWeightsVisible(bool visible) {
    for (GraphEdge* edge : m_edges) {
        edge->setWeightVisible(visible);
    }
}

void GraphScene::clearAllContent() {
    m_highlightNode = nullptr;
    m_lineStartNode = nullptr;
    if (m_tempEdge) {
        removeItem(m_tempEdge);
        m_tempEdge = nullptr;
    }
    m_pendingToolName.clear();
    m_pendingBooksimTopologyActive = false;
    m_placeTool = PlaceTool::None;

    const QList<GraphTopologyBlock*> blocks = m_topologyBlocks;
    for (GraphTopologyBlock* block : blocks) {
        if (block) {
            removeTopologyBlock(block);
        }
    }

    const QList<GraphEdge*> allEdges = m_edges;
    for (GraphEdge* edge : allEdges) {
        if (edge) {
            removeEdge(edge);
        }
    }

    const QList<GraphNode*> allNodes = m_nodes;
    for (GraphNode* node : allNodes) {
        if (node) {
            removeNode(node);
        }
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
        if (m_placeTool == PlaceTool::TopologyBlock && m_pendingBooksimTopologyActive
            && top == nullptr) {
            if (m_highlightNode) {
                m_highlightNode->setNodeState(GraphNode::Normal);
                m_highlightNode = nullptr;
            }
            createTopologyBlockAt(event->scenePos());
            m_pendingBooksimTopologyActive = false;
            m_placeTool = PlaceTool::None;
            event->accept();
            return;
        }
        if ((m_placeTool == PlaceTool::Terminal || m_placeTool == PlaceTool::Router)
            && top == nullptr) {
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
        const QPointF from = m_lineStartNode->connectionAnchorToward(event->scenePos());
        m_tempEdge->setLine(QLineF(from, event->scenePos()));
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

    if (m_topologyBlocks.size() == 1) {
        const BooksimTopologyParams p = m_topologyBlocks.first()->params();
        const bool isCMesh = (p.topologyId == QLatin1String("cmesh"));
        const bool isQTree = (p.topologyId == QLatin1String("qtree"));
        const bool isTree4 = (p.topologyId == QLatin1String("tree4"));
        const bool isFlatFly = (p.topologyId == QLatin1String("flatfly"));
        const bool isDragonfly = (p.topologyId == QLatin1String("dragonflynew"));
        const bool isFixedTree = (isQTree || isTree4);
        int exportC = isCMesh ? 4 : p.c;
        int exportN = isCMesh ? 2 : (isFixedTree ? 3 : p.n);
        if (isDragonfly) {
            exportN = 1;
        }
        globalConfigToExport.insert(QStringLiteral("topology"), p.topologyId);
        globalConfigToExport.insert(QStringLiteral("k"), QString::number(isFixedTree ? 4 : p.k));
        globalConfigToExport.insert(QStringLiteral("n"), QString::number(exportN));
        globalConfigToExport.insert(QStringLiteral("c"), QString::number(exportC));
        globalConfigToExport.insert(QStringLiteral("routing_function"), p.routingFunction);
        if (isCMesh) {
            globalConfigToExport.insert(QStringLiteral("x"), QString::number(p.k));
            globalConfigToExport.insert(QStringLiteral("y"), QString::number(p.k));
            globalConfigToExport.insert(QStringLiteral("xr"), QStringLiteral("2"));
            globalConfigToExport.insert(QStringLiteral("yr"), QStringLiteral("2"));
        } else if (isFlatFly) {
            int xr = static_cast<int>(std::sqrt(static_cast<double>(qMax(1, exportC))));
            if (xr * xr != exportC) {
                xr = 1;
                exportC = 1;
                globalConfigToExport.insert(QStringLiteral("c"), QString::number(exportC));
            }
            globalConfigToExport.insert(QStringLiteral("x"), QString::number(p.k));
            globalConfigToExport.insert(QStringLiteral("y"), QString::number(p.k));
            globalConfigToExport.insert(QStringLiteral("xr"), QString::number(xr));
            globalConfigToExport.insert(QStringLiteral("yr"), QString::number(xr));
        }
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
