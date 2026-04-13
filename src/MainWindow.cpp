#include "MainWindow.h"
#include "utils/CanvasDebugLog.h"
#include "utils/Settings.hpp"
#include <QAction>
#include <QApplication>
#include <QEvent>
#include <QKeyEvent>
#include <QKeySequence>
#include <QStringList>
#include <QTimer>

namespace {

bool isCtrlTabCycleKey(const QKeyEvent* event, bool* backward) {
    if (!event || !backward) {
        return false;
    }
    const auto mods = event->modifiers();
#ifdef Q_OS_MACOS
    const bool hasPrimaryModifier = mods.testFlag(Qt::ControlModifier)
                                    || mods.testFlag(Qt::MetaModifier);
#else
    const bool hasPrimaryModifier = mods.testFlag(Qt::ControlModifier);
#endif
    if (!hasPrimaryModifier || mods.testFlag(Qt::AltModifier)) {
        return false;
    }
    const int key = event->key();
    if (key == Qt::Key_Backtab) {
        *backward = true;
        return true;
    }
    if (key == Qt::Key_Tab) {
        *backward = mods.testFlag(Qt::ShiftModifier);
        return true;
    }
    return false;
}

QString modsToString(Qt::KeyboardModifiers mods) {
    QStringList parts;
    if (mods.testFlag(Qt::ControlModifier)) {
        parts << QStringLiteral("Ctrl");
    }
    if (mods.testFlag(Qt::MetaModifier)) {
        parts << QStringLiteral("Meta");
    }
    if (mods.testFlag(Qt::AltModifier)) {
        parts << QStringLiteral("Alt");
    }
    if (mods.testFlag(Qt::ShiftModifier)) {
        parts << QStringLiteral("Shift");
    }
    return parts.isEmpty() ? QStringLiteral("None") : parts.join(QStringLiteral("+"));
}

QString eventTypeToString(QEvent::Type t) {
    if (t == QEvent::ShortcutOverride) {
        return QStringLiteral("ShortcutOverride");
    }
    if (t == QEvent::KeyPress) {
        return QStringLiteral("KeyPress");
    }
    return QString::number(static_cast<int>(t));
}

} // namespace

MainWindow::MainWindow(QWidget* parent)
    : ElaWindow(parent) {
    initWindow();
    initModel();
    initContent();
    qApp->installEventFilter(this);
    moveToCenter();
}

MainWindow::~MainWindow() {
    qApp->removeEventFilter(this);
    settings.setValue("windowSize", size());
}

bool MainWindow::isCanvasPageActive() const {
    if (!canvasPage) {
        return false;
    }
    const QString key = canvasPage->property("ElaPageKey").toString();
    return !key.isEmpty() && key == getCurrentNavigationPageKey();
}

bool MainWindow::isSimulationPageActive() const {
    if (!simulationPage) {
        return false;
    }
    const QString key = simulationPage->property("ElaPageKey").toString();
    return !key.isEmpty() && key == getCurrentNavigationPageKey();
}

bool MainWindow::eventFilter(QObject* watched, QEvent* event) {
    if (!event || (!canvasPage && !simulationPage)) {
        return ElaWindow::eventFilter(watched, event);
    }
    if (event->type() == QEvent::ShortcutOverride || event->type() == QEvent::KeyPress) {
        auto* keyEvent = dynamic_cast<QKeyEvent*>(event);
        if (keyEvent && (keyEvent->key() == Qt::Key_Tab || keyEvent->key() == Qt::Key_Backtab)) {
            QWidget* fw = qApp ? qApp->focusWidget() : nullptr;
            canvasDebugLog(
                QStringLiteral(
                    "[ctrl-tab-debug] recv type=%1 key=%2 mods=%3 accepted=%4 canvasActive=%5 "
                    "watched=%6(%7) focus=%8(%9)")
                    .arg(eventTypeToString(event->type()),
                         QString::number(keyEvent->key()),
                         modsToString(keyEvent->modifiers()),
                         keyEvent->isAccepted() ? QStringLiteral("true") : QStringLiteral("false"),
                         isCanvasPageActive() ? QStringLiteral("true") : QStringLiteral("false"),
                         watched ? watched->metaObject()->className() : "null",
                         watched ? watched->objectName() : "",
                         fw ? fw->metaObject()->className() : "null",
                         fw ? fw->objectName() : ""));
        }
        const bool canvasActive = isCanvasPageActive();
        const bool simulationActive = isSimulationPageActive();
        if (!canvasActive && !simulationActive) {
            return ElaWindow::eventFilter(watched, event);
        }
        bool backward = false;
        if (isCtrlTabCycleKey(keyEvent, &backward)) {
            if (event->type() == QEvent::ShortcutOverride) {
                canvasDebugLog(
                    QStringLiteral("[ctrl-tab-debug] reserve+trigger shortcut backward=%1")
                        .arg(backward));
                // 在 macOS 上该组合常只到 ShortcutOverride，不会再进入 KeyPress；
                // 因此在此阶段异步触发，确保与点击 ◀ ▶ 按钮同路径执行。
                QTimer::singleShot(0, this, [this, backward]() {
                    if (isCanvasPageActive() && canvasPage) {
                        canvasDebugLog(
                            QStringLiteral(
                                "[ctrl-tab-debug] trigger from ShortcutOverride backward=%1")
                                .arg(backward));
                        canvasPage->triggerNetworkTabNavigateClick(backward);
                    } else if (isSimulationPageActive() && simulationPage) {
                        simulationPage->activateAdjacentSimulationTab(backward);
                    }
                });
                keyEvent->accept();
                return true;
            }
            if (canvasActive && canvasPage) {
                canvasDebugLog(QStringLiteral("[ctrl-tab-debug] trigger canvas click backward=%1")
                                   .arg(backward));
                canvasPage->triggerNetworkTabNavigateClick(backward);
                canvasDebugLog(
                    QStringLiteral("[ctrl-tab-debug] trigger done backward=%1 accepted=true")
                        .arg(backward));
            } else if (simulationActive && simulationPage) {
                simulationPage->activateAdjacentSimulationTab(backward);
            }
            keyEvent->accept();
            return true;
        }
    }
    return ElaWindow::eventFilter(watched, event);
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

    {
        // 网络 Tab：Ctrl+Tab 在本窗口以 ApplicationShortcut 注册（与关闭 Tab 一致），
        // 仅当 Canvas 为当前导航页时生效。

        auto* canvasNextNetworkTabAct = new QAction(this);
        canvasNextNetworkTabAct->setShortcutContext(Qt::ApplicationShortcut);
        canvasNextNetworkTabAct->setShortcuts(
            {QKeySequence(QKeySequence::NextChild), QKeySequence(Qt::CTRL | Qt::Key_Tab)});
        connect(canvasNextNetworkTabAct, &QAction::triggered, this, [this]() {
            if (isCanvasPageActive()) {
                canvasPage->triggerNetworkTabNavigateClick(false);
            } else if (isSimulationPageActive() && simulationPage) {
                simulationPage->activateAdjacentSimulationTab(false);
            }
        });
        addAction(canvasNextNetworkTabAct);

        auto* canvasPrevNetworkTabAct = new QAction(this);
        canvasPrevNetworkTabAct->setShortcutContext(Qt::ApplicationShortcut);
        canvasPrevNetworkTabAct->setShortcuts({QKeySequence(QKeySequence::PreviousChild),
                                               QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Tab),
                                               QKeySequence(Qt::CTRL | Qt::Key_Backtab)});
        connect(canvasPrevNetworkTabAct, &QAction::triggered, this, [this]() {
            if (isCanvasPageActive()) {
                canvasPage->triggerNetworkTabNavigateClick(true);
            } else if (isSimulationPageActive() && simulationPage) {
                simulationPage->activateAdjacentSimulationTab(true);
            }
        });
        addAction(canvasPrevNetworkTabAct);

        auto* closeCanvasTabAct = new QAction(this);
        closeCanvasTabAct->setShortcutContext(Qt::ApplicationShortcut);
#ifdef Q_OS_MACOS
        closeCanvasTabAct->setShortcuts({QKeySequence::Close, QKeySequence(Qt::META | Qt::Key_W)});
#else
        closeCanvasTabAct->setShortcuts({QKeySequence(Qt::CTRL | Qt::Key_W), QKeySequence::Close});
#endif
        connect(closeCanvasTabAct, &QAction::triggered, this, [this]() {
            if (isCanvasPageActive()) {
                canvasPage->closeCurrentCanvasTab();
            } else if (isSimulationPageActive() && simulationPage) {
                simulationPage->closeCurrentSimulationTab();
            }
        });
        addAction(closeCanvasTabAct);
    }

    simulationPage = new SimulationPage(this);
    addPageNode("Simulation", simulationPage, ElaIconType::Play);

    const QString resultPageTitle = tr("BookSim 结果");
    bookSimResultPage = new BookSimResultPage(this);
    addPageNode(resultPageTitle, bookSimResultPage, ElaIconType::ChartSimple);

    simulationRecordPage = new SimulationRecordPage(this);
    addPageNode(tr("仿真记录"), simulationRecordPage, ElaIconType::ClockRotateLeft);

    const QString recordVizPageTitle = tr("结果可视化");
    recordVizPage = new RecordVizPage(this);
    addPageNode(recordVizPageTitle, recordVizPage, ElaIconType::ChartLine);
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
    connect(simulationRecordPage,
            &SimulationRecordPage::showLineChartInResultRequested,
            this,
            [this, recordVizPageTitle](const QList<SimulationRecordSnapshot>& records,
                                       const QString& metricAKey,
                                       const QString& metricALabel,
                                       const QString& metricBKey,
                                       const QString& metricBLabel) {
                recordVizPage->ingestRecordLineChart(records,
                                                     metricAKey,
                                                     metricALabel,
                                                     metricBKey,
                                                     metricBLabel);
                navigation(recordVizPageTitle);
            });
    connect(simulationRecordPage,
            &SimulationRecordPage::showScatter3DInResultRequested,
            this,
            [this, recordVizPageTitle](const QList<SimulationRecordSnapshot>& records,
                                       const QString& metricAKey,
                                       const QString& metricALabel,
                                       const QString& metricBKey,
                                       const QString& metricBLabel,
                                       const QString& metricCKey,
                                       const QString& metricCLabel) {
                recordVizPage->ingestRecordScatter3D(records,
                                                     metricAKey,
                                                     metricALabel,
                                                     metricBKey,
                                                     metricBLabel,
                                                     metricCKey,
                                                     metricCLabel);
                navigation(recordVizPageTitle);
            });

    globalConfigPage = new GlobalConfigPage(this);
    addPageNode(tr("全局配置"), globalConfigPage, ElaIconType::CarWrench);
    globalConfigPage->setConfig(canvasPage->globalConfig());
    simulationPage->setSaveContext(canvasPage, globalConfigPage);

    usageGuidePage = new UsageGuidePage(this);
    addPageNode(tr("使用说明"), usageGuidePage, ElaIconType::BookOpen);

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
