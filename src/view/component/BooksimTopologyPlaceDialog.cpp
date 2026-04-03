#include "BooksimTopologyPlaceDialog.h"
#include <ElaLineEdit.h>
#include <ElaPushButton.h>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QSet>
#include <QSpinBox>
#include <QVBoxLayout>

namespace {

[[nodiscard]] QString defaultRoutingForTopology(const QString& topologyId) {
    const QString topo = topologyId.trimmed().toLower();
    if (topo == QLatin1String("mesh")) {
        return QStringLiteral("dor");
    }
    if (topo == QLatin1String("torus")) {
        return QStringLiteral("dim_order");
    }
    if (topo == QLatin1String("cmesh")) {
        return QStringLiteral("dor_no_express");
    }
    if (topo == QLatin1String("dragonflynew")) {
        return QStringLiteral("min");
    }
    if (topo == QLatin1String("flatfly")) {
        return QStringLiteral("ran_min");
    }
    if (topo == QLatin1String("fattree")) {
        return QStringLiteral("nca");
    }
    return QStringLiteral("min");
}

[[nodiscard]] bool routingAllowedForTopology(const QString& routingFunction,
                                             const QString& topologyId) {
    const QString rf = routingFunction.trimmed().toLower();
    const QString topo = topologyId.trimmed().toLower();
    if (rf.isEmpty()) {
        return false;
    }
    if (topo == QLatin1String("mesh")) {
        static const QSet<QString> allowed = {QStringLiteral("dor"),
                                              QStringLiteral("dim_order"),
                                              QStringLiteral("dim_order_ni"),
                                              QStringLiteral("dim_order_pni"),
                                              QStringLiteral("xy_yx"),
                                              QStringLiteral("adaptive_xy_yx"),
                                              QStringLiteral("romm"),
                                              QStringLiteral("romm_ni"),
                                              QStringLiteral("min_adapt"),
                                              QStringLiteral("planar_adapt"),
                                              QStringLiteral("valiant"),
                                              QStringLiteral("chaos")};
        return allowed.contains(rf);
    }
    if (topo == QLatin1String("cmesh")) {
        static const QSet<QString> allowed = {QStringLiteral("dor"),
                                              QStringLiteral("dor_no_express"),
                                              QStringLiteral("xy_yx"),
                                              QStringLiteral("xy_yx_no_express")};
        return allowed.contains(rf);
    }
    return true;
}

[[nodiscard]] QString normalizeRoutingFunction(const QString& routingFunction,
                                               const QString& topologyId) {
    const QString topo = topologyId.trimmed().toLower();
    QString rf = routingFunction.trimmed().toLower();
    const QString suffix = QStringLiteral("_") + topo;
    if (rf.endsWith(suffix)) {
        rf.chop(suffix.size());
    }
    if (rf.isEmpty()) {
        rf = defaultRoutingForTopology(topo);
    }
    if (topo == QLatin1String("mesh") && rf == QLatin1String("min")) {
        // BookSim 会拼成 "min_mesh"，该函数不存在；mesh 下统一转到 dor。
        rf = QStringLiteral("dor");
    }
    if (!routingAllowedForTopology(rf, topo)) {
        rf = defaultRoutingForTopology(topo);
    }
    return rf;
}

} // namespace

BooksimTopologyPlaceDialog::BooksimTopologyPlaceDialog(const QString& topologyId,
                                                       const QString& displayLabel,
                                                       QWidget* parent)
    : QDialog(parent)
    , m_topologyId(topologyId)
    , m_displayLabel(displayLabel) {
    buildUi(displayLabel);
    setWindowTitle(tr("BookSim 拓扑 — %1").arg(displayLabel));
    setMinimumWidth(320);
}

void BooksimTopologyPlaceDialog::buildUi(const QString& displayLabel) {
    auto* root = new QVBoxLayout(this);
    const bool autoBuildTopology = (m_topologyId == QLatin1String("mesh")
                                    || m_topologyId == QLatin1String("torus"));
    const QString hintText
        = autoBuildTopology ? tr("确认参数后，在画布空白处单击一次会放置 %1 组件，并自动生成"
                                 "路由器、终端与内部连线。\n你仍可与现有节点继续手动连线。")
                                  .arg(m_topologyId)
                            : tr("确认参数后，在画布空白处单击一次即可放置该拓扑块。\n配置将随「导"
                                 "出配置」写入 JSON（画布上仅一块时生效）。");
    auto* hint = new QLabel(hintText, this);
    hint->setWordWrap(true);
    root->addWidget(hint);

    auto* topoRow = new QLabel(tr("拓扑类型：%1 — <b>%2</b>").arg(m_topologyId, displayLabel), this);
    topoRow->setTextFormat(Qt::RichText);
    root->addWidget(topoRow);

    auto* form = new QFormLayout();
    m_kSpin = new QSpinBox(this);
    m_kSpin->setRange(2, 128);
    m_kSpin->setValue(m_topologyId == QLatin1String("mesh") ? 4 : 8);
    m_kSpin->setToolTip(tr("k：每维路由器数量（基数）"));

    m_nSpin = new QSpinBox(this);
    m_nSpin->setRange(1, 8);
    m_nSpin->setValue(2);
    m_nSpin->setToolTip(tr("n：维数"));

    m_cSpin = new QSpinBox(this);
    m_cSpin->setRange(1, 64);
    m_cSpin->setValue(1);
    m_cSpin->setToolTip(tr("c：每台路由器连接的终端/节点数（集中度）"));

    m_rfEdit = new ElaLineEdit(this);
    m_rfEdit->setText(defaultRoutingForTopology(m_topologyId));
    m_rfEdit->setPlaceholderText(QStringLiteral("dor / dim_order / min / ..."));
    m_rfEdit->setToolTip(tr("routing_function：BookSim 中与路由函数注册名一致"));

    form->addRow(tr("k"), m_kSpin);
    form->addRow(tr("n"), m_nSpin);
    form->addRow(tr("c"), m_cSpin);
    form->addRow(tr("路由函数"), m_rfEdit);
    root->addLayout(form);
    if (m_topologyId == QLatin1String("mesh")) {
        auto* tips = new QLabel(tr("mesh 推荐：dor（默认）/ dim_order / xy_yx。\n"
                                   "无需填写 *_mesh 后缀，系统会自动处理。"),
                                this);
        tips->setWordWrap(true);
        root->addWidget(tips);
    } else if (m_topologyId == QLatin1String("torus")) {
        auto* tips = new QLabel(tr("torus 推荐：dim_order（默认）/ dor。\n"
                                   "无需填写 *_torus 后缀，系统会自动处理。"),
                                this);
        tips->setWordWrap(true);
        root->addWidget(tips);
    }

    auto* box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    root->addWidget(box);
    connect(box, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(box, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

BooksimTopologyParams BooksimTopologyPlaceDialog::getParams() const {
    BooksimTopologyParams p;
    p.topologyId = m_topologyId;
    p.displayLabel = m_displayLabel;
    p.k = m_kSpin ? m_kSpin->value() : 8;
    p.n = m_nSpin ? m_nSpin->value() : 2;
    p.c = m_cSpin ? m_cSpin->value() : 1;
    p.routingFunction = normalizeRoutingFunction(m_rfEdit ? m_rfEdit->text() : QString(),
                                                 m_topologyId);
    return p;
}

void BooksimTopologyPlaceDialog::setParams(const BooksimTopologyParams& p) {
    m_topologyId = p.topologyId;
    m_displayLabel = p.displayLabel;
    if (m_kSpin) {
        m_kSpin->setValue(p.k);
    }
    if (m_nSpin) {
        m_nSpin->setValue(p.n);
    }
    if (m_cSpin) {
        m_cSpin->setValue(p.c);
    }
    if (m_rfEdit) {
        m_rfEdit->setText(normalizeRoutingFunction(p.routingFunction, p.topologyId));
    }
}
