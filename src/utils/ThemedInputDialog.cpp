#include "ThemedInputDialog.h"
#include "utils/SelectableLabel.h"
#include <ElaDef.h>
#include <ElaDialog.h>
#include <ElaLineEdit.h>
#include <ElaPushButton.h>
#include <ElaTheme.h>
#include <QCoreApplication>
#include <QFont>
#include <QHBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QVBoxLayout>

namespace BookCanvasUi {

namespace {

void showThemedAlert(QWidget* parent,
                     const QString& title,
                     const QString& text,
                     const QString& okButtonText) {
    ElaDialog dlg(parent);
    dlg.setWindowTitle(title);
    dlg.setWindowModality(Qt::WindowModal);
    dlg.setWindowButtonFlags(ElaAppBarType::CloseButtonHint);
    dlg.resize(520, 220);
    dlg.setMinimumWidth(420);

    QObject::connect(&dlg, &ElaDialog::closeButtonClicked, &dlg, &QDialog::reject);

    auto* root = new QVBoxLayout(&dlg);
    root->setContentsMargins(20, 28, 20, 16);
    root->setSpacing(12);

    auto* lab = new QLabel(text, &dlg);
    lab->setWordWrap(true);
    applySelectableLabelText(lab);

    auto* btnRow = new QHBoxLayout();
    btnRow->addStretch();
    auto* okBtn = new ElaPushButton(okButtonText, &dlg);
    btnRow->addWidget(okBtn);

    root->addWidget(lab);
    root->addLayout(btnRow);

    QObject::connect(okBtn, &ElaPushButton::clicked, &dlg, &QDialog::accept);
    dlg.exec();
}

} // namespace

void alertInformation(QWidget* parent, const QString& title, const QString& text) {
    showThemedAlert(parent, title, text, QCoreApplication::translate("BookCanvasUi", "确定"));
}

void alertWarning(QWidget* parent, const QString& title, const QString& text) {
    showThemedAlert(parent, title, text, QCoreApplication::translate("BookCanvasUi", "确定"));
}

QString promptLineText(QWidget* parent,
                       const QString& windowTitle,
                       const QString& labelText,
                       const QString& defaultText,
                       bool* ok) {
    if (ok) {
        *ok = false;
    }

    ElaDialog dlg(parent);
    dlg.setWindowTitle(windowTitle);
    dlg.setWindowModality(Qt::WindowModal);
    dlg.setWindowButtonFlags(ElaAppBarType::CloseButtonHint);
    dlg.resize(520, 200);
    dlg.setMinimumWidth(420);

    QObject::connect(&dlg, &ElaDialog::closeButtonClicked, &dlg, &QDialog::reject);

    auto* root = new QVBoxLayout(&dlg);
    root->setContentsMargins(20, 28, 20, 16);
    root->setSpacing(12);

    auto* lab = new QLabel(labelText, &dlg);
    lab->setWordWrap(true);
    applySelectableLabelText(lab);

    auto* edit = new ElaLineEdit(&dlg);
    edit->setText(defaultText);
    edit->setClearButtonEnabled(true);

    auto* btnRow = new QHBoxLayout();
    btnRow->addStretch();
    auto* okBtn = new ElaPushButton(QCoreApplication::translate("BookCanvasUi", "确定"), &dlg);
    auto* cancelBtn = new ElaPushButton(QCoreApplication::translate("BookCanvasUi", "取消"), &dlg);
    btnRow->addWidget(okBtn);
    btnRow->addWidget(cancelBtn);

    root->addWidget(lab);
    root->addWidget(edit);
    root->addLayout(btnRow);

    QObject::connect(okBtn, &ElaPushButton::clicked, &dlg, &QDialog::accept);
    QObject::connect(cancelBtn, &ElaPushButton::clicked, &dlg, &QDialog::reject);

    const int code = dlg.exec();
    if (ok) {
        *ok = (code == QDialog::Accepted);
    }
    if (code != QDialog::Accepted) {
        return {};
    }
    return edit->text();
}

void showReadOnlyTextPreview(QWidget* parent, const QString& windowTitle, const QString& bodyText) {
    ElaDialog dlg(parent);
    dlg.setWindowTitle(windowTitle);
    dlg.setWindowModality(Qt::WindowModal);
    dlg.setWindowButtonFlags(ElaAppBarType::CloseButtonHint);
    dlg.resize(920, 640);
    dlg.setMinimumSize(520, 360);

    QObject::connect(&dlg, &ElaDialog::closeButtonClicked, &dlg, &QDialog::reject);

    auto* root = new QVBoxLayout(&dlg);
    root->setContentsMargins(20, 28, 20, 16);
    root->setSpacing(12);

    auto* te = new QPlainTextEdit(&dlg);
    te->setReadOnly(true);
    te->setPlainText(bodyText);
    QFont mono = te->font();
    mono.setFamilies({QStringLiteral("SF Mono"),
                      QStringLiteral("Menlo"),
                      QStringLiteral("Consolas"),
                      QStringLiteral("monospace")});
    mono.setPointSize(11);
    te->setFont(mono);

    const auto mode = eTheme->getThemeMode();
    const QColor border = ElaThemeColor(mode, BasicBorder);
    const QColor textMain = ElaThemeColor(mode, BasicText);
    const QColor editorBg = ElaThemeColor(mode, PopupBase);
    te->setStyleSheet(
        QStringLiteral("QPlainTextEdit { background-color: %1; color: %2; border: 1px solid %3; "
                       "border-radius: 8px; }")
            .arg(editorBg.name(QColor::HexRgb),
                 textMain.name(QColor::HexRgb),
                 border.name(QColor::HexRgb)));

    auto* btnRow = new QHBoxLayout();
    btnRow->addStretch();
    auto* closeBtn = new ElaPushButton(QCoreApplication::translate("BookCanvasUi", "关闭"), &dlg);
    btnRow->addWidget(closeBtn);

    root->addWidget(te, 1);
    root->addLayout(btnRow);

    QObject::connect(closeBtn, &ElaPushButton::clicked, &dlg, &QDialog::accept);
    dlg.exec();
}

} // namespace BookCanvasUi
