#include "CanvasPage.h"
#include "component/DragButton.h"
#include "component/ExportButton.h"
#include "component/GraphNode.h"
#include "component/GraphScene.h"
#include "component/GraphView.h"
#include "component/RouterConfigDialog.h"
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
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QVBoxLayout>
#include <Version.h>

CanvasPage::CanvasPage(QWidget* parent)
    : BasePage(parent) {
    setWindowTitle("Canvas");

    auto* toolBar = new ElaToolBar();

    // 创建拖拽按钮
    auto* circleBtn = new DragButton(ElaIconType::Circle, "Circle", toolBar); // 节点创建可拖拽按钮
    auto* routerBtn = new DragButton(ElaIconType::Square, "Router", toolBar); // 路由器创建可拖拽按钮
    auto* showBtn = new ShowButton(ElaIconType::Eye, "eye", toolBar); // 线条权重显示/隐藏按钮
    auto* exportBtn = new ExportButton(ElaIconType::Download, "Export File", toolBar); // 导出按钮
    auto* exportConfigBtn = new ExportButton(ElaIconType::Gear,
                                             "Export Config",
                                             toolBar); // 导出配置按钮

    // 添加到工具栏
    toolBar->addWidget(circleBtn);
    toolBar->addWidget(routerBtn);
    toolBar->addSeparator();
    toolBar->addWidget(showBtn);
    toolBar->addSeparator();
    toolBar->addWidget(exportBtn);
    toolBar->addWidget(exportConfigBtn);
    toolBar->setFixedHeight(50);
    toolBar->setSizePolicy(toolBar->sizePolicy().horizontalPolicy(),
                           toolBar->sizePolicy().verticalPolicy());

    auto* toolBarLayout = new QHBoxLayout();
    toolBarLayout->addWidget(toolBar);
    toolBarLayout->addSpacing(20);
    toolBarLayout->setAlignment(Qt::AlignLeft);

    // auto* labelX = new QLabel("LabelX", this);
    // auto* labelY = new QLabel("LabelY", this);
    // auto* labelSceneX = new QLabel("LabelSceneX", this);
    // auto* labelSceneY = new QLabel("LabelSceneY", this);
    // labelX->setStyleSheet("color: black;");
    // labelY->setStyleSheet("color: black;");
    // labelSceneX->setStyleSheet("color: black;");
    // labelSceneY->setStyleSheet("color: black;");

    // 画布
    auto* scene = new GraphScene(this);
    auto* view = new GraphView(scene, this); // 外部传入 scene 便于控制

    // 连接ShowButton的信号到GraphScene的权重显示控制
    connect(showBtn, &ShowButton::toggled, scene, &GraphScene::setAllEdgeWeightsVisible);

    // 连接导出anynet_file按钮的信号
    connect(exportBtn, &ExportButton::exportRequested, [scene, this]() {
        QString fileName = QFileDialog::getSaveFileName(this,
                                                        "导出网络配置文件",
                                                        "network_config.txt",
                                                        "Text Files (*.txt);;All Files (*)");

        if (!fileName.isEmpty()) {
            scene->exportGraph(fileName);
            QMessageBox::information(this, "导出成功", "网络配置文件已导出到: " + fileName);
        }
    });

    // 连接导出anynet_config按钮的信号
    connect(exportConfigBtn, &ExportButton::exportRequested, [scene, this]() {
        QString fileName = QFileDialog::getSaveFileName(this,
                                                        "导出路由器配置",
                                                        "anynet_config",
                                                        "Config Files (*);;All Files (*)");

        if (!fileName.isEmpty()) {
            scene->exportRouterConfig(fileName);
            QMessageBox::information(this, "导出成功", "路由器配置已导出到: " + fileName);
        }
    });

    // 连接场景的配置请求信号（当节点右键点击时触发）
    connect(scene, &GraphScene::nodeConfigureRequested, [scene, this](GraphNode* node) {
        if (node && node->getType() == GraphNode::Router) {
            RouterConfigDialog dialog(this);
            dialog.setConfig(scene->getRouterConfig());

            if (dialog.exec() == QDialog::Accepted) {
                scene->setRouterConfig(dialog.getConfig());
            }
        }
    });

    auto* labelLayout = new QHBoxLayout();
    // labelLayout->addWidget(labelX);
    // labelLayout->addWidget(labelY);
    // labelLayout->addWidget(labelSceneX);
    // labelLayout->addWidget(labelSceneY);
    labelLayout->addStretch();

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
