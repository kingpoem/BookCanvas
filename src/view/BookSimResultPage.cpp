#include "BookSimResultPage.h"
#include <ElaPushButton.h>
#include <ElaScrollPageArea.h>
#include <ElaText.h>
#include <QApplication>
#include <QClipboard>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPalette>
#include <QProgressBar>
#include <QScrollArea>
#include <QTabWidget>
#include <QVBoxLayout>
#include <algorithm>
#include <cmath>

namespace {

[[nodiscard]] QString formatBand(const BookSimStatBand& b, int precision) {
    if (!b.hasAverage) {
        return QStringLiteral("—");
    }
    QString s = QString::number(b.average, 'f', precision);
    if (b.hasMinimum && b.hasMaximum) {
        s += QStringLiteral(" · [%1, %2]")
                 .arg(b.minimum, 0, 'f', precision)
                 .arg(b.maximum, 0, 'f', precision);
    }
    return s;
}

[[nodiscard]] QWidget* makeLatencyLadder(QWidget* parent, const BookSimTrafficClassStats& s) {
    auto* wrap = new QWidget(parent);
    auto* layout = new QVBoxLayout(wrap);
    layout->setContentsMargins(0, 4, 0, 8);
    layout->setSpacing(6);

    const double p = s.packetLatency.hasAverage ? s.packetLatency.average : 0.;
    const double n = s.networkLatency.hasAverage ? s.networkLatency.average : 0.;
    const double f = s.flitLatency.hasAverage ? s.flitLatency.average : 0.;
    const double mx = std::max({p, n, f, 1.});

    const struct {
        QString label;
        double value;
        QString tip;
    } rows[] = {
        {QObject::tr("Packet 延迟"), p, QObject::tr("含源头注入/排队")},
        {QObject::tr("Network 延迟"), n, QObject::tr("首 flit 进入路由器起算")},
        {QObject::tr("Flit 延迟"), f, QObject::tr("逐 flit 平均，通常 ≤ Network")},
    };

    for (const auto& row : rows) {
        if (row.value <= 0.) {
            continue;
        }
        auto* rowW = new QWidget(wrap);
        auto* h = new QHBoxLayout(rowW);
        h->setContentsMargins(0, 0, 0, 0);
        auto* lab = new QLabel(QStringLiteral("%1 (%2)").arg(row.label, row.tip), rowW);
        lab->setWordWrap(false);
        lab->setStyleSheet(QStringLiteral("font-size: 12px;"));
        lab->setMinimumWidth(200);
        auto* bar = new QProgressBar(rowW);
        bar->setRange(0, 1000);
        bar->setValue(static_cast<int>(std::lround(1000. * row.value / mx)));
        bar->setMaximumHeight(22);
        bar->setTextVisible(true);
        bar->setFormat(QStringLiteral("%1 周期").arg(row.value, 0, 'f', 2));
        h->addWidget(lab, 1);
        h->addWidget(bar, 2);
        layout->addWidget(rowW);
    }

    auto* hint = new QLabel(QObject::tr(
                                "一般关系：Packet ≥ Network ≥ Flit（含义因统计口径略有出入）。"),
                            wrap);
    hint->setWordWrap(true);
    hint->setForegroundRole(QPalette::PlaceholderText);
    layout->addWidget(hint);
    return wrap;
}

} // namespace

BookSimResultPage::BookSimResultPage(QWidget* parent)
    : BasePage(parent) {
    setWindowTitle(tr("BookSim 结果"));

    auto* central = new QWidget(this);
    central->setWindowTitle(tr("BookSim 结果"));
    auto* mainLay = new QVBoxLayout(central);
    mainLay->setContentsMargins(0, 0, 20, 0);
    mainLay->setSpacing(16);

    auto* toolbarArea = new ElaScrollPageArea(this);
    auto* toolbarLay = new QHBoxLayout(toolbarArea);
    toolbarLay->setContentsMargins(0, 0, 0, 0);

    m_pasteButton = new ElaPushButton(tr("从剪贴板解析"), this);
    toolbarLay->addWidget(m_pasteButton);
    toolbarLay->addStretch();

    m_statusText
        = new ElaText(tr("运行 Simulation 结束后将自动载入；也可粘贴完整终端输出后点击解析。"),
                      this);
    m_statusText->setTextPixelSize(13);
    m_statusText->setWordWrap(true);

    m_scrollInner = new QWidget(central);
    m_bodyLayout = new QVBoxLayout(m_scrollInner);
    m_bodyLayout->setContentsMargins(0, 0, 0, 0);
    m_bodyLayout->setSpacing(14);

    m_scroll = new QScrollArea(central);
    m_scroll->setWidgetResizable(true);
    m_scroll->setWidget(m_scrollInner);
    m_scroll->setFrameShape(QFrame::NoFrame);

    mainLay->addWidget(toolbarArea);
    mainLay->addWidget(m_statusText);
    mainLay->addWidget(m_scroll, 1);

    connect(m_pasteButton, &ElaPushButton::clicked, this, &BookSimResultPage::loadFromClipboard);

    addCentralWidget(central, true, true, 0);
}

void BookSimResultPage::ingestSimulationLog(const QString& text) {
    rebuildContent(BookSimStatsParser::parseOverallFromLog(text));
}

void BookSimResultPage::loadFromClipboard() {
    const QClipboard* clip = QApplication::clipboard();
    if (!clip) {
        setStatus(tr("无法访问剪贴板。"), true);
        return;
    }
    ingestSimulationLog(clip->text());
}

void BookSimResultPage::setStatus(const QString& message, bool isError) {
    m_statusText->setText(message);
    if (isError) {
        m_statusText->setForegroundRole(QPalette::Highlight);
    } else {
        m_statusText->setForegroundRole(QPalette::Text);
    }
}

void BookSimResultPage::rebuildContent(const BookSimParseResult& result) {
    while (QLayoutItem* item = m_bodyLayout->takeAt(0)) {
        if (QWidget* w = item->widget()) {
            delete w;
        }
        delete item;
    }

    if (!result.ok()) {
        setStatus(result.errorMessage, true);
        auto* err = new QLabel(result.errorMessage, m_scrollInner);
        err->setWordWrap(true);
        err->setForegroundRole(QPalette::Text);
        m_bodyLayout->addWidget(err);
        m_bodyLayout->addStretch();
        return;
    }

    setStatus(tr("解析成功：共 %1 个 traffic class。核心指标已按类别汇总如下。")
                  .arg(result.classes.size()),
              false);

    if (result.totalRunTimeSec.has_value()) {
        auto* wall
            = new QLabel(tr("总运行时间（程序报告）: %1 s").arg(*result.totalRunTimeSec, 0, 'g', 9),
                         m_scrollInner);
        wall->setStyleSheet(QStringLiteral("font-size: 13px;"));
        m_bodyLayout->addWidget(wall);
    }

    if (result.classes.size() == 1) {
        m_bodyLayout->addWidget(buildClassPanel(m_scrollInner, result.classes.front()));
    } else {
        auto* tabs = new QTabWidget(m_scrollInner);
        tabs->setDocumentMode(true);
        for (const auto& c : result.classes) {
            auto* page = new QWidget(tabs);
            auto* v = new QVBoxLayout(page);
            v->setContentsMargins(8, 8, 8, 8);
            v->addWidget(buildClassPanel(page, c));
            v->addStretch();
            tabs->addTab(page, tr("Class %1").arg(c.classId));
        }
        m_bodyLayout->addWidget(tabs);
    }

    m_bodyLayout->addStretch();
}

QFrame* BookSimResultPage::createCategoryCard(QWidget* host,
                                              const QString& title,
                                              const QString& subtitle,
                                              const QVector<BookSimMetricRow>& rows,
                                              QWidget* extraWidget) {
    auto* card = new QFrame(host);
    card->setObjectName(QStringLiteral("booksimCategoryCard"));
    card->setStyleSheet(QStringLiteral("QFrame#booksimCategoryCard {"
                                       "background: rgba(128,128,128,0.07);"
                                       "border-radius: 16px;"
                                       "border: 1px solid rgba(128,128,128,0.14);"
                                       "}"));

    auto* v = new QVBoxLayout(card);
    v->setContentsMargins(18, 16, 18, 16);
    v->setSpacing(10);

    auto* titleT = new ElaText(title, card);
    titleT->setTextPixelSize(17);
    v->addWidget(titleT);

    if (!subtitle.isEmpty()) {
        auto* sub = new QLabel(subtitle, card);
        sub->setWordWrap(true);
        sub->setForegroundRole(QPalette::PlaceholderText);
        sub->setStyleSheet(QStringLiteral("font-size: 12px;"));
        v->addWidget(sub);
    }

    if (extraWidget != nullptr) {
        v->addWidget(extraWidget);
    }

    auto* grid = new QGridLayout();
    grid->setHorizontalSpacing(12);
    grid->setVerticalSpacing(6);
    int r = 0;
    for (const BookSimMetricRow& row : rows) {
        auto* nameL = new QLabel(row.name, card);
        nameL->setStyleSheet(QStringLiteral("font-size: 13px; font-weight: 500;"));
        auto* valL = new QLabel(row.value, card);
        valL->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        valL->setStyleSheet(QStringLiteral(
            "font-size: 14px; font-family: 'SF Mono', 'Menlo', 'Consolas', monospace;"));
        grid->addWidget(nameL, r, 0);
        grid->addWidget(valL, r, 1);
        ++r;
        if (!row.hint.isEmpty()) {
            auto* hintL = new QLabel(row.hint, card);
            hintL->setWordWrap(true);
            hintL->setForegroundRole(QPalette::PlaceholderText);
            hintL->setStyleSheet(QStringLiteral("font-size: 11px;"));
            grid->addWidget(hintL, r, 0, 1, 2);
            ++r;
        }
    }
    grid->setColumnStretch(1, 1);
    v->addLayout(grid);
    return card;
}

QWidget* BookSimResultPage::buildClassPanel(QWidget* host, const BookSimTrafficClassStats& stats) {
    auto* panel = new QWidget(host);
    auto* lay = new QVBoxLayout(panel);
    lay->setSpacing(14);
    lay->setContentsMargins(0, 0, 0, 0);

    const int sampleNote = stats.packetLatency.hasAverage ? stats.packetLatency.samples : 0;
    if (sampleNote > 0) {
        auto* badge = new QLabel(tr("有效样本窗口数（samples）: %1").arg(sampleNote), panel);
        badge->setStyleSheet(QStringLiteral("font-size: 12px; opacity: 0.85;"));
        lay->addWidget(badge);
    }

    QVector<BookSimMetricRow> latencyRows;
    latencyRows.push_back({tr("Packet 延迟"),
                           formatBand(stats.packetLatency, 4),
                           tr("端到端：含注入侧等待与网内行为。")});
    latencyRows.push_back({tr("Network 延迟"),
                           formatBand(stats.networkLatency, 4),
                           tr("网内路径：从首 flit 进入路由器开始计量。")});
    latencyRows.push_back({tr("Flit 延迟"),
                           formatBand(stats.flitLatency, 4),
                           tr("微片级平均驻留；与尾微片等待等有关。")});
    latencyRows.push_back({tr("Fragmentation"),
                           formatBand(stats.fragmentation, 6),
                           tr("同一包内 flit 间隔，越小通常表示 VC/调度更顺滑。")});
    lay->addWidget(
        createCategoryCard(panel,
                           tr("延迟与微片打散"),
                           tr("判断「多快」时优先看延迟分层；再观察 Fragmentation 是否异常升高。"),
                           latencyRows,
                           makeLatencyLadder(panel, stats)));

    const double injP = stats.injectedPacketRate.hasAverage ? stats.injectedPacketRate.average : 0.;
    const double accP = stats.acceptedPacketRate.hasAverage ? stats.acceptedPacketRate.average : 0.;
    QString thrHint = tr("注入与接纳接近表示未明显饱和；若接纳显著低于注入，可能存在背压或瓶颈。");
    if (injP > 1e-9 && accP > 1e-9) {
        const double ratio = accP / injP;
        thrHint += tr(" 当前接纳/注入（包）≈ %1。").arg(ratio, 0, 'f', 3);
    }

    QVector<BookSimMetricRow> thrRows;
    thrRows.push_back({tr("注入包率"),
                       formatBand(stats.injectedPacketRate, 5),
                       tr("每周期每个节点的平均注入包数的上界趋势参考。")});
    thrRows.push_back(
        {tr("接纳包率"), formatBand(stats.acceptedPacketRate, 5), tr(" sink 侧成功接收速率。")});
    thrRows.push_back({tr("注入 flit 率"),
                       formatBand(stats.injectedFlitRate, 5),
                       tr("与包长分布共同决定链路占用。")});
    thrRows.push_back({tr("接纳 flit 率"),
                       formatBand(stats.acceptedFlitRate, 5),
                       tr("可与注入 flit 率对比判断丢包/背压。")});
    lay->addWidget(createCategoryCard(panel, tr("吞吐与接纳"), thrHint, thrRows));

    QVector<BookSimMetricRow> shapeRows;
    shapeRows.push_back({tr("注入平均包长（flits）"),
                         formatBand(stats.injectedPacketSize, 4),
                         tr("由统计口径折合成 flit 计数的平均包长。")});
    shapeRows.push_back({tr("接纳平均包长（flits）"),
                         formatBand(stats.acceptedPacketSize, 4),
                         tr("若与注入侧差异大，需留意测量边界或流量过滤。")});
    lay->addWidget(createCategoryCard(panel,
                                      tr("数据包形态"),
                                      tr("用于对照配置中的 packet size 与有效载荷比例。"),
                                      shapeRows));

    QVector<BookSimMetricRow> hopRows;
    hopRows.push_back({tr("平均跳数"),
                       formatBand(stats.hops, 5),
                       tr("与拓扑、路由函数共同决定最小可能延迟下界。")});
    lay->addWidget(createCategoryCard(panel, tr("路径与拓扑行为"), QString(), hopRows));

    return panel;
}
