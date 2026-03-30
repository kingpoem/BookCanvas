#include "SimulationPage.h"
#include "utils/BooksimPaths.h"
#include <ElaPushButton.h>
#include <ElaScrollPageArea.h>
#include <ElaText.h>
#include <QDir>
#include <QFileInfo>
#include <QFont>
#include <QHBoxLayout>
#include <QTextCursor>
#include <QVBoxLayout>

SimulationPage::SimulationPage(QWidget* parent)
    : BasePage(parent)
    , m_runButton(nullptr)
    , m_outputText(nullptr)
    , m_process(nullptr) {
    setWindowTitle("Simulation");

    // 创建中央部件
    auto* centralWidget = new QWidget(this);
    centralWidget->setWindowTitle("Simulation");
    auto* mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 20, 0);

    // 创建按钮区域
    auto* buttonArea = new ElaScrollPageArea(this);
    auto* buttonLayout = new QHBoxLayout(buttonArea);
    buttonLayout->setContentsMargins(0, 0, 0, 0);

    m_runButton = new ElaPushButton("执行仿真", this);
    buttonLayout->addWidget(m_runButton);
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
    connect(m_process, &QProcess::readyReadStandardOutput, this, &SimulationPage::onProcessReadyReadStandardOutput);
    connect(m_process, &QProcess::readyReadStandardError, this, &SimulationPage::onProcessReadyReadStandardError);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &SimulationPage::onProcessFinished);
    // clang-format on

    addCentralWidget(centralWidget, true, true, 0);
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

    const QString booksimExec = BooksimPaths::findBooksimExecutable();
    if (booksimExec.isEmpty()) {
        appendOutput("错误: 找不到 booksim 可执行文件！\n");
        appendOutput("请确保 booksim 已编译，位于 3rdpart/booksim2/src/booksim\n");
        return;
    }

    const QString configFile = BooksimPaths::configExportPathFromSettings();
    if (configFile.isEmpty() || !QFileInfo::exists(configFile)) {
        appendOutput("错误: 找不到配置文件: " + configFile + "\n");
        appendOutput("请在「设置」中确认 BookSim JSON 路径，或先在 Canvas 导出配置。\n");
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
    emit simulationFinished(m_capturedOutput);
}

void SimulationPage::appendOutput(const QString& text) {
    m_outputText->appendPlainText(text);
    // 自动滚动到底部
    QTextCursor cursor = m_outputText->textCursor();
    cursor.movePosition(QTextCursor::End);
    m_outputText->setTextCursor(cursor);
}
