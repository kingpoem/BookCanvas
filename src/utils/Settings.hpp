#pragma once

#include "utils/BooksimPaths.h"
#include <QSettings>
#include <QSize>
#include <cstdlib>
#include <filesystem>

namespace SettingsPaths {

inline std::filesystem::path configDir() {
#ifdef Q_OS_WINDOWS
    return std::filesystem::path(std::getenv("APPDATA")) / "Local" / "Book-Canvas";
#else
    return std::filesystem::path(std::getenv("HOME")) / ".config" / "Book-Canvas";
#endif
}

inline std::filesystem::path legacyConfigDir() {
#ifdef Q_OS_WINDOWS
    return std::filesystem::path(std::getenv("APPDATA")) / "Local" / "NJUPT" / "Book-Canvas";
#else
    return std::filesystem::path(std::getenv("HOME")) / ".config" / "NJUPT" / "Book-Canvas";
#endif
}

} // namespace SettingsPaths

inline QSettings settings = [] {
    const std::filesystem::path configDir = SettingsPaths::configDir();
    if (!std::filesystem::exists(configDir)) {
        std::filesystem::create_directories(configDir);
    }

    return QSettings(QString::fromStdString(configDir.string() + "/settings.ini"),
                     QSettings::IniFormat);
}();

inline void initSettings() {
    const std::filesystem::path legacyDir = SettingsPaths::legacyConfigDir();
    std::error_code ec;
    if (std::filesystem::exists(legacyDir, ec)) {
        std::filesystem::remove(legacyDir / "settings.ini", ec);
        std::filesystem::remove(legacyDir / "simulation_records.json", ec);
        std::filesystem::remove(legacyDir, ec);
    }

    if (!settings.contains("windowSize")) {
        settings.setValue("windowSize", QSize(1200, 740));
    }
    if (!settings.contains("theme")) {
        settings.setValue("theme", "light");
    }
    if (!settings.contains("micaEffect")) {
        settings.setValue("micaEffect", false);
    }
    BooksimPaths::ensureDefaultEnginePathSettings();
    BooksimPaths::ensureDefaultExportPathSettings();
}
