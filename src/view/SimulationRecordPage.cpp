#include "SimulationRecordPage.h"
#include <ElaComboBox.h>
#include <ElaLineEdit.h>
#include <ElaPushButton.h>
#include <ElaScrollPageArea.h>
#include <ElaText.h>
#include <ElaTheme.h>
#include <QDateTime>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
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
    LabelValueAsc,
    LabelValueDesc,
};

enum class NumericFilterMode {
    All = 0,
    GreaterThan,
    LessThan,
    Between,
};

double compareValue(double value, bool ascending) {
    if (value < 0.0) {
        return ascending ? 1e12 : -1e12;
    }
    return value;
}

bool tryParseDouble(const QString& text, double* outValue) {
    bool ok = false;
    const double v = text.trimmed().toDouble(&ok);
    if (!ok) {
        return false;
    }
    if (outValue) {
        *outValue = v;
    }
    return true;
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
    m_searchEdit->setPlaceholderText(tr("搜索记录名/拓扑/k,n,c/路由/注入率/延迟/吞吐/运行时长..."));
    toolbarLay->addWidget(m_searchEdit, 1);

    m_numericLabelCombo = new ElaComboBox(this);
    m_numericLabelCombo->setMaximumWidth(180);
    toolbarLay->addWidget(m_numericLabelCombo);

    m_numericFilterMode = new ElaComboBox(this);
    m_numericFilterMode->addItem(tr("不限数值"), static_cast<int>(NumericFilterMode::All));
    m_numericFilterMode->addItem(tr("大于"), static_cast<int>(NumericFilterMode::GreaterThan));
    m_numericFilterMode->addItem(tr("小于"), static_cast<int>(NumericFilterMode::LessThan));
    m_numericFilterMode->addItem(tr("区间"), static_cast<int>(NumericFilterMode::Between));
    toolbarLay->addWidget(m_numericFilterMode);

    m_rangeMinEdit = new ElaLineEdit(this);
    m_rangeMinEdit->setPlaceholderText(tr("输入下限"));
    m_rangeMinEdit->setFixedWidth(96);
    toolbarLay->addWidget(m_rangeMinEdit);

    m_rangeMaxEdit = new ElaLineEdit(this);
    m_rangeMaxEdit->setPlaceholderText(tr("输入上限"));
    m_rangeMaxEdit->setFixedWidth(96);
    toolbarLay->addWidget(m_rangeMaxEdit);

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
    m_sortCombo->addItem(tr("按选定标签数值(低->高)"),
                         static_cast<int>(RecordSortMode::LabelValueAsc));
    m_sortCombo->addItem(tr("按选定标签数值(高->低)"),
                         static_cast<int>(RecordSortMode::LabelValueDesc));
    toolbarLay->addWidget(m_sortCombo);

    auto* resetFilterBtn = new ElaPushButton(tr("重置筛选"), this);
    toolbarLay->addWidget(resetFilterBtn);

    auto* clearAllBtn = new ElaPushButton(tr("清空记录"), this);
    toolbarLay->addWidget(clearAllBtn);

    auto* createLineChartBtn = new ElaPushButton(tr("创建折线图"), this);
    toolbarLay->addWidget(createLineChartBtn);

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
    connect(m_numericFilterMode,
            QOverload<int>::of(&ElaComboBox::currentIndexChanged),
            this,
            [this](int) {
                updateNumericFilterEditors();
                rebuildCards();
            });
    connect(m_sortCombo, QOverload<int>::of(&ElaComboBox::currentIndexChanged), this, [this](int) {
        rebuildCards();
    });
    connect(m_numericLabelCombo,
            QOverload<int>::of(&ElaComboBox::currentIndexChanged),
            this,
            [this](int) { rebuildCards(); });
    connect(m_rangeMinEdit, &ElaLineEdit::textChanged, this, [this]() { rebuildCards(); });
    connect(m_rangeMaxEdit, &ElaLineEdit::textChanged, this, [this]() { rebuildCards(); });
    connect(resetFilterBtn, &ElaPushButton::clicked, this, [this]() {
        if (m_searchEdit) {
            m_searchEdit->clear();
        }
        if (m_numericFilterMode) {
            m_numericFilterMode->setCurrentIndex(0);
        }
        if (m_rangeMinEdit) {
            m_rangeMinEdit->clear();
        }
        if (m_rangeMaxEdit) {
            m_rangeMaxEdit->clear();
        }
        updateNumericFilterEditors();
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
    connect(createLineChartBtn, &ElaPushButton::clicked, this, [this]() { openLineChartDialog(); });
    connect(eTheme, &ElaTheme::themeModeChanged, this, [this](ElaThemeType::ThemeMode) {
        applyTheme();
        rebuildCards();
    });

    addCentralWidget(central, true, true, 0);
    reloadRecords();
    applyTheme();
    updateNumericFilterEditors();
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
    const QString topologyVal = record.config.value(QStringLiteral("topology"), QStringLiteral("-"));
    if (topologyVal.toLower().contains(k) || tr("拓扑: %1").arg(topologyVal).toLower().contains(k)) {
        return true;
    }
    if (record.config.value(QStringLiteral("routing_function")).toLower().contains(k)) {
        return true;
    }
    {
        const QString kDim = record.config.value(QStringLiteral("k"), QStringLiteral("-"));
        const QString nDim = record.config.value(QStringLiteral("n"), QStringLiteral("-"));
        const QString cDim = record.config.value(QStringLiteral("c"), QStringLiteral("-"));
        const QString routingVal = record.config.value(QStringLiteral("routing_function"),
                                                       QStringLiteral("-"));
        if (tr("k: %1").arg(kDim).toLower().contains(k)
            || tr("n: %1").arg(nDim).toLower().contains(k)
            || tr("c: %1").arg(cDim).toLower().contains(k)
            || tr("路由: %1").arg(routingVal).toLower().contains(k)) {
            return true;
        }
    }
    {
        const QString inj = record.config.value(QStringLiteral("injection_rate"),
                                                QStringLiteral("-"));
        const QString nvc = record.config.value(QStringLiteral("num_vcs"), QStringLiteral("-"));
        if (tr("注入率: %1").arg(inj).toLower().contains(k)
            || tr("VC: %1").arg(nvc).toLower().contains(k)) {
            return true;
        }
    }
    if (record.config.value(QStringLiteral("injection_rate")).toLower().contains(k)) {
        return true;
    }
    if (record.config.value(QStringLiteral("num_vcs")).toLower().contains(k)) {
        return true;
    }
    if (tr("网络参数").toLower().contains(k) || tr("仿真结果").toLower().contains(k)) {
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
    if (record.throughputMatchPercent >= 0.0) {
        const QString thrLabel = tr("吞吐匹配: %1%")
                                     .arg(QString::number(record.throughputMatchPercent, 'f', 1))
                                     .toLower();
        if (thrLabel.contains(k)
            || QString::number(record.throughputMatchPercent, 'f', 1).contains(kw)) {
            return true;
        }
    }
    if (record.totalRunTimeSec >= 0.0) {
        const QString rtLabel
            = tr("运行时长: %1s").arg(QString::number(record.totalRunTimeSec, 'f', 3)).toLower();
        if (rtLabel.contains(k) || QString::number(record.totalRunTimeSec, 'f', 3).contains(kw)) {
            return true;
        }
    }
    if (tr("Traffic Class: %1").arg(record.trafficClassCount).toLower().contains(k)) {
        return true;
    }
    return false;
}

bool SimulationRecordPage::matchesFilters(const SimulationRecordSnapshot& record) const {
    if (hasNumericFilter()) {
        const QString labelKey = m_numericLabelCombo ? m_numericLabelCombo->currentData().toString()
                                                     : QStringLiteral("packetLatencyAvg");
        const double value = numericLabelValue(record, labelKey);
        if (value < 0.0) {
            return false;
        }

        double minValue = 0.0;
        double maxValue = 0.0;
        const bool hasMin = m_rangeMinEdit && tryParseDouble(m_rangeMinEdit->text(), &minValue);
        const bool hasMax = m_rangeMaxEdit && tryParseDouble(m_rangeMaxEdit->text(), &maxValue);
        const auto filterMode = static_cast<NumericFilterMode>(
            m_numericFilterMode ? m_numericFilterMode->currentData().toInt()
                                : static_cast<int>(NumericFilterMode::All));

        switch (filterMode) {
        case NumericFilterMode::All:
            break;
        case NumericFilterMode::GreaterThan:
            if (hasMin && value <= minValue) {
                return false;
            }
            break;
        case NumericFilterMode::LessThan:
            if (hasMax && value >= maxValue) {
                return false;
            }
            break;
        case NumericFilterMode::Between:
            if (hasMin && hasMax) {
                const double lo = std::min(minValue, maxValue);
                const double hi = std::max(minValue, maxValue);
                if (value < lo || value > hi) {
                    return false;
                }
            } else {
                if (hasMin && value < minValue) {
                    return false;
                }
                if (hasMax && value > maxValue) {
                    return false;
                }
            }
            break;
        }
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

    const auto addTag = [card](QHBoxLayout* row, const QString& text) {
        auto* tag = new QLabel(text, card);
        tag->setProperty("isRecordTag", true);
        row->addWidget(tag);
    };

    auto* netHeading = new QLabel(tr("网络参数"), card);
    QFont headingFont = netHeading->font();
    headingFont.setBold(true);
    headingFont.setPointSize(qMax(8, headingFont.pointSize() - 1));
    netHeading->setFont(headingFont);
    layout->addWidget(netHeading);

    auto* netRowWrap = new QWidget(card);
    auto* netRow = new QHBoxLayout(netRowWrap);
    netRow->setContentsMargins(0, 0, 0, 0);
    netRow->setSpacing(6);

    const QString topology = record.config.value(QStringLiteral("topology"), QStringLiteral("-"));
    const QString kDim = record.config.value(QStringLiteral("k"), QStringLiteral("-"));
    const QString nDim = record.config.value(QStringLiteral("n"), QStringLiteral("-"));
    const QString cDim = record.config.value(QStringLiteral("c"), QStringLiteral("-"));
    const QString routing = record.config.value(QStringLiteral("routing_function"),
                                                QStringLiteral("-"));
    const QString inj = record.config.value(QStringLiteral("injection_rate"), QStringLiteral("-"));
    const QString numVc = record.config.value(QStringLiteral("num_vcs"), QStringLiteral("-"));
    addTag(netRow, tr("拓扑: %1").arg(topology));
    addTag(netRow, tr("k: %1").arg(kDim));
    addTag(netRow, tr("n: %1").arg(nDim));
    addTag(netRow, tr("c: %1").arg(cDim));
    addTag(netRow, tr("路由: %1").arg(routing));
    addTag(netRow, tr("注入率: %1").arg(inj));
    addTag(netRow, tr("VC: %1").arg(numVc));
    netRow->addStretch();
    layout->addWidget(netRowWrap);

    auto* resHeading = new QLabel(tr("仿真结果"), card);
    resHeading->setFont(headingFont);
    layout->addWidget(resHeading);

    auto* resRowWrap = new QWidget(card);
    auto* resRow = new QHBoxLayout(resRowWrap);
    resRow->setContentsMargins(0, 0, 0, 0);
    resRow->setSpacing(6);

    addTag(resRow,
           record.packetLatencyAvg >= 0.0
               ? tr("Packet延迟: %1").arg(QString::number(record.packetLatencyAvg, 'f', 3))
               : tr("Packet延迟: --"));
    addTag(resRow,
           record.networkLatencyAvg >= 0.0
               ? tr("Network延迟: %1").arg(QString::number(record.networkLatencyAvg, 'f', 3))
               : tr("Network延迟: --"));
    addTag(resRow,
           record.flitLatencyAvg >= 0.0
               ? tr("Flit延迟: %1").arg(QString::number(record.flitLatencyAvg, 'f', 3))
               : tr("Flit延迟: --"));
    addTag(resRow,
           record.throughputMatchPercent >= 0.0
               ? tr("吞吐匹配: %1%").arg(QString::number(record.throughputMatchPercent, 'f', 1))
               : tr("吞吐匹配: --"));
    addTag(resRow,
           record.totalRunTimeSec >= 0.0
               ? tr("运行时长: %1s").arg(QString::number(record.totalRunTimeSec, 'f', 3))
               : tr("运行时长: --"));
    addTag(resRow, tr("Traffic Class: %1").arg(record.trafficClassCount));
    resRow->addStretch();
    layout->addWidget(resRowWrap);

    connect(showBtn, &ElaPushButton::clicked, this, [this, record]() {
        emit showRecordInResultRequested(record.simulationLog);
        m_statusText->setText(tr("已将记录载入结果可视化页，请切换到“BookSim 结果”查看。"));
    });
    connect(deleteBtn, &ElaPushButton::clicked, this, [this, record]() {
        const int index = findRecordIndexById(record.id);
        if (index < 0 || index >= m_records.size()) {
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

void SimulationRecordPage::refreshNumericLabelOptions() {
    if (!m_numericLabelCombo) {
        return;
    }

    const QString previousSelection = m_numericLabelCombo->currentData().toString();

    const QVector<MetricOption> options = buildMetricOptions();

    QSignalBlocker blocker(m_numericLabelCombo);
    m_numericLabelCombo->clear();
    for (const auto& option : options) {
        m_numericLabelCombo->addItem(option.label, option.key);
    }
    int target = m_numericLabelCombo->findData(previousSelection);
    if (target < 0) {
        target = m_numericLabelCombo->findData(QStringLiteral("packetLatencyAvg"));
    }
    m_numericLabelCombo->setCurrentIndex(std::max(0, target));
}

QVector<int> SimulationRecordPage::buildVisibleRecordIndices(bool sorted) const {
    QVector<int> visibleIndices;
    visibleIndices.reserve(m_records.size());
    for (int i = 0; i < m_records.size(); ++i) {
        const auto& record = m_records.at(i);
        if (!matchesKeyword(record) || !matchesFilters(record)) {
            continue;
        }
        visibleIndices.push_back(i);
    }

    if (!sorted) {
        return visibleIndices;
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
        case RecordSortMode::LabelValueAsc: {
            const QString labelKey = m_numericLabelCombo
                                         ? m_numericLabelCombo->currentData().toString()
                                         : QStringLiteral("packetLatencyAvg");
            return compareValue(numericLabelValue(a, labelKey), true)
                   < compareValue(numericLabelValue(b, labelKey), true);
        }
        case RecordSortMode::LabelValueDesc: {
            const QString labelKey = m_numericLabelCombo
                                         ? m_numericLabelCombo->currentData().toString()
                                         : QStringLiteral("packetLatencyAvg");
            return compareValue(numericLabelValue(a, labelKey), false)
                   > compareValue(numericLabelValue(b, labelKey), false);
        }
        }
        return true;
    });
    return visibleIndices;
}

QVector<SimulationRecordPage::MetricOption> SimulationRecordPage::buildMetricOptions() const {
    QVector<MetricOption> options = {
        {tr("Packet 延迟均值"), QStringLiteral("packetLatencyAvg")},
        {tr("Network 延迟均值"), QStringLiteral("networkLatencyAvg")},
        {tr("Flit 延迟均值"), QStringLiteral("flitLatencyAvg")},
        {tr("吞吐匹配(%)"), QStringLiteral("throughputMatchPercent")},
        {tr("运行时长(s)"), QStringLiteral("totalRunTimeSec")},
        {tr("Traffic Class 数"), QStringLiteral("trafficClassCount")},
    };

    QSet<QString> dynamicNumericKeys;
    for (const auto& record : m_records) {
        for (auto it = record.config.begin(); it != record.config.end(); ++it) {
            double parsed = 0.0;
            if (tryParseDouble(it.value(), &parsed)) {
                dynamicNumericKeys.insert(it.key());
            }
        }
    }
    QStringList dynamicKeys = dynamicNumericKeys.values();
    std::sort(dynamicKeys.begin(), dynamicKeys.end());
    for (const QString& key : dynamicKeys) {
        options.push_back({tr("配置: %1").arg(key), QStringLiteral("config:%1").arg(key)});
    }
    return options;
}

void SimulationRecordPage::openLineChartDialog() {
    const QVector<int> visibleIndices = buildVisibleRecordIndices(true);
    if (visibleIndices.size() < 2) {
        QMessageBox::information(this,
                                 tr("无法创建折线图"),
                                 tr("当前可见记录不足 2 条。请先调整筛选条件或新增记录。"));
        return;
    }

    const QVector<MetricOption> metricOptions = buildMetricOptions();
    if (metricOptions.size() < 2) {
        QMessageBox::information(this, tr("无法创建折线图"), tr("可用指标不足 2 项。"));
        return;
    }

    QDialog dialog(this);
    dialog.setWindowTitle(tr("创建折线图"));
    dialog.resize(680, 520);

    auto* layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(10);

    auto* tip = new QLabel(tr("选择两个指标与若干记录后，将在“BookSim 结果”页面展示双折线图。"),
                           &dialog);
    tip->setWordWrap(true);
    layout->addWidget(tip);

    auto* metricRow = new QHBoxLayout();
    auto* metricALabel = new QLabel(tr("指标 A"), &dialog);
    auto* metricACombo = new ElaComboBox(&dialog);
    auto* metricBLabel = new QLabel(tr("指标 B"), &dialog);
    auto* metricBCombo = new ElaComboBox(&dialog);
    for (const auto& metric : metricOptions) {
        metricACombo->addItem(metric.label, metric.key);
        metricBCombo->addItem(metric.label, metric.key);
    }
    metricBCombo->setCurrentIndex(std::min(1, metricBCombo->count() - 1));
    metricRow->addWidget(metricALabel);
    metricRow->addWidget(metricACombo, 1);
    metricRow->addSpacing(10);
    metricRow->addWidget(metricBLabel);
    metricRow->addWidget(metricBCombo, 1);
    layout->addLayout(metricRow);

    auto* recordLabel = new QLabel(tr("勾选记录（至少 2 条）"), &dialog);
    layout->addWidget(recordLabel);

    auto* recordList = new QListWidget(&dialog);
    recordList->setSelectionMode(QAbstractItemView::NoSelection);
    for (int i = 0; i < visibleIndices.size(); ++i) {
        const int recordIndex = visibleIndices[i];
        const auto& record = m_records.at(recordIndex);
        auto* item = new QListWidgetItem(tr("%1 | 更新: %2")
                                             .arg(record.name, formatIsoLocal(record.updatedAtIso)),
                                         recordList);
        item->setData(Qt::UserRole, recordIndex);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(i < 6 ? Qt::Checked : Qt::Unchecked);
    }
    layout->addWidget(recordList, 1);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttons);

    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    const QString metricAKey = metricACombo->currentData().toString();
    const QString metricBKey = metricBCombo->currentData().toString();
    const QString metricALabelText = metricACombo->currentText();
    const QString metricBLabelText = metricBCombo->currentText();
    if (metricAKey == metricBKey) {
        QMessageBox::warning(this, tr("指标重复"), tr("请为折线图选择两个不同指标。"));
        return;
    }

    QList<SimulationRecordSnapshot> selectedRecords;
    for (int i = 0; i < recordList->count(); ++i) {
        QListWidgetItem* item = recordList->item(i);
        if (!item || item->checkState() != Qt::Checked) {
            continue;
        }
        const int recordIndex = item->data(Qt::UserRole).toInt();
        if (recordIndex < 0 || recordIndex >= m_records.size()) {
            continue;
        }
        selectedRecords.push_back(m_records.at(recordIndex));
    }

    if (selectedRecords.size() < 2) {
        QMessageBox::warning(this, tr("记录不足"), tr("请至少勾选 2 条记录。"));
        return;
    }

    emit showLineChartInResultRequested(selectedRecords,
                                        metricAKey,
                                        metricALabelText,
                                        metricBKey,
                                        metricBLabelText);
    m_statusText->setText(tr("已创建折线图，请切换到“BookSim 结果”查看。"));
}

double SimulationRecordPage::numericLabelValue(const SimulationRecordSnapshot& record,
                                               const QString& labelKey) {
    if (labelKey == QStringLiteral("packetLatencyAvg")) {
        return record.packetLatencyAvg;
    }
    if (labelKey == QStringLiteral("networkLatencyAvg")) {
        return record.networkLatencyAvg;
    }
    if (labelKey == QStringLiteral("flitLatencyAvg")) {
        return record.flitLatencyAvg;
    }
    if (labelKey == QStringLiteral("throughputMatchPercent")) {
        return record.throughputMatchPercent;
    }
    if (labelKey == QStringLiteral("totalRunTimeSec")) {
        return record.totalRunTimeSec;
    }
    if (labelKey == QStringLiteral("trafficClassCount")) {
        return static_cast<double>(record.trafficClassCount);
    }
    if (labelKey.startsWith(QStringLiteral("config:"))) {
        const QString configKey = labelKey.mid(QStringLiteral("config:").size());
        double value = 0.0;
        if (tryParseDouble(record.config.value(configKey), &value)) {
            return value;
        }
    }
    return -1.0;
}

bool SimulationRecordPage::hasNumericFilter() const {
    if (!m_numericFilterMode || !m_rangeMinEdit || !m_rangeMaxEdit) {
        return false;
    }
    const auto mode = static_cast<NumericFilterMode>(m_numericFilterMode->currentData().toInt());
    if (mode == NumericFilterMode::All) {
        return false;
    }
    double parsed = 0.0;
    const bool hasMin = tryParseDouble(m_rangeMinEdit->text(), &parsed);
    const bool hasMax = tryParseDouble(m_rangeMaxEdit->text(), &parsed);
    return hasMin || hasMax;
}

void SimulationRecordPage::updateNumericFilterEditors() {
    if (!m_numericFilterMode || !m_rangeMinEdit || !m_rangeMaxEdit) {
        return;
    }

    const auto mode = static_cast<NumericFilterMode>(m_numericFilterMode->currentData().toInt());
    switch (mode) {
    case NumericFilterMode::All:
        m_rangeMinEdit->clear();
        m_rangeMaxEdit->clear();
        m_rangeMinEdit->setEnabled(false);
        m_rangeMaxEdit->setEnabled(false);
        m_rangeMinEdit->setPlaceholderText(tr("输入下限"));
        m_rangeMaxEdit->setPlaceholderText(tr("输入上限"));
        break;
    case NumericFilterMode::GreaterThan:
        m_rangeMinEdit->setEnabled(true);
        m_rangeMaxEdit->setEnabled(false);
        m_rangeMaxEdit->clear();
        m_rangeMinEdit->setPlaceholderText(tr("大于该值"));
        m_rangeMaxEdit->setPlaceholderText(tr("未使用"));
        break;
    case NumericFilterMode::LessThan:
        m_rangeMinEdit->setEnabled(false);
        m_rangeMinEdit->clear();
        m_rangeMaxEdit->setEnabled(true);
        m_rangeMinEdit->setPlaceholderText(tr("未使用"));
        m_rangeMaxEdit->setPlaceholderText(tr("小于该值"));
        break;
    case NumericFilterMode::Between:
        m_rangeMinEdit->setEnabled(true);
        m_rangeMaxEdit->setEnabled(true);
        m_rangeMinEdit->setPlaceholderText(tr("最小值"));
        m_rangeMaxEdit->setPlaceholderText(tr("最大值"));
        break;
    }
}

void SimulationRecordPage::rebuildCards() {
    while (QLayoutItem* item = m_cardsLayout->takeAt(0)) {
        delete item->widget();
        delete item;
    }

    refreshNumericLabelOptions();

    const QVector<int> visibleIndices = buildVisibleRecordIndices(true);

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
