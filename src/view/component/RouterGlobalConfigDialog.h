#pragma once
#include <QDialog>
#include <QMap>
#include <QString>

class ElaLineEdit;
class ElaPushButton;
class QVBoxLayout;
class QScrollArea;

// 全局配置对话框，用于编辑booksim2全局参数
class RouterGlobalConfigDialog : public QDialog {
    Q_OBJECT
public:
    explicit RouterGlobalConfigDialog(QWidget* parent = nullptr);

    // 设置配置参数
    void setConfig(const QMap<QString, QString>& config);

    // 获取配置参数
    [[nodiscard]] QMap<QString, QString> getConfig() const;

private slots:
    void onSaveClicked();
    void onCancelClicked();

private:
    void setupUI();
    void addConfigItem(const QString& key, const QString& label, const QString& defaultValue);

    QVBoxLayout* m_mainLayout;
    QScrollArea* m_scrollArea;
    QMap<QString, ElaLineEdit*> m_edits;
    ElaPushButton* m_saveBtn;
    ElaPushButton* m_cancelBtn;
};

