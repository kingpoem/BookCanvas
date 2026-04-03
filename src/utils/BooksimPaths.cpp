#include "BooksimPaths.h"
#include "utils/Settings.hpp"
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>

namespace BooksimPaths {

#ifdef Q_OS_WIN
QString findInWindowsAncestors(const QString& appDir, const QString& relativePath) {
    QDir dir(appDir);
    for (int i = 0; i < 10; ++i) {
        const QString candidate = dir.absoluteFilePath(relativePath);
        if (QFileInfo::exists(candidate)) {
            return QFileInfo(candidate).absoluteFilePath();
        }
        if (!dir.cdUp()) {
            break;
        }
    }
    return {};
}
#endif

QString findBooksimExecutable() {
    const QString appDir = QCoreApplication::applicationDirPath();

    const QString path = QDir(appDir).filePath(QStringLiteral("booksim"));
    if (QFileInfo::exists(path)) {
        return QDir(appDir).absoluteFilePath(QStringLiteral("booksim"));
    }

    const QString nested = QDir(appDir).absoluteFilePath(
        QStringLiteral("../../../../3rdpart/booksim2/src/booksim"));
    if (QFileInfo::exists(nested)) {
        return nested;
    }

#ifdef Q_OS_WIN
    const QString pathExe = QDir(appDir).filePath(QStringLiteral("booksim.exe"));
    if (QFileInfo::exists(pathExe)) {
        return QDir(appDir).absoluteFilePath(QStringLiteral("booksim.exe"));
    }

    const QString scannedExe = findInWindowsAncestors(appDir,
                                                      QStringLiteral(
                                                          "3rdpart/booksim2/src/booksim.exe"));
    if (!scannedExe.isEmpty()) {
        return scannedExe;
    }

    const QString nestedExe = QDir(appDir).absoluteFilePath(
        QStringLiteral("../../../../3rdpart/booksim2/src/booksim.exe"));
    if (QFileInfo::exists(nestedExe)) {
        return nestedExe;
    }
#endif

    return {};
}

QString booksimWorkingDirectory() {
    const QString appDir = QCoreApplication::applicationDirPath();

#ifdef Q_OS_MACOS
    if (appDir.contains(QLatin1String(".app/Contents/MacOS"), Qt::CaseInsensitive)) {
        const QString srcFromBundle = QDir(appDir).absoluteFilePath(
            QStringLiteral("../../../../3rdpart/booksim2/src"));
        if (QDir(srcFromBundle).exists()) {
            return QDir(srcFromBundle).canonicalPath();
        }
    }
#endif

    const QString exe = findBooksimExecutable();
    if (!exe.isEmpty()) {
        return QFileInfo(exe).absolutePath();
    }

    const QString srcRel = QDir(appDir).absoluteFilePath(
        QStringLiteral("../../../../3rdpart/booksim2/src"));
    if (QDir(srcRel).exists()) {
        return QDir(srcRel).canonicalPath();
    }

#ifdef Q_OS_WIN
    const QString scannedSrc = findInWindowsAncestors(appDir,
                                                      QStringLiteral("3rdpart/booksim2/src"));
    if (!scannedSrc.isEmpty() && QDir(scannedSrc).exists()) {
        return QDir(scannedSrc).canonicalPath();
    }
#endif

    if (QDir(appDir).exists(QStringLiteral("booksim"))) {
        return appDir;
    }

    return appDir;
}

QString defaultTopologyExportPath() {
    return QDir(booksimWorkingDirectory()).filePath(QStringLiteral("anynet_file"));
}

QString defaultConfigExportPath() {
    return QDir(booksimWorkingDirectory()).filePath(QStringLiteral("anynet_config.json"));
}

void ensureDefaultExportPathSettings() {
    if (!settings.contains(QStringLiteral("booksimTopologyExportPath"))) {
        settings.setValue(QStringLiteral("booksimTopologyExportPath"), defaultTopologyExportPath());
    }
    if (!settings.contains(QStringLiteral("booksimConfigExportPath"))) {
        settings.setValue(QStringLiteral("booksimConfigExportPath"), defaultConfigExportPath());
    }
}

QString topologyExportPathFromSettings() {
    return settings.value(QStringLiteral("booksimTopologyExportPath")).toString().trimmed();
}

QString configExportPathFromSettings() {
    return settings.value(QStringLiteral("booksimConfigExportPath")).toString().trimmed();
}

QString scopedExportPath(const QString& basePath, const QString& scopeToken) {
    const QString path = basePath.trimmed();
    if (path.isEmpty()) {
        return {};
    }
    QString token = scopeToken.trimmed();
    token.replace(QRegularExpression(QStringLiteral("[^A-Za-z0-9_-]+")), QStringLiteral("_"));
    while (token.contains(QStringLiteral("__"))) {
        token.replace(QStringLiteral("__"), QStringLiteral("_"));
    }
    token = token.trimmed();
    if (token.isEmpty()) {
        return path;
    }

    const QFileInfo fi(path);
    const QString dir = fi.absolutePath();
    const QString fileName = fi.fileName();
    const qsizetype dot = fileName.lastIndexOf(QLatin1Char('.'));
    QString name = fileName;
    QString ext;
    if (dot > 0) {
        name = fileName.left(dot);
        ext = fileName.mid(dot);
    }
    return QDir(dir).filePath(QStringLiteral("%1__%2%3").arg(name, token, ext));
}

QString networkFileFieldForJson(const QString& topologyFilePath, const QString& configFilePath) {
    if (topologyFilePath.isEmpty()) {
        return {};
    }
    const QFileInfo topoFi(topologyFilePath);
    const QFileInfo cfgFi(configFilePath);
    if (!topoFi.isAbsolute()) {
        return topologyFilePath;
    }
    const QString topoDir = topoFi.absolutePath();
    const QString cfgDir = cfgFi.absolutePath();
    if (!cfgDir.isEmpty() && topoDir == cfgDir) {
        return topoFi.fileName();
    }
    return topoFi.absoluteFilePath();
}

} // namespace BooksimPaths
