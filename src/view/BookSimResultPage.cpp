#include "BookSimResultPage.h"
#include "utils/BookSimMetricLabels.h"
#include "view/BookSimResultUi.h"
#include <ElaDef.h>
#include <ElaText.h>
#include <ElaTheme.h>
#include <QFont>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPaintEvent>
#include <QPainter>
#include <QPalette>
#include <QPlainTextEdit>
#include <QScrollArea>
#include <QSizePolicy>
#include <QTabWidget>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QWidget>
#include <algorithm>
#include <cmath>
#include <utility>

namespace {

void applySelectableLabel(QLabel* label) {
    if (!label) {
        return;
    }
    label->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
    label->setFocusPolicy(Qt::ClickFocus);
}

void applySelectableLabel(ElaText* text) {
    applySelectableLabel(static_cast<QLabel*>(text));
}

[[nodiscard]] BookSimMetricRow metricRowFromBand(const QString& name,
                                                 const BookSimStatBand& b,
                                                 int precision) {
    BookSimMetricRow row;
    row.name = name;
    if (!b.hasAverage) {
        row.avgOrValue = QStringLiteral("—");
        row.range.clear();
        return row;
    }
    row.avgOrValue = QString::number(b.average, 'f', precision);
    if (b.hasMinimum && b.hasMaximum) {
        row.range = QStringLiteral("[%1, %2]")
                        .arg(b.minimum, 0, 'f', precision)
                        .arg(b.maximum, 0, 'f', precision);
    } else {
        row.range.clear();
    }
    return row;
}

} // namespace

BookSimResultPage::BookSimResultPage(QWidget* parent)
    : BasePage(parent) {
    setWindowTitle(tr("BookSim 结果"));

    auto* central = new QWidget(this);
    m_pageRoot = central;
    central->setWindowTitle(tr("BookSim 结果"));
    central->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    auto* mainLay = new QVBoxLayout(central);
    mainLay->setContentsMargins(8, 0, 8, 0);
    mainLay->setSpacing(10);

    m_statusText = new ElaText(tr("等待仿真结果"), this);
    m_statusText->setTextPixelSize(BookSimResultUi::TypePx::kStatus);
    m_statusText->setWordWrap(true);
    applySelectableLabel(m_statusText);

    m_scrollInner = new QWidget(central);
    m_scrollInner->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    m_bodyLayout = new QVBoxLayout(m_scrollInner);
    m_bodyLayout->setContentsMargins(0, 0, 0, 0);
    m_bodyLayout->setSpacing(16);

    m_scroll = new QScrollArea(central);
    m_scroll->setObjectName(QStringLiteral("BookSimResultScroll"));
    m_scroll->setWidgetResizable(true);
    m_scroll->setWidget(m_scrollInner);
    m_scroll->setFrameShape(QFrame::NoFrame);
    m_scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scroll->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    mainLay->addWidget(m_statusText);
    mainLay->addWidget(m_scroll, 1);

    setStatus(tr("等待仿真结果"), false);

    connect(eTheme, &ElaTheme::themeModeChanged, this, [this](ElaThemeType::ThemeMode) {
        // 主题切换时先刷新状态文案颜色，避免保留上一个主题的黑/白字。
        setStatus(m_statusText->text(), m_statusIsError);
        applyResultPageChrome();
        if (m_lastSimulationLog.isEmpty()) {
            return;
        }
        rebuildContent(BookSimStatsParser::parseOverallFromLog(m_lastSimulationLog));
    });

    addCentralWidget(central, true, true, 0);
    applyResultPageChrome();
}

void BookSimResultPage::ingestSimulationLog(const QString& text) {
    m_lastSimulationLog = text;
    rebuildContent(BookSimStatsParser::parseOverallFromLog(text));
}

void BookSimResultPage::setStatus(const QString& message, bool isError) {
    m_statusIsError = isError;
    m_statusText->setText(message);
    const BookSimResultUi::Theme th = BookSimResultUi::themeFrom(eTheme->getThemeMode());
    if (isError) {
        m_statusText->setStyleSheet(
            QStringLiteral("font-size: %1px; color: %2;")
                .arg(BookSimResultUi::TypePx::kStatus)
                .arg(ElaThemeColor(eTheme->getThemeMode(), StatusDanger).name(QColor::HexRgb)));
    } else {
        m_statusText->setStyleSheet(QStringLiteral("font-size: %1px; color: %2;")
                                        .arg(BookSimResultUi::TypePx::kStatus)
                                        .arg(th.fgMain.name(QColor::HexRgb)));
    }
}

void BookSimResultPage::applyResultPageChrome() {
    if (!m_pageRoot) {
        return;
    }
    const BookSimResultUi::Theme th = BookSimResultUi::themeFrom(eTheme->getThemeMode());
    const QString bgCss = th.pageBg.name(QColor::HexRgb);

    // ElaScrollPage::addCentralWidget 会把 central 改为 objectName=ElaScrollPage_CentralPage 且
    // background 设为 transparent，若不覆盖则外層 ElaScrollArea 的深色会透出（浅色主题下像黑底）。
    m_pageRoot->setAttribute(Qt::WA_StyledBackground, true);
    m_pageRoot->setStyleSheet(
        QStringLiteral("#ElaScrollPage_CentralPage { background-color: %1; }").arg(bgCss));

    if (QWidget* outerVp = m_pageRoot->parentWidget()) {
        outerVp->setAutoFillBackground(true);
        QPalette op = outerVp->palette();
        op.setColor(QPalette::Window, th.pageBg);
        outerVp->setPalette(op);
    }

    m_scroll->setStyleSheet(QStringLiteral("#BookSimResultScroll { background: transparent; "
                                           "border: none; }"));
    if (m_scroll->viewport()) {
        m_scroll->viewport()->setStyleSheet(QStringLiteral("background: transparent;"));
    }
    m_scrollInner->setStyleSheet(QStringLiteral("background: transparent;"));
}

void BookSimResultPage::rebuildContent(const BookSimParseResult& result) {
    while (QLayoutItem* item = m_bodyLayout->takeAt(0)) {
        delete item->widget();
        delete item;
    }

    const BookSimResultUi::Theme th = BookSimResultUi::themeFrom(eTheme->getThemeMode());

    if (!result.ok()) {
        setStatus(result.errorMessage, true);
        auto* err = new QLabel(result.errorMessage, m_scrollInner);
        err->setWordWrap(true);
        err->setStyleSheet(QStringLiteral("font-size: %1px; color: %2;")
                               .arg(BookSimResultUi::TypePx::kLead)
                               .arg(th.fgMain.name(QColor::HexRgb)));
        applySelectableLabel(err);
        m_bodyLayout->addWidget(err);
        return;
    }

    setStatus(tr("已解析：%1 个 class").arg(result.classes.size()), false);

    if (result.totalRunTimeSec.has_value()) {
        auto* wall = new QFrame(m_scrollInner);
        wall->setStyleSheet(
            QStringLiteral("QFrame { background-color: %1; border-radius: 12px; "
                           "border: 1px solid %2; }")
                .arg(BookSimResultUi::rgba(BookSimResultUi::mix(th.cardBg, th.border, 0.75)),
                     BookSimResultUi::rgba(th.border)));
        auto* wh = new QHBoxLayout(wall);
        wh->setContentsMargins(16, 10, 16, 10);
        auto* wlab = new QLabel(tr("总耗时"), wall);
        wlab->setStyleSheet(QStringLiteral("font-size: %1px; color: %2;")
                                .arg(BookSimResultUi::TypePx::kBody)
                                .arg(BookSimResultUi::rgba(th.fgDim)));
        applySelectableLabel(wlab);
        auto* wval = new QLabel(QStringLiteral("%1 s").arg(*result.totalRunTimeSec, 0, 'g', 9),
                                wall);
        wval->setStyleSheet(QStringLiteral("font-size: %1px; font-weight: 600; font-family: 'SF "
                                           "Mono', 'Menlo', 'Consolas', monospace; color: %2;")
                                .arg(BookSimResultUi::TypePx::kLead)
                                .arg(th.fgMain.name(QColor::HexRgb)));
        applySelectableLabel(wval);
        wh->addWidget(wlab);
        wh->addStretch();
        wh->addWidget(wval);
        m_bodyLayout->addWidget(wall);
    }

    if (result.classes.size() == 1) {
        m_bodyLayout->addWidget(buildClassPanel(m_scrollInner, result.classes.front()));
    } else {
        auto* tabs = new QTabWidget(m_scrollInner);
        tabs->setDocumentMode(true);
        tabs->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
        tabs->setStyleSheet(
            QStringLiteral("QTabWidget::pane { border: none; background: transparent; } "
                           "QTabBar::tab { color: %1; padding: 8px 16px; } "
                           "QTabBar::tab:selected { font-weight: 600; }")
                .arg(th.fgMain.name(QColor::HexRgb)));
        for (const auto& c : result.classes) {
            auto* page = new QWidget(tabs);
            auto* v = new QVBoxLayout(page);
            v->setContentsMargins(4, 10, 4, 10);
            v->addWidget(buildClassPanel(page, c));
            tabs->addTab(page, tr("Class %1").arg(c.classId));
        }
        m_bodyLayout->addWidget(tabs);
    }
}

QFrame* BookSimResultPage::createCategoryCard(QWidget* host,
                                              const QString& title,
                                              const QString& subtitle,
                                              const QVector<BookSimMetricRow>& rows,
                                              QWidget* extraWidget,
                                              const QString& accentColor,
                                              bool primaryEmphasis,
                                              bool splitAvgAndRange) {
    const BookSimResultUi::Theme th = BookSimResultUi::themeFrom(eTheme->getThemeMode());
    auto* card = new QFrame(host);
    card->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    const QColor accent = QColor(accentColor);
    const QString borderLeft = accent.isValid() && accent.alpha() > 0
                                   ? QStringLiteral("4px solid %1").arg(accent.name(QColor::HexRgb))
                                   : QStringLiteral("1px solid %1")
                                         .arg(BookSimResultUi::rgba(th.border));
    const QString bg = BookSimResultUi::rgba(primaryEmphasis ? th.cardBgPrimary : th.cardBg);
    card->setStyleSheet(QStringLiteral("QFrame {"
                                       "background: %1;"
                                       "border-radius: 20px;"
                                       "border: 1px solid %2;"
                                       "border-left: %3;"
                                       "}")
                            .arg(bg, BookSimResultUi::rgba(th.border), borderLeft));

    auto* v = new QVBoxLayout(card);
    v->setContentsMargins(22, 18, 22, 18);
    v->setSpacing(12);

    auto* titleT = new ElaText(title, card);
    titleT->setTextPixelSize(primaryEmphasis ? BookSimResultUi::TypePx::kCardTitleEmph
                                             : BookSimResultUi::TypePx::kCardTitle);
    titleT->setStyleSheet(QStringLiteral("color: %1;").arg(th.fgMain.name(QColor::HexRgb)));
    applySelectableLabel(titleT);
    v->addWidget(titleT);

    if (!subtitle.isEmpty()) {
        auto* sub = new QLabel(subtitle, card);
        sub->setWordWrap(true);
        sub->setStyleSheet(QStringLiteral("font-size: %1px; color: %2;")
                               .arg(BookSimResultUi::TypePx::kBody)
                               .arg(BookSimResultUi::rgba(th.fgMain)));
        applySelectableLabel(sub);
        v->addWidget(sub);
    }

    if (extraWidget != nullptr) {
        v->addWidget(extraWidget);
    }

    const QColor tableInnerBg = BookSimResultUi::mix(th.cardBg,
                                                     th.tileBg,
                                                     primaryEmphasis ? 0.38 : 0.52);
    auto* tableWrap = new QFrame(card);
    tableWrap->setObjectName(QStringLiteral("metricTableWrap"));
    tableWrap->setStyleSheet(
        QStringLiteral("QFrame#metricTableWrap { background-color: %1; border: 1px solid %2; "
                       "border-radius: 14px; }")
            .arg(BookSimResultUi::rgba(tableInnerBg), BookSimResultUi::rgba(th.border)));
    auto* tableWrapLay = new QVBoxLayout(tableWrap);
    tableWrapLay->setContentsMargins(14, 12, 14, 14);
    tableWrapLay->setSpacing(0);

    auto* grid = new QGridLayout();
    grid->setHorizontalSpacing(18);
    grid->setVerticalSpacing(splitAvgAndRange ? 10 : 8);
    int r = 0;
    const QString headerStyle
        = QStringLiteral("font-size: %1px; font-weight: 600; color: %2; padding-bottom: 8px; "
                         "border-bottom: 1px solid %3;")
              .arg(BookSimResultUi::TypePx::kBody)
              .arg(BookSimResultUi::rgba(th.textHint))
              .arg(BookSimResultUi::rgba(BookSimResultUi::mix(th.border, tableInnerBg, 0.62)));
    const QString valueStyle
        = QStringLiteral("font-size: %1px; font-family: 'SF Mono', 'Menlo', 'Consolas', monospace; "
                         "color: %2;")
              .arg(BookSimResultUi::TypePx::kMetricValue)
              .arg(th.fgMain.name(QColor::HexRgb));
    const QString nameStyle = QStringLiteral("font-size: %1px; font-weight: 500; color: %2;")
                                  .arg(BookSimResultUi::TypePx::kBody)
                                  .arg(th.fgMain.name(QColor::HexRgb));

    if (splitAvgAndRange) {
        auto* h0 = new QLabel(tr("指标"), tableWrap);
        h0->setStyleSheet(headerStyle);
        applySelectableLabel(h0);
        auto* h1 = new QLabel(tr("平均值"), tableWrap);
        h1->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        h1->setStyleSheet(headerStyle);
        applySelectableLabel(h1);
        auto* h2 = new QLabel(tr("[ 最小值， 最大值 ]"), tableWrap);
        h2->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        h2->setStyleSheet(headerStyle);
        applySelectableLabel(h2);
        grid->addWidget(h0, r, 0);
        grid->addWidget(h1, r, 1);
        grid->addWidget(h2, r, 2);
        ++r;
    } else if (!rows.isEmpty()) {
        auto* h0 = new QLabel(tr("指标"), tableWrap);
        h0->setStyleSheet(headerStyle);
        applySelectableLabel(h0);
        auto* h1 = new QLabel(tr("平均值"), tableWrap);
        h1->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        h1->setStyleSheet(headerStyle);
        applySelectableLabel(h1);
        grid->addWidget(h0, r, 0);
        grid->addWidget(h1, r, 1);
        ++r;
    }

    for (const BookSimMetricRow& row : rows) {
        auto* nameL = new QLabel(row.name, tableWrap);
        nameL->setStyleSheet(nameStyle);
        applySelectableLabel(nameL);
        grid->addWidget(nameL, r, 0);

        if (splitAvgAndRange) {
            auto* avgL = new QLabel(row.avgOrValue, tableWrap);
            avgL->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
            avgL->setStyleSheet(valueStyle);
            applySelectableLabel(avgL);
            const QString rangeShow = row.range.isEmpty() ? QStringLiteral("—") : row.range;
            auto* rangeL = new QLabel(rangeShow, tableWrap);
            rangeL->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
            rangeL->setStyleSheet(valueStyle);
            applySelectableLabel(rangeL);
            grid->addWidget(avgL, r, 1);
            grid->addWidget(rangeL, r, 2);
        } else {
            auto* valL = new QLabel(row.avgOrValue, tableWrap);
            valL->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
            valL->setStyleSheet(valueStyle);
            applySelectableLabel(valL);
            grid->addWidget(valL, r, 1);
        }
        ++r;
        if (!row.hint.isEmpty()) {
            auto* hintL = new QLabel(row.hint, tableWrap);
            hintL->setWordWrap(true);
            hintL->setStyleSheet(QStringLiteral("font-size: %1px; color: %2;")
                                     .arg(BookSimResultUi::TypePx::kBody)
                                     .arg(BookSimResultUi::rgba(th.fgMain)));
            applySelectableLabel(hintL);
            grid->addWidget(hintL, r, 0, 1, splitAvgAndRange ? 3 : 2);
            ++r;
        }
    }
    grid->setColumnStretch(0, 1);
    if (splitAvgAndRange) {
        grid->setColumnMinimumWidth(1, 112);
        grid->setColumnMinimumWidth(2, 132);
    } else {
        grid->setColumnMinimumWidth(1, 112);
        grid->setColumnStretch(1, 0);
    }
    tableWrapLay->addLayout(grid);
    v->addWidget(tableWrap);
    return card;
}

QWidget* BookSimResultPage::buildClassPanel(QWidget* host, const BookSimTrafficClassStats& stats) {
    const BookSimResultUi::Theme th = BookSimResultUi::themeFrom(eTheme->getThemeMode());

    auto* panel = new QWidget(host);
    panel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    auto* lay = new QVBoxLayout(panel);
    lay->setSpacing(16);
    lay->setContentsMargins(0, 0, 0, 0);

    auto* classHead = new ElaText(tr("流量类别 %1").arg(stats.classId), panel);
    classHead->setTextPixelSize(BookSimResultUi::TypePx::kSection);
    classHead->setStyleSheet(QStringLiteral("color: %1;").arg(th.fgMain.name(QColor::HexRgb)));
    applySelectableLabel(classHead);
    lay->addWidget(classHead);

    const int sampleNote = stats.packetLatency.hasAverage ? stats.packetLatency.samples : 0;
    if (sampleNote > 0) {
        auto* badge = new QLabel(tr("样本数: %1").arg(sampleNote), panel);
        badge->setStyleSheet(QStringLiteral("font-size: %1px; color: %2; padding: 4px 0;")
                                 .arg(BookSimResultUi::TypePx::kBody)
                                 .arg(BookSimResultUi::rgba(th.fgMain)));
        applySelectableLabel(badge);
        lay->addWidget(badge);
    }

    QVector<BookSimMetricRow> latencyRows;
    latencyRows.push_back(
        metricRowFromBand(BookSimMetricLabels::packetLatency(), stats.packetLatency, 4));
    latencyRows.push_back(
        metricRowFromBand(BookSimMetricLabels::networkLatency(), stats.networkLatency, 4));
    latencyRows.push_back(
        metricRowFromBand(BookSimMetricLabels::flitLatency(), stats.flitLatency, 4));
    latencyRows.push_back(
        metricRowFromBand(BookSimMetricLabels::fragmentation(), stats.fragmentation, 6));
    lay->addWidget(createCategoryCard(panel,
                                      BookSimMetricLabels::sectionLatencyFlit(),
                                      QString(),
                                      latencyRows,
                                      nullptr,
                                      th.kpiPacket.name(QColor::HexRgb),
                                      true,
                                      true));

    const double injP = stats.injectedPacketRate.hasAverage ? stats.injectedPacketRate.average : 0.;
    const double accP = stats.acceptedPacketRate.hasAverage ? stats.acceptedPacketRate.average : 0.;
    QString matchVal;
    QString matchFoot;
    if (injP <= 1e-12) {
        matchVal = QStringLiteral("—");
        matchFoot = tr("注入包速率为 0，无法计算比值。");
    } else {
        const double ratio = accP / injP;
        matchVal = QStringLiteral("%1%").arg(ratio * 100., 0, 'f', 1);
        if (ratio >= 0.98) {
            matchFoot.clear();
        } else if (ratio >= 0.85) {
            matchFoot = tr("偏低，可能存在背压或拥塞。");
        } else {
            matchFoot = tr("明显偏低，需关注丢包或饱和。");
        }
    }

    QVector<BookSimMetricRow> thrRows;
    thrRows.push_back({BookSimMetricLabels::throughputMatch(), matchVal, QString(), matchFoot});
    thrRows.push_back(
        metricRowFromBand(BookSimMetricLabels::injectedPacketRate(), stats.injectedPacketRate, 5));
    thrRows.push_back(
        metricRowFromBand(BookSimMetricLabels::acceptedPacketRate(), stats.acceptedPacketRate, 5));
    thrRows.push_back(
        metricRowFromBand(BookSimMetricLabels::injectedFlitRate(), stats.injectedFlitRate, 5));
    thrRows.push_back(
        metricRowFromBand(BookSimMetricLabels::acceptedFlitRate(), stats.acceptedFlitRate, 5));
    lay->addWidget(createCategoryCard(panel,
                                      BookSimMetricLabels::sectionThroughput(),
                                      QString(),
                                      thrRows,
                                      nullptr,
                                      th.thrAccPkt.name(QColor::HexRgb),
                                      false,
                                      true));

    QVector<BookSimMetricRow> shapePathRows;
    shapePathRows.push_back(metricRowFromBand(BookSimMetricLabels::injectedMeanPacketSize(),
                                              stats.injectedPacketSize,
                                              4));
    shapePathRows.push_back(metricRowFromBand(BookSimMetricLabels::acceptedMeanPacketSize(),
                                              stats.acceptedPacketSize,
                                              4));
    shapePathRows.push_back(metricRowFromBand(BookSimMetricLabels::meanHops(), stats.hops, 5));
    lay->addWidget(createCategoryCard(panel,
                                      BookSimMetricLabels::sectionShapePath(),
                                      QString(),
                                      shapePathRows,
                                      nullptr,
                                      th.accentHops.name(QColor::HexRgb),
                                      false,
                                      false));

    return panel;
}
