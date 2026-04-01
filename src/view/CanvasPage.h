#pragma once

#include "BasePage.h"
#include <QMap>

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
    [[nodiscard]] QMap<QString, QString> globalConfig() const;
    void setGlobalConfig(const QMap<QString, QString>& config);
    void exportConfigJson();

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
    QWidget* m_leftBuildPanel = nullptr;
};
