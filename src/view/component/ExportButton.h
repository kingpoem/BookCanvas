#pragma once
#include "ElaIconButton.h"
#include <QWidget>

class ExportButton : public ElaIconButton {
    Q_OBJECT
public:
    explicit ExportButton(ElaIconType::IconName awesome, const QString& text, QWidget* parent = nullptr);
    explicit ExportButton(ElaIconType::IconName awesome, int pixelSize, const QString& text, QWidget* parent = nullptr);

signals:
    void exportRequested(); // 请求导出信号

protected:
    void mousePressEvent(QMouseEvent* event) override;

private:
    QString m_text;
};