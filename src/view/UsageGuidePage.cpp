#include "UsageGuidePage.h"
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
    return {{QStringLiteral("topology"),
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
             QStringLiteral("<h4>参数定义</h4>"
                            "<p><code>routing_function</code> 用于指定路由算法，如 "
                            "<code>dor</code>、<code>dim_order</code>、<code>min</code>。</p>"
                            "<h4>调参建议</h4>"
                            "<ul>"
                            "<li>优先使用该拓扑已验证可用的算法组合。</li>"
                            "<li>关注是否仅最短路，或允许受控非最短路分流。</li>"
                            "<li>若出现吞吐异常低、延迟曲线突变，优先排查拓扑-路由兼容性。</li>"
                            "</ul>"),
             QStringLiteral("核心参数 routing_function 路由算法 dor dim_order min nca ran_min "
                            "dest_tag 死锁规避 最短路")},
            {QStringLiteral("k"),
             QStringLiteral("规则拓扑中的每维规模参数，影响节点总量与并行链路数。"),
             QStringLiteral(
                 "<h4>参数定义</h4>"
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
             QStringLiteral(
                 "<h4>参数定义</h4>"
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
        {QStringLiteral("端到端包延迟（Packet Latency）"),
         QStringLiteral("从注入到接收完成的总时延，是最常用的体验型指标。"),
         QStringLiteral("<h4>定义</h4>"
                        "<p>端到端包延迟表示一个包从源端开始注入，到目的端完整接收的总耗时，"
                        "单位通常为 <code>cycle</code>。</p>"
                        "<h4>分解公式</h4>"
                        "<p><code>L<sub>packet</sub> = L<sub>inj_queue</sub> + "
                        "L<sub>network</sub> + L<sub>eject</sub></code></p>"
                        "<h4>统计公式</h4>"
                        "<p><code>L<sub>packet</sub> = (&Sigma;<sub>i=1</sub><sup>N</sup>"
                        "(t<sub>recv,i</sub> - t<sub>inject,i</sub>)) / N</code></p>"
                        "<h4>判读建议</h4>"
                        "<ul>"
                        "<li>先建立低负载基线，再观察倍数增长趋势。</li>"
                        "<li>若延迟突增，需结合吞吐匹配度和网内延迟联动诊断。</li>"
                        "</ul>"),
         QStringLiteral("指标术语 端到端包延迟 packet latency L_packet L_inj_queue L_network "
                        "L_eject t_recv t_inject cycle 基线 拥塞")},
        {QStringLiteral("吞吐匹配度（eta_match）"),
         QStringLiteral("衡量网络接收能力对注入需求的跟随程度。"),
         QStringLiteral("<h4>定义</h4>"
                        "<p>吞吐匹配度反映“业务注入需求”与“网络接收能力”的一致性。</p>"
                        "<h4>常用公式</h4>"
                        "<p><code>&eta;<sub>match</sub> = accepted_rate / offered_rate &times; 100%"
                        "</code></p>"
                        "<h4>判读建议</h4>"
                        "<ul>"
                        "<li>接近 100% 通常表示系统仍在可承载区间。</li>"
                        "<li>持续下滑且伴随延迟斜率抬升，通常表示接近饱和。</li>"
                        "</ul>"),
         QStringLiteral("指标术语 吞吐匹配度 eta_match accepted_rate offered_rate saturation")},
        {QStringLiteral("网内延迟（Network Latency）"),
         QStringLiteral("只统计包进入网络后的时延，用于区分注入侧与网络侧瓶颈。"),
         QStringLiteral(
             "<h4>定义</h4>"
             "<p>网内延迟不含注入前排队，更聚焦拓扑、路由和竞争本身。</p>"
             "<h4>统计公式</h4>"
             "<p><code>L<sub>network</sub> = (&Sigma;<sub>i=1</sub><sup>N</sup>"
             "(t<sub>exit,i</sub> - t<sub>enter,i</sub>)) / N</code></p>"
             "<h4>工程近似</h4>"
             "<p><code>L<sub>network</sub> &asymp; H<sub>avg</sub> &times; t<sub>hop</sub> + "
             "L<sub>contention</sub></code></p>"
             "<h4>判读建议</h4>"
             "<p>若端到端延迟上升但网内延迟平稳，瓶颈更可能在注入侧排队。</p>"),
         QStringLiteral("指标术语 网内延迟 network latency L_network t_enter t_exit contention")},
        {QStringLiteral("平均跳数（H_avg）"),
         QStringLiteral("衡量路径长度效率，可用于判断是否出现绕行。"),
         QStringLiteral(
             "<h4>定义</h4>"
             "<p>平均跳数是包在路由器间经历的平均转发步数。</p>"
             "<h4>公式</h4>"
             "<p><code>H<sub>avg</sub> = (&Sigma;<sub>i=1</sub><sup>N</sup>h<sub>i</sub>) / N"
             "</code></p>"
             "<h4>对照指标</h4>"
             "<p><code>&rho;<sub>hop</sub> = H<sub>avg</sub> / H<sub>lb</sub></code>，"
             "越接近 1 越接近最短路。</p>"
             "<h4>判读建议</h4>"
             "<p>若 <code>H<sub>avg</sub></code> 稳定但延迟恶化，通常是竞争拥塞而非路径变长。</p>"),
         QStringLiteral("指标术语 平均跳数 H_avg H_lb rho_hop hop 路径长度")},
        {QStringLiteral("Router"),
         QStringLiteral("执行路由计算、VC 分配与交换仲裁的核心转发节点。"),
         QStringLiteral("<h4>职责</h4>"
                        "<ul>"
                        "<li>根据路由算法选择下一跳输出端口。</li>"
                        "<li>执行 VC 分配和交换仲裁。</li>"
                        "<li>在拥塞时产生排队并向上游传播反压。</li>"
                        "</ul>"
                        "<h4>观测建议</h4>"
                        "<p>Router 负载分布不均通常会直接反映为局部热区与尾延迟抬升。</p>"),
         QStringLiteral("指标术语 router 路由器 仲裁 vc 分配 转发节点")},
        {QStringLiteral("Terminal / Node"),
         QStringLiteral("业务流量注入与接收的端点。"),
         QStringLiteral(
             "<h4>定义</h4>"
             "<p>Terminal（或 Node）是产生和接收数据包的终端实体，通常挂接在路由器边缘。</p>"
             "<h4>影响</h4>"
             "<ul>"
             "<li>注入速率和模式直接影响全网拥塞演化。</li>"
             "<li>端点分布与映射方式会改变局部热度。</li>"
             "</ul>"),
         QStringLiteral("指标术语 terminal node 终端 端点 注入 接收")},
        {QStringLiteral("Hop"),
         QStringLiteral("包从一个路由器转发到下一个路由器的一次跨越。"),
         QStringLiteral(
             "<h4>定义</h4>"
             "<p>Hop 是路径长度的离散计量单位，常用于估计传播与仲裁累积成本。</p>"
             "<h4>实践建议</h4>"
             "<p>结合 <code>H<sub>avg</sub></code> 与延迟变化可区分“路径变长”与“竞争加剧”。</p>"),
         QStringLiteral("指标术语 hop 跳数 转发步数 路径")},
        {QStringLiteral("Flit"),
         QStringLiteral("流控最小单位，一个 packet 通常由多个 flit 组成。"),
         QStringLiteral("<h4>定义</h4>"
                        "<p>Flit（flow control digit）是链路传输和缓冲管理的基本单位。</p>"
                        "<h4>影响</h4>"
                        "<ul>"
                        "<li>flit 级仲裁会影响包级延迟分布。</li>"
                        "<li>大包由多个 flit 构成时，尾部 flit 更易受拥塞影响。</li>"
                        "</ul>"),
         QStringLiteral("指标术语 flit packet 流控单位 仲裁")},
        {QStringLiteral("VC（Virtual Channel）"),
         QStringLiteral("共享物理链路的逻辑通道，用于降低队首阻塞风险。"),
         QStringLiteral(
             "<h4>定义</h4>"
             "<p>VC 允许多条逻辑流共享同一物理端口缓冲与带宽。</p>"
             "<h4>作用</h4>"
             "<ul>"
             "<li>降低 HOL 队首阻塞概率。</li>"
             "<li>为死锁规避和服务隔离提供资源维度。</li>"
             "</ul>"
             "<h4>调参关联</h4>"
             "<p>通常与 <code>num_vcs</code> 和 <code>vc_buf_size</code> 联动分析。</p>"),
         QStringLiteral("指标术语 vc virtual channel num_vcs vc_buf_size hol")},
        {QStringLiteral("HOL Blocking"),
         QStringLiteral("队首阻塞现象：前序流受阻导致后序流无法前进。"),
         QStringLiteral(
             "<h4>定义</h4>"
             "<p>当队首包因下游资源不可用而停滞时，后续包即使目标端口可用也无法越过。</p>"
             "<h4>典型症状</h4>"
             "<ul>"
             "<li>中高负载下吞吐提升变慢。</li>"
             "<li>平均延迟尚可但尾延迟显著恶化。</li>"
             "</ul>"
             "<h4>缓解方向</h4>"
             "<p>增加 VC、优化路由分流、改进仲裁策略。</p>"),
         QStringLiteral("指标术语 hol blocking 队首阻塞 tail latency 拥塞")},
        {QStringLiteral("Backpressure"),
         QStringLiteral("下游拥塞通过流控信号向上游传播并限制注入。"),
         QStringLiteral("<h4>定义</h4>"
                        "<p>Backpressure 是拥塞从下游向上游的反向传播机制，会逐级抬高排队。</p>"
                        "<h4>观测信号</h4>"
                        "<ul>"
                        "<li>注入受限且 accepted_rate 下降。</li>"
                        "<li>热点附近路由器队列持续增厚。</li>"
                        "</ul>"
                        "<h4>处理建议</h4>"
                        "<p>优先检查热点流量模式、路由分配与缓冲配置。</p>"),
         QStringLiteral("指标术语 backpressure 反压 注入受限 accepted_rate 热点")},
        {QStringLiteral("Bisection Bandwidth"),
         QStringLiteral("网络二分带宽上限，常用于估计高负载吞吐天花板。"),
         QStringLiteral("<h4>定义</h4>"
                        "<p>将网络切分为两个等规模子集时，跨切分面的总带宽称为二分带宽。</p>"
                        "<h4>意义</h4>"
                        "<ul>"
                        "<li>用于快速评估拓扑在高负载下的潜在上限。</li>"
                        "<li>可作为跨拓扑比较的结构性参考指标。</li>"
                        "</ul>"
                        "<h4>注意事项</h4>"
                        "<p>二分带宽是上限近似，真实吞吐仍受路由、仲裁和流量模式影响。</p>"),
         QStringLiteral("指标术语 bisection bandwidth 二分带宽 吞吐上限 拓扑比较")}};
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

    addSection(tr("快捷键"),
               tr("操作"),
               tr("Canvas 页面支持键盘放置、平移与缩放。"),
               tr("<h4>放置与模式切换</h4>"
                  "<ul>"
                  "<li><b>N</b>：进入“终端点击放置”模式。</li>"
                  "<li><b>R</b>：进入“路由器点击放置”模式。</li>"
                  "<li><b>Esc</b>：退出点击放置模式，回到普通编辑状态。</li>"
                  "</ul>"
                  "<h4>撤销与重做</h4>"
                  "<ul>"
                  "<li><b>Windows / Linux</b>：<b>Ctrl + Z</b> 撤销，<b>Ctrl + Y</b>（或 "
                  "<b>Ctrl + Shift + Z</b>）重做。</li>"
                  "<li><b>macOS</b>：<b>Command + Z</b> 撤销，<b>Command + Shift + Z</b> "
                  "重做。</li>"
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
               QStringLiteral(
                   "快捷键 键盘 canvas n r esc 上下左右 平移 视口 缩放 ctrl command cmd plus "
                   "minus zoom reset undo redo ctrl+z ctrl+y ctrl+shift+z cmd+z cmd+shift+z"));

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
        tr("指标术语"),
        tr("解释"),
        tr("核心指标和术语已拆分为二级卡片，支持按术语名搜索直达。"),
        tr("<h4>阅读方式</h4>"
           "<ol>"
           "<li>先看 KPI：端到端包延迟、吞吐匹配度、网内延迟、平均跳数。</li>"
           "<li>再看结构术语：Router、Terminal、Hop、Flit、VC。</li>"
           "<li>最后看拥塞术语：HOL Blocking、Backpressure、Bisection Bandwidth。</li>"
           "</ol>"),
        QStringLiteral(
            "指标 术语 packet latency network latency throughput accepted rate saturation "
            "router terminal hop flit vc virtual channel hol blocking tail latency p99 "
            "offered load accepted load jitter backpressure bisection bandwidth 收敛 拐点 "
            "核心 kpi 端到端包延迟 吞吐匹配度 网内延迟 平均跳数 注入排队 饱和判据 "
            "accepted_rate offered_rate eta_match h_avg h_lb rho_hop"));

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

    auto* browser = new QTextBrowser(body);
    browser->setOpenExternalLinks(true);
    browser->setReadOnly(true);
    browser->setHtml(richText);
    enableAutoHeight(browser, 48);
    bodyLay->addWidget(browser);

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
    browser->setHtml(QStringLiteral("<p>%1</p>%2").arg(summary.toHtmlEscaped(), richText));
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
