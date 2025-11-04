#include "CanvasPage.h"
#include "component/DragButton.h"
#include "component/ExportButton.h"
#include "component/GraphNode.h"
#include "component/GraphScene.h"
#include "component/GraphView.h"
#include "component/RouterConfigDialog.h"
#include "component/RouterGlobalConfigDialog.h"
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
#include <QLabel>
#include <QMessageBox>
#include <QStandardPaths>
#include <QVBoxLayout>
#include <Version.h>

static QWidget* createButtonWithLabel(QWidget* button, const QString& labelText, QWidget* parent) {
    auto* container = new QWidget(parent);
    auto* layout = new QVBoxLayout(container);
    layout->setContentsMargins(2, 2, 2, 2);
    layout->setSpacing(2);
    layout->setAlignment(Qt::AlignCenter);

    layout->addWidget(button, 0, Qt::AlignCenter);

    auto* label = new QLabel(labelText, container);
    label->setAlignment(Qt::AlignCenter);
    label->setStyleSheet("font-size: 10px; color: #666;");
    layout->addWidget(label);

    container->setLayout(layout);
    return container;
}

// clang-format off
CanvasPage::CanvasPage(QWidget* parent)
    : BasePage(parent) {
    setWindowTitle("Canvas");

    auto* toolBar = new ElaToolBar();

    auto* circleBtn = new DragButton(ElaIconType::Circle, "Circle", toolBar);
    auto* routerBtn = new DragButton(ElaIconType::Square, "Router", toolBar);
    auto* showBtn = new ShowButton(ElaIconType::Eye, "eye", toolBar);
    auto* exportBtn = new ExportButton(ElaIconType::Download, "Export File", toolBar);
    auto* exportConfigBtn = new ExportButton(ElaIconType::Gear, "Export Config", toolBar);
    auto* globalConfigBtn = new ExportButton(ElaIconType::CarWrench, "Export Global Config", toolBar);

    globalConfigBtn->setToolTip("Global Config - Configure global router parameters");
    exportConfigBtn->setToolTip("Export Config - Export JSON configuration file");
    exportBtn->setToolTip("Export Topology - Export network topology file (anynet_file)");
    showBtn->setToolTip("Toggle Weight - Show or hide edge weights");
    circleBtn->setToolTip("Add Node - Drag to canvas to create a new node");
    routerBtn->setToolTip("Add Router - Drag to canvas to create a new router");

    auto* circleBtnContainer = createButtonWithLabel(circleBtn, "Add Node", toolBar);
    auto* routerBtnContainer = createButtonWithLabel(routerBtn, "Add Router", toolBar);
    auto* showBtnContainer = createButtonWithLabel(showBtn, "Toggle Weight", toolBar);
    auto* exportBtnContainer = createButtonWithLabel(exportBtn, "Export Topology", toolBar);
    auto* exportConfigBtnContainer = createButtonWithLabel(exportConfigBtn, "Export Config", toolBar);
    auto* globalConfigBtnContainer = createButtonWithLabel(globalConfigBtn, "Global Config", toolBar);

    toolBar->addWidget(circleBtnContainer);
    toolBar->addWidget(routerBtnContainer);
    toolBar->addSeparator();
    toolBar->addWidget(showBtnContainer);
    toolBar->addSeparator();
    toolBar->addWidget(globalConfigBtnContainer);
    toolBar->addSeparator();
    toolBar->addWidget(exportBtnContainer);
    toolBar->addWidget(exportConfigBtnContainer);
    toolBar->setFixedHeight(70);
    toolBar->setSizePolicy(toolBar->sizePolicy().horizontalPolicy(), toolBar->sizePolicy().verticalPolicy());

    auto* toolBarLayout = new QHBoxLayout();
    toolBarLayout->addWidget(toolBar);
    toolBarLayout->addSpacing(20);
    toolBarLayout->setAlignment(Qt::AlignLeft);

    auto* scene = new GraphScene(this);
    auto* view = new GraphView(scene, this); // 外部传入 scene 便于控制

    connect(showBtn, &ShowButton::toggled, scene, &GraphScene::setAllEdgeWeightsVisible);

    QString booksimDir = QDir(QCoreApplication::applicationDirPath()).absolutePath();

    QDir buildDir(QDir(QCoreApplication::applicationDirPath()).absolutePath());
    if (!buildDir.exists("booksim")) {
        QString possibleBooksimPath = QDir(QCoreApplication::applicationDirPath()).absoluteFilePath( "../../../../3rdpart/booksim2/src/booksim");
        QFileInfo booksimInfo(possibleBooksimPath);
        if (booksimInfo.exists()) {
            booksimDir = booksimInfo.absolutePath();
        }
    } else {
        booksimDir = buildDir.absolutePath();
    }

    connect(exportBtn, &ExportButton::exportRequested, [scene, this, booksimDir]() {
        QString defaultFilePath = QDir(booksimDir).filePath("anynet_file");
        QString fileName = QFileDialog::getSaveFileName(this, "导出网络配置文件", defaultFilePath, "All Files (*)");
        if (!fileName.isEmpty()) {
            scene->exportGraph(fileName);
            QMessageBox::information(this, "导出成功", "网络配置文件已导出到: " + fileName);
        }
    });

    connect(exportConfigBtn, &ExportButton::exportRequested, [scene, this, booksimDir]() {
        QString defaultFilePath = QDir(booksimDir).filePath("anynet_config.json");
        QString fileName = QFileDialog::getSaveFileName(this, "导出JSON配置", defaultFilePath, "JSON Files (*.json);;All Files (*)");
        if (!fileName.isEmpty()) {
            scene->exportJSONConfig(fileName);
            QMessageBox::information(this, "导出成功", "JSON配置已导出到: " + fileName);
        }
    });

    connect(globalConfigBtn, &ExportButton::exportRequested, [scene, this]() {
        RouterGlobalConfigDialog dialog(this);
        dialog.setConfig(scene->getGlobalConfig());
        if (dialog.exec() == QDialog::Accepted) {
            scene->setGlobalConfig(dialog.getConfig());
        }
    });

    connect(scene, &GraphScene::nodeConfigureRequested, [scene, this](GraphNode* node) {
        if (node && node->getType() == GraphNode::Router) {
            RouterConfigDialog dialog(node->getId(), this);
            dialog.setConfig(scene->getRouterConfig(node->getId()));
            if (dialog.exec() == QDialog::Accepted) {
                scene->setRouterConfig(node->getId(), dialog.getConfig());
            }
        }
    });

    auto centralWidget = new QWidget(this);
    centralWidget->setWindowTitle("Canvas");
    auto centerLayout = new QVBoxLayout(centralWidget);
    centerLayout->addLayout(toolBarLayout);
    centerLayout->addSpacing(15);
    centerLayout->addWidget(view);
    centerLayout->setContentsMargins(0, 0, 20, 0);
    addCentralWidget(centralWidget, true, true, 0);
}
// clang-format on
