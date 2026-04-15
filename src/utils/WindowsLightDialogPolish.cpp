#include "WindowsLightDialogPolish.h"
#include <ElaTheme.h>
#include <QEvent>
#include <QGuiApplication>
#include <QPalette>
#include <QTimer>
#include <QWidget>

#ifdef Q_OS_WIN
#include <dwmapi.h>
#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif
#endif

namespace BookCanvasUi {
namespace {

#ifdef Q_OS_WIN

void applyImmersiveDarkMode(QWidget* w, bool useDarkMode) {
    if (!w) {
        return;
    }
    const WId wid = w->winId();
    if (wid == 0) {
        return;
    }
    BOOL useDark = useDarkMode ? TRUE : FALSE;
    DwmSetWindowAttribute(reinterpret_cast<HWND>(wid),
                          DWMWA_USE_IMMERSIVE_DARK_MODE,
                          &useDark,
                          sizeof(useDark));
}

void applyLightDialogPalette(QWidget* w) {
    if (!w) {
        return;
    }
    QPalette pal = w->palette();
    const QColor white(255, 255, 255);
    const QColor black(0, 0, 0);
    const QColor grayAlt(245, 245, 245);
    const QColor grayBtn(240, 240, 240);
    const QColor grayPh(120, 120, 120);
    pal.setColor(QPalette::All, QPalette::Window, white);
    pal.setColor(QPalette::All, QPalette::WindowText, black);
    pal.setColor(QPalette::All, QPalette::Base, white);
    pal.setColor(QPalette::All, QPalette::AlternateBase, grayAlt);
    pal.setColor(QPalette::All, QPalette::Text, black);
    pal.setColor(QPalette::All, QPalette::Button, grayBtn);
    pal.setColor(QPalette::All, QPalette::ButtonText, black);
    pal.setColor(QPalette::All, QPalette::PlaceholderText, grayPh);
    pal.setColor(QPalette::All, QPalette::Highlight, QColor(0, 103, 192));
    pal.setColor(QPalette::All, QPalette::HighlightedText, white);
    w->setPalette(pal);
    w->setAutoFillBackground(true);
}

void inheritPaletteFromContext(QWidget* w) {
    if (!w) {
        return;
    }
    if (QWidget* p = w->parentWidget()) {
        w->setPalette(p->palette());
    } else {
        w->setPalette(QGuiApplication::palette());
    }
}

class WinLightDialogPolishHelper final : public QObject {
public:
    explicit WinLightDialogPolishHelper(QWidget* host)
        : QObject(host)
        , m_host(host) {
        m_host->installEventFilter(this);
        QObject::connect(eTheme, &ElaTheme::themeModeChanged, this, [this](ElaThemeType::ThemeMode) {
            refresh();
            QTimer::singleShot(0, this, [this]() { refreshFrame(); });
        });
        refresh();
    }

protected:
    bool eventFilter(QObject* watched, QEvent* event) override {
        if (watched == m_host && event->type() == QEvent::Show) {
            refresh();
            QTimer::singleShot(0, this, [this]() { refreshFrame(); });
        }
        return QObject::eventFilter(watched, event);
    }

private:
    void refresh() {
        if (!m_host) {
            return;
        }
        if (eTheme->getThemeMode() == ElaThemeType::Light) {
            applyLightDialogPalette(m_host);
        } else {
            inheritPaletteFromContext(m_host);
        }
    }

    void refreshFrame() {
        if (!m_host) {
            return;
        }
        applyImmersiveDarkMode(m_host, eTheme->getThemeMode() == ElaThemeType::Dark);
    }

    QWidget* m_host{};
};

#endif

} // namespace

void installWindowsLightTopLevelDialogPolish(QWidget* topLevel) {
#ifdef Q_OS_WIN
    if (!topLevel) {
        return;
    }
    new WinLightDialogPolishHelper(topLevel);
#else
    (void) topLevel;
#endif
}

} // namespace BookCanvasUi
