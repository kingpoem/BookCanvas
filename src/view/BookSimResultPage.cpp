#include "BookSimResultPage.h"
#include "utils/BookSimMetricLabels.h"
#include <ElaDef.h>
#include <ElaPushButton.h>
#include <ElaScrollPageArea.h>
#include <ElaText.h>
#include <ElaTheme.h>
#include <QApplication>
#include <QClipboard>
#include <QFont>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPaintEvent>
#include <QPainter>
#include <QPalette>
#include <QPlainTextEdit>
#include <QScrollArea>
#include <QSizePolicy>
#include <QTabWidget>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QWidget>
#include <algorithm>
#include <cmath>

namespace BookSimResultUi {

/// 全页字号底线：避免 10px/11px 等难以阅读的辅助文案
namespace TypePx {
constexpr int kBody = 13;      // 最小正文（注释、表格说明、KPI 脚注等）
constexpr int kLead = 14;      // 副标题、状态行、数值行
constexpr int kSection = 16;   // 分区标题（如流量类别）
constexpr int kCardTitle = 18; // 卡片标题
constexpr int kCardTitleEmph = 20;
constexpr int kStatus = 14;
constexpr int kMetricValue = 15;
} // namespace TypePx

/// 线性混合：返回 a·t + b·(1−t)。要做「在 b 上混入少许 a」时，应把「混入的浅色」放在 a、底色素放在 b，t 取小比例。
[[nodiscard]] QColor mix(const QColor& a, const QColor& b, qreal t) {
    const qreal u = 1.0 - t;
    return QColor(qBound(0, qRound(a.red() * t + b.red() * u), 255),
                  qBound(0, qRound(a.green() * t + b.green() * u), 255),
                  qBound(0, qRound(a.blue() * t + b.blue() * u), 255),
                  qBound(0, qRound(a.alpha() * t + b.alpha() * u), 255));
}

[[nodiscard]] QString rgba(const QColor& c) {
    return QStringLiteral("rgba(%1,%2,%3,%4)")
        .arg(c.red())
        .arg(c.green())
        .arg(c.blue())
        .arg(c.alphaF(), 0, 'f', 3);
}

struct Theme {
    QColor pageBg;
    QColor cardBg;
    QColor cardBgPrimary;
    QColor border;
    QColor tileBg;
    QColor heroG0;
    QColor heroG1;
    QColor heroBorder;
    QColor chartPacket;
    QColor chartNetwork;
    QColor chartFlit;
    QColor thrInjPkt;
    QColor thrAccPkt;
    QColor thrInjFlit;
    QColor thrAccFlit;
    QColor kpiPacket;
    QColor kpiNetwork;
    QColor kpiHops;
    QColor accentShape;
    QColor accentHops;
    QColor statusBalanced;
    QColor statusWarn;
    QColor statusBad;
    QColor textMuted;
    QColor textHint;
    /// 主正文色：浅色主题为黑色，暗色为亮色
    QColor fgMain;
    /// 次文案：暗色略弱；浅色与 fgMain 同为黑
    QColor fgDim;
};

[[nodiscard]] Theme themeFrom(ElaThemeType::ThemeMode mode) {
    Theme th;
    const QColor border = ElaThemeColor(mode, BasicBorder);
    th.border = border;

    if (mode == ElaThemeType::Light) {
        const QColor paper(0xFA, 0xFB, 0xFC);
        const QColor ink(0x1A, 0x1B, 0x1E);
        th.pageBg = paper;
        th.cardBg = QColor(0xFF, 0xFF, 0xFF);
        th.cardBgPrimary = QColor(0xFF, 0xFF, 0xFF);
        th.tileBg = QColor(0xF7, 0xF8, 0xFA);
        th.heroG0 = QColor(0xF7, 0xF9, 0xFC);
        th.heroG1 = QColor(0xFB, 0xFC, 0xFE);
        th.heroBorder = border;
        th.fgMain = ink;
        th.fgDim = QColor(0x4A, 0x4F, 0x57);
        th.textMuted = QColor(0x5B, 0x61, 0x6B);
        th.textHint = QColor(0x6A, 0x72, 0x7D);
        th.chartPacket = QColor(0x78, 0x99, 0xD6);
        th.chartNetwork = QColor(0x93, 0x9B, 0xD8);
        th.chartFlit = QColor(0x7F, 0xB2, 0xCF);
        th.thrInjPkt = QColor(0x99, 0xA6, 0xE0);
        th.thrAccPkt = QColor(0x86, 0xBC, 0xA4);
        th.thrInjFlit = QColor(0xB0, 0xA1, 0xD8);
        th.thrAccFlit = QColor(0x81, 0xC1, 0xC7);
        th.kpiPacket = th.chartPacket;
        th.kpiNetwork = th.chartNetwork;
        th.kpiHops = QColor(0x9A, 0x95, 0x8D);
        th.accentShape = QColor(0x9A, 0x95, 0x8D);
        th.accentHops = QColor(0xCA, 0xA6, 0x6A);
        th.statusBalanced = QColor(0x78, 0xB1, 0x92);
        th.statusWarn = QColor(0xCB, 0xAD, 0x78);
        th.statusBad = QColor(0xCC, 0x85, 0x85);
    } else {
        const QColor canvas(0x18, 0x1B, 0x20);
        const QColor surface(0x22, 0x26, 0x2D);
        th.pageBg = canvas;
        th.cardBg = QColor(0x1E, 0x22, 0x29);
        th.cardBgPrimary = QColor(0x21, 0x26, 0x2E);
        // KPI 小卡：略浅于底色的墨灰面（勿反向 mix，否则会变成大比例纯白）
        th.tileBg = mix(surface, canvas, 0.60);
        th.heroG0 = QColor(0x22, 0x27, 0x30);
        th.heroG1 = QColor(0x1C, 0x21, 0x29);
        th.heroBorder = border;
        th.fgMain = QColor(0xEB, 0xEE, 0xF3);
        const QColor lightGray(0xD9, 0xDE, 0xE6);
        th.textMuted = mix(ElaThemeColor(mode, BasicDetailsText), lightGray, 0.30);
        th.textHint = mix(ElaThemeColor(mode, BasicTextNoFocus), lightGray, 0.25);
        th.fgDim = th.textMuted;
        th.chartPacket = QColor(0x8E, 0xB1, 0xE8);
        th.chartNetwork = QColor(0xAA, 0xAF, 0xE4);
        th.chartFlit = QColor(0x92, 0xC6, 0xD8);
        th.thrInjPkt = QColor(0xB2, 0xBD, 0xE8);
        th.thrAccPkt = QColor(0x92, 0xC7, 0xAF);
        th.thrInjFlit = QColor(0xC1, 0xB6, 0xDE);
        th.thrAccFlit = QColor(0x96, 0xCD, 0xCF);
        th.kpiPacket = th.chartPacket;
        th.kpiNetwork = th.chartNetwork;
        th.kpiHops = QColor(0xB6, 0xB1, 0xAB);
        th.accentShape = QColor(0xB6, 0xB1, 0xAB);
        th.accentHops = QColor(0xD7, 0xB4, 0x82);
        th.statusBalanced = QColor(0x89, 0xC8, 0xA9);
        th.statusWarn = QColor(0xD7, 0xBC, 0x8B);
        th.statusBad = QColor(0xD8, 0x9B, 0x9B);
    }
    return th;
}

} // namespace BookSimResultUi

namespace {

void applySelectableLabel(QLabel* label) {
    if (!label) {
        return;
    }
    label->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
    label->setFocusPolicy(Qt::ClickFocus);
}

void applySelectableLabel(ElaText* text) {
    applySelectableLabel(static_cast<QLabel*>(text));
}

[[nodiscard]] BookSimMetricRow metricRowFromBand(const QString& name,
                                                 const BookSimStatBand& b,
                                                 int precision) {
    BookSimMetricRow row;
    row.name = name;
    if (!b.hasAverage) {
        row.avgOrValue = QStringLiteral("—");
        row.range.clear();
        return row;
    }
    row.avgOrValue = QString::number(b.average, 'f', precision);
    if (b.hasMinimum && b.hasMaximum) {
        row.range = QStringLiteral("[%1, %2]")
                        .arg(b.minimum, 0, 'f', precision)
                        .arg(b.maximum, 0, 'f', precision);
    } else {
        row.range.clear();
    }
    return row;
}

struct RecordMetricPoint {
    QString label;
    QString fullName;
    double metricA = 0.0;
    double metricB = 0.0;
};

[[nodiscard]] bool tryParseDoubleLoose(const QString& text, double* outValue) {
    bool ok = false;
    const double value = text.trimmed().toDouble(&ok);
    if (!ok) {
        return false;
    }
    if (outValue) {
        *outValue = value;
    }
    return true;
}

[[nodiscard]] bool extractRecordMetricValue(const SimulationRecordSnapshot& record,
                                            const QString& metricKey,
                                            double* outValue) {
    if (metricKey == QStringLiteral("packetLatencyAvg")) {
        if (record.packetLatencyAvg < 0.0) {
            return false;
        }
        *outValue = record.packetLatencyAvg;
        return true;
    }
    if (metricKey == QStringLiteral("networkLatencyAvg")) {
        if (record.networkLatencyAvg < 0.0) {
            return false;
        }
        *outValue = record.networkLatencyAvg;
        return true;
    }
    if (metricKey == QStringLiteral("flitLatencyAvg")) {
        if (record.flitLatencyAvg < 0.0) {
            return false;
        }
        *outValue = record.flitLatencyAvg;
        return true;
    }
    if (metricKey == QStringLiteral("throughputMatchPercent")) {
        if (record.throughputMatchPercent < 0.0) {
            return false;
        }
        *outValue = record.throughputMatchPercent;
        return true;
    }
    if (metricKey == QStringLiteral("totalRunTimeSec")) {
        if (record.totalRunTimeSec < 0.0) {
            return false;
        }
        *outValue = record.totalRunTimeSec;
        return true;
    }
    if (metricKey == QStringLiteral("trafficClassCount")) {
        *outValue = static_cast<double>(record.trafficClassCount);
        return true;
    }
    if (metricKey.startsWith(QStringLiteral("config:"))) {
        const QString configKey = metricKey.mid(QStringLiteral("config:").size());
        double value = 0.0;
        if (!tryParseDoubleLoose(record.config.value(configKey), &value)) {
            return false;
        }
        *outValue = value;
        return true;
    }
    return false;
}

[[nodiscard]] QVector<RecordMetricPoint> buildRecordMetricPoints(
    const QList<SimulationRecordSnapshot>& records,
    const QString& metricAKey,
    const QString& metricBKey) {
    QVector<RecordMetricPoint> points;
    points.reserve(records.size());
    int ordinal = 1;
    for (const auto& record : records) {
        double a = 0.0;
        double b = 0.0;
        if (!extractRecordMetricValue(record, metricAKey, &a)
            || !extractRecordMetricValue(record, metricBKey, &b)) {
            continue;
        }
        RecordMetricPoint point;
        point.fullName = record.name;
        point.label = QStringLiteral("#%1").arg(ordinal);
        point.metricA = a;
        point.metricB = b;
        points.push_back(point);
        ++ordinal;
    }
    std::sort(points.begin(),
              points.end(),
              [](const RecordMetricPoint& lhs, const RecordMetricPoint& rhs) {
                  return lhs.metricA < rhs.metricA;
              });
    return points;
}

[[nodiscard]] QString buildRecordChartPlainDump(const QVector<RecordMetricPoint>& points,
                                                const QString& metricALabel,
                                                const QString& metricBLabel) {
    if (points.size() < 2) {
        return {};
    }
    double minA = points.front().metricA;
    double maxA = points.front().metricA;
    double minB = points.front().metricB;
    double maxB = points.front().metricB;
    for (const auto& p : points) {
        minA = std::min(minA, p.metricA);
        maxA = std::max(maxA, p.metricA);
        minB = std::min(minB, p.metricB);
        maxB = std::max(maxB, p.metricB);
    }
    if (std::abs(maxA - minA) < 1e-12) {
        maxA += 1.0;
        minA -= 1.0;
    }
    if (std::abs(maxB - minB) < 1e-12) {
        maxB += 1.0;
        minB -= 1.0;
    }
    QString s;
    s += QStringLiteral("X轴：%1  [%2, %3]\n")
             .arg(metricALabel, QString::number(minA, 'f', 3), QString::number(maxA, 'f', 3));
    s += QStringLiteral("Y轴：%1  [%2, %3]\n")
             .arg(metricBLabel, QString::number(minB, 'f', 3), QString::number(maxB, 'f', 3));
    s += QStringLiteral("X轴刻度（底边，与图一致）：");
    for (int i = 0; i <= 6; ++i) {
        const double t = static_cast<double>(i) / 6.0;
        const double xTick = minA + t * (maxA - minA);
        if (i > 0) {
            s += QLatin1Char('\t');
        }
        s += QString::number(xTick, 'f', 2);
    }
    s += QStringLiteral(
        "\n纵轴网格：左侧刻度对应 X 指标量纲、右侧对应 Y 指标量纲（与图左右刻度一致）。\n");
    for (int i = 0; i <= 5; ++i) {
        const double t = static_cast<double>(i) / 5.0;
        const double valueA = minA + t * (maxA - minA);
        const double valueB = minB + t * (maxB - minB);
        s += QStringLiteral("左 %1\t右 %2\n")
                 .arg(QString::number(valueA, 'f', 2), QString::number(valueB, 'f', 2));
    }
    s += QStringLiteral("\n数据（制表符分隔，可复制到表格软件）：\n");
    s += QStringLiteral("记录\t%1\t%2\n").arg(metricALabel, metricBLabel);
    for (const auto& p : points) {
        s += QStringLiteral("%1 %2\t%3\t%4\n")
                 .arg(p.label,
                      p.fullName,
                      QString::number(p.metricA, 'f', 6),
                      QString::number(p.metricB, 'f', 6));
    }
    return s;
}

class DualMetricLineChartWidget : public QWidget {
public:
    DualMetricLineChartWidget(const QVector<RecordMetricPoint>& points,
                              const QString& metricALabel,
                              const QString& metricBLabel,
                              const BookSimResultUi::Theme& theme,
                              QWidget* parent = nullptr)
        : QWidget(parent)
        , m_points(points)
        , m_metricALabel(metricALabel)
        , m_metricBLabel(metricBLabel)
        , m_theme(theme) {
        setMinimumHeight(360);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    }

protected:
    void paintEvent(QPaintEvent* event) override {
        QWidget::paintEvent(event);
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setRenderHint(QPainter::TextAntialiasing, true);

        const QRectF panelRect = rect().adjusted(4, 4, -4, -4);
        painter.setPen(QPen(m_theme.border, 1));
        painter.setBrush(BookSimResultUi::mix(m_theme.cardBg, m_theme.pageBg, 0.72));
        painter.drawRoundedRect(panelRect, 12, 12);

        const QRectF chartRect = rect().adjusted(76, 46, -76, -72);
        if (chartRect.width() <= 40 || chartRect.height() <= 40 || m_points.size() < 2) {
            return;
        }

        double minA = m_points.front().metricA;
        double maxA = m_points.front().metricA;
        double minB = m_points.front().metricB;
        double maxB = m_points.front().metricB;
        for (const auto& p : m_points) {
            minA = std::min(minA, p.metricA);
            maxA = std::max(maxA, p.metricA);
            minB = std::min(minB, p.metricB);
            maxB = std::max(maxB, p.metricB);
        }
        if (std::abs(maxA - minA) < 1e-12) {
            maxA += 1.0;
            minA -= 1.0;
        }
        if (std::abs(maxB - minB) < 1e-12) {
            maxB += 1.0;
            minB -= 1.0;
        }

        auto xFromA = [&](double value) {
            const double ratio = (value - minA) / (maxA - minA);
            return chartRect.left() + ratio * chartRect.width();
        };
        auto yFromB = [&](double value) {
            const double ratio = (value - minB) / (maxB - minB);
            return chartRect.bottom() - ratio * chartRect.height();
        };

        QColor gridColor = m_theme.border;
        gridColor.setAlpha(100);
        QPen gridPen(gridColor, 1, Qt::DashLine);
        painter.setPen(gridPen);
        for (int i = 0; i <= 5; ++i) {
            const double y = chartRect.top() + chartRect.height() * (static_cast<double>(i) / 5.0);
            painter.drawLine(QPointF(chartRect.left(), y), QPointF(chartRect.right(), y));
        }
        const int pointCount = static_cast<int>(m_points.size());
        for (int i = 0; i <= 6; ++i) {
            const double x = chartRect.left() + chartRect.width() * (static_cast<double>(i) / 6.0);
            painter.drawLine(QPointF(x, chartRect.top()), QPointF(x, chartRect.bottom()));
        }

        QColor axisColor = m_theme.fgMain;
        axisColor.setAlpha(180);
        painter.setPen(QPen(axisColor, 1.4));
        painter.drawLine(chartRect.bottomLeft(), chartRect.topLeft());
        painter.drawLine(chartRect.bottomRight(), chartRect.topRight());
        painter.drawLine(chartRect.bottomLeft(), chartRect.bottomRight());

        QPolygonF lineA;
        for (int i = 0; i < pointCount; ++i) {
            lineA << QPointF(xFromA(m_points[i].metricA), yFromB(m_points[i].metricB));
        }

        QPolygonF areaA = lineA;
        areaA << QPointF(lineA.back().x(), chartRect.bottom())
              << QPointF(lineA.front().x(), chartRect.bottom());
        QColor areaAColor = m_theme.chartPacket;
        areaAColor.setAlpha(56);
        painter.setPen(Qt::NoPen);
        painter.setBrush(areaAColor);
        painter.drawPolygon(areaA);

        painter.setBrush(Qt::NoBrush);
        painter.setPen(QPen(m_theme.chartPacket, 2.6));
        painter.drawPolyline(lineA);

        painter.setPen(QPen(m_theme.pageBg, 1.2));
        painter.setBrush(m_theme.chartPacket);
        for (const auto& p : lineA) {
            painter.drawEllipse(p, 4.0, 4.0);
        }

        painter.setPen(QPen(axisColor, 1));
        const QFontMetrics fm(painter.font());
        for (int i = 0; i <= 6; ++i) {
            const double t = static_cast<double>(i) / 6.0;
            const double x = chartRect.left() + t * chartRect.width();
            const double valueA = minA + t * (maxA - minA);
            painter.drawLine(QPointF(x, chartRect.bottom()), QPointF(x, chartRect.bottom() + 4));
            const QString text = QString::number(valueA, 'f', 2);
            painter.drawText(QPointF(x - fm.horizontalAdvance(text) / 2.0, chartRect.bottom() + 20),
                             text);
        }

        painter.setPen(QPen(m_theme.chartPacket, 1.2));
        painter.drawText(QRectF(chartRect.left(), chartRect.top() - 28, chartRect.width(), 20),
                         Qt::AlignLeft | Qt::AlignVCenter,
                         QStringLiteral("X轴：%1  [%2, %3]")
                             .arg(m_metricALabel,
                                  QString::number(minA, 'f', 3),
                                  QString::number(maxA, 'f', 3)));
        painter.setPen(QPen(m_theme.chartNetwork, 1.2));
        painter.drawText(QRectF(chartRect.left(), chartRect.top() - 28, chartRect.width(), 20),
                         Qt::AlignRight | Qt::AlignVCenter,
                         QStringLiteral("Y轴：%1  [%2, %3]")
                             .arg(m_metricBLabel,
                                  QString::number(minB, 'f', 3),
                                  QString::number(maxB, 'f', 3)));

        painter.setPen(QPen(m_theme.fgDim, 1));
        for (int i = 0; i <= 5; ++i) {
            const double t = static_cast<double>(i) / 5.0;
            const double y = chartRect.bottom() - t * chartRect.height();
            const double valueA = minA + t * (maxA - minA);
            const double valueB = minB + t * (maxB - minB);
            const QString left = QString::number(valueA, 'f', 2);
            const QString right = QString::number(valueB, 'f', 2);
            painter.drawText(QRectF(chartRect.left() - 68, y - 9, 62, 18),
                             Qt::AlignRight | Qt::AlignVCenter,
                             left);
            painter.drawText(QRectF(chartRect.right() + 6, y - 9, 62, 18),
                             Qt::AlignLeft | Qt::AlignVCenter,
                             right);
        }

        painter.setPen(QPen(m_theme.fgMain, 1));
        const QPointF lastPoint = lineA.back();
        painter.drawText(QRectF(lastPoint.x() + 8, lastPoint.y() - 14, 170, 16),
                         Qt::AlignLeft | Qt::AlignVCenter,
                         QStringLiteral("(%1, %2)")
                             .arg(QString::number(m_points.back().metricA, 'f', 3),
                                  QString::number(m_points.back().metricB, 'f', 3)));
    }

private:
    QVector<RecordMetricPoint> m_points;
    QString m_metricALabel;
    QString m_metricBLabel;
    BookSimResultUi::Theme m_theme;
};

} // namespace

BookSimResultPage::BookSimResultPage(QWidget* parent)
    : BasePage(parent) {
    setWindowTitle(tr("BookSim 结果"));

    auto* central = new QWidget(this);
    m_pageRoot = central;
    central->setWindowTitle(tr("BookSim 结果"));
    central->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    auto* mainLay = new QVBoxLayout(central);
    mainLay->setContentsMargins(8, 0, 8, 0);
    mainLay->setSpacing(16);

    auto* toolbarArea = new ElaScrollPageArea(this);
    auto* toolbarLay = new QHBoxLayout(toolbarArea);
    toolbarLay->setContentsMargins(0, 0, 0, 0);

    m_pasteButton = new ElaPushButton(tr("从剪贴板解析"), this);
    toolbarLay->addWidget(m_pasteButton);
    toolbarLay->addStretch();

    m_statusText = new ElaText(tr("等待仿真结果"), this);
    m_statusText->setTextPixelSize(BookSimResultUi::TypePx::kStatus);
    m_statusText->setWordWrap(true);
    applySelectableLabel(m_statusText);

    m_scrollInner = new QWidget(central);
    m_scrollInner->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    m_bodyLayout = new QVBoxLayout(m_scrollInner);
    m_bodyLayout->setContentsMargins(0, 0, 0, 0);
    m_bodyLayout->setSpacing(16);

    m_scroll = new QScrollArea(central);
    m_scroll->setObjectName(QStringLiteral("BookSimResultScroll"));
    m_scroll->setWidgetResizable(true);
    m_scroll->setWidget(m_scrollInner);
    m_scroll->setFrameShape(QFrame::NoFrame);
    m_scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scroll->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    mainLay->addWidget(toolbarArea);
    mainLay->addWidget(m_statusText);
    mainLay->addWidget(m_scroll, 1);

    connect(m_pasteButton, &ElaPushButton::clicked, this, &BookSimResultPage::loadFromClipboard);

    setStatus(tr("等待仿真结果"), false);

    connect(eTheme, &ElaTheme::themeModeChanged, this, [this](ElaThemeType::ThemeMode) {
        // 主题切换时先刷新状态文案颜色，避免保留上一个主题的黑/白字。
        setStatus(m_statusText->text(), m_statusIsError);
        applyResultPageChrome();
        if (m_showingRecordLineChart) {
            rebuildRecordLineChart();
            return;
        }
        if (m_lastSimulationLog.isEmpty()) {
            return;
        }
        rebuildContent(BookSimStatsParser::parseOverallFromLog(m_lastSimulationLog));
    });

    addCentralWidget(central, true, true, 0);
    applyResultPageChrome();
}

void BookSimResultPage::ingestSimulationLog(const QString& text) {
    m_showingRecordLineChart = false;
    m_lastSimulationLog = text;
    rebuildContent(BookSimStatsParser::parseOverallFromLog(text));
}

void BookSimResultPage::ingestRecordLineChart(const QList<SimulationRecordSnapshot>& records,
                                              const QString& metricAKey,
                                              const QString& metricALabel,
                                              const QString& metricBKey,
                                              const QString& metricBLabel) {
    m_showingRecordLineChart = true;
    m_chartRecords = records;
    m_chartMetricAKey = metricAKey;
    m_chartMetricALabel = metricALabel;
    m_chartMetricBKey = metricBKey;
    m_chartMetricBLabel = metricBLabel;
    rebuildRecordLineChart();
}

void BookSimResultPage::loadFromClipboard() {
    const QClipboard* clip = QApplication::clipboard();
    if (!clip) {
        setStatus(tr("无法访问剪贴板。"), true);
        return;
    }
    ingestSimulationLog(clip->text());
}

void BookSimResultPage::setStatus(const QString& message, bool isError) {
    m_statusIsError = isError;
    m_statusText->setText(message);
    const BookSimResultUi::Theme th = BookSimResultUi::themeFrom(eTheme->getThemeMode());
    if (isError) {
        m_statusText->setStyleSheet(
            QStringLiteral("font-size: %1px; color: %2;")
                .arg(BookSimResultUi::TypePx::kStatus)
                .arg(ElaThemeColor(eTheme->getThemeMode(), StatusDanger).name(QColor::HexRgb)));
    } else {
        m_statusText->setStyleSheet(QStringLiteral("font-size: %1px; color: %2;")
                                        .arg(BookSimResultUi::TypePx::kStatus)
                                        .arg(th.fgMain.name(QColor::HexRgb)));
    }
}

void BookSimResultPage::applyResultPageChrome() {
    if (!m_pageRoot) {
        return;
    }
    const BookSimResultUi::Theme th = BookSimResultUi::themeFrom(eTheme->getThemeMode());
    const QString bgCss = th.pageBg.name(QColor::HexRgb);

    // ElaScrollPage::addCentralWidget 会把 central 改为 objectName=ElaScrollPage_CentralPage 且
    // background 设为 transparent，若不覆盖则外層 ElaScrollArea 的深色会透出（浅色主题下像黑底）。
    m_pageRoot->setAttribute(Qt::WA_StyledBackground, true);
    m_pageRoot->setStyleSheet(
        QStringLiteral("#ElaScrollPage_CentralPage { background-color: %1; }").arg(bgCss));

    if (QWidget* outerVp = m_pageRoot->parentWidget()) {
        outerVp->setAutoFillBackground(true);
        QPalette op = outerVp->palette();
        op.setColor(QPalette::Window, th.pageBg);
        outerVp->setPalette(op);
    }

    m_scroll->setStyleSheet(QStringLiteral("#BookSimResultScroll { background: transparent; "
                                           "border: none; }"));
    if (m_scroll->viewport()) {
        m_scroll->viewport()->setStyleSheet(QStringLiteral("background: transparent;"));
    }
    m_scrollInner->setStyleSheet(QStringLiteral("background: transparent;"));
}

void BookSimResultPage::rebuildContent(const BookSimParseResult& result) {
    while (QLayoutItem* item = m_bodyLayout->takeAt(0)) {
        delete item->widget();
        delete item;
    }

    const BookSimResultUi::Theme th = BookSimResultUi::themeFrom(eTheme->getThemeMode());

    if (!result.ok()) {
        setStatus(result.errorMessage, true);
        auto* err = new QLabel(result.errorMessage, m_scrollInner);
        err->setWordWrap(true);
        err->setStyleSheet(QStringLiteral("font-size: %1px; color: %2;")
                               .arg(BookSimResultUi::TypePx::kLead)
                               .arg(th.fgMain.name(QColor::HexRgb)));
        applySelectableLabel(err);
        m_bodyLayout->addWidget(err);
        m_bodyLayout->addStretch();
        return;
    }

    setStatus(tr("已解析：%1 个 class").arg(result.classes.size()), false);

    if (result.totalRunTimeSec.has_value()) {
        auto* wall = new QFrame(m_scrollInner);
        wall->setStyleSheet(
            QStringLiteral("QFrame { background-color: %1; border-radius: 12px; "
                           "border: 1px solid %2; }")
                .arg(BookSimResultUi::rgba(BookSimResultUi::mix(th.cardBg, th.border, 0.75)),
                     BookSimResultUi::rgba(th.border)));
        auto* wh = new QHBoxLayout(wall);
        wh->setContentsMargins(16, 10, 16, 10);
        auto* wlab = new QLabel(tr("总耗时"), wall);
        wlab->setStyleSheet(QStringLiteral("font-size: %1px; color: %2;")
                                .arg(BookSimResultUi::TypePx::kBody)
                                .arg(BookSimResultUi::rgba(th.fgDim)));
        applySelectableLabel(wlab);
        auto* wval = new QLabel(QStringLiteral("%1 s").arg(*result.totalRunTimeSec, 0, 'g', 9),
                                wall);
        wval->setStyleSheet(QStringLiteral("font-size: %1px; font-weight: 600; font-family: 'SF "
                                           "Mono', 'Menlo', 'Consolas', monospace; color: %2;")
                                .arg(BookSimResultUi::TypePx::kLead)
                                .arg(th.fgMain.name(QColor::HexRgb)));
        applySelectableLabel(wval);
        wh->addWidget(wlab);
        wh->addStretch();
        wh->addWidget(wval);
        m_bodyLayout->addWidget(wall);
    }

    if (result.classes.size() == 1) {
        m_bodyLayout->addWidget(buildClassPanel(m_scrollInner, result.classes.front()));
    } else {
        auto* tabs = new QTabWidget(m_scrollInner);
        tabs->setDocumentMode(true);
        tabs->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        tabs->setStyleSheet(
            QStringLiteral("QTabWidget::pane { border: none; background: transparent; } "
                           "QTabBar::tab { color: %1; padding: 8px 16px; } "
                           "QTabBar::tab:selected { font-weight: 600; }")
                .arg(th.fgMain.name(QColor::HexRgb)));
        for (const auto& c : result.classes) {
            auto* page = new QWidget(tabs);
            auto* v = new QVBoxLayout(page);
            v->setContentsMargins(4, 10, 4, 10);
            v->addWidget(buildClassPanel(page, c));
            v->addStretch();
            tabs->addTab(page, tr("Class %1").arg(c.classId));
        }
        m_bodyLayout->addWidget(tabs);
    }

    m_bodyLayout->addStretch();
}

void BookSimResultPage::rebuildRecordLineChart() {
    while (QLayoutItem* item = m_bodyLayout->takeAt(0)) {
        delete item->widget();
        delete item;
    }

    const BookSimResultUi::Theme th = BookSimResultUi::themeFrom(eTheme->getThemeMode());
    const QVector<RecordMetricPoint> points = buildRecordMetricPoints(m_chartRecords,
                                                                      m_chartMetricAKey,
                                                                      m_chartMetricBKey);
    if (points.size() < 2) {
        setStatus(tr("折线图创建失败：有效点不足 2 个"), true);
        auto* err = new QLabel(tr("请检查记录和指标是否为数值。"), m_scrollInner);
        err->setWordWrap(true);
        err->setStyleSheet(QStringLiteral("font-size: %1px; color: %2;")
                               .arg(BookSimResultUi::TypePx::kLead)
                               .arg(th.fgMain.name(QColor::HexRgb)));
        applySelectableLabel(err);
        m_bodyLayout->addWidget(err);
        m_bodyLayout->addStretch();
        return;
    }

    setStatus(tr("折线图：%1 条记录，X[%2]，Y[%3]")
                  .arg(points.size())
                  .arg(m_chartMetricALabel, m_chartMetricBLabel),
              false);

    auto* summary = new QFrame(m_scrollInner);
    summary->setStyleSheet(
        QStringLiteral(
            "QFrame { background-color: %1; border-radius: 12px; border: 1px solid %2; }")
            .arg(BookSimResultUi::rgba(BookSimResultUi::mix(th.cardBg, th.border, 0.75)),
                 BookSimResultUi::rgba(th.border)));
    auto* summaryLayout = new QVBoxLayout(summary);
    summaryLayout->setContentsMargins(16, 12, 16, 12);
    summaryLayout->setSpacing(6);

    auto* title = new QLabel(tr("仿真记录对比折线图"), summary);
    title->setStyleSheet(QStringLiteral("font-size: %1px; font-weight: 600; color: %2;")
                             .arg(BookSimResultUi::TypePx::kSection)
                             .arg(th.fgMain.name(QColor::HexRgb)));
    applySelectableLabel(title);
    auto* subtitle = new QLabel(tr("按 X 指标升序连线"), summary);
    subtitle->setWordWrap(true);
    subtitle->setStyleSheet(QStringLiteral("font-size: %1px; color: %2;")
                                .arg(BookSimResultUi::TypePx::kBody)
                                .arg(BookSimResultUi::rgba(th.fgMain)));
    applySelectableLabel(subtitle);
    summaryLayout->addWidget(title);
    summaryLayout->addWidget(subtitle);
    m_bodyLayout->addWidget(summary);

    auto* chartCard = new QFrame(m_scrollInner);
    chartCard->setStyleSheet(
        QStringLiteral(
            "QFrame { background-color: %1; border-radius: 16px; border: 1px solid %2; }")
            .arg(BookSimResultUi::rgba(BookSimResultUi::mix(th.cardBg, th.pageBg, 0.86)),
                 BookSimResultUi::rgba(th.border)));
    auto* chartCardLayout = new QVBoxLayout(chartCard);
    chartCardLayout->setContentsMargins(14, 14, 14, 14);
    chartCardLayout->setSpacing(10);

    auto* legend = new QLabel(QStringLiteral(
                                  "<span style=\"font-weight:600;color:%1;\">X: %2</span>"
                                  "<span style=\"color:%3;\">  |  </span>"
                                  "<span style=\"font-weight:600;color:%4;\">Y: %5</span>")
                                  .arg(th.chartPacket.name(QColor::HexRgb),
                                       m_chartMetricALabel,
                                       th.fgDim.name(QColor::HexRgb),
                                       th.chartNetwork.name(QColor::HexRgb),
                                       m_chartMetricBLabel),
                              chartCard);
    legend->setTextFormat(Qt::RichText);
    legend->setStyleSheet(QStringLiteral("font-size: %1px;").arg(BookSimResultUi::TypePx::kLead));
    applySelectableLabel(legend);
    chartCardLayout->addWidget(legend);
    chartCardLayout->addWidget(new DualMetricLineChartWidget(points,
                                                             m_chartMetricALabel,
                                                             m_chartMetricBLabel,
                                                             th,
                                                             chartCard),
                               1);
    auto* chartCopy = new QPlainTextEdit(chartCard);
    chartCopy->setReadOnly(true);
    chartCopy->setPlainText(
        buildRecordChartPlainDump(points, m_chartMetricALabel, m_chartMetricBLabel));
    chartCopy->setMaximumBlockCount(0);
    chartCopy->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    chartCopy->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    chartCopy->setMinimumHeight(140);
    chartCopy->setMaximumHeight(260);
    chartCopy->setStyleSheet(
        QStringLiteral("QPlainTextEdit { font-family: 'SF Mono', 'Menlo', 'Consolas', monospace; "
                       "font-size: %1px; color: %2; background: %3; border: 1px solid %4; "
                       "border-radius: 8px; padding: 8px; }")
            .arg(BookSimResultUi::TypePx::kBody)
            .arg(th.fgMain.name(QColor::HexRgb))
            .arg(BookSimResultUi::rgba(BookSimResultUi::mix(th.cardBg, th.pageBg, 0.92)))
            .arg(BookSimResultUi::rgba(th.border)));
    chartCardLayout->addWidget(chartCopy);
    m_bodyLayout->addWidget(chartCard);

    auto* table = new QTableWidget(static_cast<int>(points.size()), 3, m_scrollInner);
    table->setHorizontalHeaderLabels({tr("记录"), m_chartMetricALabel, m_chartMetricBLabel});
    table->verticalHeader()->setVisible(false);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionMode(QAbstractItemView::ExtendedSelection);
    table->setSelectionBehavior(QAbstractItemView::SelectItems);
    table->setAlternatingRowColors(true);
    table->setShowGrid(false);
    table->setFocusPolicy(Qt::StrongFocus);
    table->horizontalHeader()->setStretchLastSection(true);
    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    for (int i = 0; i < points.size(); ++i) {
        table->setItem(i,
                       0,
                       new QTableWidgetItem(
                           QStringLiteral("%1  %2").arg(points[i].label, points[i].fullName)));
        table->setItem(i, 1, new QTableWidgetItem(QString::number(points[i].metricA, 'f', 6)));
        table->setItem(i, 2, new QTableWidgetItem(QString::number(points[i].metricB, 'f', 6)));
    }
    table->setStyleSheet(
        QStringLiteral("QTableWidget { background: %1; border: 1px solid %2; border-radius: 10px; "
                       "alternate-background-color: %3; color: %4; } "
                       "QHeaderView::section { background: %5; color: %4; border: none; "
                       "padding: 6px 8px; font-weight: 600; }")
            .arg(BookSimResultUi::rgba(BookSimResultUi::mix(th.cardBg, th.pageBg, 0.9)),
                 BookSimResultUi::rgba(th.border),
                 BookSimResultUi::rgba(BookSimResultUi::mix(th.cardBg, th.pageBg, 0.78)),
                 th.fgMain.name(QColor::HexRgb),
                 BookSimResultUi::rgba(BookSimResultUi::mix(th.tileBg, th.pageBg, 0.72))));
    m_bodyLayout->addWidget(table);
    m_bodyLayout->addStretch();
}

QFrame* BookSimResultPage::createCategoryCard(QWidget* host,
                                              const QString& title,
                                              const QString& subtitle,
                                              const QVector<BookSimMetricRow>& rows,
                                              QWidget* extraWidget,
                                              const QString& accentColor,
                                              bool primaryEmphasis,
                                              bool splitAvgAndRange) {
    const BookSimResultUi::Theme th = BookSimResultUi::themeFrom(eTheme->getThemeMode());
    auto* card = new QFrame(host);
    card->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    const QColor accent = QColor(accentColor);
    const QString borderLeft = accent.isValid() && accent.alpha() > 0
                                   ? QStringLiteral("4px solid %1").arg(accent.name(QColor::HexRgb))
                                   : QStringLiteral("1px solid %1")
                                         .arg(BookSimResultUi::rgba(th.border));
    const QString bg = BookSimResultUi::rgba(primaryEmphasis ? th.cardBgPrimary : th.cardBg);
    card->setStyleSheet(QStringLiteral("QFrame {"
                                       "background: %1;"
                                       "border-radius: 20px;"
                                       "border: 1px solid %2;"
                                       "border-left: %3;"
                                       "}")
                            .arg(bg, BookSimResultUi::rgba(th.border), borderLeft));

    auto* v = new QVBoxLayout(card);
    v->setContentsMargins(22, 18, 22, 18);
    v->setSpacing(12);

    auto* titleT = new ElaText(title, card);
    titleT->setTextPixelSize(primaryEmphasis ? BookSimResultUi::TypePx::kCardTitleEmph
                                             : BookSimResultUi::TypePx::kCardTitle);
    titleT->setStyleSheet(QStringLiteral("color: %1;").arg(th.fgMain.name(QColor::HexRgb)));
    applySelectableLabel(titleT);
    v->addWidget(titleT);

    if (!subtitle.isEmpty()) {
        auto* sub = new QLabel(subtitle, card);
        sub->setWordWrap(true);
        sub->setStyleSheet(QStringLiteral("font-size: %1px; color: %2;")
                               .arg(BookSimResultUi::TypePx::kBody)
                               .arg(BookSimResultUi::rgba(th.fgMain)));
        applySelectableLabel(sub);
        v->addWidget(sub);
    }

    if (extraWidget != nullptr) {
        v->addWidget(extraWidget);
    }

    const QColor tableInnerBg = BookSimResultUi::mix(th.cardBg,
                                                     th.tileBg,
                                                     primaryEmphasis ? 0.38 : 0.52);
    auto* tableWrap = new QFrame(card);
    tableWrap->setObjectName(QStringLiteral("metricTableWrap"));
    tableWrap->setStyleSheet(
        QStringLiteral("QFrame#metricTableWrap { background-color: %1; border: 1px solid %2; "
                       "border-radius: 14px; }")
            .arg(BookSimResultUi::rgba(tableInnerBg), BookSimResultUi::rgba(th.border)));
    auto* tableWrapLay = new QVBoxLayout(tableWrap);
    tableWrapLay->setContentsMargins(14, 12, 14, 14);
    tableWrapLay->setSpacing(0);

    auto* grid = new QGridLayout();
    grid->setHorizontalSpacing(18);
    grid->setVerticalSpacing(splitAvgAndRange ? 10 : 8);
    int r = 0;
    const QString headerStyle
        = QStringLiteral("font-size: %1px; font-weight: 600; color: %2; padding-bottom: 8px; "
                         "border-bottom: 1px solid %3;")
              .arg(BookSimResultUi::TypePx::kBody)
              .arg(BookSimResultUi::rgba(th.textHint))
              .arg(BookSimResultUi::rgba(BookSimResultUi::mix(th.border, tableInnerBg, 0.62)));
    const QString valueStyle
        = QStringLiteral("font-size: %1px; font-family: 'SF Mono', 'Menlo', 'Consolas', monospace; "
                         "color: %2;")
              .arg(BookSimResultUi::TypePx::kMetricValue)
              .arg(th.fgMain.name(QColor::HexRgb));
    const QString nameStyle = QStringLiteral("font-size: %1px; font-weight: 500; color: %2;")
                                  .arg(BookSimResultUi::TypePx::kBody)
                                  .arg(th.fgMain.name(QColor::HexRgb));

    if (splitAvgAndRange) {
        auto* h0 = new QLabel(tr("指标"), tableWrap);
        h0->setStyleSheet(headerStyle);
        applySelectableLabel(h0);
        auto* h1 = new QLabel(tr("平均值"), tableWrap);
        h1->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        h1->setStyleSheet(headerStyle);
        applySelectableLabel(h1);
        auto* h2 = new QLabel(tr("[ 最小值， 最大值 ]"), tableWrap);
        h2->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        h2->setStyleSheet(headerStyle);
        applySelectableLabel(h2);
        grid->addWidget(h0, r, 0);
        grid->addWidget(h1, r, 1);
        grid->addWidget(h2, r, 2);
        ++r;
    } else if (!rows.isEmpty()) {
        auto* h0 = new QLabel(tr("指标"), tableWrap);
        h0->setStyleSheet(headerStyle);
        applySelectableLabel(h0);
        auto* h1 = new QLabel(tr("平均值"), tableWrap);
        h1->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        h1->setStyleSheet(headerStyle);
        applySelectableLabel(h1);
        grid->addWidget(h0, r, 0);
        grid->addWidget(h1, r, 1);
        ++r;
    }

    for (const BookSimMetricRow& row : rows) {
        auto* nameL = new QLabel(row.name, tableWrap);
        nameL->setStyleSheet(nameStyle);
        applySelectableLabel(nameL);
        grid->addWidget(nameL, r, 0);

        if (splitAvgAndRange) {
            auto* avgL = new QLabel(row.avgOrValue, tableWrap);
            avgL->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
            avgL->setStyleSheet(valueStyle);
            applySelectableLabel(avgL);
            const QString rangeShow = row.range.isEmpty() ? QStringLiteral("—") : row.range;
            auto* rangeL = new QLabel(rangeShow, tableWrap);
            rangeL->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
            rangeL->setStyleSheet(valueStyle);
            applySelectableLabel(rangeL);
            grid->addWidget(avgL, r, 1);
            grid->addWidget(rangeL, r, 2);
        } else {
            auto* valL = new QLabel(row.avgOrValue, tableWrap);
            valL->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
            valL->setStyleSheet(valueStyle);
            applySelectableLabel(valL);
            grid->addWidget(valL, r, 1);
        }
        ++r;
        if (!row.hint.isEmpty()) {
            auto* hintL = new QLabel(row.hint, tableWrap);
            hintL->setWordWrap(true);
            hintL->setStyleSheet(QStringLiteral("font-size: %1px; color: %2;")
                                     .arg(BookSimResultUi::TypePx::kBody)
                                     .arg(BookSimResultUi::rgba(th.fgMain)));
            applySelectableLabel(hintL);
            grid->addWidget(hintL, r, 0, 1, splitAvgAndRange ? 3 : 2);
            ++r;
        }
    }
    grid->setColumnStretch(0, 1);
    if (splitAvgAndRange) {
        grid->setColumnMinimumWidth(1, 112);
        grid->setColumnMinimumWidth(2, 132);
    } else {
        grid->setColumnMinimumWidth(1, 112);
        grid->setColumnStretch(1, 0);
    }
    tableWrapLay->addLayout(grid);
    v->addWidget(tableWrap);
    return card;
}

QWidget* BookSimResultPage::buildClassPanel(QWidget* host, const BookSimTrafficClassStats& stats) {
    const BookSimResultUi::Theme th = BookSimResultUi::themeFrom(eTheme->getThemeMode());

    auto* panel = new QWidget(host);
    panel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    auto* lay = new QVBoxLayout(panel);
    lay->setSpacing(16);
    lay->setContentsMargins(0, 0, 0, 0);

    auto* classHead = new ElaText(tr("流量类别 %1").arg(stats.classId), panel);
    classHead->setTextPixelSize(BookSimResultUi::TypePx::kSection);
    classHead->setStyleSheet(QStringLiteral("color: %1;").arg(th.fgMain.name(QColor::HexRgb)));
    applySelectableLabel(classHead);
    lay->addWidget(classHead);

    const int sampleNote = stats.packetLatency.hasAverage ? stats.packetLatency.samples : 0;
    if (sampleNote > 0) {
        auto* badge = new QLabel(tr("样本数: %1").arg(sampleNote), panel);
        badge->setStyleSheet(QStringLiteral("font-size: %1px; color: %2; padding: 4px 0;")
                                 .arg(BookSimResultUi::TypePx::kBody)
                                 .arg(BookSimResultUi::rgba(th.fgMain)));
        applySelectableLabel(badge);
        lay->addWidget(badge);
    }

    QVector<BookSimMetricRow> latencyRows;
    latencyRows.push_back(
        metricRowFromBand(BookSimMetricLabels::packetLatency(), stats.packetLatency, 4));
    latencyRows.push_back(
        metricRowFromBand(BookSimMetricLabels::networkLatency(), stats.networkLatency, 4));
    latencyRows.push_back(
        metricRowFromBand(BookSimMetricLabels::flitLatency(), stats.flitLatency, 4));
    latencyRows.push_back(
        metricRowFromBand(BookSimMetricLabels::fragmentation(), stats.fragmentation, 6));
    lay->addWidget(createCategoryCard(panel,
                                      BookSimMetricLabels::sectionLatencyFlit(),
                                      QString(),
                                      latencyRows,
                                      nullptr,
                                      th.kpiPacket.name(QColor::HexRgb),
                                      true,
                                      true));

    const double injP = stats.injectedPacketRate.hasAverage ? stats.injectedPacketRate.average : 0.;
    const double accP = stats.acceptedPacketRate.hasAverage ? stats.acceptedPacketRate.average : 0.;
    QString matchVal;
    QString matchFoot;
    if (injP <= 1e-12) {
        matchVal = QStringLiteral("—");
        matchFoot = tr("注入包速率为 0，无法计算比值。");
    } else {
        const double ratio = accP / injP;
        matchVal = QStringLiteral("%1%").arg(ratio * 100., 0, 'f', 1);
        if (ratio >= 0.98) {
            matchFoot.clear();
        } else if (ratio >= 0.85) {
            matchFoot = tr("偏低，可能存在背压或拥塞。");
        } else {
            matchFoot = tr("明显偏低，需关注丢包或饱和。");
        }
    }

    QVector<BookSimMetricRow> thrRows;
    thrRows.push_back({BookSimMetricLabels::throughputMatch(), matchVal, QString(), matchFoot});
    thrRows.push_back(
        metricRowFromBand(BookSimMetricLabels::injectedPacketRate(), stats.injectedPacketRate, 5));
    thrRows.push_back(
        metricRowFromBand(BookSimMetricLabels::acceptedPacketRate(), stats.acceptedPacketRate, 5));
    thrRows.push_back(
        metricRowFromBand(BookSimMetricLabels::injectedFlitRate(), stats.injectedFlitRate, 5));
    thrRows.push_back(
        metricRowFromBand(BookSimMetricLabels::acceptedFlitRate(), stats.acceptedFlitRate, 5));
    lay->addWidget(createCategoryCard(panel,
                                      BookSimMetricLabels::sectionThroughput(),
                                      QString(),
                                      thrRows,
                                      nullptr,
                                      th.thrAccPkt.name(QColor::HexRgb),
                                      false,
                                      true));

    QVector<BookSimMetricRow> shapePathRows;
    shapePathRows.push_back(metricRowFromBand(BookSimMetricLabels::injectedMeanPacketSize(),
                                              stats.injectedPacketSize,
                                              4));
    shapePathRows.push_back(metricRowFromBand(BookSimMetricLabels::acceptedMeanPacketSize(),
                                              stats.acceptedPacketSize,
                                              4));
    shapePathRows.push_back(metricRowFromBand(BookSimMetricLabels::meanHops(), stats.hops, 5));
    lay->addWidget(createCategoryCard(panel,
                                      BookSimMetricLabels::sectionShapePath(),
                                      QString(),
                                      shapePathRows,
                                      nullptr,
                                      th.accentHops.name(QColor::HexRgb),
                                      false,
                                      false));

    return panel;
}
