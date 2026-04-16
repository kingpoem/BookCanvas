#pragma once
#include "BooksimTopologyParams.h"
#include "ElaGraphicsScene.h"
#include "GraphEdge.h"
#include "GraphNode.h"
#include <QList>
#include <QMap>
#include <QPointer>
#include <QString>
#include <QTextStream>
#include <QUndoStack>

class GraphTopologyBlock;
class GraphChiplet;
class QAction;
class QUndoCommand;
class QWidget;

// 提供了节点/边的创建、删除，以及与拖拽交互、鼠标事件相关的逻辑
class GraphScene : public ElaGraphicsScene {
    Q_OBJECT
public:
    enum class PlaceTool { None, Terminal, Router, TopologyBlock };

    explicit GraphScene(QObject* parent = nullptr);

    void setPlaceTool(PlaceTool tool);
    [[nodiscard]] PlaceTool placeTool() const { return m_placeTool; }

    /// 对话框确认后进入「单击画布放置」模式（与终端/路由器互斥）
    void beginPlaceBooksimTopology(const BooksimTopologyParams& params);
    void clearBooksimTopologyPlacePending();
    void updateTopologyBlockParams(GraphTopologyBlock* block, const BooksimTopologyParams& params);

    [[nodiscard]] int topologyBlockCount() const {
        return static_cast<int>(m_topologyBlocks.size());
    }

    // 信号：节点配置请求
    Q_SIGNAL void nodeConfigureRequested(GraphNode* node);
    Q_SIGNAL void topologyBlockConfigureRequested(GraphTopologyBlock* block);
    /// 画布上出现芯粒或需要 chiplet_mesh 时请求同步「全局配置」拓扑与路由
    Q_SIGNAL void chipletMeshGlobalConfigRequested();
    Q_SIGNAL void undoStateChanged(bool canUndo,
                                   bool canRedo,
                                   const QString& undoText,
                                   const QString& redoText);

    GraphNode* createNode(const QString& id,
                          const QPointF& pos,
                          GraphNode::NodeType type = GraphNode::Node); // 创建节点，支持指定类型
    void removeNode(GraphNode* node);                                  // 删除节点和相关的边
    [[nodiscard]] QList<GraphNode*> nodes() const {
        return m_nodes;
    } // 返回当前场景中所有节点的列表

    // 边管理
    GraphEdge* createEdge(GraphNode* start,
                          GraphNode* end,
                          double weight = 1.0,
                          bool chipletRedInterconnect = false);
    void removeEdge(GraphEdge* edge);
    [[nodiscard]] QList<GraphEdge*> edges() const { return m_edges; }

    // 控制所有边的权重显示/隐藏
    void setAllEdgeWeightsVisible(bool visible);
    // 将路由器/终端分别按当前编号顺序重排为从 0 开始的连续编号
    bool renumberAllNodesFromZero();
    // 删除没有任何连线的节点（路由器/终端），返回删除数量
    int removeUnconnectedNodes();
    /// 将当前选中的终端/路由器归为一个芯粒（画布上显示为虚线包围框）
    void createChipletFromSelection(QWidget* dialogParent = nullptr);
    void removeChiplet(GraphChiplet* chiplet);

    [[nodiscard]] QList<GraphChiplet*> chiplets() const { return m_chiplets; }
    /// 与场景内 m_globalConfig 的 topology=chiplet_mesh 一致（创建芯粒后会由画布同步全局配置）
    [[nodiscard]] bool chipletGroupingAllowed() const;

    [[nodiscard]] QString chipletMeshConnect() const { return m_chipletMeshConnect; }
    void setChipletMeshConnect(QString v) { m_chipletMeshConnect = std::move(v); }

    /// 路由器–路由器边且两端属于不同芯粒分组时视为跨芯粒链路（用于视觉与菜单）
    [[nodiscard]] bool edgeConnectsDistinctChiplets(const GraphEdge* edge) const;

    // 清空画布上的全部内容（节点、边、拓扑块）
    void clearAllContent();
    void undo();
    void redo();
    [[nodiscard]] bool canUndo() const;
    [[nodiscard]] bool canRedo() const;
    [[nodiscard]] QString undoText() const;
    [[nodiscard]] QString redoText() const;
    QAction* createUndoAction(QObject* parent, const QString& prefix = {});
    QAction* createRedoAction(QObject* parent, const QString& prefix = {});

    // 检查连接是否合法（节点只能连接路由器，路由器可以连接任何节点）
    static bool isConnectionValid(GraphNode* start, GraphNode* end);

    // 导出图信息；无法创建/打开目标文件时返回 false，并可写入 QFile 错误说明
    [[nodiscard]] bool exportGraph(const QString& filePath, QString* ioErrorMessage = nullptr);
    /// 与 exportGraph 写入内容一致（任意网络文本格式）
    [[nodiscard]] QString exportTopologyFileText() const;
    // 从 BookSim network 文件恢复图；失败时返回 false，并可写出错误信息
    [[nodiscard]] bool importGraph(const QString& filePath, QString* errorMessage = nullptr);

    /// 与 exportJSONConfig 写入的顶层键一致：全局配置 + 单拓扑块覆盖 + network_file
    [[nodiscard]] QMap<QString, QString> mergedGlobalConfigForExport(
        const QString& networkFileOverride = {}) const;
    // 导出JSON配置；networkFileOverride 非空时覆盖写入的 network_file，便于与拓扑导出路径一致
    [[nodiscard]] bool exportJSONConfig(const QString& filePath,
                                        const QString& networkFileOverride = {},
                                        QString* ioErrorMessage = nullptr);
    /// 与 exportJSONConfig 写入内容一致
    [[nodiscard]] QString exportJSONConfigText(const QString& networkFileOverride = {}) const;

    // 获取和设置路由器独立配置
    [[nodiscard]] QMap<QString, QString> getRouterConfig(const QString& routerId) const;
    void setRouterConfig(const QString& routerId, const QMap<QString, QString>& config);

    // 获取和设置全局配置
    [[nodiscard]] QMap<QString, QString> getGlobalConfig() const { return m_globalConfig; }
    void setGlobalConfig(const QMap<QString, QString>& config) { m_globalConfig = config; }

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
    class AddNodeCommand;
    class AddEdgeCommand;
    class RemoveNodeCommand;
    class RemoveEdgeCommand;
    class MoveNodeCommand;
    class UpdateTopologyParamsCommand;
    class AddChipletCommand;
    class RemoveChipletCommand;
    friend class AddNodeCommand;
    friend class AddEdgeCommand;
    friend class RemoveNodeCommand;
    friend class RemoveEdgeCommand;
    friend class MoveNodeCommand;
    friend class UpdateTopologyParamsCommand;
    friend class AddChipletCommand;
    friend class RemoveChipletCommand;
    friend class GraphChiplet;

    static int extractNumberId(const QString& id); // 辅助函数
    void writeTopologyToStream(QTextStream& out) const;
    void writeJSONConfigToStream(QTextStream& out, const QString& networkFileOverride) const;
    [[nodiscard]] QString allocateNextNodeId(GraphNode::NodeType type) const;
    /// 将用户输入规范为 Router_%d / Node_%d；无效则返回空字符串
    [[nodiscard]] static QString canonicalIdFromUserInput(GraphNode::NodeType type,
                                                          const QString& raw);
    bool renameNodeToId(GraphNode* node, const QString& newId);
    void promptRenameNode(GraphNode* node);
    [[nodiscard]] QString allocateNextTopologyBlockId() const;
    void createTopologyBlockAt(const QPointF& pos);
    void removeTopologyBlock(GraphTopologyBlock* block);
    void rebuildManagedTopology(GraphTopologyBlock* block);
    void clearManagedTopology(GraphTopologyBlock* block);
    [[nodiscard]] QString allocateNextChipletId() const;
    GraphChiplet* createChipletInternal(const QString& id,
                                        const QString& label,
                                        const QStringList& members,
                                        bool assignAutoGrid = true);
    void removeChipletInternal(GraphChiplet* chiplet);
    void layoutChipletAroundMembers(GraphChiplet* chiplet);
    void refitChipletsContainingMember(const QString& nodeId);
    void stripNodeFromChiplets(const QString& nodeId);
    [[nodiscard]] GraphChiplet* findChipletById(const QString& id) const;
    [[nodiscard]] GraphChiplet* findChipletContainingMember(const QString& nodeId) const;
    void wireChiplet(GraphChiplet* chiplet);
    void promptRenameChiplet(GraphChiplet* chiplet);
    void promptConfigureChipletDie(GraphChiplet* chiplet, QWidget* dialogParent);
    void assignDefaultGridForChiplet(GraphChiplet* chiplet);
    [[nodiscard]] int defaultMeshKForChiplets() const;
    void refreshAllEdgesChipletDecoration();
    GraphNode* createNodeInternal(const QString& id, const QPointF& pos, GraphNode::NodeType type);
    void removeNodeInternal(GraphNode* node);
    GraphEdge* createEdgeInternal(GraphNode* start,
                                  GraphNode* end,
                                  double weight,
                                  bool chipletRedInterconnect = false);
    [[nodiscard]] bool canCreateChipletRedInterconnect(GraphNode* a, GraphNode* b) const;
    void removeEdgeInternal(GraphEdge* edge);
    [[nodiscard]] GraphNode* findNodeById(const QString& id) const;
    void emitUndoStateNow();
    bool isRecordingHistory() const;

    QList<GraphNode*> m_nodes; // 存放所有节点
    QList<GraphEdge*> m_edges; // 存放所有边

    // 连线临时变量
    GraphNode* m_lineStartNode = nullptr; // 存放所有边
    GraphEdge* m_tempEdge = nullptr;      // 临时连线（随着鼠标移动）

    // 工具栏拖拽生成节点
    QString m_pendingToolName;

    PlaceTool m_placeTool = PlaceTool::None;

    BooksimTopologyParams m_pendingBooksimTopology;
    bool m_pendingBooksimTopologyActive = false;

    GraphNode* m_highlightNode = nullptr;

    QList<GraphTopologyBlock*> m_topologyBlocks;
    QPointer<GraphChiplet> m_chipletHandleDragSource;
    QMap<QString, QPointF> m_chipletHandleDragStartPos;
    QList<GraphChiplet*> m_chiplets;
    QString m_chipletMeshConnect = QStringLiteral("x");
    struct ManagedTopologyState {
        BooksimTopologyParams params;
        QList<QPointer<GraphNode>> routers;
        QList<QPointer<GraphNode>> terminals;
        QList<QPointer<GraphEdge>> edges;
    };
    QMap<GraphTopologyBlock*, ManagedTopologyState> m_managedTopologies;

    // 每个路由器的独立配置（routerId -> config）
    QMap<QString, QMap<QString, QString>> m_routerConfigs;

    // 全局配置参数
    QMap<QString, QString> m_globalConfig;
    QUndoStack m_undoStack;
    bool m_historySuspended = false;
    QPointer<GraphNode> m_draggingNode;
    QPointF m_dragStartPos;
};
