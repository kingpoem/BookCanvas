#pragma once

#include <QString>

/// 仿真结果页与使用说明「结果参数含义」共用的指标显示名：中文（English），保持两边一致。
namespace BookSimMetricLabels {

inline QString sectionLatencyFlit() {
    return QStringLiteral("延迟与微片（Latency & Flits）");
}
inline QString sectionThroughput() {
    return QStringLiteral("吞吐与接纳（Throughput & Acceptance）");
}
inline QString sectionShapePath() {
    return QStringLiteral("包形态与路径（Packet Shape & Path）");
}

inline QString throughputMatch() {
    return QStringLiteral("吞吐匹配度（Throughput Match）");
}
inline QString packetLatency() {
    return QStringLiteral("端到端包延迟（Packet Latency）");
}
inline QString networkLatency() {
    return QStringLiteral("网内延迟（Network Latency）");
}
inline QString flitLatency() {
    return QStringLiteral("微片延迟（Flit Latency）");
}
inline QString fragmentation() {
    return QStringLiteral("微片打散（Fragmentation）");
}
inline QString injectedPacketRate() {
    return QStringLiteral("注入包速率（Injected Packet Rate）");
}
inline QString acceptedPacketRate() {
    return QStringLiteral("接纳包速率（Accepted Packet Rate）");
}
inline QString injectedFlitRate() {
    return QStringLiteral("注入微片速率（Injected Flit Rate）");
}
inline QString acceptedFlitRate() {
    return QStringLiteral("接纳微片速率（Accepted Flit Rate）");
}
inline QString injectedMeanPacketSize() {
    return QStringLiteral("注入平均包长（Injected Mean Packet Size）");
}
inline QString acceptedMeanPacketSize() {
    return QStringLiteral("接纳平均包长（Accepted Mean Packet Size）");
}
inline QString meanHops() {
    return QStringLiteral("平均跳数（Mean Hops）");
}

} // namespace BookSimMetricLabels
