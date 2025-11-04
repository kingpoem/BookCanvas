#include "SimulationPage.h"
#include <ElaPushButton.h>
#include <ElaScrollPageArea.h>
#include <ElaText.h>
#include <QCoreApplication>
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

    // 查找 booksim 可执行文件
    QString booksimExec = findBooksimExecutable();
    if (booksimExec.isEmpty()) {
        appendOutput("错误: 找不到 booksim 可执行文件！\n");
        appendOutput("请确保 booksim 已编译，位于 3rdpart/booksim2/src/booksim\n");
        return;
    }

    // 查找 booksim 目录（配置文件应该在同一目录）
    QString booksimDir = findBooksimDirectory();
    if (booksimDir.isEmpty()) {
        appendOutput("错误: 找不到 booksim 目录！\n");
        return;
    }

    QString configFile = QDir(booksimDir).filePath("anynet_config.json");

    // 检查配置文件是否存在
    if (!QFileInfo::exists(configFile)) {
        appendOutput("错误: 找不到配置文件 " + configFile + "\n");
        appendOutput("请先在 Canvas 页面导出 JSON 配置文件。\n");
        return;
    }

    // 设置工作目录
    m_process->setWorkingDirectory(booksimDir);

    // 准备命令和参数
    QStringList arguments;
    arguments << "anynet_config.json";

    // 显示执行的命令
    appendOutput("执行命令: " + booksimExec + " " + arguments.join(" ") + "\n");
    appendOutput("工作目录: " + booksimDir + "\n");
    appendOutput("----------------------------------------\n");

    // 禁用按钮
    m_runButton->setEnabled(false);

    // 启动进程
    m_process->start(booksimExec, arguments);
}

void SimulationPage::onProcessReadyReadStandardOutput() {
    QByteArray data = m_process->readAllStandardOutput();
    appendOutput(QString::fromUtf8(data));
}

void SimulationPage::onProcessReadyReadStandardError() {
    QByteArray data = m_process->readAllStandardError();
    appendOutput(QString::fromUtf8(data));
}

void SimulationPage::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    m_runButton->setEnabled(true);
}

void SimulationPage::appendOutput(const QString& text) {
    m_outputText->appendPlainText(text);
    // 自动滚动到底部
    QTextCursor cursor = m_outputText->textCursor();
    cursor.movePosition(QTextCursor::End);
    m_outputText->setTextCursor(cursor);
}

QString SimulationPage::findBooksimExecutable() {
    // 首先尝试从应用程序目录找到
    QString appDir = QCoreApplication::applicationDirPath();

    // 检查应用程序目录
    QString possiblePath = QDir(appDir).filePath("booksim");
    if (QFileInfo::exists(possiblePath)) {
        return QDir(appDir).absoluteFilePath("booksim");
    }

    // 尝试从相对路径找到 3rdpart/booksim2/src/booksim
    QString possibleBooksimPath = QDir(appDir).absoluteFilePath(
        "../../../../3rdpart/booksim2/src/booksim");
    if (QFileInfo::exists(possibleBooksimPath)) {
        return possibleBooksimPath;
    }

    // 在 Windows 上，尝试添加 .exe 扩展名
#ifdef Q_OS_WIN
    QString possiblePathExe = QDir(appDir).filePath("booksim.exe");
    if (QFileInfo::exists(possiblePathExe)) {
        return QDir(appDir).absoluteFilePath("booksim.exe");
    }

    QString possibleBooksimPathExe = QDir(appDir).absoluteFilePath(
        "../../../../3rdpart/booksim2/src/booksim.exe");
    if (QFileInfo::exists(possibleBooksimPathExe)) {
        return possibleBooksimPathExe;
    }
#endif

    return {};
}

QString SimulationPage::findBooksimDirectory() {
    QString booksimExec = findBooksimExecutable();
    if (booksimExec.isEmpty()) {
        return {};
    }

    // 返回可执行文件所在目录
    QFileInfo execInfo(booksimExec);
    return execInfo.absolutePath();
}
