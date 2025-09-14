#pragma once

#include "BasePage.h"
#include <ElaScrollPageArea.h>
#include <QHBoxLayout>

class CanvasPage : public BasePage {
    Q_OBJECT
public:
    explicit CanvasPage(QWidget* parent = nullptr);
};
