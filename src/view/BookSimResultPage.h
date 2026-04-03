#pragma once

#include "BasePage.h"
#include "utils/BookSimStatsParser.h"
#include <QVector>

class ElaPushButton;
class QFrame;
class ElaText;
class QScrollArea;
class QVBoxLayout;

struct BookSimMetricRow {
    QString name;
    QString value;
    QString hint;
};

class BookSimResultPage : public BasePage {
    Q_OBJECT
public:
    explicit BookSimResultPage(QWidget* parent = nullptr);

public slots:
    void ingestSimulationLog(const QString& text);
    void loadFromClipboard();

private:
    void rebuildContent(const BookSimParseResult& result);
    void setStatus(const QString& message, bool isError);
    void applyResultPageChrome();
    QWidget* buildClassPanel(QWidget* host, const BookSimTrafficClassStats& stats);
    QFrame* createCategoryCard(QWidget* host,
                               const QString& title,
                               const QString& subtitle,
                               const QVector<BookSimMetricRow>& rows,
                               QWidget* extraWidget = nullptr,
                               const QString& accentColor = QString(),
                               bool primaryEmphasis = false);

    ElaText* m_statusText{};
    ElaPushButton* m_pasteButton{};
    QScrollArea* m_scroll{};
    QWidget* m_scrollInner{};
    QVBoxLayout* m_bodyLayout{};
    QWidget* m_pageRoot{};
    /// 用于切换亮/暗主题后按同一份日志重绘样式
    QString m_lastSimulationLog;
    /// 记住状态是否为错误态，便于主题切换时刷新文字颜色
    bool m_statusIsError = false;
};
