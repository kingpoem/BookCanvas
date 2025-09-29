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

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;   // 鼠标按下事件，用于开始拖拽
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;    // 鼠标移动事件，用于拖拽节点
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override; // 鼠标释放事件，用于结束拖拽

private:
    QString m_id;                         // 节点唯一标识符
    QRectF m_rect = QRectF(0, 0, 50, 50); // 节点形状
    bool m_dragging = false;              // 是否处于拖拽状态
    QPointF m_dragStartPos;               // 拖拽起始点
};
