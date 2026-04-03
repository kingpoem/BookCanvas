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
    if (topo == QLatin1String("fly")) {
        return QStringLiteral("dest_tag");
    }
    if (topo == QLatin1String("qtree")) {
        return QStringLiteral("nca");
    }
    if (topo == QLatin1String("tree4")) {
        return QStringLiteral("nca");
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
    if (topo == QLatin1String("fly")) {
        static const QSet<QString> allowed = {QStringLiteral("dest_tag")};
        return allowed.contains(rf);
    }
    if (topo == QLatin1String("qtree")) {
        static const QSet<QString> allowed = {QStringLiteral("nca")};
        return allowed.contains(rf);
    }
    if (topo == QLatin1String("tree4")) {
        static const QSet<QString> allowed = {QStringLiteral("nca"), QStringLiteral("anca")};
        return allowed.contains(rf);
    }
    if (topo == QLatin1String("fattree")) {
        static const QSet<QString> allowed = {QStringLiteral("nca"), QStringLiteral("anca")};
        return allowed.contains(rf);
    }
    if (topo == QLatin1String("flatfly")) {
        static const QSet<QString> allowed = {QStringLiteral("ran_min"),
                                              QStringLiteral("adaptive_xyyx"),
                                              QStringLiteral("xyyx"),
                                              QStringLiteral("valiant"),
                                              QStringLiteral("ugal"),
                                              QStringLiteral("ugal_pni"),
                                              QStringLiteral("ugal_xyyx")};
        return allowed.contains(rf);
    }
    if (topo == QLatin1String("dragonflynew")) {
        static const QSet<QString> allowed = {QStringLiteral("min"), QStringLiteral("ugal")};
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
                                    || m_topologyId == QLatin1String("torus")
                                    || m_topologyId == QLatin1String("cmesh")
                                    || m_topologyId == QLatin1String("fly")
                                    || m_topologyId == QLatin1String("qtree")
                                    || m_topologyId == QLatin1String("tree4")
                                    || m_topologyId == QLatin1String("fattree")
                                    || m_topologyId == QLatin1String("flatfly")
                                    || m_topologyId == QLatin1String("dragonflynew"));
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
    const QString topo = m_topologyId.trimmed().toLower();
    m_kSpin = new QSpinBox(this);
    m_kSpin->setRange(2, 128);
    m_kSpin->setValue((topo == QLatin1String("mesh") || topo == QLatin1String("torus")
                       || topo == QLatin1String("cmesh") || topo == QLatin1String("fly")
                       || topo == QLatin1String("qtree") || topo == QLatin1String("tree4")
                       || topo == QLatin1String("fattree"))
                          ? 4
                          : 8);
    m_kSpin->setToolTip(tr("k：每维路由器数量（基数）"));
    if (topo == QLatin1String("dragonflynew")) {
        m_kSpin->setValue(2);
    }

    m_nSpin = new QSpinBox(this);
    m_nSpin->setRange(1, 8);
    m_nSpin->setValue(2);
    m_nSpin->setToolTip(tr("n：维数"));

    m_cSpin = new QSpinBox(this);
    m_cSpin->setRange(1, 64);
    m_cSpin->setValue(1);
    m_cSpin->setToolTip(tr("c：每台路由器连接的终端/节点数（集中度）"));
    if (topo == QLatin1String("cmesh")) {
        // 当前内置 BookSim2 cmesh 实现仅支持 n=2, c=4。
        m_nSpin->setRange(2, 2);
        m_nSpin->setValue(2);
        m_nSpin->setEnabled(false);
        m_cSpin->setRange(4, 4);
        m_cSpin->setValue(4);
        m_cSpin->setEnabled(false);
    } else if (topo == QLatin1String("qtree") || topo == QLatin1String("tree4")) {
        // 当前内置 BookSim2 qtree/tree4 仅支持 k=4, n=3。
        m_kSpin->setRange(4, 4);
        m_kSpin->setValue(4);
        m_kSpin->setEnabled(false);
        m_nSpin->setRange(3, 3);
        m_nSpin->setValue(3);
        m_nSpin->setEnabled(false);
    } else if (topo == QLatin1String("dragonflynew")) {
        // 当前内置 BookSim2 dragonflynew 仅支持 n=1。
        m_nSpin->setRange(1, 1);
        m_nSpin->setValue(1);
        m_nSpin->setEnabled(false);
    }

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
    } else if (m_topologyId == QLatin1String("cmesh")) {
        auto* tips = new QLabel(tr("cmesh 推荐：dor_no_express（默认）/ xy_yx_no_express。\n"
                                   "当前 BookSim2 内置 cmesh 仅支持 n=2、c=4。"),
                                this);
        tips->setWordWrap(true);
        root->addWidget(tips);
    } else if (m_topologyId == QLatin1String("fly")) {
        auto* tips = new QLabel(tr("fly 推荐：dest_tag（默认）。\n"
                                   "默认已下调为较小规模（k≈4）。"),
                                this);
        tips->setWordWrap(true);
        root->addWidget(tips);
    } else if (m_topologyId == QLatin1String("qtree")) {
        auto* tips = new QLabel(tr("qtree 推荐：nca（默认）。\n"
                                   "当前 BookSim2 内置 qtree 仅支持 k=4、n=3。"),
                                this);
        tips->setWordWrap(true);
        root->addWidget(tips);
    } else if (m_topologyId == QLatin1String("tree4")) {
        auto* tips = new QLabel(tr("tree4 推荐：nca（默认）/ anca。\n"
                                   "当前 BookSim2 内置 tree4 仅支持 k=4、n=3。"),
                                this);
        tips->setWordWrap(true);
        root->addWidget(tips);
    } else if (m_topologyId == QLatin1String("fattree")) {
        auto* tips = new QLabel(tr("fattree 推荐：nca（默认）/ anca。\n"
                                   "默认规模建议：k=4、n=2 起步。"),
                                this);
        tips->setWordWrap(true);
        root->addWidget(tips);
    } else if (m_topologyId == QLatin1String("flatfly")) {
        auto* tips = new QLabel(tr("flatfly 推荐：ran_min（默认）/ xyyx / adaptive_xyyx。\n"
                                   "建议起步：k=4、n=2、c=1。"),
                                this);
        tips->setWordWrap(true);
        root->addWidget(tips);
    } else if (m_topologyId == QLatin1String("dragonflynew")) {
        auto* tips = new QLabel(tr("dragonflynew 推荐：min（默认）/ ugal。\n"
                                   "当前 BookSim2 内置 dragonflynew 仅支持 n=1。"),
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
    if (p.topologyId == QLatin1String("cmesh")) {
        p.n = 2;
        p.c = 4;
    } else if (p.topologyId == QLatin1String("qtree") || p.topologyId == QLatin1String("tree4")) {
        p.k = 4;
        p.n = 3;
    } else if (p.topologyId == QLatin1String("dragonflynew")) {
        p.n = 1;
    }
    p.routingFunction = normalizeRoutingFunction(m_rfEdit ? m_rfEdit->text() : QString(),
                                                 m_topologyId);
    return p;
}

void BooksimTopologyPlaceDialog::setParams(const BooksimTopologyParams& p) {
    m_topologyId = p.topologyId;
    m_displayLabel = p.displayLabel;
    if (m_kSpin) {
        m_kSpin->setValue(
            (p.topologyId == QLatin1String("qtree") || p.topologyId == QLatin1String("tree4"))
                ? 4
                : p.k);
    }
    if (m_nSpin) {
        int nVal = p.n;
        if (p.topologyId == QLatin1String("cmesh")) {
            nVal = 2;
        } else if (p.topologyId == QLatin1String("qtree")
                   || p.topologyId == QLatin1String("tree4")) {
            nVal = 3;
        } else if (p.topologyId == QLatin1String("dragonflynew")) {
            nVal = 1;
        }
        m_nSpin->setValue(nVal);
    }
    if (m_cSpin) {
        m_cSpin->setValue((p.topologyId == QLatin1String("cmesh")) ? 4 : p.c);
    }
    if (m_rfEdit) {
        m_rfEdit->setText(normalizeRoutingFunction(p.routingFunction, p.topologyId));
    }
}
