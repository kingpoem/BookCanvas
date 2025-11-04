#pragma once

#include "view/AboutPage.h"
#include "view/CanvasPage.h"
#include "view/SettingPage.h"
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
    SettingPage* settingPage{};
    AboutPage* aboutPage{};

    // models
};
