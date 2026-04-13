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

struct CoreParamCard {
    QString key;
    QString summary;
    QString details;
    QString searchable;
};

QVector<CoreParamCard> coreParameterCards() {
    return {
        {QStringLiteral("topology"),
         QStringLiteral("决定网络结构与路径集合，是所有参数的前置约束。"),
         QStringLiteral(
             "<h4>参数定义</h4>"
             "<p><code>topology</code> 用于指定网络拓扑类型，会直接决定节点连接关系、"
             "最短路径分布与可选路由算法集合。</p>"
             "<h4>常见取值</h4>"
             "<ul>"
             "<li><code>mesh</code> / <code>torus</code>：规则二维/多维网络。</li>"
             "<li><code>cmesh</code> / <code>flatfly</code> / <code>dragonflynew</code>："
             "面向高带宽或低直径设计。</li>"
             "<li><code>fattree</code> / <code>anynet</code>：分层或自定义结构。</li>"
             "</ul>"
             "<h4>调参建议</h4>"
             "<p>先确定 <code>topology</code>，再设置 <code>routing_function</code> "
             "与结构参数（如 <code>k</code>、<code>n</code>、<code>c</code>）。</p>"),
         QStringLiteral("核心参数 topology 拓扑 网络结构 mesh torus cmesh flatfly dragonflynew "
                        "fattree anynet")},
        {QStringLiteral("routing_function"),
         QStringLiteral("路由算法必须与拓扑匹配，直接影响拥塞与尾延迟。"),
         QStringLiteral(
             "<h4>参数定义</h4>"
             "<p><code>routing_function</code> 为 BookSim 中的路由基名；运行时会自动拼上 "
             "<code>_&lt;topology&gt;</code> 查找实现（无需手写后缀）。</p>"
             "<h4>mesh（与内置 BookSim2 一致）</h4>"
             "<p>维序：<code>dor</code>、<code>dim_order</code>、<code>dim_order_ni</code>、"
             "<code>dim_order_pni</code>；XY/YX：<code>xy_yx</code>、<code>adaptive_xy_yx</code>；"
             "其它：<code>romm</code>、<code>romm_ni</code>、<code>min_adapt</code>、"
             "<code>planar_adapt</code>、<code>valiant</code>、<code>chaos</code>。"
             "<code>limited_adapt</code> 在上游未注册。</p>"
             "<h4>torus</h4>"
             "<p><code>dim_order</code>、<code>dim_order_ni</code>、<code>dim_order_bal</code>、"
             "<code>min_adapt</code>、<code>valiant</code>、<code>valiant_ni</code>、"
             "<code>chaos</code>；不存在 <code>dor_torus</code>。</p>"
             "<h4>调参建议</h4>"
             "<ul>"
             "<li>优先使用该拓扑已验证可用的算法组合。</li>"
             "<li>关注是否仅最短路，或允许受控非最短路分流。</li>"
             "<li>若出现吞吐异常低、延迟曲线突变，优先排查拓扑-路由兼容性。</li>"
             "</ul>"),
         QStringLiteral("核心参数 routing_function 路由算法 dor dim_order min nca ran_min "
                        "dest_tag mesh torus 死锁规避 最短路")},
        {QStringLiteral("k"),
         QStringLiteral("规则拓扑中的每维规模参数，影响节点总量与并行链路数。"),
         QStringLiteral("<h4>参数定义</h4>"
                        "<p><code>k</code> 表示每维路由器规模。通常 <code>k</code> 越大，"
                        "并行路径潜力越高，但链路与状态开销也会同步增加。</p>"
                        "<h4>关联关系</h4>"
                        "<p>在 mesh/torus 等规则拓扑中，常与 <code>n</code> 一起决定规模。</p>"),
         QStringLiteral("核心参数 k 每维规模 router scale 规则拓扑 mesh torus")},
        {QStringLiteral("n"),
         QStringLiteral("维度参数，决定路径长度分布和规模增长速度。"),
         QStringLiteral("<h4>参数定义</h4>"
                        "<p><code>n</code> 表示网络维度。以规则拓扑为例，规模通常随 "
                        "<code>k^n</code> 快速增长。</p>"
                        "<h4>调参建议</h4>"
                        "<p>增大 <code>n</code> 可能改善并行度，但也会提升配置复杂度，"
                        "建议配合注入率扫描观察拐点变化。</p>"),
         QStringLiteral("核心参数 n 维度 k^n 跳数 路径长度 规模")},
        {QStringLiteral("c"),
         QStringLiteral("每路由器终端挂接数，决定注入端口密度与局部竞争程度。"),
         QStringLiteral("<h4>参数定义</h4>"
                        "<p><code>c</code> 为每个路由器挂接的终端数量。</p>"
                        "<h4>调参建议</h4>"
                        "<ul>"
                        "<li><code>c</code> 增大可提升注入能力上限。</li>"
                        "<li>同时会抬高局部仲裁竞争，需结合 <code>num_vcs</code> "
                        "和缓冲参数评估。</li>"
                        "</ul>"),
         QStringLiteral("核心参数 c concentration terminal 终端挂接 注入密度")},
        {QStringLiteral("traffic"),
         QStringLiteral("流量模式决定源宿分布，是压测结论可比性的关键条件。"),
         QStringLiteral("<h4>参数定义</h4>"
                        "<p><code>traffic</code> 指定业务模式，如 <code>uniform</code>、"
                        "<code>transpose</code> 等。</p>"
                        "<h4>调参建议</h4>"
                        "<p>不同模式热点性质差异明显，跨实验比较时应保持 "
                        "<code>traffic</code> 一致。</p>"),
         QStringLiteral("核心参数 traffic uniform transpose 流量模式 热点 源宿分布")},
        {QStringLiteral("injection_rate"),
         QStringLiteral("最关键自变量，用于构建延迟-负载/吞吐-负载曲线。"),
         QStringLiteral("<h4>参数定义</h4>"
                        "<p><code>injection_rate</code> 为归一化注入率，通常作为横轴扫描。</p>"
                        "<h4>扫描建议</h4>"
                        "<ul>"
                        "<li>低负载区可用较大步长（如 0.01）。</li>"
                        "<li>接近拐点时缩小步长（如 0.002~0.005）。</li>"
                        "<li>建议记录饱和前后关键点，便于跨拓扑对比。</li>"
                        "</ul>"),
         QStringLiteral("核心参数 injection_rate 注入率 offered load 负载扫描 拐点 饱和")},
        {QStringLiteral("num_vcs"),
         QStringLiteral("每端口 VC 数，主要影响 HOL 缓解能力与资源开销。"),
         QStringLiteral(
             "<h4>参数定义</h4>"
             "<p><code>num_vcs</code> 表示每端口虚通道数量。</p>"
             "<h4>调参建议</h4>"
             "<p>提高 <code>num_vcs</code> 往往可缓解 HOL 队首阻塞，但会增加仲裁和状态成本。"
             "建议与 <code>vc_buf_size</code> 联动评估。</p>"),
         QStringLiteral("核心参数 num_vcs vc virtual channel hol blocking 队首阻塞")},
        {QStringLiteral("vc_buf_size"),
         QStringLiteral("每 VC 缓冲深度，影响反压出现时机与突发吸收能力。"),
         QStringLiteral("<h4>参数定义</h4>"
                        "<p><code>vc_buf_size</code> 表示每个 VC 的缓冲深度。</p>"
                        "<h4>调参建议</h4>"
                        "<ul>"
                        "<li>过小会导致反压提前传播，吞吐受限。</li>"
                        "<li>过大虽可提升容忍度，但资源成本显著增加。</li>"
                        "<li>建议与 <code>num_vcs</code> 一起做二维扫描。</li>"
                        "</ul>"),
         QStringLiteral("核心参数 vc_buf_size 缓冲深度 backpressure 反压 buffer")},
        {QStringLiteral("warmup_periods"),
         QStringLiteral("预热阶段长度，用于消除初始瞬态统计偏差。"),
         QStringLiteral(
             "<h4>参数定义</h4>"
             "<p><code>warmup_periods</code> 指预热样本长度，通常不计入最终统计。</p>"
             "<h4>调参建议</h4>"
             "<p>当低负载波动大或系统初始状态敏感时，适当增加预热长度可提高稳定性。</p>"),
         QStringLiteral("核心参数 warmup_periods 预热 瞬态 统计稳定性")},
        {QStringLiteral("sample_period / max_samples"),
         QStringLiteral("控制采样窗口与样本总量，决定结果方差与置信度。"),
         QStringLiteral("<h4>参数定义</h4>"
                        "<p><code>sample_period</code> 控制单次采样窗口，"
                        "<code>max_samples</code> 控制累计样本数量。</p>"
                        "<h4>调参建议</h4>"
                        "<p>采样不足会让结论不稳定。若延迟曲线抖动明显，优先增加采样长度。</p>"),
         QStringLiteral("核心参数 sample_period max_samples 采样窗口 方差 置信区间")},
        {QStringLiteral("sim_type"),
         QStringLiteral("仿真模式开关，决定统计口径与输出解释上下文。"),
         QStringLiteral("<h4>参数定义</h4>"
                        "<p><code>sim_type</code> 用于指定仿真模式（如延迟导向等）。</p>"
                        "<h4>调参建议</h4>"
                        "<p>做参数对比时保持 <code>sim_type</code> 一致，"
                        "避免统计口径差异影响结论。</p>"),
         QStringLiteral("核心参数 sim_type 仿真模式 latency 吞吐 统计口径")}};
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
        tr("配置文件路径"),
        tr("存储"),
        tr("应用设置和仿真记录保存到用户配置目录，不在工程仓库内。"),
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
           "</ul>"),
        QStringLiteral(
            "配置 路径 settings.ini simulation_records.json 存储 目录 windows mac linux appdata "
            "book-canvas njupt"));

    SectionUi* paramSection = addSection(
        tr("参数"),
        tr("建模"),
        tr("参数分为结构、路由、流量、缓冲与统计；核心参数已作为二级卡片嵌套在本卡片内。"),
        tr("<h4>参数使用顺序</h4>"
           "<ol>"
           "<li>先看 <code>topology</code> 与 "
           "<code>routing_function</code>，确认结构与路由兼容。</li>"
           "<li>再看结构规模参数 <code>k</code>、<code>n</code>、<code>c</code>。</li>"
           "<li>随后设置负载与流控参数：<code>traffic</code>、<code>injection_rate</code>、"
           "<code>num_vcs</code>、<code>vc_buf_size</code>。</li>"
           "<li>最后检查统计参数：<code>warmup_periods</code>、<code>sample_period</code>、"
           "<code>max_samples</code>、<code>sim_type</code>。</li>"
           "</ol>"),
        QStringLiteral(
            "参数 核心参数 二级卡片 嵌套 card nested topology routing_function k n c traffic "
            "injection_rate num_vcs vc_buf_size warmup_periods sample_period max_samples "
            "sim_type"));

    if (paramSection) {
        for (const CoreParamCard& card : coreParameterCards()) {
            addSubSection(*paramSection,
                          tr("%1").arg(card.key),
                          card.summary,
                          card.details,
                          card.searchable);
        }
    }

    SectionUi* metricSection = addSection(
        tr("结果参数含义"),
        tr("解释"),
        tr("与仿真结果页指标一一对应；每项含定义、公式（HTML）与提示。支持搜索。"),
        QString(),
        QStringLiteral(
            "结果参数含义 指标 术语 packet latency network latency throughput accepted rate "
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

void UsageGuidePage::addSubSection(SectionUi& parent,
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
}

void UsageGuidePage::applySearchFilter() {
    const QString key = (m_searchEdit ? m_searchEdit->text().trimmed().toLower() : QString());
    int matchedSections = 0;
    int matchedSubCards = 0;
    for (SectionUi& s : m_sections) {
        const bool sectionHit = key.isEmpty() || s.searchableText.contains(key);
        bool anyChildHit = false;

        for (SubSectionUi& child : s.children) {
            const bool childHit = key.isEmpty() || child.searchableText.contains(key);
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
        if (matchedSubCards > 0) {
            m_statusLabel->setText(tr("搜索 \"%1\"：匹配到 %2 个分组，%3 个二级卡片。")
                                       .arg(key)
                                       .arg(matchedSections)
                                       .arg(matchedSubCards));
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
        }
    }
}
