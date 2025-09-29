#include "CanvasPage.h"
#include "component/GraphView.h"
#include "component/DragButton.h"
#include "component/GraphScene.h"
#include "component/ShowButton.h"
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
    auto* circleBtn = new DragButton(ElaIconType::Circle, "Circle", toolBar); // 节点创建可拖拽按钮
    auto* showBtn = new ShowButton(ElaIconType::Eye, "eye", toolBar); // 线条权重显示/隐藏按钮

    // 添加到工具栏
    toolBar->addWidget(circleBtn);
    toolBar->addSeparator();
    toolBar->addWidget(showBtn);
    toolBar->setFixedHeight(50);
    toolBar->setSizePolicy(toolBar->sizePolicy().horizontalPolicy(),
                           toolBar->sizePolicy().verticalPolicy());

    auto* toolBarLayout = new QHBoxLayout();
    toolBarLayout->addWidget(toolBar);
    toolBarLayout->addSpacing(20);
    toolBarLayout->setAlignment(Qt::AlignLeft);

    // 画布
    auto* scene = new GraphScene(this);
    auto* view = new GraphView(scene, this); // 外部传入 scene 便于控制

    auto* labelX = new QLabel("LabelX", this);
    auto* labelY = new QLabel("LabelY", this);
    auto* labelSceneX = new QLabel("LabelSceneX", this);
    auto* labelSceneY = new QLabel("LabelSceneY", this);
    auto* labelLayout = new QHBoxLayout();
    labelLayout->addWidget(labelX);
    labelLayout->addWidget(labelY);
    labelLayout->addWidget(labelSceneX);
    labelLayout->addWidget(labelSceneY);
    labelLayout->addStretch();
    labelX->setStyleSheet("color: black;");
    labelY->setStyleSheet("color: black;");
    labelSceneX->setStyleSheet("color: black;");
    labelSceneY->setStyleSheet("color: black;");

    auto centralWidget = new QWidget(this);
    centralWidget->setWindowTitle("Canvas");
    auto centerLayout = new QVBoxLayout(centralWidget);
    centerLayout->addLayout(toolBarLayout);
    centerLayout->addSpacing(15);
    centerLayout->addLayout(labelLayout);
    centerLayout->addWidget(view);
    centerLayout->setContentsMargins(0, 0, 20, 0);
    addCentralWidget(centralWidget, true, true, 0);
}
