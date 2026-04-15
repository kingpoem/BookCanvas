#include "ChipletSimConfigDialogs.h"
#include "GraphChiplet.h"
#include "GraphScene.h"
#include "utils/WindowsLightDialogPolish.h"
#include <ElaComboBox.h>
#include <ElaLineEdit.h>
#include <ElaPushButton.h>
#include <ElaSpinBox.h>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QMessageBox>
#include <QVBoxLayout>

ChipletDieParamsDialog::ChipletDieParamsDialog(GraphScene* scene,
                                               GraphChiplet* chiplet,
                                               QWidget* parent)
    : QDialog(parent)
    , m_scene(scene)
    , m_chiplet(chiplet) {
    setWindowTitle(tr("芯粒仿真参数"));
    setMinimumWidth(420);

    auto* form = new QFormLayout();
    m_cxSpin = new ElaSpinBox(this);
    m_cxSpin->setRange(0, 64);
    m_cySpin = new ElaSpinBox(this);
    m_cySpin->setRange(0, 64);
    m_dieKSpin = new ElaSpinBox(this);
    m_dieKSpin->setRange(1, 256);

    m_intraEdit = new ElaLineEdit(this);
    m_intraEdit->setPlaceholderText(tr("留空表示导出时用 0（回退 chiplet_intra_latency）"));
    m_clockPeriodEdit = new ElaLineEdit(this);
    m_clockPeriodEdit->setPlaceholderText(tr("留空导出为 1（相对仿真 tick）"));
    m_clockPhaseEdit = new ElaLineEdit(this);
    m_clockPhaseEdit->setPlaceholderText(tr("留空导出为 0"));

    if (m_chiplet) {
        m_cxSpin->setValue(m_chiplet->gridCx());
        m_cySpin->setValue(m_chiplet->gridCy());
        m_dieKSpin->setValue(qMax(1, m_chiplet->dieK()));
        m_intraEdit->setText(m_chiplet->dieIntraLatencyText());
        m_clockPeriodEdit->setText(m_chiplet->dieClockPeriodText());
        m_clockPhaseEdit->setText(m_chiplet->dieClockPhaseText());
    }

    form->addRow(tr("网格列坐标 cx（自0）"), m_cxSpin);
    form->addRow(tr("网格行坐标 cy（自 0）"), m_cySpin);
    form->addRow(tr("片内 k（每边路由器数）"), m_dieKSpin);
    form->addRow(tr("片内跳延迟覆盖"), m_intraEdit);
    form->addRow(tr("片时钟周期"), m_clockPeriodEdit);
    form->addRow(tr("片时钟相位"), m_clockPhaseEdit);

    auto* hint = new QLabel(
        tr("坐标用于导出 chiplet_x×chiplet_y 网格中的位置；同一坐标只能对应一颗芯粒。"
           "导出 chiplet_mesh 时将根据所有芯粒坐标计算 chiplet_die_k 等数组。"
           "若某 die 的时钟周期≠1 或相位≠0，BookSim 要求开启 CDC：请在「全局配置」"
           "中拓扑选 chiplet_mesh 后，将「启用跨时钟域 D2D」设为 1（导出时会自动置 1）。"),
        this);
    hint->setWordWrap(true);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &ChipletDieParamsDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &ChipletDieParamsDialog::reject);

    auto* lay = new QVBoxLayout(this);
    lay->addLayout(form);
    lay->addWidget(hint);
    lay->addWidget(buttons);
    BookCanvasUi::installWindowsLightTopLevelDialogPolish(this);
}

void ChipletDieParamsDialog::accept() {
    if (!m_scene || !m_chiplet) {
        QDialog::accept();
        return;
    }
    const int cx = m_cxSpin->value();
    const int cy = m_cySpin->value();
    for (GraphChiplet* o : m_scene->chiplets()) {
        if (!o || o == m_chiplet) {
            continue;
        }
        if (o->gridCx() == cx && o->gridCy() == cy) {
            QMessageBox::warning(this,
                                 tr("坐标冲突"),
                                 tr("坐标 (%1, %2) 已被芯粒「%3」占用。")
                                     .arg(cx)
                                     .arg(cy)
                                     .arg(o->label()));
            return;
        }
    }
    QDialog::accept();
}

void ChipletDieParamsDialog::applyToChiplet() const {
    if (!m_chiplet || !m_cxSpin || !m_cySpin || !m_dieKSpin) {
        return;
    }
    m_chiplet->setGridCx(m_cxSpin->value());
    m_chiplet->setGridCy(m_cySpin->value());
    m_chiplet->setDieK(m_dieKSpin->value());
    m_chiplet->setDieIntraLatencyText(m_intraEdit ? m_intraEdit->text() : QString());
    m_chiplet->setDieClockPeriodText(m_clockPeriodEdit ? m_clockPeriodEdit->text() : QString());
    m_chiplet->setDieClockPhaseText(m_clockPhaseEdit ? m_clockPhaseEdit->text() : QString());
}

ChipletD2dGlobalsDialog::ChipletD2dGlobalsDialog(GraphScene* scene, QWidget* parent)
    : QDialog(parent)
    , m_scene(scene) {
    setWindowTitle(tr("跨芯粒链路（D2D）全局参数"));
    setMinimumWidth(460);

    auto* form = new QFormLayout();
    m_connectCombo = new ElaComboBox(this);
    m_connectCombo->addItems({QStringLiteral("x"), QStringLiteral("xy")});

    m_d2dLat = new ElaLineEdit(this);
    m_cdcEnable = new ElaLineEdit(this);
    m_cdcFifo = new ElaLineEdit(this);
    m_cdcSync = new ElaLineEdit(this);
    m_cdcCredSync = new ElaLineEdit(this);
    m_cdcGrayFifo = new ElaLineEdit(this);
    m_cdcGrayStages = new ElaLineEdit(this);
    m_intraLat = new ElaLineEdit(this);

    QMap<QString, QString> g;
    if (m_scene) {
        g = m_scene->getGlobalConfig();
    }
    auto getv = [&g](const char* k, const QString& d) -> QString {
        return g.value(QLatin1String(k), d);
    };

    if (m_scene) {
        const QString c = m_scene->chipletMeshConnect().trimmed().toLower();
        m_connectCombo->setCurrentIndex(c == QLatin1String("xy") ? 1 : 0);
    }
    m_d2dLat->setText(getv("chiplet_d2d_latency", QStringLiteral("2")));
    m_cdcEnable->setText(getv("chiplet_cdc_enable", QStringLiteral("0")));
    m_cdcFifo->setText(getv("chiplet_cdc_fifo_depth", QStringLiteral("64")));
    m_cdcSync->setText(getv("chiplet_cdc_sync_cycles", QStringLiteral("2")));
    m_cdcCredSync->setText(getv("chiplet_cdc_credit_sync_cycles", QStringLiteral("2")));
    m_cdcGrayFifo->setText(getv("chiplet_cdc_gray_fifo", QStringLiteral("0")));
    m_cdcGrayStages->setText(getv("chiplet_cdc_gray_stages", QStringLiteral("2")));
    m_intraLat->setText(getv("chiplet_intra_latency", QStringLiteral("1")));

    form->addRow(tr("chiplet_connect"), m_connectCombo);
    form->addRow(tr("chiplet_d2d_latency"), m_d2dLat);
    form->addRow(tr("chiplet_intra_latency"), m_intraLat);
    form->addRow(tr("chiplet_cdc_enable"), m_cdcEnable);
    form->addRow(tr("chiplet_cdc_fifo_depth"), m_cdcFifo);
    form->addRow(tr("chiplet_cdc_sync_cycles"), m_cdcSync);
    form->addRow(tr("chiplet_cdc_credit_sync_cycles"), m_cdcCredSync);
    form->addRow(tr("chiplet_cdc_gray_fifo"), m_cdcGrayFifo);
    form->addRow(tr("chiplet_cdc_gray_stages"), m_cdcGrayStages);

    auto* hint = new QLabel(
        tr("BookSim 中 D2D/CDC 参数为全局配置；此处修改会写入当前画布的全局配置映射，"
           "导出 JSON 时一并输出。若多 die 时钟不同，请开启 CDC 并在各芯粒上填写时钟字段。"),
        this);
    hint->setWordWrap(true);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &ChipletD2dGlobalsDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &ChipletD2dGlobalsDialog::reject);

    auto* lay = new QVBoxLayout(this);
    lay->addLayout(form);
    lay->addWidget(hint);
    lay->addWidget(buttons);
    BookCanvasUi::installWindowsLightTopLevelDialogPolish(this);
}

void ChipletD2dGlobalsDialog::accept() {
    if (m_scene) {
        m_scene->setChipletMeshConnect(m_connectCombo ? m_connectCombo->currentText()
                                                      : QStringLiteral("x"));
        QMap<QString, QString> g = m_scene->getGlobalConfig();
        g.insert(QStringLiteral("chiplet_connect"),
                 m_connectCombo ? m_connectCombo->currentText().trimmed() : QStringLiteral("x"));
        g.insert(QStringLiteral("chiplet_d2d_latency"), m_d2dLat->text().trimmed());
        g.insert(QStringLiteral("chiplet_cdc_enable"), m_cdcEnable->text().trimmed());
        g.insert(QStringLiteral("chiplet_cdc_fifo_depth"), m_cdcFifo->text().trimmed());
        g.insert(QStringLiteral("chiplet_cdc_sync_cycles"), m_cdcSync->text().trimmed());
        g.insert(QStringLiteral("chiplet_cdc_credit_sync_cycles"), m_cdcCredSync->text().trimmed());
        g.insert(QStringLiteral("chiplet_cdc_gray_fifo"), m_cdcGrayFifo->text().trimmed());
        g.insert(QStringLiteral("chiplet_cdc_gray_stages"), m_cdcGrayStages->text().trimmed());
        g.insert(QStringLiteral("chiplet_intra_latency"), m_intraLat->text().trimmed());
        m_scene->setGlobalConfig(g);
    }
    QDialog::accept();
}
