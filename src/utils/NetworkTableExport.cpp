#include "NetworkTableExport.h"
#include <QFile>
#include <QRegularExpression>
#include <QStringConverter>
#include <QTextStream>

namespace NetworkTableExport {
namespace {

[[nodiscard]] QString escapeXml(const QString& s) {
    QString o;
    o.reserve(s.size() + 8);
    for (QChar c : s) {
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
    if (s.isEmpty()) {
        return QStringLiteral("Sheet");
    }
    static const QRegularExpression bad(QStringLiteral("[\\\\/*?:\\[\\]]"));
    s.replace(bad, QStringLiteral("_"));
    if (s.size() > 31) {
        s.truncate(31);
    }
    return s;
}

void writeXmlStringRow(QTextStream& out, const QStringList& cells) {
    out << "  <Row>\n";
    for (const QString& c : cells) {
        out << "   <Cell><Data ss:Type=\"String\">" << escapeXml(c) << "</Data></Cell>\n";
    }
    out << "  </Row>\n";
}

void writeXmlNumberRow(QTextStream& out, const QString& a, const QString& b, double w) {
    out << "  <Row>\n";
    out << "   <Cell><Data ss:Type=\"String\">" << escapeXml(a) << "</Data></Cell>\n";
    out << "   <Cell><Data ss:Type=\"String\">" << escapeXml(b) << "</Data></Cell>\n";
    out << "   <Cell><Data ss:Type=\"Number\">" << QString::number(w, 'g', 15)
        << "</Data></Cell>\n";
    out << "  </Row>\n";
}

[[nodiscard]] QString csvField(const QString& s) {
    const bool needQuote = s.contains(QLatin1Char(',')) || s.contains(QLatin1Char('"'))
                           || s.contains(QLatin1Char('\n')) || s.contains(QLatin1Char('\r'));
    if (!needQuote) {
        return s;
    }
    QString t = s;
    t.replace(QLatin1Char('"'), QStringLiteral("\"\""));
    return QStringLiteral("\"%1\"").arg(t);
}

} // namespace

bool writeExcel2003Xml(const QString& filePath,
                       const QMap<QString, QString>& mergedConfig,
                       const QVector<NodeRow>& nodes,
                       const QVector<LinkRow>& links,
                       const QVector<RouterParamRow>& routerParams) {
    QFile f(filePath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }
    QTextStream out(&f);
    out.setEncoding(QStringConverter::Utf8);
    out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    out << "<?mso-application progid=\"Excel.Sheet\"?>\n";
    out << "<Workbook xmlns=\"urn:schemas-microsoft-com:office:spreadsheet\" "
           "xmlns:ss=\"urn:schemas-microsoft-com:office:spreadsheet\">\n";
    out << " <Styles>\n";
    out << "  <Style ss:ID=\"hdr\"><Font ss:Bold=\"1\"/></Style>\n";
    out << " </Styles>\n";

    // —— BookSim 合并配置
    out << " <Worksheet ss:Name=\"" << sanitizeSheetName(QStringLiteral("BookSim配置")) << "\">\n";
    out << "  <Table>\n";
    out << "   <Row ss:StyleID=\"hdr\">";
    out << "<Cell><Data ss:Type=\"String\">" << escapeXml(QStringLiteral("参数"))
        << "</Data></Cell>";
    out << "<Cell><Data ss:Type=\"String\">" << escapeXml(QStringLiteral("值")) << "</Data></Cell>";
    out << "</Row>\n";
    for (auto it = mergedConfig.begin(); it != mergedConfig.end(); ++it) {
        writeXmlStringRow(out, {it.key(), it.value()});
    }
    out << "  </Table>\n";
    out << " </Worksheet>\n";

    // —— 节点
    out << " <Worksheet ss:Name=\"" << sanitizeSheetName(QStringLiteral("节点")) << "\">\n";
    out << "  <Table>\n";
    out << "   <Row ss:StyleID=\"hdr\">";
    out << "<Cell><Data ss:Type=\"String\">" << escapeXml(QStringLiteral("节点ID"))
        << "</Data></Cell>";
    out << "<Cell><Data ss:Type=\"String\">" << escapeXml(QStringLiteral("类型"))
        << "</Data></Cell>";
    out << "<Cell><Data ss:Type=\"String\">" << escapeXml(QStringLiteral("场景X"))
        << "</Data></Cell>";
    out << "<Cell><Data ss:Type=\"String\">" << escapeXml(QStringLiteral("场景Y"))
        << "</Data></Cell>";
    out << "</Row>\n";
    for (const NodeRow& n : nodes) {
        out << "  <Row>\n";
        out << "   <Cell><Data ss:Type=\"String\">" << escapeXml(n.id) << "</Data></Cell>\n";
        out << "   <Cell><Data ss:Type=\"String\">" << escapeXml(n.type) << "</Data></Cell>\n";
        out << "   <Cell><Data ss:Type=\"Number\">" << QString::number(n.sceneX, 'g', 15)
            << "</Data></Cell>\n";
        out << "   <Cell><Data ss:Type=\"Number\">" << QString::number(n.sceneY, 'g', 15)
            << "</Data></Cell>\n";
        out << "  </Row>\n";
    }
    out << "  </Table>\n";
    out << " </Worksheet>\n";

    // —— 链路
    out << " <Worksheet ss:Name=\"" << sanitizeSheetName(QStringLiteral("链路")) << "\">\n";
    out << "  <Table>\n";
    out << "   <Row ss:StyleID=\"hdr\">";
    out << "<Cell><Data ss:Type=\"String\">" << escapeXml(QStringLiteral("起点"))
        << "</Data></Cell>";
    out << "<Cell><Data ss:Type=\"String\">" << escapeXml(QStringLiteral("终点"))
        << "</Data></Cell>";
    out << "<Cell><Data ss:Type=\"String\">" << escapeXml(QStringLiteral("权重"))
        << "</Data></Cell>";
    out << "</Row>\n";
    for (const LinkRow& l : links) {
        writeXmlNumberRow(out, l.startId, l.endId, l.weight);
    }
    out << "  </Table>\n";
    out << " </Worksheet>\n";

    // —— 路由器参数
    out << " <Worksheet ss:Name=\"" << sanitizeSheetName(QStringLiteral("路由器参数")) << "\">\n";
    out << "  <Table>\n";
    out << "   <Row ss:StyleID=\"hdr\">";
    out << "<Cell><Data ss:Type=\"String\">" << escapeXml(QStringLiteral("路由器"))
        << "</Data></Cell>";
    out << "<Cell><Data ss:Type=\"String\">" << escapeXml(QStringLiteral("参数"))
        << "</Data></Cell>";
    out << "<Cell><Data ss:Type=\"String\">" << escapeXml(QStringLiteral("值")) << "</Data></Cell>";
    out << "</Row>\n";
    for (const RouterParamRow& r : routerParams) {
        writeXmlStringRow(out, {r.routerId, r.key, r.value});
    }
    out << "  </Table>\n";
    out << " </Worksheet>\n";

    out << "</Workbook>\n";
    f.close();
    return true;
}

bool writeCsvUtf8Bom(const QString& filePath,
                     const QMap<QString, QString>& mergedConfig,
                     const QVector<NodeRow>& nodes,
                     const QVector<LinkRow>& links,
                     const QVector<RouterParamRow>& routerParams) {
    QFile f(filePath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }
    f.write(QByteArrayLiteral("\xEF\xBB\xBF"));
    QTextStream out(&f);
    out.setEncoding(QStringConverter::Utf8);

    auto writeSection = [&](const QString& name) { out << csvField(name) << "\n"; };

    writeSection(QStringLiteral("## BookSim配置"));
    out << csvField(QStringLiteral("参数")) << QLatin1Char(',') << csvField(QStringLiteral("值"))
        << QLatin1Char('\n');
    for (auto it = mergedConfig.begin(); it != mergedConfig.end(); ++it) {
        out << csvField(it.key()) << QLatin1Char(',') << csvField(it.value()) << QLatin1Char('\n');
    }
    out << QLatin1Char('\n');

    writeSection(QStringLiteral("## 节点"));
    out << csvField(QStringLiteral("节点ID")) << QLatin1Char(',')
        << csvField(QStringLiteral("类型")) << QLatin1Char(',') << csvField(QStringLiteral("场景X"))
        << QLatin1Char(',') << csvField(QStringLiteral("场景Y")) << QLatin1Char('\n');
    for (const NodeRow& n : nodes) {
        out << csvField(n.id) << QLatin1Char(',') << csvField(n.type) << QLatin1Char(',')
            << csvField(QString::number(n.sceneX, 'g', 15)) << QLatin1Char(',')
            << csvField(QString::number(n.sceneY, 'g', 15)) << QLatin1Char('\n');
    }
    out << QLatin1Char('\n');

    writeSection(QStringLiteral("## 链路"));
    out << csvField(QStringLiteral("起点")) << QLatin1Char(',') << csvField(QStringLiteral("终点"))
        << QLatin1Char(',') << csvField(QStringLiteral("权重")) << QLatin1Char('\n');
    for (const LinkRow& l : links) {
        out << csvField(l.startId) << QLatin1Char(',') << csvField(l.endId) << QLatin1Char(',')
            << csvField(QString::number(l.weight, 'g', 15)) << QLatin1Char('\n');
    }
    out << QLatin1Char('\n');

    writeSection(QStringLiteral("## 路由器参数"));
    out << csvField(QStringLiteral("路由器")) << QLatin1Char(',')
        << csvField(QStringLiteral("参数")) << QLatin1Char(',') << csvField(QStringLiteral("值"))
        << QLatin1Char('\n');
    for (const RouterParamRow& r : routerParams) {
        out << csvField(r.routerId) << QLatin1Char(',') << csvField(r.key) << QLatin1Char(',')
            << csvField(r.value) << QLatin1Char('\n');
    }

    f.close();
    return true;
}

} // namespace NetworkTableExport
