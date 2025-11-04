#include "RouterGlobalConfigDialog.h"
#include <ElaLineEdit.h>
#include <ElaPushButton.h>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>

RouterGlobalConfigDialog::RouterGlobalConfigDialog(QWidget* parent)
    : QDialog(parent)
    , m_mainLayout(nullptr)
    , m_scrollArea(nullptr)
    , m_saveBtn(nullptr)
    , m_cancelBtn(nullptr) {
    setupUI();
    setWindowTitle("全局配置");
    setMinimumSize(600, 700);
}

void RouterGlobalConfigDialog::setupUI() {
    auto* outerLayout = new QVBoxLayout(this);

    // 创建滚动区域
    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    auto* scrollContent = new QWidget();
    m_mainLayout = new QVBoxLayout(scrollContent);
    m_mainLayout->setSpacing(10);

    // 添加全局配置项
    addConfigItem("channel_file", "通道文件 (channel_file)", "");
    addConfigItem("subnets", "子网数 (subnets)", "1");
    addConfigItem("topology", "拓扑结构 (topology)", "anynet");
    addConfigItem("network_file", "网络文件 (network_file)", "anynet_file");
    addConfigItem("routing_function", "路由函数 (routing_function)", "min");
    addConfigItem("use_noc_latency", "使用NoC延迟 (use_noc_latency)", "1");
    addConfigItem("x", "X维度 (x)", "8");
    addConfigItem("y", "Y维度 (y)", "8");
    addConfigItem("xr", "X路由器 (xr)", "1");
    addConfigItem("yr", "Y路由器 (yr)", "1");
    addConfigItem("link_failures", "链接失败 (link_failures)", "0");
    addConfigItem("fail_seed", "失败种子 (fail_seed)", "0");
    addConfigItem("in_ports", "输入端口 (in_ports)", "5");
    addConfigItem("out_ports", "输出端口 (out_ports)", "5");
    addConfigItem("router", "路由器类型 (router)", "iq");
    addConfigItem("output_delay", "输出延迟 (output_delay)", "0");
    addConfigItem("credit_delay", "信用延迟 (credit_delay)", "1");
    addConfigItem("internal_speedup", "内部加速 (internal_speedup)", "1.0");
    addConfigItem("output_buffer_size", "输出缓冲大小 (output_buffer_size)", "-1");
    addConfigItem("noq", "无输出排队 (noq)", "0");
    addConfigItem("speculative", "推测分配 (speculative)", "0");
    addConfigItem("spec_check_elig", "推测检查资格 (spec_check_elig)", "1");
    addConfigItem("spec_check_cred", "推测检查信用 (spec_check_cred)", "1");
    addConfigItem("spec_mask_by_reqs", "推测掩码请求 (spec_mask_by_reqs)", "0");
    addConfigItem("spec_sw_allocator", "推测开关分配器 (spec_sw_allocator)", "prio");
    addConfigItem("num_vcs", "虚拟通道数 (num_vcs)", "8");
    addConfigItem("vc_buf_size", "VC缓冲区大小 (vc_buf_size)", "16");
    addConfigItem("buf_size", "缓冲大小 (buf_size)", "-1");
    addConfigItem("buffer_policy", "缓冲策略 (buffer_policy)", "private");
    addConfigItem("private_bufs", "私有缓冲 (private_bufs)", "-1");
    addConfigItem("private_buf_size", "私有缓冲大小 (private_buf_size)", "1");
    addConfigItem("private_buf_start_vc", "私有缓冲起始VC (private_buf_start_vc)", "-1");
    addConfigItem("private_buf_end_vc", "私有缓冲结束VC (private_buf_end_vc)", "-1");
    addConfigItem("max_held_slots", "最大保持槽位 (max_held_slots)", "-1");
    addConfigItem("feedback_aging_scale", "反馈老化比例 (feedback_aging_scale)", "1");
    addConfigItem("feedback_offset", "反馈偏移 (feedback_offset)", "0");
    addConfigItem("wait_for_tail_credit", "等待尾部信用 (wait_for_tail_credit)", "1");
    addConfigItem("vc_busy_when_full", "满时VC忙碌 (vc_busy_when_full)", "0");
    addConfigItem("vc_prioritize_empty", "VC优先空队列 (vc_prioritize_empty)", "0");
    addConfigItem("vc_priority_donation", "VC优先捐赠 (vc_priority_donation)", "0");
    addConfigItem("vc_shuffle_requests", "VC随机请求 (vc_shuffle_requests)", "0");
    addConfigItem("hold_switch_for_packet", "为包保持开关 (hold_switch_for_packet)", "0");
    addConfigItem("input_speedup", "输入加速 (input_speedup)", "1");
    addConfigItem("output_speedup", "输出加速 (output_speedup)", "1");
    addConfigItem("routing_delay", "路由延迟 (routing_delay)", "1");
    addConfigItem("vc_alloc_delay", "VC分配延迟 (vc_alloc_delay)", "1");
    addConfigItem("sw_alloc_delay", "开关分配延迟 (sw_alloc_delay)", "1");
    addConfigItem("st_prepare_delay", "ST准备延迟 (st_prepare_delay)", "0");
    addConfigItem("st_final_delay", "ST最终延迟 (st_final_delay)", "1");
    addConfigItem("vct", "VCT模式 (vct)", "0");
    addConfigItem("vc_allocator", "VC分配器 (vc_allocator)", "islip");
    addConfigItem("sw_allocator", "开关分配器 (sw_allocator)", "islip");
    addConfigItem("arb_type", "仲裁类型 (arb_type)", "round_robin");
    addConfigItem("alloc_iters", "分配迭代次数 (alloc_iters)", "1");
    addConfigItem("classes", "类别数 (classes)", "1");
    addConfigItem("traffic", "流量模式 (traffic)", "uniform");
    addConfigItem("class_priority", "类别优先级 (class_priority)", "0");
    addConfigItem("perm_seed", "排列种子 (perm_seed)", "0");
    addConfigItem("injection_rate", "注入速率 (injection_rate)", "0.05");
    addConfigItem("injection_rate_uses_flits", "注入速率使用flits (injection_rate_uses_flits)", "0");
    addConfigItem("packet_size", "包大小 (packet_size)", "5");
    addConfigItem("packet_size_rate", "包大小速率 (packet_size_rate)", "1");
    addConfigItem("injection_process", "注入过程 (injection_process)", "bernoulli");
    addConfigItem("burst_alpha", "突发alpha (burst_alpha)", "0.5");
    addConfigItem("burst_beta", "突发beta (burst_beta)", "0.5");
    addConfigItem("burst_r1", "突发r1 (burst_r1)", "-1.0");
    addConfigItem("priority", "优先级 (priority)", "none");
    addConfigItem("batch_size", "批次大小 (batch_size)", "1000");
    addConfigItem("batch_count", "批次数量 (batch_count)", "1");
    addConfigItem("max_outstanding_requests", "最大未完成请求 (max_outstanding_requests)", "0");
    addConfigItem("use_read_write", "使用读写 (use_read_write)", "0");
    addConfigItem("write_fraction", "写入分数 (write_fraction)", "0.5");
    addConfigItem("read_request_begin_vc", "读请求起始VC (read_request_begin_vc)", "0");
    addConfigItem("read_request_end_vc", "读请求结束VC (read_request_end_vc)", "3");
    addConfigItem("write_request_begin_vc", "写请求起始VC (write_request_begin_vc)", "4");
    addConfigItem("write_request_end_vc", "写请求结束VC (write_request_end_vc)", "7");
    addConfigItem("read_reply_begin_vc", "读回复起始VC (read_reply_begin_vc)", "0");
    addConfigItem("read_reply_end_vc", "读回复结束VC (read_reply_end_vc)", "3");
    addConfigItem("write_reply_begin_vc", "写回复起始VC (write_reply_begin_vc)", "4");
    addConfigItem("write_reply_end_vc", "写回复结束VC (write_reply_end_vc)", "7");
    addConfigItem("read_request_subnet", "读请求子网 (read_request_subnet)", "0");
    addConfigItem("read_reply_subnet", "读回复子网 (read_reply_subnet)", "0");
    addConfigItem("write_request_subnet", "写请求子网 (write_request_subnet)", "0");
    addConfigItem("write_reply_subnet", "写回复子网 (write_reply_subnet)", "0");
    addConfigItem("read_request_size", "读请求大小 (read_request_size)", "1");
    addConfigItem("write_request_size", "写请求大小 (write_request_size)", "1");
    addConfigItem("read_reply_size", "读回复大小 (read_reply_size)", "4");
    addConfigItem("write_reply_size", "写回复大小 (write_reply_size)", "4");
    addConfigItem("sim_type", "仿真类型 (sim_type)", "latency");
    addConfigItem("warmup_periods", "预热周期 (warmup_periods)", "3");
    addConfigItem("sample_period", "采样周期 (sample_period)", "5000");
    addConfigItem("max_samples", "最大采样数 (max_samples)", "10");
    addConfigItem("measure_stats", "测量统计 (measure_stats)", "1");
    addConfigItem("pair_stats", "配对统计 (pair_stats)", "0");
    addConfigItem("latency_thres", "延迟阈值 (latency_thres)", "1000.0");
    addConfigItem("warmup_thres", "预热阈值 (warmup_thres)", "0.05");
    addConfigItem("acc_warmup_thres", "累积预热阈值 (acc_warmup_thres)", "0.05");
    addConfigItem("stopping_thres", "停止阈值 (stopping_thres)", "0.05");
    addConfigItem("acc_stopping_thres", "累积停止阈值 (acc_stopping_thres)", "0.05");
    addConfigItem("sim_count", "仿真计数 (sim_count)", "1");
    addConfigItem("include_queuing", "包含排队 (include_queuing)", "1");
    addConfigItem("seed", "随机种子 (seed)", "42");
    addConfigItem("print_activity", "打印活动 (print_activity)", "0");
    addConfigItem("print_csv_results", "打印CSV结果 (print_csv_results)", "0");
    addConfigItem("deadlock_warn_timeout", "死锁警告超时 (deadlock_warn_timeout)", "512");
    addConfigItem("viewer_trace", "查看器跟踪 (viewer_trace)", "0");
    addConfigItem("watch_file", "监视文件 (watch_file)", "");
    addConfigItem("watch_flits", "监视flits (watch_flits)", "");
    addConfigItem("watch_packets", "监视包 (watch_packets)", "");
    addConfigItem("watch_transactions", "监视事务 (watch_transactions)", "");
    addConfigItem("watch_out", "监视输出 (watch_out)", "");
    addConfigItem("stats_out", "统计输出 (stats_out)", "");
    addConfigItem("sim_power", "仿真功耗 (sim_power)", "0");
    addConfigItem("power_output_file", "功耗输出文件 (power_output_file)", "pwr_tmp");
    addConfigItem("tech_file", "技术文件 (tech_file)", "");
    addConfigItem("channel_width", "通道宽度 (channel_width)", "128");
    addConfigItem("channel_sweep", "通道扫描 (channel_sweep)", "0");

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

    connect(m_saveBtn, &ElaPushButton::clicked, this, &RouterGlobalConfigDialog::onSaveClicked);
    connect(m_cancelBtn, &ElaPushButton::clicked, this, &RouterGlobalConfigDialog::onCancelClicked);
}

void RouterGlobalConfigDialog::addConfigItem(const QString& key,
                                             const QString& label,
                                             const QString& defaultValue) {
    auto* labelWidget = new QLabel(label, this);
    labelWidget->setMinimumWidth(250);
    auto* edit = new ElaLineEdit(this);
    edit->setText(defaultValue);

    m_edits[key] = edit;

    auto* itemLayout = new QHBoxLayout();
    itemLayout->addWidget(labelWidget);
    itemLayout->addWidget(edit);

    m_mainLayout->addLayout(itemLayout);
}

void RouterGlobalConfigDialog::setConfig(const QMap<QString, QString>& config) {
    for (auto it = config.begin(); it != config.end(); ++it) {
        if (m_edits.contains(it.key())) {
            m_edits[it.key()]->setText(it.value());
        }
    }
}

QMap<QString, QString> RouterGlobalConfigDialog::getConfig() const {
    QMap<QString, QString> config;
    for (auto it = m_edits.begin(); it != m_edits.end(); ++it) {
        config[it.key()] = it.value()->text();
    }
    return config;
}

void RouterGlobalConfigDialog::onSaveClicked() {
    accept();
}

void RouterGlobalConfigDialog::onCancelClicked() {
    reject();
}
