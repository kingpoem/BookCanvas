#pragma once
#include "ElaGraphicsView.h"
#include "GraphScene.h"

class GraphView : public ElaGraphicsView {
    Q_OBJECT
public:
    explicit GraphView(GraphScene* scene, QWidget* parent = nullptr); // GraphScene 由外部控制

protected:
    void wheelEvent(QWheelEvent* event) override; // 支持缩放
};
