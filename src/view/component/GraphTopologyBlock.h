#pragma once

#include "BooksimTopologyParams.h"
#include "ElaGraphicsItem.h"
#include <QGraphicsSceneContextMenuEvent>

/// 画布上的 BookSim 内置拓扑示意块（导出 JSON 时可合并 topology / k / n / c / routing_function）
class GraphTopologyBlock final : public ElaGraphicsItem {
    Q_OBJECT
public:
    explicit GraphTopologyBlock(QString blockId,
                                BooksimTopologyParams params,
                                QGraphicsItem* parent = nullptr);

    [[nodiscard]] QString blockId() const { return m_blockId; }
    [[nodiscard]] BooksimTopologyParams params() const { return m_params; }
    void setParams(const BooksimTopologyParams& p);

    [[nodiscard]] QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

Q_SIGNALS:
    void configureRequested(GraphTopologyBlock* block);
    void deleteRequested(GraphTopologyBlock* block);

protected:
    void contextMenuEvent(QGraphicsSceneContextMenuEvent* event) override;

private:
    QString m_blockId;
    BooksimTopologyParams m_params;
    QRectF m_rect = QRectF(0, 0, 168, 88);
};
