#pragma once
#include "ElaGraphicsScene.h"
#include <QBrush>
#include <QGraphicsEllipseItem>
#include <QGraphicsPolygonItem>
#include <QGraphicsRectItem>
#include <QPen>

class CanvasScene : public ElaGraphicsScene {
    Q_OBJECT
public:
    explicit CanvasScene(QObject* parent = nullptr);

protected:
    void dragEnterEvent(QGraphicsSceneDragDropEvent* event) override;
    void dragMoveEvent(QGraphicsSceneDragDropEvent* event) override;
    void dropEvent(QGraphicsSceneDragDropEvent* event) override;

private:
    void addToolItem(const QString& toolName, const QPointF& pos);
};
