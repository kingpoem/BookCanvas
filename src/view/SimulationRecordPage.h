#pragma once

#include "BasePage.h"
#include "utils/SimulationRecordStore.h"
#include <QList>
#include <QMap>

class ElaLineEdit;
class ElaText;
class QScrollArea;
class QVBoxLayout;

class SimulationRecordPage : public BasePage {
    Q_OBJECT
public:
    explicit SimulationRecordPage(QWidget* parent = nullptr);

signals:
    void showRecordInResultRequested(const QString& simulationLog);

public slots:
    void appendRecord(const QString& simulationLog, const QMap<QString, QString>& config);

private:
    void reloadRecords();
    void rebuildCards();
    void applyTheme();
    [[nodiscard]] bool matchesKeyword(const SimulationRecordSnapshot& record) const;
    [[nodiscard]] QWidget* buildRecordCard(const SimulationRecordSnapshot& record, int index);
    [[nodiscard]] QString buildConfigSummary(const QMap<QString, QString>& cfg) const;
    [[nodiscard]] QString buildMetricSummary(const SimulationRecordSnapshot& record) const;
    void persistRecords();

    QList<SimulationRecordSnapshot> m_records;
    ElaLineEdit* m_searchEdit = nullptr;
    ElaText* m_statusText = nullptr;
    QScrollArea* m_scrollArea = nullptr;
    QWidget* m_scrollInner = nullptr;
    QVBoxLayout* m_cardsLayout = nullptr;
    QWidget* m_pageRoot = nullptr;
};
