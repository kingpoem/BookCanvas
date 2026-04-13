#pragma once

#include <ElaDef.h>
#include <ElaTheme.h>
#include <QColor>
#include <QString>

namespace BookSimResultUi {

namespace TypePx {
constexpr int kBody = 13;
constexpr int kLead = 14;
constexpr int kSection = 16;
constexpr int kCardTitle = 18;
constexpr int kCardTitleEmph = 20;
constexpr int kStatus = 14;
constexpr int kMetricValue = 15;
} // namespace TypePx

[[nodiscard]] inline QColor mix(const QColor& a, const QColor& b, qreal t) {
    const qreal u = 1.0 - t;
    return QColor(qBound(0, qRound(a.red() * t + b.red() * u), 255),
                  qBound(0, qRound(a.green() * t + b.green() * u), 255),
                  qBound(0, qRound(a.blue() * t + b.blue() * u), 255),
                  qBound(0, qRound(a.alpha() * t + b.alpha() * u), 255));
}

[[nodiscard]] inline QString rgba(const QColor& c) {
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
    QColor fgMain;
    QColor fgDim;
};

[[nodiscard]] inline Theme themeFrom(ElaThemeType::ThemeMode mode) {
    Theme th;
    const QColor border = ElaThemeColor(mode, BasicBorder);
    th.border = border;

    if (mode == ElaThemeType::Light) {
        const QColor paper(0xFA, 0xFB, 0xFC);
        const QColor ink(0x1A, 0x1B, 0x1E);
        th.pageBg = paper;
        th.cardBg = QColor(0xFF, 0xFF, 0xFF);
        th.cardBgPrimary = QColor(0xFF, 0xFF, 0xFF);
        th.tileBg = QColor(0xF7, 0xF8, 0xFA);
        th.heroG0 = QColor(0xF7, 0xF9, 0xFC);
        th.heroG1 = QColor(0xFB, 0xFC, 0xFE);
        th.heroBorder = border;
        th.fgMain = ink;
        th.fgDim = QColor(0x4A, 0x4F, 0x57);
        th.textMuted = QColor(0x5B, 0x61, 0x6B);
        th.textHint = QColor(0x6A, 0x72, 0x7D);
        th.chartPacket = QColor(0x78, 0x99, 0xD6);
        th.chartNetwork = QColor(0x93, 0x9B, 0xD8);
        th.chartFlit = QColor(0x7F, 0xB2, 0xCF);
        th.thrInjPkt = QColor(0x99, 0xA6, 0xE0);
        th.thrAccPkt = QColor(0x86, 0xBC, 0xA4);
        th.thrInjFlit = QColor(0xB0, 0xA1, 0xD8);
        th.thrAccFlit = QColor(0x81, 0xC1, 0xC7);
        th.kpiPacket = th.chartPacket;
        th.kpiNetwork = th.chartNetwork;
        th.kpiHops = QColor(0x9A, 0x95, 0x8D);
        th.accentShape = QColor(0x9A, 0x95, 0x8D);
        th.accentHops = QColor(0xCA, 0xA6, 0x6A);
        th.statusBalanced = QColor(0x78, 0xB1, 0x92);
        th.statusWarn = QColor(0xCB, 0xAD, 0x78);
        th.statusBad = QColor(0xCC, 0x85, 0x85);
    } else {
        const QColor canvas(0x18, 0x1B, 0x20);
        const QColor surface(0x22, 0x26, 0x2D);
        th.pageBg = canvas;
        th.cardBg = QColor(0x1E, 0x22, 0x29);
        th.cardBgPrimary = QColor(0x21, 0x26, 0x2E);
        th.tileBg = mix(surface, canvas, 0.60);
        th.heroG0 = QColor(0x22, 0x27, 0x30);
        th.heroG1 = QColor(0x1C, 0x21, 0x29);
        th.heroBorder = border;
        th.fgMain = QColor(0xEB, 0xEE, 0xF3);
        const QColor lightGray(0xD9, 0xDE, 0xE6);
        th.textMuted = mix(ElaThemeColor(mode, BasicDetailsText), lightGray, 0.30);
        th.textHint = mix(ElaThemeColor(mode, BasicTextNoFocus), lightGray, 0.25);
        th.fgDim = th.textMuted;
        th.chartPacket = QColor(0x8E, 0xB1, 0xE8);
        th.chartNetwork = QColor(0xAA, 0xAF, 0xE4);
        th.chartFlit = QColor(0x92, 0xC6, 0xD8);
        th.thrInjPkt = QColor(0xB2, 0xBD, 0xE8);
        th.thrAccPkt = QColor(0x92, 0xC7, 0xAF);
        th.thrInjFlit = QColor(0xC1, 0xB6, 0xDE);
        th.thrAccFlit = QColor(0x96, 0xCD, 0xCF);
        th.kpiPacket = th.chartPacket;
        th.kpiNetwork = th.chartNetwork;
        th.kpiHops = QColor(0xB6, 0xB1, 0xAB);
        th.accentShape = QColor(0xB6, 0xB1, 0xAB);
        th.accentHops = QColor(0xD7, 0xB4, 0x82);
        th.statusBalanced = QColor(0x89, 0xC8, 0xA9);
        th.statusWarn = QColor(0xD7, 0xBC, 0x8B);
        th.statusBad = QColor(0xD8, 0x9B, 0x9B);
    }
    return th;
}

} // namespace BookSimResultUi
