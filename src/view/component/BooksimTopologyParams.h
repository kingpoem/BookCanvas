#pragma once

#include <QString>

struct BooksimTopologyParams {
    QString topologyId;
    QString displayLabel;
    int k = 8;
    int n = 2;
    int c = 1;
    QString routingFunction = QStringLiteral("min");
};
