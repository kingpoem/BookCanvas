#include "GraphNode.h"
#include "utils/CanvasDebugLog.h"
#include <QDebug>
#include <QFont>
#include <QGraphicsSceneContextMenuEvent>
#include <QGuiApplication>
#include <QMenu>
#include <QPainter>
#include <QStyleOptionGraphicsItem>

namespace {

QString abbrevForId(const QString& id, GraphNode::NodeType type) {
    if (type == GraphNode::Router && id.startsWith(QLatin1String("Router_"))) {
        return QLatin1String("R") + id.mid(7);
    }
    if (type == GraphNode::Node && id.startsWith(QLatin1String("Node_"))) {
        return QLatin1String("T") + id.mid(5);
    }
    return id.size() > 5 ? id.left(5) : id;
}

} // namespace

GraphNode::GraphNode(QString id, NodeType type, QGraphicsItem* parent)
    : ElaGraphicsItem(parent)
    , m_id(std::move(id))
    , m_type(type) {
    setFlags(ItemIsMovable | ItemIsSelectable
             | ItemSendsGeometryChanges); // 可移动 选中 报告几何变化

    setAcceptedMouseButtons(Qt::LeftButton | Qt::RightButton);
}

QRectF GraphNode::boundingRect() const {
    return m_rect;
}

void GraphNode::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
    painter->setRenderHint(QPainter::Antialiasing, true);

    const QPalette& pal = widget ? widget->palette() : QGuiApplication::palette();
    const QColor baseBg = pal.color(QPalette::Base);
    const bool dark = baseBg.lightness() < 140;

    QColor termFill = dark ? QColor(0x3d4d68) : QColor(0xd8, 0xdf, 0xf2);
    QColor routFill = dark ? QColor(0x3d5c48) : QColor(0xd0, 0xe5, 0xd5);
    QColor stroke = dark ? QColor(0xa8, 0xb8, 0xd0) : QColor(0x45, 0x50, 0x60);

    const QColor labelColor(0, 0, 0);

    int penW = 2;
    if (m_state == Highlighted) {
        stroke = QColor(0xff, 0x8a, 0x65);
        penW = 3;
        termFill = termFill.lighter(125);
        routFill = routFill.lighter(125);
    } else if (option->state & QStyle::State_Selected) {
        stroke = pal.color(QPalette::Highlight);
        penW = 3;
    }

    QPen outline(stroke, penW);

    if (m_type == Router) {
        const QRectF rr = m_rect.adjusted(6, 6, -6, -6);
        painter->setPen(Qt::NoPen);
        painter->setBrush(routFill);
        painter->drawRoundedRect(rr, 6, 6);

        painter->setBrush(Qt::NoBrush);
        painter->setPen(outline);
        painter->drawRoundedRect(rr, 6, 6);

        const qreal stub = 5;
        const qreal half = 1.0;
        painter->setPen(QPen(stroke, half + 0.5));
        painter->drawLine(
            QLineF(rr.center().x() - stub, rr.top(), rr.center().x() + stub, rr.top()));
        painter->drawLine(
            QLineF(rr.center().x() - stub, rr.bottom(), rr.center().x() + stub, rr.bottom()));
        painter->drawLine(
            QLineF(rr.left(), rr.center().y() - stub, rr.left(), rr.center().y() + stub));
        painter->drawLine(
            QLineF(rr.right(), rr.center().y() - stub, rr.right(), rr.center().y() + stub));
    } else {
        const QRectF rr = m_rect.adjusted(6, 6, -6, -14);
        painter->setPen(Qt::NoPen);
        painter->setBrush(termFill);
        painter->drawRoundedRect(rr, 8, 8);

        painter->setBrush(Qt::NoBrush);
        painter->setPen(outline);
        painter->drawRoundedRect(rr, 8, 8);

        const qreal pinW = 4;
        const qreal pinH = 5;
        const qreal pinTop = rr.bottom() + 1;
        const qreal cx = rr.center().x();
        painter->setPen(Qt::NoPen);
        painter->setBrush(stroke.darker(dark ? 115 : 85));
        painter->drawRect(QRectF(cx - 12 - pinW / 2, pinTop, pinW, pinH));
        painter->drawRect(QRectF(cx - pinW / 2, pinTop, pinW, pinH));
        painter->drawRect(QRectF(cx + 12 - pinW / 2, pinTop, pinW, pinH));
    }

    painter->setPen(QPen(labelColor, 1));
    QFont font = painter->font();
    font.setPointSize(8);
    font.setBold(true);
    painter->setFont(font);
    const QString abbrev = abbrevForId(m_id, m_type);
    painter->drawText(m_rect.adjusted(0, 2, 0, -12), Qt::AlignHCenter | Qt::AlignTop, abbrev);
}

// 捕捉选中状态变化
QVariant GraphNode::itemChange(GraphicsItemChange change, const QVariant& value) {
    if (change == ItemSelectedChange) {
        emit posChanged(this->pos(), this->scenePos());
        bool selected = value.toBool();
        if (selected) {
            qDebug() << "节点被选中:" << m_id;
        } else {
            qDebug() << "节点取消选中:" << m_id;
        }
    }
    return ElaGraphicsItem::itemChange(change, value);
}

// 右键菜单事件
void GraphNode::contextMenuEvent(QGraphicsSceneContextMenuEvent* event) {
    QMenu menu;
    QAction* configAction = nullptr;
    if (m_type == Router) {
        configAction = menu.addAction(tr("配置参数"));
    }
    QAction* deleteAction = menu.addAction(tr("删除"));

    QAction* selected = menu.exec(event->screenPos());
    if (selected == configAction) {
        emit configureRequested(this);
    } else if (selected == deleteAction) {
        canvasDebugLog(QStringLiteral("GraphNode.cpp: menu 删除 id=%1 ptr=0x%2 before emit")
                           .arg(m_id)
                           .arg(quintptr(this), 0, 16));
        emit deleteRequested(this);
        canvasDebugLog(QStringLiteral("GraphNode.cpp: after deleteRequested emit (same stack)"));
    }
}
