#pragma once

#include <QObject>
#include <QString>

void printObjectTree(QObject* obj, const QString& prefix = "", bool isLast = true);
