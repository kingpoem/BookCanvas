#include "BookSimResultPage.h"
#include <ElaDef.h>
#include <ElaPushButton.h>
#include <ElaScrollPageArea.h>
#include <ElaText.h>
#include <ElaTheme.h>
#include <QApplication>
#include <QClipboard>
#include <QFont>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPalette>
#include <QProgressBar>
#include <QScrollArea>
#include <QSizePolicy>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QWidget>
#include <algorithm>
#include <cmath>

namespace BookSimResultUi {

/// 全页字号底线：避免 10px/11px 等难以阅读的辅助文案
namespace TypePx {
constexpr int kBody = 13;      // 最小正文（注释、表格说明、KPI 脚注等）
constexpr int kLead = 14;      // 副标题、状态行、数值行
constexpr int kSection = 16;   // 分区标题（如流量类别）
constexpr int kCardTitle = 18; // 卡片标题
constexpr int kCardTitleEmph = 20;
constexpr int kStatus = 14;
constexpr int kMetricValue = 15;
constexpr int kKpiValue = 28;
} // namespace TypePx

/// 线性混合：返回 a·t + b·(1−t)。要做「在 b 上混入少许 a」时，应把「混入的浅色」放在 a、底色素放在 b，t 取小比例。
[[nodiscard]] QColor mix(const QColor& a, const QColor& b, qreal t) {
    const qreal u = 1.0 - t;
    return QColor(qBound(0, qRound(a.red() * t + b.red() * u), 255),
                  qBound(0, qRound(a.green() * t + b.green() * u), 255),
                  qBound(0, qRound(a.blue() * t + b.blue() * u), 255),
                  qBound(0, qRound(a.alpha() * t + b.alpha() * u), 255));
}

[[nodiscard]] QString rgba(const QColor& c) {
    return QStringLiteral("rgba(%1,%2,%3,%4)")
        .arg(c.red())
        .arg(c.green())
        .arg(c.blue())
        .arg(c.alphaF(), 0, 'f', 3);
}

struct Theme {
    QColor pageBg;
    QColor cardBg;
    QColor cardBgPrimary;
    QColor border;
    QColor tileBg;
    QColor heroG0;
    QColor heroG1;
    QColor heroBorder;
    QColor barTrack;
    QColor chartPacket;
    QColor chartNetwork;
    QColor chartFlit;
    QColor thrInjPkt;
    QColor thrAccPkt;
    QColor thrInjFlit;
    QColor thrAccFlit;
    QColor kpiPacket;
    QColor kpiNetwork;
    QColor kpiHops;
    QColor accentShape;
    QColor accentHops;
    QColor statusBalanced;
    QColor statusWarn;
    QColor statusBad;
    QColor textMuted;
    QColor textHint;
    /// 主正文色：浅色主题为黑色，暗色为亮色
    QColor fgMain;
    /// 次文案：暗色略弱；浅色与 fgMain 同为黑
    QColor fgDim;
};

[[nodiscard]] Theme themeFrom(ElaThemeType::ThemeMode mode) {
    Theme th;
    const QColor border = ElaThemeColor(mode, BasicBorder);
    th.border = border;

    if (mode == ElaThemeType::Light) {
        const QColor paper = Qt::white;
        const QColor ink(0x0a, 0x0a, 0x0a);
        th.pageBg = paper;
        th.cardBg = paper;
        th.cardBgPrimary = paper;
        th.tileBg = paper;
        th.heroG0 = QColor(0xf5, 0xf5, 0xf5);
        th.heroG1 = QColor(0xfa, 0xfa, 0xfa);
        th.heroBorder = border;
        th.barTrack = QColor(0xe5, 0xe5, 0xe5);
        th.fgMain = ink;
        th.fgDim = ink;
        th.textMuted = ink;
        th.textHint = ink;
        th.chartPacket = QColor(0x1D, 0x4E, 0xD8);
        th.chartNetwork = QColor(0x4F, 0x46, 0xE5);
        th.chartFlit = QColor(0x03, 0x69, 0xA1);
        th.thrInjPkt = QColor(0x4F, 0x46, 0xE5);
        th.thrAccPkt = QColor(0x04, 0x78, 0x57);
        th.thrInjFlit = QColor(0x6D, 0x28, 0xD9);
        th.thrAccFlit = QColor(0x0F, 0x76, 0x69);
        th.kpiPacket = th.chartPacket;
        th.kpiNetwork = th.chartNetwork;
        th.kpiHops = QColor(0x57, 0x53, 0x4E);
        th.accentShape = QColor(0x57, 0x53, 0x4E);
        th.accentHops = QColor(0xB4, 0x53, 0x09);
        th.statusBalanced = QColor(0x04, 0x78, 0x57);
        th.statusWarn = QColor(0xB4, 0x53, 0x09);
        th.statusBad = ElaThemeColor(mode, StatusDanger);
    } else {
        const QColor canvas = ElaThemeColor(mode, WindowBase);
        const QColor surface = ElaThemeColor(mode, PopupBase);
        th.pageBg = canvas;
        th.cardBg = canvas;
        th.cardBgPrimary = canvas;
        // KPI 小卡：略浅于底色的墨灰面（勿反向 mix，否则会变成大比例纯白）
        th.tileBg = mix(surface, canvas, 0.62);
        th.heroG0 = canvas;
        th.heroG1 = mix(surface, canvas, 0.35);
        th.heroBorder = border;
        const QColor base = ElaThemeColor(mode, BasicBase);
        th.barTrack = mix(QColor(255, 255, 255), base, 0.14);
        th.fgMain = ElaThemeColor(mode, BasicText);
        const QColor lightGray(0xE4, 0xE4, 0xE7);
        th.textMuted = mix(ElaThemeColor(mode, BasicDetailsText), lightGray, 0.22);
        th.textHint = mix(ElaThemeColor(mode, BasicTextNoFocus), lightGray, 0.18);
        th.fgDim = th.textMuted;
        th.chartPacket = QColor(0x60, 0xA5, 0xFA);
        th.chartNetwork = QColor(0x81, 0x8C, 0xF8);
        th.chartFlit = QColor(0x38, 0xBD, 0xF8);
        th.thrInjPkt = QColor(0xA5, 0xB4, 0xFC);
        th.thrAccPkt = QColor(0x34, 0xD3, 0x99);
        th.thrInjFlit = QColor(0xC4, 0xB5, 0xFD);
        th.thrAccFlit = QColor(0x2D, 0xD4, 0xBF);
        th.kpiPacket = th.chartPacket;
        th.kpiNetwork = th.chartNetwork;
        th.kpiHops = QColor(0xA8, 0xA2, 0x9E);
        th.accentShape = QColor(0xA8, 0xA2, 0x9E);
        th.accentHops = QColor(0xF5, 0x9E, 0x0B);
        th.statusBalanced = QColor(0x34, 0xD3, 0x99);
        th.statusWarn = QColor(0xFB, 0xBF, 0x24);
        th.statusBad = QColor(0xF8, 0x71, 0x71);
    }
    return th;
}

} // namespace BookSimResultUi

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

[[nodiscard]] QString formatAverageOnly(const BookSimStatBand& b, int precision) {
    if (!b.hasAverage) {
        return QStringLiteral("—");
    }
    return QString::number(b.average, 'f', precision);
}

void styleProgressTrack(QProgressBar* bar, const QColor& chunk, const BookSimResultUi::Theme& th) {
    bar->setTextVisible(false);
    bar->setMinimumHeight(14);
    bar->setMaximumHeight(14);
    bar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    bar->setStyleSheet(
        QStringLiteral("QProgressBar { border: none; border-radius: 6px; "
                       "background-color: %1; }"
                       "QProgressBar::chunk { border-radius: 6px; background-color: %2; }")
            .arg(BookSimResultUi::rgba(th.barTrack), chunk.name(QColor::HexRgb)));
}

[[nodiscard]] QWidget* makeKpiTile(QWidget* parent,
                                   const BookSimResultUi::Theme& th,
                                   const QString& caption,
                                   const QString& value,
                                   const QString& footnote,
                                   const QColor& accentColor) {
    auto* tile = new QFrame(parent);
    tile->setMinimumWidth(120);
    tile->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    tile->setAttribute(Qt::WA_StyledBackground, true);
    tile->setStyleSheet(QStringLiteral("QFrame { background-color: %1; border-radius: 16px; "
                                       "border: 1px solid %2; border-top: 3px solid %3; }")
                            .arg(BookSimResultUi::rgba(th.tileBg),
                                 BookSimResultUi::rgba(th.border),
                                 accentColor.name(QColor::HexRgb)));

    auto* v = new QVBoxLayout(tile);
    v->setContentsMargins(16, 16, 16, 16);
    v->setSpacing(8);

    auto* cap = new QLabel(caption, tile);
    cap->setStyleSheet(QStringLiteral("font-size: %1px; color: %2;")
                           .arg(BookSimResultUi::TypePx::kBody)
                           .arg(BookSimResultUi::rgba(th.fgDim)));
    cap->setWordWrap(true);

    auto* val = new QLabel(value, tile);
    val->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    val->setMinimumHeight(40);
    val->setStyleSheet(QStringLiteral("font-size: %1px; font-weight: 700; font-family: 'SF Mono', "
                                      "'Menlo', 'Consolas', monospace; color: %2; padding: 4px 0;")
                           .arg(BookSimResultUi::TypePx::kKpiValue)
                           .arg(BookSimResultUi::rgba(th.fgMain)));

    auto* foot = new QLabel(footnote, tile);
    foot->setWordWrap(true);
    foot->setStyleSheet(QStringLiteral("font-size: %1px; color: %2;")
                            .arg(BookSimResultUi::TypePx::kBody)
                            .arg(BookSimResultUi::rgba(th.textHint)));

    v->addWidget(cap);
    v->addWidget(val);
    v->addWidget(foot);
    return tile;
}

[[nodiscard]] QWidget* makeHeroKpiStrip(QWidget* parent,
                                        const BookSimTrafficClassStats& s,
                                        const BookSimResultUi::Theme& th) {
    auto* wrap = new QFrame(parent);
    wrap->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    wrap->setStyleSheet(
        QStringLiteral("QFrame { background: qlineargradient(x1:0,y1:0,x2:1,y2:1, "
                       "stop:0 %1, stop:1 %2); border-radius: 20px; border: 1px solid %3; }")
            .arg(BookSimResultUi::rgba(th.heroG0),
                 BookSimResultUi::rgba(th.heroG1),
                 BookSimResultUi::rgba(th.heroBorder)));

    auto* outer = new QVBoxLayout(wrap);
    outer->setContentsMargins(22, 18, 22, 18);
    outer->setSpacing(14);

    auto* hint = new QLabel(QObject::tr("核心 KPI — 优先判断延迟是否达标、网络是否饱和"), wrap);
    hint->setStyleSheet(QStringLiteral("font-size: %1px; font-weight: 600; color: %2;")
                            .arg(BookSimResultUi::TypePx::kLead)
                            .arg(BookSimResultUi::rgba(th.fgMain)));
    outer->addWidget(hint);

    auto* row = new QHBoxLayout();
    row->setSpacing(16);

    const QString pktLat = formatAverageOnly(s.packetLatency, 3);
    row->addWidget(makeKpiTile(wrap,
                               th,
                               QObject::tr("端到端包延迟（周期）"),
                               pktLat,
                               QObject::tr("含注入排队与网内行为"),
                               th.kpiPacket),
                   1);

    const double injP = s.injectedPacketRate.hasAverage ? s.injectedPacketRate.average : 0.;
    const double accP = s.acceptedPacketRate.hasAverage ? s.acceptedPacketRate.average : 0.;
    QString satValue;
    QString satFoot;
    QColor satAccent = QColor(0x0D, 0x94, 0x88);
    if (injP <= 1e-12) {
        satValue = QStringLiteral("—");
        satFoot = QObject::tr("无有效注入率数据");
    } else {
        const double ratio = accP / injP;
        satValue = QStringLiteral("%1%").arg(ratio * 100., 0, 'f', 1);
        satFoot = QObject::tr("接纳/注入（包率）");
        if (ratio >= 0.98) {
            satAccent = th.statusBalanced;
            satFoot += QObject::tr(" · 平衡");
        } else if (ratio >= 0.85) {
            satAccent = th.statusWarn;
            satFoot += QObject::tr(" · 轻微背压");
        } else {
            satAccent = th.statusBad;
            satFoot += QObject::tr(" · 瓶颈/丢包风险");
        }
    }
    row->addWidget(makeKpiTile(wrap, th, QObject::tr("吞吐匹配度"), satValue, satFoot, satAccent),
                   1);

    row->addWidget(makeKpiTile(wrap,
                               th,
                               QObject::tr("网内延迟（周期）"),
                               formatAverageOnly(s.networkLatency, 3),
                               QObject::tr("首 flit 进路由器起算"),
                               th.kpiNetwork),
                   1);

    row->addWidget(makeKpiTile(wrap,
                               th,
                               QObject::tr("平均跳数"),
                               formatAverageOnly(s.hops, 4),
                               QObject::tr("与拓扑/路由下界对照"),
                               th.kpiHops),
                   1);

    outer->addLayout(row);
    return wrap;
}

[[nodiscard]] QWidget* makeThroughputCompare(QWidget* parent,
                                             const BookSimTrafficClassStats& s,
                                             const BookSimResultUi::Theme& th) {
    auto* wrap = new QWidget(parent);
    auto* layout = new QVBoxLayout(wrap);
    layout->setContentsMargins(0, 4, 0, 8);
    layout->setSpacing(12);

    auto addBarRow =
        [wrap,
         &th](const QString& label, double a, double b, const QColor& colorA, const QString& tip) {
            const double mx = std::max({a, b, 1e-12});
            auto* rowW = new QWidget(wrap);
            auto* h = new QHBoxLayout(rowW);
            h->setContentsMargins(0, 0, 0, 0);
            h->setSpacing(14);
            auto* lab = new QLabel(QStringLiteral("%1 — %2").arg(label, tip), rowW);
            lab->setStyleSheet(QStringLiteral("font-size: %1px; color: %2;")
                                   .arg(BookSimResultUi::TypePx::kBody)
                                   .arg(BookSimResultUi::rgba(th.fgMain)));
            lab->setMinimumWidth(220);
            lab->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
            auto* bar = new QProgressBar(rowW);
            bar->setRange(0, 1000);
            bar->setValue(static_cast<int>(std::lround(1000. * a / mx)));
            styleProgressTrack(bar, colorA, th);
            auto* val = new QLabel(QString::number(a, 'f', 5), rowW);
            val->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
            val->setStyleSheet(QStringLiteral("font-size: %1px; font-family: 'SF Mono', 'Menlo', "
                                              "'Consolas', monospace; color: %2;")
                                   .arg(BookSimResultUi::TypePx::kBody)
                                   .arg(BookSimResultUi::rgba(th.fgMain)));
            val->setMinimumWidth(96);
            h->addWidget(lab, 0);
            h->addWidget(bar, 1);
            h->addWidget(val, 0);
            return rowW;
        };

    const double injPkt = s.injectedPacketRate.hasAverage ? s.injectedPacketRate.average : 0.;
    const double accPkt = s.acceptedPacketRate.hasAverage ? s.acceptedPacketRate.average : 0.;
    const double injFlit = s.injectedFlitRate.hasAverage ? s.injectedFlitRate.average : 0.;
    const double accFlit = s.acceptedFlitRate.hasAverage ? s.acceptedFlitRate.average : 0.;

    auto* sub = new QLabel(QObject::tr(
                               "并排对比便于一眼看出注入与接纳是否对齐；条形按各行最大值归一化。"),
                           wrap);
    sub->setWordWrap(true);
    sub->setStyleSheet(QStringLiteral("font-size: %1px; color: %2;")
                           .arg(BookSimResultUi::TypePx::kBody)
                           .arg(BookSimResultUi::rgba(th.fgMain)));
    layout->addWidget(sub);

    layout->addWidget(addBarRow(QObject::tr("包率（pkt/cycle）"),
                                injPkt,
                                accPkt,
                                th.thrInjPkt,
                                QObject::tr("注入")),
                      0);
    layout->addWidget(addBarRow(QObject::tr("包率（pkt/cycle）"),
                                accPkt,
                                injPkt,
                                th.thrAccPkt,
                                QObject::tr("接纳")),
                      0);

    layout->addWidget(addBarRow(QObject::tr("Flit 率"),
                                injFlit,
                                accFlit,
                                th.thrInjFlit,
                                QObject::tr("注入")),
                      0);
    layout->addWidget(addBarRow(QObject::tr("Flit 率"),
                                accFlit,
                                injFlit,
                                th.thrAccFlit,
                                QObject::tr("接纳")),
                      0);

    return wrap;
}

[[nodiscard]] QWidget* makeLatencyLadder(QWidget* parent,
                                         const BookSimTrafficClassStats& s,
                                         const BookSimResultUi::Theme& th) {
    auto* wrap = new QWidget(parent);
    auto* layout = new QVBoxLayout(wrap);
    layout->setContentsMargins(0, 4, 0, 8);
    layout->setSpacing(10);

    const double p = s.packetLatency.hasAverage ? s.packetLatency.average : 0.;
    const double n = s.networkLatency.hasAverage ? s.networkLatency.average : 0.;
    const double f = s.flitLatency.hasAverage ? s.flitLatency.average : 0.;
    const double mx = std::max({p, n, f, 1.});

    const struct RowData {
        QString label;
        double value;
        QString tip;
        QColor color;
    } rowData[] = {
        {QObject::tr("Packet 延迟"), p, QObject::tr("含源头注入/排队"), th.chartPacket},
        {QObject::tr("Network 延迟"), n, QObject::tr("首 flit 进入路由器起算"), th.chartNetwork},
        {QObject::tr("Flit 延迟"), f, QObject::tr("逐 flit 平均，通常 ≤ Network"), th.chartFlit},
    };

    for (const auto& row : rowData) {
        if (row.value <= 0.) {
            continue;
        }
        auto* rowW = new QWidget(wrap);
        auto* h = new QHBoxLayout(rowW);
        h->setContentsMargins(0, 0, 0, 0);
        h->setSpacing(16);
        auto* lab = new QLabel(QStringLiteral("%1 (%2)").arg(row.label, row.tip), rowW);
        lab->setWordWrap(false);
        lab->setStyleSheet(QStringLiteral("font-size: %1px; color: %2;")
                               .arg(BookSimResultUi::TypePx::kBody)
                               .arg(BookSimResultUi::rgba(th.fgMain)));
        lab->setMinimumWidth(260);
        lab->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
        auto* bar = new QProgressBar(rowW);
        bar->setRange(0, 1000);
        bar->setValue(static_cast<int>(std::lround(1000. * row.value / mx)));
        bar->setMinimumHeight(28);
        bar->setMaximumHeight(30);
        bar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        bar->setTextVisible(true);
        bar->setFormat(QStringLiteral("%1 周期").arg(row.value, 0, 'f', 2));
        bar->setStyleSheet(QStringLiteral("QProgressBar { border: none; border-radius: 10px; "
                                          "background-color: %1; padding-right: 10px; text-align: "
                                          "right; font-size: %3px; "
                                          "font-family: 'SF Mono', 'Menlo', 'Consolas', monospace; "
                                          "color: %4; }"
                                          "QProgressBar::chunk { border-radius: 10px; "
                                          "background-color: %2; }")
                               .arg(BookSimResultUi::rgba(th.barTrack),
                                    row.color.name(QColor::HexRgb),
                                    QString::number(BookSimResultUi::TypePx::kBody),
                                    th.fgMain.name(QColor::HexRgb)));
        h->addWidget(lab, 0);
        h->addWidget(bar, 1);
        layout->addWidget(rowW);
    }

    auto* hint = new QLabel(QObject::tr(
                                "一般关系：Packet ≥ Network ≥ "
                                "Flit（口径略有差异时除外）。分解有助定位瓶颈在注入侧还是网内。"),
                            wrap);
    hint->setWordWrap(true);
    hint->setStyleSheet(QStringLiteral("font-size: %1px; color: %2;")
                            .arg(BookSimResultUi::TypePx::kBody)
                            .arg(BookSimResultUi::rgba(th.fgMain)));
    layout->addWidget(hint);
    return wrap;
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
    mainLay->setSpacing(16);

    auto* toolbarArea = new ElaScrollPageArea(this);
    auto* toolbarLay = new QHBoxLayout(toolbarArea);
    toolbarLay->setContentsMargins(0, 0, 0, 0);

    m_pasteButton = new ElaPushButton(tr("从剪贴板解析"), this);
    toolbarLay->addWidget(m_pasteButton);
    toolbarLay->addStretch();

    m_statusText = new ElaText(tr("运行 Simulation 结束后将自动载入"), this);
    m_statusText->setTextPixelSize(BookSimResultUi::TypePx::kStatus);
    m_statusText->setWordWrap(true);

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

    mainLay->addWidget(toolbarArea);
    mainLay->addWidget(m_statusText);
    mainLay->addWidget(m_scroll, 1);

    connect(m_pasteButton, &ElaPushButton::clicked, this, &BookSimResultPage::loadFromClipboard);

    setStatus(tr("运行 Simulation 结束后将自动载入"), false);

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

void BookSimResultPage::loadFromClipboard() {
    const QClipboard* clip = QApplication::clipboard();
    if (!clip) {
        setStatus(tr("无法访问剪贴板。"), true);
        return;
    }
    ingestSimulationLog(clip->text());
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
        if (QWidget* w = item->widget()) {
            delete w;
        }
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
        m_bodyLayout->addWidget(err);
        m_bodyLayout->addStretch();
        return;
    }

    setStatus(tr("解析成功：共 %1 个 traffic class").arg(result.classes.size()), false);

    if (result.totalRunTimeSec.has_value()) {
        auto* wall = new QFrame(m_scrollInner);
        wall->setStyleSheet(
            QStringLiteral("QFrame { background-color: %1; border-radius: 12px; "
                           "border: 1px solid %2; }")
                .arg(BookSimResultUi::rgba(BookSimResultUi::mix(th.cardBg, th.border, 0.75)),
                     BookSimResultUi::rgba(th.border)));
        auto* wh = new QHBoxLayout(wall);
        wh->setContentsMargins(16, 10, 16, 10);
        auto* wlab = new QLabel(tr("总运行时间（程序报告）"), wall);
        wlab->setStyleSheet(QStringLiteral("font-size: %1px; color: %2;")
                                .arg(BookSimResultUi::TypePx::kBody)
                                .arg(BookSimResultUi::rgba(th.fgDim)));
        auto* wval = new QLabel(QStringLiteral("%1 s").arg(*result.totalRunTimeSec, 0, 'g', 9),
                                wall);
        wval->setStyleSheet(QStringLiteral("font-size: %1px; font-weight: 600; font-family: 'SF "
                                           "Mono', 'Menlo', 'Consolas', monospace; color: %2;")
                                .arg(BookSimResultUi::TypePx::kLead)
                                .arg(th.fgMain.name(QColor::HexRgb)));
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
        tabs->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
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
                                              QWidget* extraWidget,
                                              const QString& accentColor,
                                              bool primaryEmphasis) {
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
    v->addWidget(titleT);

    if (!subtitle.isEmpty()) {
        auto* sub = new QLabel(subtitle, card);
        sub->setWordWrap(true);
        sub->setStyleSheet(QStringLiteral("font-size: %1px; color: %2;")
                               .arg(BookSimResultUi::TypePx::kBody)
                               .arg(BookSimResultUi::rgba(th.fgMain)));
        v->addWidget(sub);
    }

    if (extraWidget != nullptr) {
        v->addWidget(extraWidget);
    }

    auto* grid = new QGridLayout();
    grid->setHorizontalSpacing(16);
    grid->setVerticalSpacing(8);
    int r = 0;
    for (const BookSimMetricRow& row : rows) {
        auto* nameL = new QLabel(row.name, card);
        nameL->setStyleSheet(QStringLiteral("font-size: %1px; font-weight: 500; color: %2;")
                                 .arg(BookSimResultUi::TypePx::kBody)
                                 .arg(th.fgMain.name(QColor::HexRgb)));
        auto* valL = new QLabel(row.value, card);
        valL->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        valL->setStyleSheet(QStringLiteral("font-size: %1px; font-family: 'SF Mono', 'Menlo', "
                                           "'Consolas', monospace; color: %2;")
                                .arg(BookSimResultUi::TypePx::kMetricValue)
                                .arg(th.fgMain.name(QColor::HexRgb)));
        grid->addWidget(nameL, r, 0);
        grid->addWidget(valL, r, 1);
        ++r;
        if (!row.hint.isEmpty()) {
            auto* hintL = new QLabel(row.hint, card);
            hintL->setWordWrap(true);
            hintL->setStyleSheet(QStringLiteral("font-size: %1px; color: %2;")
                                     .arg(BookSimResultUi::TypePx::kBody)
                                     .arg(BookSimResultUi::rgba(th.fgMain)));
            grid->addWidget(hintL, r, 0, 1, 2);
            ++r;
        }
    }
    grid->setColumnStretch(1, 1);
    v->addLayout(grid);
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
    lay->addWidget(classHead);

    const int sampleNote = stats.packetLatency.hasAverage ? stats.packetLatency.samples : 0;
    if (sampleNote > 0) {
        auto* badge = new QLabel(tr("观测样本窗口（samples）: %1").arg(sampleNote), panel);
        badge->setStyleSheet(QStringLiteral("font-size: %1px; color: %2; padding: 4px 0;")
                                 .arg(BookSimResultUi::TypePx::kBody)
                                 .arg(BookSimResultUi::rgba(th.fgMain)));
        lay->addWidget(badge);
    }

    lay->addWidget(makeHeroKpiStrip(panel, stats, th));

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
                           tr("性能分析核心：先看分层延迟，再观察 Fragmentation 是否异常升高。"),
                           latencyRows,
                           makeLatencyLadder(panel, stats, th),
                           th.kpiPacket.name(QColor::HexRgb),
                           true));

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
    lay->addWidget(createCategoryCard(panel,
                                      tr("吞吐与接纳"),
                                      thrHint,
                                      thrRows,
                                      makeThroughputCompare(panel, stats, th),
                                      th.thrAccPkt.name(QColor::HexRgb),
                                      false));

    auto* detailRow = new QWidget(panel);
    detailRow->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    auto* detailLay = new QHBoxLayout(detailRow);
    detailLay->setContentsMargins(0, 0, 0, 0);
    detailLay->setSpacing(16);

    QVector<BookSimMetricRow> shapeRows;
    shapeRows.push_back({tr("注入平均包长（flits）"),
                         formatBand(stats.injectedPacketSize, 4),
                         tr("由统计口径折合成 flit 计数的平均包长。")});
    shapeRows.push_back({tr("接纳平均包长（flits）"),
                         formatBand(stats.acceptedPacketSize, 4),
                         tr("若与注入侧差异大，需留意测量边界或流量过滤。")});

    QVector<BookSimMetricRow> hopRows;
    hopRows.push_back({tr("平均跳数"),
                       formatBand(stats.hops, 5),
                       tr("与拓扑、路由函数共同决定最小可能延迟下界。")});

    detailLay->addWidget(createCategoryCard(detailRow,
                                            tr("数据包形态"),
                                            tr("对照配置中的 packet size 与有效载荷比例。"),
                                            shapeRows,
                                            nullptr,
                                            th.accentShape.name(QColor::HexRgb),
                                            false),
                         1);
    detailLay->addWidget(createCategoryCard(detailRow,
                                            tr("路径与拓扑"),
                                            QString(),
                                            hopRows,
                                            nullptr,
                                            th.accentHops.name(QColor::HexRgb),
                                            false),
                         1);
    lay->addWidget(detailRow);

    return panel;
}
