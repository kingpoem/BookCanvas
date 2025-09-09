#pragma once

#include <QSettings>
#include <QSize>
#include <filesystem>

inline QSettings settings = [] {
    const std::filesystem::path configDir =
#ifdef Q_OS_WINDOWS
        std::filesystem::path(std::getenv("APPDATA")) / "Local" / "NJUPT" / "Book-Canvas";
#else
        std::filesystem::path(std::getenv("HOME")) / ".config" / "NJUPT" / "Book-Canvas";
#endif

    if (!std::filesystem::exists(configDir)) {
        std::filesystem::create_directories(configDir);
    }

    return QSettings(QString::fromStdString(configDir.string() + "/settings.ini"),
                     QSettings::IniFormat);
}();

inline void initSettings() {
    if (!settings.contains("windowSize")) {
        settings.setValue("windowSize", QSize(1200, 740));
    }
    if (!settings.contains("theme")) {
        settings.setValue("theme", "light");
    }
    if (!settings.contains("micaEffect")) {
        settings.setValue("micaEffect", false);
    }
}
