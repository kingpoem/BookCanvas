#pragma once

#include "BasePage.h"
#include <QMap>

class ElaLineEdit;
class ElaPushButton;
class QVBoxLayout;
class QScrollArea;

class GlobalConfigPage : public BasePage {
    Q_OBJECT
public:
    explicit GlobalConfigPage(QWidget* parent = nullptr);

    void setConfig(const QMap<QString, QString>& config);
    [[nodiscard]] QMap<QString, QString> config() const { return m_config; }
    [[nodiscard]] QMap<QString, QString> collectCurrentConfig();

signals:
    void globalConfigChanged(const QMap<QString, QString>& config);

private:
    void setupUi();
    void addSectionTitle(const QString& title);
    void addConfigItem(const QString& key, const QString& label, const QString& defaultValue);
    [[nodiscard]] QMap<QString, QString> collectConfigFromUi() const;

    QMap<QString, QString> m_config;
    QVBoxLayout* m_formLayout = nullptr;
    QScrollArea* m_scrollArea = nullptr;
    QMap<QString, ElaLineEdit*> m_edits;
    ElaPushButton* m_resetBtn = nullptr;
};
