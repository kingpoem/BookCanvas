#include "SimpleOoxmlXlsx.h"
#include <QFile>
#include <QRegularExpression>

namespace SimpleOoxmlXlsx {
namespace {

[[nodiscard]] quint32 crc32Bytes(const QByteArray& data) {
    quint32 crc = 0xFFFFFFFF;
    for (unsigned char b : data) {
        crc ^= b;
        for (int i = 0; i < 8; ++i) {
            crc = (crc >> 1) ^ (0xEDB88320u & (0u - (crc & 1u)));
        }
    }
    return crc ^ 0xFFFFFFFF;
}

void appendLe16(QByteArray& out, quint16 v) {
    out.append(static_cast<char>(v & 0xFF));
    out.append(static_cast<char>((v >> 8) & 0xFF));
}

void appendLe32(QByteArray& out, quint32 v) {
    out.append(static_cast<char>(v & 0xFF));
    out.append(static_cast<char>((v >> 8) & 0xFF));
    out.append(static_cast<char>((v >> 16) & 0xFF));
    out.append(static_cast<char>((v >> 24) & 0xFF));
}

[[nodiscard]] QString columnLetters(int col0) {
    QString s;
    int n = col0 + 1;
    while (n > 0) {
        const int rem = (n - 1) % 26;
        s.prepend(QChar(u'A' + rem));
        n = (n - 1) / 26;
    }
    return s;
}

[[nodiscard]] QString cellRef(int col0, int row1) {
    return columnLetters(col0) + QString::number(row1);
}

[[nodiscard]] QString escapeXmlText(const QString& t) {
    QString o;
    o.reserve(t.size() + 8);
    for (QChar c : t) {
        switch (c.unicode()) {
        case u'&':
            o += QStringLiteral("&amp;");
            break;
        case u'<':
            o += QStringLiteral("&lt;");
            break;
        case u'>':
            o += QStringLiteral("&gt;");
            break;
        case u'"':
            o += QStringLiteral("&quot;");
            break;
        default:
            o += c;
            break;
        }
    }
    return o;
}

[[nodiscard]] QString sanitizeSheetName(const QString& name) {
    QString s = name.trimmed();
    static const QRegularExpression bad(QStringLiteral(R"([\\/*?:\[\]])"));
    s.replace(bad, QStringLiteral("_"));
    if (s.size() > 31) {
        s.truncate(31);
    }
    if (s.isEmpty()) {
        return QStringLiteral("Sheet1");
    }
    return s;
}

[[nodiscard]] QString buildWorksheetXml(const QStringList& headers,
                                        const QVector<QStringList>& rows) {
    QString xml;
    xml += QStringLiteral(
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
        "<worksheet xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\">"
        "<sheetData>\n");
    int rowNum = 1;
    {
        xml += QStringLiteral("<row r=\"%1\">").arg(rowNum);
        for (int c = 0; c < headers.size(); ++c) {
            xml += QStringLiteral("<c r=\"%1\" t=\"inlineStr\"><is><t>").arg(cellRef(c, rowNum));
            xml += escapeXmlText(headers[c]);
            xml += QStringLiteral("</t></is></c>");
        }
        xml += QStringLiteral("</row>\n");
        ++rowNum;
    }
    for (const QStringList& row : rows) {
        xml += QStringLiteral("<row r=\"%1\">").arg(rowNum);
        const int cols = headers.size();
        for (int c = 0; c < cols; ++c) {
            const QString cell = (c < row.size()) ? row[c] : QString();
            if (c == 0) {
                xml += QStringLiteral("<c r=\"%1\" t=\"inlineStr\"><is><t>").arg(cellRef(c, rowNum));
                xml += escapeXmlText(cell);
                xml += QStringLiteral("</t></is></c>");
                continue;
            }
            bool ok = false;
            const double dv = cell.trimmed().toDouble(&ok);
            if (ok) {
                xml += QStringLiteral("<c r=\"%1\"><v>%2</v></c>")
                           .arg(cellRef(c, rowNum), QString::number(dv, 'g', 15));
            } else {
                xml += QStringLiteral("<c r=\"%1\" t=\"inlineStr\"><is><t>").arg(cellRef(c, rowNum));
                xml += escapeXmlText(cell);
                xml += QStringLiteral("</t></is></c>");
            }
        }
        xml += QStringLiteral("</row>\n");
        ++rowNum;
    }
    xml += QStringLiteral("</sheetData></worksheet>");
    return xml;
}

struct ZipEntry {
    QString pathInZip;
    QByteArray data;
};

[[nodiscard]] bool writeZipStoreOnly(const QString& filePath, const QVector<ZipEntry>& entries) {
    QFile f(filePath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }

    QByteArray fileData;
    QByteArray centralDir;
    quint32 offset = 0;

    for (const ZipEntry& e : entries) {
        const QByteArray nameBytes = e.pathInZip.toUtf8();
        const quint32 crc = crc32Bytes(e.data);
        const quint32 sz = static_cast<quint32>(e.data.size());

        QByteArray local;
        appendLe32(local, 0x04034b50);
        appendLe16(local, 20); // version needed
        appendLe16(local, 0);  // flags
        appendLe16(local, 0);  // store
        appendLe16(local, 0);  // mod time
        appendLe16(local, 0);  // mod date
        appendLe32(local, crc);
        appendLe32(local, sz);
        appendLe32(local, sz);
        appendLe16(local, static_cast<quint16>(nameBytes.size()));
        appendLe16(local, 0); // extra len
        local.append(nameBytes);
        local.append(e.data);

        fileData.append(local);

        QByteArray cent;
        appendLe32(cent, 0x02014b50);
        appendLe16(cent, 0x0314); // version made by (unix + 2.0)
        appendLe16(cent, 20);
        appendLe16(cent, 0);
        appendLe16(cent, 0);
        appendLe16(cent, 0);
        appendLe16(cent, 0);
        appendLe32(cent, crc);
        appendLe32(cent, sz);
        appendLe32(cent, sz);
        appendLe16(cent, static_cast<quint16>(nameBytes.size()));
        appendLe16(cent, 0); // extra
        appendLe16(cent, 0); // comment
        appendLe16(cent, 0);
        appendLe16(cent, 0);
        appendLe32(cent, 0);
        appendLe32(cent, offset);
        cent.append(nameBytes);
        centralDir.append(cent);

        offset += static_cast<quint32>(local.size());
    }

    const quint32 centralSize = static_cast<quint32>(centralDir.size());
    const quint32 centralOffset = offset;

    QByteArray endRecord;
    appendLe32(endRecord, 0x06054b50);
    appendLe16(endRecord, 0);
    appendLe16(endRecord, 0);
    appendLe16(endRecord, static_cast<quint16>(entries.size()));
    appendLe16(endRecord, static_cast<quint16>(entries.size()));
    appendLe32(endRecord, centralSize);
    appendLe32(endRecord, centralOffset);
    appendLe16(endRecord, 0);

    fileData.append(centralDir);
    fileData.append(endRecord);

    return f.write(fileData) == fileData.size();
}

[[nodiscard]] bool writeSingleSheetTableImpl(const QString& filePath,
                                             const QString& sheetName,
                                             const QStringList& columnHeaders,
                                             const QVector<QStringList>& dataRows) {
    if (columnHeaders.isEmpty()) {
        return false;
    }
    const QString sheet = sanitizeSheetName(sheetName);

    const QString sheetXml = buildWorksheetXml(columnHeaders, dataRows);

    const QString workbookFilled
        = QStringLiteral(
              "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
              "<workbook xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\" "
              "xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\">"
              "<sheets><sheet name=\"%1\" sheetId=\"1\" r:id=\"rId1\"/></sheets>"
              "</workbook>")
              .arg(escapeXmlText(sheet));

    static const char* kWorkbookRels
        = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
          "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">"
          "<Relationship Id=\"rId1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/"
          "relationships/worksheet\" Target=\"worksheets/sheet1.xml\"/>"
          "<Relationship Id=\"rId2\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/"
          "relationships/styles\" Target=\"styles.xml\"/>"
          "</Relationships>";

    static const char* kRootRels
        = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
          "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">"
          "<Relationship Id=\"rId1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/"
          "relationships/officeDocument\" Target=\"xl/workbook.xml\"/>"
          "</Relationships>";

    static const char* kContentTypes
        = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
          "<Types xmlns=\"http://schemas.openxmlformats.org/package/2006/content-types\">"
          "<Default Extension=\"rels\" "
          "ContentType=\"application/vnd.openxmlformats-package.relationships+xml\"/>"
          "<Default Extension=\"xml\" ContentType=\"application/xml\"/>"
          "<Override PartName=\"/xl/workbook.xml\" "
          "ContentType=\"application/"
          "vnd.openxmlformats-officedocument.spreadsheetml.sheet.main+xml\"/>"
          "<Override PartName=\"/xl/worksheets/sheet1.xml\" "
          "ContentType=\"application/"
          "vnd.openxmlformats-officedocument.spreadsheetml.worksheet+xml\"/>"
          "<Override PartName=\"/xl/styles.xml\" "
          "ContentType=\"application/vnd.openxmlformats-officedocument.spreadsheetml.styles+xml\"/>"
          "</Types>";

    static const char* kStyles
        = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
          "<styleSheet xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\">"
          "<fonts count=\"1\"><font><sz val=\"11\"/><color theme=\"1\"/><name val=\"Calibri\"/>"
          "<family val=\"2\"/></font></fonts>"
          "<fills count=\"1\"><fill><patternFill patternType=\"none\"/></fill></fills>"
          "<borders "
          "count=\"1\"><border><left/><right/><top/><bottom/><diagonal/></border></borders>"
          "<cellXfs count=\"1\"><xf numFmtId=\"0\" fontId=\"0\" fillId=\"0\" borderId=\"0\" "
          "xfId=\"0\"/></cellXfs>"
          "</styleSheet>";

    QVector<ZipEntry> parts;
    parts.push_back({QStringLiteral("[Content_Types].xml"), QByteArray(kContentTypes)});
    parts.push_back({QStringLiteral("_rels/.rels"), QByteArray(kRootRels)});
    parts.push_back({QStringLiteral("xl/workbook.xml"), workbookFilled.toUtf8()});
    parts.push_back({QStringLiteral("xl/styles.xml"), QByteArray(kStyles)});
    parts.push_back({QStringLiteral("xl/worksheets/sheet1.xml"), sheetXml.toUtf8()});
    parts.push_back({QStringLiteral("xl/_rels/workbook.xml.rels"), QByteArray(kWorkbookRels)});

    return writeZipStoreOnly(filePath, parts);
}

} // namespace

bool writeSingleSheetTable(const QString& filePath,
                           const QString& sheetName,
                           const QStringList& columnHeaders,
                           const QVector<QStringList>& dataRows) {
    return writeSingleSheetTableImpl(filePath, sheetName, columnHeaders, dataRows);
}

} // namespace SimpleOoxmlXlsx
