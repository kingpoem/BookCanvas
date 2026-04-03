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

    const QString resultPageTitle = tr("BookSim 结果");
    bookSimResultPage = new BookSimResultPage(this);
    addPageNode(resultPageTitle, bookSimResultPage, ElaIconType::ChartSimple);

    simulationRecordPage = new SimulationRecordPage(this);
    addPageNode(tr("仿真记录"), simulationRecordPage, ElaIconType::ClockRotateLeft);
    connect(simulationPage,
            &SimulationPage::simulationFinished,
            bookSimResultPage,
            &BookSimResultPage::ingestSimulationLog);
    connect(simulationPage,
            &SimulationPage::simulationFinishedWithContext,
            simulationRecordPage,
            &SimulationRecordPage::appendRecord);
    connect(simulationRecordPage,
            &SimulationRecordPage::showRecordInResultRequested,
            this,
            [this, resultPageTitle](const QString& log) {
                bookSimResultPage->ingestSimulationLog(log);
                navigation(resultPageTitle);
            });

    globalConfigPage = new GlobalConfigPage(this);
    addPageNode(tr("全局配置"), globalConfigPage, ElaIconType::CarWrench);
    globalConfigPage->setConfig(canvasPage->globalConfig());
    simulationPage->setSaveContext(canvasPage, globalConfigPage);

    usageGuidePage = new UsageGuidePage(this);
    addPageNode(tr("使用说明"), usageGuidePage, ElaIconType::CircleInfo);

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
