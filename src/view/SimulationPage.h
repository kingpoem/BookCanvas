#pragma once

#include "BasePage.h"
#include <QPlainTextEdit>
#include <QProcess>

class ElaPushButton;

class SimulationPage : public BasePage {
    Q_OBJECT
public:
    explicit SimulationPage(QWidget* parent = nullptr);

private slots:
    void onRunSimulation();
    void onProcessReadyReadStandardOutput();
    void onProcessReadyReadStandardError();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    void appendOutput(const QString& text);
    static QString findBooksimExecutable();
    static QString findBooksimDirectory();

    ElaPushButton* m_runButton;
    QPlainTextEdit* m_outputText;
    QProcess* m_process;
};
