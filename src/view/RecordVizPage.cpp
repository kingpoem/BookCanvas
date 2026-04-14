#include "RecordVizPage.h"
#include "utils/SimpleOoxmlXlsx.h"
#include "view/BookSimResultUi.h"
#include <ElaPushButton.h>
#include <ElaScrollPageArea.h>
#include <ElaText.h>
#include <ElaTheme.h>
#include <QAbstractItemView>
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QFrame>
#include <QHeaderView>
#include <QLabel>
#include <QLayout>
#include <QMessageBox>
#include <QPainter>
#include <QPalette>
#include <QPlainTextEdit>
#include <QScrollArea>
#include <QSizePolicy>
#include <QStandardPaths>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>
#include <QtSvg/QSvgGenerator>
#include <algorithm>
#include <cmath>
#include <utility>

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

struct RecordMetricPoint3D {
    QString label;
    QString fullName;
    double metricA = 0.0;
    double metricB = 0.0;
    double metricC = 0.0;
};

[[nodiscard]] QVector<RecordMetricPoint3D> buildRecordMetricPoints3D(
    const QList<SimulationRecordSnapshot>& records,
    const QString& metricAKey,
    const QString& metricBKey,
    const QString& metricCKey) {
    QVector<RecordMetricPoint3D> points;
    points.reserve(records.size());
    int ordinal = 1;
    for (const auto& record : records) {
        double a = 0.0;
        double b = 0.0;
        double c = 0.0;
        if (!extractRecordMetricValue(record, metricAKey, &a)
            || !extractRecordMetricValue(record, metricBKey, &b)
            || !extractRecordMetricValue(record, metricCKey, &c)) {
            continue;
        }
        RecordMetricPoint3D point;
        point.fullName = record.name;
        point.label = QStringLiteral("#%1").arg(ordinal);
        point.metricA = a;
        point.metricB = b;
        point.metricC = c;
        points.push_back(point);
        ++ordinal;
    }
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
    s += QLatin1Char('\n');
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

        if (m_points.size() < 2) {
            return;
        }

        const QFontMetrics fmChart(painter.font());

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

        const QString xAxisCaption = QStringLiteral("X轴：%1  [%2, %3]")
                                         .arg(m_metricALabel,
                                              QString::number(minA, 'f', 3),
                                              QString::number(maxA, 'f', 3));
        const QString yAxisCaption = QStringLiteral("Y轴：%1  [%2, %3]")
                                         .arg(m_metricBLabel,
                                              QString::number(minB, 'f', 3),
                                              QString::number(maxB, 'f', 3));
        // 左侧：原有刻度区68px + 为竖排 Y 轴标题预留（避免与右侧 Y 刻度数字重叠）
        const int yCaptionLeftGutter = fmChart.height() + 14;
        const int leftMargin = 76 + yCaptionLeftGutter;
        const int rightMargin = qBound(72, 68 + 28, 160);
        const QRectF chartRect = rect().adjusted(leftMargin, 26, -rightMargin, -84);
        if (chartRect.width() <= 40 || chartRect.height() <= 40) {
            return;
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
        for (int i = 0; i <= 6; ++i) {
            const double t = static_cast<double>(i) / 6.0;
            const double x = chartRect.left() + t * chartRect.width();
            const double valueA = minA + t * (maxA - minA);
            painter.drawLine(QPointF(x, chartRect.bottom()), QPointF(x, chartRect.bottom() + 4));
            const QString text = QString::number(valueA, 'f', 2);
            painter.drawText(QPointF(x - fmChart.horizontalAdvance(text) / 2.0,
                                     chartRect.bottom() + 20),
                             text);
        }

        painter.setPen(QPen(m_theme.chartPacket, 1.2));
        {
            const int capY = static_cast<int>(chartRect.bottom() + 24);
            painter.drawText(QRectF(chartRect.left(),
                                    static_cast<double>(capY),
                                    chartRect.width(),
                                    static_cast<double>(fmChart.height() + 2)),
                             Qt::AlignRight | Qt::AlignTop,
                             xAxisCaption);
        }

        painter.setPen(QPen(m_theme.chartNetwork, 1.2));
        painter.save();
        {
            const int th = fmChart.height();
            const int tw = fmChart.horizontalAdvance(yAxisCaption);
            // 标题放在图区左侧、左刻度列以左，竖排不挡右侧 Y 刻度
            const double labelCenterX = chartRect.left() - 68.0 - 8.0
                                        - static_cast<double>(th) * 0.5;
            painter.translate(labelCenterX, chartRect.center().y());
            painter.rotate(-90.0);
            painter.drawText(QRect(-tw / 2, -th / 2, tw, th), Qt::AlignCenter, yAxisCaption);
        }
        painter.restore();

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

[[nodiscard]] QString buildRecordScatter3DPlainDump(const QVector<RecordMetricPoint3D>& points,
                                                    const QString& metricALabel,
                                                    const QString& metricBLabel,
                                                    const QString& metricCLabel) {
    if (points.isEmpty()) {
        return {};
    }
    double minA = points.front().metricA;
    double maxA = points.front().metricA;
    double minB = points.front().metricB;
    double maxB = points.front().metricB;
    double minC = points.front().metricC;
    double maxC = points.front().metricC;
    for (const auto& p : points) {
        minA = std::min(minA, p.metricA);
        maxA = std::max(maxA, p.metricA);
        minB = std::min(minB, p.metricB);
        maxB = std::max(maxB, p.metricB);
        minC = std::min(minC, p.metricC);
        maxC = std::max(maxC, p.metricC);
    }
    if (std::abs(maxA - minA) < 1e-12) {
        maxA += 1.0;
        minA -= 1.0;
    }
    if (std::abs(maxB - minB) < 1e-12) {
        maxB += 1.0;
        minB -= 1.0;
    }
    if (std::abs(maxC - minC) < 1e-12) {
        maxC += 1.0;
        minC -= 1.0;
    }
    QString s;
    s += QStringLiteral("X：%1  [%2, %3]\n")
             .arg(metricALabel, QString::number(minA, 'f', 3), QString::number(maxA, 'f', 3));
    s += QStringLiteral("Y：%1  [%2, %3]\n")
             .arg(metricBLabel, QString::number(minB, 'f', 3), QString::number(maxB, 'f', 3));
    s += QStringLiteral("Z：%1  [%2, %3]\n")
             .arg(metricCLabel, QString::number(minC, 'f', 3), QString::number(maxC, 'f', 3));
    s += QLatin1Char('\n');
    s += QStringLiteral("记录\t%1\t%2\t%3\n").arg(metricALabel, metricBLabel, metricCLabel);
    for (const auto& p : points) {
        s += QStringLiteral("%1 %2\t%3\t%4\t%5\n")
                 .arg(p.label,
                      p.fullName,
                      QString::number(p.metricA, 'f', 6),
                      QString::number(p.metricB, 'f', 6),
                      QString::number(p.metricC, 'f', 6));
    }
    return s;
}

class TripleMetricScatter3DWidget : public QWidget {
public:
    TripleMetricScatter3DWidget(const QVector<RecordMetricPoint3D>& points,
                                const QString& metricALabel,
                                const QString& metricBLabel,
                                const QString& metricCLabel,
                                const BookSimResultUi::Theme& theme,
                                QWidget* parent = nullptr)
        : QWidget(parent)
        , m_points(points)
        , m_metricALabel(metricALabel)
        , m_metricBLabel(metricBLabel)
        , m_metricCLabel(metricCLabel)
        , m_theme(theme) {
        setMinimumHeight(420);
        setMouseTracking(true);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        setCursor(Qt::OpenHandCursor);
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

        const QRectF chartRect = rect().adjusted(56, 44, -56, -76);
        if (chartRect.width() <= 40 || chartRect.height() <= 40 || m_points.isEmpty()) {
            return;
        }

        double minA = m_points.front().metricA;
        double maxA = m_points.front().metricA;
        double minB = m_points.front().metricB;
        double maxB = m_points.front().metricB;
        double minC = m_points.front().metricC;
        double maxC = m_points.front().metricC;
        for (const auto& p : m_points) {
            minA = std::min(minA, p.metricA);
            maxA = std::max(maxA, p.metricA);
            minB = std::min(minB, p.metricB);
            maxB = std::max(maxB, p.metricB);
            minC = std::min(minC, p.metricC);
            maxC = std::max(maxC, p.metricC);
        }
        if (std::abs(maxA - minA) < 1e-12) {
            maxA += 1.0;
            minA -= 1.0;
        }
        if (std::abs(maxB - minB) < 1e-12) {
            maxB += 1.0;
            minB -= 1.0;
        }
        if (std::abs(maxC - minC) < 1e-12) {
            maxC += 1.0;
            minC -= 1.0;
        }

        auto normX = [&](double v) { return (v - minA) / (maxA - minA) - 0.5; };
        auto normY = [&](double v) { return (v - minB) / (maxB - minB) - 0.5; };
        auto normZ = [&](double v) { return (v - minC) / (maxC - minC) - 0.5; };

        const QPointF center = chartRect.center();
        const double unit = 0.42 * std::min(chartRect.width(), chartRect.height());

        auto rotateProject =
            [&](double cx, double cy, double cz, double* outX, double* outY, double* outZ) {
                const double cyaw = std::cos(m_yaw);
                const double syaw = std::sin(m_yaw);
                const double cp = std::cos(m_pitch);
                const double sp = std::sin(m_pitch);
                const double x1 = cx * cyaw + cz * syaw;
                const double z1 = -cx * syaw + cz * cyaw;
                const double y2 = cy * cp - z1 * sp;
                const double z2 = cy * sp + z1 * cp;
                *outX = x1;
                *outY = y2;
                *outZ = z2;
            };

        auto toScreen = [&](double cx, double cy, double cz) {
            double x1 = 0.0;
            double y2 = 0.0;
            double z2 = 0.0;
            rotateProject(cx, cy, cz, &x1, &y2, &z2);
            return std::pair<QPointF, double>{QPointF(center.x() + x1 * unit,
                                                      center.y() - y2 * unit),
                                              z2};
        };

        const double axisHalf = 0.58;
        const QPointF origin = toScreen(-axisHalf, -axisHalf, -axisHalf).first;
        const QPointF tipX = toScreen(axisHalf, -axisHalf, -axisHalf).first;
        const QPointF tipY = toScreen(-axisHalf, axisHalf, -axisHalf).first;
        const QPointF tipZ = toScreen(-axisHalf, -axisHalf, axisHalf).first;

        painter.setPen(QPen(m_theme.chartPacket, 2.0));
        painter.drawLine(origin, tipX);
        painter.setPen(QPen(m_theme.chartNetwork, 2.0));
        painter.drawLine(origin, tipY);
        painter.setPen(QPen(m_theme.chartFlit, 2.0));
        painter.drawLine(origin, tipZ);

        QFont axisFont = painter.font();
        axisFont.setPointSizeF(std::max(9.0, axisFont.pointSizeF()));
        painter.setFont(axisFont);
        painter.setPen(QPen(m_theme.chartPacket, 1));
        painter.drawText(tipX + QPointF(6, 4), QStringLiteral("X %1").arg(m_metricALabel));
        painter.setPen(QPen(m_theme.chartNetwork, 1));
        painter.drawText(tipY + QPointF(6, 4), QStringLiteral("Y %1").arg(m_metricBLabel));
        painter.setPen(QPen(m_theme.chartFlit, 1));
        painter.drawText(tipZ + QPointF(6, 4), QStringLiteral("Z %1").arg(m_metricCLabel));

        QVector<std::pair<double, int>> order;
        order.reserve(m_points.size());
        for (int i = 0; i < m_points.size(); ++i) {
            const auto& p = m_points[i];
            double x1 = 0.0;
            double y2 = 0.0;
            double z2 = 0.0;
            rotateProject(normX(p.metricA), normY(p.metricB), normZ(p.metricC), &x1, &y2, &z2);
            order.push_back({z2, i});
        }
        std::sort(order.begin(), order.end(), [](const auto& lhs, const auto& rhs) {
            return lhs.first < rhs.first;
        });

        for (const auto& [depth, idx] : order) {
            Q_UNUSED(depth);
            const auto& p = m_points[idx];
            const auto [pt, z2] = toScreen(normX(p.metricA), normY(p.metricB), normZ(p.metricC));
            Q_UNUSED(z2);
            const double t = (z2 + 1.2) / 2.4; // 粗略深度明暗（[-1.2,1.2] 近似）
            const double k = std::clamp(t, 0.0, 1.0);
            QColor fill = BookSimResultUi::mix(m_theme.chartPacket,
                                               m_theme.fgMain,
                                               0.35 * (1.0 - k));
            fill.setAlpha(230);
            painter.setPen(QPen(m_theme.pageBg, 1.2));
            painter.setBrush(fill);
            painter.drawEllipse(pt, 5.0, 5.0);
        }

        painter.setBrush(Qt::NoBrush);
        painter.setPen(QPen(m_theme.fgDim, 1));
        painter.setFont(painter.font());
        painter.drawText(QRectF(chartRect.left(), chartRect.bottom() + 8, chartRect.width(), 22),
                         Qt::AlignHCenter | Qt::AlignTop,
                         QCoreApplication::translate("RecordVizPage", "拖动鼠标旋转视角"));
    }

    void mousePressEvent(QMouseEvent* event) override {
        if (event->button() == Qt::LeftButton) {
            m_dragging = true;
            m_lastMouse = event->pos();
            setCursor(Qt::ClosedHandCursor);
        }
        QWidget::mousePressEvent(event);
    }

    void mouseMoveEvent(QMouseEvent* event) override {
        if (m_dragging) {
            const QPoint delta = event->pos() - m_lastMouse;
            m_lastMouse = event->pos();
            m_yaw += static_cast<double>(delta.x()) * 0.012;
            m_pitch -= static_cast<double>(delta.y()) * 0.012;
            m_pitch = std::clamp(m_pitch, -1.48, 1.48);
            update();
        }
        QWidget::mouseMoveEvent(event);
    }

    void mouseReleaseEvent(QMouseEvent* event) override {
        if (event->button() == Qt::LeftButton) {
            m_dragging = false;
            setCursor(Qt::OpenHandCursor);
        }
        QWidget::mouseReleaseEvent(event);
    }

private:
    QVector<RecordMetricPoint3D> m_points;
    QString m_metricALabel;
    QString m_metricBLabel;
    QString m_metricCLabel;
    BookSimResultUi::Theme m_theme;
    double m_yaw = 0.65;
    double m_pitch = 0.42;
    bool m_dragging = false;
    QPoint m_lastMouse;
};

} // namespace

RecordVizPage::RecordVizPage(QWidget* parent)
    : BasePage(parent) {
    setWindowTitle(tr("结果可视化"));

    auto* central = new QWidget(this);
    m_pageRoot = central;
    central->setWindowTitle(tr("结果可视化"));
    central->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    auto* mainLay = new QVBoxLayout(central);
    mainLay->setContentsMargins(8, 0, 8, 0);
    mainLay->setSpacing(16);

    auto* toolbarArea = new ElaScrollPageArea(this);
    auto* toolbarLay = new QHBoxLayout(toolbarArea);
    toolbarLay->setContentsMargins(0, 0, 0, 0);

    m_exportPngBtn = new ElaPushButton(tr("导出 PNG"), this);
    m_exportJpgBtn = new ElaPushButton(tr("导出 JPG"), this);
    m_exportSvgBtn = new ElaPushButton(tr("导出 SVG"), this);
    m_exportXlsxBtn = new ElaPushButton(tr("导出 XLSX"), this);
    toolbarLay->addWidget(m_exportPngBtn);
    toolbarLay->addWidget(m_exportJpgBtn);
    toolbarLay->addWidget(m_exportSvgBtn);
    toolbarLay->addWidget(m_exportXlsxBtn);
    toolbarLay->addStretch();

    m_statusText = new ElaText(tr("暂无图表"), this);
    m_statusText->setTextPixelSize(BookSimResultUi::TypePx::kStatus);
    m_statusText->setWordWrap(true);
    applySelectableLabel(m_statusText);

    m_scrollInner = new QWidget(central);
    m_scrollInner->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    m_bodyLayout = new QVBoxLayout(m_scrollInner);
    m_bodyLayout->setContentsMargins(0, 0, 0, 0);
    m_bodyLayout->setSpacing(16);

    m_scroll = new QScrollArea(central);
    m_scroll->setObjectName(QStringLiteral("RecordVizScroll"));
    m_scroll->setWidgetResizable(true);
    m_scroll->setWidget(m_scrollInner);
    m_scroll->setFrameShape(QFrame::NoFrame);
    m_scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scroll->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    mainLay->addWidget(toolbarArea);
    mainLay->addWidget(m_statusText);
    mainLay->addWidget(m_scroll, 1);

    connect(m_exportPngBtn, &ElaPushButton::clicked, this, &RecordVizPage::exportChartPng);
    connect(m_exportJpgBtn, &ElaPushButton::clicked, this, &RecordVizPage::exportChartJpg);
    connect(m_exportSvgBtn, &ElaPushButton::clicked, this, &RecordVizPage::exportChartSvg);
    connect(m_exportXlsxBtn, &ElaPushButton::clicked, this, &RecordVizPage::exportChartDataXlsx);

    connect(eTheme, &ElaTheme::themeModeChanged, this, [this](ElaThemeType::ThemeMode) {
        setStatus(m_statusText->text(), m_statusIsError);
        applyPageChrome();
        if (m_vizKind == VizKind::Line) {
            rebuildLineChart();
            return;
        }
        if (m_vizKind == VizKind::Scatter3D) {
            rebuildScatter3D();
            return;
        }
        rebuildEmptyHint();
    });

    addCentralWidget(central, true, true, 0);
    applyPageChrome();
    rebuildEmptyHint();
}

void RecordVizPage::clearBody() {
    while (QLayoutItem* item = m_bodyLayout->takeAt(0)) {
        delete item->widget();
        delete item;
    }
    m_chartExportHost = nullptr;
}

void RecordVizPage::rebuildEmptyHint() {
    clearBody();
    m_vizKind = VizKind::None;
    const BookSimResultUi::Theme th = BookSimResultUi::themeFrom(eTheme->getThemeMode());
    auto* hint = new QLabel(tr("请在「仿真记录」页创建折线图或三维图，图表将在此显示。"),
                            m_scrollInner);
    hint->setWordWrap(true);
    hint->setStyleSheet(QStringLiteral("font-size: %1px; color: %2;")
                            .arg(BookSimResultUi::TypePx::kLead)
                            .arg(th.fgMain.name(QColor::HexRgb)));
    applySelectableLabel(hint);
    m_bodyLayout->addWidget(hint);
    m_bodyLayout->addStretch();
    setStatus(tr("暂无图表"), false);
    updateExportButtonsEnabled();
}

void RecordVizPage::setStatus(const QString& message, bool isError) {
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

void RecordVizPage::applyPageChrome() {
    if (!m_pageRoot) {
        return;
    }
    const BookSimResultUi::Theme th = BookSimResultUi::themeFrom(eTheme->getThemeMode());
    const QString bgCss = th.pageBg.name(QColor::HexRgb);
    m_pageRoot->setAttribute(Qt::WA_StyledBackground, true);
    m_pageRoot->setStyleSheet(
        QStringLiteral("#ElaScrollPage_CentralPage { background-color: %1; }").arg(bgCss));
    if (QWidget* outerVp = m_pageRoot->parentWidget()) {
        outerVp->setAutoFillBackground(true);
        QPalette op = outerVp->palette();
        op.setColor(QPalette::Window, th.pageBg);
        outerVp->setPalette(op);
    }
    m_scroll->setStyleSheet(QStringLiteral("#RecordVizScroll { background: transparent; "
                                           "border: none; }"));
    if (m_scroll->viewport()) {
        m_scroll->viewport()->setStyleSheet(QStringLiteral("background: transparent;"));
    }
    m_scrollInner->setStyleSheet(QStringLiteral("background: transparent;"));
}

void RecordVizPage::updateExportButtonsEnabled() {
    const bool ok = m_chartExportHost != nullptr;
    if (m_exportPngBtn) {
        m_exportPngBtn->setEnabled(ok);
    }
    if (m_exportJpgBtn) {
        m_exportJpgBtn->setEnabled(ok);
    }
    if (m_exportSvgBtn) {
        m_exportSvgBtn->setEnabled(ok);
    }
    if (m_exportXlsxBtn) {
        m_exportXlsxBtn->setEnabled(ok);
    }
}

QString RecordVizPage::defaultDownloadsPath(const QString& fileName) const {
    QString dir = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    if (dir.isEmpty()) {
        dir = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    }
    QDir().mkpath(dir);
    return QDir(dir).filePath(fileName);
}

void RecordVizPage::exportChartPng() {
    if (!m_chartExportHost) {
        return;
    }
    const QString stem = (m_vizKind == VizKind::Scatter3D) ? QStringLiteral("BookCanvas_scatter3d")
                                                           : QStringLiteral("BookCanvas_linechart");
    const QString path = defaultDownloadsPath(
        QStringLiteral("%1_%2.png")
            .arg(stem, QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_HHmmss"))));
    const QPixmap px = m_chartExportHost->grab();
    if (!px.save(path, "PNG")) {
        QMessageBox::warning(this, tr("导出失败"), tr("无法写入 PNG：%1").arg(path));
        return;
    }
    setStatus(tr("已导出 PNG：%1").arg(path), false);
}

void RecordVizPage::exportChartJpg() {
    if (!m_chartExportHost) {
        return;
    }
    const QString stem = (m_vizKind == VizKind::Scatter3D) ? QStringLiteral("BookCanvas_scatter3d")
                                                           : QStringLiteral("BookCanvas_linechart");
    const QString path = defaultDownloadsPath(
        QStringLiteral("%1_%2.jpg")
            .arg(stem, QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_HHmmss"))));
    QImage img = m_chartExportHost->grab().toImage();
    if (!img.save(path, "JPEG", 92)) {
        QMessageBox::warning(this, tr("导出失败"), tr("无法写入 JPG：%1").arg(path));
        return;
    }
    setStatus(tr("已导出 JPG：%1").arg(path), false);
}

void RecordVizPage::exportChartSvg() {
    if (!m_chartExportHost) {
        return;
    }
    const QString stem = (m_vizKind == VizKind::Scatter3D) ? QStringLiteral("BookCanvas_scatter3d")
                                                           : QStringLiteral("BookCanvas_linechart");
    const QString path = defaultDownloadsPath(
        QStringLiteral("%1_%2.svg")
            .arg(stem, QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_HHmmss"))));
    const QSize sz = m_chartExportHost->size();
    QSvgGenerator generator;
    generator.setFileName(path);
    generator.setSize(sz);
    generator.setViewBox(QRect(0, 0, sz.width(), sz.height()));
    generator.setTitle(QStringLiteral("BookCanvas"));
    generator.setDescription(stem);
    QPainter painter(&generator);
    m_chartExportHost->render(&painter);
    painter.end();
    if (!QFileInfo::exists(path)) {
        QMessageBox::warning(this, tr("导出失败"), tr("无法写入 SVG：%1").arg(path));
        return;
    }
    setStatus(tr("已导出 SVG：%1").arg(path), false);
}

void RecordVizPage::exportChartDataXlsx() {
    if (!m_chartExportHost || m_vizKind == VizKind::None) {
        return;
    }
    QStringList headers;
    QVector<QStringList> rows;
    QString sheetName;
    if (m_vizKind == VizKind::Line) {
        const QVector<RecordMetricPoint> points = buildRecordMetricPoints(m_chartRecords,
                                                                          m_chartMetricAKey,
                                                                          m_chartMetricBKey);
        if (points.size() < 2) {
            QMessageBox::warning(this, tr("导出失败"), tr("当前折线图有效数据不足。"));
            return;
        }
        headers = {tr("记录"), m_chartMetricALabel, m_chartMetricBLabel};
        rows.reserve(points.size());
        for (const auto& p : points) {
            rows.push_back({QStringLiteral("%1  %2").arg(p.label, p.fullName),
                            QString::number(p.metricA, 'f', 6),
                            QString::number(p.metricB, 'f', 6)});
        }
        sheetName = tr("折线图");
    } else if (m_vizKind == VizKind::Scatter3D) {
        const QVector<RecordMetricPoint3D> points = buildRecordMetricPoints3D(m_chartRecords,
                                                                              m_chartMetricAKey,
                                                                              m_chartMetricBKey,
                                                                              m_chartMetricCKey);
        if (points.size() < 2) {
            QMessageBox::warning(this, tr("导出失败"), tr("当前三维图有效数据不足。"));
            return;
        }
        headers = {tr("记录"), m_chartMetricALabel, m_chartMetricBLabel, m_chartMetricCLabel};
        rows.reserve(points.size());
        for (const auto& p : points) {
            rows.push_back({QStringLiteral("%1  %2").arg(p.label, p.fullName),
                            QString::number(p.metricA, 'f', 6),
                            QString::number(p.metricB, 'f', 6),
                            QString::number(p.metricC, 'f', 6)});
        }
        sheetName = tr("三维散点");
    } else {
        return;
    }

    const QString stem = (m_vizKind == VizKind::Scatter3D) ? QStringLiteral("BookCanvas_scatter3d")
                                                           : QStringLiteral("BookCanvas_linechart");
    const QString path = defaultDownloadsPath(
        QStringLiteral("%1_%2.xlsx")
            .arg(stem, QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_HHmmss"))));
    if (!SimpleOoxmlXlsx::writeSingleSheetTable(path, sheetName, headers, rows)) {
        QMessageBox::warning(this, tr("导出失败"), tr("无法写入 XLSX：%1").arg(path));
        return;
    }
    setStatus(tr("已导出 XLSX：%1").arg(path), false);
}

void RecordVizPage::ingestRecordLineChart(const QList<SimulationRecordSnapshot>& records,
                                          const QString& metricAKey,
                                          const QString& metricALabel,
                                          const QString& metricBKey,
                                          const QString& metricBLabel) {
    m_chartRecords = records;
    m_chartMetricAKey = metricAKey;
    m_chartMetricALabel = metricALabel;
    m_chartMetricBKey = metricBKey;
    m_chartMetricBLabel = metricBLabel;
    m_chartMetricCKey.clear();
    m_chartMetricCLabel.clear();
    rebuildLineChart();
}

void RecordVizPage::ingestRecordScatter3D(const QList<SimulationRecordSnapshot>& records,
                                          const QString& metricAKey,
                                          const QString& metricALabel,
                                          const QString& metricBKey,
                                          const QString& metricBLabel,
                                          const QString& metricCKey,
                                          const QString& metricCLabel) {
    m_chartRecords = records;
    m_chartMetricAKey = metricAKey;
    m_chartMetricALabel = metricALabel;
    m_chartMetricBKey = metricBKey;
    m_chartMetricBLabel = metricBLabel;
    m_chartMetricCKey = metricCKey;
    m_chartMetricCLabel = metricCLabel;
    rebuildScatter3D();
}

void RecordVizPage::rebuildLineChart() {
    clearBody();
    m_vizKind = VizKind::Line;

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
        updateExportButtonsEnabled();
        return;
    }

    setStatus(tr("折线图：%1 条记录，X[%2]，Y[%3]")
                  .arg(points.size())
                  .arg(m_chartMetricALabel, m_chartMetricBLabel),
              false);

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
    m_bodyLayout->addWidget(chartCard);
    m_chartExportHost = chartCard;

    auto* dataCard = new QFrame(m_scrollInner);
    dataCard->setStyleSheet(
        QStringLiteral(
            "QFrame { background-color: %1; border-radius: 16px; border: 1px solid %2; }")
            .arg(BookSimResultUi::rgba(BookSimResultUi::mix(th.cardBg, th.pageBg, 0.86)),
                 BookSimResultUi::rgba(th.border)));
    auto* dataCardLayout = new QVBoxLayout(dataCard);
    dataCardLayout->setContentsMargins(14, 14, 14, 14);
    dataCardLayout->setSpacing(8);
    auto* chartCopy = new QPlainTextEdit(dataCard);
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
    dataCardLayout->addWidget(chartCopy);
    m_bodyLayout->addWidget(dataCard);

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
    updateExportButtonsEnabled();
}

void RecordVizPage::rebuildScatter3D() {
    clearBody();
    m_vizKind = VizKind::Scatter3D;

    const BookSimResultUi::Theme th = BookSimResultUi::themeFrom(eTheme->getThemeMode());
    const QVector<RecordMetricPoint3D> points = buildRecordMetricPoints3D(m_chartRecords,
                                                                          m_chartMetricAKey,
                                                                          m_chartMetricBKey,
                                                                          m_chartMetricCKey);
    if (points.size() < 2) {
        setStatus(tr("三维图创建失败：有效点不足 2 个"), true);
        auto* err = new QLabel(tr("请检查记录和三个指标是否均为数值。"), m_scrollInner);
        err->setWordWrap(true);
        err->setStyleSheet(QStringLiteral("font-size: %1px; color: %2;")
                               .arg(BookSimResultUi::TypePx::kLead)
                               .arg(th.fgMain.name(QColor::HexRgb)));
        applySelectableLabel(err);
        m_bodyLayout->addWidget(err);
        m_bodyLayout->addStretch();
        updateExportButtonsEnabled();
        return;
    }

    setStatus(tr("三维散点图：%1 条记录，X[%2]，Y[%3]，Z[%4]")
                  .arg(points.size())
                  .arg(m_chartMetricALabel, m_chartMetricBLabel, m_chartMetricCLabel),
              false);

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
                                  "<span style=\"font-weight:600;color:%4;\">Y: %5</span>"
                                  "<span style=\"color:%3;\">  |  </span>"
                                  "<span style=\"font-weight:600;color:%6;\">Z: %7</span>")
                                  .arg(th.chartPacket.name(QColor::HexRgb),
                                       m_chartMetricALabel,
                                       th.fgDim.name(QColor::HexRgb),
                                       th.chartNetwork.name(QColor::HexRgb),
                                       m_chartMetricBLabel,
                                       th.chartFlit.name(QColor::HexRgb),
                                       m_chartMetricCLabel),
                              chartCard);
    legend->setTextFormat(Qt::RichText);
    legend->setStyleSheet(QStringLiteral("font-size: %1px;").arg(BookSimResultUi::TypePx::kLead));
    applySelectableLabel(legend);
    chartCardLayout->addWidget(legend);
    chartCardLayout->addWidget(new TripleMetricScatter3DWidget(points,
                                                               m_chartMetricALabel,
                                                               m_chartMetricBLabel,
                                                               m_chartMetricCLabel,
                                                               th,
                                                               chartCard),
                               1);
    m_bodyLayout->addWidget(chartCard);
    m_chartExportHost = chartCard;

    auto* dataCard = new QFrame(m_scrollInner);
    dataCard->setStyleSheet(
        QStringLiteral(
            "QFrame { background-color: %1; border-radius: 16px; border: 1px solid %2; }")
            .arg(BookSimResultUi::rgba(BookSimResultUi::mix(th.cardBg, th.pageBg, 0.86)),
                 BookSimResultUi::rgba(th.border)));
    auto* dataCardLayout = new QVBoxLayout(dataCard);
    dataCardLayout->setContentsMargins(14, 14, 14, 14);
    dataCardLayout->setSpacing(8);
    auto* chartCopy = new QPlainTextEdit(dataCard);
    chartCopy->setReadOnly(true);
    chartCopy->setPlainText(buildRecordScatter3DPlainDump(points,
                                                          m_chartMetricALabel,
                                                          m_chartMetricBLabel,
                                                          m_chartMetricCLabel));
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
    dataCardLayout->addWidget(chartCopy);
    m_bodyLayout->addWidget(dataCard);

    auto* table = new QTableWidget(static_cast<int>(points.size()), 4, m_scrollInner);
    table->setHorizontalHeaderLabels(
        {tr("记录"), m_chartMetricALabel, m_chartMetricBLabel, m_chartMetricCLabel});
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
    table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    for (int i = 0; i < points.size(); ++i) {
        table->setItem(i,
                       0,
                       new QTableWidgetItem(
                           QStringLiteral("%1  %2").arg(points[i].label, points[i].fullName)));
        table->setItem(i, 1, new QTableWidgetItem(QString::number(points[i].metricA, 'f', 6)));
        table->setItem(i, 2, new QTableWidgetItem(QString::number(points[i].metricB, 'f', 6)));
        table->setItem(i, 3, new QTableWidgetItem(QString::number(points[i].metricC, 'f', 6)));
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
    updateExportButtonsEnabled();
}
