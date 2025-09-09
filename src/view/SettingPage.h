#pragma once

#include "BasePage.h"
#include <ElaScrollPageArea.h>
#include <ElaText.h>
#include <QHBoxLayout>

class SettingPage : public BasePage {
    Q_OBJECT
public:
    explicit SettingPage(QWidget* parent = nullptr);

private:
    template<typename... Widget>
    ElaScrollPageArea* createScrollPageArea(const QString& label, Widget*... widgets)
        requires(std::is_base_of_v<QWidget, Widget> && ...)
    {
        auto* area = new ElaScrollPageArea(this);
        auto* layout = new QHBoxLayout(area);
        auto* labelText = new ElaText(label, this);
        labelText->setWordWrap(false);
        labelText->setTextPixelSize(15);
        layout->addWidget(labelText);
        layout->addStretch();
        (layout->addWidget(widgets), ...);
        return area;
    }
};
