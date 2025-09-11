#include "MainWindow.h"
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
    resize(settings.value("windowSize").toSize());
    setUserInfoCardVisible(false);
    setWindowTitle("BookCanvas");

    setWindowButtonFlag(ElaAppBarType::ButtonType::StayTopButtonHint, false);
}

void MainWindow::initContent() {
    // pages
    canvasPage = new CanvasPage(this);
    addPageNode("Canvas", canvasPage, ElaIconType::Flag);

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
