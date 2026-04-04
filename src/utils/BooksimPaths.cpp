#include "BooksimPaths.h"
#include "utils/Settings.hpp"
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>

namespace {

/// 将已存在的路径规范为无「..」的绝对路径（优先解析符号链接；文件或目录均可）。
[[nodiscard]] QString normalizedExistingPath(const QString& path) {
    if (!QFileInfo::exists(path)) {
        return {};
    }
    const QFileInfo fi(path);
    const QString canonical = fi.canonicalFilePath();
    if (!canonical.isEmpty()) {
        return canonical;
    }
    return QDir::cleanPath(fi.absoluteFilePath());
}

} // namespace

namespace BooksimPaths {

#ifdef Q_OS_WIN
QString findInWindowsAncestors(const QString& appDir, const QString& relativePath) {
    QDir dir(appDir);
    for (int i = 0; i < 10; ++i) {
        const QString candidate = dir.absoluteFilePath(relativePath);
        if (const QString p = normalizedExistingPath(candidate); !p.isEmpty()) {
            return p;
        }
        if (!dir.cdUp()) {
            break;
        }
    }
    return {};
}
#endif

namespace {

[[nodiscard]] QString findBooksimExecutableAuto() {
    const QString appDir = QCoreApplication::applicationDirPath();

    if (const QString p = normalizedExistingPath(QDir(appDir).filePath(QStringLiteral("booksim")));
        !p.isEmpty()) {
        return p;
    }

    const QString nested = QDir::cleanPath(
        QDir(appDir).absoluteFilePath(QStringLiteral("../../../../3rdpart/booksim2/src/booksim")));
    if (const QString p = normalizedExistingPath(nested); !p.isEmpty()) {
        return p;
    }

#ifdef Q_OS_WIN
    if (const QString p = normalizedExistingPath(
            QDir(appDir).filePath(QStringLiteral("booksim.exe")));
        !p.isEmpty()) {
        return p;
    }

    const QString scannedExe = findInWindowsAncestors(appDir,
                                                      QStringLiteral(
                                                          "3rdpart/booksim2/src/booksim.exe"));
    if (!scannedExe.isEmpty()) {
        return normalizedExistingPath(scannedExe);
    }

    const QString nestedExe = QDir::cleanPath(QDir(appDir).absoluteFilePath(
        QStringLiteral("../../../../3rdpart/booksim2/src/booksim.exe")));
    if (const QString p = normalizedExistingPath(nestedExe); !p.isEmpty()) {
        return p;
    }
#endif

    return {};
}

} // namespace

QString defaultBooksimEnginePath() {
    return findBooksimExecutableAuto();
}

void ensureDefaultEnginePathSettings() {
    const QString existing = settings.value(QStringLiteral("booksimEnginePath")).toString().trimmed();
    if (!settings.contains(QStringLiteral("booksimEnginePath")) || existing.isEmpty()) {
        settings.setValue(QStringLiteral("booksimEnginePath"), defaultBooksimEnginePath());
        return;
    }
    if (const QString n = normalizedExistingPath(existing);
        !n.isEmpty() && QFileInfo(n).isFile() && n != existing) {
        settings.setValue(QStringLiteral("booksimEnginePath"), n);
    }
}

QString findBooksimExecutable() {
    const QString configured
        = settings.value(QStringLiteral("booksimEnginePath")).toString().trimmed();
    if (!configured.isEmpty()) {
        if (const QString p = normalizedExistingPath(configured); !p.isEmpty()) {
            const QFileInfo fi(p);
            if (fi.isFile()) {
                return p;
            }
        }
    }
    return findBooksimExecutableAuto();
}

QString booksimWorkingDirectory() {
    const QString appDir = QCoreApplication::applicationDirPath();

    const QString configuredEngine
        = settings.value(QStringLiteral("booksimEnginePath")).toString().trimmed();
    if (!configuredEngine.isEmpty()) {
        if (const QString p = normalizedExistingPath(configuredEngine); !p.isEmpty()) {
            const QFileInfo fi(p);
            if (fi.isFile()) {
                return fi.absolutePath();
            }
        }
    }

#ifdef Q_OS_MACOS
    if (appDir.contains(QLatin1String(".app/Contents/MacOS"), Qt::CaseInsensitive)) {
        const QString srcFromBundle = QDir::cleanPath(
            QDir(appDir).absoluteFilePath(QStringLiteral("../../../../3rdpart/booksim2/src")));
        if (QDir(srcFromBundle).exists()) {
            return normalizedExistingPath(srcFromBundle);
        }
    }
#endif

    const QString exe = findBooksimExecutable();
    if (!exe.isEmpty()) {
        return QFileInfo(exe).absolutePath();
    }

    const QString srcRel = QDir::cleanPath(
        QDir(appDir).absoluteFilePath(QStringLiteral("../../../../3rdpart/booksim2/src")));
    if (QDir(srcRel).exists()) {
        if (const QString c = normalizedExistingPath(srcRel); !c.isEmpty()) {
            return c;
        }
    }

#ifdef Q_OS_WIN
    const QString scannedSrc = findInWindowsAncestors(appDir,
                                                      QStringLiteral("3rdpart/booksim2/src"));
    if (!scannedSrc.isEmpty() && QDir(scannedSrc).exists()) {
        if (const QString c = normalizedExistingPath(scannedSrc); !c.isEmpty()) {
            return c;
        }
    }
#endif

    if (QDir(appDir).exists(QStringLiteral("booksim"))) {
        return QDir::cleanPath(appDir);
    }

    return QDir::cleanPath(appDir);
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
