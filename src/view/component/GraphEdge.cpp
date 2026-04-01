#include "GraphEdge.h"
#include "GraphScene.h"
#include <ElaDef.h>
#include <ElaTheme.h>
#include <QBrush>
#include <QGuiApplication>
#include <QInputDialog>
#include <QMenu>
#include <QPainterPath>
#include <QPainterPathStroker>
#include <QPen>
#include <QPointer>
#include <QTimer>

namespace {

constexpr qreal kHitStrokeWidth = 14.0;
constexpr qreal kHandleRadius = 6.0;
constexpr qreal kHandleDiameter = kHandleRadius * 2;
/// 拐点允许偏离端点包络盒的距离；超出后曼哈顿拼接易产生「探出」多余线段
constexpr qreal kBendCorridorMargin = 52.0;

[[nodiscard]] QRectF bendCorridorRect(const QPointF& sceneA, const QPointF& sceneB) {
    QRectF r = QRectF(sceneA, sceneB)
                   .normalized()
                   .adjusted(-kBendCorridorMargin,
                             -kBendCorridorMargin,
                             kBendCorridorMargin,
                             kBendCorridorMargin);
    constexpr qreal kMinSpan = 28.0;
    if (r.width() < kMinSpan) {
        const qreal cx = r.center().x();
        r.setLeft(cx - kMinSpan * 0.5);
        r.setRight(cx + kMinSpan * 0.5);
    }
    if (r.height() < kMinSpan) {
        const qreal cy = r.center().y();
        r.setTop(cy - kMinSpan * 0.5);
        r.setBottom(cy + kMinSpan * 0.5);
    }
    return r;
}

[[nodiscard]] QPointF clampBendToCorridor(const QPointF& sceneA,
                                          const QPointF& sceneB,
                                          const QPointF& bend) {
    const QRectF r = bendCorridorRect(sceneA, sceneB);
    return QPointF(qBound(r.left(), bend.x(), r.right()), qBound(r.top(), bend.y(), r.bottom()));
}

[[nodiscard]] QVector<QPointF> dedupeConsecutivePoints(const QVector<QPointF>& pts) {
    QVector<QPointF> o;
    o.reserve(pts.size());
    for (const QPointF& p : pts) {
        if (o.isEmpty() || QLineF(o.last(), p).length() > 1e-3) {
            o.append(p);
        }
    }
    return o;
}

[[nodiscard]] bool orthoCollinear3(const QPointF& a, const QPointF& b, const QPointF& c) {
    const bool vert = qAbs(a.x() - b.x()) < 1e-3 && qAbs(b.x() - c.x()) < 1e-3;
    const bool horiz = qAbs(a.y() - b.y()) < 1e-3 && qAbs(b.y() - c.y()) < 1e-3;
    return vert || horiz;
}

/// b 落在 a–c 的正交闭线段上（三点共线为正交线）
[[nodiscard]] bool orthoMiddleRedundant(const QPointF& a, const QPointF& b, const QPointF& c) {
    if (!orthoCollinear3(a, b, c)) {
        return false;
    }
    return b.x() >= qMin(a.x(), c.x()) - 1e-3 && b.x() <= qMax(a.x(), c.x()) + 1e-3
           && b.y() >= qMin(a.y(), c.y()) - 1e-3 && b.y() <= qMax(a.y(), c.y()) + 1e-3;
}

[[nodiscard]] QVector<QPointF> simplifyOrthoScenePolyline(QVector<QPointF> pts) {
    pts = dedupeConsecutivePoints(pts);
    if (pts.size() < 3) {
        return pts;
    }
    QVector<QPointF> out;
    out.append(pts.first());
    for (int i = 1; i < pts.size(); ++i) {
        while (out.size() >= 2 && orthoMiddleRedundant(out[out.size() - 2], out.last(), pts.at(i))) {
            out.removeLast();
        }
        out.append(pts.at(i));
    }
    return out;
}

QVector<QPointF> orthoPolyline(const QPointF& a, const QPointF& b) {
    QVector<QPointF> pts;
    pts.append(a);
    const qreal dx = b.x() - a.x();
    const qreal dy = b.y() - a.y();
    if (qFuzzyIsNull(dx) || qFuzzyIsNull(dy)) {
        pts.append(b);
        return pts;
    }
    if (qAbs(dx) >= qAbs(dy)) {
        pts.append(QPointF(b.x(), a.y()));
    } else {
        pts.append(QPointF(a.x(), b.y()));
    }
    pts.append(b);
    return pts;
}

QVector<QPointF> mergeOrthoPath(const QVector<QPointF>& first, const QVector<QPointF>& second) {
    if (first.isEmpty()) {
        return second;
    }
    if (second.isEmpty()) {
        return first;
    }
    QVector<QPointF> out = first;
    int start = 0;
    if (QLineF(out.last(), second.first()).length() < 1e-3) {
        start = 1;
    }
    for (int i = start; i < second.size(); ++i) {
        out.append(second.at(i));
    }
    return out;
}

QPointF midAlongPolyline(const QVector<QPointF>& pts) {
    if (pts.isEmpty()) {
        return {};
    }
    if (pts.size() == 1) {
        return pts[0];
    }
    qreal total = 0;
    for (int i = 0; i + 1 < pts.size(); ++i) {
        total += QLineF(pts[i], pts[i + 1]).length();
    }
    if (total <= 1e-9) {
        return pts.first();
    }
    qreal half = total * 0.5;
    for (int i = 0; i + 1 < pts.size(); ++i) {
        QLineF seg(pts[i], pts[i + 1]);
        const qreal len = seg.length();
        if (len <= 1e-9) {
            continue;
        }
        if (half <= len) {
            return seg.pointAt(half / len);
        }
        half -= len;
    }
    return pts.last();
}

QPainterPath polylinePath(const QVector<QPointF>& pts) {
    QPainterPath path;
    if (pts.size() < 2) {
        return path;
    }
    path.moveTo(pts.first());
    for (int i = 1; i < pts.size(); ++i) {
        path.lineTo(pts.at(i));
    }
    return path;
}

} // namespace

class GraphEdgeBendHandle : public QGraphicsEllipseItem {
public:
    explicit GraphEdgeBendHandle(GraphEdge* edge)
        : QGraphicsEllipseItem(-kHandleRadius, -kHandleRadius, kHandleDiameter, kHandleDiameter, edge)
        , m_edge(edge) {
        setFlag(QGraphicsItem::ItemIsMovable, true);
        setFlag(QGraphicsItem::ItemIsSelectable, false);
        setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
        setAcceptHoverEvents(true);
        setCursor(Qt::SizeAllCursor);
        setZValue(2);
        const auto mode = eTheme->getThemeMode();
        QColor stroke = ElaThemeColor(mode, PrimaryNormal);
        setPen(QPen(stroke.darker(110), 1.25));
        setBrush(ElaThemeColor(mode, BasicBase));
        hide();
    }

    void applyThemeChrome() {
        const auto mode = eTheme->getThemeMode();
        QColor stroke = ElaThemeColor(mode, PrimaryNormal);
        setPen(QPen(stroke.darker(110), 1.25));
        setBrush(ElaThemeColor(mode, BasicBase));
    }

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override {
        if (change == ItemPositionChange && m_edge) {
            return QVariant::fromValue(m_edge->clampedHandleTopLeft(value.toPointF()));
        }
        if (change == ItemPositionHasChanged && m_edge) {
            m_edge->onBendHandleMoved(pos());
        }
        return QGraphicsEllipseItem::itemChange(change, value);
    }

private:
    GraphEdge* m_edge;
};

GraphEdge::GraphEdge(GraphNode* startNode, GraphNode* endNode, QGraphicsItem* parent)
    : ElaGraphicsItem(parent)
    , m_startNode(startNode)
    , m_endNode(endNode) {
    setZValue(-1); // 线在节点下方

    setFlag(QGraphicsItem::ItemIsMovable, false);
    setFlag(QGraphicsItem::ItemIsSelectable, true);

    setAcceptedMouseButtons(Qt::LeftButton | Qt::RightButton);
    setAcceptHoverEvents(true);

    m_weightText = new QGraphicsTextItem(this);
    m_weightText->setDefaultTextColor(QGuiApplication::palette().color(QPalette::Link));
    m_weightText->setVisible(false);

    m_bendHandle = new GraphEdgeBendHandle(this);
    QObject::connect(eTheme, &ElaTheme::themeModeChanged, this, [this](ElaThemeType::ThemeMode) {
        if (m_bendHandle) {
            m_bendHandle->applyThemeChrome();
        }
        update();
    });
}

void GraphEdge::setWeight(double w) {
    m_weight = w;

    if (m_weight != 1.0) {
        m_weightText->setDefaultTextColor(QGuiApplication::palette().color(QPalette::Link));
        m_weightText->setPlainText(QString::number(m_weight));
        m_weightText->setVisible(true);
        placeWeightLabel();
    } else {
        m_weightText->setVisible(false);
    }
}

void GraphEdge::setWeightVisible(bool visible) {
    if (m_weight != 1.0) {
        m_weightText->setVisible(visible);
    }
}

void GraphEdge::applyPathThroughBendScene(const QPointF& sceneA, const QPointF& sceneB) {
    m_bendScene = clampBendToCorridor(sceneA, sceneB, m_bendScene);
    QVector<QPointF> merged = mergeOrthoPath(orthoPolyline(sceneA, m_bendScene),
                                             orthoPolyline(m_bendScene, sceneB));
    merged = simplifyOrthoScenePolyline(merged);
    m_polylineLocal.clear();
    m_polylineLocal.reserve(merged.size());
    for (const QPointF& p : merged) {
        m_polylineLocal.append(mapFromScene(p));
    }
    if (m_polylineLocal.size() >= 2) {
        m_line = QLineF(m_polylineLocal.first(), m_polylineLocal.last());
    }
}

QPointF GraphEdge::clampedHandleTopLeft(const QPointF& handleTopLeft) const {
    if (!m_startNode || !m_endNode || !scene()) {
        return handleTopLeft;
    }
    const QPointF sceneA = m_startNode->connectionAnchorToward(
        m_endNode->sceneBoundingRect().center());
    const QPointF sceneB = m_endNode->connectionAnchorToward(
        m_startNode->sceneBoundingRect().center());
    const QPointF centerLocal = handleTopLeft + QPointF(kHandleRadius, kHandleRadius);
    const QPointF bend = clampBendToCorridor(sceneA, sceneB, mapToScene(centerLocal));
    return mapFromScene(bend) - QPointF(kHandleRadius, kHandleRadius);
}

void GraphEdge::rebuildPolylineFromAnchors() {
    if (!m_startNode || !m_endNode || !scene()) {
        return;
    }
    const QPointF sceneA = m_startNode->connectionAnchorToward(
        m_endNode->sceneBoundingRect().center());
    const QPointF sceneB = m_endNode->connectionAnchorToward(
        m_startNode->sceneBoundingRect().center());

    if (!m_bendUserEdited) {
        const QVector<QPointF> autoPts = orthoPolyline(sceneA, sceneB);
        m_bendScene = (autoPts.size() >= 3) ? autoPts.at(1) : QLineF(sceneA, sceneB).pointAt(0.5);
    }

    applyPathThroughBendScene(sceneA, sceneB);
}

void GraphEdge::placeWeightLabel() {
    if (!m_weightText || m_weight == 1.0) {
        return;
    }
    const QPointF mid = midAlongPolyline(m_polylineLocal);
    m_weightText->setPos(mid
                         - QPointF(m_weightText->boundingRect().width() / 2,
                                   m_weightText->boundingRect().height() / 2));
}

void GraphEdge::syncBendHandlePos() {
    if (!m_bendHandle || !scene()) {
        return;
    }
    m_syncingBendHandle = true;
    const QPointF bl = mapFromScene(m_bendScene);
    m_bendHandle->setPos(bl - QPointF(kHandleRadius, kHandleRadius));
    m_syncingBendHandle = false;
}

void GraphEdge::onBendHandleMoved(const QPointF& handleTopLeft) {
    if (m_syncingBendHandle || !m_startNode || !m_endNode || !scene()) {
        return;
    }
    const QPointF centerLocal = handleTopLeft + QPointF(kHandleRadius, kHandleRadius);
    m_bendScene = mapToScene(centerLocal);
    m_bendUserEdited = true;

    prepareGeometryChange();
    const QPointF sceneA = m_startNode->connectionAnchorToward(
        m_endNode->sceneBoundingRect().center());
    const QPointF sceneB = m_endNode->connectionAnchorToward(
        m_startNode->sceneBoundingRect().center());
    applyPathThroughBendScene(sceneA, sceneB);
    placeWeightLabel();
    update();
}

void GraphEdge::updatePosition() {
    if (!m_startNode || !m_endNode) {
        return;
    }
    if (!scene()) {
        return;
    }

    prepareGeometryChange();
    rebuildPolylineFromAnchors();
    placeWeightLabel();
    update();
    if (isSelected()) {
        syncBendHandlePos();
    }
}

QRectF GraphEdge::boundingRect() const {
    QRectF r;
    if (m_polylineLocal.isEmpty()) {
        r = QRectF(m_line.p1(), m_line.p2()).normalized();
    } else {
        for (const QPointF& p : m_polylineLocal) {
            r |= QRectF(p, QSizeF(1, 1));
        }
    }
    if (m_weightText && m_weight != 1.0 && m_weightText->isVisible()) {
        r |= m_weightText->sceneBoundingRect();
    }
    const qreal pad = 8.0;
    return r.adjusted(-pad, -pad, pad, pad);
}

QPainterPath GraphEdge::shape() const {
    QPainterPath centerline = polylinePath(m_polylineLocal);
    if (centerline.isEmpty() && m_line.length() > 1e-6) {
        centerline.moveTo(m_line.p1());
        centerline.lineTo(m_line.p2());
    }
    QPainterPathStroker stroker;
    stroker.setWidth(kHitStrokeWidth);
    stroker.setCapStyle(Qt::RoundCap);
    stroker.setJoinStyle(Qt::RoundJoin);
    return stroker.createStroke(centerline);
}

void GraphEdge::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) {
    const QColor sceneBg = QGuiApplication::palette().color(QPalette::Base);
    const bool lightCanvas = sceneBg.lightness() > 128;
    const auto mode = eTheme->getThemeMode();
    QColor c = lightCanvas ? ElaThemeColor(mode, BasicBorderDeep)
                           : ElaThemeColor(mode, BasicBaseLine);
    c = lightCanvas ? c.darker(142) : c.lighter(150);
    if (qAbs(c.lightness() - sceneBg.lightness()) < 45) {
        c = lightCanvas ? QColor(52, 72, 92) : QColor(208, 216, 228);
    }

    painter->setRenderHint(QPainter::Antialiasing, true);

    QColor halo = c;
    halo.setAlpha(lightCanvas ? 50 : 65);
    QPen haloPen(halo, 4.25, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    painter->setPen(haloPen);
    if (m_polylineLocal.size() >= 2) {
        painter->drawPolyline(m_polylineLocal.constData(), static_cast<int>(m_polylineLocal.size()));
    } else {
        painter->drawLine(m_line);
    }

    QPen corePen(c, 1.08, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    painter->setPen(corePen);
    if (m_polylineLocal.size() >= 2) {
        painter->drawPolyline(m_polylineLocal.constData(), static_cast<int>(m_polylineLocal.size()));
    } else {
        painter->drawLine(m_line);
    }
}

QVariant GraphEdge::itemChange(GraphicsItemChange change, const QVariant& value) {
    if (change == ItemSelectedHasChanged) {
        const bool sel = value.toBool();
        if (m_bendHandle) {
            m_bendHandle->setVisible(sel);
            if (sel) {
                syncBendHandlePos();
            }
        }
    }
    return ElaGraphicsItem::itemChange(change, value);
}

void GraphEdge::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) {
    bool ok;
    double newWeight
        = QInputDialog::getDouble(nullptr, "Set Edge Weight", "Weight:", m_weight, 0, 1000, 2, &ok);
    if (ok) {
        setWeight(newWeight);
    }
    Q_UNUSED(event);
}

void GraphEdge::contextMenuEvent(QGraphicsSceneContextMenuEvent* event) {
    QMenu menu;
    QAction* editAction = menu.addAction("Edit Weight");
    QAction* resetRouteAction = menu.addAction(tr("恢复自动布线"));
    QAction* deleteAction = menu.addAction("Delete Edge");

    QAction* selected = menu.exec(event->screenPos());
    if (selected == editAction) {
        bool ok;
        double newWeight = QInputDialog::getDouble(nullptr,
                                                   "Set Edge Weight",
                                                   "Weight:",
                                                   m_weight,
                                                   0,
                                                   1000,
                                                   2,
                                                   &ok);
        if (ok) {
            setWeight(newWeight);
        }
    } else if (selected == resetRouteAction) {
        m_bendUserEdited = false;
        updatePosition();
    } else if (selected == deleteAction) {
        if (scene()) {
            auto* gscene = dynamic_cast<GraphScene*>(scene());
            if (gscene) {
                QPointer<GraphEdge> edgeGuard(this);
                QTimer::singleShot(0, gscene, [gscene, edgeGuard]() {
                    if (gscene && edgeGuard) {
                        gscene->removeEdge(edgeGuard.data());
                    }
                });
            }
        }
    }
}

void GraphEdge::setLine(const QLineF& line) {
    prepareGeometryChange();
    const QPointF lp1 = scene() ? mapFromScene(line.p1()) : line.p1();
    const QPointF lp2 = scene() ? mapFromScene(line.p2()) : line.p2();
    m_polylineLocal = orthoPolyline(lp1, lp2);
    if (m_polylineLocal.size() >= 2) {
        m_line = QLineF(m_polylineLocal.first(), m_polylineLocal.last());
    } else {
        m_line = QLineF(lp1, lp2);
    }
    placeWeightLabel();
    update();
}
