#include "SimulationRecordPage.h"
#include <ElaComboBox.h>
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
#include <QMessageBox>
#include <QPalette>
#include <QScrollArea>
#include <QSet>
#include <QSignalBlocker>
#include <QVBoxLayout>
#include <QVector>
#include <algorithm>

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

enum class RecordSortMode {
    UpdatedDesc = 0,
    UpdatedAsc,
    PacketLatencyAsc,
    PacketLatencyDesc,
    ThroughputMatchDesc,
    RunTimeDesc,
};

enum class LatencyFilterMode {
    All = 0,
    Low,
    Medium,
    High,
    Unknown,
};

double compareValue(double value, bool ascending) {
    if (value < 0.0) {
        return ascending ? 1e12 : -1e12;
    }
    return value;
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
    m_searchEdit->setPlaceholderText(tr("搜索记录名/拓扑/路由/注入率/延迟/吞吐/运行时长..."));
    toolbarLay->addWidget(m_searchEdit, 1);

    m_topologyFilter = new ElaComboBox(this);
    m_topologyFilter->addItem(tr("全部拓扑"), QStringLiteral("__all__"));
    toolbarLay->addWidget(m_topologyFilter);

    m_latencyFilter = new ElaComboBox(this);
    m_latencyFilter->addItem(tr("全部延迟"), static_cast<int>(LatencyFilterMode::All));
    m_latencyFilter->addItem(tr("低延迟(<20)"), static_cast<int>(LatencyFilterMode::Low));
    m_latencyFilter->addItem(tr("中延迟(20~60)"), static_cast<int>(LatencyFilterMode::Medium));
    m_latencyFilter->addItem(tr("高延迟(>=60)"), static_cast<int>(LatencyFilterMode::High));
    m_latencyFilter->addItem(tr("延迟未知"), static_cast<int>(LatencyFilterMode::Unknown));
    toolbarLay->addWidget(m_latencyFilter);

    m_sortCombo = new ElaComboBox(this);
    m_sortCombo->addItem(tr("按更新时间(新->旧)"), static_cast<int>(RecordSortMode::UpdatedDesc));
    m_sortCombo->addItem(tr("按更新时间(旧->新)"), static_cast<int>(RecordSortMode::UpdatedAsc));
    m_sortCombo->addItem(tr("按平均延迟(低->高)"),
                         static_cast<int>(RecordSortMode::PacketLatencyAsc));
    m_sortCombo->addItem(tr("按平均延迟(高->低)"),
                         static_cast<int>(RecordSortMode::PacketLatencyDesc));
    m_sortCombo->addItem(tr("按吞吐匹配(高->低)"),
                         static_cast<int>(RecordSortMode::ThroughputMatchDesc));
    m_sortCombo->addItem(tr("按运行时长(长->短)"), static_cast<int>(RecordSortMode::RunTimeDesc));
    toolbarLay->addWidget(m_sortCombo);

    auto* clearAllBtn = new ElaPushButton(tr("一键清除"), this);
    toolbarLay->addWidget(clearAllBtn);

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
    connect(m_topologyFilter,
            QOverload<int>::of(&ElaComboBox::currentIndexChanged),
            this,
            [this](int) { rebuildCards(); });
    connect(m_latencyFilter,
            QOverload<int>::of(&ElaComboBox::currentIndexChanged),
            this,
            [this](int) { rebuildCards(); });
    connect(m_sortCombo, QOverload<int>::of(&ElaComboBox::currentIndexChanged), this, [this](int) {
        rebuildCards();
    });
    connect(clearAllBtn, &ElaPushButton::clicked, this, [this]() {
        if (m_records.isEmpty()) {
            return;
        }
        const auto reply = QMessageBox::question(this,
                                                 tr("确认清除"),
                                                 tr("确定要清除全部仿真记录吗？该操作不可撤销。"),
                                                 QMessageBox::Yes | QMessageBox::No,
                                                 QMessageBox::No);
        if (reply != QMessageBox::Yes) {
            return;
        }
        m_records.clear();
        persistRecords();
        rebuildCards();
    });
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
    if (record.config.value(QStringLiteral("num_vcs")).toLower().contains(k)) {
        return true;
    }
    if (record.config.value(QStringLiteral("k")).toLower().contains(k)
        || record.config.value(QStringLiteral("n")).toLower().contains(k)
        || record.config.value(QStringLiteral("c")).toLower().contains(k)) {
        return true;
    }

    const QString packet = metricLabel(tr("Packet"), record.packetLatencyAvg).toLower();
    const QString network = metricLabel(tr("Network"), record.networkLatencyAvg).toLower();
    const QString flit = metricLabel(tr("Flit"), record.flitLatencyAvg).toLower();
    if (packet.contains(k) || network.contains(k) || flit.contains(k)) {
        return true;
    }
    if (record.packetLatencyAvg >= 0.0
        && QString::number(record.packetLatencyAvg, 'f', 3).contains(kw)) {
        return true;
    }
    if (record.throughputMatchPercent >= 0.0
        && QString::number(record.throughputMatchPercent, 'f', 1).contains(kw)) {
        return true;
    }
    if (record.totalRunTimeSec >= 0.0
        && QString::number(record.totalRunTimeSec, 'f', 3).contains(kw)) {
        return true;
    }
    return false;
}

bool SimulationRecordPage::matchesFilters(const SimulationRecordSnapshot& record) const {
    const QString selectedTopology = m_topologyFilter ? m_topologyFilter->currentData().toString()
                                                      : QStringLiteral("__all__");
    if (!selectedTopology.isEmpty() && selectedTopology != QStringLiteral("__all__")
        && record.config.value(QStringLiteral("topology")) != selectedTopology) {
        return false;
    }

    const auto latencyMode = static_cast<LatencyFilterMode>(
        m_latencyFilter ? m_latencyFilter->currentData().toInt()
                        : static_cast<int>(LatencyFilterMode::All));
    const double latency = record.packetLatencyAvg;
    switch (latencyMode) {
    case LatencyFilterMode::All:
        return true;
    case LatencyFilterMode::Low:
        return latency >= 0.0 && latency < 20.0;
    case LatencyFilterMode::Medium:
        return latency >= 20.0 && latency < 60.0;
    case LatencyFilterMode::High:
        return latency >= 60.0;
    case LatencyFilterMode::Unknown:
        return latency < 0.0;
    }
    return true;
}

int SimulationRecordPage::findRecordIndexById(const QString& id) const {
    for (int i = 0; i < m_records.size(); ++i) {
        if (m_records[i].id == id) {
            return i;
        }
    }
    return -1;
}

QWidget* SimulationRecordPage::buildRecordCard(const SimulationRecordSnapshot& record) {
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

    auto* deleteBtn = new ElaPushButton(tr("删除"), card);
    auto* renameBtn = new ElaPushButton(tr("重命名"), card);
    auto* showBtn = new ElaPushButton(tr("在结果页显示"), card);
    titleRow->addWidget(deleteBtn);
    titleRow->addWidget(renameBtn);
    titleRow->addWidget(showBtn);
    layout->addLayout(titleRow);

    auto* time = new QLabel(tr("创建: %1 | 更新: %2")
                                .arg(formatIsoLocal(record.createdAtIso),
                                     formatIsoLocal(record.updatedAtIso)),
                            card);
    time->setWordWrap(true);
    layout->addWidget(time);

    auto* tagsWrap = new QWidget(card);
    auto* tagsLayout = new QHBoxLayout(tagsWrap);
    tagsLayout->setContentsMargins(0, 0, 0, 0);
    tagsLayout->setSpacing(6);

    const auto addTag = [card, tagsLayout](const QString& text) {
        auto* tag = new QLabel(text, card);
        tag->setProperty("isRecordTag", true);
        tagsLayout->addWidget(tag);
    };

    const QString topology = record.config.value(QStringLiteral("topology"), QStringLiteral("-"));
    const QString routing = record.config.value(QStringLiteral("routing_function"),
                                                QStringLiteral("-"));
    const QString inj = record.config.value(QStringLiteral("injection_rate"), QStringLiteral("-"));
    addTag(tr("拓扑: %1").arg(topology));
    addTag(tr("路由: %1").arg(routing));
    addTag(tr("注入率: %1").arg(inj));
    addTag(record.packetLatencyAvg >= 0.0
               ? tr("Packet延迟: %1").arg(QString::number(record.packetLatencyAvg, 'f', 3))
               : tr("Packet延迟: --"));
    addTag(record.networkLatencyAvg >= 0.0
               ? tr("Network延迟: %1").arg(QString::number(record.networkLatencyAvg, 'f', 3))
               : tr("Network延迟: --"));
    addTag(record.flitLatencyAvg >= 0.0
               ? tr("Flit延迟: %1").arg(QString::number(record.flitLatencyAvg, 'f', 3))
               : tr("Flit延迟: --"));
    addTag(record.throughputMatchPercent >= 0.0
               ? tr("吞吐匹配: %1%").arg(QString::number(record.throughputMatchPercent, 'f', 1))
               : tr("吞吐匹配: --"));
    addTag(record.totalRunTimeSec >= 0.0
               ? tr("运行时长: %1s").arg(QString::number(record.totalRunTimeSec, 'f', 3))
               : tr("运行时长: --"));
    addTag(tr("Traffic Class: %1").arg(record.trafficClassCount));
    tagsLayout->addStretch();
    layout->addWidget(tagsWrap);

    auto* classes = new QLabel(tr("Traffic Class 数: %1").arg(record.trafficClassCount), card);
    classes->setWordWrap(true);
    layout->addWidget(classes);

    connect(showBtn, &ElaPushButton::clicked, this, [this, record]() {
        emit showRecordInResultRequested(record.simulationLog);
        m_statusText->setText(tr("已将记录载入结果可视化页，请切换到“BookSim 结果”查看。"));
    });
    connect(deleteBtn, &ElaPushButton::clicked, this, [this, record]() {
        const int index = findRecordIndexById(record.id);
        if (index < 0 || index >= m_records.size()) {
            return;
        }
        const auto reply
            = QMessageBox::question(this,
                                    tr("删除记录"),
                                    tr("确定删除记录“%1”吗？").arg(m_records[index].name),
                                    QMessageBox::Yes | QMessageBox::No,
                                    QMessageBox::No);
        if (reply != QMessageBox::Yes) {
            return;
        }
        m_records.removeAt(index);
        persistRecords();
        rebuildCards();
    });
    connect(renameBtn, &ElaPushButton::clicked, this, [this, record]() {
        const int index = findRecordIndexById(record.id);
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

void SimulationRecordPage::refreshTopologyFilterOptions() {
    if (!m_topologyFilter) {
        return;
    }

    const QString previousSelection = m_topologyFilter->currentData().toString();
    QSet<QString> topologies;
    for (const auto& record : m_records) {
        const QString topology = record.config.value(QStringLiteral("topology")).trimmed();
        if (!topology.isEmpty()) {
            topologies.insert(topology);
        }
    }

    QStringList sorted = topologies.values();
    std::sort(sorted.begin(), sorted.end());

    QSignalBlocker blocker(m_topologyFilter);
    m_topologyFilter->clear();
    m_topologyFilter->addItem(tr("全部拓扑"), QStringLiteral("__all__"));
    for (const QString& topology : sorted) {
        m_topologyFilter->addItem(topology, topology);
    }
    const int target = std::max(0, m_topologyFilter->findData(previousSelection));
    m_topologyFilter->setCurrentIndex(target);
}

void SimulationRecordPage::rebuildCards() {
    while (QLayoutItem* item = m_cardsLayout->takeAt(0)) {
        delete item->widget();
        delete item;
    }

    refreshTopologyFilterOptions();

    QVector<int> visibleIndices;
    visibleIndices.reserve(m_records.size());
    for (int i = 0; i < m_records.size(); ++i) {
        const auto& record = m_records.at(i);
        if (!matchesKeyword(record) || !matchesFilters(record)) {
            continue;
        }
        visibleIndices.push_back(i);
    }

    const auto sortMode = static_cast<RecordSortMode>(
        m_sortCombo ? m_sortCombo->currentData().toInt()
                    : static_cast<int>(RecordSortMode::UpdatedDesc));
    std::sort(visibleIndices.begin(), visibleIndices.end(), [this, sortMode](int lhs, int rhs) {
        const auto& a = m_records.at(lhs);
        const auto& b = m_records.at(rhs);
        const QDateTime aTime = QDateTime::fromString(a.updatedAtIso, Qt::ISODate);
        const QDateTime bTime = QDateTime::fromString(b.updatedAtIso, Qt::ISODate);
        switch (sortMode) {
        case RecordSortMode::UpdatedDesc:
            return aTime > bTime;
        case RecordSortMode::UpdatedAsc:
            return aTime < bTime;
        case RecordSortMode::PacketLatencyAsc:
            return compareValue(a.packetLatencyAvg, true) < compareValue(b.packetLatencyAvg, true);
        case RecordSortMode::PacketLatencyDesc:
            return compareValue(a.packetLatencyAvg, false)
                   > compareValue(b.packetLatencyAvg, false);
        case RecordSortMode::ThroughputMatchDesc:
            return compareValue(a.throughputMatchPercent, false)
                   > compareValue(b.throughputMatchPercent, false);
        case RecordSortMode::RunTimeDesc:
            return compareValue(a.totalRunTimeSec, false) > compareValue(b.totalRunTimeSec, false);
        }
        return true;
    });

    for (int idx : visibleIndices) {
        m_cardsLayout->addWidget(buildRecordCard(m_records.at(idx)));
    }
    m_cardsLayout->addStretch();

    if (m_records.isEmpty()) {
        m_statusText->setText(tr("暂无仿真记录。每次仿真完成后会自动追加并持久化保存。"));
    } else if (visibleIndices.isEmpty()) {
        m_statusText->setText(tr("没有匹配项。可调整搜索词、筛选条件或排序方式。"));
    } else {
        m_statusText->setText(
            tr("共 %1 条记录，当前匹配 %2 条。").arg(m_records.size()).arg(visibleIndices.size()));
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
                           "border-radius: 12px; } QLabel { color: %3; } "
                           "QLabel[isRecordTag=\"true\"] { background: rgba(127, 127, 127, 0.15); "
                           "border: 1px solid %2; border-radius: 8px; padding: 2px 8px; }")
                .arg(panelBg.name(QColor::HexRgb),
                     border.name(QColor::HexRgb),
                     text.name(QColor::HexRgb)));
    }
    if (m_statusText) {
        m_statusText->setStyleSheet(QStringLiteral("color: %1;").arg(textSub.name(QColor::HexRgb)));
    }
}
