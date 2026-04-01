#include "CanvasPage.h"
#include "component/BooksimTopologyPlaceDialog.h"
#include "component/DragButton.h"
#include "component/ExportButton.h"
#include "component/GraphNode.h"
#include "component/GraphScene.h"
#include "component/GraphTopologyBlock.h"
#include "component/GraphView.h"
#include "component/RouterConfigDialog.h"
#include "component/RouterGlobalConfigDialog.h"
#include "component/ShowButton.h"
#include "utils/BooksimPaths.h"
#include <ElaDef.h>
#include <ElaGraphicsScene.h>
#include <ElaGraphicsView.h>
#include <ElaIconButton.h>
#include <ElaPushButton.h>
#include <ElaTheme.h>
#include <ElaToolBar.h>
#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QFrame>
#include <QHBoxLayout>
#include <QHideEvent>
#include <QLabel>
#include <QMessageBox>
#include <QMouseEvent>
#include <QScrollArea>
#include <QShortcut>
#include <QShowEvent>
#include <QSizePolicy>
#include <QSplitter>
#include <QSplitterHandle>
#include <QVBoxLayout>
#include <Version.h>

namespace {

void bindLabelToElaBasicText(QLabel* label) {
    if (!label) {
        return;
    }
    const auto apply = [label]() {
        QPalette pal = label->palette();
        pal.setColor(QPalette::WindowText, ElaThemeColor(eTheme->getThemeMode(), BasicText));
        label->setPalette(pal);
    };
    apply();
    QObject::connect(eTheme, &ElaTheme::themeModeChanged, label, [apply](ElaThemeType::ThemeMode) {
        apply();
    });
}

void bindWidgetToElaPanelChrome(QWidget* w) {
    if (!w) {
        return;
    }
    const auto apply = [w]() {
        w->setAutoFillBackground(true);
        QPalette pal = w->palette();
        const auto mode = eTheme->getThemeMode();
        const QColor bg = ElaThemeColor(mode, WindowBase);
        pal.setColor(QPalette::Window, bg);
        pal.setColor(QPalette::Base, bg);
        w->setPalette(pal);
    };
    apply();
    QObject::connect(eTheme, &ElaTheme::themeModeChanged, w, [apply](ElaThemeType::ThemeMode) {
        apply();
    });
}

bool canvasWidgetIsDescendantOf(QWidget* w, QWidget* ancestor) {
    while (w) {
        if (w == ancestor) {
            return true;
        }
        w = w->parentWidget();
    }
    return false;
}

} // namespace

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
    bindLabelToElaBasicText(label);
    layout->addWidget(label);

    container->setLayout(layout);
    return container;
}

// clang-format off
CanvasPage::CanvasPage(QWidget* parent)
    : BasePage(parent) {
    setWindowTitle("Canvas");

    m_scene = new GraphScene(this);
    m_graphView = new GraphView(m_scene, this);

    auto* mainRow = new QHBoxLayout();
    mainRow->setContentsMargins(0, 0, 0, 0);
    mainRow->setSpacing(0);

    auto* leftBuildPanel = new QWidget(this);
    constexpr int kBuildSidebarMinW = 96;
    constexpr int kBuildSidebarMaxW = 520;
    /// 启动时左侧构建栏在分割条中的初始宽度（紧凑，仍可拖宽）
    constexpr int kBuildSidebarDefaultW = 116;
    leftBuildPanel->setMinimumWidth(kBuildSidebarMinW);
    leftBuildPanel->setMaximumWidth(kBuildSidebarMaxW);
    leftBuildPanel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    m_leftBuildPanel = leftBuildPanel;
    bindWidgetToElaPanelChrome(leftBuildPanel);

    auto* stripScroll = new QScrollArea(leftBuildPanel);
    stripScroll->setWidgetResizable(true);
    stripScroll->setFrameShape(QFrame::NoFrame);
    stripScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    stripScroll->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    auto* stripInner = new QWidget();
    stripInner->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
    auto* stripLay = new QVBoxLayout(stripInner);
    stripLay->setContentsMargins(10, 12, 10, 12);
    stripLay->setSpacing(10);
    stripScroll->setWidget(stripInner);
    bindWidgetToElaPanelChrome(stripScroll);
    bindWidgetToElaPanelChrome(stripScroll->viewport());
    bindWidgetToElaPanelChrome(stripInner);

    auto* leftCol = new QVBoxLayout(leftBuildPanel);
    leftCol->setContentsMargins(0, 0, 0, 0);
    leftCol->addWidget(stripScroll, 1);

    auto* buildTitle = new QLabel(tr("拖拽放置"), stripInner);
    buildTitle->setForegroundRole(QPalette::WindowText);
    bindLabelToElaBasicText(buildTitle);
    QFont tf = buildTitle->font();
    tf.setBold(true);
    tf.setPointSize(10);
    buildTitle->setFont(tf);
    stripLay->addWidget(buildTitle);
    stripLay->addSpacing(4);

    auto* terminalBtn = new DragButton(ElaIconType::Microchip, QStringLiteral("Circle"), stripInner);
    auto* routerBtn = new DragButton(ElaIconType::NetworkWired, QStringLiteral("Router"), stripInner);

    terminalBtn->setToolTip(
        tr("拖拽到画布：添加终端（Node）\n或开启右侧「点击放置」后在空白处单击\n快捷键 N 进入点击放置 · Esc 取消"));
    routerBtn->setToolTip(tr("拖拽到画布：添加路由器\n或开启右侧「点击放置」\n快捷键 R 进入点击放置 · Esc 取消"));

    stripLay->addWidget(createButtonWithLabel(terminalBtn, tr("终端"), stripInner));
    stripLay->addWidget(createButtonWithLabel(routerBtn, tr("路由器"), stripInner));

    auto* placeHint = new QLabel(tr("点击放置"), stripInner);
    placeHint->setForegroundRole(QPalette::WindowText);
    bindLabelToElaBasicText(placeHint);
    QFont ph = placeHint->font();
    ph.setPointSize(9);
    placeHint->setFont(ph);
    placeHint->setWordWrap(true);
    stripLay->addWidget(placeHint);

    m_placeTermPick = new ElaIconButton(ElaIconType::CrosshairsSimple, 18, stripInner);
    m_placeRouterPick = new ElaIconButton(ElaIconType::LocationCrosshairs, 18, stripInner);
    m_placeTermPick->setCheckable(true);
    m_placeRouterPick->setCheckable(true);
    m_placeTermPick->setBorderRadius(8);
    m_placeRouterPick->setBorderRadius(8);
    m_placeTermPick->setToolTip(tr("在画布空白处单击放置终端"));
    m_placeRouterPick->setToolTip(tr("在画布空白处单击放置路由器"));

    stripLay->addWidget(createButtonWithLabel(m_placeTermPick, tr("终端"), stripInner));
    stripLay->addWidget(createButtonWithLabel(m_placeRouterPick, tr("路由器"), stripInner));

    auto* bookTopoTitle = new QLabel(tr("BookSim 拓扑"), stripInner);
    bookTopoTitle->setForegroundRole(QPalette::WindowText);
    bindLabelToElaBasicText(bookTopoTitle);
    QFont bt = bookTopoTitle->font();
    bt.setBold(true);
    bt.setPointSize(10);
    bookTopoTitle->setFont(bt);
    bookTopoTitle->setWordWrap(true);
    stripLay->addWidget(bookTopoTitle);
    stripLay->addSpacing(4);

    auto startBooksimTopoPlace = [this](const QString& topoId, const QString& caption) {
        BooksimTopologyPlaceDialog dlg(topoId, caption, this);
        if (dlg.exec() != QDialog::Accepted) {
            return;
        }
        m_placeTermPick->blockSignals(true);
        m_placeRouterPick->blockSignals(true);
        m_placeTermPick->setChecked(false);
        m_placeRouterPick->setChecked(false);
        m_placeTermPick->setIsSelected(false);
        m_placeRouterPick->setIsSelected(false);
        m_placeTermPick->blockSignals(false);
        m_placeRouterPick->blockSignals(false);
        m_scene->setPlaceTool(GraphScene::PlaceTool::None);
        m_scene->beginPlaceBooksimTopology(dlg.getParams());
    };

    auto addTopoButton = [stripInner, stripLay, startBooksimTopoPlace](const QString& id,
                                                                       const QString& caption) {
        auto* b = new ElaPushButton(caption, stripInner);
        b->setMinimumHeight(30);
        b->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        QObject::connect(b, &ElaPushButton::clicked, stripInner, [startBooksimTopoPlace, id, caption]() {
            startBooksimTopoPlace(id, caption);
        });
        stripLay->addWidget(b);
    };

    addTopoButton(QStringLiteral("mesh"), QStringLiteral("mesh"));
    addTopoButton(QStringLiteral("torus"), QStringLiteral("torus"));
    addTopoButton(QStringLiteral("cmesh"), QStringLiteral("cmesh"));
    addTopoButton(QStringLiteral("fly"), QStringLiteral("fly"));
    addTopoButton(QStringLiteral("qtree"), QStringLiteral("qtree"));
    addTopoButton(QStringLiteral("tree4"), QStringLiteral("tree4"));
    addTopoButton(QStringLiteral("fattree"), QStringLiteral("fattree"));
    addTopoButton(QStringLiteral("flatfly"), QStringLiteral("flatfly"));
    addTopoButton(QStringLiteral("dragonflynew"), QStringLiteral("dragonflynew"));
    addTopoButton(QStringLiteral("anynet"), QStringLiteral("anynet"));

    stripLay->addStretch(1);

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
    bindLabelToElaBasicText(viewGroup);
    QFont hg = viewGroup->font();
    hg.setBold(true);
    hg.setPointSize(9);
    viewGroup->setFont(hg);
    auto* booksimGroup = new QLabel(tr("BookSim"), toolBar);
    booksimGroup->setForegroundRole(QPalette::WindowText);
    bindLabelToElaBasicText(booksimGroup);
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
    viewFrameLay->addWidget(m_graphView);

    rightColumn->addLayout(toolBarLayout);
    rightColumn->addWidget(viewFrame, 1);

    auto* rightWrap = new QWidget(this);
    rightWrap->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    auto* rightWrapLay = new QVBoxLayout(rightWrap);
    rightWrapLay->setContentsMargins(0, 0, 0, 0);
    rightWrapLay->setSpacing(12);
    rightWrapLay->addLayout(rightColumn);

    auto* splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setChildrenCollapsible(false);
    splitter->setHandleWidth(10);
    splitter->addWidget(leftBuildPanel);
    splitter->addWidget(rightWrap);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes({kBuildSidebarDefaultW, 4000});

    mainRow->addWidget(splitter, 1);

    connect(showBtn, &ShowButton::toggled, m_scene, &GraphScene::setAllEdgeWeightsVisible);

    connect(m_placeTermPick, &ElaIconButton::toggled, this, [this](bool on) {
        if (on) {
            m_placeRouterPick->blockSignals(true);
            m_placeRouterPick->setChecked(false);
            m_placeRouterPick->setIsSelected(false);
            m_placeRouterPick->blockSignals(false);
            m_placeTermPick->setIsSelected(true);
            m_scene->setPlaceTool(GraphScene::PlaceTool::Terminal);
        } else {
            m_placeTermPick->setIsSelected(false);
            if (!m_placeRouterPick->isChecked()) {
                m_scene->setPlaceTool(GraphScene::PlaceTool::None);
            }
        }
    });
    connect(m_placeRouterPick, &ElaIconButton::toggled, this, [this](bool on) {
        if (on) {
            m_placeTermPick->blockSignals(true);
            m_placeTermPick->setChecked(false);
            m_placeTermPick->setIsSelected(false);
            m_placeTermPick->blockSignals(false);
            m_placeRouterPick->setIsSelected(true);
            m_scene->setPlaceTool(GraphScene::PlaceTool::Router);
        } else {
            m_placeRouterPick->setIsSelected(false);
            if (!m_placeTermPick->isChecked()) {
                m_scene->setPlaceTool(GraphScene::PlaceTool::None);
            }
        }
    });

    auto* escShortcut = new QShortcut(QKeySequence(Qt::Key_Escape), this, nullptr, nullptr, Qt::WindowShortcut);
    connect(escShortcut, &QShortcut::activated, this, &CanvasPage::clearPlaceMode);

    auto* placeTermShortcut = new QShortcut(QKeySequence(Qt::Key_N), this, nullptr, nullptr, Qt::WindowShortcut);
    connect(placeTermShortcut, &QShortcut::activated, this, [this]() {
        m_placeTermPick->setChecked(true);
    });
    auto* placeRouterShortcut = new QShortcut(QKeySequence(Qt::Key_R), this, nullptr, nullptr, Qt::WindowShortcut);
    connect(placeRouterShortcut, &QShortcut::activated, this, [this]() {
        m_placeRouterPick->setChecked(true);
    });

    connect(exportBtn, &ExportButton::exportRequested, [this]() {
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
        m_scene->exportGraph(path);
        QMessageBox::information(this, QObject::tr("导出成功"),
                                 QObject::tr("网络拓扑已导出到:\n%1").arg(path));
    });

    connect(exportConfigBtn, &ExportButton::exportRequested, [this]() {
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
        if (m_scene->topologyBlockCount() > 1) {
            QMessageBox::warning(
                this,
                tr("导出提示"),
                tr("画布上存在多个 BookSim 拓扑块。仅当恰好有一个拓扑块时，「导出配置」才会把该块的 "
                   "topology / k / n / c / routing_function 写入 JSON；本次将保留全局配置中的对应字段。"));
        }
        const QString netField = BooksimPaths::networkFileFieldForJson(topoPath, cfgPath);
        m_scene->exportJSONConfig(cfgPath, netField);
        QMessageBox::information(this, QObject::tr("导出成功"),
                                 QObject::tr("JSON 配置已导出到:\n%1").arg(cfgPath));
    });

    connect(globalConfigBtn, &ExportButton::exportRequested, [this]() {
        RouterGlobalConfigDialog dialog(this);
        dialog.setConfig(m_scene->getGlobalConfig());
        if (dialog.exec() == QDialog::Accepted) {
            m_scene->setGlobalConfig(dialog.getConfig());
        }
    });

    connect(m_scene, &GraphScene::nodeConfigureRequested, [this](GraphNode* node) {
        if (node && node->getType() == GraphNode::Router) {
            RouterConfigDialog dialog(node->getId(), this);
            dialog.setConfig(m_scene->getRouterConfig(node->getId()));
            if (dialog.exec() == QDialog::Accepted) {
                m_scene->setRouterConfig(node->getId(), dialog.getConfig());
            }
        }
    });

    connect(m_scene, &GraphScene::topologyBlockConfigureRequested, this, [this](GraphTopologyBlock* block) {
        if (!block) {
            return;
        }
        const BooksimTopologyParams cur = block->params();
        BooksimTopologyPlaceDialog dlg(cur.topologyId, cur.displayLabel, this);
        dlg.setParams(cur);
        if (dlg.exec() == QDialog::Accepted) {
            block->setParams(dlg.getParams());
        }
    });

    auto centralWidget = new QWidget(this);
    centralWidget->setWindowTitle("Canvas");
    auto centerLayout = new QVBoxLayout(centralWidget);
    centerLayout->addLayout(mainRow);
    centerLayout->setContentsMargins(0, 0, 20, 0);
    addCentralWidget(centralWidget, true, true, 0);
}

CanvasPage::~CanvasPage() {
    qApp->removeEventFilter(this);
}

void CanvasPage::clearPlaceMode() {
    if (!m_placeTermPick || !m_placeRouterPick || !m_scene) {
        return;
    }
    m_placeTermPick->blockSignals(true);
    m_placeRouterPick->blockSignals(true);
    m_placeTermPick->setChecked(false);
    m_placeRouterPick->setChecked(false);
    m_placeTermPick->setIsSelected(false);
    m_placeRouterPick->setIsSelected(false);
    m_placeTermPick->blockSignals(false);
    m_placeRouterPick->blockSignals(false);
    m_scene->setPlaceTool(GraphScene::PlaceTool::None);
}

void CanvasPage::showEvent(QShowEvent* event) {
    BasePage::showEvent(event);
    qApp->installEventFilter(this);
}

void CanvasPage::hideEvent(QHideEvent* event) {
    qApp->removeEventFilter(this);
    BasePage::hideEvent(event);
}

bool CanvasPage::eventFilter(QObject* watched, QEvent* event) {
    if (event->type() == QEvent::MouseButtonPress && m_scene && m_graphView && m_leftBuildPanel) {
        auto* mouseEvent = dynamic_cast<QMouseEvent*>(event);
        if (mouseEvent && mouseEvent->button() == Qt::LeftButton &&
            m_scene->placeTool() != GraphScene::PlaceTool::None) {
            auto* w = qobject_cast<QWidget*>(watched);
            if (w) {
                for (QWidget* x = w; x != nullptr; x = x->parentWidget()) {
                    if (qobject_cast<QSplitterHandle*>(x) != nullptr) {
                        return BasePage::eventFilter(watched, event);
                    }
                }
            }
            if (w && !canvasWidgetIsDescendantOf(w, m_graphView)) {
                if (!canvasWidgetIsDescendantOf(w, m_leftBuildPanel)) {
                    clearPlaceMode();
                }
            }
        }
    }
    return BasePage::eventFilter(watched, event);
}
// clang-format on
