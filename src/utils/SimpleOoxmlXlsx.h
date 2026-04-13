#pragma once

#include <QString>
#include <QStringList>
#include <QVector>

namespace SimpleOoxmlXlsx {

/// 写入单工作表的 Office Open XML（.xlsx），UTF-8，首行为表头。
[[nodiscard]] bool writeSingleSheetTable(const QString& filePath,
                                         const QString& sheetName,
                                         const QStringList& columnHeaders,
                                         const QVector<QStringList>& dataRows);

} // namespace SimpleOoxmlXlsx
