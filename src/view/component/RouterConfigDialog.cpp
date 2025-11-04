#include "RouterConfigDialog.h"
#include <ElaLineEdit.h>
#include <ElaPushButton.h>
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

    // 添加路由器配置项
    addConfigItem("num_vcs", "虚拟通道数 (num_vcs)", "8");
    addConfigItem("vc_buf_size", "VC缓冲区大小 (vc_buf_size)", "16");
    addConfigItem("input_speedup", "输入加速 (input_speedup)", "1");
    addConfigItem("output_speedup", "输出加速 (output_speedup)", "1");
    addConfigItem("internal_speedup", "内部加速 (internal_speedup)", "1.0");
    addConfigItem("vc_allocator", "VC分配器 (vc_allocator)", "islip");
    addConfigItem("sw_allocator", "开关分配器 (sw_allocator)", "islip");
    addConfigItem("alloc_iters", "分配迭代次数 (alloc_iters)", "1");
    addConfigItem("routing_delay", "路由延迟 (routing_delay)", "1");
    addConfigItem("vc_alloc_delay", "VC分配延迟 (vc_alloc_delay)", "1");
    addConfigItem("sw_alloc_delay", "开关分配延迟 (sw_alloc_delay)", "1");
    addConfigItem("credit_delay", "信用延迟 (credit_delay)", "1");
    addConfigItem("speculative", "推测分配 (speculative)", "0");
    addConfigItem("vc_busy_when_full", "满时VC忙碌 (vc_busy_when_full)", "0");
    addConfigItem("vc_prioritize_empty", "VC优先空队列 (vc_prioritize_empty)", "0");
    addConfigItem("vc_shuffle_requests", "VC随机请求 (vc_shuffle_requests)", "0");
    addConfigItem("hold_switch_for_packet", "为包保持开关 (hold_switch_for_packet)", "0");
    addConfigItem("wait_for_tail_credit", "等待尾部信用 (wait_for_tail_credit)", "1");
    addConfigItem("output_buffer_size", "输出缓冲大小 (output_buffer_size)", "-1");
    addConfigItem("noq", "无输出排队 (noq)", "0");

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
