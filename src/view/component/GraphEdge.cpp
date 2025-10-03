#include "GraphEdge.h"
#include "GraphScene.h"
#include <QBrush>
#include <QInputDialog>
#include <QMenu>
#include <QPen>

GraphEdge::GraphEdge(GraphNode* startNode, GraphNode* endNode, QGraphicsItem* parent)
    : ElaGraphicsItem(parent)
    , m_startNode(startNode)
    , m_endNode(endNode) {
    setZValue(-1); // 线在节点下方

    // 禁止拖动这条边
    setFlag(QGraphicsItem::ItemIsMovable, false);
    setFlag(QGraphicsItem::ItemIsSelectable, true);

    // 右键/双击能被接受
    setAcceptedMouseButtons(Qt::LeftButton | Qt::RightButton);
    setAcceptHoverEvents(true);

    // 权重文本
    m_weightText = new QGraphicsTextItem(this);
    m_weightText->setDefaultTextColor(Qt::red);
    m_weightText->setVisible(false);

    // scene::createEdge addItem 后调用
    // updatePosition();
}

void GraphEdge::setWeight(double w) {
    m_weight = w;

    if (m_weight != 1.0) {
        m_weightText->setPlainText(QString::number(m_weight));
        m_weightText->setVisible(true);

        // 设置文本位置在线段中点
        // mid 是局部坐标
        QPointF mid = (m_line.p1() + m_line.p2()) / 2;
        m_weightText->setPos(mid
                             - QPointF(m_weightText->boundingRect().width() / 2,
                                       m_weightText->boundingRect().height() / 2));
    } else {
        m_weightText->setVisible(false);
    }
}

void GraphEdge::setWeightVisible(bool visible) {
    if (m_weight != 1.0) {
        m_weightText->setVisible(visible);
    }
}

void GraphEdge::updatePosition() {
    if (!m_startNode || !m_endNode) {
        return;
    }
    if (!scene()) {
        return;
    }

    // 用节点的 圆心（scene 坐标）
    QPointF sceneP1 = m_startNode->sceneBoundingRect().center();
    QPointF sceneP2 = m_endNode->sceneBoundingRect().center();

    // 将 scene 坐标映射为本 item 的局部坐标
    QPointF localP1 = mapFromScene(sceneP1);
    QPointF localP2 = mapFromScene(sceneP2);

    prepareGeometryChange();

    // m_line = QLineF(m_startNode->sceneBoundingRect().center(),
    //                 m_endNode->sceneBoundingRect().center());
    m_line = QLineF(localP1, localP2);

    // 更新权重位置
    if (m_weight != 1.0 && m_weightText) {
        QPointF mid = (m_line.p1() + m_line.p2()) / 2;
        m_weightText->setPos(mid
                             - QPointF(m_weightText->boundingRect().width() / 2,
                                       m_weightText->boundingRect().height() / 2));
    }

    update();
    // prepareGeometryChange(); // 通知Qt边界更新
}

QRectF GraphEdge::boundingRect() const {
    // m_line 已经是局部坐标
    QRectF r = QRectF(m_line.p1(), m_line.p2()).normalized();
    // 扩一点以容纳 pen 宽度与点击容差
    const qreal pad = 4.0;
    return r.adjusted(-pad, -pad, pad, pad);
}

void GraphEdge::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) {
    painter->setPen(QPen(Qt::black, 2));
    painter->drawLine(m_line);
}
void GraphEdge::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) {
    bool ok;
    double newWeight
        = QInputDialog::getDouble(nullptr, "Set Edge Weight", "Weight:", m_weight, 0, 1000, 2, &ok);
    if (ok) {
        setWeight(newWeight);
    }
}

void GraphEdge::contextMenuEvent(QGraphicsSceneContextMenuEvent* event) {
    QMenu menu;
    QAction* editAction = menu.addAction("Edit Weight");
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
        if (ok)
            setWeight(newWeight);
    } else if (selected == deleteAction) {
        if (scene()) {
            auto* gscene = dynamic_cast<GraphScene*>(scene());
            if (gscene)
                gscene->removeEdge(this);
        }
    }
}

void GraphEdge::setLine(const QLineF& line) {
    if (scene()) {
        QPointF p1 = mapFromScene(line.p1());
        QPointF p2 = mapFromScene(line.p2());
        prepareGeometryChange();
        m_line = QLineF(p1, p2);
    } else {
        m_line = line;
    }
    update();
}
