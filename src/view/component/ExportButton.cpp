#include "ExportButton.h"
#include <QMouseEvent>

ExportButton::ExportButton(ElaIconType::IconName awesome, const QString& text, QWidget* parent)
    : ElaIconButton(awesome, parent)
    , m_text(text) {
    setCursor(Qt::PointingHandCursor);
    setToolTip(text);
}

ExportButton::ExportButton(ElaIconType::IconName awesome,
                           int pixelSize,
                           const QString& text,
                           QWidget* parent)
    : ElaIconButton(awesome, pixelSize, parent)
    , m_text(text) {
    setCursor(Qt::PointingHandCursor);
    setToolTip(text);
}

void ExportButton::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        emit exportRequested();
    }
    ElaIconButton::mousePressEvent(event);
}
