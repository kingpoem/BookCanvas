#pragma once
#include "CanvasScene.h"
#include "ElaGraphicsView.h"
#include "GraphScene.h"

class CanvasView : public ElaGraphicsView {
    Q_OBJECT
public:
    explicit CanvasView(QWidget* parent = nullptr);
    explicit CanvasView(CanvasScene* scene, QWidget* parent = nullptr);
    explicit CanvasView(GraphScene* scene, QWidget* parent = nullptr);

protected:
    void wheelEvent(QWheelEvent* event) override; // 支持缩放
};
