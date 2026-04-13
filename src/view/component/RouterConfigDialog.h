#pragma once
#include <ElaDialog.h>
#include <QMap>
#include <QString>
#include <QStringList>

class QLabel;
class ElaPushButton;
class QScrollArea;
class QVBoxLayout;
class QWidget;

class RouterConfigDialog : public ElaDialog {
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
    void applyDialogChrome();
    void addSectionTitle(const QString& title);
    void bindLabelToTheme(QLabel* label, bool sectionTitle);
    void addFieldRow(const QString& labelText, QWidget* field);

    void addBool01Field(const QString& key, const QString& label, const QString& defaultValue);
    void addIntField(
        const QString& key, const QString& label, int minV, int maxV, const QString& defaultValue);
    /// min=-1（含）至 maxV，用于 BookSim 中表示无限制
    void addIntFieldNeg1(const QString& key,
                         const QString& label,
                         int maxV,
                         const QString& defaultValue);
    void addDoubleField(const QString& key,
                        const QString& label,
                        double minV,
                        double maxV,
                        double step,
                        int decimals,
                        const QString& defaultValue);
    void addAllocCombo(const QString& key,
                       const QString& label,
                       const QString& defaultValue,
                       bool vc);

    static QString widgetToConfigString(const QWidget* w);
    static void setWidgetFromConfigString(QWidget* w, const QString& text);

    QString m_routerId;
    QVBoxLayout* m_mainLayout = nullptr;
    QScrollArea* m_scrollArea = nullptr;
    QWidget* m_scrollContent = nullptr;
    QMap<QString, QWidget*> m_fields;
    ElaPushButton* m_saveBtn = nullptr;
    ElaPushButton* m_cancelBtn = nullptr;
};
