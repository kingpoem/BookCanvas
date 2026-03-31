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
#include <ElaToolBar.h>
#include <QDir>
#include <QFileInfo>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QShortcut>
#include <QSizePolicy>
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
    label->setForegroundRole(QPalette::WindowText);
    label->setTextInteractionFlags(Qt::NoTextInteraction);
    QFont lf = label->font();
    lf.setPointSize(9);
    label->setFont(lf);
    layout->addWidget(label);

    container->setLayout(layout);
    return container;
}

// clang-format off
CanvasPage::CanvasPage(QWidget* parent)
    : BasePage(parent) {
    setWindowTitle("Canvas");

    auto* scene = new GraphScene(this);
    auto* view = new GraphView(scene, this);

    auto* mainRow = new QHBoxLayout();
    mainRow->setSpacing(12);

    auto* buildStrip = new QFrame(this);
    buildStrip->setFrameShape(QFrame::StyledPanel);
    buildStrip->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    buildStrip->setFixedWidth(118);
    auto* stripLay = new QVBoxLayout(buildStrip);
    stripLay->setContentsMargins(10, 12, 10, 12);
    stripLay->setSpacing(10);

    auto* buildTitle = new QLabel(tr("构建"), buildStrip);
    buildTitle->setForegroundRole(QPalette::WindowText);
    QFont tf = buildTitle->font();
    tf.setBold(true);
    tf.setPointSize(10);
    buildTitle->setFont(tf);
    stripLay->addWidget(buildTitle);
    stripLay->addSpacing(4);

    auto* terminalBtn = new DragButton(ElaIconType::Microchip, QStringLiteral("Circle"), buildStrip);
    auto* routerBtn = new DragButton(ElaIconType::NetworkWired, QStringLiteral("Router"), buildStrip);

    terminalBtn->setToolTip(
        tr("拖拽到画布：添加终端（Node）\n或开启右侧「点击放置」后在空白处单击\n快捷键 N 进入点击放置 · Esc 取消"));
    routerBtn->setToolTip(tr("拖拽到画布：添加路由器\n或开启右侧「点击放置」\n快捷键 R 进入点击放置 · Esc 取消"));

    stripLay->addWidget(createButtonWithLabel(terminalBtn, tr("终端"), buildStrip));
    stripLay->addWidget(createButtonWithLabel(routerBtn, tr("路由器"), buildStrip));

    auto* placeHint = new QLabel(tr("点击放置"), buildStrip);
    placeHint->setForegroundRole(QPalette::WindowText);
    QFont ph = placeHint->font();
    ph.setPointSize(9);
    placeHint->setFont(ph);
    placeHint->setWordWrap(true);
    stripLay->addWidget(placeHint);

    auto* placeTermPick = new ElaIconButton(ElaIconType::CrosshairsSimple, 18, buildStrip);
    auto* placeRouterPick = new ElaIconButton(ElaIconType::LocationCrosshairs, 18, buildStrip);
    placeTermPick->setCheckable(true);
    placeRouterPick->setCheckable(true);
    placeTermPick->setToolTip(tr("在画布空白处单击放置终端"));
    placeRouterPick->setToolTip(tr("在画布空白处单击放置路由器"));

    stripLay->addWidget(createButtonWithLabel(placeTermPick, tr("终端"), buildStrip));
    stripLay->addWidget(createButtonWithLabel(placeRouterPick, tr("路由器"), buildStrip));
    stripLay->addStretch(1);

    mainRow->addWidget(buildStrip);

    auto* rightColumn = new QVBoxLayout();
    rightColumn->setSpacing(12);

    auto* toolBar = new ElaToolBar();

    auto* showBtn = new ShowButton(ElaIconType::Eye, "eye", toolBar);
    auto* exportBtn = new ExportButton(ElaIconType::Download, "Export File", toolBar);
    auto* exportConfigBtn = new ExportButton(ElaIconType::Gear, "Export Config", toolBar);
    auto* globalConfigBtn = new ExportButton(ElaIconType::CarWrench, "Export Global Config", toolBar);

    globalConfigBtn->setToolTip(tr("全局路由参数（BookSim JSON）"));
    exportConfigBtn->setToolTip(tr("导出 JSON 配置文件"));
    exportBtn->setToolTip(tr("导出网络拓扑（anynet）"));
    showBtn->setToolTip(tr("显示 / 隐藏非单位链路权重"));

    auto* viewGroup = new QLabel(tr("视图"), toolBar);
    viewGroup->setForegroundRole(QPalette::WindowText);
    QFont hg = viewGroup->font();
    hg.setBold(true);
    hg.setPointSize(9);
    viewGroup->setFont(hg);
    auto* booksimGroup = new QLabel(tr("BookSim"), toolBar);
    booksimGroup->setForegroundRole(QPalette::WindowText);
    booksimGroup->setFont(hg);

    toolBar->addWidget(viewGroup);
    toolBar->addWidget(createButtonWithLabel(showBtn, tr("链路权重"), toolBar));
    toolBar->addSeparator();
    toolBar->addWidget(booksimGroup);
    toolBar->addWidget(createButtonWithLabel(globalConfigBtn, tr("全局配置"), toolBar));
    toolBar->addWidget(createButtonWithLabel(exportBtn, tr("导出拓扑"), toolBar));
    toolBar->addWidget(createButtonWithLabel(exportConfigBtn, tr("导出配置"), toolBar));
    toolBar->setFixedHeight(70);
    toolBar->setSizePolicy(toolBar->sizePolicy().horizontalPolicy(), toolBar->sizePolicy().verticalPolicy());

    auto* toolBarLayout = new QHBoxLayout();
    toolBarLayout->addWidget(toolBar);
    toolBarLayout->addStretch(1);

    auto* viewFrame = new QFrame(this);
    viewFrame->setFrameShape(QFrame::StyledPanel);
    viewFrame->setObjectName(QStringLiteral("CanvasViewFrame"));
    viewFrame->setStyleSheet(QStringLiteral(
        "QFrame#CanvasViewFrame { border: 1px solid palette(mid); border-radius: 8px; background: transparent; }"));
    auto* viewFrameLay = new QVBoxLayout(viewFrame);
    viewFrameLay->setContentsMargins(6, 6, 6, 6);
    viewFrameLay->addWidget(view);

    rightColumn->addLayout(toolBarLayout);
    rightColumn->addWidget(viewFrame, 1);

    mainRow->addLayout(rightColumn, 1);

    connect(showBtn, &ShowButton::toggled, scene, &GraphScene::setAllEdgeWeightsVisible);

    auto clearPlaceUi = [scene, placeTermPick, placeRouterPick]() {
        placeTermPick->blockSignals(true);
        placeRouterPick->blockSignals(true);
        placeTermPick->setChecked(false);
        placeRouterPick->setChecked(false);
        placeTermPick->blockSignals(false);
        placeRouterPick->blockSignals(false);
        scene->setPlaceTool(GraphScene::PlaceTool::None);
    };

    connect(placeTermPick, &ElaIconButton::toggled, this, [=](bool on) {
        if (on) {
            placeRouterPick->blockSignals(true);
            placeRouterPick->setChecked(false);
            placeRouterPick->blockSignals(false);
            scene->setPlaceTool(GraphScene::PlaceTool::Terminal);
        } else if (!placeRouterPick->isChecked()) {
            scene->setPlaceTool(GraphScene::PlaceTool::None);
        }
    });
    connect(placeRouterPick, &ElaIconButton::toggled, this, [=](bool on) {
        if (on) {
            placeTermPick->blockSignals(true);
            placeTermPick->setChecked(false);
            placeTermPick->blockSignals(false);
            scene->setPlaceTool(GraphScene::PlaceTool::Router);
        } else if (!placeTermPick->isChecked()) {
            scene->setPlaceTool(GraphScene::PlaceTool::None);
        }
    });

    auto* escShortcut = new QShortcut(QKeySequence(Qt::Key_Escape), this, nullptr, nullptr, Qt::WindowShortcut);
    connect(escShortcut, &QShortcut::activated, this, clearPlaceUi);

    auto* placeTermShortcut = new QShortcut(QKeySequence(Qt::Key_N), this, nullptr, nullptr, Qt::WindowShortcut);
    connect(placeTermShortcut, &QShortcut::activated, this, [=]() {
        placeTermPick->setChecked(true);
    });
    auto* placeRouterShortcut = new QShortcut(QKeySequence(Qt::Key_R), this, nullptr, nullptr, Qt::WindowShortcut);
    connect(placeRouterShortcut, &QShortcut::activated, this, [=]() {
        placeRouterPick->setChecked(true);
    });

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
    centerLayout->addLayout(mainRow);
    centerLayout->setContentsMargins(0, 0, 20, 0);
    addCentralWidget(centralWidget, true, true, 0);
}
// clang-format on
