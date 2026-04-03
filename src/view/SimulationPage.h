#pragma once

#include "BasePage.h"
#include <QPlainTextEdit>
#include <QProcess>

class ElaPushButton;
class CanvasPage;
class GlobalConfigPage;

class SimulationPage : public BasePage {
    Q_OBJECT
public:
    explicit SimulationPage(QWidget* parent = nullptr);
    void setSaveContext(CanvasPage* canvasPage, GlobalConfigPage* globalConfigPage);

signals:
    void simulationFinished(const QString& fullConsoleOutput);

private slots:
    void onRunSimulation();
    void onProcessReadyReadStandardOutput();
    void onProcessReadyReadStandardError();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    bool prepareFilesForSimulation();
    void appendOutput(const QString& text);

    ElaPushButton* m_runButton;
    QPlainTextEdit* m_outputText;
    QProcess* m_process;
    QString m_capturedOutput;
    CanvasPage* m_canvasPage = nullptr;
    GlobalConfigPage* m_globalConfigPage = nullptr;
};
