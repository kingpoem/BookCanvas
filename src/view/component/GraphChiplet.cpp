#include "GraphChiplet.h"
#include "GraphNode.h"
#include "GraphScene.h"
#include "utils/CanvasDebugLog.h"
#include <QFontMetricsF>
#include <QGraphicsSceneContextMenuEvent>
#include <QGraphicsSceneHoverEvent>
#include <QGraphicsSceneMouseEvent>
#include <QGuiApplication>
#include <QMenu>
#include <QPainter>
#include <QPalette>
#include <QRegularExpression>
#include <QStyleOptionGraphicsItem>

namespace {
bool isTerminalOrRouter(const GraphNode* n) {
    return n && (n->getType() == GraphNode::Node || n->getType() == GraphNode::Router);
}

/// 画布标题仅用于绘制：默认中文「芯粒 N」显示为「Chiplet N」，其它自定义名称原样显示。
[[nodiscard]] QString chipletTitlePaintText(const QString& storedLabel) {
    static const QRegularExpression re(QStringLiteral(R"(^芯粒\s*(\d+)$)"));
    const QRegularExpressionMatch m = re.match(storedLabel.trimmed());
    if (m.hasMatch()) {
        return QStringLiteral("Chiplet %1").arg(m.captured(1));
    }
    return storedLabel;
}
} // namespace

GraphChiplet::GraphChiplet(QString chipletId, QString label, QGraphicsItem* parent)
    : ElaGraphicsItem(parent)
    , m_chipletId(std::move(chipletId))
    , m_label(std::move(label)) {
    setFlags(ItemIsSelectable | ItemSendsGeometryChanges);
    setAcceptedMouseButtons(Qt::LeftButton | Qt::RightButton);
    setAcceptHoverEvents(true);
}

void GraphChiplet::setLabel(QString label) {
    m_label = std::move(label);
    update();
}

QStringList GraphChiplet::memberIdsSorted() const {
    QStringList out = m_memberIds.values();
    out.sort();
    return out;
}

bool GraphChiplet::hasMember(const QString& id) const {
    return m_memberIds.contains(id);
}

void GraphChiplet::setMembers(const QStringList& ids) {
    m_memberIds.clear();
    for (const QString& id : ids) {
        if (!id.isEmpty()) {
            m_memberIds.insert(id);
        }
    }
    update();
}

bool GraphChiplet::addMember(const QString& id) {
    if (id.isEmpty()) {
        return false;
    }
    const int before = static_cast<int>(m_memberIds.size());
    m_memberIds.insert(id);
    return static_cast<int>(m_memberIds.size()) != before;
}

bool GraphChiplet::removeMember(const QString& id) {
    return m_memberIds.remove(id) > 0;
}

int GraphChiplet::liveTerminalRouterMemberCount() const {
    auto* gs = qobject_cast<GraphScene*>(scene());
    if (!gs) {
        return 0;
    }
    int n = 0;
    for (const QString& id : m_memberIds) {
        if (GraphNode* node = gs->findNodeById(id)) {
            if (isTerminalOrRouter(node)) {
                ++n;
            }
        }
    }
    return n;
}

void GraphChiplet::renameMemberId(const QString& oldId, const QString& newId) {
    if (oldId.isEmpty() || oldId == newId || !m_memberIds.contains(oldId)) {
        return;
    }
    m_memberIds.remove(oldId);
    if (!newId.isEmpty()) {
        m_memberIds.insert(newId);
    }
}

void GraphChiplet::fitAroundNodes(const QList<GraphNode*>& nodes, qreal margin) {
    if (nodes.isEmpty()) {
        m_rect = QRectF(0, 0, 120, 72);
        update();
        return;
    }
    QRectF u;
    bool any = false;
    for (GraphNode* n : nodes) {
        if (!n) {
            continue;
        }
        const QRectF sb = n->sceneBoundingRect();
        if (!any) {
            u = sb;
            any = true;
        } else {
            u = u.united(sb);
        }
    }
    if (!any) {
        m_rect = QRectF(0, 0, 120, 72);
        update();
        return;
    }
    const QRectF padded = u.adjusted(-margin, -margin, margin, margin);
    setPos(padded.topLeft());
    m_rect = QRectF(QPointF(0, 0), padded.size());
    update();
}

QRectF GraphChiplet::boundingRect() const {
    return m_rect;
}

QRectF GraphChiplet::titleHandleRect() const {
    const QRectF rr = m_rect.adjusted(3, 3, -3, -3);
    QFont titleFont;
    titleFont.setBold(true);
    titleFont.setPointSize(9);
    const QFontMetricsF fm(titleFont);
    const qreal textW = fm.horizontalAdvance(chipletTitlePaintText(m_label));
    const qreal textH = fm.height();
    constexpr qreal padX = 10.0;
    constexpr qreal padY = 6.0;
    const qreal w = textW + padX * 2.0;
    const qreal h = textH + padY * 2.0;
    return {rr.left() + 8.0, rr.top() + 8.0, w, h};
}

void GraphChiplet::paintTitleLabel(QPainter* painter,
                                   const bool selected,
                                   const QPalette& pal,
                                   const bool dark) {
    const QRectF handle = titleHandleRect();

    QColor labelBg = dark ? QColor(15, 55, 65, 220) : QColor(0xff, 0xff, 0xff, 235);
    QColor labelBorder = dark ? QColor(0x5eead4) : QColor(0x0d, 0x94, 0x88);
    const QColor labelText = dark ? QColor(0xecfeff) : QColor(0x0f, 0x76, 0x72);

    if (selected) {
        labelBorder = pal.color(QPalette::Highlight);
        labelBg = labelBg.lighter(dark ? 115 : 105);
    }

    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setPen(QPen(labelBorder, 1));
    painter->setBrush(labelBg);
    painter->drawRoundedRect(handle, 6, 6);

    QFont titleFont = painter->font();
    titleFont.setBold(true);
    titleFont.setPointSize(9);
    painter->setFont(titleFont);
    painter->setPen(labelText);
    painter->drawText(handle.adjusted(10, 0, -10, 0),
                      Qt::AlignVCenter | Qt::AlignLeft,
                      chipletTitlePaintText(m_label));
}

void GraphChiplet::paint(QPainter* painter,
                         const QStyleOptionGraphicsItem* option,
                         QWidget* widget) {
    painter->setRenderHint(QPainter::Antialiasing, true);

    const QPalette& pal = widget ? widget->palette() : QGuiApplication::palette();
    const QColor baseBg = pal.color(QPalette::Base);
    const bool dark = baseBg.lightness() < 140;

    QColor fill = dark ? QColor(30, 77, 92, 90) : QColor(0xd0, 0xf0, 0xf4, 110);
    QColor stroke = dark ? QColor(0x5eead4) : QColor(0x0d, 0x94, 0x88);

    int penW = 2;
    if (option->state & QStyle::State_Selected) {
        stroke = pal.color(QPalette::Highlight);
        penW = 3;
        fill = fill.lighter(dark ? 120 : 108);
    }

    const QRectF rr = m_rect.adjusted(3, 3, -3, -3);
    painter->setPen(Qt::NoPen);
    painter->setBrush(fill);
    painter->drawRoundedRect(rr, 10, 10);
    painter->setBrush(Qt::NoBrush);
    QPen p(stroke, penW);
    p.setStyle(Qt::DashLine);
    painter->setPen(p);
    painter->drawRoundedRect(rr, 10, 10);

    const QPalette& optPal = widget ? widget->palette() : QGuiApplication::palette();
    paintTitleLabel(painter, option->state & QStyle::State_Selected, optPal, dark);

    painter->setPen(dark ? QColor(0xecfeff) : QColor(0x0f, 0x76, 0x72));
    QFont sub = painter->font();
    sub.setBold(false);
    sub.setPointSize(8);
    painter->setFont(sub);
    const QString subline = tr("成员 %1 个 · 芯粒").arg(liveTerminalRouterMemberCount());
    painter->drawText(rr.adjusted(10, 28, -10, -8), Qt::AlignLeft | Qt::AlignBottom, subline);
}

void GraphChiplet::hoverMoveEvent(QGraphicsSceneHoverEvent* event) {
    if (!m_handleDragging) {
        if (titleHandleRect().contains(event->pos())) {
            setCursor(Qt::OpenHandCursor);
        } else {
            unsetCursor();
        }
    }
    ElaGraphicsItem::hoverMoveEvent(event);
}

void GraphChiplet::hoverLeaveEvent(QGraphicsSceneHoverEvent* event) {
    if (!m_handleDragging) {
        unsetCursor();
    }
    ElaGraphicsItem::hoverLeaveEvent(event);
}

void GraphChiplet::mousePressEvent(QGraphicsSceneMouseEvent* event) {
    if (event->button() == Qt::LeftButton && titleHandleRect().contains(event->pos())) {
        m_handleDragging = true;
        m_lastDragScenePos = event->scenePos();
        grabMouse();
        setCursor(Qt::ClosedHandCursor);
        emit handleDragStarted(this);
        event->accept();
        return;
    }
    ElaGraphicsItem::mousePressEvent(event);
}

void GraphChiplet::mouseMoveEvent(QGraphicsSceneMouseEvent* event) {
    if (m_handleDragging && (event->buttons() & Qt::LeftButton)) {
        const QPointF delta = event->scenePos() - m_lastDragScenePos;
        m_lastDragScenePos = event->scenePos();
        if (!delta.isNull()) {
            emit handleDragDelta(this, delta);
        }
        event->accept();
        return;
    }
    ElaGraphicsItem::mouseMoveEvent(event);
}

void GraphChiplet::mouseReleaseEvent(QGraphicsSceneMouseEvent* event) {
    if (m_handleDragging && event->button() == Qt::LeftButton) {
        m_handleDragging = false;
        ungrabMouse();
        unsetCursor();
        emit handleDragReleased(this);
        event->accept();
        return;
    }
    ElaGraphicsItem::mouseReleaseEvent(event);
}

void GraphChiplet::contextMenuEvent(QGraphicsSceneContextMenuEvent* event) {
    QMenu menu;
    QAction* cfgAction = nullptr;
    auto* gs = qobject_cast<GraphScene*>(scene());
    if (gs) {
        cfgAction = menu.addAction(tr("芯粒仿真参数…"));
        menu.addSeparator();
    }
    QAction* renameAction = menu.addAction(tr("重命名…"));
    QAction* delAction = menu.addAction(tr("删除芯粒"));
    QAction* sel = menu.exec(event->screenPos());
    if (cfgAction && sel == cfgAction) {
        emit configureDieParamsRequested(this);
    } else if (sel == renameAction) {
        emit renameRequested(this);
    } else if (sel == delAction) {
        canvasDebugLog(QStringLiteral("GraphChiplet menu 删除 id=%1").arg(m_chipletId));
        emit deleteRequested(this);
    }
}

void GraphChiplet::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) {
    if (event && event->button() == Qt::LeftButton) {
        if (titleHandleRect().contains(event->pos())) {
            emit renameRequested(this);
            event->accept();
            return;
        }
    }
    ElaGraphicsItem::mouseDoubleClickEvent(event);
}
