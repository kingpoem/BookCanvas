#pragma once
#include "ElaGraphicsItem.h"
#include <QGraphicsSceneMouseEvent>
#include <QRectF>
#include <QString>

class GraphNode : public ElaGraphicsItem {
    Q_OBJECT
public:
    explicit GraphNode(const QString& id, QGraphicsItem* parent = nullptr);

    [[nodiscard]] QRectF boundingRect() const override; // 定义节点边界矩形
    void paint(QPainter* painter,
               const QStyleOptionGraphicsItem* option,
               QWidget* widget) override; // 绘制节点外观 QGraphicsItem 虚函数 刷新界面时自动调用

    [[nodiscard]] QPointF getPortPosition(int portIndex) const; // 根据端口索引获取节点上的连接点坐标
                                                                // portIndex = 0 → 左侧中点
                                                                // portIndex = 1 → 右侧中点
                                                                // 其他值 → 节点中心
    void setLabel(const QString& label) { m_label = label; }    // 获取节点的 label
    QString getLabel() const { return m_label; };

    QString getId() const { return m_id; };

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;   // 鼠标按下事件，用于开始拖拽
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;    // 鼠标移动事件，用于拖拽节点
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override; // 鼠标释放事件，用于结束拖拽

private:
    QString m_id;                         // 节点唯一标识符
    QString m_label;                      // 节点显示文本（标签）
    QRectF m_rect = QRectF(0, 0, 80, 50); // 节点矩形
    bool m_dragging = false;              // 是否处于拖拽状态
    QPointF m_dragStartPos;               // 拖拽起始点
};
