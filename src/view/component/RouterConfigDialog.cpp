#include "RouterConfigDialog.h"
#include <ElaLineEdit.h>
#include <ElaPushButton.h>
#include <QFont>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>

RouterConfigDialog::RouterConfigDialog(const QString& routerId, QWidget* parent)
    : QDialog(parent)
    , m_routerId(routerId)
    , m_mainLayout(nullptr)
    , m_scrollArea(nullptr)
    , m_saveBtn(nullptr)
    , m_cancelBtn(nullptr) {
    setupUI();
    setWindowTitle("路由器配置: " + routerId);
    setMinimumSize(600, 700);
}

void RouterConfigDialog::setupUI() {
    auto* outerLayout = new QVBoxLayout(this);

    // 创建滚动区域
    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    auto* scrollContent = new QWidget();
    m_mainLayout = new QVBoxLayout(scrollContent);
    m_mainLayout->setSpacing(10);

    // 虚拟通道配置
    addSectionTitle("虚拟通道配置");
    addConfigItem("num_vcs", "虚拟通道数 (num_vcs)", "8");
    addConfigItem("vc_buf_size", "VC缓冲区大小 (vc_buf_size)", "16");

    // 路由器基础参数
    addSectionTitle("路由器基础参数");
    addConfigItem("internal_speedup", "内部加速 (internal_speedup)", "1.0");
    addConfigItem("output_buffer_size", "输出缓冲大小 (output_buffer_size)", "-1");
    addConfigItem("noq", "无输出排队 (noq)", "0");

    // 端口加速参数
    addSectionTitle("端口加速参数");
    addConfigItem("input_speedup", "输入加速 (input_speedup)", "1");
    addConfigItem("output_speedup", "输出加速 (output_speedup)", "1");
    addConfigItem("hold_switch_for_packet", "为包保持开关 (hold_switch_for_packet)", "0");

    // 分配器配置
    addSectionTitle("分配器配置");
    addConfigItem("vc_allocator", "VC分配器 (vc_allocator)", "islip");
    addConfigItem("sw_allocator", "开关分配器 (sw_allocator)", "islip");
    addConfigItem("alloc_iters", "分配迭代次数 (alloc_iters)", "1");

    // 流水线延迟参数
    addSectionTitle("流水线延迟参数");
    addConfigItem("routing_delay", "路由延迟 (routing_delay)", "1");
    addConfigItem("vc_alloc_delay", "VC分配延迟 (vc_alloc_delay)", "1");
    addConfigItem("sw_alloc_delay", "开关分配延迟 (sw_alloc_delay)", "1");
    addConfigItem("credit_delay", "信用延迟 (credit_delay)", "1");

    // 推测执行参数
    addSectionTitle("推测执行参数");
    addConfigItem("speculative", "推测分配 (speculative)", "0");

    // 虚拟通道行为
    addSectionTitle("虚拟通道行为");
    addConfigItem("vc_busy_when_full", "满时VC忙碌 (vc_busy_when_full)", "0");
    addConfigItem("vc_prioritize_empty", "VC优先空队列 (vc_prioritize_empty)", "0");
    addConfigItem("vc_shuffle_requests", "VC随机请求 (vc_shuffle_requests)", "0");
    addConfigItem("wait_for_tail_credit", "等待尾部信用 (wait_for_tail_credit)", "1");

    m_mainLayout->addStretch();

    m_scrollArea->setWidget(scrollContent);
    outerLayout->addWidget(m_scrollArea);

    // 按钮布局
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

void RouterConfigDialog::addConfigItem(const QString& key,
                                       const QString& label,
                                       const QString& defaultValue) {
    auto* labelWidget = new QLabel(label, this);
    labelWidget->setMinimumWidth(200);
    auto* edit = new ElaLineEdit(this);
    edit->setText(defaultValue);

    m_edits[key] = edit;

    auto* itemLayout = new QHBoxLayout();
    itemLayout->addWidget(labelWidget);
    itemLayout->addWidget(edit);

    m_mainLayout->addLayout(itemLayout);
}

void RouterConfigDialog::setConfig(const QMap<QString, QString>& config) {
    for (auto it = config.begin(); it != config.end(); ++it) {
        if (m_edits.contains(it.key())) {
            m_edits[it.key()]->setText(it.value());
        }
    }
}

QMap<QString, QString> RouterConfigDialog::getConfig() const {
    QMap<QString, QString> config;
    for (auto it = m_edits.begin(); it != m_edits.end(); ++it) {
        config[it.key()] = it.value()->text();
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
    // 添加分隔线（除了第一个分组）
    if (m_mainLayout->count() > 0) {
        auto* line = new QFrame(this);
        line->setFrameShape(QFrame::HLine);
        line->setFrameShadow(QFrame::Sunken);
        m_mainLayout->addWidget(line);
    }

    // 添加分组标题
    auto* titleLabel = new QLabel(title, this);
    QFont font = titleLabel->font();
    font.setBold(true);
    font.setPointSize(font.pointSize() + 1);
    titleLabel->setFont(font);
    titleLabel->setStyleSheet("color: #2c3e50; margin-top: 5px; margin-bottom: 5px;");
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
