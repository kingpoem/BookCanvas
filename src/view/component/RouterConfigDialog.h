#pragma once
#include <QDialog>
#include <QMap>
#include <QString>

class ElaLineEdit;
class ElaPushButton;
class QVBoxLayout;
class QScrollArea;

class RouterConfigDialog : public QDialog {
    Q_OBJECT
public:
    explicit RouterConfigDialog(const QString& routerId, QWidget* parent = nullptr);
    void setConfig(const QMap<QString, QString>& config);
    [[nodiscard]] QMap<QString, QString> getConfig() const;
    static QMap<QString, QString> getDefaultConfig();

private slots:
    void onSaveClicked();
    void onCancelClicked();

private:
    void setupUI();
    void addConfigItem(const QString& key, const QString& label, const QString& defaultValue);
    void addSectionTitle(const QString& title);

    QString m_routerId;
    QVBoxLayout* m_mainLayout;
    QScrollArea* m_scrollArea;
    QMap<QString, ElaLineEdit*> m_edits;
    ElaPushButton* m_saveBtn;
    ElaPushButton* m_cancelBtn;
};
