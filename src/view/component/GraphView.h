#pragma once
#include "ElaGraphicsView.h"
#include "GraphScene.h"
#include <QLabel>

class GraphView : public ElaGraphicsView {
    Q_OBJECT
public:
    explicit GraphView(GraphScene* scene,
                       QLabel* labelX,
                       QLabel* labelY,
                       QLabel* labelSceneX,
                       QLabel* labelSceneY,
                       QWidget* parent = nullptr); // GraphScene 由外部控制

    explicit GraphView(GraphScene* scene, QWidget* parent = nullptr);

protected:
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void drawBackground(QPainter* painter, const QRectF& rect) override;
    void mouseMoveEvent(QMouseEvent* event) override;

private:
    double m_scale = 1.0;
    const double m_minScale = 0.2;
    const double m_maxScale = 3.0;
    QLabel* m_labelX;
    QLabel* m_labelY;
    QLabel* m_labelSceneX;
    QLabel* m_labelSceneY;
};
