#pragma once
#include <QDialog>
#include <QMap>
#include <QString>

class ElaLineEdit;
class ElaPushButton;
class QVBoxLayout;

// 路由器配置对话框，用于编辑anynet_config参数
class RouterConfigDialog : public QDialog {
    Q_OBJECT
public:
    explicit RouterConfigDialog(QWidget* parent = nullptr);

    // 设置配置参数
    void setConfig(const QMap<QString, QString>& config);

    // 获取配置参数
    [[nodiscard]] QMap<QString, QString> getConfig() const;

private slots:
    void onSaveClicked();
    void onCancelClicked();

private:
    void setupUI();
    void initializeDefaultConfig();

    QVBoxLayout* m_mainLayout;
    QMap<QString, ElaLineEdit*> m_edits;
    ElaPushButton* m_saveBtn;
    ElaPushButton* m_cancelBtn;

    // 默认配置
    static QMap<QString, QString> s_defaultConfig;
};
