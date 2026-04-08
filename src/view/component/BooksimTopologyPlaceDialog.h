#pragma once

#include "BooksimTopologyParams.h"
#include <QDialog>

class QSpinBox;
class ElaComboBox;
class ElaPushButton;

/// 放置或编辑单个 BookSim2 内置拓扑时的参数对话框（k / n / c / routing_function）
class BooksimTopologyPlaceDialog final : public QDialog {
    Q_OBJECT
public:
    explicit BooksimTopologyPlaceDialog(const QString& topologyId,
                                        const QString& displayLabel,
                                        QWidget* parent = nullptr);

    [[nodiscard]] BooksimTopologyParams getParams() const;
    void setParams(const BooksimTopologyParams& p);

private:
    void buildUi(const QString& displayLabel);
    void syncRoutingCombo(const QString& topologyId, const QString& routingOrEmpty);

    QString m_topologyId;
    QString m_displayLabel;
    QSpinBox* m_kSpin = nullptr;
    QSpinBox* m_nSpin = nullptr;
    QSpinBox* m_cSpin = nullptr;
    ElaComboBox* m_rfCombo = nullptr;
};
