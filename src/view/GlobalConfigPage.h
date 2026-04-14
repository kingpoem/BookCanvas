#pragma once

#include "BasePage.h"
#include <QMap>
#include <QVector>

class QWidget;
class ElaLineEdit;
class ElaComboBox;
class ElaPushButton;
class QVBoxLayout;
class QScrollArea;
class QFrame;
class QToolButton;

class GlobalConfigPage : public BasePage {
    Q_OBJECT
public:
    explicit GlobalConfigPage(QWidget* parent = nullptr);

    void setConfig(const QMap<QString, QString>& config);
    [[nodiscard]] QMap<QString, QString> config() const { return m_config; }
    [[nodiscard]] QMap<QString, QString> collectCurrentConfig();

    /// 将拓扑设为 chiplet_mesh，并选择 dim_order_chiplet_mesh（供画布创建芯粒等流程调用）
    void applyChipletMeshTopologyAndRouting();

signals:
    void globalConfigChanged(const QMap<QString, QString>& config);

private:
    struct SectionUi {
        QFrame* card = nullptr;
        QWidget* body = nullptr;
        QToolButton* toggle = nullptr;
        QVBoxLayout* bodyLayout = nullptr;
    };

    void setupUi();
    void applyTheme();
    void addSectionTitle(const QString& title);
    void addConfigItem(const QString& key, const QString& label, const QString& defaultValue);
    void wireTopologyRoutingForGlobalConfig();
    void refreshGlobalRoutingComboFromUiConfig(const QString& preferredRouting = {});
    void refreshTopologyFieldVisibility();
    [[nodiscard]] QString currentTopologyId() const;
    [[nodiscard]] QMap<QString, QString> collectConfigFromUi() const;
    void onViewRawConfigFileClicked();
    void onImportConfigClicked();
    void showRawConfigContent(const QString& title, const QString& content);
    [[nodiscard]] static QMap<QString, QString> parseRawConfigText(const QString& text);

    QMap<QString, QString> m_config;
    QMap<QString, QWidget*> m_inputRows;
    QVBoxLayout* m_formLayout = nullptr;
    QVBoxLayout* m_activeSectionLayout = nullptr;
    QScrollArea* m_scrollArea = nullptr;
    QVector<SectionUi> m_sections;
    QMap<QString, QWidget*> m_inputs;
    ElaPushButton* m_viewRawBtn = nullptr;
    ElaPushButton* m_importBtn = nullptr;
    ElaPushButton* m_resetBtn = nullptr;
    QWidget* m_pageRoot = nullptr;
    QFrame* m_chipletMeshSectionCard = nullptr;
    bool m_topologyRoutingWired = false;
};
