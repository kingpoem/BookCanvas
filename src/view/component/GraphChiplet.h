#pragma once

#include "ElaGraphicsItem.h"
#include <QGraphicsSceneContextMenuEvent>
#include <QGraphicsSceneHoverEvent>
#include <QGraphicsSceneMouseEvent>
#include <QPointF>
#include <QSet>
#include <QStringList>

class GraphNode;
class QPalette;

/// 画布上的芯粒（chiplet）：逻辑上为一组节点，自动根据成员包围盒绘制半透明圆角框
class GraphChiplet final : public ElaGraphicsItem {
    Q_OBJECT
public:
    explicit GraphChiplet(QString chipletId, QString label, QGraphicsItem* parent = nullptr);

    [[nodiscard]] QString chipletId() const { return m_chipletId; }
    [[nodiscard]] QString label() const { return m_label; }
    void setLabel(QString label);

    [[nodiscard]] QStringList memberIdsSorted() const;
    [[nodiscard]] bool hasMember(const QString& id) const;
    void setMembers(const QStringList& ids);
    bool addMember(const QString& id);
    bool removeMember(const QString& id);
    void renameMemberId(const QString& oldId, const QString& newId);
    [[nodiscard]] int memberCount() const { return static_cast<int>(m_memberIds.size()); }
    /// 当前场景中仍为终端或路由器的成员数（不含失效 id）
    [[nodiscard]] int liveTerminalRouterMemberCount() const;

    [[nodiscard]] int gridCx() const { return m_gridCx; }
    [[nodiscard]] int gridCy() const { return m_gridCy; }
    void setGridCx(int v) { m_gridCx = qMax(0, v); }
    void setGridCy(int v) { m_gridCy = qMax(0, v); }

    [[nodiscard]] int dieK() const { return m_dieK; }
    void setDieK(int v) { m_dieK = qMax(1, v); }

    [[nodiscard]] QString dieIntraLatencyText() const { return m_dieIntraLatencyText; }
    void setDieIntraLatencyText(QString v) { m_dieIntraLatencyText = std::move(v); }

    [[nodiscard]] QString dieClockPeriodText() const { return m_dieClockPeriodText; }
    void setDieClockPeriodText(QString v) { m_dieClockPeriodText = std::move(v); }

    [[nodiscard]] QString dieClockPhaseText() const { return m_dieClockPhaseText; }
    void setDieClockPhaseText(QString v) { m_dieClockPhaseText = std::move(v); }

    /// 根据节点场景包围盒更新本项的位置与本地矩形（含 margin）
    void fitAroundNodes(const QList<GraphNode*>& nodes, qreal margin = 22.0);

    [[nodiscard]] QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

Q_SIGNALS:
    void deleteRequested(GraphChiplet* chiplet);
    void renameRequested(GraphChiplet* chiplet);
    void configureDieParamsRequested(GraphChiplet* chiplet);
    void handleDragStarted(GraphChiplet* chiplet);
    void handleDragDelta(GraphChiplet* chiplet, QPointF sceneDelta);
    void handleDragReleased(GraphChiplet* chiplet);

protected:
    void contextMenuEvent(QGraphicsSceneContextMenuEvent* event) override;
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) override;
    void hoverMoveEvent(QGraphicsSceneHoverEvent* event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;

private:
    [[nodiscard]] QRectF titleHandleRect() const;
    void paintTitleLabel(QPainter* painter, bool selected, const QPalette& pal, bool dark);

    QString m_chipletId;
    QString m_label;
    QSet<QString> m_memberIds;
    int m_gridCx = 0;
    int m_gridCy = 0;
    int m_dieK = 1;
    QString m_dieIntraLatencyText;
    QString m_dieClockPeriodText;
    QString m_dieClockPhaseText;
    QRectF m_rect = QRectF(0, 0, 120, 80);
    bool m_handleDragging = false;
    QPointF m_lastDragScenePos;
};
