#include "CanvasPage.h"
#include "component/CanvasView.h"
#include "component/DragButton.h"
#include "component/GraphScene.h"
#include <ElaGraphicsScene.h>
#include <ElaGraphicsView.h>
#include <ElaIconButton.h>
#include <ElaImageCard.h>
#include <ElaMessageBar.h>
#include <ElaText.h>
#include <ElaToolBar.h>
#include <QApplication>
#include <QClipboard>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <Version.h>

CanvasPage::CanvasPage(QWidget* parent)
    : BasePage(parent) {
    setWindowTitle("Canvas");

    auto* toolBar = new ElaToolBar();

    // 创建拖拽按钮
    auto* circleBtn = new DragButton(ElaIconType::Circle, "Circle", toolBar);
    auto* squareBtn = new DragButton(ElaIconType::Square, "Square", toolBar);
    auto* arrowBtn = new DragButton(ElaIconType::ArrowRight, "ArrowRight", toolBar);

    // 添加到工具栏
    toolBar->addWidget(circleBtn);
    toolBar->addSeparator();
    toolBar->addWidget(squareBtn);
    toolBar->addSeparator();
    toolBar->addWidget(arrowBtn);
    toolBar->setStyleSheet("background-color: purple;");
    toolBar->setFixedHeight(50);
    toolBar->setSizePolicy(toolBar->sizePolicy().horizontalPolicy(),
                           toolBar->sizePolicy().verticalPolicy());

    auto* toolBarLayout = new QHBoxLayout();
    toolBarLayout->addWidget(toolBar);
    toolBarLayout->addSpacing(20);
    toolBarLayout->setAlignment(Qt::AlignLeft);

    // 画布
    auto* scene = new GraphScene(this);
    auto* view = new CanvasView(scene, this);

    auto centralWidget = new QWidget(this);
    centralWidget->setWindowTitle("Canvas");
    auto centerLayout = new QVBoxLayout(centralWidget);
    centerLayout->addLayout(toolBarLayout);
    centerLayout->addSpacing(15);
    centerLayout->addWidget(view);
    centerLayout->setContentsMargins(0, 0, 20, 0);
    addCentralWidget(centralWidget, true, true, 0);
}
