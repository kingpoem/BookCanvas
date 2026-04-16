#pragma once

#include "BasePage.h"
#include <QHash>
#include <QList>
#include <QMap>
#include <QProcess>

class ElaPushButton;
class QPlainTextEdit;
class QTabWidget;
class CanvasPage;
class GlobalConfigPage;

class SimulationPage : public BasePage {
    Q_OBJECT
public:
    explicit SimulationPage(QWidget* parent = nullptr);
    void setSaveContext(CanvasPage* canvasPage, GlobalConfigPage* globalConfigPage);
    void activateAdjacentSimulationTab(bool backward);
    void closeCurrentSimulationTab();

signals:
    void simulationFinished(const QString& fullConsoleOutput);
    void simulationFinishedWithContext(const QString& fullConsoleOutput,
                                       const QMap<QString, QString>& config);

private slots:
    void onRunSimulation();
    void onRunSimulationForSelectedNetwork();
    void onRunSimulationForAllNetworks();
    void onClearSimulationRecord();

private:
    struct RunTask {
        int simulationTabId = -1;
        int tabIndex = -1;
        QString tabTitle;
        QString configFilePath;
        QMap<QString, QString> recordConfig;
        QString taskKey;
    };

    struct SimulationTabContext {
        int id = -1;
        QWidget* page = nullptr;
        QPlainTextEdit* outputText = nullptr;
    };

    bool prepareRunTaskForTab(int tabIndex,
                              const QMap<QString, QString>& globalConfig,
                              int simulationTabId,
                              RunTask* outTask);
    bool collectRunTasksForAllTabs(int simulationTabId, QList<RunTask>* tasks);
    bool chooseCanvasTabForSimulation(int* outTabIndex, QString* outTabTitle) const;
    void createSimulationTab();
    void closeSimulationTab(int index);
    SimulationTabContext currentSimulationTab() const;
    QPlainTextEdit* outputTextForSimulationTab(int simulationTabId) const;
    void startRunTask(const QString& booksimExec, const RunTask& task);
    void handleProcessOutput(QProcess* process, bool stderrStream);
    void handleProcessFinished(QProcess* process, int exitCode, QProcess::ExitStatus exitStatus);
    QString booksimExecutableOrReportError();
    void appendOutput(const QString& text, int simulationTabId = -1);
    static QString taskPrefix(const RunTask& task);
    void appendRoutingHint(const QString& prefix, int simulationTabId);
    void appendBooksimExportPathPermissionHint(const QString& prefix, int simulationTabId);
    void applyTheme();

    ElaPushButton* m_runButton;
    ElaPushButton* m_runSelectedButton;
    ElaPushButton* m_runAllButton;
    ElaPushButton* m_clearButton;
    QTabWidget* m_simulationTabs = nullptr;
    int m_nextSimulationTabId = 1;
    QHash<int, SimulationTabContext> m_simulationTabContexts;
    QHash<QProcess*, RunTask> m_runningTasks;
    QHash<QProcess*, QString> m_capturedOutputs;
    QHash<QString, QProcess*> m_processByTaskKey;
    int m_pendingRunCount = 0;
    CanvasPage* m_canvasPage = nullptr;
    GlobalConfigPage* m_globalConfigPage = nullptr;
    QWidget* m_pageRoot = nullptr;
};
