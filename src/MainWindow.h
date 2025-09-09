#pragma once

#include <ElaWindow.h>
#include "view/SettingPage.h"
#include "view/AboutPage.h"

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
    SettingPage* settingPage {};
    AboutPage* aboutPage {};

    // models

};

