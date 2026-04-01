#pragma once

#include "view/AboutPage.h"
#include "view/BookSimResultPage.h"
#include "view/CanvasPage.h"
#include "view/GlobalConfigPage.h"
#include "view/SettingPage.h"
#include "view/SimulationPage.h"
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

private:
    // pages
    CanvasPage* canvasPage{};
    GlobalConfigPage* globalConfigPage{};
    SimulationPage* simulationPage{};
    BookSimResultPage* bookSimResultPage{};
    UsageGuidePage* usageGuidePage{};
    SettingPage* settingPage{};
    AboutPage* aboutPage{};

    // models
};
