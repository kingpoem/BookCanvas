#pragma once
#include "ElaGraphicsScene.h"
#include <QBrush>
#include <QGraphicsEllipseItem>
#include <QGraphicsPolygonItem>
#include <QGraphicsRectItem>
#include <QPen>

/**
 * @class CanvasScene
 * @brief 自定义场景类，支持拖拽放置和绘制图形元素
 *
 * 处理拖拽进入、移动和放置事件
 * 根据拖拽的工具类型在场景中添加相应的图形元素
 */
class CanvasScene : public ElaGraphicsScene {
    Q_OBJECT
public:
    explicit CanvasScene(QObject* parent = nullptr);

protected:
    /**
     * @brief 拖拽进入事件处理函数
     * @param event 拖拽进入事件
     *
     * 当拖拽对象进入场景区域时触发，可用于检查拖拽的数据类型是否被接受。
     */
    void dragEnterEvent(QGraphicsSceneDragDropEvent* event) override;
    // 拖拽移动事件处理函数
    void dragMoveEvent(QGraphicsSceneDragDropEvent* event) override;
    // 拖拽离开场景
    // void dragLeaveEvent(QGraphicsSceneDragDropEvent* event) override;
    // 拖拽放置事件处理函数
    void dropEvent(QGraphicsSceneDragDropEvent* event) override;

private:
    /**
     * @brief 添加工具元素
     * @param toolName 工具名称（决定添加的图形类型，例如矩形、椭圆、多边形）
     * @param pos 放置位置（场景坐标）
     *
     * 根据传入的工具名称和位置，创建相应的 QGraphicsItem，并将其添加到场景中。
     */
    void addToolItem(const QString& toolName, const QPointF& pos);
};
