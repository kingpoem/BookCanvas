#pragma once

#include <QMap>
#include <QString>
#include <QVector>

namespace NetworkTableExport {

struct NodeRow {
    QString id;
    QString type;
    double sceneX = 0.0;
    double sceneY = 0.0;
};

struct LinkRow {
    QString startId;
    QString endId;
    double weight = 1.0;
};

struct RouterParamRow {
    QString routerId;
    QString key;
    QString value;
};

/// Excel 可直接打开的 SpreadsheetML（.xml）
[[nodiscard]] bool writeExcel2003Xml(const QString& filePath,
                                     const QMap<QString, QString>& mergedConfig,
                                     const QVector<NodeRow>& nodes,
                                     const QVector<LinkRow>& links,
                                     const QVector<RouterParamRow>& routerParams);

/// UTF-8 带 BOM 的 CSV，Excel 可导入
[[nodiscard]] bool writeCsvUtf8Bom(const QString& filePath,
                                   const QMap<QString, QString>& mergedConfig,
                                   const QVector<NodeRow>& nodes,
                                   const QVector<LinkRow>& links,
                                   const QVector<RouterParamRow>& routerParams);

} // namespace NetworkTableExport
