#pragma once

#include "BasePage.h"

class QHideEvent;
class QShowEvent;
class ElaIconButton;
class GraphScene;
class GraphView;

class CanvasPage : public BasePage {
    Q_OBJECT
public:
    explicit CanvasPage(QWidget* parent = nullptr);
    ~CanvasPage() override;

protected:
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void clearPlaceMode();

    GraphScene* m_scene = nullptr;
    GraphView* m_graphView = nullptr;
    ElaIconButton* m_placeTermPick = nullptr;
    ElaIconButton* m_placeRouterPick = nullptr;
    QWidget* m_placeTermHost = nullptr;
    QWidget* m_placeRouterHost = nullptr;
};
