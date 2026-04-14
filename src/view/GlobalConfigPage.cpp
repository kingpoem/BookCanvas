#include "GlobalConfigPage.h"
#include "component/RouterGlobalConfigDialog.h"
#include "utils/BooksimRoutingCatalog.h"
#include "utils/SelectableLabel.h"
#include <ElaComboBox.h>
#include <ElaLineEdit.h>
#include <ElaPushButton.h>
#include <ElaTheme.h>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPalette>
#include <QPlainTextEdit>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QScrollArea>
#include <QSet>
#include <QSignalBlocker>
#include <QTextStream>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidget>

namespace {
enum class InputKind {
    Text,
    Integer,
    Float,
    Boolean,
    Enum,
    RoutingPick,
};

struct InputSpec {
    InputKind kind = InputKind::Text;
    QStringList options;
    bool editable = false;
    QString placeholder;
};

[[nodiscard]] QString normalizeBoolLike(const QString& raw) {
    const QString t = raw.trimmed().toLower();
    if (t == QLatin1String("1") || t == QLatin1String("true") || t == QLatin1String("yes")
        || t == QLatin1String("on")) {
        return QStringLiteral("1");
    }
    if (t == QLatin1String("0") || t == QLatin1String("false") || t == QLatin1String("no")
        || t == QLatin1String("off")) {
        return QStringLiteral("0");
    }
    return raw.trimmed();
}

[[nodiscard]] InputSpec inputSpecForKey(const QString& key) {
    if (key == QLatin1String("routing_function")) {
        InputSpec spec;
        spec.kind = InputKind::RoutingPick;
        return spec;
    }
    static const QSet<QString> kBooleanKeys = {
        QStringLiteral("use_noc_latency"),
        QStringLiteral("noq"),
        QStringLiteral("wait_for_tail_credit"),
        QStringLiteral("vc_busy_when_full"),
        QStringLiteral("vc_prioritize_empty"),
        QStringLiteral("vc_priority_donation"),
        QStringLiteral("vc_shuffle_requests"),
        QStringLiteral("speculative"),
        QStringLiteral("spec_check_elig"),
        QStringLiteral("spec_check_cred"),
        QStringLiteral("spec_mask_by_reqs"),
        QStringLiteral("hold_switch_for_packet"),
        QStringLiteral("vct"),
        QStringLiteral("injection_rate_uses_flits"),
        QStringLiteral("use_read_write"),
        QStringLiteral("measure_stats"),
        QStringLiteral("pair_stats"),
        QStringLiteral("include_queuing"),
        QStringLiteral("print_activity"),
        QStringLiteral("print_csv_results"),
        QStringLiteral("viewer_trace"),
        QStringLiteral("sim_power"),
        QStringLiteral("channel_sweep"),
        QStringLiteral("chiplet_cdc_enable"),
        QStringLiteral("chiplet_cdc_gray_fifo"),
    };

    static const QSet<QString> kFloatKeys = {
        QStringLiteral("internal_speedup"),
        QStringLiteral("injection_rate"),
        QStringLiteral("burst_alpha"),
        QStringLiteral("burst_beta"),
        QStringLiteral("burst_r1"),
        QStringLiteral("write_fraction"),
        QStringLiteral("latency_thres"),
        QStringLiteral("warmup_thres"),
        QStringLiteral("acc_warmup_thres"),
        QStringLiteral("stopping_thres"),
        QStringLiteral("acc_stopping_thres"),
    };

    static const QSet<QString> kTextPathLikeKeys = {
        QStringLiteral("channel_file"),
        QStringLiteral("watch_file"),
        QStringLiteral("watch_out"),
        QStringLiteral("stats_out"),
        QStringLiteral("power_output_file"),
        QStringLiteral("tech_file"),
    };

    // 与画布「BookSim 拓扑」按钮顺序一致，末尾保留 anynet（自定义拓扑文件）
    static const QMap<QString, QStringList> kEnumOptions = {
        {QStringLiteral("chiplet_connect"), {QStringLiteral("x"), QStringLiteral("xy")}},
        {QStringLiteral("topology"),
         {QStringLiteral("mesh"),
          QStringLiteral("torus"),
          QStringLiteral("cmesh"),
          QStringLiteral("fly"),
          QStringLiteral("qtree"),
          QStringLiteral("tree4"),
          QStringLiteral("fattree"),
          QStringLiteral("flatfly"),
          QStringLiteral("dragonflynew"),
          QStringLiteral("chiplet_mesh"),
          QStringLiteral("anynet")}},
        {QStringLiteral("router"),
         {QStringLiteral("iq"), QStringLiteral("event"), QStringLiteral("chaos")}},
        {QStringLiteral("buffer_policy"), {QStringLiteral("private"), QStringLiteral("shared")}},
        {QStringLiteral("spec_sw_allocator"), {QStringLiteral("prio"), QStringLiteral("islip")}},
        {QStringLiteral("vc_allocator"), {QStringLiteral("islip"), QStringLiteral("prio")}},
        {QStringLiteral("sw_allocator"), {QStringLiteral("islip"), QStringLiteral("prio")}},
        {QStringLiteral("arb_type"),
         {QStringLiteral("round_robin"), QStringLiteral("matrix"), QStringLiteral("prio")}},
        {QStringLiteral("traffic"),
         {QStringLiteral("uniform"),
          QStringLiteral("bitcomp"),
          QStringLiteral("transpose"),
          QStringLiteral("tornado"),
          QStringLiteral("hotspot")}},
        {QStringLiteral("injection_process"),
         {QStringLiteral("bernoulli"), QStringLiteral("on_off")}},
        {QStringLiteral("priority"),
         {QStringLiteral("none"),
          QStringLiteral("class"),
          QStringLiteral("age"),
          QStringLiteral("network_age")}},
        {QStringLiteral("sim_type"), {QStringLiteral("latency"), QStringLiteral("throughput")}},
    };

    InputSpec spec;
    if (kBooleanKeys.contains(key)) {
        spec.kind = InputKind::Boolean;
        return spec;
    }
    if (kEnumOptions.contains(key)) {
        spec.kind = InputKind::Enum;
        spec.options = kEnumOptions.value(key);
        spec.editable = true;
        return spec;
    }
    if (kFloatKeys.contains(key)) {
        spec.kind = InputKind::Float;
        return spec;
    }
    if (kTextPathLikeKeys.contains(key)) {
        spec.kind = InputKind::Text;
        spec.placeholder = QStringLiteral("支持文件名或绝对路径");
        return spec;
    }
    spec.kind = InputKind::Integer;
    return spec;
}

[[nodiscard]] const QSet<QString>& networkTopologySectionKeys() {
    static const QSet<QString> k{QStringLiteral("channel_file"),
                                 QStringLiteral("topology"),
                                 QStringLiteral("k"),
                                 QStringLiteral("n"),
                                 QStringLiteral("c"),
                                 QStringLiteral("x"),
                                 QStringLiteral("y"),
                                 QStringLiteral("xr"),
                                 QStringLiteral("yr"),
                                 QStringLiteral("link_failures"),
                                 QStringLiteral("fail_seed"),
                                 QStringLiteral("in_ports"),
                                 QStringLiteral("out_ports")};
    return k;
}

[[nodiscard]] const QSet<QString>& chipletMeshSectionKeys() {
    static const QSet<QString> k{QStringLiteral("chiplet_connect"),
                                 QStringLiteral("chiplet_d2d_latency"),
                                 QStringLiteral("chiplet_intra_latency"),
                                 QStringLiteral("chiplet_cdc_enable"),
                                 QStringLiteral("chiplet_cdc_fifo_depth"),
                                 QStringLiteral("chiplet_cdc_sync_cycles"),
                                 QStringLiteral("chiplet_cdc_credit_sync_cycles"),
                                 QStringLiteral("chiplet_cdc_gray_fifo"),
                                 QStringLiteral("chiplet_cdc_gray_stages")};
    return k;
}

[[nodiscard]] QSet<QString> topologyStructureVisibleKeys(const QString& topologyId) {
    const QString t = topologyId.trimmed().toLower();
    QSet<QString> vis{QStringLiteral("channel_file"),
                      QStringLiteral("topology"),
                      QStringLiteral("link_failures"),
                      QStringLiteral("fail_seed"),
                      QStringLiteral("in_ports"),
                      QStringLiteral("out_ports")};
    if (t == QLatin1String("anynet")) {
        return vis;
    }
    vis.insert(QStringLiteral("k"));
    vis.insert(QStringLiteral("n"));
    vis.insert(QStringLiteral("c"));
    if (t == QLatin1String("cmesh") || t == QLatin1String("flatfly")) {
        vis.insert(QStringLiteral("x"));
        vis.insert(QStringLiteral("y"));
        vis.insert(QStringLiteral("xr"));
        vis.insert(QStringLiteral("yr"));
    }
    return vis;
}

[[nodiscard]] bool useNoCLatencyApplicableTopology(const QString& topologyId) {
    const QString t = topologyId.trimmed().toLower();
    return t == QLatin1String("mesh") || t == QLatin1String("torus") || t == QLatin1String("cmesh")
           || t == QLatin1String("flatfly") || t == QLatin1String("chiplet_mesh");
}

[[nodiscard]] bool configKeyUsesMergedValueWhenHidden(const QString& key,
                                                      const QString& topologyId) {
    if (key == QLatin1String("use_noc_latency")) {
        return !useNoCLatencyApplicableTopology(topologyId);
    }
    if (!networkTopologySectionKeys().contains(key)) {
        return false;
    }
    return !topologyStructureVisibleKeys(topologyId).contains(key);
}

[[nodiscard]] QString readInputValue(QWidget* input, const QString& key) {
    if (auto* edit = qobject_cast<ElaLineEdit*>(input)) {
        return edit->text().trimmed();
    }
    if (auto* combo = qobject_cast<ElaComboBox*>(input)) {
        if (key == QLatin1String("topology") || combo->isEditable()) {
            return combo->currentText().trimmed();
        }
        if (inputSpecForKey(key).kind == InputKind::RoutingPick) {
            const QString id = combo->currentData(Qt::UserRole).toString().trimmed();
            if (!id.isEmpty()) {
                return id;
            }
        }
        return combo->currentData(Qt::UserRole).toString().trimmed();
    }
    return QString();
}

[[nodiscard]] QString stripInlineComment(const QString& line) {
    bool inSingleQuote = false;
    bool inDoubleQuote = false;
    const int lineLength = static_cast<int>(line.size());
    for (int i = 0; i < lineLength; ++i) {
        const QChar ch = line.at(i);
        if (ch == QLatin1Char('"') && !inSingleQuote) {
            inDoubleQuote = !inDoubleQuote;
            continue;
        }
        if (ch == QLatin1Char('\'') && !inDoubleQuote) {
            inSingleQuote = !inSingleQuote;
            continue;
        }
        if (!inSingleQuote && !inDoubleQuote) {
            if (ch == QLatin1Char('#')) {
                return line.left(i).trimmed();
            }
            if (ch == QLatin1Char('/') && i + 1 < lineLength && line.at(i + 1) == QLatin1Char('/')) {
                return line.left(i).trimmed();
            }
        }
    }
    return line.trimmed();
}

[[nodiscard]] QString unquote(const QString& value) {
    if (value.size() < 2) {
        return value;
    }
    const QChar first = value.front();
    const QChar last = value.back();
    if ((first == QLatin1Char('"') && last == QLatin1Char('"'))
        || (first == QLatin1Char('\'') && last == QLatin1Char('\''))) {
        return value.mid(1, value.size() - 2).trimmed();
    }
    return value;
}
} // namespace

GlobalConfigPage::GlobalConfigPage(QWidget* parent)
    : BasePage(parent)
    , m_config(RouterGlobalConfigDialog::getDefaultConfig()) {
    setWindowTitle(tr("全局配置"));
    setupUi();
    setConfig(m_config);
}

void GlobalConfigPage::setupUi() {
    auto* centralWidget = new QWidget(this);
    m_pageRoot = centralWidget;
    centralWidget->setWindowTitle(tr("全局配置"));

    auto* root = new QVBoxLayout(centralWidget);
    root->setContentsMargins(0, 0, 20, 12);
    root->setSpacing(10);

    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    auto* scrollContent = new QWidget();
    m_formLayout = new QVBoxLayout(scrollContent);
    m_formLayout->setSpacing(10);

    addSectionTitle("网络拓扑参数");
    addConfigItem("channel_file", "通道文件 (channel_file)", "");
    addConfigItem("topology", "拓扑结构 (topology)", "anynet");
    addConfigItem("k", "每维基数 (k)", "3");
    addConfigItem("n", "维数 (n)", "2");
    addConfigItem("c", "集中度 (c)", "1");
    addConfigItem("x", "X维度 (x)", "8");
    addConfigItem("y", "Y维度 (y)", "8");
    addConfigItem("xr", "X路由器 (xr)", "1");
    addConfigItem("yr", "Y路由器 (yr)", "1");
    addConfigItem("link_failures", "链接失败 (link_failures)", "0");
    addConfigItem("fail_seed", "失败种子 (fail_seed)", "0");
    addConfigItem("in_ports", "输入端口 (in_ports)", "5");
    addConfigItem("out_ports", "输出端口 (out_ports)", "5");

    addSectionTitle("路由参数");
    addConfigItem("routing_function", "路由函数 (routing_function)", "min");
    addConfigItem("use_noc_latency", "使用NoC延迟 (use_noc_latency)", "1");

    addSectionTitle("路由器基础参数");
    addConfigItem("router", "路由器类型 (router)", "iq");
    addConfigItem("output_delay", "输出延迟 (output_delay)", "0");
    addConfigItem("credit_delay", "信用延迟 (credit_delay)", "1");
    addConfigItem("internal_speedup", "内部加速 (internal_speedup)", "1.0");
    addConfigItem("output_buffer_size", "输出缓冲大小 (output_buffer_size)", "-1");
    addConfigItem("noq", "无输出排队 (noq)", "0");

    addSectionTitle("虚拟通道配置");
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

    addSectionTitle("推测执行参数");
    addConfigItem("speculative", "推测分配 (speculative)", "0");
    addConfigItem("spec_check_elig", "推测检查资格 (spec_check_elig)", "1");
    addConfigItem("spec_check_cred", "推测检查信用 (spec_check_cred)", "1");
    addConfigItem("spec_mask_by_reqs", "推测掩码请求 (spec_mask_by_reqs)", "0");
    addConfigItem("spec_sw_allocator", "推测开关分配器 (spec_sw_allocator)", "prio");

    addSectionTitle("流水线延迟参数");
    addConfigItem("routing_delay", "路由延迟 (routing_delay)", "1");
    addConfigItem("vc_alloc_delay", "VC分配延迟 (vc_alloc_delay)", "1");
    addConfigItem("sw_alloc_delay", "开关分配延迟 (sw_alloc_delay)", "1");
    addConfigItem("st_prepare_delay", "ST准备延迟 (st_prepare_delay)", "0");
    addConfigItem("st_final_delay", "ST最终延迟 (st_final_delay)", "1");

    addSectionTitle("端口加速参数");
    addConfigItem("input_speedup", "输入加速 (input_speedup)", "1");
    addConfigItem("output_speedup", "输出加速 (output_speedup)", "1");
    addConfigItem("hold_switch_for_packet", "为包保持开关 (hold_switch_for_packet)", "0");

    addSectionTitle("Event-Driven 路由器参数");
    addConfigItem("vct", "VCT模式 (vct)", "0");

    addSectionTitle("分配器配置");
    addConfigItem("vc_allocator", "VC分配器 (vc_allocator)", "islip");
    addConfigItem("sw_allocator", "开关分配器 (sw_allocator)", "islip");
    addConfigItem("arb_type", "仲裁类型 (arb_type)", "round_robin");
    addConfigItem("alloc_iters", "分配迭代次数 (alloc_iters)", "1");

    addSectionTitle("流量配置");
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

    addSectionTitle("请求-应答流量配置");
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

    addSectionTitle(tr("芯粒 mesh — 跨 die（D2D / CDC）"));
    m_chipletMeshSectionCard = m_sections.isEmpty() ? nullptr : m_sections.last().card;
    addConfigItem("chiplet_connect", tr("芯粒互连方向 (chiplet_connect)"), "x");
    addConfigItem("chiplet_d2d_latency", tr("跨 die 线延迟 (chiplet_d2d_latency)"), "2");
    addConfigItem("chiplet_intra_latency", tr("片内默认跳延迟 (chiplet_intra_latency)"), "1");
    addConfigItem("chiplet_cdc_enable", tr("启用跨时钟域 D2D (chiplet_cdc_enable)"), "0");
    addConfigItem("chiplet_cdc_fifo_depth", tr("CDC异步 FIFO 深度 (chiplet_cdc_fifo_depth)"), "64");
    addConfigItem("chiplet_cdc_sync_cycles", tr("CDC flit 同步周期 (chiplet_cdc_sync_cycles)"), "2");
    addConfigItem("chiplet_cdc_credit_sync_cycles",
                  tr("CDC credit 同步周期 (chiplet_cdc_credit_sync_cycles)"),
                  "2");
    addConfigItem("chiplet_cdc_gray_fifo", tr("CDC Gray 指针 FIFO (chiplet_cdc_gray_fifo)"), "0");
    addConfigItem("chiplet_cdc_gray_stages", tr("CDC Gray 级数 (chiplet_cdc_gray_stages)"), "2");

    addSectionTitle("仿真控制参数");
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

    addSectionTitle("功耗相关参数");
    addConfigItem("sim_power", "仿真功耗 (sim_power)", "0");
    addConfigItem("power_output_file", "功耗输出文件 (power_output_file)", "pwr_tmp");
    addConfigItem("tech_file", "技术文件 (tech_file)", "");
    addConfigItem("channel_width", "通道宽度 (channel_width)", "128");
    addConfigItem("channel_sweep", "通道扫描 (channel_sweep)", "0");

    if (m_activeSectionLayout) {
        m_activeSectionLayout->addStretch(1);
    }
    m_formLayout->addStretch(1);
    m_scrollArea->setWidget(scrollContent);
    root->addWidget(m_scrollArea);

    auto* buttonRow = new QHBoxLayout();
    m_viewRawBtn = new ElaPushButton(tr("查看原始配置文件"), this);
    m_importBtn = new ElaPushButton(tr("导入配置"), this);
    m_resetBtn = new ElaPushButton(tr("恢复默认"), this);
    buttonRow->addWidget(m_viewRawBtn);
    buttonRow->addWidget(m_importBtn);
    buttonRow->addStretch();
    buttonRow->addWidget(m_resetBtn);
    root->addLayout(buttonRow);
    root->addSpacing(6);

    connect(m_viewRawBtn,
            &ElaPushButton::clicked,
            this,
            &GlobalConfigPage::onViewRawConfigFileClicked);
    connect(m_importBtn, &ElaPushButton::clicked, this, &GlobalConfigPage::onImportConfigClicked);
    connect(m_resetBtn, &ElaPushButton::clicked, this, [this]() {
        const auto answer = QMessageBox::question(this,
                                                  tr("恢复默认"),
                                                  tr("确定将全局配置恢复为默认参数吗？"));
        if (answer == QMessageBox::Yes) {
            setConfig(RouterGlobalConfigDialog::getDefaultConfig());
        }
    });
    connect(eTheme, &ElaTheme::themeModeChanged, this, [this](ElaThemeType::ThemeMode) {
        applyTheme();
    });
    wireTopologyRoutingForGlobalConfig();
    refreshTopologyFieldVisibility();
    addCentralWidget(centralWidget, true, true, 0);
    applyTheme();
}

void GlobalConfigPage::wireTopologyRoutingForGlobalConfig() {
    if (m_topologyRoutingWired) {
        return;
    }
    auto* topoInput = m_inputs.value(QStringLiteral("topology"));
    auto* routeInput = m_inputs.value(QStringLiteral("routing_function"));
    auto* topo = qobject_cast<ElaComboBox*>(topoInput);
    auto* route = qobject_cast<ElaComboBox*>(routeInput);
    if (!topo || !route) {
        return;
    }
    if (inputSpecForKey(QStringLiteral("routing_function")).kind != InputKind::RoutingPick) {
        return;
    }
    m_topologyRoutingWired = true;
    connect(topo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
        refreshGlobalRoutingComboFromUiConfig();
        refreshTopologyFieldVisibility();
    });
    connect(topo, &QComboBox::currentTextChanged, this, [this](const QString&) {
        refreshGlobalRoutingComboFromUiConfig();
        refreshTopologyFieldVisibility();
    });
}

QString GlobalConfigPage::currentTopologyId() const {
    auto* tcombo = qobject_cast<ElaComboBox*>(m_inputs.value(QStringLiteral("topology")));
    if (!tcombo) {
        return QStringLiteral("anynet");
    }
    const QString td = tcombo->currentData(Qt::UserRole).toString().trimmed().toLower();
    QString topo = td.isEmpty() ? tcombo->currentText().trimmed().toLower() : td;
    return topo.isEmpty() ? QStringLiteral("anynet") : topo;
}

void GlobalConfigPage::refreshTopologyFieldVisibility() {
    auto mergeUi = [this](const QString& key) {
        QWidget* w = m_inputs.value(key);
        if (!w) {
            return;
        }
        QString value = readInputValue(w, key);
        if (inputSpecForKey(key).kind == InputKind::Boolean) {
            value = normalizeBoolLike(value);
            if (value != QLatin1String("0") && value != QLatin1String("1")) {
                value = QStringLiteral("0");
            }
        }
        m_config[key] = value;
    };
    for (const QString& key : networkTopologySectionKeys()) {
        mergeUi(key);
    }
    mergeUi(QStringLiteral("use_noc_latency"));
    for (const QString& key : chipletMeshSectionKeys()) {
        if (QWidget* w = m_inputs.value(key)) {
            QString value = readInputValue(w, key);
            if (inputSpecForKey(key).kind == InputKind::Boolean) {
                value = normalizeBoolLike(value);
                if (value != QLatin1String("0") && value != QLatin1String("1")) {
                    value = QStringLiteral("0");
                }
            }
            m_config[key] = value;
        }
    }

    const QString topo = currentTopologyId();
    const QSet<QString> structVis = topologyStructureVisibleKeys(topo);
    for (const QString& key : networkTopologySectionKeys()) {
        if (QWidget* row = m_inputRows.value(key)) {
            row->setVisible(structVis.contains(key));
        }
    }
    if (QWidget* row = m_inputRows.value(QStringLiteral("use_noc_latency"))) {
        row->setVisible(useNoCLatencyApplicableTopology(topo));
    }
    if (m_chipletMeshSectionCard) {
        m_chipletMeshSectionCard->setVisible(topo == QLatin1String("chiplet_mesh"));
    }

    const QStringList lockableKeys{QStringLiteral("k"),
                                   QStringLiteral("n"),
                                   QStringLiteral("c"),
                                   QStringLiteral("x"),
                                   QStringLiteral("y"),
                                   QStringLiteral("xr"),
                                   QStringLiteral("yr")};
    for (const QString& key : lockableKeys) {
        if (QWidget* w = m_inputs.value(key)) {
            w->setEnabled(true);
        }
    }
    auto forceTopologyField = [this](const QString& key, const QString& v) {
        if (auto* edit = qobject_cast<ElaLineEdit*>(m_inputs.value(key))) {
            const QSignalBlocker b(edit);
            edit->setText(v);
            m_config[key] = v;
        }
    };
    auto setFieldEnabled = [this](const QString& key, bool en) {
        if (QWidget* w = m_inputs.value(key)) {
            w->setEnabled(en);
        }
    };
    if (topo == QLatin1String("cmesh")) {
        forceTopologyField(QStringLiteral("n"), QStringLiteral("2"));
        forceTopologyField(QStringLiteral("c"), QStringLiteral("4"));
        setFieldEnabled(QStringLiteral("n"), false);
        setFieldEnabled(QStringLiteral("c"), false);
    } else if (topo == QLatin1String("qtree") || topo == QLatin1String("tree4")) {
        forceTopologyField(QStringLiteral("k"), QStringLiteral("4"));
        forceTopologyField(QStringLiteral("n"), QStringLiteral("3"));
        setFieldEnabled(QStringLiteral("k"), false);
        setFieldEnabled(QStringLiteral("n"), false);
    } else if (topo == QLatin1String("dragonflynew")) {
        forceTopologyField(QStringLiteral("n"), QStringLiteral("1"));
        setFieldEnabled(QStringLiteral("n"), false);
    }
}

void GlobalConfigPage::refreshGlobalRoutingComboFromUiConfig(const QString& preferredRouting) {
    auto* rcombo = qobject_cast<ElaComboBox*>(m_inputs.value(QStringLiteral("routing_function")));
    if (!rcombo
        || inputSpecForKey(QStringLiteral("routing_function")).kind != InputKind::RoutingPick) {
        return;
    }
    auto* tcombo = qobject_cast<ElaComboBox*>(m_inputs.value(QStringLiteral("topology")));
    QString topo = QStringLiteral("anynet");
    if (tcombo) {
        const QString td = tcombo->currentData(Qt::UserRole).toString().trimmed().toLower();
        topo = td.isEmpty() ? tcombo->currentText().trimmed().toLower() : td;
        if (topo.isEmpty()) {
            topo = QStringLiteral("anynet");
        }
    }
    QString keep = preferredRouting.trimmed();
    if (keep.isEmpty()) {
        keep = rcombo->currentData(Qt::UserRole).toString().trimmed();
        if (keep.isEmpty()) {
            keep = rcombo->currentText().trimmed();
        }
    }
    const QSignalBlocker blocker(rcombo);
    rcombo->clear();
    const QStringList ids = routingIdsForTopology(topo);
    for (const QString& id : ids) {
        rcombo->addItem(routingUiLabel(topo, id), id);
    }
    if (rcombo->count() == 0) {
        const QString fb = defaultRoutingIdForTopology(topo);
        rcombo->addItem(routingUiLabel(topo, fb), fb);
    }
    const QString pick = normalizeRoutingForTopology(keep, topo);
    const int idx = rcombo->findData(pick);
    rcombo->setCurrentIndex(idx >= 0 ? idx : 0);
}

void GlobalConfigPage::setConfig(const QMap<QString, QString>& config) {
    m_config = config;
    const QMap<QString, QString> fallbacks = RouterGlobalConfigDialog::getDefaultConfig();
    for (auto it = m_inputs.begin(); it != m_inputs.end(); ++it) {
        const QString& key = it.key();
        if (key == QLatin1String("routing_function")) {
            continue;
        }
        QString raw = m_config.value(key);
        if (raw.trimmed().isEmpty()
            && (key == QLatin1String("k") || key == QLatin1String("n")
                || key == QLatin1String("c"))) {
            raw = fallbacks.value(key);
        }
        if (auto* edit = qobject_cast<ElaLineEdit*>(it.value())) {
            edit->setText(raw);
            continue;
        }
        auto* combo = qobject_cast<ElaComboBox*>(it.value());
        if (!combo) {
            continue;
        }
        const InputSpec spec = inputSpecForKey(key);
        if (spec.kind == InputKind::Boolean) {
            const QString normalized = normalizeBoolLike(raw);
            const int idx = combo->findData(normalized);
            combo->setCurrentIndex((idx >= 0) ? idx : 0);
            continue;
        }
        const int byData = combo->findData(raw);
        if (byData >= 0) {
            combo->setCurrentIndex(byData);
            continue;
        }
        const int byText = combo->findText(raw);
        if (byText >= 0) {
            combo->setCurrentIndex(byText);
            continue;
        }
        if (combo->isEditable()) {
            combo->setEditText(raw);
        }
    }
    refreshGlobalRoutingComboFromUiConfig(m_config.value(QStringLiteral("routing_function")));
    refreshTopologyFieldVisibility();
}

void GlobalConfigPage::addSectionTitle(const QString& title) {
    auto* card = new QFrame(this);
    card->setObjectName(QStringLiteral("globalConfigSectionCard"));
    auto* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(10, 8, 10, 10);
    cardLayout->setSpacing(8);

    auto* toggle = new QToolButton(card);
    toggle->setObjectName(QStringLiteral("globalConfigSectionToggle"));
    toggle->setText(title);
    toggle->setCheckable(true);
    toggle->setChecked(false);
    toggle->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    toggle->setArrowType(Qt::RightArrow);
    toggle->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    cardLayout->addWidget(toggle);

    auto* body = new QWidget(card);
    body->setObjectName(QStringLiteral("globalConfigSectionBody"));
    auto* bodyLayout = new QVBoxLayout(body);
    bodyLayout->setContentsMargins(0, 0, 0, 0);
    bodyLayout->setSpacing(6);
    body->setVisible(false);
    cardLayout->addWidget(body);

    connect(toggle, &QToolButton::toggled, this, [toggle, body](bool on) {
        body->setVisible(on);
        toggle->setArrowType(on ? Qt::DownArrow : Qt::RightArrow);
    });

    SectionUi sec;
    sec.card = card;
    sec.body = body;
    sec.toggle = toggle;
    sec.bodyLayout = bodyLayout;
    m_sections.append(sec);
    m_activeSectionLayout = bodyLayout;
    m_formLayout->addWidget(card);
}

void GlobalConfigPage::addConfigItem(const QString& key,
                                     const QString& label,
                                     const QString& defaultValue) {
    auto* labelWidget = new QLabel(label, this);
    labelWidget->setMinimumWidth(250);
    applySelectableLabelText(labelWidget);

    const InputSpec spec = inputSpecForKey(key);
    QWidget* input = nullptr;
    if (spec.kind == InputKind::Boolean) {
        auto* combo = new ElaComboBox(this);
        combo->addItem(tr("0（关闭）"), QStringLiteral("0"));
        combo->addItem(tr("1（开启）"), QStringLiteral("1"));
        const QString normalized = normalizeBoolLike(defaultValue);
        const int idx = combo->findData(normalized);
        combo->setCurrentIndex((idx >= 0) ? idx : 0);
        combo->setToolTip(tr("支持 0/1、true/false、yes/no 格式"));
        input = combo;
    } else if (spec.kind == InputKind::Enum) {
        auto* combo = new ElaComboBox(this);
        for (const QString& option : spec.options) {
            combo->addItem(option, option);
        }
        combo->setEditable(spec.editable);
        const int idx = combo->findData(defaultValue);
        if (idx >= 0) {
            combo->setCurrentIndex(idx);
        } else if (combo->isEditable()) {
            combo->setEditText(defaultValue);
        }
        input = combo;
    } else if (spec.kind == InputKind::RoutingPick) {
        auto* combo = new ElaComboBox(this);
        combo->setSizeAdjustPolicy(QComboBox::AdjustToContents);
        combo->setToolTip(tr(
            "选项随「拓扑结构」变化；保存值为 BookSim routing_function 基名（不含 _拓扑 后缀）。"));
        input = combo;
    } else {
        auto* edit = new ElaLineEdit(this);
        edit->setText(defaultValue);
        if (!spec.placeholder.isEmpty()) {
            edit->setPlaceholderText(spec.placeholder);
        }
        if (spec.kind == InputKind::Integer) {
            auto* validator = new QRegularExpressionValidator(QRegularExpression(
                                                                  QStringLiteral("^-?\\d+$")),
                                                              edit);
            edit->setValidator(validator);
        } else if (spec.kind == InputKind::Float) {
            auto* validator = new QRegularExpressionValidator(QRegularExpression(QStringLiteral(
                                                                  "^-?(?:\\d+\\.?\\d*|\\.\\d+)$")),
                                                              edit);
            edit->setValidator(validator);
        }
        input = edit;
    }
    m_inputs[key] = input;

    QWidget* rowParent = m_formLayout ? m_formLayout->parentWidget() : nullptr;
    if (!rowParent) {
        rowParent = this;
    }
    auto* row = new QWidget(rowParent);
    auto* itemLayout = new QHBoxLayout(row);
    itemLayout->setContentsMargins(0, 0, 0, 0);
    itemLayout->addWidget(labelWidget);
    itemLayout->addWidget(input);
    m_inputRows.insert(key, row);
    if (m_activeSectionLayout) {
        m_activeSectionLayout->addWidget(row);
    } else {
        m_formLayout->addWidget(row);
    }
}

QMap<QString, QString> GlobalConfigPage::collectConfigFromUi() const {
    const QString topo = currentTopologyId();
    const QMap<QString, QString> fallbacks = RouterGlobalConfigDialog::getDefaultConfig();
    QMap<QString, QString> config;
    for (auto it = m_inputs.begin(); it != m_inputs.end(); ++it) {
        const QString& key = it.key();
        if (chipletMeshSectionKeys().contains(key) && topo != QLatin1String("chiplet_mesh")) {
            config[key] = m_config.value(key, fallbacks.value(key));
            continue;
        }
        if (configKeyUsesMergedValueWhenHidden(key, topo)) {
            const QString merged = m_config.value(key, fallbacks.value(key));
            config[key] = merged;
            continue;
        }
        QString value = readInputValue(it.value(), key);
        if (inputSpecForKey(key).kind == InputKind::Boolean) {
            value = normalizeBoolLike(value);
            if (value != QLatin1String("0") && value != QLatin1String("1")) {
                value = QStringLiteral("0");
            }
        }
        config[key] = value;
    }
    if (config.contains(QStringLiteral("routing_function"))
        && config.contains(QStringLiteral("topology"))) {
        config.insert(QStringLiteral("routing_function"),
                      normalizeRoutingForTopology(config.value(QStringLiteral("routing_function")),
                                                  config.value(QStringLiteral("topology"))));
    }
    const QString nfKey = QStringLiteral("network_file");
    if (!config.contains(nfKey)) {
        config.insert(nfKey, m_config.value(nfKey, fallbacks.value(nfKey)));
    }
    config.insert(QStringLiteral("subnets"), QStringLiteral("1"));
    return config;
}

QMap<QString, QString> GlobalConfigPage::collectCurrentConfig() {
    m_config = collectConfigFromUi();
    return m_config;
}

void GlobalConfigPage::applyChipletMeshTopologyAndRouting() {
    auto* tcombo = qobject_cast<ElaComboBox*>(m_inputs.value(QStringLiteral("topology")));
    if (!tcombo) {
        return;
    }
    const int idxMesh = tcombo->findData(QStringLiteral("chiplet_mesh"));
    if (idxMesh >= 0) {
        const QSignalBlocker b(tcombo);
        tcombo->setCurrentIndex(idxMesh);
    }
    refreshGlobalRoutingComboFromUiConfig(QStringLiteral("dim_order_chiplet_mesh"));
    refreshTopologyFieldVisibility();
}

void GlobalConfigPage::onViewRawConfigFileClicked() {
    const QMap<QString, QString> currentConfig = collectConfigFromUi();
    QStringList rawLines;
    rawLines.reserve(static_cast<int>(currentConfig.size()));
    for (auto it = currentConfig.begin(); it != currentConfig.end(); ++it) {
        rawLines.append(QStringLiteral("%1 = %2;").arg(it.key(), it.value()));
    }
    showRawConfigContent(tr("当前全局配置（原始文本）"), rawLines.join(QLatin1Char('\n')));
}

void GlobalConfigPage::onImportConfigClicked() {
    const QString filePath
        = QFileDialog::getOpenFileName(this,
                                       tr("导入配置文件"),
                                       QString(),
                                       tr("配置文件 (*.cfg *.txt *.conf);;所有文件 (*)"));
    if (filePath.isEmpty()) {
        return;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this,
                             tr("导入失败"),
                             tr("无法打开配置文件：\n%1").arg(QFileInfo(filePath).fileName()));
        return;
    }
    QTextStream stream(&file);
    const QString content = stream.readAll();
    file.close();

    const QMap<QString, QString> parsedConfig = parseRawConfigText(content);
    if (parsedConfig.isEmpty()) {
        QMessageBox::information(this,
                                 tr("导入配置"),
                                 tr("未解析到有效参数。请检查文件格式是否为 key = value;"));
        return;
    }

    QMap<QString, QString> merged = collectConfigFromUi();
    int appliedCount = 0;
    int ignoredCount = 0;
    for (auto it = parsedConfig.begin(); it != parsedConfig.end(); ++it) {
        if (m_inputs.contains(it.key())) {
            merged[it.key()] = it.value();
            ++appliedCount;
        } else {
            ++ignoredCount;
        }
    }

    if (appliedCount <= 0) {
        QMessageBox::information(this, tr("导入配置"), tr("未匹配到当前页面支持的参数。"));
        return;
    }

    setConfig(merged);
    QMessageBox::information(this,
                             tr("导入成功"),
                             tr("已导入 %1 个参数，忽略 %2 个未支持参数。")
                                 .arg(appliedCount)
                                 .arg(ignoredCount));
}

void GlobalConfigPage::showRawConfigContent(const QString& title, const QString& content) {
    auto* dialog = new QDialog(this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setWindowTitle(title);
    dialog->resize(860, 620);

    auto* layout = new QVBoxLayout(dialog);
    auto* editor = new QPlainTextEdit(dialog);
    editor->setReadOnly(true);
    editor->setPlainText(content);
    layout->addWidget(editor);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Close, dialog);
    connect(buttons, &QDialogButtonBox::rejected, dialog, &QDialog::close);
    layout->addWidget(buttons);
    dialog->open();
}

QMap<QString, QString> GlobalConfigPage::parseRawConfigText(const QString& text) {
    QMap<QString, QString> parsed;
    const QStringList lines = text.split(QRegularExpression(QStringLiteral("[\r\n]+")),
                                         Qt::SkipEmptyParts);
    for (const QString& rawLine : lines) {
        const QString line = stripInlineComment(rawLine).trimmed();
        if (line.isEmpty()) {
            continue;
        }
        const qsizetype eqPos = line.indexOf(QLatin1Char('='));
        if (eqPos <= 0) {
            continue;
        }
        const QString key = line.left(eqPos).trimmed();
        if (key.isEmpty()) {
            continue;
        }
        QString value = line.mid(eqPos + 1).trimmed();
        while (!value.isEmpty()
               && (value.endsWith(QLatin1Char(';')) || value.endsWith(QLatin1Char(',')))) {
            value.chop(1);
            value = value.trimmed();
        }
        value = unquote(value);
        if (value.isEmpty()) {
            continue;
        }
        parsed.insert(key, value);
    }
    return parsed;
}

void GlobalConfigPage::applyTheme() {
    if (!m_pageRoot) {
        return;
    }

    const auto mode = eTheme->getThemeMode();
    const QColor pageBg = ElaThemeColor(mode, WindowBase);
    const QColor textMain = (mode == ElaThemeType::Light) ? QColor(Qt::black)
                                                          : ElaThemeColor(mode, BasicText);
    const QColor border = ElaThemeColor(mode, BasicBorder);

    m_pageRoot->setAttribute(Qt::WA_StyledBackground, true);
    m_pageRoot->setStyleSheet(
        QStringLiteral("#ElaScrollPage_CentralPage { background-color: %1; } "
                       "QLabel { color: %2; } "
                       "QFrame#globalConfigSectionCard { border: 1px solid %3; "
                       "border-radius: 10px; background: %1; } "
                       "QToolButton#globalConfigSectionToggle { border: none; "
                       "background: transparent; color: %2; font-weight: 600; "
                       "text-align: left; padding: 2px 0; }")
            .arg(pageBg.name(QColor::HexRgb),
                 textMain.name(QColor::HexRgb),
                 border.name(QColor::HexRgb)));

    if (QWidget* outerVp = m_pageRoot->parentWidget()) {
        outerVp->setAutoFillBackground(true);
        QPalette op = outerVp->palette();
        op.setColor(QPalette::Window, pageBg);
        outerVp->setPalette(op);
    }

    if (m_scrollArea) {
        m_scrollArea->setStyleSheet(
            QStringLiteral("QScrollArea { background: transparent; border: 1px solid %1; "
                           "border-radius: 8px; }")
                .arg(border.name(QColor::HexRgb)));
        if (QWidget* vp = m_scrollArea->viewport()) {
            vp->setStyleSheet(QStringLiteral("background: transparent;"));
        }
    }
}
