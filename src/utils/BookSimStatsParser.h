#pragma once

#include <QString>
#include <optional>
#include <vector>

struct BookSimStatBand {
    double average{0.};
    double minimum{0.};
    double maximum{0.};
    int samples{0};
    bool hasAverage{false};
    bool hasMinimum{false};
    bool hasMaximum{false};
};

struct BookSimTrafficClassStats {
    int classId{-1};
    BookSimStatBand packetLatency;
    BookSimStatBand networkLatency;
    BookSimStatBand flitLatency;
    BookSimStatBand fragmentation;
    BookSimStatBand injectedPacketRate;
    BookSimStatBand acceptedPacketRate;
    BookSimStatBand injectedFlitRate;
    BookSimStatBand acceptedFlitRate;
    BookSimStatBand injectedPacketSize;
    BookSimStatBand acceptedPacketSize;
    BookSimStatBand hops;
};

struct BookSimParseResult {
    std::vector<BookSimTrafficClassStats> classes;
    std::optional<double> totalRunTimeSec;
    QString errorMessage;

    [[nodiscard]] bool ok() const { return errorMessage.isEmpty() && !classes.empty(); }
};

namespace BookSimStatsParser {

/// 从仿真完整控制台输出中解析 `====== Overall Traffic Statistics ======` 汇总段及末尾 `Total run time`。
[[nodiscard]] BookSimParseResult parseOverallFromLog(QStringView log);

} // namespace BookSimStatsParser
