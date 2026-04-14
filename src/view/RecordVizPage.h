#pragma once

#include "BasePage.h"
#include "utils/SimulationRecordStore.h"
#include <QList>

class ElaPushButton;
class ElaText;
class QFrame;
class QScrollArea;
class QVBoxLayout;

class RecordVizPage : public BasePage {
    Q_OBJECT
public:
    explicit RecordVizPage(QWidget* parent = nullptr);

public slots:
    void ingestRecordLineChart(const QList<SimulationRecordSnapshot>& records,
                               const QString& metricAKey,
                               const QString& metricALabel,
                               const QString& metricBKey,
                               const QString& metricBLabel);
    void ingestRecordScatter3D(const QList<SimulationRecordSnapshot>& records,
                               const QString& metricAKey,
                               const QString& metricALabel,
                               const QString& metricBKey,
                               const QString& metricBLabel,
                               const QString& metricCKey,
                               const QString& metricCLabel);

private:
    enum class VizKind { None, Line, Scatter3D };

    void clearBody();
    void rebuildEmptyHint();
    void rebuildLineChart();
    void rebuildScatter3D();
    void setStatus(const QString& message, bool isError);
    void applyPageChrome();
    void updateExportButtonsEnabled();
    void exportChartPng();
    void exportChartJpg();
    void exportChartDataXlsx();
    [[nodiscard]] QString defaultDownloadsPath(const QString& fileName) const;

    ElaText* m_statusText{};
    ElaPushButton* m_exportPngBtn{};
    ElaPushButton* m_exportJpgBtn{};
    ElaPushButton* m_exportXlsxBtn{};
    QScrollArea* m_scroll{};
    QWidget* m_scrollInner{};
    QVBoxLayout* m_bodyLayout{};
    QWidget* m_pageRoot{};
    QFrame* m_chartExportHost{};

    QList<SimulationRecordSnapshot> m_chartRecords;
    QString m_chartMetricAKey;
    QString m_chartMetricALabel;
    QString m_chartMetricBKey;
    QString m_chartMetricBLabel;
    QString m_chartMetricCKey;
    QString m_chartMetricCLabel;
    VizKind m_vizKind = VizKind::None;
    bool m_statusIsError = false;
};
