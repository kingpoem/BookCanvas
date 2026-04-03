#include "UsageGuidePage.h"
#include <ElaLineEdit.h>
#include <ElaPushButton.h>
#include <ElaText.h>
#include <ElaTheme.h>
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
                   "minus zoom reset"));

    addSection(
        tr("参数"),
        tr("建模"),
        tr("参数分为结构、路由、流量、缓冲与统计五类；建议先验证结构合法性，再做负载扫描。"),
        tr("<h4>一、结构参数</h4>"
           "<ul>"
           "<li><b>topology</b>：网络拓扑类型，决定节点连接图与最短路径集合。"
           "常见值包括 mesh、torus、cmesh、flatfly、dragonflynew、fattree、anynet。</li>"
           "<li><b>k</b>：每维路由器规模。"
           "在规则拓扑中，k 的增加会提升路径并行性，但同步增加链路与状态空间。</li>"
           "<li><b>n</b>：维度数。以 mesh/torus 为例，路由器总数约为 k^n；"
           "n 增大通常会提高平均跳数分布宽度与路由决策复杂度。</li>"
           "<li><b>c</b>：每个路由器挂接终端数。"
           "c 增大可提高注入端口密度，但会抬高路由器局部竞争压力。</li>"
           "<li><b>network_file</b>：anynet 模式下的外部拓扑定义文件。"
           "其图结构与端口映射应与 routing_function 协同验证。</li>"
           "</ul>"
           "<h4>二、路由参数</h4>"
           "<ul>"
           "<li><b>routing_function</b>：路由算法标识，必须与 topology 兼容。"
           "建议填写算法名本体（如 dor、dim_order、min），避免手写拓扑后缀。</li>"
           "<li><b>最短路约束</b>：确认算法是否只走最短路径，或允许受控非最短路径。"
           "该差异会影响拥塞分散能力与尾延迟。</li>"
           "<li><b>死锁规避</b>：对自适应或多路径算法，需结合 VC 类别划分与转弯限制共同验证。</li>"
           "<li><b>拓扑匹配异常信号</"
           "b>：常见表现为启动报错、收敛失败、吞吐异常低或延迟曲线不连续。</li>"
           "</ul>"
           "<h4>三、流量参数</h4>"
           "<ul>"
           "<li><b>traffic</b>：流量模式（如 uniform、transpose 等），决定源宿分布与热点性质。</li>"
           "<li><b>injection_rate</b>：归一化注入率。"
           "它是最关键自变量，通常用于绘制延迟-负载曲线与吞吐-负载曲线。</li>"
           "<li><b>步长建议</b>：低负载区可用较大步长（如 0.01），拐点附近应缩小步长（如 "
           "0.002~0.005）。</li>"
           "<li><b>负载区间划分</b>：可按“低负载线性区-过渡区-饱和区”三段记录关键点，"
           "便于跨拓扑对比。</li>"
           "<li><b>随机性控制</b>：固定随机种子或进行多种子重复实验，报告均值与离散度。</li>"
           "</ul>"
           "<h4>四、缓冲与并发参数</h4>"
           "<ul>"
           "<li><b>num_vcs</b>：每端口虚通道数。较高 num_vcs 可缓解 HOL 阻塞，"
           "但增加仲裁与状态开销。</li>"
           "<li><b>vc_buf_size</b>：每 VC 缓冲深度。"
           "缓冲不足会导致上游反压提前出现，缓冲过大则带来资源成本增长。</li>"
           "<li><b>参数耦合</b>：num_vcs 与 vc_buf_size 通常共同决定可吸收突发的能力，"
           "仅提高其中一项可能收益有限。</li>"
           "<li><b>资源视角</b>：在硬件可实现性分析中，应同步记录总缓冲开销"
           "（近似与端口数 × VC 数 × 每 VC 深度成正比）。</li>"
           "</ul>"
           "<h4>五、统计控制参数</h4>"
           "<ul>"
           "<li><b>warmup_periods</b>：预热阶段样本，不计入最终统计；用于消除初始瞬态偏差。</li>"
           "<li><b>sample_period</b> 与 <b>max_samples</b>：控制采样长度与统计稳定性。"
           "采样不足会导致方差偏大、结论不稳定。</li>"
           "<li><b>sim_type</b>：仿真模式。延迟研究建议在收敛条件明确后进行负载扫描。</li>"
           "<li><b>收敛判据</b>：建议监控统计窗口内均值变化率或置信区间宽度，"
           "避免以单次波动作为结束条件。</li>"
           "<li><b>可复现性</b>：记录配置文件版本、提交哈希、随机种子、平台与编译模式。</li>"
           "</ul>"),
        QStringLiteral("参数 topology k n c network_file anynet routing_function dor dim_order min "
                       "traffic injection_rate offered load num_vcs vc_buf_size warmup_periods "
                       "sample_period max_samples sim_type 建模 收敛 扫描 步长 置信区间 可复现"));

    addSection(
        tr("指标术语"),
        tr("解释"),
        tr("读图建议：同时看延迟、吞吐与饱和区间，避免只凭单一指标下结论。"),
        tr("<h4>一、核心指标</h4>"
           "<ul>"
           "<li><b>Packet "
           "Latency</b>：端到端包延迟，通常包含注入排队、网络传输与目的端接收等待。</li>"
           "<li><b>Network Latency</b>：仅统计进入网络后的在网时间，用于分离注入端拥塞影响。</li>"
           "<li><b>Throughput / Accepted Rate</b>：单位时间成功送达负载。"
           "在低负载区通常随 injection_rate 近线性增长。</li>"
           "<li><b>Saturation Point</b>：系统由近线性区转入拥塞区的临界负载。"
           "表现为延迟斜率显著增大、吞吐增长趋缓。</li>"
           "<li><b>Offered Load vs Accepted "
           "Load</b>：输入负载与实际接收负载的差值可用于量化拥塞损失。</li>"
           "<li><b>Tail Latency "
           "(P95/P99)</b>：尾部时延反映最坏体验，对突发流量和不均衡模式更敏感。</li>"
           "<li><b>Latency Jitter</b>：时延抖动用于衡量稳定性，常以方差或分位差表示。</li>"
           "</ul>"
           "<h4>二、结构与流控术语</h4>"
           "<ul>"
           "<li><b>Router</b>：执行路由计算、VC 分配与交换仲裁的核心转发节点。</li>"
           "<li><b>Terminal / Node</b>：流量注入与接收端点。</li>"
           "<li><b>Hop</b>：经过一个路由节点到下一路由节点的转发步数。</li>"
           "<li><b>Flit</b>：流控最小单位；一个 packet 可由多个 flit 组成。</li>"
           "<li><b>VC (Virtual Channel)</b>：共享物理链路上的逻辑通道，用于降低队首阻塞风险。</li>"
           "<li><b>HOL Blocking</b>：队首阻塞，前序流受阻导致后序流无法前进的现象。</li>"
           "<li><b>Backpressure</b>：下游拥塞通过流控反向传播到上游，引发注入受限。</li>"
           "<li><b>Bisection Bandwidth</b>：网络二分带宽上限，常用于估计高负载潜在吞吐天花板。</li>"
           "</ul>"
           "<h4>三、结果判读建议</h4>"
           "<ul>"
           "<li>优先比较同一 topology/routing 下的曲线形状差异，再比较绝对值大小。</li>"
           "<li>若高负载下 latency 波动过大，先检查 warmup 与采样长度是否足够。</li>"
           "<li>报告结论时建议同时给出：实验配置、注入率区间、关键拐点与异常日志。</li>"
           "<li>若比较不同拓扑，建议先按每节点注入率归一化，再比较跨规模结论。</li>"
           "<li>出现“吞吐提升但尾延迟恶化”时，应补充分位数曲线，避免平均值掩盖风险。</li>"
           "<li>建议同时展示 latency-throughput 双轴图和关键配置表，以提升可审阅性。</li>"
           "</ul>"),
        QStringLiteral(
            "指标 术语 packet latency network latency throughput accepted rate saturation "
            "router terminal hop flit vc virtual channel hol blocking tail latency p99 "
            "offered load accepted load jitter backpressure bisection bandwidth 收敛 拐点"));

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

void UsageGuidePage::addSection(const QString& title,
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
    browser->setMinimumHeight(260);
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
}

void UsageGuidePage::applySearchFilter() {
    const QString key = (m_searchEdit ? m_searchEdit->text().trimmed().toLower() : QString());
    int matched = 0;
    for (SectionUi& s : m_sections) {
        const bool hit = key.isEmpty() || s.searchableText.contains(key);
        if (s.card) {
            s.card->setVisible(hit);
        }
        if (hit) {
            ++matched;
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
        m_statusLabel->setText(tr("搜索 \"%1\"：匹配到 %2 个分组。").arg(key).arg(matched));
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
                                   "ul, ol { margin-top: 4px; margin-bottom: 4px; }"
                                   "a { color: %3; }")
                        .arg(textMain, editorBg, link));
            }
        }
    }
}
