#include "SimulationRecordStore.h"
#include "BookSimStatsParser.h"
#include "Settings.hpp"
#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QUuid>

namespace {

QJsonObject toJson(const SimulationRecordSnapshot& record) {
    QJsonObject obj;
    obj.insert(QStringLiteral("id"), record.id);
    obj.insert(QStringLiteral("name"), record.name);
    obj.insert(QStringLiteral("createdAtIso"), record.createdAtIso);
    obj.insert(QStringLiteral("updatedAtIso"), record.updatedAtIso);
    obj.insert(QStringLiteral("simulationLog"), record.simulationLog);
    obj.insert(QStringLiteral("trafficClassCount"), record.trafficClassCount);
    obj.insert(QStringLiteral("packetLatencyAvg"), record.packetLatencyAvg);
    obj.insert(QStringLiteral("networkLatencyAvg"), record.networkLatencyAvg);
    obj.insert(QStringLiteral("flitLatencyAvg"), record.flitLatencyAvg);
    obj.insert(QStringLiteral("throughputMatchPercent"), record.throughputMatchPercent);
    obj.insert(QStringLiteral("totalRunTimeSec"), record.totalRunTimeSec);

    QJsonObject cfg;
    for (auto it = record.config.begin(); it != record.config.end(); ++it) {
        cfg.insert(it.key(), it.value());
    }
    obj.insert(QStringLiteral("config"), cfg);
    return obj;
}

SimulationRecordSnapshot fromJson(const QJsonObject& obj) {
    SimulationRecordSnapshot record;
    record.id = obj.value(QStringLiteral("id")).toString();
    record.name = obj.value(QStringLiteral("name")).toString();
    record.createdAtIso = obj.value(QStringLiteral("createdAtIso")).toString();
    record.updatedAtIso = obj.value(QStringLiteral("updatedAtIso")).toString();
    record.simulationLog = obj.value(QStringLiteral("simulationLog")).toString();
    record.trafficClassCount = obj.value(QStringLiteral("trafficClassCount")).toInt();
    record.packetLatencyAvg = obj.value(QStringLiteral("packetLatencyAvg")).toDouble(-1.0);
    record.networkLatencyAvg = obj.value(QStringLiteral("networkLatencyAvg")).toDouble(-1.0);
    record.flitLatencyAvg = obj.value(QStringLiteral("flitLatencyAvg")).toDouble(-1.0);
    record.throughputMatchPercent = obj.value(QStringLiteral("throughputMatchPercent"))
                                        .toDouble(-1.0);
    record.totalRunTimeSec = obj.value(QStringLiteral("totalRunTimeSec")).toDouble(-1.0);

    const QJsonObject cfg = obj.value(QStringLiteral("config")).toObject();
    for (auto it = cfg.begin(); it != cfg.end(); ++it) {
        record.config.insert(it.key(), it.value().toString());
    }
    return record;
}

double extractThroughputMatchPercent(const BookSimTrafficClassStats& stats) {
    if (!stats.injectedPacketRate.hasAverage || !stats.acceptedPacketRate.hasAverage) {
        return -1.0;
    }
    if (stats.injectedPacketRate.average <= 1e-12) {
        return -1.0;
    }
    return (stats.acceptedPacketRate.average / stats.injectedPacketRate.average) * 100.0;
}

} // namespace

QString SimulationRecordStore::storagePath() {
    const QFileInfo settingsFileInfo(settings.fileName());
    return settingsFileInfo.absolutePath() + QStringLiteral("/simulation_records.json");
}

QList<SimulationRecordSnapshot> SimulationRecordStore::loadRecords() {
    QList<SimulationRecordSnapshot> records;
    const QString path = storagePath();
    QFile file(path);
    if (!file.exists()) {
        return records;
    }
    if (!file.open(QIODevice::ReadOnly)) {
        return records;
    }
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    if (!doc.isArray()) {
        return records;
    }

    const QJsonArray arr = doc.array();
    records.reserve(arr.size());
    for (const QJsonValue& value : arr) {
        if (!value.isObject()) {
            continue;
        }
        records.push_back(fromJson(value.toObject()));
    }
    return records;
}

bool SimulationRecordStore::saveRecords(const QList<SimulationRecordSnapshot>& records,
                                        QString* errorMessage) {
    QJsonArray arr;
    for (const auto& record : records) {
        arr.push_back(toJson(record));
    }
    const QJsonDocument doc(arr);

    QSaveFile file(storagePath());
    if (!file.open(QIODevice::WriteOnly)) {
        if (errorMessage) {
            *errorMessage = QObject::tr("无法写入仿真记录文件。");
        }
        return false;
    }
    file.write(doc.toJson(QJsonDocument::Indented));
    if (!file.commit()) {
        if (errorMessage) {
            *errorMessage = QObject::tr("保存仿真记录失败。");
        }
        return false;
    }
    return true;
}

SimulationRecordSnapshot SimulationRecordStore::makeRecord(const QString& simulationLog,
                                                           const QMap<QString, QString>& config) {
    SimulationRecordSnapshot record;
    const QDateTime now = QDateTime::currentDateTimeUtc();
    const QString nowIso = now.toString(Qt::ISODate);
    record.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    record.name = QObject::tr("仿真 %1").arg(
        now.toLocalTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss")));
    record.createdAtIso = nowIso;
    record.updatedAtIso = nowIso;
    record.simulationLog = simulationLog;
    record.config = config;

    const BookSimParseResult parsed = BookSimStatsParser::parseOverallFromLog(simulationLog);
    if (!parsed.ok()) {
        return record;
    }

    record.trafficClassCount = static_cast<int>(parsed.classes.size());
    if (parsed.totalRunTimeSec.has_value()) {
        record.totalRunTimeSec = parsed.totalRunTimeSec.value();
    }
    if (parsed.classes.empty()) {
        return record;
    }
    const auto& first = parsed.classes.front();
    record.packetLatencyAvg = first.packetLatency.hasAverage ? first.packetLatency.average : -1.0;
    record.networkLatencyAvg = first.networkLatency.hasAverage ? first.networkLatency.average
                                                               : -1.0;
    record.flitLatencyAvg = first.flitLatency.hasAverage ? first.flitLatency.average : -1.0;
    record.throughputMatchPercent = extractThroughputMatchPercent(first);
    return record;
}
