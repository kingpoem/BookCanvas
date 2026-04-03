#pragma once

#include "BasePage.h"
#include "utils/SimulationRecordStore.h"
#include <QList>
#include <QMap>

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

public slots:
    void appendRecord(const QString& simulationLog, const QMap<QString, QString>& config);

private:
    void reloadRecords();
    void rebuildCards();
    void applyTheme();
    void refreshTopologyFilterOptions();
    [[nodiscard]] bool matchesKeyword(const SimulationRecordSnapshot& record) const;
    [[nodiscard]] bool matchesFilters(const SimulationRecordSnapshot& record) const;
    [[nodiscard]] int findRecordIndexById(const QString& id) const;
    [[nodiscard]] QWidget* buildRecordCard(const SimulationRecordSnapshot& record);
    void persistRecords();

    QList<SimulationRecordSnapshot> m_records;
    ElaLineEdit* m_searchEdit = nullptr;
    ElaComboBox* m_topologyFilter = nullptr;
    ElaComboBox* m_latencyFilter = nullptr;
    ElaComboBox* m_sortCombo = nullptr;
    ElaText* m_statusText = nullptr;
    QScrollArea* m_scrollArea = nullptr;
    QWidget* m_scrollInner = nullptr;
    QVBoxLayout* m_cardsLayout = nullptr;
    QWidget* m_pageRoot = nullptr;
};
