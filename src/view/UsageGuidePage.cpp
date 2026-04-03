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

    auto* heading = new ElaText(tr("BookSim 使用说明（系统化指南）"), this);
    heading->setWordWrap(false);
    heading->setTextPixelSize(22);
    centerLayout->addWidget(heading);

    m_searchEdit = new ElaLineEdit(this);
    m_searchEdit->setPlaceholderText(
        tr("搜索关键词：例如 流程 / routing_function / 饱和 / 收敛 ..."));
    centerLayout->addWidget(m_searchEdit);

    m_statusLabel
        = new QLabel(tr("按“流程-参数-指标-诊断”组织内容，支持全文搜索、分组折叠和模板复制。"),
                     this);
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
        tr("A1. 标准实验流程（建议按序执行）"),
        tr("流程"),
        tr("以可复现实验为目标：先固定变量，再做单因素扫描。"),
        tr("<ol>"
           "<li><b>构建拓扑</b>：在 Canvas 放置单一拓扑块（推荐先 mesh/torus 小规模）。</li>"
           "<li><b>设置全局配置</b>：在「全局配置」页确认 topology、routing_function、traffic、"
           "injection_rate。</li>"
           "<li><b>导出文件</b>：先导出拓扑（anynet_file），再导出 JSON。</li>"
           "<li><b>执行仿真</b>：在 Simulation 页面运行并检查日志告警。</li>"
           "<li><b>读取结果</b>：关注 latency、throughput 与 saturation 拐点。</li>"
           "</ol>"
           "<p>推荐方法：一次仅修改 1~2 个参数，记录版本、输入与输出，保证结果可追溯。</p>"),
        QStringLiteral("流程 实验 可复现 导出 仿真 结果 分析 扫描 单因素"),
        {});

    addSection(
        tr("B1. 快速可运行模板（推荐起步）"),
        tr("参数模板"),
        tr("先跑通，再调优：模板用于降低“参数不匹配”与“规模过大”风险。"),
        tr("<ul>"
           "<li><b>mesh</b>：k=4, n=2, c=1, routing_function=dor</li>"
           "<li><b>torus</b>：k=4, n=2, c=1, routing_function=dim_order</li>"
           "<li><b>cmesh</b>：k=4, n=2, c=4, routing_function=dor_no_express</li>"
           "<li><b>fly</b>：k=4, n=2, routing_function=dest_tag</li>"
           "<li><b>qtree</b>：k=4, n=3, routing_function=nca</li>"
           "<li><b>tree4</b>：k=4, n=3, routing_function=nca</li>"
           "<li><b>fattree</b>：k=4, n=2, routing_function=nca</li>"
           "<li><b>flatfly</b>：k=4, n=2, c=1, routing_function=ran_min</li>"
           "<li><b>dragonflynew</b>：k=2, n=1, routing_function=min</li>"
           "<li><b>anynet</b>：由 network_file 决定拓扑，routing_function=min</li>"
           "</ul>"
           "<p>建议：先把 injection_rate 控制在 0.02~0.05，确认链路与路由函数匹配后再提负载。</p>"),
        QStringLiteral("快速 起步 模板 mesh torus cmesh fly qtree tree4 fattree flatfly "
                       "dragonflynew anynet k n c routing_function injection_rate"),
        {{tr("复制 mesh 模板"), meshTemplate()},
         {tr("复制 torus 模板"), torusTemplate()},
         {tr("复制 cmesh 模板"), cmeshTemplate()},
         {tr("复制 fly 模板"), flyTemplate()},
         {tr("复制 qtree 模板"), qtreeTemplate()},
         {tr("复制 tree4 模板"), tree4Template()},
         {tr("复制 fattree 模板"), fattreeTemplate()},
         {tr("复制 flatfly 模板"), flatflyTemplate()},
         {tr("复制 dragonfly 模板"), dragonflyTemplate()},
         {tr("复制 anynet 模板"), anynetTemplate()}});

    addSection(tr("B2. 参数体系与建模含义"),
               tr("参数体系"),
               tr("参数可分为结构层、路由层、流量层、统计层。优先保证结构与路由合法。"),
               tr("<h4>topology</h4>"
                  "<p>"
                  "决定网络连接结构。常见值：mesh、torus、cmesh、dragonflynew、flatfly、fattree、an"
                  "ynet。</p>"
                  "<h4>k / n / c</h4>"
                  "<ul>"
                  "<li><b>k</b>：每维路由器数量</li>"
                  "<li><b>n</b>：维度数（mesh/torus 路由器总数约为 k^n）</li>"
                  "<li><b>c</b>：每个路由器挂接终端数（concentration）</li>"
                  "</ul>"
                  "<h4>routing_function</h4>"
                  "<p>路由算法名（不需要填写后缀，例如 mesh 下填 dor，而不是 dor_mesh）。</p>"
                  "<ul>"
                  "<li>mesh 常用：dor / dim_order / xy_yx</li>"
                  "<li>cmesh 常用：dor_no_express / xy_yx_no_express</li>"
                  "<li>anynet：min</li>"
                  "</ul>"
                  "<h4>traffic / injection_rate</h4>"
                  "<p>决定输入负载模型与强度。注入率过高会触发拥塞，延迟急剧上升。</p>"
                  "<h4>warmup / sample / max_samples</h4>"
                  "<p>控制统计收敛质量。样本过少会导致结果波动大、可比性差。</p>"),
               QStringLiteral("topology mesh torus cmesh anynet k n c concentration "
                              "routing_function dor dim_order xy_yx dor_no_express "
                              "network_file dragonfly flatfly fattree traffic warmup sample"));

    addSection(tr("C1. 指标与术语（结果解释）"),
               tr("指标解释"),
               tr("建议同时观察延迟和吞吐，识别系统从线性区进入饱和区的拐点。"),
               tr("<ul>"
                  "<li><b>Router</b>：路由器节点，负责转发 flit。</li>"
                  "<li><b>Terminal/Node</b>：注入与接收流量的端点。</li>"
                  "<li><b>Hop</b>：经过一个路由器到下一个路由器的跳数。</li>"
                  "<li><b>Flit</b>：流控粒度（flow control unit），一个包由多个 flit 组成。</li>"
                  "<li><b>VC（Virtual Channel）</b>：虚通道，用于缓解 HOL 阻塞。</li>"
                  "<li><b>Injection Rate</b>：注入率，每周期注入网络的负载强度。</li>"
                  "<li><b>Packet Latency</b>：端到端包延迟，含注入等待与网内延迟。</li>"
                  "<li><b>Network Latency</b>：网内延迟，只统计进入网络后的传输与排队。</li>"
                  "<li><b>Throughput</b>：吞吐，单位时间成功传输的负载。</li>"
                  "<li><b>Saturation</b>：饱和点，超过后延迟急剧上升、吞吐增长趋缓。</li>"
                  "</ul>"),
               QStringLiteral("router terminal node hop flit vc virtual channel injection rate "
                              "packet latency network latency throughput saturation 术语"));

    addSection(
        tr("D1. 诊断与排查清单"),
        tr("故障排查"),
        tr("从合法性、规模、负载、收敛四个维度逐项排查。"),
        tr("<ul>"
           "<li><b>Invalid routing function</b>：检查 routing_function 与 topology 是否匹配，"
           "并避免手写 *_topology 后缀。</li>"
           "<li><b>仿真启动但结果异常</b>：先降低 injection_rate（例如 0.02~0.05），再观察。"
           "</li>"
           "<li><b>大规模卡顿</b>：减小 k/n/c，先验证小规模，再逐级放大。</li>"
           "<li><b>结果波动较大</b>：增加 sample_period / max_samples，确保统计收敛。</li>"
           "<li><b>结果难以比较</b>：固定 seed 与其余参数，仅改变目标变量。</li>"
           "</ul>"),
        QStringLiteral("invalid routing function 报错 排查 injection_rate k n c 卡顿 收敛 seed"));

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
    toggle->setChecked(true);
    toggle->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    toggle->setArrowType(Qt::DownArrow);
    toggle->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    auto* categoryLabel = new QLabel(category, headRow);
    auto* summaryLabel = new QLabel(summary, headRow);
    summaryLabel->setWordWrap(true);

    headLay->addWidget(toggle, 1);
    headLay->addWidget(categoryLabel, 0);
    headLay->addWidget(summaryLabel, 2);
    cardLayout->addWidget(headRow);

    auto* body = new QWidget(card);
    auto* bodyLay = new QVBoxLayout(body);
    bodyLay->setContentsMargins(0, 0, 0, 0);
    bodyLay->setSpacing(8);

    auto* browser = new QTextBrowser(body);
    browser->setOpenExternalLinks(true);
    browser->setReadOnly(true);
    browser->setHtml(richText);
    browser->setMinimumHeight(140);
    bodyLay->addWidget(browser);

    if (!templates.isEmpty()) {
        bodyLay->addWidget(createTemplateRow(templates, body));
    }

    cardLayout->addWidget(body);

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
    sec.categoryLabel = categoryLabel;
    sec.summaryLabel = summaryLabel;
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
        m_statusLabel->setText(
            tr("按“流程-参数-指标-诊断”组织内容，支持全文搜索、分组折叠和模板复制。"));
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
