#pragma once

#include "BasePage.h"
#include <QMap>
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
    void simulationFinishedWithContext(const QString& fullConsoleOutput,
                                       const QMap<QString, QString>& config);

private slots:
    void onRunSimulation();
    void onClearSimulationRecord();
    void onProcessReadyReadStandardOutput();
    void onProcessReadyReadStandardError();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    bool prepareFilesForSimulation();
    void appendOutput(const QString& text);
    void applyTheme();

    ElaPushButton* m_runButton;
    ElaPushButton* m_clearButton;
    QPlainTextEdit* m_outputText;
    QProcess* m_process;
    QString m_capturedOutput;
    QMap<QString, QString> m_lastRunConfig;
    CanvasPage* m_canvasPage = nullptr;
    GlobalConfigPage* m_globalConfigPage = nullptr;
    QWidget* m_pageRoot = nullptr;
};
