#pragma once

#include <QList>
#include <QMap>
#include <QString>

struct SimulationRecordSnapshot {
    QString id;
    QString name;
    QString createdAtIso;
    QString updatedAtIso;
    QString simulationLog;
    QMap<QString, QString> config;
    int trafficClassCount = 0;
    double packetLatencyAvg = -1.0;
    double networkLatencyAvg = -1.0;
    double flitLatencyAvg = -1.0;
    double throughputMatchPercent = -1.0;
    double totalRunTimeSec = -1.0;
};

namespace SimulationRecordStore {

[[nodiscard]] QString storagePath();
[[nodiscard]] QList<SimulationRecordSnapshot> loadRecords();
bool saveRecords(const QList<SimulationRecordSnapshot>& records, QString* errorMessage = nullptr);
[[nodiscard]] SimulationRecordSnapshot makeRecord(const QString& simulationLog,
                                                  const QMap<QString, QString>& config);

} // namespace SimulationRecordStore
