#include "GraphEdge.h"
#include <QBrush>
#include <QInputDialog>
#include <QPen>

GraphEdge::GraphEdge(GraphNode* startNode, GraphNode* endNode, QGraphicsItem* parent)
    : ElaGraphicsItem(parent)
    , m_startNode(startNode)
    , m_endNode(endNode) {
    setZValue(-1); // 线在节点下方

    // 权重文本
    m_weightText = new QGraphicsTextItem(this);
    m_weightText->setDefaultTextColor(Qt::red);
    m_weightText->setVisible(false);

    updatePosition();
}

void GraphEdge::setWeight(double w) {
    m_weight = w;

    if (m_weight != 1.0) {
        m_weightText->setPlainText(QString::number(m_weight));
        m_weightText->setVisible(true);

        // 设置文本位置在线段中点
        QPointF mid = (m_line.p1() + m_line.p2()) / 2;
        m_weightText->setPos(mid
                             - QPointF(m_weightText->boundingRect().width() / 2,
                                       m_weightText->boundingRect().height() / 2));
    } else {
        m_weightText->setVisible(false);
    }
}

void GraphEdge::updatePosition() {
    if (!m_startNode || !m_endNode)
        return;

    m_line = QLineF(m_startNode->sceneBoundingRect().center(),
                    m_endNode->sceneBoundingRect().center());

    // 更新权重位置
    if (m_weight != 1.0) {
        QPointF mid = (m_line.p1() + m_line.p2()) / 2;
        m_weightText->setPos(mid
                             - QPointF(m_weightText->boundingRect().width() / 2,
                                       m_weightText->boundingRect().height() / 2));
    }

    prepareGeometryChange(); // 通知Qt边界更新
}

QRectF GraphEdge::boundingRect() const {
    return QRectF(m_line.p1(), m_line.p2()).normalized().adjusted(-2, -2, 2, 2);
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
