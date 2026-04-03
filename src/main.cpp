#include "MainWindow.h"
#include <ElaApplication.h>
#include <ElaTheme.h>
#include <QApplication>
#include <QSize>
#include <utils/Settings.hpp>

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
    QObject::connect(ElaTheme::getInstance(),
                     &ElaTheme::themeModeChanged,
                     &a,
                     [](ElaThemeType::ThemeMode themeMode) {
                         settings.setValue(QStringLiteral("theme"),
                                           themeMode == ElaThemeType::Dark
                                               ? QStringLiteral("dark")
                                               : QStringLiteral("light"));
                     });
    MainWindow w;
    w.show();
    return a.exec();
}
