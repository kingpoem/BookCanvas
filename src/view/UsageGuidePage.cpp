#include "UsageGuidePage.h"
#include "utils/BookSimMetricLabels.h"
#include <ElaLineEdit.h>
#include <ElaPushButton.h>
#include <ElaText.h>
#include <ElaTheme.h>
#include <QAbstractTextDocumentLayout>
#include <QApplication>
#include <QClipboard>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QMap>
#include <QScrollArea>
#include <QTextBrowser>
#include <QTextDocument>
#include <QToolButton>
#include <QVBoxLayout>
#include <QtMath>

namespace {

QString meshTemplate() {
    return QStringLiteral("{\n"
                          "  \"topology\": \"mesh\",\n"
                          "  \"k\": 4,\n"
                          "  \"n\": 2,\n"
                          "  \"c\": 1,\n"
                          "  \"routing_function\": \"dor\",\n"
                          "  \"traffic\": \"uniform\",\n"
                          "  \"injection_rate\": 0.05,\n"
                          "  \"num_vcs\": 8,\n"
                          "  \"vc_buf_size\": 16,\n"
                          "  \"sim_type\": \"latency\"\n"
                          "}");
}

QString torusTemplate() {
    return QStringLiteral("{\n"
                          "  \"topology\": \"torus\",\n"
                          "  \"k\": 4,\n"
                          "  \"n\": 2,\n"
                          "  \"c\": 1,\n"
                          "  \"routing_function\": \"dim_order\",\n"
                          "  \"traffic\": \"uniform\",\n"
                          "  \"injection_rate\": 0.04,\n"
                          "  \"num_vcs\": 8,\n"
                          "  \"vc_buf_size\": 16,\n"
                          "  \"sim_type\": \"latency\"\n"
                          "}");
}

QString cmeshTemplate() {
    return QStringLiteral("{\n"
                          "  \"topology\": \"cmesh\",\n"
                          "  \"k\": 4,\n"
                          "  \"n\": 2,\n"
                          "  \"c\": 4,\n"
                          "  \"routing_function\": \"dor_no_express\",\n"
                          "  \"traffic\": \"uniform\",\n"
                          "  \"injection_rate\": 0.03,\n"
                          "  \"num_vcs\": 8,\n"
                          "  \"vc_buf_size\": 16,\n"
                          "  \"sim_type\": \"latency\"\n"
                          "}");
}

QString flyTemplate() {
    return QStringLiteral("{\n"
                          "  \"topology\": \"fly\",\n"
                          "  \"k\": 4,\n"
                          "  \"n\": 2,\n"
                          "  \"routing_function\": \"dest_tag\",\n"
                          "  \"traffic\": \"uniform\",\n"
                          "  \"injection_rate\": 0.03,\n"
                          "  \"num_vcs\": 8,\n"
                          "  \"vc_buf_size\": 16,\n"
                          "  \"sim_type\": \"latency\"\n"
                          "}");
}

QString qtreeTemplate() {
    return QStringLiteral("{\n"
                          "  \"topology\": \"qtree\",\n"
                          "  \"k\": 4,\n"
                          "  \"n\": 3,\n"
                          "  \"routing_function\": \"nca\",\n"
                          "  \"traffic\": \"uniform\",\n"
                          "  \"injection_rate\": 0.02,\n"
                          "  \"num_vcs\": 8,\n"
                          "  \"vc_buf_size\": 16,\n"
                          "  \"sim_type\": \"latency\"\n"
                          "}");
}

QString tree4Template() {
    return QStringLiteral("{\n"
                          "  \"topology\": \"tree4\",\n"
                          "  \"k\": 4,\n"
                          "  \"n\": 3,\n"
                          "  \"routing_function\": \"nca\",\n"
                          "  \"traffic\": \"uniform\",\n"
                          "  \"injection_rate\": 0.02,\n"
                          "  \"num_vcs\": 8,\n"
                          "  \"vc_buf_size\": 16,\n"
                          "  \"sim_type\": \"latency\"\n"
                          "}");
}

QString fattreeTemplate() {
    return QStringLiteral("{\n"
                          "  \"topology\": \"fattree\",\n"
                          "  \"k\": 4,\n"
                          "  \"n\": 2,\n"
                          "  \"routing_function\": \"nca\",\n"
                          "  \"traffic\": \"uniform\",\n"
                          "  \"injection_rate\": 0.02,\n"
                          "  \"num_vcs\": 8,\n"
                          "  \"vc_buf_size\": 16,\n"
                          "  \"sim_type\": \"latency\"\n"
                          "}");
}

QString flatflyTemplate() {
    return QStringLiteral("{\n"
                          "  \"topology\": \"flatfly\",\n"
                          "  \"k\": 4,\n"
                          "  \"n\": 2,\n"
                          "  \"c\": 1,\n"
                          "  \"x\": 4,\n"
                          "  \"y\": 4,\n"
                          "  \"xr\": 1,\n"
                          "  \"yr\": 1,\n"
                          "  \"routing_function\": \"ran_min\",\n"
                          "  \"traffic\": \"uniform\",\n"
                          "  \"injection_rate\": 0.02,\n"
                          "  \"num_vcs\": 8,\n"
                          "  \"vc_buf_size\": 16,\n"
                          "  \"sim_type\": \"latency\"\n"
                          "}");
}

QString dragonflyTemplate() {
    return QStringLiteral("{\n"
                          "  \"topology\": \"dragonflynew\",\n"
                          "  \"k\": 2,\n"
                          "  \"n\": 1,\n"
                          "  \"routing_function\": \"min\",\n"
                          "  \"traffic\": \"uniform\",\n"
                          "  \"injection_rate\": 0.02,\n"
                          "  \"num_vcs\": 3,\n"
                          "  \"vc_buf_size\": 16,\n"
                          "  \"sim_type\": \"latency\"\n"
                          "}");
}

QString anynetTemplate() {
    return QStringLiteral("{\n"
                          "  \"topology\": \"anynet\",\n"
                          "  \"network_file\": \"anynet_file\",\n"
                          "  \"routing_function\": \"min\",\n"
                          "  \"traffic\": \"uniform\",\n"
                          "  \"injection_rate\": 0.03,\n"
                          "  \"num_vcs\": 8,\n"
                          "  \"vc_buf_size\": 16,\n"
                          "  \"sim_type\": \"latency\"\n"
                          "}");
}

struct ConfigParamGroup {
    QString title;
    QString summary;
    QString searchable;
    QStringList keys;
};

[[nodiscard]] QString configParamSummary(const QString& key) {
    static const QMap<QString, QString> kSpecific{
        {QStringLiteral("topology"),
         QStringLiteral("选定 BookSim 拓扑构造器；一改即改边集、端口度分布与合法路由注册名。")},
        {QStringLiteral("channel_file"),
         QStringLiteral("逐通道线长或延迟表；空则拓扑用内置默认跳延迟。")},
        {QStringLiteral("k"), QStringLiteral("k-ary 结构里每维 radix，mesh 上常等于单边格宽。")},
        {QStringLiteral("n"), QStringLiteral("维数：与 k 一起把路由器总数锁进 k^n 型增长曲线。")},
        {QStringLiteral("c"),
         QStringLiteral("终端集中度：一路由器下挂的 IP/核数，>1 时启用 x/y/xr/yr 几何。")},
        {QStringLiteral("x"),
         QStringLiteral("片上网格：沿 X 向铺开的逻辑路由器列数，供几何链路延迟用。")},
        {QStringLiteral("y"),
         QStringLiteral("片上网格：沿 Y 向铺开的逻辑路由器行数，与 x 构成平面规模。")},
        {QStringLiteral("xr"),
         QStringLiteral("每路由器在局部 X 向挂靠的终端数，只在 c>1 时参与几何。")},
        {QStringLiteral("yr"),
         QStringLiteral("每路由器在局部 Y 向挂靠的终端数，与 xr 拼出终端子网格。")},
        {QStringLiteral("routing_function"),
         QStringLiteral("路由算法注册短名；运行期自动拼 _拓扑 进工厂表。")},
        {QStringLiteral("use_noc_latency"),
         QStringLiteral("链路延迟按平面几何缩放，替代「每跳固定1 cycle」的抽象。")},
        {QStringLiteral("noq"),
         QStringLiteral("在出口侧为下一跳输入建镜像队列，反压落在 hop 边界另一侧。")},
        {QStringLiteral("wait_for_tail_credit"),
         QStringLiteral("尾片 credit 回到上游后才释放 VC，避免分组尾与头在时间上重叠。")},
        {QStringLiteral("vc_busy_when_full"),
         QStringLiteral("credit 用尽仍标 busy，VA 眼里该 VC 继续占位。")},
        {QStringLiteral("vc_prioritize_empty"),
         QStringLiteral("新分组优先占空 VC，换更干净的队首但可能浪费槽位。")},
        {QStringLiteral("vc_priority_donation"),
         QStringLiteral("高优先级把仲裁权让给挡路低优先级头包，削弱 VA 饥饿。")},
        {QStringLiteral("vc_shuffle_requests"),
         QStringLiteral("VA 入口前对请求随机重排，削弱端口编号引起的长期优先级倾斜。")},
        {QStringLiteral("speculative"),
         QStringLiteral("VC 分配尚未最终确定时可先行发起交换分配（推测式 SA），以缩短流水线"
                        "气泡；非法授予由 spec_check_* 约束。")},
        {QStringLiteral("spec_check_elig"),
         QStringLiteral("投机 grant 前再验输出 VC 仍在路由合法集内。")},
        {QStringLiteral("spec_check_cred"),
         QStringLiteral("投机 grant 前是否数下游空槽，省算力但有回滚风险。")},
        {QStringLiteral("spec_mask_by_reqs"),
         QStringLiteral("用本周期真实 SA 掩码裁剪投机候选，减少假阳性。")},
        {QStringLiteral("hold_switch_for_packet"),
         QStringLiteral("分组传输期间保持同一交叉开关配置，降低 SA 频次、增加时间槽占用。")},
        {QStringLiteral("vct"), QStringLiteral("头片到下游即可订缓冲，不必 wormhole 占满路径。")},
        {QStringLiteral("injection_rate_uses_flits"),
         QStringLiteral("注入率口径在「每周期 flit」与「每周期包」之间切换。")},
        {QStringLiteral("use_read_write"),
         QStringLiteral("四类事务（读/写请求与应答）分 VC、分子网建模。")},
        {QStringLiteral("measure_stats"),
         QStringLiteral("按类开关：该类是否进入延迟/吞吐统计管线。")},
        {QStringLiteral("pair_stats"),
         QStringLiteral("源–宿全对统计，适于诊断；内存与存储随节点数平方增长。")},
        {QStringLiteral("include_queuing"),
         QStringLiteral("plat 是否把源端等注入的队列时间算进端到端。")},
        {QStringLiteral("print_activity"),
         QStringLiteral("周期 dump路由器内部活动，日志可轻易撑爆磁盘。")},
        {QStringLiteral("print_csv_results"),
         QStringLiteral("把汇总指标写成 CSV，方便 matplotlib/R 画图。")},
        {QStringLiteral("viewer_trace"),
         QStringLiteral("吐纳 BookSim 外部动画/波形 viewer 的事件流。")},
        {QStringLiteral("sim_power"), QStringLiteral("跑仿真同时按工艺表估路由器与线网能耗。")},
        {QStringLiteral("router"),
         QStringLiteral("微架构后端：iq 周期流水、event 事件驱动、chaos 专用模型。")},
        {QStringLiteral("internal_speedup"),
         QStringLiteral("片内逻辑比片外链路时钟快多少倍，浮点缩放。")},
        {QStringLiteral("output_buffer_size"),
         QStringLiteral("出口 FIFO 槽位上限；-1 表示不建模溢出上限。")},
        {QStringLiteral("buf_size"),
         QStringLiteral("共享池总 flit 容量，配合 buffer_policy 解释。")},
        {QStringLiteral("buffer_policy"),
         QStringLiteral("选 BufferState 派生类：私池、全共享、限额共享或反馈调节。")},
        {QStringLiteral("private_bufs"),
         QStringLiteral("把 num_vcs 切成几组专用池，每组深度由 private_buf_* 给。")},
        {QStringLiteral("max_held_slots"),
         QStringLiteral("限额共享下单 VC 在共享池中可同时占用的最大槽位数，抑制单流独占缓冲。")},
        {QStringLiteral("feedback_aging_scale"),
         QStringLiteral("反馈缓冲策略里 RTT 平滑窗口：大则跟拥塞更快。")},
        {QStringLiteral("feedback_offset"),
         QStringLiteral("反馈阈值整体平移，松一点或紧一点放行。")},
        {QStringLiteral("spec_sw_allocator"),
         QStringLiteral("投机路径单独指定 SA 算法，可与主 sw_allocator 不同。")},
        {QStringLiteral("alloc_iters"),
         QStringLiteral("单周期内分离式匹配器最多扫几轮，提高匹配度换时间。")},
        {QStringLiteral("classes"), QStringLiteral("并行流量类个数，决定注入率/包长向量维数。")},
        {QStringLiteral("traffic"),
         QStringLiteral("源宿联合分布的模式名：均匀、转置、置换、热点等。")},
        {QStringLiteral("class_priority"), QStringLiteral("类间静态优先级编码，进 VA/SA 排序。")},
        {QStringLiteral("perm_seed"), QStringLiteral("置换类 traffic 的抽签种子，定死可复现配对。")},
        {QStringLiteral("injection_rate"),
         QStringLiteral("每类提供负载：Bernoulli 下常作每节点每周期注入概率。")},
        {QStringLiteral("packet_size"), QStringLiteral("包长 flit 数，可向量化成多种包长混布。")},
        {QStringLiteral("packet_size_rate"), QStringLiteral("与 packet_size 向量对齐的抽档概率。")},
        {QStringLiteral("injection_process"),
         QStringLiteral("到达过程族：bernoulli、batch 等，决定与 burst 参数耦合。")},
        {QStringLiteral("burst_alpha"),
         QStringLiteral("突发模型里「关」段长度尺度，小则更碎、更像均匀。")},
        {QStringLiteral("burst_beta"), QStringLiteral("突发模型里「开」段长度尺度，大则洪峰更宽。")},
        {QStringLiteral("burst_r1"),
         QStringLiteral("突发内微观注入强度的旋钮，默认 -1 走派生默认。")},
        {QStringLiteral("priority"),
         QStringLiteral("flit 携带优先级字段的生成规则：none、age 等字符串。")},
        {QStringLiteral("batch_size"),
         QStringLiteral("批注入模式下每个批次连续准入网络的分组数量。")},
        {QStringLiteral("batch_count"), QStringLiteral("batch 注入重复几轮或控制统计分段。")},
        {QStringLiteral("max_outstanding_requests"),
         QStringLiteral("源端未完成事务数严格上限；0 表示不限制（应用层不施加反压）。")},
        {QStringLiteral("write_fraction"), QStringLiteral("读写并存时写事务占比，读取补集。")},
        {QStringLiteral("sim_type"),
         QStringLiteral("latency 扫延迟曲线，throughput 找可持续吞吐点。")},
        {QStringLiteral("warmup_periods"), QStringLiteral("扔掉前若干采样段，让队列填满再记数。")},
        {QStringLiteral("sample_period"), QStringLiteral("两次快照之间的仿真周期跨度。")},
        {QStringLiteral("max_samples"), QStringLiteral("自动停之前最多攒几条采样窗。")},
        {QStringLiteral("latency_thres"),
         QStringLiteral("平均延迟冲破它就当网络失稳、可截断本次点。")},
        {QStringLiteral("warmup_thres"),
         QStringLiteral("相邻窗延迟/吞吐相对抖动低于它 → 算预热完。")},
        {QStringLiteral("acc_warmup_thres"),
         QStringLiteral("基于滑动累积均值判定预热完成，降低单采样窗随机波动影响。")},
        {QStringLiteral("stopping_thres"),
         QStringLiteral("相邻窗指标相对变化低于它 → 宣布收敛停机。")},
        {QStringLiteral("acc_stopping_thres"), QStringLiteral("用累积曲线判收敛，比逐窗更钝感。")},
        {QStringLiteral("sim_count"), QStringLiteral("同一 JSON 外层重复跑几次独立随机流。")},
        {QStringLiteral("seed"), QStringLiteral("主 RNG；写 time 则每次启动不同抽签。")},
        {QStringLiteral("deadlock_warn_timeout"),
         QStringLiteral("若干连续周期内无 flit 级进展时输出疑似死锁告警。")},
        {QStringLiteral("watch_file"),
         QStringLiteral("从文件读取待跟踪的标识列表，避免命令行过长。")},
        {QStringLiteral("watch_flits"), QStringLiteral("空格/逗号分隔 flit id，写入 watch_out。")},
        {QStringLiteral("watch_packets"), QStringLiteral("按 packet id 过滤追踪，看整包轨迹。")},
        {QStringLiteral("watch_transactions"), QStringLiteral("读写模型下按 transaction id 过滤。")},
        {QStringLiteral("watch_out"), QStringLiteral("观测日志落盘路径，与 stdout 解耦。")},
        {QStringLiteral("stats_out"), QStringLiteral("除标准输出外另写一份统计文本。")},
        {QStringLiteral("power_output_file"), QStringLiteral("功耗模块写结果的文件前缀。")},
        {QStringLiteral("tech_file"),
         QStringLiteral("工艺金属线、器件电容电压表，喂 PowerConfig。")},
        {QStringLiteral("channel_width"),
         QStringLiteral("功耗公式里数据线宽度，决定线电容与翻转电容。")},
        {QStringLiteral("channel_sweep"), QStringLiteral("功耗扫参：位宽按步长递减重算直至归零。")},
        {QStringLiteral("in_ports"), QStringLiteral("路由器入向物理端口个数，须贴合拓扑入度。")},
        {QStringLiteral("out_ports"), QStringLiteral("路由器出向物理端口个数，须贴合拓扑出度。")},
        {QStringLiteral("link_failures"), QStringLiteral("拓扑建好后随机拔掉几条通道。")},
        {QStringLiteral("fail_seed"), QStringLiteral("拔链抽签种子，与 link_failures 绑定可复现。")},
        {QStringLiteral("num_vcs"),
         QStringLiteral("每入端口虚通道条数，死锁规避与 VA 代价都随它涨。")},
        {QStringLiteral("vc_buf_size"), QStringLiteral("单 VC FIFO 深度，深则抗突发、浅则早反压。")},
        {QStringLiteral("private_buf_size"),
         QStringLiteral("各 VC 组保底深度向量，字符串里可写多档。")},
        {QStringLiteral("private_buf_start_vc"),
         QStringLiteral("某一专用池从哪条 VC 下标开始覆盖。")},
        {QStringLiteral("private_buf_end_vc"),
         QStringLiteral("专用池覆盖到哪条 VC 下标为止（含）。")},
        {QStringLiteral("output_delay"),
         QStringLiteral("JSON 模板字段：本树 Router 未读，改它通常不影响周期。")},
        {QStringLiteral("credit_delay"),
         QStringLiteral("credit 飞回上游要飞的周期数，环路越长越早反压。")},
        {QStringLiteral("routing_delay"), QStringLiteral("显式 RC 级数；0 则走 lookahead 捷径。")},
        {QStringLiteral("vc_alloc_delay"),
         QStringLiteral("VA 流水拍数，拍数大则头包拿到 VC 更晚。")},
        {QStringLiteral("sw_alloc_delay"),
         QStringLiteral("SA 流水拍数，决定 grant 相对请求的相位。")},
        {QStringLiteral("st_prepare_delay"),
         QStringLiteral("crossbar 延迟里划给「traverse 前」的那几拍。")},
        {QStringLiteral("st_final_delay"),
         QStringLiteral("crossbar 延迟里划给「traverse 后」的那几拍，常承载主要寄存。")},
        {QStringLiteral("input_speedup"),
         QStringLiteral("将单入端口在 SA 矩阵中展开为多列逻辑端口，并行向多输出提交分配请求。")},
        {QStringLiteral("output_speedup"),
         QStringLiteral("单输出端口每周期可接收多路交换授予，提高出口侧时间复用与吞吐上界。")},
        {QStringLiteral("vc_allocator"),
         QStringLiteral("谁来做输入×输出 VC 的匹配：islip、separable 等。")},
        {QStringLiteral("sw_allocator"), QStringLiteral("谁来做交叉开关请求矩阵的极大匹配。")},
        {QStringLiteral("arb_type"), QStringLiteral("分离式分配器里子端口仲裁轮转还是矩阵优先级。")},
        {QStringLiteral("read_request_begin_vc"),
         QStringLiteral("读请求 VC 窗左端，和 end夹出可用集合。")},
        {QStringLiteral("read_request_end_vc"),
         QStringLiteral("读请求 VC 窗右端，窗宽即读并发上限。")},
        {QStringLiteral("write_request_begin_vc"),
         QStringLiteral("写请求 VC 窗左端，常与读窗错开防混用。")},
        {QStringLiteral("write_request_end_vc"), QStringLiteral("写请求 VC 窗右端。")},
        {QStringLiteral("read_reply_begin_vc"),
         QStringLiteral("读回应答数据 VC 窗左端，常与请求窗分离。")},
        {QStringLiteral("read_reply_end_vc"),
         QStringLiteral("读回应答 VC 窗右端，大块读需要更宽窗。")},
        {QStringLiteral("write_reply_begin_vc"),
         QStringLiteral("写完成确认 VC 窗左端，短控制包即可。")},
        {QStringLiteral("write_reply_end_vc"), QStringLiteral("写完成确认 VC 窗右端。")},
        {QStringLiteral("read_request_subnet"),
         QStringLiteral("读请求从哪个子网注入，多平面拓扑里定平面。")},
        {QStringLiteral("read_reply_subnet"),
         QStringLiteral("读数据从哪个子网回到源，可建模分层返回。")},
        {QStringLiteral("write_request_subnet"),
         QStringLiteral("写请求子网，与读路径独立配置时分道扬镳。")},
        {QStringLiteral("write_reply_subnet"), QStringLiteral("写确认子网，可与写请求同面或异面。")},
        {QStringLiteral("read_request_size"),
         QStringLiteral("读请求 flit 数，通常远小于读回数据包。")},
        {QStringLiteral("write_request_size"), QStringLiteral("写请求 flit 数，地址+控制为主。")},
        {QStringLiteral("read_reply_size"),
         QStringLiteral("读回数据 flit 数，决定应答侧带宽压力。")},
        {QStringLiteral("write_reply_size"), QStringLiteral("写确认 flit 数，建模 ACK 通道宽度。")},
    };
    if (kSpecific.contains(key)) {
        return kSpecific.value(key);
    }
    return QStringLiteral("BookSim 全局配置字段；语义见详述。");
}

[[nodiscard]] QString configParamDetail(const QString& key) {
    static const QMap<QString, QString> kDetails = [] {
        QMap<QString, QString> m;
        m.insert(QStringLiteral("topology"),
                 QStringLiteral(
                     "<p><code>topology</code> 字符串选中某一 <code>Network</code> 子类："
                     "例如 <code>mesh</code> 生成笛卡尔网格，<code>torus</code> 在边界"
                     "回绕，<code>anynet</code> 从 <code>network_file</code> 读自定义图。"
                     "结构一变，<code>k</code>/<code>n</code> 等字段的语义、端口度、以及"
                     "<code>routing_function</code> 运行期解析成的 "
                     "<code>函数名_拓扑</code> 全部联动；拼写错误会在工厂注册阶段直接"
                     "失败。</p>"));
        m.insert(QStringLiteral("channel_file"),
                 QStringLiteral("<p>指向文本列表：按行或按 BookSim 约定格式给出每条物理通道的"
                                "线长或等效周期延迟，供拓扑在 <code>SetLatency</code> 时读取。"
                                "留空表示不读外置表，所有通道沿用该拓扑构造器写死的默认（常见为"
                                "单位延迟或模板值）。做 NoC floorplan 对比时，这是把微米距离折成"
                                "仿真周期的入口。</p>"));
        m.insert(QStringLiteral("k"),
                 QStringLiteral("<p>在 kncube 族拓扑里，<code>k</code> 是「每一维有多少个"
                                "路由器」。二维 mesh 上它常等于 X/Y 向格宽（若两向相等）；"
                                "维序路由的最短路径长度分布、二分带宽的可分性都随它变。</p>"));
        m.insert(QStringLiteral("n"),
                 QStringLiteral(
                     "<p><code>n</code> 为维数：例如二维 mesh 取2、三维取 3。路由器总数相对 "
                     "<code>n</code> 按 <code>k^n</code> 增长；平均跳数随 <code>n</code> 与路由策略"
                     "变化，竞争热点由边区向角区/维界迁移的模式亦不同。</p>"));
        m.insert(QStringLiteral("c"),
                 QStringLiteral("<p><code>c</code> 把终端挂到路由器上：<code>c=1</code> 时一节点"
                                "一路由器最常见；<code>c&gt;1</code> 表示一个路由器端口组服务"
                                "多个终端（chiplet、集中式 NI）。此时必须配 "
                                "<code>x</code>、<code>y</code>、<code>xr</code>、<code>yr</code> "
                                "描述终端在路由器周围的二维排布，否则几何延迟与注入端口计数"
                                "会对不上。</p>"));
        m.insert(QStringLiteral("x"),
                 QStringLiteral("<p>在片上网格坐标里，沿<strong>水平向</strong>排了多少个"
                                "<strong>路由器</strong>；与 <code>y</code> 相乘给出平面路由器"
                                "规模。只在 <code>use_noc_latency</code> 打开时参与把几何距离"
                                "折成链路延迟，不影响纯拓扑抽象下的跳数。</p>"));
        m.insert(QStringLiteral("y"),
                 QStringLiteral("<p>沿<strong>竖直向</strong>的路由器行数，与 <code>x</code> 正交"
                                "定义铺砖区域。dragonfly、flatfly 等不规则拓扑若不用该几何字段"
                                "可忽略；但在二维 NoC 论文复现时，<code>x</code>、<code>y</code> "
                                "常与芯片长宽比一致。</p>"));
        m.insert(QStringLiteral("xr"),
                 QStringLiteral("<p>每个路由器<strong>局部</strong>在 X 向挂靠几个终端，只在 "
                                "<code>c&gt;1</code> 时有意义。它与全局 <code>x</code>（路由器"
                                "列数）不是同一概念：前者是「路由器脚垫上的终端栅格」，后者是"
                                "「路由器自身栅格」。</p>"));
        m.insert(QStringLiteral("yr"),
                 QStringLiteral("<p>局部 Y 向每路由器终端数，与 <code>xr</code> 配对；"
                                "<code>xr×yr</code> 应能解释该路由器下 <code>c</code> 个终端"
                                "如何排布（或与其成比例），供距离公式使用。</p>"));
        m.insert(QStringLiteral("link_failures"),
                 QStringLiteral("<p>拓扑与通道建完后，从中<strong>随机挑若干条 channel 标记为"
                                "断链</strong>，断链上不再转发 flit。设 <code>0</code> 即理想"
                                "互连；设正值则配合 <code>fail_seed</code> 固定抽签，便于对比"
                                "「同一故障图」下的路由容错。若断得太多或断在割集上，可能出现"
                                "不可达或极早饱和，这属于实验设定而非 Bug。</p>"));
        m.insert(QStringLiteral("fail_seed"),
                 QStringLiteral("<p>只影响「哪几条通道被标故障」这一随机过程，与 "
                                "<code>seed</code>（流量、注入）独立。整数固定则故障集合固定；"
                                "BookSim 另提供字符串通道写 <code>time</code> 以墙钟初始化，"
                                "适合单次探索但不利于论文复现。</p>"));
        m.insert(QStringLiteral("in_ports"),
                 QStringLiteral("<p>实例化路由器时声明的<strong>入向物理端口</strong>个数，"
                                "必须与拓扑里连到该节点的<strong>入边</strong>数一致，否则"
                                "通道索引与缓冲数组错位。ChaosRouter 等实现还要求与 "
                                "<code>out_ports</code> 对称，违背时断言在构造期触发。</p>"));
        m.insert(QStringLiteral("out_ports"),
                 QStringLiteral("<p><strong>出向物理端口</strong>个数，决定交叉开关输出维、"
                                "输出缓冲数组长度以及 SA 矩阵列数。不规则拓扑里不同节点度不同"
                                "时，BookSim 仍可能用统一默认，此时要以实际拓扑生成代码为准；"
                                "anynet 等自定义拓扑常逐节点指定度。</p>"));
        m.insert(QStringLiteral("routing_function"),
                 QStringLiteral("<p>这是<strong>选路函数</strong>的短名，不是微架构名。"
                                "BookSim 在运行期把它与 <code>topology</code> 字符串用下划线"
                                "拼接后去全局表找工厂：因此 <code>dor</code> 在 mesh 与 cmesh "
                                "上可能指向完全不同的 C++ 类。<code>min</code>、<code>ugal</code> "
                                "等名字只在特定拓扑里有意义；抄错拓扑-算法组合会在仿真启动时"
                                "才爆「未注册」。</p>"));
        m.insert(QStringLiteral("use_noc_latency"),
                 QStringLiteral("<p>打开后，注入/弹出通道以及部分互连通道的 "
                                "<code>SetLatency</code> 会按几何距离填大于 1 的延迟，"
                                "模拟长导线；关闭时多数拓扑退化为每跳固定单位延迟的理想互连模型。"
                                "它不改变路由器的 RC/VA/SA 逻辑，只改变链路排队前的传播相位。"
                                "</p>"));
        m.insert(QStringLiteral("router"),
                 QStringLiteral("<p><code>iq</code>（IQRouter）是主line：离散周期、四级流水、"
                                "与本书式参数一一对应。<code>event</code> 把同一微架构事件化，"
                                "VC 上千时 CPU 时间更省。<code>chaos</code> 走另一套队列状态机，"
                                "与 chaos <strong>路由函数</strong>不是一回事。换 "
                                "<code>router</code> 会改变可用的配置键集合与断言。</p>"));
        m.insert(QStringLiteral("output_delay"),
                 QStringLiteral("<p>BookSim 默认 JSON/配置里与 <code>credit_delay</code> 并列的"
                                "「输出延迟」字段；当前附带源码的 <code>Router::Router</code> 与 "
                                "<code>IQRouter</code> 构造<strong>未调用</strong> "
                                "<code>GetRouterInt(..., \"output_delay\")</code>，"
                                "<code>networks/*</code> 亦未引用。导出保留主要为与官方 anynet "
                                "模板兼容；在本构建上调节它通常<strong>不改变</strong>周期级仿真"
                                "结果，出口寄存请用通道 <code>SetLatency</code> 或 "
                                "<code>st_prepare_delay</code>/<code>st_final_delay</code>。"
                                "</p>"));
        m.insert(QStringLiteral("credit_delay"),
                 QStringLiteral(
                     "<p><code>Router</code> 基类从配置读入：下游消费槽位产生的 credit "
                     "经 CreditChannel 回到上游路由器、被输入 VC 处理前所经历的周期数。"
                     "它拉长「数据发送—credit 返回」环路；在固定 <code>vc_buf_size</code> "
                     "下等价于让上游更早停发，零负载跳数公式（见 "
                     "<code>iq_router.cpp</code> 中 <code>min_latency</code>）也显式加上"
                     "该项。增大该值常使负载-延迟曲线在更低注入率处拐弯。</p>"));
        m.insert(QStringLiteral("internal_speedup"),
                 QStringLiteral("<p>将路由器<strong>内部状态机</strong>相对链路接口侧加快的倍率："
                                "以浮点因子缩放周期推进，等效于片上逻辑相对链路边界更快。"
                                "它不增加物理端口并行度——后者由 <code>input_speedup</code> / "
                                "<code>output_speedup</code> 的整数扩展给出；二者可叠加：前者主要"
                                "吸收排队引入的时序裕量，后者提高交换分配矩阵的有效维数。</p>"));
        m.insert(QStringLiteral("output_buffer_size"),
                 QStringLiteral("<p>输出侧可缓存的 flit 槽位数；默认 <code>-1</code> 表示无上限。"
                                "BookSim 注释：在带输出加速时若输出缓冲满，会取消交换分配请求，"
                                "从而把反压反映到上游 SA。</p>"));
        m.insert(QStringLiteral("noq"),
                 QStringLiteral("<p>Next-hop-output-queueing：在本地输出端口为下游节点的输入队列"
                                "建立镜像排队，改变相邻路由器之间的缓冲与 credit 边界语义。</p>"));
        m.insert(QStringLiteral("num_vcs"),
                 QStringLiteral("<p>每个入端口挂几条逻辑队列。维序路由里常用多条 VC 做死锁豁免；"
                                "自适应路由里 VC 还承担「哪条出口队列」的语义。数值越大，"
                                "VA 匹配问题规模越大、仿真开销越高，但一般有利于缓解队首阻塞。"
                                "</p>"));
        m.insert(QStringLiteral("vc_buf_size"),
                 QStringLiteral(
                     "<p>在 <code>private</code> 策略下，这是<strong>每条 VC 独享</strong>"
                     "的 flit 槽数；在共享池策略里，它常作为<strong>每 VC 配额或"
                     "下限</strong>参与 <code>buf_size</code> 切分。深度浅则 credit "
                     "很快打回上游，深则吞突发但占内存。</p>"));
        m.insert(QStringLiteral("buf_size"),
                 QStringLiteral("<p>所有 VC 共用的<strong>大池</strong>总 flit 容量，只在 "
                                "<code>shared</code>、<code>limited</code>、<code>feedback</code> "
                                "等派生类里生效。<code>-1</code> 让构造器按其它字段推导或走"
                                "兼容路径；配错会出现「池子比 VC 之和小」的隐性饥饿。</p>"));
        m.insert(QStringLiteral("buffer_policy"),
                 QStringLiteral("<p>缓冲会计方式：<code>private</code> 每 VC 独立额度；"
                                "<code>shared</code> 共用池；<code>limited</code> / "
                                "<code>dynamic</code> / <code>shifting</code> 在共享池上增加"
                                "每 VC 占用上限或动态重分；<code>feedback</code> / "
                                "<code>simplefeedback</code> 根据下游占用与 RTT 反馈调节可用空间。"
                                "策略类定义见 <code>buffer_state.cpp</code>。</p>"));
        m.insert(QStringLiteral("private_bufs"),
                 QStringLiteral("<p>把 VC 划到若干专用缓冲组的组数；与 "
                                "<code>private_buf_size</code>、<code>private_buf_start_vc</code>、"
                                "<code>private_buf_end_vc</code> 一起描述每组覆盖的 VC 区间及配额。"
                                "</p>"));
        m.insert(QStringLiteral("private_buf_size"),
                 QStringLiteral("<p>字符串或标量形式给出各专用组的深度向量，用于 "
                                "<code>shared</code> 族策略中为部分 VC 预留最小保障。</p>"));
        m.insert(QStringLiteral("private_buf_start_vc"),
                 QStringLiteral("<p>每个专用缓冲组起始 VC 编号配置；需与 end_vc 形成不重叠划分。"
                                "</p>"));
        m.insert(QStringLiteral("private_buf_end_vc"),
                 QStringLiteral("<p>每个专用缓冲组结束 VC（含）配置。</p>"));
        m.insert(QStringLiteral("max_held_slots"),
                 QStringLiteral("<p>在 <code>limited</code> 及其动态变体中，限制单个 VC 在共享池里"
                                "可同时占用的槽位上限；<code>-1</code> 时实现会回退为 "
                                "<code>vc_buf_size</code> 或等效默认值，用于抑制单个 VC 饿死他人。"
                                "</p>"));
        m.insert(QStringLiteral("feedback_aging_scale"),
                 QStringLiteral("<p>仅 <code>buffer_policy=feedback</code> 路径有效：缩放 RTT 估计"
                                "的指数平滑或老化步长，数值越大对近期样本权重越高，占用阈值随"
                                "拥塞变化更快。</p>"));
        m.insert(QStringLiteral("feedback_offset"),
                 QStringLiteral("<p>反馈策略里对「可接受占用」的偏置量，与测得的往返时延共同决定"
                                "何时收紧或放松 VC 的共享额度。</p>"));
        m.insert(QStringLiteral("wait_for_tail_credit"),
                 QStringLiteral("<p>尾 flit 对应 credit 回到上游之前，不回收、不重分配该 VC，"
                                "避免当前分组尾片与下一分组在协议时间线上交错。</p>"));
        m.insert(QStringLiteral("vc_busy_when_full"),
                 QStringLiteral("<p>VC 已无可用 credit 时仍保持 busy，上游 VA 将其视为仍被占用，"
                                "可选 VC 集合与资源依赖图随之变化。</p>"));
        m.insert(QStringLiteral("vc_prioritize_empty"),
                 QStringLiteral("<p>VC 分配仲裁优先把新分组放到空 VC，减轻不同分组在同一 VC上"
                                "队首交织（HOL mixing），可能降低 VC 槽位利用率。</p>"));
        m.insert(QStringLiteral("vc_priority_donation"),
                 QStringLiteral("<p>高优先级 flit 可把优先级让给阻塞在前方的低优先级头包，"
                                "缓解 VA 侧优先级反转与饥饿。</p>"));
        m.insert(QStringLiteral("vc_shuffle_requests"),
                 QStringLiteral("<p>送入 VC 分配器之前对请求顺序做随机置换，削弱固定端口编号"
                                "带来的长期不公平。</p>"));
        m.insert(QStringLiteral("speculative"),
                 QStringLiteral(
                     "<p>推测式交换分配（speculative switch allocation）：在虚通道分配"
                     "尚未最终完成时提前发起交换分配，以缩短流水线气泡；由 "
                     "<code>spec_check_*</code> 限制不合法授予。关闭时须先完成 VA 再进入 "
                     "SA。</p>"));
        m.insert(QStringLiteral("spec_check_elig"),
                 QStringLiteral("<p>推测 SA 授予前检查目标输出 VC 对当前分组仍 eligible，"
                                "即仍落在路由函数给出的合法输出集合内。</p>"));
        m.insert(QStringLiteral("spec_check_cred"),
                 QStringLiteral("<p>推测路径上验证下游仍有足够 credit；跳过检查可减少开销，"
                                "但可能出现需撤销的错误 grant。</p>"));
        m.insert(QStringLiteral("spec_mask_by_reqs"),
                 QStringLiteral("<p>用当前周期实际 SA 请求掩码收紧推测候选集，使投机与真实请求"
                                "更一致。</p>"));
        m.insert(QStringLiteral("spec_sw_allocator"),
                 QStringLiteral("<p>推测阶段使用的交换分配器类型字符串（如 "
                                "<code>prio</code>），与主 <code>sw_allocator</code> 可不同，"
                                "用于把投机路径上的仲裁成本与行为解耦。</p>"));
        m.insert(QStringLiteral("routing_delay"),
                 QStringLiteral(
                     "<p>IQRouter 中路由计算（RC）的流水深度（周期）。配置为 "
                     "<code>0</code> 时，<code>vc.cpp</code> 与 "
                     "<code>trafficmanager.cpp</code> 打开 <b>lookahead routing</b>："
                     "头片可在实现允许的前提下提前获知合法输出，减少 RC 气泡，但 RC与 "
                     "VA/SA 的时间对齐方式与「显式 RC」不同。配置 <code>&ge;1</code> "
                     "时每个头包至少经历这么多个周期的 RC 才能排队参加 VA。"
                     "<code>trafficmanager.cpp</code> 用 "
                     "<code>routing_delay</code> 与 VA/SA 延迟及 <code>speculative</code> "
                     "一起估算路由器贡献的最小跳延迟：推测模式下取 "
                     "<code>max(vc_alloc_delay, sw_alloc_delay)</code>，否则两段串联求和。"
                     "</p>"));
        m.insert(QStringLiteral("vc_alloc_delay"),
                 QStringLiteral("<p>虚通道分配流水：从本周期提交 VA 到 grant 在数据路径上生效之间"
                                "至少间隔这么多个周期。IQRouter 在配置为 <code>0</code> 时会改写成"
                                "合法正值。与 <code>alloc_iters</code> 共同限制单周期可完成的匹配"
                                "深度；瓶颈会向后推迟 body flit 获得 VC 的时刻，从而抬高排队延迟。"
                                "</p>"));
        m.insert(QStringLiteral("sw_alloc_delay"),
                 QStringLiteral("<p>交换分配流水：SA 矩阵产生 grant 到交叉开关实际接受新配置的"
                                "间隔。非推测模式下它与 VA 延迟在时间上串联；推测模式下与 VA "
                                "延迟按周期取较大值并行重叠，因而可能缩短关键路径但不能突破二者"
                                "各自的流水深度。</p>"));
        m.insert(QStringLiteral("st_prepare_delay"),
                 QStringLiteral(
                     "<p>在 <code>router.cpp</code> 中与 <code>st_final_delay</code> "
                     "<b>相加</b>得到 <code>_crossbar_delay</code>：表示 flit 在交换核"
                     "内从 grant 到出现在输出侧之间的总寄存/导线延迟中，划分在「遍历"
                     "前」的那一段。常见默认把准备级配成 <code>0</code>，把全部 crossbar "
                     "延迟放在 <code>st_final_delay</code>。</p>"));
        m.insert(QStringLiteral("st_final_delay"),
                 QStringLiteral(
                     "<p>与 <code>st_prepare_delay</code> 之和为 <code>_crossbar_delay</code>。"
                     "IQRouter 将获批的 flit 放入 crossbar 队列，在 "
                     "<code>_crossbar_delay</code> 个周期后写入输出缓冲；该值直接进"
                     "零负载最小延迟估算，并与 <code>output_speedup</code> 一起出现在"
                     "输出缓冲上界断言中（满输出缓冲会反向抑制 SA）。</p>"));
        m.insert(QStringLiteral("input_speedup"),
                 QStringLiteral(
                     "<p>每个物理输入在 SA 视角下展开为 <code>input_speedup</code> 条"
                     "逻辑子端口：同一周期内可向不同输出端口同时提交多路请求，"
                     "二维请求矩阵变「更宽」，竞争形态从单端口串行变为多端口并行。"
                     "大于 <code>1</code> 时常需配合有限的 <code>output_buffer_size</code>，"
                     "否则易出现输出侧背压取消 grant。</p>"));
        m.insert(QStringLiteral("output_speedup"),
                 QStringLiteral(
                     "<p>每个物理输出每周期最多可接纳的输入 grant 数上界，实现为交叉"
                     "开关时间复用。它放大输出端口服务率，也放大对输出 FIFO 的瞬时写入"
                     "；IQRouter 用 <code>output_speedup</code> 与 <code>_crossbar_delay</code> "
                     "共同推导输出缓冲必要深度上界。</p>"));
        m.insert(QStringLiteral("hold_switch_for_packet"),
                 QStringLiteral("<p>分组获得交换配置后，在整包所有 flit 传输完成前保持同一配置，"
                                "减少逐 flit 重新仲裁；短包上降低 SA 抖动，长包上可能降低交叉开关"
                                "时间复用。</p>"));
        m.insert(QStringLiteral("vct"),
                 QStringLiteral("<p>Virtual cut-through：头 flit 到达下游即可预约缓冲并继续推进，"
                                "不必像 wormhole 那样在整条路径上同时占用缓冲。</p>"));
        m.insert(QStringLiteral("vc_allocator"),
                 QStringLiteral("<p>解决多个入端口同时竞争同一出端口虚通道的二维匹配问题。"
                                "<code>islip</code> 做迭代滑窗，<code>separable_input_first</code> "
                                "先按入端口仲裁再按出端口，公平性与延迟特性不同。错误字符串"
                                "会在构造分配器时直接抛错。</p>"));
        m.insert(QStringLiteral("sw_allocator"),
                 QStringLiteral("<p>解决「多个入端口要同一物理出端口」的时隙竞争，输出是 "
                                "grant 位图。它与 VC 分配器前后级联：VA 决定 flit 进哪条输出"
                                "队列，SA 决定哪条队列占用 crossbar 时隙。</p>"));
        m.insert(QStringLiteral("arb_type"),
                 QStringLiteral("<p>在 separable 族分配器里，子仲裁采用轮转或固定优先级矩阵；"
                                "配置不同将改变各端口长期获得 grant 的概率分布，对非均匀流量"
                                "敏感，与 <code>vc_shuffle_requests</code> 联合时可削弱或叠加"
                                "系统性优先级倾斜。</p>"));
        m.insert(QStringLiteral("alloc_iters"),
                 QStringLiteral("<p>分离式匹配每周期最多迭代几轮：轮数少可能剩未匹配请求，"
                                "轮数多更接近最大匹配但仿真计算量近似线性增加。与 "
                                "<code>vc_alloc_delay</code> 的<strong>流水拍数</strong>不同："
                                "后者是时序深度，前者是组合深度在周期内的展开次数。</p>"));
        m.insert(QStringLiteral("classes"),
                 QStringLiteral("<p>BookSim 支持多类 QoS：每类可配独立 "
                                "<code>injection_rate</code>、<code>packet_size</code> 向量、"
                                "<code>measure_stats</code> 开关。<code>class_priority</code> "
                                "决定类间仲裁顺序。类数增加会让配置向量变长，注意 GUI 与 JSON "
                                "对齐。</p>"));
        m.insert(QStringLiteral("traffic"),
                 QStringLiteral("<p>内置流量模式通过函数对象生成 (src,dst) 或注入过程："
                                "<code>uniform</code> 独立均匀，<code>transpose</code> 做坐标"
                                "转置，<code>bitcomp</code> 做位补，热点模式将目的地址集中于少数"
                                "节点。"
                                "模式名拼错在运行期才报错；与拓扑节点数不兼容时表现为异常分布"
                                "或除零。</p>"));
        m.insert(QStringLiteral("class_priority"),
                 QStringLiteral("<p>整数或向量：数值大的类在冲突时先拿资源。与 "
                                "<code>priority</code>（报文级 age策略）是两层：前者定类，"
                                "后者定类内 flit 次序。</p>"));
        m.insert(QStringLiteral("perm_seed"),
                 QStringLiteral("<p>专供需要随机置换矩阵的流量模式；与全局 <code>seed</code> "
                                "分离，便于只重抽流量图而不重抽注入伯努利序列。</p>"));
        m.insert(QStringLiteral("injection_rate"),
                 QStringLiteral("<p>每仿真周期、每注入节点尝试往网络里塞多少业务：Bernoulli "
                                "模式下是概率，batch 模式下可与批参数合成等效均值。"
                                "与拓扑节点数、<code>traffic</code> 模式共同决定全网 Offered "
                                "load；扫参时这是横轴核心。</p>"));
        m.insert(QStringLiteral("injection_rate_uses_flits"),
                 QStringLiteral("<p>切换后同一数字代表不同物理量：按包算是 "
                                "「包/周期/节点」，按 flit 算是「flit/周期/节点」。混用会"
                                "让延迟-负载曲线与论文坐标轴对不齐。</p>"));
        m.insert(QStringLiteral("packet_size"),
                 QStringLiteral("<p>包头到包尾占几条 flit，直接影响单包对缓冲与链路的占用"
                                "时长。配成向量可混合短控制包与长数据包；与 "
                                "<code>packet_size_rate</code> 组合成离散分布。</p>"));
        m.insert(QStringLiteral("packet_size_rate"),
                 QStringLiteral("<p>与 <code>packet_size</code> 向量逐项对齐的权重，不必归一化"
                                "到1，BookSim 内部会按分布抽样；全0 或长度不匹配会触发解析"
                                "错误。</p>"));
        m.insert(QStringLiteral("injection_process"),
                 QStringLiteral(
                     "<p><code>bernoulli</code> 每周期独立伯努利试投；<code>batch</code> "
                     "按块脉冲式投放。换过程会改变与 <code>burst_alpha</code> / "
                     "<code>burst_beta</code> 的耦合方式，扫参时勿混用口径。</p>"));
        m.insert(QStringLiteral("burst_alpha"),
                 QStringLiteral("<p>在 on-off 源模型里调制<strong>静默段</strong>长度分布："
                                "值较小则源端 on/off 切换更频繁，注入过程起伏更大；与 "
                                "<code>burst_beta</code> "
                                "共同决定占空比，进而改变网络看到的突发性而不改平均率。</p>"));
        m.insert(QStringLiteral("burst_beta"),
                 QStringLiteral("<p>调制<strong>突发段</strong>持续时间：取值较大时单次突发内连续"
                                "注入分组更多，短时 flit 到达率可接近链路容量，易激发缓冲队列"
                                "振荡；与 alpha 联合可在固定平均负载下单独扫描突发形态。</p>"));
        m.insert(QStringLiteral("burst_r1"),
                 QStringLiteral("<p>细调突发段内有效注入强度的辅助系数；保持 "
                                "<code>-1</code> 时由 BookSim 用 <code>injection_rate</code> "
                                "推导默认值。仅在高级 traffic 生成路径里起作用。</p>"));
        m.insert(QStringLiteral("priority"),
                 QStringLiteral("<p>控制 flit 头里 priority 字段如何随时间更新："
                                "<code>none</code> 扁平，<code>age</code> 随等待增长。它作用在"
                                "微架构仲裁树叶子，与类优先级相乘决定最终次序。</p>"));
        m.insert(QStringLiteral("batch_size"),
                 QStringLiteral("<p>每一批许可中连续准入网络的分组数目；批注入在时间轴上形成近似"
                                "矩形包率脉冲，较 Bernoulli 过程更易激发缓冲队列在上升/下降沿的"
                                "瞬态响应。</p>"));
        m.insert(QStringLiteral("batch_count"),
                 QStringLiteral("<p>批注入重复轮次或批次数上限；与 <code>sample_period</code> "
                                "配合可使统计窗口与批边界对齐，减轻窗口截断对批脉冲的影响。</p>"));
        m.insert(QStringLiteral("max_outstanding_requests"),
                 QStringLiteral("<p>在读写源模型里限制「飞在空中」的事务条数，模拟主机或缓存"
                                "的 MSHR/队列容量。<code>0</code> 表示不截断；设小值会在源端"
                                "形成第二重反压，改变有效注入过程。</p>"));
        m.insert(QStringLiteral("use_read_write"),
                 QStringLiteral("<p>打开后，注入器按事务类型打不同 flit 头，走不同 VC/子网"
                                "区间，便于对 coherence 或内存一致性流量建模仿真；关闭则"
                                "退化为单一匿名分组，配置里的 read/write 区间被忽略。</p>"));
        m.insert(QStringLiteral("write_fraction"),
                 QStringLiteral("<p>在混合事务源里，下一次抽样写事务的概率质量；读事务占剩余份额。"
                                "只影响源类型比例，不改变单种包长，需与四类包长配置自洽。</p>"));
        m.insert(QStringLiteral("read_request_begin_vc"),
                 QStringLiteral(
                     "<p><code>use_read_write</code> 开启时，读<strong>请求</strong>分组"
                     "允许使用的 VC 闭区间下界；上界为 <code>read_request_end_vc</code>。"
                     "该区间须与其它三类（写请求、读/写应答）区间在 VC 维上可区分，以便"
                     "路由与死锁规避策略对不同类型分配独立资源。</p>"));
        m.insert(QStringLiteral("read_request_end_vc"),
                 QStringLiteral("<p>读请求 VC 闭区间上界，与 <code>read_request_begin_vc</code> "
                                "配对；区间宽度决定读请求可占用的 VC 并行度上限。</p>"));
        m.insert(QStringLiteral("write_request_begin_vc"),
                 QStringLiteral(
                     "<p>写<strong>请求</strong>分组 VC 闭区间下界；BookSim 默认示例"
                     "常与读请求区间交错或相邻，用于在共享物理链路上隔离事务类型。</p>"));
        m.insert(QStringLiteral("write_request_end_vc"),
                 QStringLiteral("<p>写请求 VC 闭区间上界；与读请求区间重叠会导致同一 VC 同时承载"
                                "多种报文形态，一般需避免除非协议明确复用。</p>"));
        m.insert(QStringLiteral("read_reply_begin_vc"),
                 QStringLiteral("<p>读<strong>应答</strong>（数据从内存/目标返回源）分组使用的 VC "
                                "区间下界；常与请求区间分离，使往返路径在虚通道维上可独立流控。"
                                "</p>"));
        m.insert(QStringLiteral("read_reply_end_vc"),
                 QStringLiteral("<p>读应答 VC 闭区间上界；应答往往长于请求，区间需容纳突发式返回"
                                "流量。</p>"));
        m.insert(QStringLiteral("write_reply_begin_vc"),
                 QStringLiteral("<p>写<strong>应答</strong>（完成确认）分组 VC 区间下界；与写请求"
                                "分离可避免确认包与仍在途的写数据在 VC 分配上互相阻塞。</p>"));
        m.insert(QStringLiteral("write_reply_end_vc"),
                 QStringLiteral("<p>写应答 VC 窗右端；窗通常只需1～2 条 VC 即可承载短 ACK，"
                                "不必像读回应答那样宽大。</p>"));
        m.insert(QStringLiteral("read_request_subnet"),
                 QStringLiteral("<p>读请求注入的物理子网编号；多子网拓扑下把读事务约束在某一平面"
                                "或某一复制网络上，与 <code>subnets</code> 计数一致。</p>"));
        m.insert(QStringLiteral("read_reply_subnet"),
                 QStringLiteral("<p>读应答所走子网；若与请求子网不一致，建模的是跨子网返回路径或"
                                "分层互连中的分离路由。</p>"));
        m.insert(QStringLiteral("write_request_subnet"),
                 QStringLiteral("<p>写请求从哪个子网进入互连；多子网片上系统中可映射到「写到"
                                "远端内存控制器所在平面」。与读请求子网独立配置时可建模"
                                "不对称路由。</p>"));
        m.insert(QStringLiteral("write_reply_subnet"),
                 QStringLiteral("<p>写完成/ACK 经哪条子网回到主控；若与请求子网分离，可模拟"
                                "请求走控制面、确认走数据面的分裂拓扑。</p>"));
        m.insert(QStringLiteral("read_request_size"),
                 QStringLiteral("<p>单个读请求分组占用的 flit 数（含头/尾）；影响注入带宽需求与"
                                "与 <code>read_reply_size</code> 的往返不对称程度。</p>"));
        m.insert(QStringLiteral("write_request_size"),
                 QStringLiteral("<p>写请求分组 flit 数；通常含地址与控制信息，往往短于携带数据的"
                                "写应答或读应答。</p>"));
        m.insert(QStringLiteral("read_reply_size"),
                 QStringLiteral("<p>读应答数据分组 flit 数；大块读时该值大，对缓冲与反压更敏感。"
                                "</p>"));
        m.insert(QStringLiteral("write_reply_size"),
                 QStringLiteral("<p>写完成确认分组 flit 数；建模写确认通道宽度。</p>"));
        m.insert(QStringLiteral("sim_type"),
                 QStringLiteral("<p><code>latency</code> 模式：固定或扫描注入率，关心延迟样本"
                                "分布与均值，适合画延迟-负载曲线。<code>throughput</code> 模式："
                                "在同一注入条件下寻找网络能长期吞下的最大 Offered load，"
                                "适合画吞吐饱和前沿。二者调用的停止逻辑与统计类相同，但内部"
                                "判稳侧重点不同。</p>"));
        m.insert(QStringLiteral("warmup_periods"),
                 QStringLiteral("<p>丢弃最前面若干完整 <code>sample_period</code> 窗，避免"
                                "空网、空队列的瞬态拉低或抬高均值。设太小会看到「前几分钟"
                                "异常」；设太大浪费 CPU——尤其对高注入率长弛豫过程。</p>"));
        m.insert(QStringLiteral("sample_period"),
                 QStringLiteral("<p>每个统计窗覆盖的仿真周期长度：窗太短估计方差大，窗太长"
                                "对漂移不敏感。与 <code>warmup_thres</code> 联用：连续两窗"
                                "指标相对差小于阈值才认为稳。</p>"));
        m.insert(QStringLiteral("max_samples"),
                 QStringLiteral("<p>硬上限：最多采几窗就强制停，防止判据过松时无限跑。与"
                                "<code>stopping_thres</code> 竞速，先到先停。</p>"));
        m.insert(QStringLiteral("measure_stats"),
                 QStringLiteral("<p>向量按类屏蔽统计：关掉的类仍注入、仍占带宽，只是不进 CSV/"
                                "延迟直方图，省内存。扫多类 QoS 时常关掉不关心的一类。</p>"));
        m.insert(QStringLiteral("pair_stats"),
                 QStringLiteral("<p>把矩阵从「全网聚合」细化到「每一对 (src,dst)」：输出文件"
                                "与 RAM 按 <code>N²</code> 涨，只适合小 N 或定位局部热点。"
                                "</p>"));
        m.insert(QStringLiteral("latency_thres"),
                 QStringLiteral("<p>自动扫参时的<strong>熔断</strong>：单窗平均延迟一旦超过"
                                "该绝对值，认为网络已进入不可接受拥塞或振荡，提前结束该注入"
                                "点，减少高负载区无效仿真时间。</p>"));
        m.insert(QStringLiteral("warmup_thres"),
                 QStringLiteral("<p>比较<strong>相邻两窗</strong>原始样本：若延迟或吞吐相对"
                                "变化都小于此比例，宣布「预热完成，开始记有效样本」。对周期性"
                                "流量要设得比随机抖动略大。</p>"));
        m.insert(QStringLiteral("acc_warmup_thres"),
                 QStringLiteral("<p>与 <code>warmup_thres</code> 同类判据，但作用在<strong>累积"
                                "滑动平均</strong>上，滤掉单窗 spike造成的假「未稳」。</p>"));
        m.insert(QStringLiteral("stopping_thres"),
                 QStringLiteral("<p>正式统计阶段：连续窗之间若指标相对波动低于该值，认为"
                                "曲线已平，结束仿真。设得过严会永远跑不满 "
                                "<code>max_samples</code>。</p>"));
        m.insert(QStringLiteral("acc_stopping_thres"),
                 QStringLiteral("<p>在累积均值上判停：适合延迟曲线长尾重、单窗噪声大的场景；"
                                "与 <code>stopping_thres</code> 二选一或分层使用取决于 BookSim"
                                "版本配置解析顺序。</p>"));
        m.insert(QStringLiteral("sim_count"),
                 QStringLiteral("<p>同一配置文件串行跑多次试验，每次可用不同 "
                                "<code>seed</code>或 traffic实现；用于估置信区间或画误差"
                                "棒，而不是改网络结构。</p>"));
        m.insert(QStringLiteral("include_queuing"),
                 QStringLiteral("<p>端到端包延迟样本是否包含包头创建后、注入网络前在源端的排队"
                                "时间；关闭时延迟更接近纯网内段（与 nlat/plat 分解一致）。</p>"));
        m.insert(QStringLiteral("seed"),
                 QStringLiteral("<p>驱动注入伯努利试投、随机流量、仲裁扰动等的主 RNG。"
                                "固定可复现整条实验；设 <code>time</code> 则每次启动不同。"
                                "<code>perm_seed</code>、<code>fail_seed</code> 与之独立分流。"
                                "</p>"));
        m.insert(QStringLiteral("print_activity"),
                 QStringLiteral("<p>各仿真周期可向标准输出写入大量路由器内部状态，适于与波形或"
                                "日志对照调试；在大规模网络上长时间启用时 I/O 易成为瓶颈。</p>"));
        m.insert(QStringLiteral("print_csv_results"),
                 QStringLiteral("<p>在仿真收尾额外写一行或多行 CSV，列名与 BookSim 版本绑定；"
                                "BookCanvas 结果页若解析 stdout，可与该文件交叉验证。</p>"));
        m.insert(QStringLiteral("deadlock_warn_timeout"),
                 QStringLiteral("<p>全局 flit 计数或活动检测连续停滞超过该周期数，打印「疑似"
                                "死锁」——可能是路由环、VC 分配成环、或阈值设太短误判慢包。"
                                "</p>"));
        m.insert(QStringLiteral("viewer_trace"),
                 QStringLiteral("<p>输出结构化 trace（格式随 BookSim 版本），供外部 Java/Python "
                                "viewer 做动画；体积介于 print_activity 与纯统计之间。</p>"));
        m.insert(QStringLiteral("watch_file"),
                 QStringLiteral("<p>将待跟踪标识列表置于磁盘，避免命令行过长；解析器按行或"
                                "空格切分，与内联 <code>watch_flits</code> 等字段功能重叠，"
                                "具体优先级以 BookSim 实现为准。</p>"));
        m.insert(QStringLiteral("watch_flits"),
                 QStringLiteral("<p>按微片标识过滤：用于追踪 wormhole 流水线上某一 flit 在各跳、"
                                "各虚通道上的滞留情况；标识由 BookSim 在创建 flit 时分配。</p>"));
        m.insert(QStringLiteral("watch_packets"),
                 QStringLiteral("<p>按分组标识过滤：可查看同一包内全部 flit 的轨迹，较微片级列表"
                                "更短，适于粗粒度时延核对。</p>"));
        m.insert(QStringLiteral("watch_transactions"),
                 QStringLiteral("<p>在 <code>use_read_write</code> 语义下，按事务 ID 关联"
                                "读/写请求与应答，查一致性或往返路径。</p>"));
        m.insert(QStringLiteral("watch_out"),
                 QStringLiteral("<p>把 watch 输出从 stdout 剥离开，便于大仿真只落盘关心的"
                                "子集；路径相对 BookSim 工作目录解析。</p>"));
        m.insert(QStringLiteral("stats_out"),
                 QStringLiteral("<p>除控制台摘要外再写一份人类可读统计；与 "
                                "<code>print_csv_results</code> 互补，一者偏叙述一者偏表。"
                                "</p>"));
        m.insert(QStringLiteral("sim_power"),
                 QStringLiteral("<p>调用 <code>Power_Module</code>：在 flit 活动时累加开关功耗，"
                                "空闲时累加泄漏；需要有效 <code>tech_file</code>，否则电容电阻"
                                "参数无意义。CPU 时间通常乘数倍。</p>"));
        m.insert(QStringLiteral("power_output_file"),
                 QStringLiteral("<p>功耗模块写文件的 basename；不同模块可能追加 "
                                "<code>_router</code>、<code>_link</code> 等后缀，勿在多进程"
                                "实验里共用同名以免覆盖。</p>"));
        m.insert(QStringLiteral("tech_file"),
                 QStringLiteral("<p>自定义文本格式描述线电容、管脚电容、电压、金属间距等；"
                                "解析进 <code>PowerConfig</code>。空路径或缺字段会让内部变量"
                                "保持0，功耗读数不可信。</p>"));
        m.insert(QStringLiteral("channel_width"),
                 QStringLiteral("<p>比特宽度进入线电容模型：宽总线翻转能耗高但每比特能耗可"
                                "摊薄；与 <code>channel_sweep</code> 联动做总线宽度 PPA扫参。"
                                "</p>"));
        m.insert(QStringLiteral("channel_sweep"),
                 QStringLiteral("<p>自动化实验：从当前位宽开始，每轮减 <code>channel_sweep</code>，"
                                "重跑功耗流程直到宽度归零，得到能耗-位宽包络。为 0 关闭扫参。"
                                "</p>"));
        return m;
    }();
    if (const auto it = kDetails.constFind(key); it != kDetails.cend()) {
        return it.value();
    }
    return QStringLiteral(
               "<p>键名 <code>%1</code> 未在使用说明中单独维护释义；请以 BookCanvas 全局配置"
               "界面标签为准，并对照 BookSim 源码 <code>booksim_config.cpp</code> 中默认值与"
               "字段注释。</p>")
        .arg(key);
}

[[nodiscard]] QVector<ConfigParamGroup> configParamGroups() {
    return {
        {QStringLiteral("网络拓扑参数"),
         QStringLiteral("图结构、几何尺度、端口度与随机断链：决定「连谁」与「线有多长」。"),
         QStringLiteral("拓扑 参数 k n c x y xr yr link_failures fail_seed in_ports out_ports"),
         {QStringLiteral("channel_file"),
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
          QStringLiteral("out_ports")}},
        {QStringLiteral("路由参数"),
         QStringLiteral("选路算法注册名与是否按片上图距离拉长线延迟。"),
         QStringLiteral("routing_function use_noc_latency"),
         {QStringLiteral("routing_function"), QStringLiteral("use_noc_latency")}},
        {QStringLiteral("路由器基础参数"),
         QStringLiteral("微架构后端选型、出口/信用时序、内部加速与输出队列建模。"),
         QStringLiteral("router output_delay credit_delay internal_speedup output_buffer_size noq"),
         {QStringLiteral("router"),
          QStringLiteral("output_delay"),
          QStringLiteral("credit_delay"),
          QStringLiteral("internal_speedup"),
          QStringLiteral("output_buffer_size"),
          QStringLiteral("noq")}},
        {QStringLiteral("虚拟通道配置"),
         QStringLiteral("队列条数、缓冲会计、反馈与 VA 行为微调。"),
         QStringLiteral("num_vcs vc_buf_size buf_size buffer_policy private buffer feedback"),
         {QStringLiteral("num_vcs"),
          QStringLiteral("vc_buf_size"),
          QStringLiteral("buf_size"),
          QStringLiteral("buffer_policy"),
          QStringLiteral("private_bufs"),
          QStringLiteral("private_buf_size"),
          QStringLiteral("private_buf_start_vc"),
          QStringLiteral("private_buf_end_vc"),
          QStringLiteral("max_held_slots"),
          QStringLiteral("feedback_aging_scale"),
          QStringLiteral("feedback_offset"),
          QStringLiteral("wait_for_tail_credit"),
          QStringLiteral("vc_busy_when_full"),
          QStringLiteral("vc_prioritize_empty"),
          QStringLiteral("vc_priority_donation"),
          QStringLiteral("vc_shuffle_requests")}},
        {QStringLiteral("推测执行参数"),
         QStringLiteral("是否启用推测式 SA（先于 VA 完成）、以及对 eligibility、credit 与请求掩码的"
                        "校验。"),
         QStringLiteral("speculative spec_check spec_mask spec_sw_allocator"),
         {QStringLiteral("speculative"),
          QStringLiteral("spec_check_elig"),
          QStringLiteral("spec_check_cred"),
          QStringLiteral("spec_mask_by_reqs"),
          QStringLiteral("spec_sw_allocator")}},
        {QStringLiteral("流水线延迟参数"),
         QStringLiteral("RC/VA/SA 流水拍与 crossbar 前后级寄存，决定零负载跳延迟与气泡。"),
         QStringLiteral(
             "routing_delay vc_alloc_delay sw_alloc_delay st_prepare_delay st_final_delay"),
         {QStringLiteral("routing_delay"),
          QStringLiteral("vc_alloc_delay"),
          QStringLiteral("sw_alloc_delay"),
          QStringLiteral("st_prepare_delay"),
          QStringLiteral("st_final_delay")}},
        {QStringLiteral("端口加速参数"),
         QStringLiteral("交换矩阵有效维度扩展、输出端口时间复用度，以及分组级交叉开关配置保持。"),
         QStringLiteral("input_speedup output_speedup hold_switch_for_packet"),
         {QStringLiteral("input_speedup"),
          QStringLiteral("output_speedup"),
          QStringLiteral("hold_switch_for_packet")}},
        {QStringLiteral("交换分配参数"),
         QStringLiteral("是否按 virtual cut-through 规则预约下游缓冲。"),
         QStringLiteral("vct"),
         {QStringLiteral("vct")}},
        {QStringLiteral("仲裁器参数"),
         QStringLiteral("VC 与 crossbar 匹配算法族、子仲裁策略、单周期迭代深度。"),
         QStringLiteral("vc_allocator sw_allocator arb_type alloc_iters"),
         {QStringLiteral("vc_allocator"),
          QStringLiteral("sw_allocator"),
          QStringLiteral("arb_type"),
          QStringLiteral("alloc_iters")}},
        {QStringLiteral("流量参数"),
         QStringLiteral("谁跟谁通信、多类 QoS、到达过程、包长混布与批脉冲。"),
         QStringLiteral("traffic injection_rate batch burst priority"),
         {QStringLiteral("classes"),
          QStringLiteral("traffic"),
          QStringLiteral("class_priority"),
          QStringLiteral("perm_seed"),
          QStringLiteral("injection_rate"),
          QStringLiteral("injection_rate_uses_flits"),
          QStringLiteral("packet_size"),
          QStringLiteral("packet_size_rate"),
          QStringLiteral("injection_process"),
          QStringLiteral("burst_alpha"),
          QStringLiteral("burst_beta"),
          QStringLiteral("burst_r1"),
          QStringLiteral("priority"),
          QStringLiteral("batch_size"),
          QStringLiteral("batch_count"),
          QStringLiteral("max_outstanding_requests")}},
        {QStringLiteral("读写请求参数"),
         QStringLiteral("四类事务的 VC 窗、子网与 flit 长度，模拟内存/一致性流量。"),
         QStringLiteral("read write request reply vc subnet size"),
         {QStringLiteral("use_read_write"),
          QStringLiteral("write_fraction"),
          QStringLiteral("read_request_begin_vc"),
          QStringLiteral("read_request_end_vc"),
          QStringLiteral("write_request_begin_vc"),
          QStringLiteral("write_request_end_vc"),
          QStringLiteral("read_reply_begin_vc"),
          QStringLiteral("read_reply_end_vc"),
          QStringLiteral("write_reply_begin_vc"),
          QStringLiteral("write_reply_end_vc"),
          QStringLiteral("read_request_subnet"),
          QStringLiteral("read_reply_subnet"),
          QStringLiteral("write_request_subnet"),
          QStringLiteral("write_reply_subnet"),
          QStringLiteral("read_request_size"),
          QStringLiteral("write_request_size"),
          QStringLiteral("read_reply_size"),
          QStringLiteral("write_reply_size")}},
        {QStringLiteral("仿真控制参数"),
         QStringLiteral("latency/throughput 模式、预热与收敛判据、日志、观测与统计落盘。"),
         QStringLiteral("sim_type warmup sample seed watch stats"),
         {QStringLiteral("sim_type"),
          QStringLiteral("warmup_periods"),
          QStringLiteral("sample_period"),
          QStringLiteral("max_samples"),
          QStringLiteral("measure_stats"),
          QStringLiteral("pair_stats"),
          QStringLiteral("latency_thres"),
          QStringLiteral("warmup_thres"),
          QStringLiteral("acc_warmup_thres"),
          QStringLiteral("stopping_thres"),
          QStringLiteral("acc_stopping_thres"),
          QStringLiteral("sim_count"),
          QStringLiteral("include_queuing"),
          QStringLiteral("seed"),
          QStringLiteral("print_activity"),
          QStringLiteral("print_csv_results"),
          QStringLiteral("deadlock_warn_timeout"),
          QStringLiteral("viewer_trace"),
          QStringLiteral("watch_file"),
          QStringLiteral("watch_flits"),
          QStringLiteral("watch_packets"),
          QStringLiteral("watch_transactions"),
          QStringLiteral("watch_out"),
          QStringLiteral("stats_out")}},
        {QStringLiteral("功耗相关参数"),
         QStringLiteral("工艺表、位宽、结果文件与可选位宽扫参。"),
         QStringLiteral("sim_power power_output_file tech_file channel_width channel_sweep"),
         {QStringLiteral("sim_power"),
          QStringLiteral("power_output_file"),
          QStringLiteral("tech_file"),
          QStringLiteral("channel_width"),
          QStringLiteral("channel_sweep")}},
        {QStringLiteral("其他"),
         QStringLiteral("界面显隐、导出规则与画布拓扑覆盖。"),
         QStringLiteral("其他 显隐 导出 画布 覆盖"),
         {}},
    };
}

void enableAutoHeight(QTextBrowser* browser, int minHeight = 0) {
    if (!browser || !browser->document() || !browser->document()->documentLayout()) {
        return;
    }

    browser->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    browser->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    browser->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    const auto updateHeight = [browser, minHeight]() {
        if (!browser || !browser->document()) {
            return;
        }
        browser->document()->setTextWidth(browser->viewport()->width());
        const int docHeight = qCeil(browser->document()->size().height());
        const int frame = browser->frameWidth() * 2;
        const QMargins margins = browser->contentsMargins();
        const int padding = 6;
        browser->setFixedHeight(
            qMax(minHeight, docHeight + frame + margins.top() + margins.bottom() + padding));
    };

    QObject::connect(browser->document()->documentLayout(),
                     &QAbstractTextDocumentLayout::documentSizeChanged,
                     browser,
                     [updateHeight](const QSizeF&) { updateHeight(); });
    updateHeight();
}

struct MetricTermCard {
    QString key;
    QString summary;
    QString details;
    QString searchable;
};

QVector<MetricTermCard> metricTermCards() {
    return {
        {BookSimMetricLabels::packetLatency(),
         QString(),
         QStringLiteral(
             "<h4>定义</h4>"
             "<p>BookSim "
             "在尾微片退休时统计的包级端到端时延（plat）：从包头<strong>创建时刻</strong>到"
             "尾微片在目的端到达时刻，单位为仿真周期。</p>"
             "<p>与网内延迟（nlat）的关系：<strong>端到端包延迟 = 网内延迟 + "
             "源端排队时间</strong>；"
             "其中源端排队时间为包头创建至注入网络前在源端等待的时间（对应 "
             "<code>itime<sub>head</sub> "
             "− ctime<sub>head</sub></code> 等口径）。</p>"
             "<h4>公式</h4>"
             "<p><code>plat = t<sub>arrive,tail</sub> − t<sub>create,head</sub></code>"
             "（仿真内对应 <code>atime<sub>tail</sub> − ctime<sub>head</sub></code>）</p>"),
         QStringLiteral("结果参数 延迟 微片 plat packet latency 端到端")},
        {BookSimMetricLabels::networkLatency(),
         QString(),
         QStringLiteral(
             "<h4>定义</h4>"
             "<p>BookSim "
             "统计的网内段时延（nlat）：尾微片到达时刻与包头<strong>注入网络时刻</strong>之差，"
             "含网内路由、排队与竞争，不含创建到注入之间的等待。</p>"
             "<h4>公式</h4>"
             "<p><code>nlat = t<sub>arrive,tail</sub> − t<sub>inject,head</sub></code>"
             "（<code>atime<sub>tail</sub> − itime<sub>head</sub></code>）</p>"),
         QStringLiteral("结果参数 nlat network latency 网内")},
        {BookSimMetricLabels::flitLatency(),
         QString(),
         QStringLiteral("<h4>定义</h4>"
                        "<p>对每个微片统计「到达 − "
                        "注入」再取样本平均（flat），反映微片粒度上传输与流控带来的时延。</p>"
                        "<h4>公式</h4>"
                        "<p>对微片 <code>j</code>：<code>lat<sub>j</sub> = atime<sub>j</sub> − "
                        "itime<sub>j</sub></code>；"
                        "<code>flat = mean(lat<sub>j</sub>)</code></p>"
                        "<h4>提示</h4>"
                        "<p>通常小于同页包级端到端延迟；若差距异常需结合包长与打散一并看。</p>"),
         QStringLiteral("结果参数 flit latency flat 微片")},
        {BookSimMetricLabels::fragmentation(),
         QString(),
         QStringLiteral(
             "<h4>定义</h4>"
             "<p>"
             "同一包内微片在目的端到达时间的「打散」程度：实际到达跨度与按微片序号理想跨"
             "度的差。</p>"
             "<h4>公式</h4>"
             "<p>在尾微片样本上统计，<code>f</code> 为尾微片、"
             "<code>head</code> 为包头微片：</p>"
             "<p><code>frag = (f-&gt;atime − head-&gt;atime) − (f-&gt;id − head-&gt;id)</code></p>"
             "<ul style=\"margin-top:0.5em;\">"
             "<li><code>f-&gt;atime − head-&gt;atime</code>：尾 flit 与头 flit 的到达时间差</li>"
             "<li><code>f-&gt;id − head-&gt;id</code>：包内 flit 数量（ID 差值）</li>"
             "<li><code>Fragmentation</code>：实际到达时间间隔 − 理想连续到达时间间隔</li>"
             "</ul>"
             "<h4>提示</h4>"
             "<p>数值越大表示微片到达越分散；持续为 0 "
             "表示到达紧凑。</p>"),
         QStringLiteral("结果参数 fragmentation 打散")},
        {BookSimMetricLabels::throughputMatch(),
         QString(),
         QStringLiteral(
             "<h4>定义</h4>"
             "<p>"
             "结果页根据包级速率计算的匹配程度：接纳相对注入的百分比，用于快速判断网络是否跟上提供"
             "负载。</p>"
             "<h4>公式</h4>"
             "<p><code>吞吐匹配度 = (R<sub>acc, pkt</sub> / R<sub>inj, pkt</sub>) × 100%</code>；"
             "注入包速率为 0 时不定义，界面显示「—」。</p>"
             "<h4>提示</h4>"
             "<p>接近 100% 一般表示匹配良好；明显低于 100% "
             "且持续，需结合延迟与速率行排查拥塞或反压。</p>"),
         QStringLiteral("结果参数 throughput match 吞吐匹配 eta")},
        {BookSimMetricLabels::injectedPacketRate(),
         QString(),
         QStringLiteral("<h4>定义</h4>"
                        "<p>源端向网络注入包的平均速率，单位为每仿真周期包数（pkt/cycle）。</p>"),
         QStringLiteral("结果参数 injected packet rate pkt")},
        {BookSimMetricLabels::acceptedPacketRate(),
         QString(),
         QStringLiteral("<h4>定义</h4>"
                        "<p>目的端成功完成接收的包平均速率，单位为 pkt/cycle。</p>"),
         QStringLiteral("结果参数 accepted packet rate")},
        {BookSimMetricLabels::injectedFlitRate(),
         QString(),
         QStringLiteral("<h4>定义</h4>"
                        "<p>注入侧微片平均速率，单位为每周期微片数（flits/cycle）。</p>"),
         QStringLiteral("结果参数 injected flit rate")},
        {BookSimMetricLabels::acceptedFlitRate(),
         QString(),
         QStringLiteral("<h4>定义</h4>"
                        "<p>目的端接纳微片的平均速率，单位为 flits/cycle。</p>"),
         QStringLiteral("结果参数 accepted flit rate")},
        {BookSimMetricLabels::injectedMeanPacketSize(),
         QString(),
         QStringLiteral(
             "<h4>定义</h4>"
             "<p>注入业务侧统计得到的平均包长，以微片（flit）为单位。</p>"
             "<h4>提示</h4>"
             "<p>与配置包长分布一致即可；与接纳平均包长差异大时需检查是否有截断或统计类不一致。</"
             "p>"),
         QStringLiteral("结果参数 injected packet size flits")},
        {BookSimMetricLabels::acceptedMeanPacketSize(),
         QString(),
         QStringLiteral(
             "<h4>定义</h4>"
             "<p>目的端观测到的平均包长，以微片为单位。</p>"
             "<h4>提示</h4>"
             "<p>正常情况下与注入平均包长接近；读数用于验证仿真是否按预期包形态运行。</p>"),
         QStringLiteral("结果参数 accepted packet size")},
        {BookSimMetricLabels::meanHops(),
         QString(),
         QStringLiteral("<h4>定义</h4>"
                        "<p>"
                        "包从源到目的在路由器之间转发的平均跳数；统计口径为经过的路由器跳步，不含终"
                        "端节点。</p>"
                        "<h4>提示</h4>"
                        "<p>"
                        "与拓扑最短路径下界比较可判断是否绕行；跳数稳定而延迟上升时更可能是竞争而非"
                        "路径变长。</p>"),
         QStringLiteral("结果参数 hops平均跳数 mean hops")}};
}

} // namespace

UsageGuidePage::UsageGuidePage(QWidget* parent)
    : BasePage(parent) {
    setWindowTitle(tr("使用说明"));

    auto* centralWidget = new QWidget(this);
    centralWidget->setWindowTitle(tr("使用说明"));
    auto* centerLayout = new QVBoxLayout(centralWidget);
    centerLayout->setContentsMargins(0, 0, 20, 0);
    centerLayout->setSpacing(10);

    m_searchEdit = new ElaLineEdit(this);
    m_searchEdit->setPlaceholderText(tr("搜索关键词"));
    centerLayout->addWidget(m_searchEdit);

    m_statusLabel = new QLabel(QString(), this);
    m_statusLabel->setWordWrap(true);
    centerLayout->addWidget(m_statusLabel);

    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);

    m_sectionHost = new QWidget(scroll);
    m_sectionLayout = new QVBoxLayout(m_sectionHost);
    m_sectionLayout->setContentsMargins(0, 0, 0, 0);
    m_sectionLayout->setSpacing(10);

    addSection(
        tr("快捷键"),
        tr("操作"),
        tr("Canvas 页面支持键盘放置、平移与缩放。"),
        tr("<h4>Tab 管理</h4>"
           "<p><b>Ctrl+Tab</b> / <b>Ctrl+Shift+Tab</b> 与点击标题栏右侧 <b>&lt; &gt;</b> "
           "按钮相同（循环）。</p>"
           "<ul>"
           "<li>新建：<b>Ctrl+T</b>；macOS 另支持 <b>⌘T</b>。</li>"
           "<li>关闭：<b>Ctrl+W</b>；macOS <b>⌘W</b>（仅剩一个时会先新开再关）。</li>"
           "<li>上一个 / 下一个：<b>Ctrl+Shift+Tab</b> / <b>Ctrl+Tab</b>（与 ◀ ▶ 一致）。</li>"
           "</ul>"
           "<h4>放置与模式切换</h4>"
           "<ul>"
           "<li><b>N</b>：进入“终端点击放置”模式。</li>"
           "<li><b>R</b>：进入“路由器点击放置”模式。</li>"
           "<li><b>Esc</b>：退出点击放置模式，回到普通编辑状态。</li>"
           "</ul>"
           "<h4>撤销与重做</h4>"
           "<ul>"
           "<li><b>Windows / Linux</b>：<b>Ctrl + Z</b> 撤销，<b>Ctrl + Y</b>（或 "
           "<b>Ctrl + Shift + Z</b>）重做。</li>"
           "<li><b>macOS</b>：<b>Command + Z</b> 撤销，<b>Command + Y</b>（或 "
           "<b>Command + Shift + Z</b>）重做。</li>"
           "</ul>"
           "<h4>视口平移</h4>"
           "<ul>"
           "<li><b>↑ / ↓ / ← / →</b>：按固定步长平移画布视口。该操作与当前缩放比例解耦，"
           "在大图或空白区域同样可用。</li>"
           "</ul>"
           "<h4>缩放控制</h4>"
           "<ul>"
           "<li><b>Windows / Linux</b>：<b>Ctrl + +</b> 放大，<b>Ctrl + -</b> 缩小，"
           "<b>Ctrl + 0</b> 重置为 1.0 倍。</li>"
           "<li><b>macOS</b>：<b>Command + +</b> 放大，<b>Command + -</b> 缩小，"
           "<b>Command + 0</b> 重置为 1.0 倍。</li>"
           "</ul>"),
        QStringLiteral("快捷键 canvas tab ctrl+tab 切换 新建 关闭 cmd+w ctrl+t cmd+t n r "
                       "esc 平移 缩放 undo redo"));

    addSection(
        tr("文件路径"),
        tr("路径"),
        tr("应用设置与仿真记录在用户配置目录；BookSim 导出的网络拓扑与全局 JSON 默认在 BookSim "
           "工作目录，可在「设置」中修改。"),
        tr("<h4>macOS / Linux</h4>"
           "<ul>"
           "<li>配置目录：<b>~/.config/Book-Canvas/</b></li>"
           "<li>设置文件：<b>~/.config/Book-Canvas/settings.ini</b></li>"
           "<li>仿真记录：<b>~/.config/Book-Canvas/simulation_records.json</b></li>"
           "</ul>"
           "<h4>Windows</h4>"
           "<ul>"
           "<li>配置目录：<b>%APPDATA%\\Local\\Book-Canvas\\</b></li>"
           "<li>设置文件：<b>%APPDATA%\\Local\\Book-Canvas\\settings.ini</b></li>"
           "<li>仿真记录：<b>%APPDATA%\\Local\\Book-Canvas\\simulation_records.json</b></li>"
           "</ul>"
           "<h4>BookSim 网络拓扑与全局配置（默认导出路径）</h4>"
           "<ul>"
           "<li><b>网络拓扑文件</b>：默认 <b>&lt;BookSim "
           "工作目录&gt;/anynet_file</b>（与全局配置里 "
           "<code>network_file</code> 的默认文件名一致，通常无扩展名）。工作目录由 BookSim "
           "可执行路径推导；开发构建常见为工程内 <b>3rdpart/booksim2/src</b>。</li>"
           "<li><b>全局配置文件（JSON）</b>：默认 <b>&lt;BookSim "
           "工作目录&gt;/anynet_config.json</b>。"
           "</li>"
           "<li>上述两项可在「设置」中通过「拓扑文件模板」「JSON 配置模板」改为任意路径；多 "
           "Tab 导出时文件名可能自动追加后缀。</li>"
           "</ul>"),
        QStringLiteral("文件 路径 settings.ini simulation_records.json anynet_file "
                       "anynet_config.json 拓扑 导出 "
                       "json 全局配置 book-canvas 存储 目录 windows mac linux appdata njupt"));

    SectionUi* paramSection = addSection(
        tr("配置参数"),
        tr("建模"),
        tr("按 BookSim 语义分组：拓扑与几何、路由器微架构、流控与分配器、流量与事务、统计与"
           "功耗。每组下为参数名卡片，摘要与详述均针对该键单独撰写。"),
        QString(),
        QStringLiteral(
            "配置参数 参数 核心参数 二级卡片 嵌套 card nested topology 路由算法概览 "
            "routing_function router iq "
            "event chaos k n c traffic "
            "injection_rate num_vcs vc_buf_size warmup_periods sample_period max_samples "
            "sim_type link_failures fail_seed 链路故障 随机故障 in_ports out_ports 输入端口 "
            "输出端口"));

    if (paramSection) {
        for (const ConfigParamGroup& group : configParamGroups()) {
            SubSectionUi* sub = addSubSection(*paramSection,
                                              group.title,
                                              group.summary,
                                              QString(),
                                              group.searchable);
            if (!sub) {
                continue;
            }
            for (const QString& key : group.keys) {
                addThirdSection(*sub,
                                key,
                                configParamSummary(key),
                                configParamDetail(key),
                                key + QStringLiteral(" ") + configParamSummary(key));
            }
            if (group.title == QLatin1String("其他")) {
                addThirdSection(
                    *sub,
                    tr("参数显隐与导出"),
                    tr("页面只显示当前拓扑相关字段，隐藏字段会保留内存值并参与导出。"),
                    tr("<p>为了减少误配，部分参数会随 <code>topology</code> 动态显示或锁定。"
                       "被隐藏的字段不会丢失，导出时仍按“当前内存值或默认值”写入。</p>"),
                    tr("拓扑 显隐 导出 合并"));
                addThirdSection(
                    *sub,
                    tr("数值与路径"),
                    tr("相对路径相对 BookSim 工作目录解析；跨机复现宜用绝对路径。"),
                    tr("<p>仅含文件名的路径按 BookSim 工作目录解析；需要精确定位或跨机复现时"
                       "使用绝对路径。</p>"),
                    tr("路径 工作目录 绝对路径"));
                addThirdSection(
                    *sub,
                    tr("与画布拓扑块关系"),
                    tr("当画布只有一个拓扑块时，topology/k/n/c/routing_function 会由该块覆盖。"),
                    tr("<p>导出配置时，若画布存在且仅存在一个拓扑块，系统会优先写入该块参数。"
                       "存在多个拓扑块时，会保留全局配置对应字段并给出提示。</p>"),
                    tr("画布 拓扑块 覆盖 全局配置"));
            }
        }
    }

    SectionUi* metricSection = addSection(
        tr("结果参数"),
        tr("解释"),
        tr("与仿真结果页指标一一对应；每项含定义、公式（HTML）与提示。支持搜索。"),
        QString(),
        QStringLiteral(
            "结果参数 结果参数含义 指标 术语 packet latency network latency throughput accepted "
            "rate "
            "saturation "
            "hol blocking tail latency p99 "
            "offered load accepted load jitter backpressure bisection bandwidth 收敛 拐点 "
            "核心 kpi 端到端包延迟 吞吐匹配度 网内延迟 平均跳数 注入排队 饱和判据 "
            "accepted_rate offered_rate eta_match h_avg h_lb rho_hop "
            "延迟与微片打散 fragmentation flit latency plat nlat flat "
            "吞吐与接纳 包率 flit 率 pkt/cycle injected accepted packet rate flit rate"));

    if (metricSection) {
        for (const MetricTermCard& card : metricTermCards()) {
            addSubSection(*metricSection,
                          tr("%1").arg(card.key),
                          card.summary,
                          card.details,
                          card.searchable);
        }
    }

    m_sectionLayout->addStretch(1);
    scroll->setWidget(m_sectionHost);
    centerLayout->addWidget(scroll, 1);

    connect(m_searchEdit, &ElaLineEdit::textChanged, this, [this]() { applySearchFilter(); });
    connect(eTheme, &ElaTheme::themeModeChanged, this, [this](ElaThemeType::ThemeMode) {
        applyTheme();
    });

    addCentralWidget(centralWidget, true, true, 0);
    applyTheme();
}

QWidget* UsageGuidePage::createTemplateRow(const QVector<QPair<QString, QString>>& templates,
                                           QWidget* parent) {
    auto* rowWrap = new QWidget(parent);
    auto* row = new QHBoxLayout(rowWrap);
    row->setContentsMargins(0, 0, 0, 0);
    row->setSpacing(8);
    for (const auto& t : templates) {
        auto* btn = new ElaPushButton(t.first, rowWrap);
        QObject::connect(btn, &ElaPushButton::clicked, this, [this, t]() {
            if (QClipboard* clip = QApplication::clipboard()) {
                clip->setText(t.second);
                if (m_statusLabel) {
                    m_statusLabel->setText(tr("已复制模板：%1").arg(t.first));
                }
            }
        });
        row->addWidget(btn);
    }
    row->addStretch(1);
    return rowWrap;
}

UsageGuidePage::SectionUi* UsageGuidePage::addSection(
    const QString& title,
    const QString& category,
    const QString& summary,
    const QString& richText,
    const QString& searchableText,
    const QVector<QPair<QString, QString>>& templates) {
    auto* card = new QFrame(m_sectionHost);
    card->setFrameShape(QFrame::StyledPanel);

    auto* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(10, 8, 10, 10);
    cardLayout->setSpacing(8);

    auto* headRow = new QWidget(card);
    auto* headLay = new QHBoxLayout(headRow);
    headLay->setContentsMargins(0, 0, 0, 0);
    headLay->setSpacing(8);

    auto* toggle = new QToolButton(headRow);
    toggle->setText(title);
    toggle->setCheckable(true);
    toggle->setChecked(false);
    toggle->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    toggle->setArrowType(Qt::RightArrow);
    toggle->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    headLay->addWidget(toggle, 1);
    cardLayout->addWidget(headRow);

    auto* body = new QWidget(card);
    auto* bodyLay = new QVBoxLayout(body);
    bodyLay->setContentsMargins(0, 0, 0, 0);
    bodyLay->setSpacing(8);

    QTextBrowser* browser = nullptr;
    if (!richText.trimmed().isEmpty()) {
        browser = new QTextBrowser(body);
        browser->setOpenExternalLinks(true);
        browser->setReadOnly(true);
        browser->setHtml(richText);
        enableAutoHeight(browser, 48);
        bodyLay->addWidget(browser);
    }

    if (!templates.isEmpty()) {
        bodyLay->addWidget(createTemplateRow(templates, body));
    }

    cardLayout->addWidget(body);
    body->setVisible(false);

    connect(toggle, &QToolButton::toggled, this, [toggle, body](bool on) {
        body->setVisible(on);
        toggle->setArrowType(on ? Qt::DownArrow : Qt::RightArrow);
    });

    SectionUi sec;
    sec.category = category;
    sec.title = title;
    sec.searchableText = (category + u' ' + title + u' ' + summary + u' ' + searchableText).toLower();
    sec.card = card;
    sec.body = body;
    sec.toggle = toggle;
    sec.categoryLabel = nullptr;
    sec.summaryLabel = nullptr;
    sec.browser = browser;
    m_sections.append(sec);
    m_sectionLayout->addWidget(card);
    return &m_sections.last();
}

UsageGuidePage::SubSectionUi* UsageGuidePage::addSubSection(SectionUi& parent,
                                                            const QString& title,
                                                            const QString& summary,
                                                            const QString& richText,
                                                            const QString& searchableText) {
    if (!parent.body) {
        return nullptr;
    }

    auto* bodyLay = qobject_cast<QVBoxLayout*>(parent.body->layout());
    if (!bodyLay) {
        return nullptr;
    }

    auto* card = new QFrame(parent.body);
    card->setFrameShape(QFrame::StyledPanel);

    auto* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(10, 8, 10, 10);
    cardLayout->setSpacing(8);

    auto* headRow = new QWidget(card);
    auto* headLay = new QHBoxLayout(headRow);
    headLay->setContentsMargins(0, 0, 0, 0);
    headLay->setSpacing(8);

    auto* toggle = new QToolButton(headRow);
    toggle->setText(title);
    toggle->setCheckable(true);
    toggle->setChecked(false);
    toggle->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    toggle->setArrowType(Qt::RightArrow);
    toggle->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    headLay->addWidget(toggle, 1);
    cardLayout->addWidget(headRow);

    auto* body = new QWidget(card);
    auto* nestedBodyLay = new QVBoxLayout(body);
    nestedBodyLay->setContentsMargins(0, 0, 0, 0);
    nestedBodyLay->setSpacing(8);

    auto* browser = new QTextBrowser(body);
    browser->setOpenExternalLinks(true);
    browser->setReadOnly(true);
    const QString htmlBody = summary.trimmed().isEmpty()
                                 ? richText
                                 : QStringLiteral("<p>%1</p>%2")
                                       .arg(summary.toHtmlEscaped(), richText);
    browser->setHtml(htmlBody);
    enableAutoHeight(browser, 42);
    nestedBodyLay->addWidget(browser);

    cardLayout->addWidget(body);
    body->setVisible(false);

    connect(toggle, &QToolButton::toggled, this, [toggle, body](bool on) {
        body->setVisible(on);
        toggle->setArrowType(on ? Qt::DownArrow : Qt::RightArrow);
    });

    SubSectionUi sub;
    sub.title = title;
    sub.searchableText = (title + u' ' + summary + u' ' + searchableText).toLower();
    sub.card = card;
    sub.body = body;
    sub.toggle = toggle;
    sub.browser = browser;
    parent.children.append(sub);
    bodyLay->addWidget(card);
    return &parent.children.last();
}

void UsageGuidePage::addThirdSection(SubSectionUi& parent,
                                     const QString& title,
                                     const QString& summary,
                                     const QString& richText,
                                     const QString& searchableText) {
    if (!parent.body) {
        return;
    }
    auto* bodyLay = qobject_cast<QVBoxLayout*>(parent.body->layout());
    if (!bodyLay) {
        return;
    }

    auto* card = new QFrame(parent.body);
    card->setFrameShape(QFrame::StyledPanel);

    auto* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(8, 6, 8, 8);
    cardLayout->setSpacing(6);

    auto* toggle = new QToolButton(card);
    toggle->setText(title);
    toggle->setCheckable(true);
    toggle->setChecked(false);
    toggle->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    toggle->setArrowType(Qt::RightArrow);
    toggle->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    cardLayout->addWidget(toggle);

    auto* body = new QWidget(card);
    auto* nestedLay = new QVBoxLayout(body);
    nestedLay->setContentsMargins(0, 0, 0, 0);
    nestedLay->setSpacing(6);

    auto* browser = new QTextBrowser(body);
    browser->setOpenExternalLinks(true);
    browser->setReadOnly(true);
    const QString htmlBody = summary.trimmed().isEmpty()
                                 ? richText
                                 : QStringLiteral("<p>%1</p>%2")
                                       .arg(summary.toHtmlEscaped(), richText);
    browser->setHtml(htmlBody);
    enableAutoHeight(browser, 38);
    nestedLay->addWidget(browser);

    body->setVisible(false);
    cardLayout->addWidget(body);
    connect(toggle, &QToolButton::toggled, this, [toggle, body](bool on) {
        body->setVisible(on);
        toggle->setArrowType(on ? Qt::DownArrow : Qt::RightArrow);
    });

    ThirdSectionUi third;
    third.title = title;
    third.searchableText = (title + u' ' + summary + u' ' + searchableText).toLower();
    third.card = card;
    third.body = body;
    third.toggle = toggle;
    third.browser = browser;
    parent.children.append(third);
    bodyLay->addWidget(card);
}

void UsageGuidePage::applySearchFilter() {
    const QString key = (m_searchEdit ? m_searchEdit->text().trimmed().toLower() : QString());
    int matchedSections = 0;
    int matchedSubCards = 0;
    int matchedThirdCards = 0;
    for (SectionUi& s : m_sections) {
        const bool sectionHit = key.isEmpty() || s.searchableText.contains(key);
        bool anyChildHit = false;

        for (SubSectionUi& child : s.children) {
            bool anyThirdHit = false;
            for (ThirdSectionUi& third : child.children) {
                const bool thirdHit = key.isEmpty() || third.searchableText.contains(key);
                const bool thirdVisible = key.isEmpty() || sectionHit || thirdHit;
                if (third.card) {
                    third.card->setVisible(thirdVisible);
                }
                if (!key.isEmpty() && thirdVisible && third.toggle && !third.toggle->isChecked()) {
                    third.toggle->setChecked(true);
                }
                if (thirdHit) {
                    anyThirdHit = true;
                    if (!key.isEmpty()) {
                        ++matchedThirdCards;
                    }
                }
            }

            const bool childHit = key.isEmpty() || child.searchableText.contains(key)
                                  || anyThirdHit;
            const bool childVisible = key.isEmpty() || sectionHit || childHit;
            if (child.card) {
                child.card->setVisible(childVisible);
            }
            if (!key.isEmpty() && childVisible && child.toggle && !child.toggle->isChecked()) {
                child.toggle->setChecked(true);
            }
            if (childHit) {
                anyChildHit = true;
                if (!key.isEmpty()) {
                    ++matchedSubCards;
                }
            }
        }

        const bool visible = key.isEmpty() || sectionHit || anyChildHit;
        if (s.card) {
            s.card->setVisible(visible);
        }
        if (visible) {
            ++matchedSections;
            if (!key.isEmpty() && s.toggle && !s.toggle->isChecked()) {
                s.toggle->setChecked(true);
            }
        }
    }
    if (!m_statusLabel) {
        return;
    }
    if (key.isEmpty()) {
        m_statusLabel->clear();
    } else {
        if (matchedSubCards > 0 || matchedThirdCards > 0) {
            m_statusLabel->setText(
                tr("搜索 \"%1\"：匹配到 %2 个分组，%3 个二级卡片，%4 个三级卡片。")
                    .arg(key)
                    .arg(matchedSections)
                    .arg(matchedSubCards)
                    .arg(matchedThirdCards));
        } else {
            m_statusLabel->setText(
                tr("搜索 \"%1\"：匹配到 %2 个分组。").arg(key).arg(matchedSections));
        }
    }
}

void UsageGuidePage::applyTheme() {
    const auto mode = eTheme->getThemeMode();
    const bool isLight = (mode == ElaThemeType::Light);
    const QString border = ElaThemeColor(mode, BasicBorder).name(QColor::HexRgb);
    const QString pageBg = ElaThemeColor(mode, WindowBase).name(QColor::HexRgb);
    const QString cardBg = ElaThemeColor(mode, PopupBase).name(QColor::HexRgb);
    const QString textMain = (isLight ? QColor(Qt::black) : ElaThemeColor(mode, BasicText))
                                 .name(QColor::HexRgb);
    const QString textMuted = (isLight ? QColor(Qt::black) : ElaThemeColor(mode, BasicDetailsText))
                                  .name(QColor::HexRgb);
    const QString textHint = (isLight ? QColor(Qt::black) : ElaThemeColor(mode, BasicTextNoFocus))
                                 .name(QColor::HexRgb);
    const QString editorBg = ElaThemeColor(mode, WindowBase).name(QColor::HexRgb);
    const QString link = ElaThemeColor(mode, PrimaryNormal).name(QColor::HexRgb);

    if (QWidget* root = this->findChild<QWidget*>(QStringLiteral("ElaScrollPage_CentralPage"))) {
        root->setStyleSheet(QStringLiteral("background-color: %1;").arg(pageBg));
    }
    if (m_sectionHost) {
        m_sectionHost->setStyleSheet(QStringLiteral("background-color: %1;").arg(pageBg));
    }

    if (m_statusLabel) {
        m_statusLabel->setStyleSheet(QStringLiteral("color: %1;").arg(textHint));
    }

    for (SectionUi& s : m_sections) {
        if (s.card) {
            s.card->setStyleSheet(
                QStringLiteral(
                    "QFrame { border: 1px solid %1; border-radius: 10px; background: %2; }")
                    .arg(border, cardBg));
        }
        if (s.toggle) {
            s.toggle->setStyleSheet(
                QStringLiteral("QToolButton { border: none; background: transparent; color: %1; "
                               "font-weight: 600; text-align: left; padding: 2px 0; }")
                    .arg(textMain));
        }
        if (s.categoryLabel) {
            s.categoryLabel->setStyleSheet(
                QStringLiteral(
                    "QLabel { border: 1px solid %1; border-radius: 8px; padding: 1px 8px; "
                    "color: %2; background: transparent; }")
                    .arg(border, textHint));
        }
        if (s.summaryLabel) {
            s.summaryLabel->setStyleSheet(QStringLiteral("color: %1;").arg(textMuted));
        }
        if (s.browser) {
            s.browser->setStyleSheet(
                QStringLiteral("QTextBrowser { border: 1px solid %1; border-radius: 8px; "
                               "background-color: %2; color: %3; padding: 6px; }"
                               "QTextBrowser a { color: %4; }")
                    .arg(border, editorBg, textMain, link));
            if (s.browser->document()) {
                s.browser->document()->setDefaultStyleSheet(
                    QStringLiteral("body { color: %1; background-color: %2; }"
                                   "h4 { color: %1; margin: 8px 0 4px 0; }"
                                   "p, li { color: %1; }"
                                   "code { color: %1; background-color: transparent; "
                                   "font-family: Menlo, Consolas, 'Courier New', monospace; }"
                                   "ul, ol { margin-top: 4px; margin-bottom: 4px; }"
                                   "a { color: %3; }")
                        .arg(textMain, editorBg, link));
            }
        }
        for (SubSectionUi& child : s.children) {
            if (child.card) {
                child.card->setStyleSheet(
                    QStringLiteral(
                        "QFrame { border: 1px solid %1; border-radius: 8px; background: %2; }")
                        .arg(border, editorBg));
            }
            if (child.toggle) {
                child.toggle->setStyleSheet(
                    QStringLiteral(
                        "QToolButton { border: none; background: transparent; color: %1; "
                        "font-weight: 500; text-align: left; padding: 2px 0; }")
                        .arg(textMain));
            }
            if (child.browser) {
                child.browser->setStyleSheet(
                    QStringLiteral("QTextBrowser { border: 1px solid %1; border-radius: 8px; "
                                   "background-color: %2; color: %3; padding: 6px; }"
                                   "QTextBrowser a { color: %4; }")
                        .arg(border, pageBg, textMain, link));
                if (child.browser->document()) {
                    child.browser->document()->setDefaultStyleSheet(
                        QStringLiteral("body { color: %1; background-color: %2; }"
                                       "h4 { color: %1; margin: 8px 0 4px 0; }"
                                       "p, li { color: %1; }"
                                       "code { color: %1; background-color: transparent; "
                                       "font-family: Menlo, Consolas, 'Courier New', monospace; }"
                                       "ul, ol { margin-top: 4px; margin-bottom: 4px; }"
                                       "a { color: %3; }")
                            .arg(textMain, pageBg, link));
                }
            }
            for (ThirdSectionUi& third : child.children) {
                if (third.card) {
                    third.card->setStyleSheet(
                        QStringLiteral(
                            "QFrame { border: 1px solid %1; border-radius: 8px; background: %2; }")
                            .arg(border, pageBg));
                }
                if (third.toggle) {
                    third.toggle->setStyleSheet(
                        QStringLiteral(
                            "QToolButton { border: none; background: transparent; color: %1; "
                            "font-weight: 500; text-align: left; padding: 2px 0; }")
                            .arg(textMain));
                }
                if (third.browser) {
                    third.browser->setStyleSheet(
                        QStringLiteral("QTextBrowser { border: 1px solid %1; border-radius: 8px; "
                                       "background-color: %2; color: %3; padding: 6px; }"
                                       "QTextBrowser a { color: %4; }")
                            .arg(border, editorBg, textMain, link));
                    if (third.browser->document()) {
                        third.browser->document()->setDefaultStyleSheet(
                            QStringLiteral(
                                "body { color: %1; background-color: %2; }"
                                "h4 { color: %1; margin: 8px 0 4px 0; }"
                                "p, li { color: %1; }"
                                "code { color: %1; background-color: transparent; "
                                "font-family: Menlo, Consolas, 'Courier New', monospace; }"
                                "ul, ol { margin-top: 4px; margin-bottom: 4px; }"
                                "a { color: %3; }")
                                .arg(textMain, editorBg, link));
                    }
                }
            }
        }
    }
}
