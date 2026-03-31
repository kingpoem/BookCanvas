#include "MainWindow.h"
#include "utils/CanvasDebugLog.h"
#include "utils/Settings.hpp"

MainWindow::MainWindow(QWidget* parent)
    : ElaWindow(parent) {
    initWindow();
    initModel();
    initContent();
    moveToCenter();
}

MainWindow::~MainWindow() {
    settings.setValue("windowSize", size());
}

void MainWindow::initWindow() {
    canvasDebugLog(QStringLiteral("=== BookCanvas session start | 删除/画布调试日志见程序目录下 "
                                  "logs/bookcanvas_canvas_debug.log "
                                  "绝对路径: %1 ===")
                       .arg(canvasDebugLogFilePath()));
    resize(settings.value("windowSize").toSize());
    setUserInfoCardVisible(false);
    setWindowTitle("BookCanvas");

    setWindowButtonFlag(ElaAppBarType::ButtonType::StayTopButtonHint, false);
}

void MainWindow::initContent() {
    canvasPage = new CanvasPage(this);
    addPageNode("Canvas", canvasPage, ElaIconType::Flag);

    simulationPage = new SimulationPage(this);
    addPageNode("Simulation", simulationPage, ElaIconType::Play);

    bookSimResultPage = new BookSimResultPage(this);
    addPageNode(tr("BookSim 结果"), bookSimResultPage, ElaIconType::ChartSimple);
    connect(simulationPage,
            &SimulationPage::simulationFinished,
            bookSimResultPage,
            &BookSimResultPage::ingestSimulationLog);

    aboutPage = new AboutPage(this);
    QString aboutPageKey;
    addFooterNode("About", aboutPage, aboutPageKey, 0, ElaIconType::CircleInfo);

    settingPage = new SettingPage(this);
    QString settingPageKey;
    addFooterNode("Setting", settingPage, settingPageKey, 0, ElaIconType::GearComplex);
}

void MainWindow::initModel() {
    // clang-format off
    // QObject::connect(diskScanner, &DiskScanner::fileCreated, mediaModel, &MediaListModel::appendEntries);
    // QObject::connect(diskScanner, &DiskScanner::fileDeleted, mediaModel, &MediaListModel::removeEntries);
    // QObject::connect(diskScanner, &DiskScanner::fileModified, mediaModel, &MediaListModel::modifiedEntries);
    // QObject::connect(diskScanner, &DiskScanner::fullScan, mediaModel, &MediaListModel::resetEntries);
    // clang-format on
}
