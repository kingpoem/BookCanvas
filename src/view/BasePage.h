#pragma once

#include <ElaScrollPage.h>

class BasePage : public ElaScrollPage {
    Q_OBJECT
public:
    explicit BasePage(QWidget* parent = nullptr);
    ~BasePage() override;
};
