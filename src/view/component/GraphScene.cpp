#include "GraphScene.h"
#include "ChipletSimConfigDialogs.h"
#include "GraphChiplet.h"
#include "GraphTopologyBlock.h"
#include "RouterConfigDialog.h"
#include "RouterGlobalConfigDialog.h"
#include "utils/CanvasDebugLog.h"
#include "utils/ThemedInputDialog.h"
#include <QBrush>
#include <QDialog>
#include <QFile>
#include <QGraphicsLineItem>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsView>
#include <QMap>
#include <QMessageBox>
#include <QMimeData>
#include <QPen>
#include <QPointer>
#include <QRegularExpression>
#include <QSet>
#include <QTextStream>
#include <QTimer>
#include <QUndoCommand>
#include <QVector>
#include <cmath>
#include <limits>

namespace {

[[nodiscard]] QWidget* graphSceneWindowParent(GraphScene* scene) {
    if (!scene || scene->views().isEmpty()) {
        return nullptr;
    }
    return scene->views().first()->window();
}

[[nodiscard]] bool nodeIsTerminalOrRouter(const GraphNode* n) {
    return n && (n->getType() == GraphNode::Node || n->getType() == GraphNode::Router);
}

constexpr int kPowIntMax = std::numeric_limits<int>::max();

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

class BoolResetGuard {
public:
    explicit BoolResetGuard(bool& flag)
        : m_flag(flag)
        , m_old(flag) {}
    ~BoolResetGuard() { m_flag = m_old; }

private:
    bool& m_flag;
    bool m_old = false;
};

} // namespace

class GraphScene::AddNodeCommand final : public QUndoCommand {
public:
    AddNodeCommand(GraphScene* scene, QString id, QPointF pos, GraphNode::NodeType type)
        : m_scene(scene)
        , m_id(std::move(id))
        , m_pos(pos)
        , m_type(type) {
        setText(QObject::tr("添加节点"));
    }

    void redo() override {
        if (!m_scene) {
            return;
        }
        m_scene->createNodeInternal(m_id, m_pos, m_type);
    }

    void undo() override {
        if (!m_scene) {
            return;
        }
        if (GraphNode* node = m_scene->findNodeById(m_id)) {
            m_scene->removeNodeInternal(node);
        }
    }

private:
    QPointer<GraphScene> m_scene;
    QString m_id;
    QPointF m_pos;
    GraphNode::NodeType m_type = GraphNode::Node;
};

class GraphScene::AddEdgeCommand final : public QUndoCommand {
public:
    AddEdgeCommand(GraphScene* scene, QString startId, QString endId, double weight, bool chipletRed)
        : m_scene(scene)
        , m_startId(std::move(startId))
        , m_endId(std::move(endId))
        , m_weight(weight)
        , m_chipletRed(chipletRed) {
        setText(QObject::tr("添加连线"));
    }

    void redo() override {
        if (!m_scene) {
            return;
        }
        GraphNode* start = m_scene->findNodeById(m_startId);
        GraphNode* end = m_scene->findNodeById(m_endId);
        if (!start || !end) {
            return;
        }
        m_scene->createEdgeInternal(start, end, m_weight, m_chipletRed);
    }

    void undo() override {
        if (!m_scene) {
            return;
        }
        for (GraphEdge* edge : m_scene->m_edges) {
            if (!edge) {
                continue;
            }
            if (edge->startNode() && edge->endNode() && edge->startNode()->getId() == m_startId
                && edge->endNode()->getId() == m_endId && qFuzzyCompare(edge->weight(), m_weight)
                && edge->isChipletRedInterconnect() == m_chipletRed) {
                m_scene->removeEdgeInternal(edge);
                return;
            }
        }
    }

private:
    QPointer<GraphScene> m_scene;
    QString m_startId;
    QString m_endId;
    double m_weight = 1.0;
    bool m_chipletRed = false;
};

class GraphScene::RemoveEdgeCommand final : public QUndoCommand {
public:
    RemoveEdgeCommand(
        GraphScene* scene, QString startId, QString endId, double weight, bool chipletRed)
        : m_scene(scene)
        , m_startId(std::move(startId))
        , m_endId(std::move(endId))
        , m_weight(weight)
        , m_chipletRed(chipletRed) {
        setText(QObject::tr("删除连线"));
    }

    void redo() override {
        if (!m_scene) {
            return;
        }
        for (GraphEdge* edge : m_scene->m_edges) {
            if (!edge || !edge->startNode() || !edge->endNode()) {
                continue;
            }
            if (edge->startNode()->getId() == m_startId && edge->endNode()->getId() == m_endId
                && qFuzzyCompare(edge->weight(), m_weight)
                && edge->isChipletRedInterconnect() == m_chipletRed) {
                m_scene->removeEdgeInternal(edge);
                return;
            }
        }
    }

    void undo() override {
        if (!m_scene) {
            return;
        }
        GraphNode* start = m_scene->findNodeById(m_startId);
        GraphNode* end = m_scene->findNodeById(m_endId);
        if (!start || !end) {
            return;
        }
        m_scene->createEdgeInternal(start, end, m_weight, m_chipletRed);
    }

private:
    QPointer<GraphScene> m_scene;
    QString m_startId;
    QString m_endId;
    double m_weight = 1.0;
    bool m_chipletRed = false;
};

class GraphScene::RemoveNodeCommand final : public QUndoCommand {
public:
    struct EdgeSnapshot {
        QString startId;
        QString endId;
        double weight = 1.0;
        bool chipletRedInterconnect = false;
    };

    struct ChipletMembershipSnap {
        QString chipletId;
        QString label;
        QStringList members;
        int gridCx = 0;
        int gridCy = 0;
        int dieK = 1;
        QString dieIntra;
        QString dieClockPeriod;
        QString dieClockPhase;
    };

    RemoveNodeCommand(GraphScene* scene, GraphNode* node)
        : m_scene(scene) {
        if (!node) {
            return;
        }
        m_nodeId = node->getId();
        m_nodeType = node->getType();
        m_nodePos = node->pos();
        if (m_nodeType == GraphNode::Router) {
            m_routerConfig = scene->getRouterConfig(m_nodeId);
        }
        for (GraphEdge* edge : scene->m_edges) {
            if (!edge || !edge->startNode() || !edge->endNode()) {
                continue;
            }
            if (edge->startNode() == node || edge->endNode() == node) {
                m_incidentEdges.push_back({edge->startNode()->getId(),
                                           edge->endNode()->getId(),
                                           edge->weight(),
                                           edge->isChipletRedInterconnect()});
            }
        }
        for (GraphChiplet* c : scene->m_chiplets) {
            if (c && c->hasMember(m_nodeId)) {
                ChipletMembershipSnap s;
                s.chipletId = c->chipletId();
                s.label = c->label();
                s.members = c->memberIdsSorted();
                s.gridCx = c->gridCx();
                s.gridCy = c->gridCy();
                s.dieK = c->dieK();
                s.dieIntra = c->dieIntraLatencyText();
                s.dieClockPeriod = c->dieClockPeriodText();
                s.dieClockPhase = c->dieClockPhaseText();
                m_chipletSnaps.push_back(std::move(s));
            }
        }
        setText(QObject::tr("删除节点"));
    }

    void redo() override {
        if (!m_scene) {
            return;
        }
        if (GraphNode* node = m_scene->findNodeById(m_nodeId)) {
            m_scene->removeNodeInternal(node);
        }
    }

    void undo() override {
        if (!m_scene) {
            return;
        }
        GraphNode* node = m_scene->createNodeInternal(m_nodeId, m_nodePos, m_nodeType);
        if (!node) {
            return;
        }
        if (m_nodeType == GraphNode::Router && !m_routerConfig.isEmpty()) {
            m_scene->setRouterConfig(m_nodeId, m_routerConfig);
        }
        for (const EdgeSnapshot& edge : m_incidentEdges) {
            GraphNode* start = m_scene->findNodeById(edge.startId);
            GraphNode* end = m_scene->findNodeById(edge.endId);
            if (!start || !end) {
                continue;
            }
            m_scene->createEdgeInternal(start, end, edge.weight, edge.chipletRedInterconnect);
        }
        for (const ChipletMembershipSnap& s : m_chipletSnaps) {
            GraphChiplet* c = m_scene->findChipletById(s.chipletId);
            if (c) {
                c->setMembers(s.members);
                c->setGridCx(s.gridCx);
                c->setGridCy(s.gridCy);
                c->setDieK(s.dieK);
                c->setDieIntraLatencyText(s.dieIntra);
                c->setDieClockPeriodText(s.dieClockPeriod);
                c->setDieClockPhaseText(s.dieClockPhase);
                m_scene->layoutChipletAroundMembers(c);
            } else if (GraphChiplet* n = m_scene->createChipletInternal(s.chipletId,
                                                                        s.label,
                                                                        s.members,
                                                                        false)) {
                n->setGridCx(s.gridCx);
                n->setGridCy(s.gridCy);
                n->setDieK(s.dieK);
                n->setDieIntraLatencyText(s.dieIntra);
                n->setDieClockPeriodText(s.dieClockPeriod);
                n->setDieClockPhaseText(s.dieClockPhase);
                m_scene->layoutChipletAroundMembers(n);
            }
        }
        m_scene->refreshAllEdgesChipletDecoration();
    }

private:
    QPointer<GraphScene> m_scene;
    QString m_nodeId;
    GraphNode::NodeType m_nodeType = GraphNode::Node;
    QPointF m_nodePos;
    QMap<QString, QString> m_routerConfig;
    QVector<EdgeSnapshot> m_incidentEdges;
    QVector<ChipletMembershipSnap> m_chipletSnaps;
};

class GraphScene::MoveNodeCommand final : public QUndoCommand {
public:
    MoveNodeCommand(GraphScene* scene, QString nodeId, QPointF from, QPointF to)
        : m_scene(scene)
        , m_nodeId(std::move(nodeId))
        , m_from(from)
        , m_to(to) {
        setText(QObject::tr("移动节点"));
    }

    void redo() override {
        if (!m_scene) {
            return;
        }
        if (GraphNode* node = m_scene->findNodeById(m_nodeId)) {
            node->setPos(m_to);
        }
    }

    void undo() override {
        if (!m_scene) {
            return;
        }
        if (GraphNode* node = m_scene->findNodeById(m_nodeId)) {
            node->setPos(m_from);
        }
    }

    int id() const override { return 0xB00CC1; }

    bool mergeWith(const QUndoCommand* other) override {
        auto* rhs = dynamic_cast<const MoveNodeCommand*>(other);
        if (!rhs || rhs->m_nodeId != m_nodeId) {
            return false;
        }
        m_to = rhs->m_to;
        return true;
    }

private:
    QPointer<GraphScene> m_scene;
    QString m_nodeId;
    QPointF m_from;
    QPointF m_to;
};

class GraphScene::UpdateTopologyParamsCommand final : public QUndoCommand {
public:
    UpdateTopologyParamsCommand(GraphScene* scene,
                                GraphTopologyBlock* block,
                                BooksimTopologyParams oldParams,
                                BooksimTopologyParams newParams)
        : m_scene(scene)
        , m_block(block)
        , m_oldParams(std::move(oldParams))
        , m_newParams(std::move(newParams)) {
        setText(QObject::tr("修改拓扑参数"));
    }

    void redo() override { apply(m_newParams); }

    void undo() override { apply(m_oldParams); }

private:
    void apply(const BooksimTopologyParams& params) {
        if (!m_scene || !m_block) {
            return;
        }
        m_block->setParams(params);
        m_scene->rebuildManagedTopology(m_block.data());
    }

    QPointer<GraphScene> m_scene;
    QPointer<GraphTopologyBlock> m_block;
    BooksimTopologyParams m_oldParams;
    BooksimTopologyParams m_newParams;
};

class GraphScene::AddChipletCommand final : public QUndoCommand {
public:
    AddChipletCommand(GraphScene* scene, QString id, QString label, QStringList members)
        : m_scene(scene)
        , m_id(std::move(id))
        , m_label(std::move(label))
        , m_members(std::move(members)) {
        setText(QObject::tr("创建芯粒"));
    }

    void redo() override {
        if (!m_scene || m_scene->findChipletById(m_id)) {
            return;
        }
        m_scene->createChipletInternal(m_id, m_label, m_members);
    }

    void undo() override {
        if (!m_scene) {
            return;
        }
        if (GraphChiplet* c = m_scene->findChipletById(m_id)) {
            m_scene->removeChipletInternal(c);
        }
    }

private:
    QPointer<GraphScene> m_scene;
    QString m_id;
    QString m_label;
    QStringList m_members;
};

class GraphScene::RemoveChipletCommand final : public QUndoCommand {
public:
    RemoveChipletCommand(GraphScene* scene, GraphChiplet* chiplet)
        : m_scene(scene) {
        if (chiplet) {
            m_id = chiplet->chipletId();
            m_label = chiplet->label();
            m_members = chiplet->memberIdsSorted();
            m_gridCx = chiplet->gridCx();
            m_gridCy = chiplet->gridCy();
            m_dieK = chiplet->dieK();
            m_dieIntra = chiplet->dieIntraLatencyText();
            m_dieClockPeriod = chiplet->dieClockPeriodText();
            m_dieClockPhase = chiplet->dieClockPhaseText();
        }
        setText(QObject::tr("删除芯粒"));
    }

    void redo() override {
        if (!m_scene) {
            return;
        }
        if (GraphChiplet* c = m_scene->findChipletById(m_id)) {
            m_scene->removeChipletInternal(c);
        }
    }

    void undo() override {
        if (!m_scene || m_scene->findChipletById(m_id)) {
            return;
        }
        if (GraphChiplet* c = m_scene->createChipletInternal(m_id, m_label, m_members, false)) {
            c->setGridCx(m_gridCx);
            c->setGridCy(m_gridCy);
            c->setDieK(m_dieK);
            c->setDieIntraLatencyText(m_dieIntra);
            c->setDieClockPeriodText(m_dieClockPeriod);
            c->setDieClockPhaseText(m_dieClockPhase);
            m_scene->layoutChipletAroundMembers(c);
            m_scene->refreshAllEdgesChipletDecoration();
        }
    }

private:
    QPointer<GraphScene> m_scene;
    QString m_id;
    QString m_label;
    QStringList m_members;
    int m_gridCx = 0;
    int m_gridCy = 0;
    int m_dieK = 1;
    QString m_dieIntra;
    QString m_dieClockPeriod;
    QString m_dieClockPhase;
};

GraphScene::GraphScene(QObject* parent)
    : ElaGraphicsScene(parent) {
    setSceneRect(QRectF(0, 0, 1200, 600));
    // (x, y ,sceneX, sceneY) (x, y) 控制画布左上角点 (sceneX, sceneY) 控制宽高
    connect(&m_undoStack, &QUndoStack::indexChanged, this, [this](int) { emitUndoStateNow(); });
    emitUndoStateNow();
}

void GraphScene::setPlaceTool(PlaceTool tool) {
    if (m_highlightNode) {
        m_highlightNode->setNodeState(GraphNode::Normal);
        m_highlightNode = nullptr;
    }
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
    const BooksimTopologyParams oldParams = block->params();
    if (isRecordingHistory()) {
        m_undoStack.push(new UpdateTopologyParamsCommand(this, block, oldParams, params));
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
    const bool oldSuspended = m_historySuspended;
    m_historySuspended = true;
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
    m_historySuspended = oldSuspended;
}

void GraphScene::rebuildManagedTopology(GraphTopologyBlock* block) {
    auto it = m_managedTopologies.find(block);
    if (it == m_managedTopologies.end()) {
        return;
    }
    BoolResetGuard historyRestore(m_historySuspended);
    m_historySuspended = true;
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

    // 自动生成时统一把终端摆在路由器斜右上方（场景坐标 y 向下为正）
    const qreal kRouterBox = 50.0;
    const qreal kTerminalBoxH = 50.0;
    const qreal kTermGapAboveRouter = 12.0;
    const qreal kTermGapRightOfRouter = 10.0;
    auto terminalPosUpperRightOfRouter =
        [&](const QPointF& rPos, int index, int count, qreal stepX) -> QPointF {
        const qreal y = rPos.y() - kTermGapAboveRouter - kTerminalBoxH;
        const qreal x0 = rPos.x() + kRouterBox + kTermGapRightOfRouter
                         - ((count - 1) * stepX / 2.0);
        return QPointF(x0 + index * stepX, y);
    };

    if (isFly) {
        const int flyK = qMax(2, k);
        const int flyN = qMax(1, n);
        int perStage = 0;
        if (!safePowInt(flyK, qMax(0, flyN - 1), kPowIntMax, perStage)) {
            QMessageBox::warning(nullptr,
                                 tr("Fly 规模过大"),
                                 tr("当前参数生成的 fly 规模过大，请减小 k / n 后重试。"));
            return;
        }
        const int routerCount = flyN * perStage;
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
            if (!safePowInt(flyK, qMax(0, flyN - stage - 2), kPowIntMax, shift)) {
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
                addTerminalAt(router, terminalPosUpperRightOfRouter(rPos, port, flyK, 14.0));
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
                if (!safePowInt(treeK, h, kPowIntMax, count)) {
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
                    addTerminalAt(leafRouter, terminalPosUpperRightOfRouter(rPos, i, treeK, 14.0));
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
                    addTerminalAt(leafRouter, terminalPosUpperRightOfRouter(rPos, i, treeK, 14.0));
                }
            }
        }
        return;
    }

    if (isFatTree) {
        const int fatK = qMax(2, k);
        const int fatN = qMax(2, n);
        int nPos = 0;
        if (!safePowInt(fatK, qMax(0, fatN - 1), kPowIntMax, nPos)) {
            QMessageBox::warning(nullptr,
                                 tr("FatTree 规模过大"),
                                 tr("当前参数生成的 FatTree 规模过大，请减小 k / n 后重试。"));
            return;
        }
        const int routerCount = fatN * nPos;

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
                addTerminalAt(leafRouter, terminalPosUpperRightOfRouter(rPos, i, fatK, 14.0));
            }
        }
        return;
    }

    if (isFlatFly) {
        const int ffK = qMax(2, k);
        const int ffN = qMax(1, n);
        const int ffC = qMax(1, c);
        int routerCount = 0;
        if (!safePowInt(ffK, ffN, kPowIntMax, routerCount)) {
            QMessageBox::warning(nullptr,
                                 tr("FlatFly 规模过大"),
                                 tr("当前参数生成的 FlatFly 规模过大，请减小 k / n 后重试。"));
            return;
        }

        int planeCount = 1;
        if (!safePowInt(ffK, qMax(0, ffN - 2), kPowIntMax, planeCount)) {
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
                addTerminalAt(router, terminalPosUpperRightOfRouter(rPos, i, ffC, 14.0));
            }
        }
        return;
    }

    if (isDragonfly) {
        const int p = qMax(2, k);
        const int a = 2 * p;
        const int g = a * p + 1;
        const int routerCount = a * g;

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
                addTerminalAt(router, terminalPosUpperRightOfRouter(rPos, i, p, 13.0));
            }
        }
        return;
    }

    const QString baseTopoName = isCMesh ? tr("CMesh") : (isTorus ? tr("Torus") : tr("Mesh"));
    const int layoutN = isCMesh ? 2 : n;

    int routerCount = 0;
    if (!safePowInt(k, layoutN, kPowIntMax, routerCount)) {
        QMessageBox::warning(nullptr,
                             tr("%1 规模过大").arg(baseTopoName),
                             tr("当前参数 k=%1, n=%2 生成的路由器数量过大。").arg(k).arg(layoutN));
        return;
    }

    int planeCount = 1;
    if (!safePowInt(k, qMax(0, layoutN - 2), kPowIntMax, planeCount)) {
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
            addTerminalAt(router, terminalPosUpperRightOfRouter(rPos, i, c, 18.0));
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
    for (GraphChiplet* c : m_chiplets) {
        if (c) {
            c->renameMemberId(oldId, newId);
        }
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

    QWidget* const dlgParent = graphSceneWindowParent(this);
    bool ok = false;
    const QString text
        = BookCanvasUi::promptLineText(dlgParent,
                                       tr("重命名节点"),
                                       isRouter ? tr("路由器编号（非负整数，或 R0 / Router_0）：")
                                                : tr("终端编号（非负整数，或 T0 / Node_0）："),
                                       suggest,
                                       &ok);
    if (!ok) {
        return;
    }
    const QString newId = canonicalIdFromUserInput(node->getType(), text);
    if (newId.isEmpty()) {
        QMessageBox::warning(dlgParent,
                             tr("无效编号"),
                             tr("请输入非负整数，或使用 T0、Node_1、R0、Router_1 等形式。"));
        return;
    }
    if (!renameNodeToId(node, newId)) {
        QMessageBox::warning(dlgParent, tr("重命名失败"), tr("该编号已被其他节点使用。"));
    }
}

GraphNode* GraphScene::createNodeInternal(const QString& id,
                                          const QPointF& pos,
                                          GraphNode::NodeType type) {
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

    QPointer<GraphNode> posGuard(node);
    connect(node, &GraphNode::posChanged, this, [this, posGuard](const QPointF&, const QPointF&) {
        if (!posGuard) {
            return;
        }
        refitChipletsContainingMember(posGuard->getId());
    });
    return node;
}

GraphNode* GraphScene::createNode(const QString& id, const QPointF& pos, GraphNode::NodeType type) {
    if (!isRecordingHistory()) {
        return createNodeInternal(id, pos, type);
    }
    m_undoStack.push(new AddNodeCommand(this, id, pos, type));
    return findNodeById(id);
}

void GraphScene::removeNodeInternal(GraphNode* node) {
    canvasDebugLog(QStringLiteral("GraphScene::removeNode [enter] ptr=0x%1 edges=%2")
                       .arg(quintptr(node), 0, 16)
                       .arg(m_edges.size()));
    if (!node) {
        return;
    }

    stripNodeFromChiplets(node->getId());

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

void GraphScene::removeNode(GraphNode* node) {
    if (!node) {
        return;
    }
    if (!isRecordingHistory()) {
        removeNodeInternal(node);
        return;
    }
    m_undoStack.push(new RemoveNodeCommand(this, node));
}

GraphEdge* GraphScene::createEdgeInternal(GraphNode* start,
                                          GraphNode* end,
                                          double weight,
                                          bool chipletRedInterconnect) {
    if (!start || !end) {
        return nullptr;
    }

    // 检查连接是否合法
    if (!isConnectionValid(start, end)) {
        qDebug() << "非法连接：节点只能连接路由器";
        return nullptr;
    }

    for (GraphEdge* ex : m_edges) {
        if (!ex || !ex->startNode() || !ex->endNode()) {
            continue;
        }
        const bool samePair = (ex->startNode() == start && ex->endNode() == end)
                              || (ex->startNode() == end && ex->endNode() == start);
        if (samePair && qFuzzyCompare(ex->weight(), weight)
            && ex->isChipletRedInterconnect() == chipletRedInterconnect) {
            return ex;
        }
    }

    auto* edge = new GraphEdge(start, end);
    addItem(edge);

    edge->setWeight(weight);
    edge->setChipletRedInterconnect(chipletRedInterconnect);
    edge->updatePosition();

    // 让边随节点移动（节点发出 posChanged）
    connect(start, &GraphNode::posChanged, edge, &GraphEdge::updatePosition);
    connect(end, &GraphNode::posChanged, edge, &GraphEdge::updatePosition);

    m_edges.append(edge);
    return edge;
}

bool GraphScene::canCreateChipletRedInterconnect(GraphNode* a, GraphNode* b) const {
    if (!chipletGroupingAllowed() || !a || !b) {
        return false;
    }
    if (a->getType() != GraphNode::Router || b->getType() != GraphNode::Router) {
        return false;
    }
    GraphChiplet* ca = findChipletContainingMember(a->getId());
    GraphChiplet* cb = findChipletContainingMember(b->getId());
    return ca && cb && ca != cb;
}

GraphEdge* GraphScene::createEdge(GraphNode* start,
                                  GraphNode* end,
                                  double weight,
                                  bool chipletRedInterconnect) {
    if (!start || !end) {
        return nullptr;
    }
    for (GraphEdge* ex : m_edges) {
        if (!ex || !ex->startNode() || !ex->endNode()) {
            continue;
        }
        const bool samePair = (ex->startNode() == start && ex->endNode() == end)
                              || (ex->startNode() == end && ex->endNode() == start);
        if (samePair && qFuzzyCompare(ex->weight(), weight)
            && ex->isChipletRedInterconnect() == chipletRedInterconnect) {
            return ex;
        }
    }
    if (!isRecordingHistory()) {
        return createEdgeInternal(start, end, weight, chipletRedInterconnect);
    }
    m_undoStack.push(
        new AddEdgeCommand(this, start->getId(), end->getId(), weight, chipletRedInterconnect));
    for (GraphEdge* edge : m_edges) {
        if (!edge || !edge->startNode() || !edge->endNode()) {
            continue;
        }
        if (edge->startNode()->getId() == start->getId() && edge->endNode()->getId() == end->getId()
            && qFuzzyCompare(edge->weight(), weight)
            && edge->isChipletRedInterconnect() == chipletRedInterconnect) {
            return edge;
        }
    }
    return nullptr;
}

void GraphScene::removeEdgeInternal(GraphEdge* edge) {
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

void GraphScene::removeEdge(GraphEdge* edge) {
    if (!edge || !edge->startNode() || !edge->endNode()) {
        return;
    }
    if (!isRecordingHistory()) {
        removeEdgeInternal(edge);
        return;
    }
    m_undoStack.push(new RemoveEdgeCommand(this,
                                           edge->startNode()->getId(),
                                           edge->endNode()->getId(),
                                           edge->weight(),
                                           edge->isChipletRedInterconnect()));
}

void GraphScene::setAllEdgeWeightsVisible(bool visible) {
    for (GraphEdge* edge : m_edges) {
        edge->setWeightVisible(visible);
    }
}

bool GraphScene::renumberAllNodesFromZero() {
    QList<GraphNode*> routers;
    QList<GraphNode*> terminals;
    routers.reserve(m_nodes.size());
    terminals.reserve(m_nodes.size());
    for (GraphNode* node : m_nodes) {
        if (!node) {
            continue;
        }
        if (node->getType() == GraphNode::Router) {
            routers.append(node);
        } else {
            terminals.append(node);
        }
    }

    auto byNumericId = [](GraphNode* lhs, GraphNode* rhs) {
        if (lhs == nullptr || rhs == nullptr) {
            return lhs < rhs;
        }
        const int lnum = extractNumberId(lhs->getId());
        const int rnum = extractNumberId(rhs->getId());
        if (lnum != rnum) {
            return lnum < rnum;
        }
        return lhs->getId() < rhs->getId();
    };
    std::sort(routers.begin(), routers.end(), byNumericId);
    std::sort(terminals.begin(), terminals.end(), byNumericId);

    auto renumberGroup = [this](const QList<GraphNode*>& group, const QString& prefix) {
        for (int i = 0; i < group.size(); ++i) {
            GraphNode* node = group[i];
            const QString tempId = QStringLiteral("__tmp_%1_%2__").arg(prefix).arg(i);
            if (!renameNodeToId(node, tempId)) {
                return false;
            }
        }
        for (int i = 0; i < group.size(); ++i) {
            GraphNode* node = group[i];
            const QString finalId = QStringLiteral("%1_%2").arg(prefix).arg(i);
            if (!renameNodeToId(node, finalId)) {
                return false;
            }
        }
        return true;
    };

    return renumberGroup(routers, QStringLiteral("Router"))
           && renumberGroup(terminals, QStringLiteral("Node"));
}

int GraphScene::removeUnconnectedNodes() {
    QSet<GraphNode*> connectedNodes;
    connectedNodes.reserve(m_edges.size() * 2);
    for (GraphEdge* edge : m_edges) {
        if (!edge) {
            continue;
        }
        if (GraphNode* start = edge->startNode()) {
            connectedNodes.insert(start);
        }
        if (GraphNode* end = edge->endNode()) {
            connectedNodes.insert(end);
        }
    }

    QList<GraphNode*> isolatedNodes;
    isolatedNodes.reserve(m_nodes.size());
    for (GraphNode* node : m_nodes) {
        if (!node) {
            continue;
        }
        if (!connectedNodes.contains(node)) {
            isolatedNodes.append(node);
        }
    }

    for (GraphNode* node : isolatedNodes) {
        removeNode(node);
    }
    return static_cast<int>(isolatedNodes.size());
}

void GraphScene::clearAllContent() {
    const bool recordMacro = isRecordingHistory();
    if (recordMacro) {
        m_undoStack.beginMacro(tr("清空画布"));
    }
    m_highlightNode = nullptr;
    m_lineStartNode = nullptr;
    if (m_tempEdge) {
        removeItem(m_tempEdge);
        m_tempEdge = nullptr;
    }
    m_pendingToolName.clear();
    m_pendingBooksimTopologyActive = false;
    m_placeTool = PlaceTool::None;
    m_chipletMeshConnect = QStringLiteral("x");

    const QList<GraphChiplet*> chiplets = m_chiplets;
    for (GraphChiplet* c : chiplets) {
        if (c) {
            removeChipletInternal(c);
        }
    }

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
    if (recordMacro) {
        m_undoStack.endMacro();
    }
}

void GraphScene::undo() {
    if (m_undoStack.canUndo()) {
        m_historySuspended = true;
        m_undoStack.undo();
        m_historySuspended = false;
    }
}

void GraphScene::redo() {
    if (m_undoStack.canRedo()) {
        m_historySuspended = true;
        m_undoStack.redo();
        m_historySuspended = false;
    }
}

bool GraphScene::canUndo() const {
    return m_undoStack.canUndo();
}

bool GraphScene::canRedo() const {
    return m_undoStack.canRedo();
}

QString GraphScene::undoText() const {
    return m_undoStack.undoText();
}

QString GraphScene::redoText() const {
    return m_undoStack.redoText();
}

QAction* GraphScene::createUndoAction(QObject* parent, const QString& prefix) {
    return m_undoStack.createUndoAction(parent, prefix);
}

QAction* GraphScene::createRedoAction(QObject* parent, const QString& prefix) {
    return m_undoStack.createRedoAction(parent, prefix);
}

GraphNode* GraphScene::findNodeById(const QString& id) const {
    for (GraphNode* node : m_nodes) {
        if (node && node->getId() == id) {
            return node;
        }
    }
    return nullptr;
}

QString GraphScene::allocateNextChipletId() const {
    QSet<QString> used;
    for (GraphChiplet* c : m_chiplets) {
        if (c) {
            used.insert(c->chipletId());
        }
    }
    for (int i = 0;; ++i) {
        const QString cand = QStringLiteral("Chiplet_%1").arg(i);
        if (!used.contains(cand)) {
            return cand;
        }
    }
}

GraphChiplet* GraphScene::findChipletById(const QString& id) const {
    for (GraphChiplet* c : m_chiplets) {
        if (c && c->chipletId() == id) {
            return c;
        }
    }
    return nullptr;
}

GraphChiplet* GraphScene::findChipletContainingMember(const QString& nodeId) const {
    if (nodeId.isEmpty()) {
        return nullptr;
    }
    for (GraphChiplet* c : m_chiplets) {
        if (c && c->hasMember(nodeId)) {
            return c;
        }
    }
    return nullptr;
}

void GraphScene::layoutChipletAroundMembers(GraphChiplet* chiplet) {
    if (!chiplet) {
        return;
    }
    QList<GraphNode*> nodes;
    for (const QString& mid : chiplet->memberIdsSorted()) {
        if (GraphNode* n = findNodeById(mid)) {
            nodes.append(n);
        }
    }
    chiplet->fitAroundNodes(nodes);
}

void GraphScene::refitChipletsContainingMember(const QString& nodeId) {
    for (GraphChiplet* c : m_chiplets) {
        if (c && c->hasMember(nodeId)) {
            layoutChipletAroundMembers(c);
        }
    }
}

void GraphScene::stripNodeFromChiplets(const QString& nodeId) {
    QList<GraphChiplet*> dead;
    for (GraphChiplet* c : m_chiplets) {
        if (!c || !c->hasMember(nodeId)) {
            continue;
        }
        c->removeMember(nodeId);
        if (c->liveTerminalRouterMemberCount() == 0) {
            dead.append(c);
        } else {
            layoutChipletAroundMembers(c);
        }
    }
    for (GraphChiplet* c : dead) {
        removeChipletInternal(c);
    }
    refreshAllEdgesChipletDecoration();
}

GraphChiplet* GraphScene::createChipletInternal(const QString& id,
                                                const QString& label,
                                                const QStringList& members,
                                                const bool assignAutoGrid) {
    auto* c = new GraphChiplet(id, label);
    addItem(c);
    c->setZValue(-120);
    for (const QString& mid : members) {
        if (mid.isEmpty()) {
            continue;
        }
        GraphNode* n = findNodeById(mid);
        if (!nodeIsTerminalOrRouter(n)) {
            continue;
        }
        c->addMember(mid);
    }
    m_chiplets.append(c);
    if (assignAutoGrid) {
        assignDefaultGridForChiplet(c);
        c->setDieK(qMax(1, defaultMeshKForChiplets()));
    }
    layoutChipletAroundMembers(c);
    wireChiplet(c);
    refreshAllEdgesChipletDecoration();
    emit chipletMeshGlobalConfigRequested();
    return c;
}

void GraphScene::removeChipletInternal(GraphChiplet* chiplet) {
    if (!chiplet) {
        return;
    }
    m_chiplets.removeAll(chiplet);
    removeItem(chiplet);
    refreshAllEdgesChipletDecoration();
}

void GraphScene::wireChiplet(GraphChiplet* chiplet) {
    if (!chiplet) {
        return;
    }
    connect(chiplet, &GraphChiplet::deleteRequested, this, [this](GraphChiplet* ch) {
        if (!ch) {
            return;
        }
        const QPointer<GraphChiplet> guard(ch);
        QTimer::singleShot(0, this, [this, guard]() {
            if (guard) {
                removeChiplet(guard.data());
            }
        });
    });
    connect(chiplet, &GraphChiplet::renameRequested, this, [this](GraphChiplet* ch) {
        promptRenameChiplet(ch);
    });
    connect(chiplet, &GraphChiplet::configureDieParamsRequested, this, [this](GraphChiplet* ch) {
        promptConfigureChipletDie(ch, graphSceneWindowParent(this));
    });
    connect(chiplet, &GraphChiplet::handleDragStarted, this, [this](GraphChiplet* c) {
        m_chipletHandleDragSource = c;
        m_chipletHandleDragStartPos.clear();
        if (!c) {
            return;
        }
        for (const QString& id : c->memberIdsSorted()) {
            if (GraphNode* n = findNodeById(id)) {
                m_chipletHandleDragStartPos.insert(id, n->pos());
            }
        }
    });
    connect(chiplet, &GraphChiplet::handleDragDelta, this, [this](GraphChiplet* c, QPointF d) {
        if (!c || d.isNull()) {
            return;
        }
        for (const QString& id : c->memberIdsSorted()) {
            if (GraphNode* n = findNodeById(id)) {
                n->setPos(n->pos() + d);
            }
        }
        layoutChipletAroundMembers(c);
    });
    connect(chiplet, &GraphChiplet::handleDragReleased, this, [this](GraphChiplet* ch) {
        if (!ch) {
            m_chipletHandleDragSource = nullptr;
            m_chipletHandleDragStartPos.clear();
            return;
        }
        const QMap<QString, QPointF> startPos = m_chipletHandleDragStartPos;
        m_chipletHandleDragSource = nullptr;
        m_chipletHandleDragStartPos.clear();

        if (!isRecordingHistory()) {
            return;
        }
        QVector<QString> ids;
        QVector<QPointF> froms;
        QVector<QPointF> tos;
        for (auto it = startPos.begin(); it != startPos.end(); ++it) {
            GraphNode* n = findNodeById(it.key());
            if (!n) {
                continue;
            }
            const QPointF from = it.value();
            const QPointF to = n->pos();
            if (!qFuzzyCompare(from.x() + 1.0, to.x() + 1.0)
                || !qFuzzyCompare(from.y() + 1.0, to.y() + 1.0)) {
                ids.append(it.key());
                froms.append(from);
                tos.append(to);
            }
        }
        if (ids.isEmpty()) {
            return;
        }
        m_undoStack.beginMacro(tr("移动芯粒"));
        for (int i = 0; i < ids.size(); ++i) {
            m_undoStack.push(new MoveNodeCommand(this, ids[i], froms[i], tos[i]));
        }
        m_undoStack.endMacro();
    });
}

void GraphScene::promptConfigureChipletDie(GraphChiplet* chiplet, QWidget* dialogParent) {
    if (!chiplet) {
        return;
    }
    QWidget* const p = dialogParent ? dialogParent : graphSceneWindowParent(this);
    ChipletDieParamsDialog dlg(this, chiplet, p);
    if (dlg.exec() == QDialog::Accepted) {
        dlg.applyToChiplet();
        layoutChipletAroundMembers(chiplet);
        refreshAllEdgesChipletDecoration();
    }
}

bool GraphScene::chipletGroupingAllowed() const {
    return m_globalConfig.value(QStringLiteral("topology")).trimmed().toLower()
           == QLatin1String("chiplet_mesh");
}

bool GraphScene::edgeConnectsDistinctChiplets(const GraphEdge* edge) const {
    if (!edge) {
        return false;
    }
    const GraphNode* a = edge->startNode();
    const GraphNode* b = edge->endNode();
    if (!a || !b) {
        return false;
    }
    if (a->getType() != GraphNode::Router || b->getType() != GraphNode::Router) {
        return false;
    }
    const GraphChiplet* ca = findChipletContainingMember(a->getId());
    const GraphChiplet* cb = findChipletContainingMember(b->getId());
    return ca && cb && ca != cb;
}

void GraphScene::assignDefaultGridForChiplet(GraphChiplet* chiplet) {
    if (!chiplet) {
        return;
    }
    QSet<QString> used;
    used.reserve(m_chiplets.size() * 2);
    auto key = [](int x, int y) { return QStringLiteral("%1,%2").arg(x).arg(y); };
    for (GraphChiplet* o : m_chiplets) {
        if (o && o != chiplet) {
            used.insert(key(o->gridCx(), o->gridCy()));
        }
    }
    int maxCx = 0;
    int maxCy = 0;
    for (GraphChiplet* o : m_chiplets) {
        if (o && o != chiplet) {
            maxCx = qMax(maxCx, o->gridCx());
            maxCy = qMax(maxCy, o->gridCy());
        }
    }
    const int spanX = qMax(0, maxCx);
    const int spanY = qMax(0, maxCy);
    for (int cy = 0; cy <= spanY + 2; ++cy) {
        for (int cx = 0; cx <= spanX + 2; ++cx) {
            if (!used.contains(key(cx, cy))) {
                chiplet->setGridCx(cx);
                chiplet->setGridCy(cy);
                return;
            }
        }
    }
    chiplet->setGridCx(spanX + 1);
    chiplet->setGridCy(0);
}

int GraphScene::defaultMeshKForChiplets() const {
    if (m_topologyBlocks.size() == 1) {
        const BooksimTopologyParams p = m_topologyBlocks.first()->params();
        if (p.topologyId.toLower() == QLatin1String("mesh")) {
            return qMax(1, p.k);
        }
    }
    bool ok = false;
    const int k = m_globalConfig.value(QStringLiteral("k")).toInt(&ok);
    if (ok && k > 0) {
        return k;
    }
    return 1;
}

void GraphScene::refreshAllEdgesChipletDecoration() {
    for (GraphEdge* e : m_edges) {
        if (e) {
            e->updatePosition();
        }
    }
}

void GraphScene::promptRenameChiplet(GraphChiplet* chiplet) {
    if (!chiplet) {
        return;
    }
    QWidget* const p = graphSceneWindowParent(this);
    bool ok = false;
    const QString t = BookCanvasUi::promptLineText(p,
                                                   tr("重命名芯粒"),
                                                   tr("名称："),
                                                   chiplet->label(),
                                                   &ok);
    if (!ok) {
        return;
    }
    const QString trimmed = t.trimmed();
    if (trimmed.isEmpty()) {
        return;
    }
    chiplet->setLabel(trimmed);
}

void GraphScene::createChipletFromSelection(QWidget* dialogParent) {
    QWidget* const p = dialogParent ? dialogParent : graphSceneWindowParent(this);
    QList<GraphNode*> sel;
    for (QGraphicsItem* it : selectedItems()) {
        if (auto* n = dynamic_cast<GraphNode*>(it)) {
            sel.append(n);
        }
    }
    if (sel.isEmpty()) {
        BookCanvasUi::alertInformation(p,
                                       tr("创建芯粒"),
                                       tr("请先在画布上选中至少一个终端或路由器。"));
        return;
    }
    QStringList memberIds;
    memberIds.reserve(sel.size());
    for (GraphNode* n : sel) {
        if (!nodeIsTerminalOrRouter(n)) {
            continue;
        }
        memberIds.append(n->getId());
    }
    if (memberIds.isEmpty()) {
        BookCanvasUi::alertInformation(p,
                                       tr("创建芯粒"),
                                       tr("请先在画布上选中至少一个终端或路由器。"));
        return;
    }
    QString conflictLines;
    for (const QString& nid : memberIds) {
        if (GraphChiplet* existing = findChipletContainingMember(nid)) {
            conflictLines += tr("• %1 已在「%2」中\n").arg(nid, existing->label());
        }
    }
    if (!conflictLines.isEmpty()) {
        BookCanvasUi::alertWarning(p,
                                   tr("无法创建芯粒"),
                                   tr("每个终端或路由器只能属于一个芯粒：\n\n%1")
                                       .arg(conflictLines.trimmed()));
        return;
    }
    bool ok = false;
    const QString defaultLabel = tr("芯粒 %1").arg(m_chiplets.size());
    const QString label = BookCanvasUi::promptLineText(p,
                                                       tr("创建芯粒"),
                                                       tr("名称："),
                                                       defaultLabel,
                                                       &ok);
    if (!ok) {
        return;
    }
    const QString trimmed = label.trimmed();
    if (trimmed.isEmpty()) {
        return;
    }
    const QString cid = allocateNextChipletId();
    if (!isRecordingHistory()) {
        createChipletInternal(cid, trimmed, memberIds);
        return;
    }
    m_undoStack.push(new AddChipletCommand(this, cid, trimmed, memberIds));
}

void GraphScene::removeChiplet(GraphChiplet* chiplet) {
    if (!chiplet) {
        return;
    }
    if (!isRecordingHistory()) {
        removeChipletInternal(chiplet);
        return;
    }
    m_undoStack.push(new RemoveChipletCommand(this, chiplet));
}

void GraphScene::emitUndoStateNow() {
    emit undoStateChanged(m_undoStack.canUndo(),
                          m_undoStack.canRedo(),
                          m_undoStack.undoText(),
                          m_undoStack.redoText());
}

bool GraphScene::isRecordingHistory() const {
    return !m_historySuspended;
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
    if (event->button() == Qt::LeftButton && node) {
        m_draggingNode = node;
        m_dragStartPos = node->pos();
    } else if (event->button() == Qt::LeftButton) {
        m_draggingNode = nullptr;
    }

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
                // 有高亮节点且点了别的节点 → 创建连线（跨芯粒路由器互连时为红色）
                const bool redCrossChiplet = canCreateChipletRedInterconnect(m_highlightNode, node);
                createEdge(m_highlightNode, node, 1.0, redCrossChiplet);
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
    if (event->button() == Qt::LeftButton && m_draggingNode) {
        GraphNode* movedNode = m_draggingNode.data();
        const QPointF from = m_dragStartPos;
        const QPointF to = movedNode->pos();
        m_draggingNode = nullptr;
        if (!qFuzzyCompare(from.x() + 1.0, to.x() + 1.0)
            || !qFuzzyCompare(from.y() + 1.0, to.y() + 1.0)) {
            if (isRecordingHistory()) {
                m_undoStack.push(new MoveNodeCommand(this, movedNode->getId(), from, to));
            }
        }
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

void GraphScene::writeTopologyToStream(QTextStream& out) const {
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
}

// 导出图信息
void GraphScene::exportGraph(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return;
    }
    QTextStream out(&file);
    writeTopologyToStream(out);
}

QString GraphScene::exportTopologyFileText() const {
    QString s;
    QTextStream out(&s);
    writeTopologyToStream(out);
    return s;
}

bool GraphScene::importGraph(const QString& filePath, QString* errorMessage) {
    struct PendingLink {
        bool toRouter = false;
        int targetId = -1;
        double weight = 1.0;
    };

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (errorMessage) {
            *errorMessage = tr("无法打开文件：%1").arg(filePath);
        }
        return false;
    }

    const QRegularExpression ws(QStringLiteral("\\s+"));
    QMap<int, QVector<PendingLink>> linksByRouter;
    QSet<int> routerIds;
    int lineNo = 0;
    QTextStream in(&file);
    while (!in.atEnd()) {
        const QString line = in.readLine();
        ++lineNo;
        const QString trimmed = line.trimmed();
        if (trimmed.isEmpty() || trimmed.startsWith(u'#')) {
            continue;
        }
        const QStringList tk = trimmed.split(ws, Qt::SkipEmptyParts);
        if (tk.size() < 2 || tk[0].compare(QLatin1String("router"), Qt::CaseInsensitive) != 0) {
            if (errorMessage) {
                *errorMessage = tr("第 %1 行格式错误：应以 \"router <id>\" 开头。").arg(lineNo);
            }
            return false;
        }

        bool okRouterId = false;
        const int routerId = tk[1].toInt(&okRouterId);
        if (!okRouterId || routerId < 0) {
            if (errorMessage) {
                *errorMessage = tr("第 %1 行路由器编号无效。").arg(lineNo);
            }
            return false;
        }
        routerIds.insert(routerId);

        int i = 2;
        while (i < tk.size()) {
            const QString kind = tk[i].toLower();
            if (kind != QLatin1String("router") && kind != QLatin1String("node")) {
                if (errorMessage) {
                    *errorMessage = tr("第 %1 行连接类型无效：%2").arg(lineNo).arg(tk[i]);
                }
                return false;
            }
            ++i;
            if (i >= tk.size()) {
                if (errorMessage) {
                    *errorMessage = tr("第 %1 行缺少连接目标编号。").arg(lineNo);
                }
                return false;
            }

            bool okTargetId = false;
            const int targetId = tk[i].toInt(&okTargetId);
            if (!okTargetId || targetId < 0) {
                if (errorMessage) {
                    *errorMessage = tr("第 %1 行连接目标编号无效：%2").arg(lineNo).arg(tk[i]);
                }
                return false;
            }
            ++i;

            double weight = 1.0;
            if (i < tk.size()) {
                bool okWeight = false;
                const double parsed = tk[i].toDouble(&okWeight);
                if (okWeight) {
                    weight = parsed;
                    ++i;
                }
            }

            PendingLink link;
            link.toRouter = (kind == QLatin1String("router"));
            link.targetId = targetId;
            link.weight = weight;
            linksByRouter[routerId].append(link);
            if (link.toRouter) {
                routerIds.insert(targetId);
            }
        }
    }
    file.close();

    if (routerIds.isEmpty()) {
        if (errorMessage) {
            *errorMessage = tr("文件中未找到任何 router 定义。");
        }
        return false;
    }

    QList<int> sortedRouterIds = routerIds.values();
    std::sort(sortedRouterIds.begin(), sortedRouterIds.end());

    clearAllContent();

    QMap<int, GraphNode*> routerById;
    const int routerCount = static_cast<int>(sortedRouterIds.size());
    const int cols = qMax(1,
                          static_cast<int>(std::ceil(std::sqrt(static_cast<double>(routerCount)))));
    constexpr qreal baseX = 120.0;
    constexpr qreal baseY = 110.0;
    constexpr qreal stepX = 190.0;
    constexpr qreal stepY = 150.0;
    for (int idx = 0; idx < sortedRouterIds.size(); ++idx) {
        const int rid = sortedRouterIds[idx];
        const int row = idx / cols;
        const int col = idx % cols;
        const QPointF p(baseX + col * stepX, baseY + row * stepY);
        GraphNode* router = createNode(QStringLiteral("Router_%1").arg(rid), p, GraphNode::Router);
        if (!router) {
            if (errorMessage) {
                *errorMessage = tr("恢复网络失败：创建路由器 Router_%1 失败。").arg(rid);
            }
            return false;
        }
        routerById.insert(rid, router);
    }

    QMap<int, GraphNode*> nodeById;
    QMap<int, int> nodeRankInRouter;
    QSet<quint64> routerEdges;
    for (auto it = linksByRouter.begin(); it != linksByRouter.end(); ++it) {
        const int srcRouterId = it.key();
        GraphNode* srcRouter = routerById.value(srcRouterId, nullptr);
        if (!srcRouter) {
            continue;
        }
        for (const PendingLink& link : it.value()) {
            if (link.toRouter) {
                GraphNode* dstRouter = routerById.value(link.targetId, nullptr);
                if (!dstRouter || dstRouter == srcRouter) {
                    continue;
                }
                const int lo = qMin(srcRouterId, link.targetId);
                const int hi = qMax(srcRouterId, link.targetId);
                const quint64 edgeKey = (static_cast<quint64>(static_cast<quint32>(lo)) << 32)
                                        | static_cast<quint32>(hi);
                if (routerEdges.contains(edgeKey)) {
                    continue;
                }
                GraphEdge* e = createEdge(srcRouter, dstRouter, link.weight);
                if (e) {
                    routerEdges.insert(edgeKey);
                }
                continue;
            }

            GraphNode* node = nodeById.value(link.targetId, nullptr);
            if (!node) {
                const int rank = nodeRankInRouter[srcRouterId]++;
                const int localCol = rank % 5;
                const int localRow = rank / 5;
                const qreal x = srcRouter->pos().x() - 32.0 + localCol * 16.0;
                const qreal y = srcRouter->pos().y() + 72.0 + localRow * 24.0;
                node = createNode(QStringLiteral("Node_%1").arg(link.targetId),
                                  QPointF(x, y),
                                  GraphNode::Node);
                if (!node) {
                    if (errorMessage) {
                        *errorMessage = tr("恢复网络失败：创建终端 Node_%1 失败。")
                                            .arg(link.targetId);
                    }
                    return false;
                }
                nodeById.insert(link.targetId, node);
            }
            createEdge(node, srcRouter, link.weight);
        }
    }

    return true;
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

QMap<QString, QString> GraphScene::mergedGlobalConfigForExport(
    const QString& networkFileOverride) const {
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

    if (!m_chiplets.isEmpty()) {
        int maxCx = -1;
        int maxCy = -1;
        for (GraphChiplet* ch : m_chiplets) {
            if (!ch) {
                continue;
            }
            maxCx = qMax(maxCx, ch->gridCx());
            maxCy = qMax(maxCy, ch->gridCy());
        }
        const int Cx = qMax(1, maxCx + 1);
        const int Cy = qMax(1, maxCy + 1);
        const int fallbackK = defaultMeshKForChiplets();

        QStringList dieK;
        QStringList dil;
        QStringList dcp;
        QStringList dcph;
        int kMax = 1;
        for (int cy = 0; cy < Cy; ++cy) {
            for (int cx = 0; cx < Cx; ++cx) {
                GraphChiplet* hit = nullptr;
                for (GraphChiplet* c : m_chiplets) {
                    if (c && c->gridCx() == cx && c->gridCy() == cy) {
                        hit = c;
                        break;
                    }
                }
                const int k = hit ? hit->dieK() : fallbackK;
                kMax = qMax(kMax, k);
                dieK.append(QString::number(qMax(1, k)));

                int intra = 0;
                if (hit && !hit->dieIntraLatencyText().trimmed().isEmpty()) {
                    bool ok = false;
                    intra = hit->dieIntraLatencyText().trimmed().toInt(&ok);
                    if (!ok) {
                        intra = 0;
                    }
                }
                dil.append(QString::number(intra));

                int cp = 1;
                if (hit && !hit->dieClockPeriodText().trimmed().isEmpty()) {
                    bool ok = false;
                    cp = hit->dieClockPeriodText().trimmed().toInt(&ok);
                    if (!ok || cp < 1) {
                        cp = 1;
                    }
                }
                dcp.append(QString::number(cp));

                int cph = 0;
                if (hit && !hit->dieClockPhaseText().trimmed().isEmpty()) {
                    bool ok = false;
                    cph = hit->dieClockPhaseText().trimmed().toInt(&ok);
                    if (!ok) {
                        cph = 0;
                    }
                }
                dcph.append(QString::number(cph));
            }
        }

        QString conn = globalConfigToExport
                           .value(QStringLiteral("chiplet_connect"), m_chipletMeshConnect)
                           .trimmed()
                           .toLower();
        if (conn != QLatin1String("x") && conn != QLatin1String("xy")) {
            conn = QStringLiteral("x");
        }

        bool nonDefaultDieClock = false;
        for (int i = 0; i < dcp.size(); ++i) {
            bool okP = false;
            bool okPh = false;
            const int period = dcp[i].toInt(&okP);
            const int phase = dcph[i].toInt(&okPh);
            if (!okP || !okPh || period != 1 || phase != 0) {
                nonDefaultDieClock = true;
                break;
            }
        }
        if (nonDefaultDieClock) {
            globalConfigToExport.insert(QStringLiteral("chiplet_cdc_enable"), QStringLiteral("1"));
        }

        globalConfigToExport.insert(QStringLiteral("topology"), QStringLiteral("chiplet_mesh"));
        globalConfigToExport.insert(QStringLiteral("routing_function"),
                                    QStringLiteral("dim_order_chiplet_mesh"));
        globalConfigToExport.insert(QStringLiteral("chiplet_x"), QString::number(Cx));
        globalConfigToExport.insert(QStringLiteral("chiplet_y"), QString::number(Cy));
        globalConfigToExport.insert(QStringLiteral("chiplet_k"), QString::number(kMax));
        globalConfigToExport.insert(QStringLiteral("chiplet_connect"), conn);
        globalConfigToExport.insert(QStringLiteral("chiplet_die_k"),
                                    QStringLiteral("{%1}").arg(dieK.join(QLatin1Char(','))));
        globalConfigToExport.insert(QStringLiteral("chiplet_die_intra_latency"),
                                    QStringLiteral("{%1}").arg(dil.join(QLatin1Char(','))));
        globalConfigToExport.insert(QStringLiteral("chiplet_die_clock_period"),
                                    QStringLiteral("{%1}").arg(dcp.join(QLatin1Char(','))));
        globalConfigToExport.insert(QStringLiteral("chiplet_die_clock_phase"),
                                    QStringLiteral("{%1}").arg(dcph.join(QLatin1Char(','))));
    }

    return globalConfigToExport;
}

void GraphScene::writeJSONConfigToStream(QTextStream& out,
                                         const QString& networkFileOverride) const {
    out << "{\n";

    QMap<QString, QString> globalConfigToExport = mergedGlobalConfigForExport(networkFileOverride);

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
            if (!firstRouter) {
                out << ",\n";
            }
            firstRouter = false;

            // 获取路由器配置（如果有），否则使用默认配置
            QMap<QString, QString> routerConfig = m_routerConfigs.value(it.value());
            if (routerConfig.isEmpty()) {
                routerConfig = RouterConfigDialog::getDefaultConfig();
            }

            out << "    \"" << it.key() << "\": {\n";

            bool firstParam = true;
            for (auto paramIt = routerConfig.begin(); paramIt != routerConfig.end(); ++paramIt) {
                if (!firstParam) {
                    out << ",\n";
                }
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

    if (!m_chiplets.isEmpty()) {
        out << ",\n\n  \"bookcanvas_chiplets\": [\n";
        bool firstChip = true;
        for (GraphChiplet* ch : m_chiplets) {
            if (!ch) {
                continue;
            }
            if (!firstChip) {
                out << ",\n";
            }
            firstChip = false;
            out << "    {\n";
            out << "      \"id\": \"" << escapeJsonString(ch->chipletId()) << "\",\n";
            out << "      \"label\": \"" << escapeJsonString(ch->label()) << "\",\n";
            out << "      \"members\": [";
            bool firstM = true;
            for (const QString& mid : ch->memberIdsSorted()) {
                if (!firstM) {
                    out << ", ";
                }
                firstM = false;
                out << "\"" << escapeJsonString(mid) << "\"";
            }
            out << "],\n";
            out << "      \"grid_cx\": " << ch->gridCx() << ",\n";
            out << "      \"grid_cy\": " << ch->gridCy() << ",\n";
            out << "      \"die_k\": " << ch->dieK() << ",\n";
            out << "      \"die_intra_latency\": \"" << escapeJsonString(ch->dieIntraLatencyText())
                << "\",\n";
            out << "      \"die_clock_period\": \"" << escapeJsonString(ch->dieClockPeriodText())
                << "\",\n";
            out << "      \"die_clock_phase\": \"" << escapeJsonString(ch->dieClockPhaseText())
                << "\"\n";
            out << "    }";
        }
        out << "\n  ]";
        out << ",\n\n  \"bookcanvas_chiplet_mesh\": {\n";
        out << "    \"connect\": \"" << escapeJsonString(m_chipletMeshConnect) << "\"\n";
        out << "  }";
    }

    QList<GraphEdge*> redExport;
    for (GraphEdge* e : m_edges) {
        if (e && e->isChipletRedInterconnect() && e->startNode() && e->endNode()) {
            redExport.append(e);
        }
    }
    if (!redExport.isEmpty()) {
        out << ",\n\n  \"bookcanvas_chiplet_red_links\": [\n";
        bool firstR = true;
        for (GraphEdge* e : redExport) {
            if (!firstR) {
                out << ",\n";
            }
            firstR = false;
            out << "    { \"from\": \"" << escapeJsonString(e->startNode()->getId()) << "\", ";
            out << "\"to\": \"" << escapeJsonString(e->endNode()->getId()) << "\", ";
            out << "\"weight\": " << e->weight() << " }";
        }
        out << "\n  ]";
    }

    out << "}\n";
}

// 导出JSON配置
void GraphScene::exportJSONConfig(const QString& filePath, const QString& networkFileOverride) {
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return;
    }
    QTextStream out(&file);
    writeJSONConfigToStream(out, networkFileOverride);
}

QString GraphScene::exportJSONConfigText(const QString& networkFileOverride) const {
    QString s;
    QTextStream out(&s);
    writeJSONConfigToStream(out, networkFileOverride);
    return s;
}
