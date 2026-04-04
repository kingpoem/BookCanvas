#include "SimulationPage.h"
#include "CanvasPage.h"
#include "GlobalConfigPage.h"
#include "utils/BooksimPaths.h"
#include <ElaPushButton.h>
#include <ElaScrollPageArea.h>
#include <ElaText.h>
#include <ElaTheme.h>
#include <QDir>
#include <QFileInfo>
#include <QFont>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QPalette>
#include <QPlainTextEdit>
#include <QProcess>
#include <QShortcut>
#include <QTabWidget>
#include <QTextCursor>
#include <QVBoxLayout>

SimulationPage::SimulationPage(QWidget* parent)
    : BasePage(parent)
    , m_runButton(nullptr)
    , m_runSelectedButton(nullptr)
    , m_runAllButton(nullptr)
    , m_clearButton(nullptr) {
    setWindowTitle("Simulation");

    auto* centralWidget = new QWidget(this);
    m_pageRoot = centralWidget;
    centralWidget->setWindowTitle("Simulation");
    auto* mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 20, 0);

    auto* buttonArea = new ElaScrollPageArea(this);
    auto* buttonLayout = new QHBoxLayout(buttonArea);
    buttonLayout->setContentsMargins(0, 0, 0, 0);

    m_runButton = new ElaPushButton("执行仿真", this);
    buttonLayout->addWidget(m_runButton);

    m_runSelectedButton = new ElaPushButton("执行指定网络仿真", this);
    buttonLayout->addWidget(m_runSelectedButton);

    m_runAllButton = new ElaPushButton("并行执行全部网络仿真", this);
    buttonLayout->addWidget(m_runAllButton);

    m_clearButton = new ElaPushButton("清空记录", this);
    buttonLayout->addWidget(m_clearButton);
    buttonLayout->addStretch();
    mainLayout->addWidget(buttonArea);

    m_simulationTabs = new QTabWidget(this);
    m_simulationTabs->setTabsClosable(true);
    m_simulationTabs->setMovable(true);
    m_simulationTabs->setDocumentMode(true);
    auto* addTabBtn = new ElaPushButton(QStringLiteral("+"), m_simulationTabs);
    addTabBtn->setFixedSize(28, 28);
    addTabBtn->setToolTip(tr("新建仿真Tab"));
    m_simulationTabs->setCornerWidget(addTabBtn, Qt::TopRightCorner);
    mainLayout->addWidget(m_simulationTabs, 1);

    connect(m_runButton, &ElaPushButton::clicked, this, &SimulationPage::onRunSimulation);
    connect(m_runSelectedButton,
            &ElaPushButton::clicked,
            this,
            &SimulationPage::onRunSimulationForSelectedNetwork);
    connect(m_runAllButton,
            &ElaPushButton::clicked,
            this,
            &SimulationPage::onRunSimulationForAllNetworks);
    connect(m_clearButton, &ElaPushButton::clicked, this, &SimulationPage::onClearSimulationRecord);
    connect(addTabBtn, &ElaPushButton::clicked, this, &SimulationPage::createSimulationTab);
    connect(m_simulationTabs,
            &QTabWidget::tabCloseRequested,
            this,
            &SimulationPage::closeSimulationTab);
    connect(eTheme, &ElaTheme::themeModeChanged, this, [this](ElaThemeType::ThemeMode) {
        applyTheme();
    });

    auto* newTabShortcutCtrlT = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_T),
                                              this,
                                              nullptr,
                                              nullptr,
                                              Qt::WindowShortcut);
    connect(newTabShortcutCtrlT, &QShortcut::activated, this, [this]() {
        if (isVisible()) {
            createSimulationTab();
        }
    });
#ifdef Q_OS_MACOS
    auto* newTabShortcutCommandT = new QShortcut(QKeySequence(Qt::META | Qt::Key_T),
                                                 this,
                                                 nullptr,
                                                 nullptr,
                                                 Qt::WindowShortcut);
    connect(newTabShortcutCommandT, &QShortcut::activated, this, [this]() {
        if (isVisible()) {
            createSimulationTab();
        }
    });
#endif
    auto* nextTabShortcut = new QShortcut(QKeySequence(QKeySequence::NextChild),
                                          this,
                                          nullptr,
                                          nullptr,
                                          Qt::WindowShortcut);
    connect(nextTabShortcut, &QShortcut::activated, this, [this]() {
        if (isVisible()) {
            activateAdjacentSimulationTab(false);
        }
    });
    auto* prevTabShortcut = new QShortcut(QKeySequence(QKeySequence::PreviousChild),
                                          this,
                                          nullptr,
                                          nullptr,
                                          Qt::WindowShortcut);
    connect(prevTabShortcut, &QShortcut::activated, this, [this]() {
        if (isVisible()) {
            activateAdjacentSimulationTab(true);
        }
    });
    auto* closeTabShortcut = new QShortcut(QKeySequence::Close,
                                           this,
                                           nullptr,
                                           nullptr,
                                           Qt::WindowShortcut);
    connect(closeTabShortcut, &QShortcut::activated, this, [this]() {
        if (isVisible()) {
            closeCurrentSimulationTab();
        }
    });

    addCentralWidget(centralWidget, true, true, 0);
    createSimulationTab();
    applyTheme();
}

void SimulationPage::setSaveContext(CanvasPage* canvasPage, GlobalConfigPage* globalConfigPage) {
    m_canvasPage = canvasPage;
    m_globalConfigPage = globalConfigPage;
}

void SimulationPage::activateAdjacentSimulationTab(bool backward) {
    if (!m_simulationTabs || m_simulationTabs->count() <= 1) {
        return;
    }
    const int n = m_simulationTabs->count();
    const int c = m_simulationTabs->currentIndex();
    m_simulationTabs->setCurrentIndex(backward ? (c - 1 + n) % n : (c + 1) % n);
}

void SimulationPage::closeCurrentSimulationTab() {
    if (!m_simulationTabs) {
        return;
    }
    closeSimulationTab(m_simulationTabs->currentIndex());
}

bool SimulationPage::prepareRunTaskForTab(int tabIndex,
                                          const QMap<QString, QString>& globalConfig,
                                          int simulationTabId,
                                          RunTask* outTask) {
    if (!outTask || !m_canvasPage || tabIndex < 0) {
        return false;
    }
    m_canvasPage->activateCanvasTabByIndex(tabIndex);
    m_canvasPage->setGlobalConfig(globalConfig);

    const QString tabTitle = m_canvasPage->canvasTabTitle(tabIndex).trimmed();
    const QString displayTitle = tabTitle.isEmpty() ? tr("网络 %1").arg(tabIndex + 1) : tabTitle;
    const QString prefix = QStringLiteral("[网络%1:%2] ").arg(tabIndex + 1).arg(displayTitle);

    QString saveError;
    if (!m_canvasPage->exportTopologySilently(&saveError)) {
        appendOutput(prefix + tr("拓扑保存失败: %1\n").arg(saveError), simulationTabId);
        return false;
    }
    if (!m_canvasPage->exportConfigJsonSilently(&saveError)) {
        appendOutput(prefix + tr("配置保存失败: %1\n").arg(saveError), simulationTabId);
        return false;
    }

    const QString configFilePath = m_canvasPage->currentConfigExportPath();
    if (configFilePath.isEmpty() || !QFileInfo::exists(configFilePath)) {
        appendOutput(prefix + tr("找不到配置文件: %1\n").arg(configFilePath), simulationTabId);
        return false;
    }

    outTask->simulationTabId = simulationTabId;
    outTask->tabIndex = tabIndex;
    outTask->tabTitle = displayTitle;
    outTask->configFilePath = configFilePath;
    outTask->recordConfig = m_canvasPage->mergedBooksimConfigForSimulationRecord();
    if (outTask->recordConfig.isEmpty()) {
        outTask->recordConfig = globalConfig;
    }
    outTask->taskKey = QStringLiteral("simtab:%1|canvas:%2").arg(simulationTabId).arg(tabIndex);
    return true;
}

bool SimulationPage::collectRunTasksForAllTabs(int simulationTabId, QList<RunTask>* tasks) {
    if (!tasks || !m_canvasPage || !m_globalConfigPage) {
        appendOutput("错误: 页面上下文未初始化。\n", simulationTabId);
        return false;
    }
    tasks->clear();
    const int tabCount = m_canvasPage->canvasTabCount();
    if (tabCount <= 0) {
        appendOutput("错误: 当前没有可执行仿真的网络 Tab。\n", simulationTabId);
        return false;
    }

    const QMap<QString, QString> globalConfig = m_globalConfigPage->collectCurrentConfig();
    const int originalIndex = m_canvasPage->currentCanvasTabIndex();
    const auto restoreTab = [this, originalIndex]() {
        if (m_canvasPage && originalIndex >= 0) {
            m_canvasPage->activateCanvasTabByIndex(originalIndex);
        }
    };

    for (int i = 0; i < tabCount; ++i) {
        RunTask task;
        if (!prepareRunTaskForTab(i, globalConfig, simulationTabId, &task)) {
            restoreTab();
            return false;
        }
        tasks->push_back(task);
    }
    restoreTab();
    return !tasks->isEmpty();
}

bool SimulationPage::chooseCanvasTabForSimulation(int* outTabIndex, QString* outTabTitle) const {
    if (!outTabIndex || !m_canvasPage) {
        return false;
    }
    const int tabCount = m_canvasPage->canvasTabCount();
    if (tabCount <= 0) {
        return false;
    }
    QStringList items;
    items.reserve(tabCount);
    for (int i = 0; i < tabCount; ++i) {
        const QString title = m_canvasPage->canvasTabTitle(i).trimmed();
        const QString display = title.isEmpty() ? tr("网络 %1").arg(i + 1) : title;
        items.push_back(QStringLiteral("%1: %2").arg(i + 1).arg(display));
    }
    bool ok = false;
    const int current = qMax(0, m_canvasPage->currentCanvasTabIndex());
    const int maxIndex = items.isEmpty() ? 0 : static_cast<int>(items.size()) - 1;
    const QString selected = QInputDialog::getItem(const_cast<SimulationPage*>(this),
                                                   tr("选择网络Tab"),
                                                   tr("请选择要执行仿真的 Canvas 网络："),
                                                   items,
                                                   qMin(current, maxIndex),
                                                   false,
                                                   &ok);
    if (!ok || selected.isEmpty()) {
        return false;
    }
    const qsizetype pos = items.indexOf(selected);
    if (pos < 0) {
        return false;
    }
    *outTabIndex = static_cast<int>(pos);
    if (outTabTitle) {
        const QString title = m_canvasPage->canvasTabTitle(*outTabIndex).trimmed();
        *outTabTitle = title.isEmpty() ? tr("网络 %1").arg(*outTabIndex + 1) : title;
    }
    return true;
}

void SimulationPage::createSimulationTab() {
    if (!m_simulationTabs) {
        return;
    }
    const int tabId = m_nextSimulationTabId++;
    auto* tabPage = new QWidget(m_simulationTabs);
    tabPage->setProperty("simulationTabId", tabId);
    auto* lay = new QVBoxLayout(tabPage);
    lay->setContentsMargins(0, 0, 0, 0);

    auto* output = new QPlainTextEdit(tabPage);
    output->setReadOnly(true);
    output->setFont(QFont("Courier", 10));
    output->setPlaceholderText(tr("仿真输出将显示在这里..."));
    lay->addWidget(output);

    const int index = m_simulationTabs->addTab(tabPage, tr("仿真 %1").arg(tabId));
    m_simulationTabs->setCurrentIndex(index);
    m_simulationTabContexts.insert(tabId, SimulationTabContext{tabId, tabPage, output});
    applyTheme();
}

void SimulationPage::closeSimulationTab(int index) {
    if (!m_simulationTabs || index < 0 || index >= m_simulationTabs->count()) {
        return;
    }
    if (m_simulationTabs->count() <= 1) {
        const SimulationTabContext ctx = currentSimulationTab();
        if (ctx.outputText) {
            ctx.outputText->clear();
        }
        return;
    }

    QWidget* page = m_simulationTabs->widget(index);
    if (!page) {
        return;
    }
    const int tabId = page->property("simulationTabId").toInt();
    m_simulationTabContexts.remove(tabId);
    m_simulationTabs->removeTab(index);
    page->deleteLater();
}

SimulationPage::SimulationTabContext SimulationPage::currentSimulationTab() const {
    if (!m_simulationTabs) {
        return {};
    }
    QWidget* page = m_simulationTabs->currentWidget();
    if (!page) {
        return {};
    }
    const int tabId = page->property("simulationTabId").toInt();
    return m_simulationTabContexts.value(tabId);
}

QPlainTextEdit* SimulationPage::outputTextForSimulationTab(int simulationTabId) const {
    const auto it = m_simulationTabContexts.constFind(simulationTabId);
    if (it == m_simulationTabContexts.constEnd()) {
        return nullptr;
    }
    return it.value().outputText;
}

QString SimulationPage::booksimExecutableOrReportError() {
    const QString booksimExec = BooksimPaths::findBooksimExecutable();
    if (booksimExec.isEmpty()) {
        appendOutput("错误: 找不到 booksim 可执行文件！\n");
        appendOutput("请在「设置」中指定 BookSim 后端引擎路径，或将可执行文件放在应用目录 / "
                     "3rdpart/booksim2/src。\n");
    }
    return booksimExec;
}

void SimulationPage::onRunSimulation() {
    if (!m_canvasPage || !m_globalConfigPage) {
        appendOutput("错误: 页面上下文未初始化。\n");
        return;
    }
    const QString booksimExec = booksimExecutableOrReportError();
    if (booksimExec.isEmpty()) {
        return;
    }
    const SimulationTabContext simCtx = currentSimulationTab();
    if (!simCtx.outputText) {
        appendOutput("错误: 当前仿真Tab不可用。\n");
        return;
    }

    const int currentIndex = m_canvasPage->currentCanvasTabIndex();
    if (currentIndex < 0) {
        appendOutput("错误: 当前未选中网络 Tab。\n", simCtx.id);
        return;
    }

    const int originalIndex = currentIndex;
    const auto restoreTab = [this, originalIndex]() {
        if (m_canvasPage && originalIndex >= 0) {
            m_canvasPage->activateCanvasTabByIndex(originalIndex);
        }
    };
    const QMap<QString, QString> globalConfig = m_globalConfigPage->collectCurrentConfig();
    RunTask task;
    const bool ok = prepareRunTaskForTab(currentIndex, globalConfig, simCtx.id, &task);
    restoreTab();
    if (!ok) {
        return;
    }
    startRunTask(booksimExec, task);
}

void SimulationPage::onRunSimulationForSelectedNetwork() {
    if (!m_canvasPage || !m_globalConfigPage) {
        appendOutput("错误: 指定网络执行上下文不可用。\n");
        return;
    }
    const QString booksimExec = booksimExecutableOrReportError();
    if (booksimExec.isEmpty()) {
        return;
    }
    const SimulationTabContext simCtx = currentSimulationTab();
    if (!simCtx.outputText) {
        appendOutput("错误: 当前仿真Tab不可用。\n");
        return;
    }

    int tabIndex = -1;
    QString selectedTitle;
    if (!chooseCanvasTabForSimulation(&tabIndex, &selectedTitle)) {
        return;
    }
    const int originalIndex = m_canvasPage->currentCanvasTabIndex();
    const auto restoreTab = [this, originalIndex]() {
        if (m_canvasPage && originalIndex >= 0) {
            m_canvasPage->activateCanvasTabByIndex(originalIndex);
        }
    };

    const QMap<QString, QString> globalConfig = m_globalConfigPage->collectCurrentConfig();
    RunTask task;
    const bool ok = prepareRunTaskForTab(tabIndex, globalConfig, simCtx.id, &task);
    restoreTab();
    if (!ok) {
        return;
    }
    startRunTask(booksimExec, task);
}

void SimulationPage::onRunSimulationForAllNetworks() {
    const SimulationTabContext simCtx = currentSimulationTab();
    if (!simCtx.outputText) {
        appendOutput("错误: 当前仿真Tab不可用。\n");
        return;
    }
    const QString booksimExec = booksimExecutableOrReportError();
    if (booksimExec.isEmpty()) {
        return;
    }
    QList<RunTask> tasks;
    if (!collectRunTasksForAllTabs(simCtx.id, &tasks)) {
        return;
    }
    appendOutput(tr("并行仿真任务数: %1\n").arg(tasks.size()), simCtx.id);
    appendOutput("----------------------------------------\n", simCtx.id);
    for (const auto& task : tasks) {
        startRunTask(booksimExec, task);
    }
}

void SimulationPage::onClearSimulationRecord() {
    const SimulationTabContext simCtx = currentSimulationTab();
    if (!simCtx.outputText) {
        return;
    }
    simCtx.outputText->clear();
}

void SimulationPage::startRunTask(const QString& booksimExec, const RunTask& task) {
    if (booksimExec.isEmpty()) {
        return;
    }
    if (m_processByTaskKey.contains(task.taskKey)) {
        appendOutput(taskPrefix(task) + tr("该任务已在运行，忽略重复启动。\n"),
                     task.simulationTabId);
        return;
    }

    const QFileInfo configInfo(task.configFilePath);
    auto* process = new QProcess(this);
    process->setWorkingDirectory(configInfo.absolutePath());
    m_runningTasks.insert(process, task);
    m_capturedOutputs.insert(process, QString());
    m_processByTaskKey.insert(task.taskKey, process);
    ++m_pendingRunCount;

    connect(process, &QProcess::readyReadStandardOutput, this, [this, process]() {
        handleProcessOutput(process, false);
    });
    connect(process, &QProcess::readyReadStandardError, this, [this, process]() {
        handleProcessOutput(process, true);
    });
    connect(process,
            QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this,
            [this, process](int exitCode, QProcess::ExitStatus exitStatus) {
                handleProcessFinished(process, exitCode, exitStatus);
            });

    const QStringList arguments{configInfo.fileName()};
    const QString prefix = taskPrefix(task);
    appendOutput(prefix + tr("执行命令: %1 %2\n").arg(booksimExec, arguments.join(" ")),
                 task.simulationTabId);
    appendOutput(prefix + tr("工作目录: %1\n").arg(configInfo.absolutePath()), task.simulationTabId);
    process->start(booksimExec, arguments);
}

void SimulationPage::handleProcessOutput(QProcess* process, bool stderrStream) {
    if (!process || !m_runningTasks.contains(process)) {
        return;
    }
    QByteArray data = stderrStream ? process->readAllStandardError()
                                   : process->readAllStandardOutput();
    if (data.isEmpty()) {
        return;
    }
    const QString chunk = QString::fromUtf8(data);
    m_capturedOutputs[process] += chunk;
    const RunTask task = m_runningTasks.value(process);
    const QString prefix = taskPrefix(task);
    appendOutput((stderrStream ? prefix + QStringLiteral("[stderr] ") : prefix) + chunk,
                 task.simulationTabId);
}

void SimulationPage::handleProcessFinished(QProcess* process,
                                           int exitCode,
                                           QProcess::ExitStatus exitStatus) {
    if (!process || !m_runningTasks.contains(process)) {
        return;
    }
    const RunTask task = m_runningTasks.take(process);
    const QString output = m_capturedOutputs.take(process);
    m_processByTaskKey.remove(task.taskKey);
    const QString prefix = taskPrefix(task);
    if (exitStatus == QProcess::NormalExit) {
        appendOutput(prefix + tr("仿真结束，退出码: %1\n").arg(exitCode), task.simulationTabId);
    } else {
        appendOutput(prefix + tr("仿真异常结束（进程崩溃或启动失败）\n"), task.simulationTabId);
    }

    if (output.contains(QStringLiteral("Invalid routing function:"), Qt::CaseInsensitive)) {
        appendRoutingHint(prefix, task.simulationTabId);
    }

    emit simulationFinished(output);
    emit simulationFinishedWithContext(output, task.recordConfig);

    if (m_pendingRunCount > 0) {
        --m_pendingRunCount;
    }
    if (m_pendingRunCount <= 0) {
        m_pendingRunCount = 0;
        appendOutput("========================================\n", task.simulationTabId);
        appendOutput("全部并行仿真任务已结束。\n", task.simulationTabId);
    }
    process->deleteLater();
}

QString SimulationPage::taskPrefix(const RunTask& task) {
    const QString title = task.tabTitle.trimmed().isEmpty() ? tr("未命名网络")
                                                            : task.tabTitle.trimmed();
    return QStringLiteral("[仿真Tab%1][网络%2:%3] ")
        .arg(task.simulationTabId)
        .arg(task.tabIndex + 1)
        .arg(title);
}

void SimulationPage::appendRoutingHint(const QString& prefix, int simulationTabId) {
    appendOutput(QStringLiteral("\n") + prefix + "[参数提示]\n", simulationTabId);
    appendOutput(prefix + "检测到 routing_function 与 topology 不匹配。\n", simulationTabId);
    appendOutput(prefix + "请在 Canvas 的拓扑参数中检查 routing_function：\n", simulationTabId);
    appendOutput(prefix + "- mesh 推荐 dor / dim_order / xy_yx\n", simulationTabId);
    appendOutput(prefix + "- torus 推荐 dim_order / dor\n", simulationTabId);
    appendOutput(prefix + "- cmesh 推荐 dor_no_express / xy_yx_no_express\n", simulationTabId);
    appendOutput(prefix + "- fly 推荐 dest_tag\n", simulationTabId);
    appendOutput(prefix + "- qtree 推荐 nca\n", simulationTabId);
    appendOutput(prefix + "- tree4 推荐 nca / anca\n", simulationTabId);
    appendOutput(prefix + "- fattree 推荐 nca / anca\n", simulationTabId);
    appendOutput(prefix + "- flatfly 推荐 ran_min / xyyx / adaptive_xyyx\n", simulationTabId);
    appendOutput(prefix + "- dragonflynew 推荐 min / ugal\n", simulationTabId);
    appendOutput(prefix + "- anynet 使用 min\n", simulationTabId);
    appendOutput(prefix + "注意不要手动填写 *_topology 后缀，BookSim 会自动拼接。\n",
                 simulationTabId);
}

void SimulationPage::appendOutput(const QString& text, int simulationTabId) {
    QPlainTextEdit* output = outputTextForSimulationTab(simulationTabId);
    if (!output) {
        output = currentSimulationTab().outputText;
    }
    if (!output) {
        return;
    }
    output->appendPlainText(text);
    QTextCursor cursor = output->textCursor();
    cursor.movePosition(QTextCursor::End);
    output->setTextCursor(cursor);
}

void SimulationPage::applyTheme() {
    if (!m_pageRoot) {
        return;
    }

    const auto mode = eTheme->getThemeMode();
    const QColor pageBg = ElaThemeColor(mode, WindowBase);
    const QColor border = ElaThemeColor(mode, BasicBorder);
    const QColor textMain = ElaThemeColor(mode, BasicText);
    const QColor textHint = ElaThemeColor(mode, BasicTextNoFocus);
    const QColor editorBg = ElaThemeColor(mode, PopupBase);

    m_pageRoot->setAttribute(Qt::WA_StyledBackground, true);
    m_pageRoot->setStyleSheet(QStringLiteral("#ElaScrollPage_CentralPage { background-color: %1; }")
                                  .arg(pageBg.name(QColor::HexRgb)));
    if (QWidget* outerVp = m_pageRoot->parentWidget()) {
        outerVp->setAutoFillBackground(true);
        QPalette op = outerVp->palette();
        op.setColor(QPalette::Window, pageBg);
        outerVp->setPalette(op);
    }

    for (auto it = m_simulationTabContexts.begin(); it != m_simulationTabContexts.end(); ++it) {
        QPlainTextEdit* output = it.value().outputText;
        if (!output) {
            continue;
        }
        output->setStyleSheet(
            QStringLiteral("QPlainTextEdit { background-color: %1; color: %2; border: 1px solid "
                           "%3; border-radius: 8px; }")
                .arg(editorBg.name(QColor::HexRgb),
                     textMain.name(QColor::HexRgb),
                     border.name(QColor::HexRgb)));
        QPalette pal = output->palette();
        pal.setColor(QPalette::PlaceholderText, textHint);
        output->setPalette(pal);
    }
}
