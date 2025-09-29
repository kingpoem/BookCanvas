#pragma once
#include "ElaGraphicsItem.h"
#include <QGraphicsSceneMouseEvent>
#include <QRectF>
#include <QString>

class GraphNode : public ElaGraphicsItem {
    Q_OBJECT
public:
    explicit GraphNode(QString id, QGraphicsItem* parent = nullptr);

    [[nodiscard]] QRectF boundingRect() const override; // 定义节点边界矩形
    void paint(QPainter* painter,
               const QStyleOptionGraphicsItem* option,
               QWidget* widget) override; // 绘制节点外观 QGraphicsItem 虚函数 刷新界面时自动调用

    [[nodiscard]] QString getId() const { return m_id; };

signals:
    void posChanged(QPointF localPos, QPointF scenePos);

private:
    QString m_id;                         // 节点唯一标识符
    QRectF m_rect = QRectF(0, 0, 50, 50); // 节点形状
};
