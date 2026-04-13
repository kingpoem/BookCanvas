#include "MainWindow.h"
#include <ElaApplication.h>
#include <ElaTheme.h>
#include <QApplication>
#include <QSize>
#include <utils/Settings.hpp>
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
#include <QGuiApplication>
#include <QStyleHints>
#endif

int main(int argc, char* argv[]) {
    QApplication a(argc, argv);
    QApplication::setWindowIcon(QIcon(":/res/icon/app_icon.svg"));
    initSettings();
    ElaApplication::getInstance()->init();
    const QString savedTheme
        = settings.value("theme", QStringLiteral("light")).toString().trimmed().toLower();
    const ElaThemeType::ThemeMode initialTheme = (savedTheme == QStringLiteral("dark"))
                                                     ? ElaThemeType::Dark
                                                     : ElaThemeType::Light;
    ElaTheme::getInstance()->setThemeMode(initialTheme);
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
    const auto syncQtColorScheme = [](ElaThemeType::ThemeMode themeMode) {
        if (QGuiApplication::styleHints()) {
            QGuiApplication::styleHints()->setColorScheme(
                themeMode == ElaThemeType::Dark ? Qt::ColorScheme::Dark : Qt::ColorScheme::Light);
        }
    };
    syncQtColorScheme(initialTheme);
#else
    const auto syncQtColorScheme = [](ElaThemeType::ThemeMode) {};
#endif
    QObject::connect(ElaTheme::getInstance(),
                     &ElaTheme::themeModeChanged,
                     &a,
                     [syncQtColorScheme](ElaThemeType::ThemeMode themeMode) {
                         syncQtColorScheme(themeMode);
                         settings.setValue(QStringLiteral("theme"),
                                           themeMode == ElaThemeType::Dark
                                               ? QStringLiteral("dark")
                                               : QStringLiteral("light"));
                     });
    MainWindow w;
    w.show();
    return a.exec();
}
