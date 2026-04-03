#include "SimulationRecordPage.h"
#include <ElaLineEdit.h>
#include <ElaPushButton.h>
#include <ElaScrollPageArea.h>
#include <ElaText.h>
#include <ElaTheme.h>
#include <QDateTime>
#include <QFrame>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPalette>
#include <QScrollArea>
#include <QVBoxLayout>

namespace {

QString formatIsoLocal(const QString& iso) {
    const QDateTime dt = QDateTime::fromString(iso, Qt::ISODate);
    if (!dt.isValid()) {
        return iso;
    }
    return dt.toLocalTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"));
}

QString metricLabel(const QString& name, double value, int precision = 3) {
    if (value < 0.0) {
        return QStringLiteral("%1: --").arg(name);
    }
    return QStringLiteral("%1: %2").arg(name, QString::number(value, 'f', precision));
}

} // namespace

SimulationRecordPage::SimulationRecordPage(QWidget* parent)
    : BasePage(parent) {
    setWindowTitle(tr("仿真记录"));

    auto* central = new QWidget(this);
    m_pageRoot = central;
    central->setWindowTitle(tr("仿真记录"));
    auto* root = new QVBoxLayout(central);
    root->setContentsMargins(0, 0, 20, 0);
    root->setSpacing(10);

    auto* toolbar = new ElaScrollPageArea(this);
    auto* toolbarLay = new QHBoxLayout(toolbar);
    toolbarLay->setContentsMargins(0, 0, 0, 0);
    toolbarLay->setSpacing(8);

    m_searchEdit = new ElaLineEdit(this);
    m_searchEdit->setPlaceholderText(tr("搜索记录名/拓扑/路由函数/注入率..."));
    toolbarLay->addWidget(m_searchEdit, 1);

    m_statusText = new ElaText(tr("正在加载仿真记录..."), this);
    m_statusText->setWordWrap(true);
    m_statusText->setTextPixelSize(14);

    m_scrollInner = new QWidget(this);
    m_cardsLayout = new QVBoxLayout(m_scrollInner);
    m_cardsLayout->setContentsMargins(0, 0, 0, 0);
    m_cardsLayout->setSpacing(12);

    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setWidget(m_scrollInner);

    root->addWidget(toolbar);
    root->addWidget(m_statusText);
    root->addWidget(m_scrollArea, 1);

    connect(m_searchEdit, &ElaLineEdit::textChanged, this, [this]() { rebuildCards(); });
    connect(eTheme, &ElaTheme::themeModeChanged, this, [this](ElaThemeType::ThemeMode) {
        applyTheme();
        rebuildCards();
    });

    addCentralWidget(central, true, true, 0);
    reloadRecords();
    applyTheme();
    rebuildCards();
}

void SimulationRecordPage::appendRecord(const QString& simulationLog,
                                        const QMap<QString, QString>& config) {
    const auto record = SimulationRecordStore::makeRecord(simulationLog, config);
    m_records.prepend(record);
    persistRecords();
    rebuildCards();
}

void SimulationRecordPage::reloadRecords() {
    m_records = SimulationRecordStore::loadRecords();
}

void SimulationRecordPage::persistRecords() {
    QString saveError;
    if (!SimulationRecordStore::saveRecords(m_records, &saveError)) {
        m_statusText->setText(tr("记录已更新，但保存失败：%1").arg(saveError));
    }
}

bool SimulationRecordPage::matchesKeyword(const SimulationRecordSnapshot& record) const {
    const QString kw = m_searchEdit ? m_searchEdit->text().trimmed() : QString();
    if (kw.isEmpty()) {
        return true;
    }

    const QString k = kw.toLower();
    if (record.name.toLower().contains(k)) {
        return true;
    }
    if (record.config.value(QStringLiteral("topology")).toLower().contains(k)) {
        return true;
    }
    if (record.config.value(QStringLiteral("routing_function")).toLower().contains(k)) {
        return true;
    }
    if (record.config.value(QStringLiteral("injection_rate")).toLower().contains(k)) {
        return true;
    }
    return false;
}

QString SimulationRecordPage::buildConfigSummary(const QMap<QString, QString>& cfg) const {
    const QString topology = cfg.value(QStringLiteral("topology"), QStringLiteral("-"));
    const QString routing = cfg.value(QStringLiteral("routing_function"), QStringLiteral("-"));
    const QString k = cfg.value(QStringLiteral("k"),
                                cfg.value(QStringLiteral("x"), QStringLiteral("-")));
    const QString n = cfg.value(QStringLiteral("n"),
                                cfg.value(QStringLiteral("y"), QStringLiteral("-")));
    const QString c = cfg.value(QStringLiteral("c"),
                                cfg.value(QStringLiteral("xr"), QStringLiteral("-")));
    const QString vcs = cfg.value(QStringLiteral("num_vcs"), QStringLiteral("-"));
    const QString inj = cfg.value(QStringLiteral("injection_rate"), QStringLiteral("-"));
    return tr("拓扑 %1 | k=%2 n=%3 c=%4 | routing=%5 | num_vcs=%6 | injection_rate=%7")
        .arg(topology, k, n, c, routing, vcs, inj);
}

QString SimulationRecordPage::buildMetricSummary(const SimulationRecordSnapshot& record) const {
    QStringList parts;
    parts << metricLabel(tr("Packet"), record.packetLatencyAvg)
          << metricLabel(tr("Network"), record.networkLatencyAvg)
          << metricLabel(tr("Flit"), record.flitLatencyAvg);

    if (record.throughputMatchPercent >= 0.0) {
        parts << tr("吞吐匹配: %1%").arg(QString::number(record.throughputMatchPercent, 'f', 1));
    } else {
        parts << tr("吞吐匹配: --");
    }
    if (record.totalRunTimeSec >= 0.0) {
        parts << tr("运行时长: %1 s").arg(QString::number(record.totalRunTimeSec, 'f', 3));
    } else {
        parts << tr("运行时长: --");
    }
    return parts.join(QStringLiteral(" | "));
}

QWidget* SimulationRecordPage::buildRecordCard(const SimulationRecordSnapshot& record, int index) {
    auto* card = new QFrame(m_scrollInner);
    card->setObjectName(QStringLiteral("SimulationRecordCard"));
    card->setProperty("isRecordCard", true);

    auto* layout = new QVBoxLayout(card);
    layout->setContentsMargins(16, 14, 16, 14);
    layout->setSpacing(8);

    auto* titleRow = new QHBoxLayout();
    auto* title = new QLabel(record.name, card);
    QFont titleFont = title->font();
    titleFont.setBold(true);
    titleFont.setPointSize(titleFont.pointSize() + 1);
    title->setFont(titleFont);
    titleRow->addWidget(title, 1);

    auto* renameBtn = new ElaPushButton(tr("重命名"), card);
    auto* showBtn = new ElaPushButton(tr("在结果页显示"), card);
    titleRow->addWidget(renameBtn);
    titleRow->addWidget(showBtn);
    layout->addLayout(titleRow);

    auto* time = new QLabel(tr("创建: %1 | 更新: %2")
                                .arg(formatIsoLocal(record.createdAtIso),
                                     formatIsoLocal(record.updatedAtIso)),
                            card);
    time->setWordWrap(true);
    layout->addWidget(time);

    auto* cfg = new QLabel(buildConfigSummary(record.config), card);
    cfg->setWordWrap(true);
    layout->addWidget(cfg);

    auto* metrics = new QLabel(buildMetricSummary(record), card);
    metrics->setWordWrap(true);
    layout->addWidget(metrics);

    auto* classes = new QLabel(tr("Traffic Class 数: %1").arg(record.trafficClassCount), card);
    classes->setWordWrap(true);
    layout->addWidget(classes);

    connect(showBtn, &ElaPushButton::clicked, this, [this, record]() {
        emit showRecordInResultRequested(record.simulationLog);
        m_statusText->setText(tr("已将记录载入结果可视化页，请切换到“BookSim 结果”查看。"));
    });
    connect(renameBtn, &ElaPushButton::clicked, this, [this, index]() {
        if (index < 0 || index >= m_records.size()) {
            return;
        }
        bool ok = false;
        const QString newName = QInputDialog::getText(this,
                                                      tr("重命名记录"),
                                                      tr("新的记录名称"),
                                                      QLineEdit::Normal,
                                                      m_records[index].name,
                                                      &ok)
                                    .trimmed();
        if (!ok || newName.isEmpty()) {
            return;
        }
        m_records[index].name = newName;
        m_records[index].updatedAtIso = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
        persistRecords();
        rebuildCards();
    });
    return card;
}

void SimulationRecordPage::rebuildCards() {
    while (QLayoutItem* item = m_cardsLayout->takeAt(0)) {
        if (QWidget* w = item->widget()) {
            delete w;
        }
        delete item;
    }

    int matched = 0;
    for (int i = 0; i < m_records.size(); ++i) {
        const auto& record = m_records[i];
        if (!matchesKeyword(record)) {
            continue;
        }
        ++matched;
        m_cardsLayout->addWidget(buildRecordCard(record, i));
    }
    m_cardsLayout->addStretch();

    if (m_records.isEmpty()) {
        m_statusText->setText(tr("暂无仿真记录。每次仿真完成后会自动追加并持久化保存。"));
    } else if (matched == 0) {
        m_statusText->setText(tr("没有匹配项。可按记录名、拓扑、路由函数、注入率搜索。"));
    } else {
        m_statusText->setText(
            tr("共 %1 条记录，当前匹配 %2 条。").arg(m_records.size()).arg(matched));
    }
}

void SimulationRecordPage::applyTheme() {
    if (!m_pageRoot) {
        return;
    }
    const auto mode = eTheme->getThemeMode();
    const QColor pageBg = ElaThemeColor(mode, WindowBase);
    const QColor panelBg = ElaThemeColor(mode, PopupBase);
    const QColor border = ElaThemeColor(mode, BasicBorder);
    const QColor text = ElaThemeColor(mode, BasicText);
    const QColor textSub = ElaThemeColor(mode, BasicTextNoFocus);

    m_pageRoot->setAttribute(Qt::WA_StyledBackground, true);
    m_pageRoot->setStyleSheet(QStringLiteral("#ElaScrollPage_CentralPage { background-color: %1; }")
                                  .arg(pageBg.name(QColor::HexRgb)));
    if (QWidget* outerVp = m_pageRoot->parentWidget()) {
        outerVp->setAutoFillBackground(true);
        QPalette op = outerVp->palette();
        op.setColor(QPalette::Window, pageBg);
        outerVp->setPalette(op);
    }

    if (m_scrollArea) {
        m_scrollArea->setStyleSheet(
            QStringLiteral("QScrollArea { background: transparent; border: none; }"));
    }
    if (m_scrollArea && m_scrollArea->viewport()) {
        m_scrollArea->viewport()->setStyleSheet(QStringLiteral("background: transparent;"));
    }
    if (m_scrollInner) {
        m_scrollInner->setStyleSheet(
            QStringLiteral("QFrame[isRecordCard=\"true\"] { background: %1; border: 1px solid %2; "
                           "border-radius: 12px; } QLabel { color: %3; }")
                .arg(panelBg.name(QColor::HexRgb),
                     border.name(QColor::HexRgb),
                     text.name(QColor::HexRgb)));
    }
    if (m_statusText) {
        m_statusText->setStyleSheet(QStringLiteral("color: %1;").arg(textSub.name(QColor::HexRgb)));
    }
}
