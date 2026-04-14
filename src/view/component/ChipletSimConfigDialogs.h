#pragma once

#include <QDialog>
#include <QMap>
#include <QString>

class GraphChiplet;
class GraphScene;
class ElaComboBox;
class ElaLineEdit;
class ElaSpinBox;

/// 单颗芯粒在 chiplet_mesh 中的 BookSim 相关参数（网格坐标与 die 级配置）
class ChipletDieParamsDialog final : public QDialog {
    Q_OBJECT
public:
    ChipletDieParamsDialog(GraphScene* scene, GraphChiplet* chiplet, QWidget* parent = nullptr);

    void applyToChiplet() const;

protected:
    void accept() override;

private:
    GraphScene* m_scene = nullptr;
    GraphChiplet* m_chiplet = nullptr;
    ElaSpinBox* m_cxSpin = nullptr;
    ElaSpinBox* m_cySpin = nullptr;
    ElaSpinBox* m_dieKSpin = nullptr;
    ElaLineEdit* m_intraEdit = nullptr;
    ElaLineEdit* m_clockPeriodEdit = nullptr;
    ElaLineEdit* m_clockPhaseEdit = nullptr;
};

/// 跨芯粒（D2D）链路在 BookSim 中对应的全局参数编辑入口
class ChipletD2dGlobalsDialog final : public QDialog {
    Q_OBJECT
public:
    explicit ChipletD2dGlobalsDialog(GraphScene* scene, QWidget* parent = nullptr);

private:
    void accept() override;

    GraphScene* m_scene = nullptr;
    ElaComboBox* m_connectCombo = nullptr;
    ElaLineEdit* m_d2dLat = nullptr;
    ElaLineEdit* m_cdcEnable = nullptr;
    ElaLineEdit* m_cdcFifo = nullptr;
    ElaLineEdit* m_cdcSync = nullptr;
    ElaLineEdit* m_cdcCredSync = nullptr;
    ElaLineEdit* m_cdcGrayFifo = nullptr;
    ElaLineEdit* m_cdcGrayStages = nullptr;
    ElaLineEdit* m_intraLat = nullptr;
};
