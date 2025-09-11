#include "Tools.h"
#include <QDebug.h>

void printObjectTree(QObject* obj, const QString& prefix, bool isLast) { // NOLINT(misc-no-recursion)
    QString branch = prefix + (isLast ? "└── " : "├── ");

    // 打印对象名（空则显示 None）和 qt 类名
    QString name = obj->objectName().isEmpty() ? "None" : obj->objectName();
    qDebug().noquote() << branch
                       << "\033[34m" << name << "\033[0m"
                       << " ("
                       << "\033[32m" << obj->metaObject()->className() << "\033[0m"
                       << ")";

    const auto& children = obj->children();
    for (int i = 0; i < children.size(); ++i) {
        bool lastChild = (i == children.size() - 1);
        QString newPrefix = prefix + (isLast ? "    " : "│   ");
        printObjectTree(children[i], newPrefix, lastChild);
    }
}
