#include "BooksimTopologyPlaceDialog.h"
#include <ElaLineEdit.h>
#include <ElaPushButton.h>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QSpinBox>
#include <QVBoxLayout>

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
    const QString hintText
        = (m_topologyId == QLatin1String("mesh"))
              ? tr("确认参数后，在画布空白处单击一次会放置 mesh 组件，并自动生成"
                   "路由器、终端与内部连线。\n你仍可与现有节点继续手动连线。")
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
    m_kSpin->setValue(8);
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
    m_rfEdit->setText(QStringLiteral("min"));
    m_rfEdit->setPlaceholderText(QStringLiteral("min / dim_order / ..."));
    m_rfEdit->setToolTip(tr("routing_function：BookSim 中与路由函数注册名一致"));

    form->addRow(tr("k"), m_kSpin);
    form->addRow(tr("n"), m_nSpin);
    form->addRow(tr("c"), m_cSpin);
    form->addRow(tr("路由函数"), m_rfEdit);
    root->addLayout(form);

    auto* box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    root->addWidget(box);
    connect(box, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(box, &QDialogButtonBox::rejected, this, &QDialog::reject);

    if (m_topologyId == QLatin1String("anynet")) {
        m_kSpin->setEnabled(false);
        m_nSpin->setEnabled(false);
        m_cSpin->setEnabled(false);
        m_kSpin->setToolTip(tr("anynet 拓扑由导出的 network_file 描述，k/n/c 由列表文件决定。"));
        m_nSpin->setToolTip(m_kSpin->toolTip());
        m_cSpin->setToolTip(m_kSpin->toolTip());
    }
}

BooksimTopologyParams BooksimTopologyPlaceDialog::getParams() const {
    BooksimTopologyParams p;
    p.topologyId = m_topologyId;
    p.displayLabel = m_displayLabel;
    p.k = m_kSpin ? m_kSpin->value() : 8;
    p.n = m_nSpin ? m_nSpin->value() : 2;
    p.c = m_cSpin ? m_cSpin->value() : 1;
    p.routingFunction = m_rfEdit ? m_rfEdit->text().trimmed() : QStringLiteral("min");
    if (p.routingFunction.isEmpty()) {
        p.routingFunction = QStringLiteral("min");
    }
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
        m_rfEdit->setText(p.routingFunction);
    }
}
