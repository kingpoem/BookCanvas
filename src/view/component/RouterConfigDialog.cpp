#include "RouterConfigDialog.h"
#include <ElaComboBox.h>
#include <ElaDef.h>
#include <ElaDialog.h>
#include <ElaDoubleSpinBox.h>
#include <ElaPushButton.h>
#include <ElaSpinBox.h>
#include <ElaTheme.h>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFont>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPalette>
#include <QScrollArea>
#include <QSizePolicy>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QVariant>
#include <QtGlobal>

namespace {

void paintScrollViewport(QWidget* viewport, const QColor& bg) {
    if (!viewport) {
        return;
    }
    viewport->setAutoFillBackground(true);
    QPalette vpPal = viewport->palette();
    vpPal.setColor(QPalette::Window, bg);
    vpPal.setColor(QPalette::Base, bg);
    viewport->setPalette(vpPal);
}

[[nodiscard]] QStringList vcAllocatorChoices() {
    return {QStringLiteral("piggyback"),
            QStringLiteral("max_size"),
            QStringLiteral("pim"),
            QStringLiteral("islip"),
            QStringLiteral("loa"),
            QStringLiteral("wavefront"),
            QStringLiteral("rr_wavefront"),
            QStringLiteral("select"),
            QStringLiteral("separable_input_first"),
            QStringLiteral("separable_output_first")};
}

[[nodiscard]] QStringList swAllocatorChoices() {
    return {QStringLiteral("max_size"),
            QStringLiteral("pim"),
            QStringLiteral("islip"),
            QStringLiteral("loa"),
            QStringLiteral("wavefront"),
            QStringLiteral("rr_wavefront"),
            QStringLiteral("select"),
            QStringLiteral("separable_input_first"),
            QStringLiteral("separable_output_first")};
}

[[nodiscard]] QString trimDoubleString(double v, int decimals) {
    QString t = QString::number(v, 'f', decimals);
    while (t.contains(QLatin1Char('.')) && t.endsWith(QLatin1Char('0'))) {
        t.chop(1);
    }
    if (t.endsWith(QLatin1Char('.'))) {
        t.chop(1);
    }
    return t;
}

} // namespace

QString RouterConfigDialog::widgetToConfigString(const QWidget* w) {
    if (!w) {
        return {};
    }
    if (const auto* c = qobject_cast<const QComboBox*>(w)) {
        const QVariant d = c->currentData(Qt::UserRole);
        if (d.isValid()) {
            const QString stored = d.toString();
            if (!stored.isEmpty()) {
                return stored;
            }
        }
        return c->currentText().trimmed();
    }
    if (const auto* s = qobject_cast<const QSpinBox*>(w)) {
        return QString::number(s->value());
    }
    if (const auto* d = qobject_cast<const QDoubleSpinBox*>(w)) {
        return trimDoubleString(d->value(), 6);
    }
    return {};
}

void RouterConfigDialog::setWidgetFromConfigString(QWidget* w, const QString& text) {
    if (!w) {
        return;
    }
    const QString t = text.trimmed();
    if (auto* c = qobject_cast<QComboBox*>(w)) {
        int idx = -1;
        for (int i = 0; i < c->count(); ++i) {
            if (c->itemData(i, Qt::UserRole).toString() == t) {
                idx = i;
                break;
            }
        }
        if (idx < 0) {
            idx = c->findText(t, Qt::MatchExactly);
        }
        if (idx < 0) {
            idx = c->findText(t, Qt::MatchFixedString);
        }
        if (idx >= 0) {
            c->setCurrentIndex(idx);
        } else if (!t.isEmpty()) {
            c->insertItem(0, t);
            c->setCurrentIndex(0);
        }
        return;
    }
    if (auto* s = qobject_cast<QSpinBox*>(w)) {
        bool ok = false;
        const int v = t.toInt(&ok);
        if (ok) {
            s->setValue(v);
        }
        return;
    }
    if (auto* d = qobject_cast<QDoubleSpinBox*>(w)) {
        bool ok = false;
        const double v = t.toDouble(&ok);
        if (ok) {
            d->setValue(qBound(d->minimum(), v, d->maximum()));
        }
    }
}

RouterConfigDialog::RouterConfigDialog(const QString& routerId, QWidget* parent)
    : ElaDialog(parent)
    , m_routerId(routerId) {
    setWindowButtonFlags(ElaAppBarType::CloseButtonHint);
    QObject::connect(this, &ElaDialog::closeButtonClicked, this, &QDialog::reject);
    setupUI();
    setWindowTitle("路由器配置: " + routerId);
    setMinimumSize(680, 700);
    applyDialogChrome();
    QObject::connect(eTheme, &ElaTheme::themeModeChanged, this, [this](ElaThemeType::ThemeMode) {
        applyDialogChrome();
    });
}

void RouterConfigDialog::addFieldRow(const QString& labelText, QWidget* field) {
    auto* labelWidget = new QLabel(labelText, m_scrollContent);
    labelWidget->setMinimumWidth(220);
    labelWidget->setProperty("routerConfigLabelRole", QStringLiteral("field"));
    bindLabelToTheme(labelWidget, false);

    if (field) {
        field->setMinimumWidth(qMax(field->minimumWidth(), 200));
        field->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
    }

    auto* itemLayout = new QHBoxLayout();
    itemLayout->addWidget(labelWidget);
    itemLayout->addWidget(field, 1);
    m_mainLayout->addLayout(itemLayout);
}

void RouterConfigDialog::addBool01Field(const QString& key,
                                        const QString& label,
                                        const QString& defaultValue) {
    auto* box = new ElaComboBox(m_scrollContent);
    box->addItem(tr("0（关闭）"), QStringLiteral("0"));
    box->addItem(tr("1（开启）"), QStringLiteral("1"));
    box->setMaxVisibleItems(2);
    const int idx = defaultValue.trimmed() == QStringLiteral("1") ? 1 : 0;
    box->setCurrentIndex(idx);
    m_fields.insert(key, box);
    addFieldRow(label, box);
}

void RouterConfigDialog::addIntField(const QString& key,
                                     const QString& label,
                                     const int minV,
                                     const int maxV,
                                     const QString& defaultValue) {
    auto* sp = new ElaSpinBox(m_scrollContent);
    sp->setRange(minV, maxV);
    bool ok = false;
    const int v = defaultValue.toInt(&ok);
    sp->setValue(ok ? qBound(minV, v, maxV) : minV);
    m_fields.insert(key, sp);
    addFieldRow(label, sp);
}

void RouterConfigDialog::addIntFieldNeg1(const QString& key,
                                         const QString& label,
                                         const int maxV,
                                         const QString& defaultValue) {
    auto* sp = new ElaSpinBox(m_scrollContent);
    sp->setRange(-1, maxV);
    bool ok = false;
    const int v = defaultValue.toInt(&ok);
    sp->setValue(ok ? qBound(-1, v, maxV) : -1);
    m_fields.insert(key, sp);
    addFieldRow(label, sp);
}

void RouterConfigDialog::addDoubleField(const QString& key,
                                        const QString& label,
                                        const double minV,
                                        const double maxV,
                                        const double step,
                                        const int decimals,
                                        const QString& defaultValue) {
    auto* sp = new ElaDoubleSpinBox(m_scrollContent);
    sp->setRange(minV, maxV);
    sp->setSingleStep(step);
    sp->setDecimals(decimals);
    sp->setMinimumWidth(220);
    bool ok = false;
    const double v = defaultValue.toDouble(&ok);
    sp->setValue(ok ? qBound(minV, v, maxV) : minV);
    m_fields.insert(key, sp);
    addFieldRow(label, sp);
}

void RouterConfigDialog::addAllocCombo(const QString& key,
                                       const QString& label,
                                       const QString& defaultValue,
                                       const bool vc) {
    auto* box = new ElaComboBox(m_scrollContent);
    box->addItems(vc ? vcAllocatorChoices() : swAllocatorChoices());
    box->setMaxVisibleItems(16);
    m_fields.insert(key, box);
    addFieldRow(label, box);
    setWidgetFromConfigString(box, defaultValue);
}

void RouterConfigDialog::setupUI() {
    auto* outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(16, 8, 16, 16);

    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    m_scrollContent = new QWidget();
    m_mainLayout = new QVBoxLayout(m_scrollContent);
    m_mainLayout->setSpacing(10);

    addSectionTitle("虚拟通道配置");
    addIntField("num_vcs", "虚拟通道数 (num_vcs)", 1, 256, QStringLiteral("8"));
    addIntField("vc_buf_size", "VC缓冲区大小 (vc_buf_size)", 1, 1 << 20, QStringLiteral("16"));

    addSectionTitle("路由器基础参数");
    addDoubleField("internal_speedup",
                   "内部加速 (internal_speedup)",
                   0.01,
                   256.0,
                   0.01,
                   4,
                   QStringLiteral("1.0"));
    addIntFieldNeg1("output_buffer_size",
                    "输出缓冲大小 (output_buffer_size)",
                    1 << 20,
                    QStringLiteral("-1"));
    addBool01Field("noq", "无输出排队 (noq)", QStringLiteral("0"));

    addSectionTitle("端口加速参数");
    addIntField("input_speedup", "输入加速 (input_speedup)", 1, 64, QStringLiteral("1"));
    addIntField("output_speedup", "输出加速 (output_speedup)", 1, 64, QStringLiteral("1"));
    addBool01Field("hold_switch_for_packet",
                   "为包保持开关 (hold_switch_for_packet)",
                   QStringLiteral("0"));

    addSectionTitle("分配器配置");
    addAllocCombo("vc_allocator", "VC分配器 (vc_allocator)", QStringLiteral("islip"), true);
    addAllocCombo("sw_allocator", "开关分配器 (sw_allocator)", QStringLiteral("islip"), false);
    addIntField("alloc_iters", "分配迭代次数 (alloc_iters)", 1, 128, QStringLiteral("1"));

    addSectionTitle("流水线延迟参数");
    addIntField("routing_delay", "路由延迟 (routing_delay)", 0, 256, QStringLiteral("1"));
    addIntField("vc_alloc_delay", "VC分配延迟 (vc_alloc_delay)", 0, 256, QStringLiteral("1"));
    addIntField("sw_alloc_delay", "开关分配延迟 (sw_alloc_delay)", 0, 256, QStringLiteral("1"));
    addIntField("credit_delay", "信用延迟 (credit_delay)", 0, 256, QStringLiteral("1"));

    addSectionTitle("推测执行参数");
    addBool01Field("speculative", "推测分配 (speculative)", QStringLiteral("0"));

    addSectionTitle("虚拟通道行为");
    addBool01Field("vc_busy_when_full", "满时VC忙碌 (vc_busy_when_full)", QStringLiteral("0"));
    addBool01Field("vc_prioritize_empty", "VC优先空队列 (vc_prioritize_empty)", QStringLiteral("0"));
    addBool01Field("vc_shuffle_requests", "VC随机请求 (vc_shuffle_requests)", QStringLiteral("0"));
    addBool01Field("wait_for_tail_credit",
                   "等待尾部信用 (wait_for_tail_credit)",
                   QStringLiteral("1"));

    m_mainLayout->addStretch();

    m_scrollArea->setWidget(m_scrollContent);
    outerLayout->addWidget(m_scrollArea);

    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    m_saveBtn = new ElaPushButton("保存", this);
    m_cancelBtn = new ElaPushButton("取消", this);

    buttonLayout->addWidget(m_saveBtn);
    buttonLayout->addWidget(m_cancelBtn);

    outerLayout->addLayout(buttonLayout);

    connect(m_saveBtn, &ElaPushButton::clicked, this, &RouterConfigDialog::onSaveClicked);
    connect(m_cancelBtn, &ElaPushButton::clicked, this, &RouterConfigDialog::onCancelClicked);
}

void RouterConfigDialog::applyDialogChrome() {
    const ElaThemeType::ThemeMode mode = eTheme->getThemeMode();
    const QColor panelBg = ElaThemeColor(mode, WindowBase);
    const QColor border = ElaThemeColor(mode, BasicBorder);

    if (m_scrollContent) {
        m_scrollContent->setAutoFillBackground(true);
        QPalette cPal = m_scrollContent->palette();
        cPal.setColor(QPalette::Window, panelBg);
        m_scrollContent->setPalette(cPal);
        m_scrollContent->setStyleSheet(
            QStringLiteral("background-color: %1;").arg(panelBg.name(QColor::HexRgb)));
    }
    if (m_scrollArea) {
        paintScrollViewport(m_scrollArea->viewport(), panelBg);
    }

    for (QFrame* line : findChildren<QFrame*>(QStringLiteral("routerConfigSep"))) {
        line->setStyleSheet(QStringLiteral("background-color: %1; border: none; min-height: 1px; "
                                           "max-height: 1px; margin: 4px 0;")
                                .arg(border.name(QColor::HexRgb)));
    }

    for (QLabel* lab : findChildren<QLabel*>()) {
        if (lab->property("routerConfigLabelRole").toString() == QLatin1String("section")) {
            bindLabelToTheme(lab, true);
        } else if (lab->property("routerConfigLabelRole").toString() == QLatin1String("field")) {
            bindLabelToTheme(lab, false);
        }
    }
}

void RouterConfigDialog::bindLabelToTheme(QLabel* label, bool sectionTitle) {
    if (!label) {
        return;
    }
    const auto mode = eTheme->getThemeMode();
    const QColor text = sectionTitle ? ElaThemeColor(mode, BasicDetailsText)
                                     : ElaThemeColor(mode, BasicText);
    const int margin = sectionTitle ? 5 : 0;
    label->setStyleSheet(
        QStringLiteral("background: transparent; color: %1; margin-top: %2px; margin-bottom: %2px;")
            .arg(text.name(QColor::HexRgb))
            .arg(margin));
    QPalette pal = label->palette();
    pal.setColor(QPalette::WindowText, text);
    label->setPalette(pal);
}

void RouterConfigDialog::setConfig(const QMap<QString, QString>& config) {
    for (auto it = config.begin(); it != config.end(); ++it) {
        if (m_fields.contains(it.key())) {
            setWidgetFromConfigString(m_fields[it.key()], it.value());
        }
    }
}

QMap<QString, QString> RouterConfigDialog::getConfig() const {
    QMap<QString, QString> config;
    for (auto it = m_fields.begin(); it != m_fields.end(); ++it) {
        config[it.key()] = widgetToConfigString(it.value());
    }
    return config;
}

void RouterConfigDialog::onSaveClicked() {
    accept();
}

void RouterConfigDialog::onCancelClicked() {
    reject();
}

void RouterConfigDialog::addSectionTitle(const QString& title) {
    if (m_mainLayout->count() > 0) {
        auto* line = new QFrame(m_scrollContent);
        line->setObjectName(QStringLiteral("routerConfigSep"));
        line->setFrameShape(QFrame::NoFrame);
        line->setFixedHeight(1);
        m_mainLayout->addWidget(line);
    }

    auto* titleLabel = new QLabel(title, m_scrollContent);
    titleLabel->setProperty("routerConfigLabelRole", QStringLiteral("section"));
    QFont font = titleLabel->font();
    font.setBold(true);
    font.setPointSize(font.pointSize() + 1);
    titleLabel->setFont(font);
    bindLabelToTheme(titleLabel, true);
    m_mainLayout->addWidget(titleLabel);
}

QMap<QString, QString> RouterConfigDialog::getDefaultConfig() {
    QMap<QString, QString> defaultConfig;

    defaultConfig["num_vcs"] = "8";
    defaultConfig["vc_buf_size"] = "16";
    defaultConfig["input_speedup"] = "1";
    defaultConfig["output_speedup"] = "1";
    defaultConfig["internal_speedup"] = "1.0";
    defaultConfig["vc_allocator"] = "islip";
    defaultConfig["sw_allocator"] = "islip";
    defaultConfig["alloc_iters"] = "1";
    defaultConfig["routing_delay"] = "1";
    defaultConfig["vc_alloc_delay"] = "1";
    defaultConfig["sw_alloc_delay"] = "1";
    defaultConfig["credit_delay"] = "1";
    defaultConfig["speculative"] = "0";
    defaultConfig["vc_busy_when_full"] = "0";
    defaultConfig["vc_prioritize_empty"] = "0";
    defaultConfig["vc_shuffle_requests"] = "0";
    defaultConfig["hold_switch_for_packet"] = "0";
    defaultConfig["wait_for_tail_credit"] = "1";
    defaultConfig["output_buffer_size"] = "-1";
    defaultConfig["noq"] = "0";

    return defaultConfig;
}
