#pragma once

#include "BasePage.h"
#include <QVector>

class ElaLineEdit;
class QLabel;
class QVBoxLayout;
class QWidget;
class QToolButton;
class QTextBrowser;

class UsageGuidePage : public BasePage {
    Q_OBJECT
public:
    explicit UsageGuidePage(QWidget* parent = nullptr);

private:
    struct ThirdSectionUi {
        QString title;
        QString searchableText;
        QWidget* card = nullptr;
        QWidget* body = nullptr;
        QToolButton* toggle = nullptr;
        QTextBrowser* browser = nullptr;
    };

    struct SubSectionUi {
        QString title;
        QString searchableText;
        QWidget* card = nullptr;
        QWidget* body = nullptr;
        QToolButton* toggle = nullptr;
        QTextBrowser* browser = nullptr;
        QVector<ThirdSectionUi> children;
    };

    struct SectionUi {
        QString category;
        QString title;
        QString searchableText;
        QWidget* card = nullptr;
        QWidget* body = nullptr;
        QToolButton* toggle = nullptr;
        QLabel* categoryLabel = nullptr;
        QLabel* summaryLabel = nullptr;
        QTextBrowser* browser = nullptr;
        QVector<SubSectionUi> children;
    };

    SectionUi* addSection(const QString& title,
                          const QString& category,
                          const QString& summary,
                          const QString& richText,
                          const QString& searchableText,
                          const QVector<QPair<QString, QString>>& templates = {});
    SubSectionUi* addSubSection(SectionUi& parent,
                                const QString& title,
                                const QString& summary,
                                const QString& richText,
                                const QString& searchableText);
    void addThirdSection(SubSectionUi& parent,
                         const QString& title,
                         const QString& summary,
                         const QString& richText,
                         const QString& searchableText);
    void applySearchFilter();
    void applyTheme();
    QWidget* createTemplateRow(const QVector<QPair<QString, QString>>& templates, QWidget* parent);

    ElaLineEdit* m_searchEdit = nullptr;
    QLabel* m_statusLabel = nullptr;
    QWidget* m_sectionHost = nullptr;
    QVBoxLayout* m_sectionLayout = nullptr;
    QVector<SectionUi> m_sections;
};
