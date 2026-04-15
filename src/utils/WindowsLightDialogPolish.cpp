#include "WindowsLightDialogPolish.h"
#include <ElaTheme.h>
#include <QAbstractSpinBox>
#include <QEvent>
#include <QGuiApplication>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPalette>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTimer>
#include <QWidget>
#include <string_view>

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

[[nodiscard]] bool isElaThemedWidget(const QWidget* w) {
    if (!w) {
        return false;
    }
    const char* const cn = w->metaObject()->className();
    return std::string_view(cn).starts_with("Ela");
}

void polishWindowsLightDialogWidgets(QWidget* root, bool light) {
    if (!root) {
        return;
    }
    const QList<QWidget*> widgets = root->findChildren<QWidget*>(QString(),
                                                                 Qt::FindChildrenRecursively);
    for (QWidget* cw : widgets) {
        if (!cw || cw == root || isElaThemedWidget(cw)) {
            continue;
        }
        const bool isLabel = qobject_cast<QLabel*>(cw) != nullptr;
        const bool isSpin = qobject_cast<QAbstractSpinBox*>(cw) != nullptr;
        const bool isList = qobject_cast<QListWidget*>(cw) != nullptr;
        const bool isPlain = qobject_cast<QPlainTextEdit*>(cw) != nullptr;
        const bool isLineEdit = qobject_cast<QLineEdit*>(cw) != nullptr;
        const bool isBtn = qobject_cast<QPushButton*>(cw) != nullptr;
        if (!(isLabel || isSpin || isList || isPlain || isLineEdit || isBtn)) {
            continue;
        }
        if (light) {
            QPalette p = cw->palette();
            const QColor white(255, 255, 255);
            const QColor black(0, 0, 0);
            const QColor grayAlt(245, 245, 245);
            const QColor grayBtn(243, 243, 243);
            p.setColor(QPalette::All, QPalette::Window, white);
            p.setColor(QPalette::All, QPalette::Base, white);
            p.setColor(QPalette::All, QPalette::AlternateBase, grayAlt);
            p.setColor(QPalette::All, QPalette::Text, black);
            p.setColor(QPalette::All, QPalette::WindowText, black);
            p.setColor(QPalette::All, QPalette::Button, grayBtn);
            p.setColor(QPalette::All, QPalette::ButtonText, black);
            p.setColor(QPalette::All, QPalette::Highlight, QColor(0, 103, 192));
            p.setColor(QPalette::All, QPalette::HighlightedText, white);
            cw->setPalette(p);
            cw->setAutoFillBackground(true);
            if (isSpin) {
                cw->setStyleSheet(
                    QStringLiteral("QAbstractSpinBox { background-color: #ffffff; color: #000000; "
                                   "selection-background-color: #cce4ff; "
                                   "selection-color: #000000; }"));
            } else if (isList) {
                cw->setStyleSheet(QStringLiteral(
                    "QListWidget { background-color: #ffffff; color: #000000; }"
                    "QListWidget::item { color: #000000; background-color: #ffffff; }"
                    "QListWidget::item:selected { background-color: #e5f3ff; color: #000000; }"
                    "QListWidget::item:hover { background-color: #f0f0f0; }"));
            } else if (isLabel) {
                cw->setStyleSheet(
                    QStringLiteral("QLabel { color: #000000; background-color: transparent; }"));
            } else if (isPlain || isLineEdit) {
                cw->setStyleSheet(
                    QStringLiteral("QPlainTextEdit, QLineEdit { background-color: #ffffff; "
                                   "color: #000000; }"));
            } else if (isBtn) {
                cw->setStyleSheet(
                    QStringLiteral("QPushButton { background-color: #f3f3f3; color: #000000; "
                                   "border: 1px solid #cccccc; padding: 4px 14px; }"
                                   "QPushButton:default { border: 1px solid #0067c0; }"));
            }
        } else {
            cw->setPalette(QPalette());
            cw->setStyleSheet(QString());
            cw->setAutoFillBackground(false);
        }
    }
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
            QTimer::singleShot(0, this, [this]() {
                refreshFrame();
                if (m_host && eTheme->getThemeMode() == ElaThemeType::Light) {
                    polishWindowsLightDialogWidgets(m_host, true);
                }
            });
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
            polishWindowsLightDialogWidgets(m_host, true);
            QTimer::singleShot(0, this, [this]() {
                if (m_host && eTheme->getThemeMode() == ElaThemeType::Light) {
                    polishWindowsLightDialogWidgets(m_host, true);
                }
            });
        } else {
            inheritPaletteFromContext(m_host);
            polishWindowsLightDialogWidgets(m_host, false);
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
