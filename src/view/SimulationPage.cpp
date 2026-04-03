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
#include <QPalette>
#include <QTextCursor>
#include <QVBoxLayout>

SimulationPage::SimulationPage(QWidget* parent)
    : BasePage(parent)
    , m_runButton(nullptr)
    , m_clearButton(nullptr)
    , m_outputText(nullptr)
    , m_process(nullptr) {
    setWindowTitle("Simulation");

    // 创建中央部件
    auto* centralWidget = new QWidget(this);
    m_pageRoot = centralWidget;
    centralWidget->setWindowTitle("Simulation");
    auto* mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 20, 0);

    // 创建按钮区域
    auto* buttonArea = new ElaScrollPageArea(this);
    auto* buttonLayout = new QHBoxLayout(buttonArea);
    buttonLayout->setContentsMargins(0, 0, 0, 0);

    m_runButton = new ElaPushButton("执行仿真", this);
    buttonLayout->addWidget(m_runButton);
    m_clearButton = new ElaPushButton("清空记录", this);
    buttonLayout->addWidget(m_clearButton);
    buttonLayout->addStretch();

    mainLayout->addWidget(buttonArea);

    // 创建输出文本区域
    m_outputText = new QPlainTextEdit(this);
    m_outputText->setReadOnly(true);
    m_outputText->setFont(QFont("Courier", 10));
    m_outputText->setPlaceholderText("仿真输出将显示在这里...");
    mainLayout->addWidget(m_outputText);

    // 创建 QProcess
    m_process = new QProcess(this);

    // clang-format off
    connect(m_runButton, &ElaPushButton::clicked, this, &SimulationPage::onRunSimulation);
    connect(m_clearButton, &ElaPushButton::clicked, this, &SimulationPage::onClearSimulationRecord);
    connect(m_process, &QProcess::readyReadStandardOutput, this, &SimulationPage::onProcessReadyReadStandardOutput);
    connect(m_process, &QProcess::readyReadStandardError, this, &SimulationPage::onProcessReadyReadStandardError);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &SimulationPage::onProcessFinished);
    // clang-format on
    connect(eTheme, &ElaTheme::themeModeChanged, this, [this](ElaThemeType::ThemeMode) {
        applyTheme();
    });

    addCentralWidget(centralWidget, true, true, 0);
    applyTheme();
}

void SimulationPage::setSaveContext(CanvasPage* canvasPage, GlobalConfigPage* globalConfigPage) {
    m_canvasPage = canvasPage;
    m_globalConfigPage = globalConfigPage;
}

bool SimulationPage::prepareFilesForSimulation() {
    if (!m_canvasPage || !m_globalConfigPage) {
        appendOutput("错误: 页面上下文未初始化，无法保存仿真输入文件。\n");
        return false;
    }

    m_canvasPage->setGlobalConfig(m_globalConfigPage->collectCurrentConfig());

    QString saveError;
    if (!m_canvasPage->exportTopologySilently(&saveError)) {
        appendOutput("错误: 拓扑保存失败: " + saveError + "\n");
        return false;
    }
    if (!m_canvasPage->exportConfigJsonSilently(&saveError)) {
        appendOutput("错误: 配置保存失败: " + saveError + "\n");
        return false;
    }
    return true;
}

void SimulationPage::onRunSimulation() {
    // 如果进程正在运行，先终止它
    if (m_process->state() != QProcess::NotRunning) {
        m_process->kill();
        m_process->waitForFinished();
    }

    // 清空输出
    m_outputText->clear();
    m_capturedOutput.clear();
    if (!prepareFilesForSimulation()) {
        return;
    }

    const QString booksimExec = BooksimPaths::findBooksimExecutable();
    if (booksimExec.isEmpty()) {
        appendOutput("错误: 找不到 booksim 可执行文件！\n");
        appendOutput("请确保可执行文件位于 3rdpart/booksim2/src（Windows 为 "
                     "booksim.exe，其他平台为 booksim）。\n");
        return;
    }

    const QString configFile = BooksimPaths::configExportPathFromSettings();
    if (configFile.isEmpty() || !QFileInfo::exists(configFile)) {
        appendOutput("错误: 找不到配置文件: " + configFile + "\n");
        appendOutput("请在「设置」中确认 BookSim JSON 路径。\n");
        return;
    }

    const QFileInfo configInfo(configFile);
    m_process->setWorkingDirectory(configInfo.absolutePath());

    QStringList arguments;
    arguments << configInfo.fileName();

    appendOutput("执行命令: " + booksimExec + " " + arguments.join(" ") + "\n");
    appendOutput("工作目录: " + configInfo.absolutePath() + "\n");
    appendOutput("----------------------------------------\n");

    // 禁用按钮
    m_runButton->setEnabled(false);

    // 启动进程
    m_process->start(booksimExec, arguments);
}

void SimulationPage::onClearSimulationRecord() {
    m_outputText->clear();
    m_capturedOutput.clear();
}

void SimulationPage::onProcessReadyReadStandardOutput() {
    QByteArray data = m_process->readAllStandardOutput();
    const QString chunk = QString::fromUtf8(data);
    m_capturedOutput += chunk;
    appendOutput(chunk);
}

void SimulationPage::onProcessReadyReadStandardError() {
    QByteArray data = m_process->readAllStandardError();
    const QString chunk = QString::fromUtf8(data);
    m_capturedOutput += chunk;
    appendOutput(chunk);
}

void SimulationPage::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    m_runButton->setEnabled(true);
    if (m_capturedOutput.contains(QStringLiteral("Invalid routing function:"),
                                  Qt::CaseInsensitive)) {
        appendOutput("\n[参数提示]\n");
        appendOutput("检测到 routing_function 与 topology 不匹配。\n");
        appendOutput("请在 Canvas 的拓扑参数中检查 routing_function：\n");
        appendOutput("- mesh 推荐 dor / dim_order / xy_yx\n");
        appendOutput("- torus 推荐 dim_order / dor\n");
        appendOutput("- cmesh 推荐 dor_no_express / xy_yx_no_express\n");
        appendOutput("- fly 推荐 dest_tag\n");
        appendOutput("- qtree 推荐 nca\n");
        appendOutput("- tree4 推荐 nca / anca\n");
        appendOutput("- fattree 推荐 nca / anca\n");
        appendOutput("- flatfly 推荐 ran_min / xyyx / adaptive_xyyx\n");
        appendOutput("- dragonflynew 推荐 min / ugal\n");
        appendOutput("- anynet 使用 min\n");
        appendOutput("注意不要手动填写 *_topology 后缀，BookSim 会自动拼接。\n");
    }
    emit simulationFinished(m_capturedOutput);
}

void SimulationPage::appendOutput(const QString& text) {
    m_outputText->appendPlainText(text);
    // 自动滚动到底部
    QTextCursor cursor = m_outputText->textCursor();
    cursor.movePosition(QTextCursor::End);
    m_outputText->setTextCursor(cursor);
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

    if (m_outputText) {
        m_outputText->setStyleSheet(
            QStringLiteral("QPlainTextEdit { background-color: %1; color: %2; border: 1px solid "
                           "%3; border-radius: 8px; }")
                .arg(editorBg.name(QColor::HexRgb),
                     textMain.name(QColor::HexRgb),
                     border.name(QColor::HexRgb)));
        QPalette pal = m_outputText->palette();
        pal.setColor(QPalette::PlaceholderText, textHint);
        m_outputText->setPalette(pal);
    }
}
