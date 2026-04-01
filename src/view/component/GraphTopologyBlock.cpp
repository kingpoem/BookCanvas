#include "GraphTopologyBlock.h"
#include "utils/CanvasDebugLog.h"
#include <QGraphicsSceneContextMenuEvent>
#include <QGuiApplication>
#include <QMenu>
#include <QPainter>
#include <QStyleOptionGraphicsItem>

GraphTopologyBlock::GraphTopologyBlock(QString blockId,
                                       BooksimTopologyParams params,
                                       QGraphicsItem* parent)
    : ElaGraphicsItem(parent)
    , m_blockId(std::move(blockId))
    , m_params(std::move(params)) {
    setFlags(ItemIsMovable | ItemIsSelectable | ItemSendsGeometryChanges);
    setAcceptedMouseButtons(Qt::LeftButton | Qt::RightButton);
}

void GraphTopologyBlock::setParams(const BooksimTopologyParams& p) {
    m_params = p;
    update();
}

QRectF GraphTopologyBlock::boundingRect() const {
    return m_rect;
}

void GraphTopologyBlock::paint(QPainter* painter,
                               const QStyleOptionGraphicsItem* option,
                               QWidget* widget) {
    painter->setRenderHint(QPainter::Antialiasing, true);

    const QPalette& pal = widget ? widget->palette() : QGuiApplication::palette();
    const QColor baseBg = pal.color(QPalette::Base);
    const bool dark = baseBg.lightness() < 140;

    QColor fill = dark ? QColor(0x4a3d68) : QColor(0xe8, 0xe0, 0xf5);
    QColor stroke = dark ? QColor(0xc0, 0xa8, 0xe8) : QColor(0x5c, 0x4a, 0x8a);

    int penW = 2;
    if (option->state & QStyle::State_Selected) {
        stroke = pal.color(QPalette::Highlight);
        penW = 3;
        fill = fill.lighter(dark ? 115 : 105);
    }

    const QRectF rr = m_rect.adjusted(4, 4, -4, -4);
    painter->setPen(Qt::NoPen);
    painter->setBrush(fill);
    painter->drawRoundedRect(rr, 8, 8);
    painter->setBrush(Qt::NoBrush);
    painter->setPen(QPen(stroke, penW));
    painter->drawRoundedRect(rr, 8, 8);

    painter->setPen(dark ? QColor(0xf0, 0xec, 0xff) : QColor(0x28, 0x1a, 0x46));
    QFont titleFont = painter->font();
    titleFont.setBold(true);
    titleFont.setPointSize(9);
    painter->setFont(titleFont);
    painter->drawText(rr.adjusted(8, 8, -8, 0), Qt::AlignLeft | Qt::AlignTop, m_params.displayLabel);

    QFont sub = painter->font();
    sub.setBold(false);
    sub.setPointSize(8);
    painter->setFont(sub);
    const QString line1 = QStringLiteral("BookSim · %1").arg(m_params.topologyId);
    painter->drawText(rr.adjusted(8, 26, -8, 0), Qt::AlignLeft | Qt::AlignTop, line1);

    const QString line2
        = QStringLiteral("k=%1  n=%2  c=%3").arg(m_params.k).arg(m_params.n).arg(m_params.c);
    painter->drawText(rr.adjusted(8, 42, -8, 0), Qt::AlignLeft | Qt::AlignTop, line2);

    painter->drawText(rr.adjusted(8, 58, -8, -8),
                      Qt::AlignLeft | Qt::AlignBottom,
                      tr("RF: %1").arg(m_params.routingFunction));
}

void GraphTopologyBlock::contextMenuEvent(QGraphicsSceneContextMenuEvent* event) {
    QMenu menu;
    QAction* editAction = menu.addAction(tr("编辑参数…"));
    QAction* delAction = menu.addAction(tr("删除"));
    QAction* sel = menu.exec(event->screenPos());
    if (sel == editAction) {
        emit configureRequested(this);
    } else if (sel == delAction) {
        canvasDebugLog(QStringLiteral("GraphTopologyBlock menu 删除 id=%1").arg(m_blockId));
        emit deleteRequested(this);
    }
}
