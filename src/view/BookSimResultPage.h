#pragma once

#include "BasePage.h"
#include "utils/BookSimStatsParser.h"
#include <QString>
#include <QVector>

class QFrame;
class ElaText;
class QScrollArea;
class QVBoxLayout;

struct BookSimMetricRow {
    QString name;
    /// splitAvgAndRange 为真时表示「平均值」列；为假时表示整格数值（忽略 range）
    QString avgOrValue;
    /// splitAvgAndRange 为真时表示区间列（空则显示「—」）；为假时忽略
    QString range;
    QString hint;
};

class BookSimResultPage : public BasePage {
    Q_OBJECT
public:
    explicit BookSimResultPage(QWidget* parent = nullptr);

public slots:
    void ingestSimulationLog(const QString& text);

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
                               bool primaryEmphasis = false,
                               bool splitAvgAndRange = false);

    ElaText* m_statusText{};
    QScrollArea* m_scroll{};
    QWidget* m_scrollInner{};
    QVBoxLayout* m_bodyLayout{};
    QWidget* m_pageRoot{};
    /// 用于切换亮/暗主题后按同一份日志重绘样式
    QString m_lastSimulationLog;
    /// 记住状态是否为错误态，便于主题切换时刷新文字颜色
    bool m_statusIsError = false;
};
