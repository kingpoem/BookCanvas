#pragma once

#include <QString>

/// BookSim2：运行时键为 routing_function + "_" + topology，与 gRoutingFunctionMap 一致。
[[nodiscard]] QStringList routingIdsForTopology(const QString& topologyId);
[[nodiscard]] QString defaultRoutingIdForTopology(const QString& topologyId);
[[nodiscard]] bool isRoutingValidForTopology(const QString& routingId, const QString& topologyId);
[[nodiscard]] QString normalizeRoutingForTopology(const QString& routingFunction,
                                                  const QString& topologyId);
/// 下拉展示用文案（与 BookSim 路由基名对应）。
[[nodiscard]] QString routingUiLabel(const QString& topologyId, const QString& routingId);
