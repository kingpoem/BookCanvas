#include "BookSimStatsParser.h"
#include <QRegularExpression>

namespace {

enum class ActiveMetric {
    None,
    PacketLatency,
    NetworkLatency,
    FlitLatency,
    Fragmentation,
    InjectedPacketRate,
    AcceptedPacketRate,
    InjectedFlitRate,
    AcceptedFlitRate,
    InjectedPacketSize,
    AcceptedPacketSize,
    Hops,
};

[[nodiscard]] ActiveMetric metricFromName(QStringView name) {
    if (name == u"Packet latency") {
        return ActiveMetric::PacketLatency;
    }
    if (name == u"Network latency") {
        return ActiveMetric::NetworkLatency;
    }
    if (name == u"Flit latency") {
        return ActiveMetric::FlitLatency;
    }
    if (name == u"Fragmentation") {
        return ActiveMetric::Fragmentation;
    }
    if (name == u"Injected packet rate") {
        return ActiveMetric::InjectedPacketRate;
    }
    if (name == u"Accepted packet rate") {
        return ActiveMetric::AcceptedPacketRate;
    }
    if (name == u"Injected flit rate") {
        return ActiveMetric::InjectedFlitRate;
    }
    if (name == u"Accepted flit rate") {
        return ActiveMetric::AcceptedFlitRate;
    }
    if (name == u"Injected packet size") {
        return ActiveMetric::InjectedPacketSize;
    }
    if (name == u"Accepted packet size") {
        return ActiveMetric::AcceptedPacketSize;
    }
    if (name == u"Hops") {
        return ActiveMetric::Hops;
    }
    return ActiveMetric::None;
}

void assignAverage(BookSimTrafficClassStats& s, ActiveMetric m, double v, int samples) {
    auto fill = [v, samples](BookSimStatBand& b) {
        b.average = v;
        b.samples = samples;
        b.hasAverage = true;
    };
    switch (m) {
    case ActiveMetric::PacketLatency:
        fill(s.packetLatency);
        break;
    case ActiveMetric::NetworkLatency:
        fill(s.networkLatency);
        break;
    case ActiveMetric::FlitLatency:
        fill(s.flitLatency);
        break;
    case ActiveMetric::Fragmentation:
        fill(s.fragmentation);
        break;
    case ActiveMetric::InjectedPacketRate:
        fill(s.injectedPacketRate);
        break;
    case ActiveMetric::AcceptedPacketRate:
        fill(s.acceptedPacketRate);
        break;
    case ActiveMetric::InjectedFlitRate:
        fill(s.injectedFlitRate);
        break;
    case ActiveMetric::AcceptedFlitRate:
        fill(s.acceptedFlitRate);
        break;
    case ActiveMetric::InjectedPacketSize:
        fill(s.injectedPacketSize);
        break;
    case ActiveMetric::AcceptedPacketSize:
        fill(s.acceptedPacketSize);
        break;
    case ActiveMetric::Hops:
        fill(s.hops);
        break;
    case ActiveMetric::None:
        break;
    }
}

void assignMin(BookSimTrafficClassStats& s, ActiveMetric m, double v, int samples) {
    auto fill = [v, samples](BookSimStatBand& b) {
        b.minimum = v;
        b.samples = samples;
        b.hasMinimum = true;
    };
    switch (m) {
    case ActiveMetric::PacketLatency:
        fill(s.packetLatency);
        break;
    case ActiveMetric::NetworkLatency:
        fill(s.networkLatency);
        break;
    case ActiveMetric::FlitLatency:
        fill(s.flitLatency);
        break;
    case ActiveMetric::Fragmentation:
        fill(s.fragmentation);
        break;
    case ActiveMetric::InjectedPacketRate:
        fill(s.injectedPacketRate);
        break;
    case ActiveMetric::AcceptedPacketRate:
        fill(s.acceptedPacketRate);
        break;
    case ActiveMetric::InjectedFlitRate:
        fill(s.injectedFlitRate);
        break;
    case ActiveMetric::AcceptedFlitRate:
        fill(s.acceptedFlitRate);
        break;
    case ActiveMetric::InjectedPacketSize:
        fill(s.injectedPacketSize);
        break;
    case ActiveMetric::AcceptedPacketSize:
        fill(s.acceptedPacketSize);
        break;
    case ActiveMetric::Hops:
        fill(s.hops);
        break;
    case ActiveMetric::None:
        break;
    }
}

void assignMax(BookSimTrafficClassStats& s, ActiveMetric m, double v, int samples) {
    auto fill = [v, samples](BookSimStatBand& b) {
        b.maximum = v;
        b.samples = samples;
        b.hasMaximum = true;
    };
    switch (m) {
    case ActiveMetric::PacketLatency:
        fill(s.packetLatency);
        break;
    case ActiveMetric::NetworkLatency:
        fill(s.networkLatency);
        break;
    case ActiveMetric::FlitLatency:
        fill(s.flitLatency);
        break;
    case ActiveMetric::Fragmentation:
        fill(s.fragmentation);
        break;
    case ActiveMetric::InjectedPacketRate:
        fill(s.injectedPacketRate);
        break;
    case ActiveMetric::AcceptedPacketRate:
        fill(s.acceptedPacketRate);
        break;
    case ActiveMetric::InjectedFlitRate:
        fill(s.injectedFlitRate);
        break;
    case ActiveMetric::AcceptedFlitRate:
        fill(s.acceptedFlitRate);
        break;
    case ActiveMetric::InjectedPacketSize:
        fill(s.injectedPacketSize);
        break;
    case ActiveMetric::AcceptedPacketSize:
        fill(s.acceptedPacketSize);
        break;
    case ActiveMetric::Hops:
        fill(s.hops);
        break;
    case ActiveMetric::None:
        break;
    }
}

} // namespace

BookSimParseResult BookSimStatsParser::parseOverallFromLog(QStringView log) {
    BookSimParseResult out;
    const QString full = log.toString();
    const auto marker = QStringLiteral("====== Overall Traffic Statistics ======");
    const int start = full.indexOf(marker);
    if (start < 0) {
        out.errorMessage = QStringLiteral(
            "未找到 Overall Traffic Statistics 汇总段，请确认仿真已完整结束。");
        return out;
    }

    static const QRegularExpression reClass(
        QStringLiteral(R"(^====== Traffic class (\d+) ======\s*$)"));
    static const QRegularExpression reAverage(
        QStringLiteral(R"(^([\w ]+) average = ([\d.+-eE]+) \((\d+) samples\)\s*$)"));
    static const QRegularExpression reMin(
        QStringLiteral(R"(^minimum = ([\d.+-eE]+) \((\d+) samples\)\s*$)"));
    static const QRegularExpression reMax(
        QStringLiteral(R"(^maximum = ([\d.+-eE]+) \((\d+) samples\)\s*$)"));
    static const QRegularExpression reWall(QStringLiteral(R"(^Total run time\s+([\d.+-eE]+)\s*$)"));

    const QStringList lines = full.mid(start).split(u'\n', Qt::SkipEmptyParts);

    BookSimTrafficClassStats* current = nullptr;
    ActiveMetric active = ActiveMetric::None;

    for (const QString& rawLine : lines) {
        const QString line = rawLine.trimmed();
        if (reWall.match(line).hasMatch()) {
            const auto m = reWall.match(line);
            out.totalRunTimeSec = m.capturedView(1).toDouble();
            continue;
        }

        {
            const auto m = reClass.match(line);
            if (m.hasMatch()) {
                out.classes.push_back(BookSimTrafficClassStats{});
                current = &out.classes.back();
                current->classId = m.capturedView(1).toInt();
                active = ActiveMetric::None;
                continue;
            }
        }

        if (!current) {
            continue;
        }

        {
            const auto m = reAverage.match(line);
            if (m.hasMatch()) {
                const QString name = m.captured(1).trimmed();
                active = metricFromName(name);
                const double v = m.capturedView(2).toDouble();
                const int samples = m.capturedView(3).toInt();
                if (active != ActiveMetric::None) {
                    assignAverage(*current, active, v, samples);
                }
                continue;
            }
        }
        {
            const auto m = reMin.match(line);
            if (m.hasMatch() && active != ActiveMetric::None) {
                const double v = m.capturedView(1).toDouble();
                const int samples = m.capturedView(2).toInt();
                assignMin(*current, active, v, samples);
                continue;
            }
        }
        {
            const auto m = reMax.match(line);
            if (m.hasMatch() && active != ActiveMetric::None) {
                const double v = m.capturedView(1).toDouble();
                const int samples = m.capturedView(2).toInt();
                assignMax(*current, active, v, samples);
                continue;
            }
        }
    }

    if (out.classes.empty()) {
        out.errorMessage = QStringLiteral("汇总段中没有可用的 Traffic class 数据。");
    }
    return out;
}
