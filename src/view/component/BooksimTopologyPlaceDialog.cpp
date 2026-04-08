#include "BooksimTopologyPlaceDialog.h"
#include "utils/BooksimRoutingCatalog.h"
#include <ElaComboBox.h>
#include <ElaPushButton.h>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QVariant>

void BooksimTopologyPlaceDialog::syncRoutingCombo(const QString& topologyId,
                                                  const QString& routingOrEmpty) {
    if (!m_rfCombo) {
        return;
    }
    const QString topo = topologyId.trimmed().toLower();
    const QString pick = normalizeRoutingForTopology(routingOrEmpty.isEmpty()
                                                         ? defaultRoutingIdForTopology(topo)
                                                         : routingOrEmpty,
                                                     topologyId);

    m_rfCombo->clear();
    const QStringList ids = routingIdsForTopology(topo);
    for (const QString& id : ids) {
        m_rfCombo->addItem(routingUiLabel(topo, id), id);
    }
    if (m_rfCombo->count() == 0) {
        const QString fb = defaultRoutingIdForTopology(topo);
        m_rfCombo->addItem(tr("%1 · 请与 BookSim 一致").arg(routingUiLabel(topo, fb)), fb);
    }
    const int idx = m_rfCombo->findData(pick);
    m_rfCombo->setCurrentIndex(idx >= 0 ? idx : 0);
}

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
        m_nSpin->setRange(2, 2);
        m_nSpin->setValue(2);
        m_nSpin->setEnabled(false);
        m_cSpin->setRange(4, 4);
        m_cSpin->setValue(4);
        m_cSpin->setEnabled(false);
    } else if (topo == QLatin1String("qtree") || topo == QLatin1String("tree4")) {
        m_kSpin->setRange(4, 4);
        m_kSpin->setValue(4);
        m_kSpin->setEnabled(false);
        m_nSpin->setRange(3, 3);
        m_nSpin->setValue(3);
        m_nSpin->setEnabled(false);
    } else if (topo == QLatin1String("dragonflynew")) {
        m_nSpin->setRange(1, 1);
        m_nSpin->setValue(1);
        m_nSpin->setEnabled(false);
    }

    m_rfCombo = new ElaComboBox(this);
    m_rfCombo->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    m_rfCombo->setToolTip(
        tr("与 BookSim2 中 routing_function 基名一致；仿真时会自动拼接「_拓扑类型」再查表。"));
    syncRoutingCombo(m_topologyId, defaultRoutingIdForTopology(topo));

    form->addRow(tr("k"), m_kSpin);
    form->addRow(tr("n"), m_nSpin);
    form->addRow(tr("c"), m_cSpin);
    form->addRow(tr("路由算法"), m_rfCombo);
    root->addLayout(form);

    if (topo == QLatin1String("cmesh")) {
        auto* tips = new QLabel(tr("参数：内置 cmesh 固定为 n=2、c=4。"), this);
        tips->setWordWrap(true);
        root->addWidget(tips);
    } else if (topo == QLatin1String("qtree") || topo == QLatin1String("tree4")) {
        auto* tips = new QLabel(tr("参数：内置 qtree / tree4 固定为 k=4、n=3。"), this);
        tips->setWordWrap(true);
        root->addWidget(tips);
    } else if (topo == QLatin1String("dragonflynew")) {
        auto* tips = new QLabel(tr("参数：内置 dragonflynew 固定为 n=1。"), this);
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
    QString rfFromUi;
    if (m_rfCombo) {
        const QVariant v = m_rfCombo->currentData(Qt::UserRole);
        rfFromUi = v.toString();
        if (rfFromUi.isEmpty()) {
            rfFromUi = m_rfCombo->currentText();
        }
    }
    p.routingFunction = normalizeRoutingForTopology(rfFromUi, m_topologyId);
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
    syncRoutingCombo(m_topologyId, p.routingFunction);
}
