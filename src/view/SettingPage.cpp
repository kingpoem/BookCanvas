#include "SettingPage.h"
#include "utils/BooksimPaths.h"
#include "utils/Settings.hpp"
#include <ElaApplication.h>
#include <ElaComboBox.h>
#include <ElaLineEdit.h>
#include <ElaPushButton.h>
#include <ElaRadioButton.h>
#include <ElaTheme.h>
#include <ElaToggleSwitch.h>
#include <ElaWindow.h>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QVBoxLayout>

SettingPage::SettingPage(QWidget* parent)
    : BasePage(parent) {
    auto* window = dynamic_cast<ElaWindow*>(parent);
    setWindowTitle("Setting");

    auto appearanceText = new ElaText("Appearance", this);
    appearanceText->setWordWrap(false);
    appearanceText->setTextPixelSize(18);

    auto themeComboBox = new ElaComboBox(this);
    themeComboBox->addItem("Light");
    themeComboBox->addItem("Dark");
    auto themeSwitchArea = createScrollPageArea("Themes", themeComboBox);
    connect(themeComboBox,
            QOverload<int>::of(&ElaComboBox::currentIndexChanged),
            this,
            [=](int index) {
                if (index == 0) {
                    eTheme->setThemeMode(ElaThemeType::Light);
                    settings.setValue("theme", "light");
                } else {
                    eTheme->setThemeMode(ElaThemeType::Dark);
                    settings.setValue("theme", "dark");
                }
            });
    connect(eTheme, &ElaTheme::themeModeChanged, this, [=](ElaThemeType::ThemeMode themeMode) {
        themeComboBox->blockSignals(true);
        themeComboBox->setCurrentIndex(themeMode);
        themeComboBox->blockSignals(false);
    });
    themeComboBox->setCurrentIndex(settings.value("theme").toString() == "dark");

    // auto micaSwitchButton = new ElaToggleSwitch(this);
    // auto micaSwitchArea = createScrollPageArea("Mica Effect", micaSwitchButton);
    // connect(micaSwitchButton, &ElaToggleSwitch::toggled, this, [=](bool checked) {
    //     eApp->setIsEnableMica(checked);
    //     settings.setValue("micaEffect", checked);
    // });
    // micaSwitchButton->setIsToggled(settings.value("micaEffect").toBool());

    auto minimumButton = new ElaRadioButton("Minimum", this);
    auto compactButton = new ElaRadioButton("Compact", this);
    auto maximumButton = new ElaRadioButton("Maximum", this);
    auto autoButton = new ElaRadioButton("Auto", this);
    auto displayModeArea = createScrollPageArea("Navigation Bar Display Mode",
                                                minimumButton,
                                                compactButton,
                                                maximumButton,
                                                autoButton);
    connect(minimumButton, &ElaRadioButton::toggled, this, [=](bool checked) {
        if (checked) {
            window->setNavigationBarDisplayMode(ElaNavigationType::Minimal);
            settings.setValue("navigationBarDisplayMode", 1);
        }
    });
    connect(compactButton, &ElaRadioButton::toggled, this, [=](bool checked) {
        if (checked) {
            window->setNavigationBarDisplayMode(ElaNavigationType::Compact);
            settings.setValue("navigationBarDisplayMode", 2);
        }
    });
    connect(maximumButton, &ElaRadioButton::toggled, this, [=](bool checked) {
        if (checked) {
            window->setNavigationBarDisplayMode(ElaNavigationType::Maximal);
            settings.setValue("navigationBarDisplayMode", 3);
        }
    });
    connect(autoButton, &ElaRadioButton::toggled, this, [=](bool checked) {
        if (checked) {
            window->setNavigationBarDisplayMode(ElaNavigationType::Auto);
            settings.setValue("navigationBarDisplayMode", 0);
        }
    });
    if (settings.value("navigationBarDisplayMode").toInt() == 1) {
        minimumButton->setChecked(true);
    } else if (settings.value("navigationBarDisplayMode").toInt() == 2) {
        compactButton->setChecked(true);
    } else if (settings.value("navigationBarDisplayMode").toInt() == 3) {
        maximumButton->setChecked(true);
    } else {
        autoButton->setChecked(true);
    }

    auto* bookSimHeading = new ElaText(tr("BookSim 导出"), this);
    bookSimHeading->setWordWrap(false);
    bookSimHeading->setTextPixelSize(18);

    auto* topoEdit = new ElaLineEdit(this);
    topoEdit->setText(settings.value(QStringLiteral("booksimTopologyExportPath")).toString());
    topoEdit->setClearButtonEnabled(true);
    auto* topoBrowse = new ElaPushButton(tr("浏览…"), this);
    auto* topoBlock = new QWidget(this);
    auto* topoV = new QVBoxLayout(topoBlock);
    topoV->setContentsMargins(0, 0, 0, 0);
    topoV->setSpacing(6);
    auto* topoLabel = new ElaText(tr("拓扑文件（Canvas「导出拓扑」）"), this);
    topoLabel->setTextPixelSize(14);
    topoV->addWidget(topoLabel);
    auto* topoH = new QHBoxLayout();
    topoH->addWidget(topoEdit, 1);
    topoH->addWidget(topoBrowse);
    topoV->addLayout(topoH);

    auto* cfgEdit = new ElaLineEdit(this);
    cfgEdit->setText(settings.value(QStringLiteral("booksimConfigExportPath")).toString());
    cfgEdit->setClearButtonEnabled(true);
    auto* cfgBrowse = new ElaPushButton(tr("浏览…"), this);
    auto* cfgBlock = new QWidget(this);
    auto* cfgV = new QVBoxLayout(cfgBlock);
    cfgV->setContentsMargins(0, 0, 0, 0);
    cfgV->setSpacing(6);
    auto* cfgLabel = new ElaText(tr("JSON 配置（Canvas「导出配置」与仿真）"), this);
    cfgLabel->setTextPixelSize(14);
    cfgV->addWidget(cfgLabel);
    auto* cfgH = new QHBoxLayout();
    cfgH->addWidget(cfgEdit, 1);
    cfgH->addWidget(cfgBrowse);
    cfgV->addLayout(cfgH);

    auto* resetBookSimPaths = new ElaPushButton(tr("恢复为检测到的 BookSim 工作目录中的默认文件名"),
                                                this);

    connect(topoEdit, &ElaLineEdit::editingFinished, this, [topoEdit]() {
        settings.setValue(QStringLiteral("booksimTopologyExportPath"), topoEdit->text().trimmed());
    });
    connect(cfgEdit, &ElaLineEdit::editingFinished, this, [cfgEdit]() {
        settings.setValue(QStringLiteral("booksimConfigExportPath"), cfgEdit->text().trimmed());
    });

    connect(topoBrowse, &ElaPushButton::clicked, this, [this, topoEdit]() {
        const QString path
            = QFileDialog::getSaveFileName(this,
                                           tr("选择拓扑导出路径"),
                                           topoEdit->text().isEmpty()
                                               ? BooksimPaths::defaultTopologyExportPath()
                                               : topoEdit->text(),
                                           tr("All Files (*)"));
        if (!path.isEmpty()) {
            topoEdit->setText(path);
            settings.setValue(QStringLiteral("booksimTopologyExportPath"), path.trimmed());
        }
    });
    connect(cfgBrowse, &ElaPushButton::clicked, this, [this, cfgEdit]() {
        const QString path
            = QFileDialog::getSaveFileName(this,
                                           tr("选择 JSON 配置导出路径"),
                                           cfgEdit->text().isEmpty()
                                               ? BooksimPaths::defaultConfigExportPath()
                                               : cfgEdit->text(),
                                           tr("JSON (*.json);;All Files (*)"));
        if (!path.isEmpty()) {
            cfgEdit->setText(path);
            settings.setValue(QStringLiteral("booksimConfigExportPath"), path.trimmed());
        }
    });
    connect(resetBookSimPaths, &ElaPushButton::clicked, this, [topoEdit, cfgEdit]() {
        const QString td = BooksimPaths::defaultTopologyExportPath();
        const QString cd = BooksimPaths::defaultConfigExportPath();
        topoEdit->setText(td);
        cfgEdit->setText(cd);
        settings.setValue(QStringLiteral("booksimTopologyExportPath"), td);
        settings.setValue(QStringLiteral("booksimConfigExportPath"), cd);
    });

    auto centralWidget = new QWidget(this);
    centralWidget->setWindowTitle("Setting");
    auto centerLayout = new QVBoxLayout(centralWidget);
    centerLayout->addSpacing(30);
    centerLayout->addWidget(appearanceText);
    centerLayout->addSpacing(10);
    centerLayout->addWidget(themeSwitchArea);
    centerLayout->addWidget(displayModeArea);
    centerLayout->addSpacing(24);
    centerLayout->addWidget(bookSimHeading);
    centerLayout->addSpacing(8);
    centerLayout->addWidget(topoBlock);
    centerLayout->addWidget(cfgBlock);
    centerLayout->addWidget(resetBookSimPaths);
    centerLayout->addSpacing(15);
    centerLayout->addStretch();
    centerLayout->setContentsMargins(0, 0, 20, 0);
    addCentralWidget(centralWidget, true, true, 0);
}
