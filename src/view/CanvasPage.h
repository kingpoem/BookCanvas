#pragma once

#include "BasePage.h"
#include <QMap>

class QHideEvent;
class QShowEvent;
class QTabWidget;
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
    [[nodiscard]] bool exportTopologySilently(QString* errorMessage = nullptr);
    [[nodiscard]] bool exportConfigJsonSilently(QString* errorMessage = nullptr);
    [[nodiscard]] QString currentTopologyExportPath() const;
    [[nodiscard]] QString currentConfigExportPath() const;
    /// 与当前页导出到 BookSim 的 JSON 顶层参数一致（含单拓扑块的 k/n/c/routing 等），供仿真记录存档
    [[nodiscard]] QMap<QString, QString> mergedBooksimConfigForSimulationRecord() const;

protected:
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    [[nodiscard]] static QString tabNameSettingKey(const QString& scopeToken);
    [[nodiscard]] static QString loadPersistedTabName(const QString& scopeToken,
                                                      const QString& fallbackName);
    static void savePersistedTabName(const QString& scopeToken, const QString& name);
    static void removePersistedTabName(const QString& scopeToken);
    void createCanvasTab();
    void closeCanvasTab(int index);
    void refreshCurrentCanvasContext();
    [[nodiscard]] GraphScene* currentScene() const;
    [[nodiscard]] GraphView* currentGraphView() const;
    [[nodiscard]] QString currentTabScopeToken() const;
    void clearPlaceMode();

    QTabWidget* m_canvasTabs = nullptr;
    GraphScene* m_scene = nullptr;
    GraphView* m_graphView = nullptr;
    ElaIconButton* m_placeTermPick = nullptr;
    ElaIconButton* m_placeRouterPick = nullptr;
    QWidget* m_leftBuildPanel = nullptr;
    int m_nextCanvasTabId = 1;
};
