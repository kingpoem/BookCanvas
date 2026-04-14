#pragma once

#include "BasePage.h"
#include <QMap>

class QHideEvent;
class QShowEvent;
class QTabWidget;
class ElaIconButton;
class GraphScene;
class GraphView;
class GlobalConfigPage;

class CanvasPage : public BasePage {
    Q_OBJECT
public:
    explicit CanvasPage(QWidget* parent = nullptr);
    ~CanvasPage() override;
    /// 与「全局配置」页绑定后，进入 Canvas 或创建芯粒前会把该页当前选项同步到各网络 Tab 的场景
    void setGlobalConfigSyncSource(GlobalConfigPage* page);
    [[nodiscard]] QMap<QString, QString> globalConfig() const;
    void setGlobalConfig(const QMap<QString, QString>& config);
    void exportConfigJson();
    [[nodiscard]] bool exportTopologySilently(QString* errorMessage = nullptr);
    [[nodiscard]] bool exportConfigJsonSilently(QString* errorMessage = nullptr);
    [[nodiscard]] QString currentTopologyExportPath() const;
    [[nodiscard]] QString currentConfigExportPath() const;
    /// 与当前页导出到 BookSim 的 JSON 顶层参数一致（含单拓扑块的 k/n/c/routing 等），供仿真记录存档
    [[nodiscard]] QMap<QString, QString> mergedBooksimConfigForSimulationRecord() const;
    [[nodiscard]] int canvasTabCount() const;
    [[nodiscard]] int currentCanvasTabIndex() const;
    [[nodiscard]] QString canvasTabTitle(int zeroBasedIndex) const;

    void activateAdjacentCanvasTab(bool backward);
    void activateCanvasTabByIndex(int zeroBasedIndex);
    void closeCurrentCanvasTab();
    /// 与点击 Tab 栏 ◀ ▶ 相同（主窗口 ApplicationShortcut 调此方法，确保走按钮 `click()`）
    void triggerNetworkTabNavigateClick(bool backward);

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
    /// 将「全局配置」切到 chiplet_mesh 并下发到所有网络 Tab 的场景
    void ensureChipletMeshGlobalConfig();
    void installNetworkTabKeyboardShortcuts();
    void updateCanvasTabNavigateButtons();
    void clickCanvasTabNavigateButton(bool backward);
    void previewCurrentNetworkConfigJson();
    void previewCurrentNetworkTopologyFile();
    void pullGlobalConfigIntoAllScenes();

    QTabWidget* m_canvasTabs = nullptr;
    GraphScene* m_scene = nullptr;
    GraphView* m_graphView = nullptr;
    ElaIconButton* m_placeTermPick = nullptr;
    ElaIconButton* m_placeRouterPick = nullptr;
    ElaIconButton* m_tabPrevBtn = nullptr;
    ElaIconButton* m_tabNextBtn = nullptr;
    QWidget* m_leftBuildPanel = nullptr;
    GlobalConfigPage* m_globalConfigSyncSource = nullptr;
    int m_nextCanvasTabId = 1;
};
