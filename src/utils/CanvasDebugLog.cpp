#include "CanvasDebugLog.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QMutex>
#include <QTextStream>
#include <QThread>

namespace {

QMutex g_canvasLogMutex;

QString resolvedLogPath() {
    static QString path;
    if (!path.isEmpty()) {
        return path;
    }
    QString base = QCoreApplication::applicationDirPath();
    if (base.isEmpty()) {
        base = QDir::tempPath();
    }
    QDir logDir(base + QStringLiteral("/logs"));
    logDir.mkpath(QStringLiteral("."));
    path = logDir.absoluteFilePath(QStringLiteral("bookcanvas_canvas_debug.log"));
    return path;
}

} // namespace

QString canvasDebugLogFilePath() {
    return resolvedLogPath();
}

void canvasDebugLog(const QString& message) {
    QMutexLocker lock(&g_canvasLogMutex);
    QFile f(resolvedLogPath());
    if (!f.open(QIODevice::Append | QIODevice::Text)) {
        return;
    }
    QTextStream out(&f);
    out << QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd hh:mm:ss.zzz"))
        << QStringLiteral(" thread=0x") << QString::number(quintptr(QThread::currentThreadId()), 16)
        << ' ' << message << QChar('\n');
    out.flush();
    f.flush();
}
