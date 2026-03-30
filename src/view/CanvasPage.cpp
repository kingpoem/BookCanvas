#include "CanvasPage.h"
#include "component/DragButton.h"
#include "component/ExportButton.h"
#include "component/GraphNode.h"
#include "component/GraphScene.h"
#include "component/GraphView.h"
#include "component/RouterConfigDialog.h"
#include "component/RouterGlobalConfigDialog.h"
#include "component/ShowButton.h"
#include "utils/BooksimPaths.h"
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

    connect(exportBtn, &ExportButton::exportRequested, [scene, this]() {
        const QString path = BooksimPaths::topologyExportPathFromSettings();
        if (path.isEmpty()) {
            QMessageBox::warning(
                this,
                QObject::tr("导出失败"),
                QObject::tr("未配置拓扑导出路径，请在「设置」中设置 BookSim 拓扑文件路径。"));
            return;
        }
        if (!QDir().mkpath(QFileInfo(path).absolutePath())) {
            QMessageBox::warning(this, QObject::tr("导出失败"), QObject::tr("无法创建目标目录。"));
            return;
        }
        scene->exportGraph(path);
        QMessageBox::information(this, QObject::tr("导出成功"),
                                 QObject::tr("网络拓扑已导出到:\n%1").arg(path));
    });

    connect(exportConfigBtn, &ExportButton::exportRequested, [scene, this]() {
        const QString cfgPath = BooksimPaths::configExportPathFromSettings();
        const QString topoPath = BooksimPaths::topologyExportPathFromSettings();
        if (cfgPath.isEmpty()) {
            QMessageBox::warning(
                this,
                QObject::tr("导出失败"),
                QObject::tr("未配置 JSON 导出路径，请在「设置」中设置 BookSim 配置文件路径。"));
            return;
        }
        if (!QDir().mkpath(QFileInfo(cfgPath).absolutePath())) {
            QMessageBox::warning(this, QObject::tr("导出失败"), QObject::tr("无法创建目标目录。"));
            return;
        }
        const QString netField = BooksimPaths::networkFileFieldForJson(topoPath, cfgPath);
        scene->exportJSONConfig(cfgPath, netField);
        QMessageBox::information(this, QObject::tr("导出成功"),
                                 QObject::tr("JSON 配置已导出到:\n%1").arg(cfgPath));
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
