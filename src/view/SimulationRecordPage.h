#pragma once

#include "BasePage.h"
#include "utils/SimulationRecordStore.h"
#include <QList>
#include <QMap>
#include <QVector>

class ElaLineEdit;
class ElaText;
class ElaComboBox;
class QScrollArea;
class QVBoxLayout;

class SimulationRecordPage : public BasePage {
    Q_OBJECT
public:
    explicit SimulationRecordPage(QWidget* parent = nullptr);

signals:
    void showRecordInResultRequested(const QString& simulationLog);
    void showLineChartInResultRequested(const QList<SimulationRecordSnapshot>& records,
                                        const QString& metricAKey,
                                        const QString& metricALabel,
                                        const QString& metricBKey,
                                        const QString& metricBLabel);

public slots:
    void appendRecord(const QString& simulationLog, const QMap<QString, QString>& config);

private:
    struct MetricOption {
        QString label;
        QString key;
    };

    void reloadRecords();
    void rebuildCards();
    void applyTheme();
    void refreshNumericLabelOptions();
    void updateNumericFilterEditors();
    [[nodiscard]] static double numericLabelValue(const SimulationRecordSnapshot& record,
                                                  const QString& labelKey);
    [[nodiscard]] bool hasNumericFilter() const;
    [[nodiscard]] bool matchesKeyword(const SimulationRecordSnapshot& record) const;
    [[nodiscard]] bool matchesFilters(const SimulationRecordSnapshot& record) const;
    [[nodiscard]] QVector<int> buildVisibleRecordIndices(bool sorted) const;
    [[nodiscard]] QVector<MetricOption> buildMetricOptions() const;
    [[nodiscard]] int findRecordIndexById(const QString& id) const;
    [[nodiscard]] QWidget* buildRecordCard(const SimulationRecordSnapshot& record);
    void openLineChartDialog();
    void persistRecords();

    QList<SimulationRecordSnapshot> m_records;
    ElaLineEdit* m_searchEdit = nullptr;
    ElaComboBox* m_numericFilterMode = nullptr;
    ElaComboBox* m_sortCombo = nullptr;
    ElaComboBox* m_numericLabelCombo = nullptr;
    ElaLineEdit* m_rangeMinEdit = nullptr;
    ElaLineEdit* m_rangeMaxEdit = nullptr;
    ElaText* m_statusText = nullptr;
    QScrollArea* m_scrollArea = nullptr;
    QWidget* m_scrollInner = nullptr;
    QVBoxLayout* m_cardsLayout = nullptr;
    QWidget* m_pageRoot = nullptr;
};
