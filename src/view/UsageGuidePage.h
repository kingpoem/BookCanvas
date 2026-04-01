#pragma once

#include "BasePage.h"
#include <QVector>

class ElaLineEdit;
class QLabel;
class QVBoxLayout;
class QWidget;
class QToolButton;

class UsageGuidePage : public BasePage {
    Q_OBJECT
public:
    explicit UsageGuidePage(QWidget* parent = nullptr);

private:
    struct SectionUi {
        QString category;
        QString title;
        QString searchableText;
        QWidget* card = nullptr;
        QWidget* body = nullptr;
        QToolButton* toggle = nullptr;
    };

    void addSection(const QString& title,
                    const QString& category,
                    const QString& summary,
                    const QString& richText,
                    const QString& searchableText,
                    const QVector<QPair<QString, QString>>& templates = {});
    void applySearchFilter();
    QWidget* createTemplateRow(const QVector<QPair<QString, QString>>& templates, QWidget* parent);

    ElaLineEdit* m_searchEdit = nullptr;
    QLabel* m_statusLabel = nullptr;
    QWidget* m_sectionHost = nullptr;
    QVBoxLayout* m_sectionLayout = nullptr;
    QVector<SectionUi> m_sections;
};
