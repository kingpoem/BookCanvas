#pragma once

#include "view/AboutPage.h"
#include "view/BookSimResultPage.h"
#include "view/CanvasPage.h"
#include "view/GlobalConfigPage.h"
#include "view/RecordVizPage.h"
#include "view/SettingPage.h"
#include "view/SimulationPage.h"
#include "view/SimulationRecordPage.h"
#include "view/UsageGuidePage.h"
#include <ElaWindow.h>

class MainWindow : public ElaWindow {
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

    void initWindow();
    void initContent();
    void initModel();

    [[nodiscard]] bool isCanvasPageActive() const;
    [[nodiscard]] bool isSimulationPageActive() const;

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    // pages
    CanvasPage* canvasPage{};
    GlobalConfigPage* globalConfigPage{};
    SimulationPage* simulationPage{};
    SimulationRecordPage* simulationRecordPage{};
    BookSimResultPage* bookSimResultPage{};
    RecordVizPage* recordVizPage{};
    UsageGuidePage* usageGuidePage{};
    SettingPage* settingPage{};
    AboutPage* aboutPage{};

    // models
};
