#include "CanvasPage.h"
#include "GlobalConfigPage.h"
#include "component/BooksimTopologyPlaceDialog.h"
#include "component/GraphEdge.h"
#include "component/GraphNode.h"
#include "component/GraphScene.h"
#include "component/GraphTopologyBlock.h"
#include "component/GraphView.h"
#include "component/RouterConfigDialog.h"
#include "component/RouterGlobalConfigDialog.h"
#include "component/ShowButton.h"
#include "utils/BooksimPaths.h"
#include "utils/CanvasDebugLog.h"
#include "utils/Settings.hpp"
#include "utils/ThemedInputDialog.h"
#include <ElaDef.h>
#include <ElaGraphicsScene.h>
#include <ElaGraphicsView.h>
#include <ElaIconButton.h>
#include <ElaPushButton.h>
#include <ElaTheme.h>
#include <QApplication>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QHBoxLayout>
#include <QHideEvent>
#include <QKeyEvent>
#include <QLabel>
#include <QMessageBox>
#include <QMouseEvent>
#include <QScrollArea>
#include <QShortcut>
#include <QShowEvent>
#include <QSizePolicy>
#include <QSplitter>
#include <QSplitterHandle>
#include <QTabWidget>
#include <QVBoxLayout>
#include <Version.h>
#include <functional>

namespace {

constexpr auto kCanvasTabScopeProperty = "canvasTabScope";

void bindLabelToElaBasicText(QLabel* label) {
    if (!label) {
        return;
    }
    const auto apply = [label]() {
        const QColor text = ElaThemeColor(eTheme->getThemeMode(), BasicText);
        QPalette pal = label->palette();
        pal.setColor(QPalette::WindowText, text);
        label->setPalette(pal);
        label->setStyleSheet(
            QStringLiteral("background: transparent; color: %1;").arg(text.name(QColor::HexRgb)));
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
        w->setAttribute(Qt::WA_StyledBackground, true);
        w->setStyleSheet(QStringLiteral("background-color: %1;").arg(bg.name(QColor::HexRgb)));
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

void CanvasPage::installNetworkTabKeyboardShortcuts() {
    // Tab 前一项/后一项：Ctrl+Tab / Ctrl+Shift+Tab 由 MainWindow 的全局拦截统一处理，
    // 与点 ◀ ▶ 按钮保持同一路径；数字跳转快捷键已移除。
}

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

    auto* mainRow = new QHBoxLayout();
    mainRow->setContentsMargins(0, 0, 0, 0);
    mainRow->setSpacing(0);

    auto* leftBuildPanel = new QWidget(this);
    constexpr int kBuildSidebarMinW = 96;
    constexpr int kBuildSidebarMaxW = 520;
    /// 启动时左侧构建栏在分割条中的初始宽度（略宽于最窄值，便于 mesh 等按钮完整显示）
    constexpr int kBuildSidebarDefaultW = 130;
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

    auto* showBtn = new ShowButton(ElaIconType::Eye, "eye", stripInner);
    showBtn->setToolTip(tr("显示 / 隐藏非单位链路权重"));
    auto* clearCanvasBtn = new ElaIconButton(ElaIconType::TrashCan, 18, stripInner);
    clearCanvasBtn->setBorderRadius(8);
    clearCanvasBtn->setToolTip(tr("一键清除当前画布上的全部节点、链路与拓扑块"));
    auto* smartRenumberBtn = new ElaIconButton(ElaIconType::WandMagicSparkles, 18, stripInner);
    smartRenumberBtn->setBorderRadius(8);
    smartRenumberBtn->setToolTip(tr("将路由器与终端分别从 0 开始自动连续编号"));
    auto* pruneUnconnectedBtn = new ElaIconButton(ElaIconType::LinkSlash, 18, stripInner);
    pruneUnconnectedBtn->setBorderRadius(8);
    pruneUnconnectedBtn->setToolTip(tr("删除没有任何连线的路由器与终端"));
    auto* importNetworkBtn = new ElaIconButton(ElaIconType::NetworkWired, 18, stripInner);
    importNetworkBtn->setBorderRadius(8);
    importNetworkBtn->setToolTip(tr("选择拓扑网络文件，并在画布上恢复网络"));

    auto* leftCol = new QVBoxLayout(leftBuildPanel);
    leftCol->setContentsMargins(0, 0, 0, 0);
    leftCol->addWidget(stripScroll, 1);

    auto* actionsTitle = new QLabel(tr("BookSim 操作"), stripInner);
    actionsTitle->setForegroundRole(QPalette::WindowText);
    bindLabelToElaBasicText(actionsTitle);
    QFont at = actionsTitle->font();
    at.setBold(true);
    at.setPointSize(10);
    actionsTitle->setFont(at);
    stripLay->addWidget(actionsTitle);
    stripLay->addSpacing(4);
    stripLay->addWidget(createButtonWithLabel(showBtn, tr("链路权重"), stripInner));
    stripLay->addWidget(createButtonWithLabel(clearCanvasBtn, tr("清空画布"), stripInner));
    stripLay->addWidget(createButtonWithLabel(smartRenumberBtn, tr("智能重编号"), stripInner));
    stripLay->addWidget(createButtonWithLabel(pruneUnconnectedBtn, tr("清理孤立节点"), stripInner));
    stripLay->addWidget(createButtonWithLabel(importNetworkBtn, tr("恢复网络"), stripInner));

    auto* placeHint = new QLabel(tr("点击放置"), stripInner);
    placeHint->setForegroundRole(QPalette::WindowText);
    bindLabelToElaBasicText(placeHint);
    QFont ph = placeHint->font();
    ph.setBold(true);
    ph.setPointSize(10);
    placeHint->setFont(ph);
    placeHint->setWordWrap(true);
    stripLay->addWidget(placeHint);

    m_placeTermPick = new ElaIconButton(ElaIconType::Microchip, 18, stripInner);
    m_placeRouterPick = new ElaIconButton(ElaIconType::NetworkWired, 18, stripInner);
    m_placeTermPick->setCheckable(true);
    m_placeRouterPick->setCheckable(true);
    m_placeTermPick->setBorderRadius(8);
    m_placeRouterPick->setBorderRadius(8);
    m_placeTermPick->setToolTip(tr("在画布空白处单击放置终端（Node）\n快捷键 N · Esc 取消"));
    m_placeRouterPick->setToolTip(tr("在画布空白处单击放置路由器\n快捷键 R · Esc 取消"));

    stripLay->addWidget(createButtonWithLabel(m_placeTermPick, tr("终端"), stripInner));
    stripLay->addWidget(createButtonWithLabel(m_placeRouterPick, tr("路由器"), stripInner));

    auto* chipletTitle = new QLabel(tr("芯粒"), stripInner);
    chipletTitle->setForegroundRole(QPalette::WindowText);
    bindLabelToElaBasicText(chipletTitle);
    QFont chf = chipletTitle->font();
    chf.setBold(true);
    chf.setPointSize(10);
    chipletTitle->setFont(chf);
    chipletTitle->setWordWrap(true);
    stripLay->addWidget(chipletTitle);
    stripLay->addSpacing(4);

    auto* chipletGroupBtn = new ElaIconButton(ElaIconType::ObjectGroup, 18, stripInner);
    chipletGroupBtn->setBorderRadius(8);
    chipletGroupBtn->setToolTip(
        tr("将当前选中的终端或路由器归为一个芯粒：画布上会显示虚线围成的组框。\n"
           "会自动将「全局配置」拓扑切换为 chiplet_mesh；右键芯粒组框可配置参数，或重命名/删除。"));
    stripLay->addWidget(createButtonWithLabel(chipletGroupBtn, tr("创建芯粒"), stripInner));

    connect(chipletGroupBtn, &ElaIconButton::clicked, this, [this]() {
        clearPlaceMode();
        ensureChipletMeshGlobalConfig();
        if (m_scene) {
            m_scene->createChipletFromSelection(this);
        }
    });

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

    stripLay->addStretch(1);

    auto* exportPreviewTitle = new QLabel(tr("导出内容预览"), stripInner);
    exportPreviewTitle->setForegroundRole(QPalette::WindowText);
    bindLabelToElaBasicText(exportPreviewTitle);
    QFont ept = exportPreviewTitle->font();
    ept.setBold(true);
    ept.setPointSize(10);
    exportPreviewTitle->setFont(ept);
    exportPreviewTitle->setWordWrap(true);
    stripLay->addWidget(exportPreviewTitle);
    stripLay->addSpacing(4);

    auto* viewConfigJsonBtn = new ElaIconButton(ElaIconType::FileCode, 18, stripInner);
    viewConfigJsonBtn->setBorderRadius(8);
    viewConfigJsonBtn->setToolTip(
        tr("查看当前网络 Tab 将导出到 BookSim 的 JSON 配置全文（与「导出配置」写入内容一致，"
 "随 Tab 切换变化；未导出到磁盘时也可预览）。"));
    auto* viewTopologyBtn = new ElaIconButton(ElaIconType::ShareNodes, 18, stripInner);
    viewTopologyBtn->setBorderRadius(8);
    viewTopologyBtn->setToolTip(
        tr("查看当前网络 Tab 将导出的 anynet 拓扑文本（与「导出拓扑」写入内容一致，随 Tab 切换变化）。"));
    stripLay->addWidget(createButtonWithLabel(viewConfigJsonBtn, tr("配置 JSON"), stripInner));
    stripLay->addWidget(createButtonWithLabel(viewTopologyBtn, tr("拓扑文件"), stripInner));

    connect(viewConfigJsonBtn, &ElaIconButton::clicked, this, [this]() {
        previewCurrentNetworkConfigJson();
    });
    connect(viewTopologyBtn, &ElaIconButton::clicked, this, [this]() {
        previewCurrentNetworkTopologyFile();
    });

    auto* rightColumn = new QVBoxLayout();
    rightColumn->setSpacing(12);

    auto* viewFrame = new QFrame(this);
    viewFrame->setFrameShape(QFrame::StyledPanel);
    viewFrame->setObjectName(QStringLiteral("CanvasViewFrame"));
    viewFrame->setStyleSheet(QStringLiteral(
        "QFrame#CanvasViewFrame { border: 1px solid palette(mid); border-radius: 8px; background: transparent; }"));
    auto* viewFrameLay = new QVBoxLayout(viewFrame);
    viewFrameLay->setContentsMargins(6, 6, 6, 6);

    m_canvasTabs = new QTabWidget(viewFrame);
    m_canvasTabs->setTabsClosable(true);
    m_canvasTabs->setMovable(true);
    m_canvasTabs->setDocumentMode(true);

    auto* tabCorner = new QWidget(m_canvasTabs);
    auto* tabCornerLay = new QHBoxLayout(tabCorner);
    tabCornerLay->setContentsMargins(0, 0, 4, 0);
    tabCornerLay->setSpacing(2);

    m_tabPrevBtn = new ElaIconButton(ElaIconType::ChevronLeft, 18, tabCorner);
    m_tabNextBtn = new ElaIconButton(ElaIconType::ChevronRight, 18, tabCorner);
    m_tabPrevBtn->setFixedSize(28, 28);
    m_tabNextBtn->setFixedSize(28, 28);
    m_tabPrevBtn->setToolTip(tr("上一个网络 Tab（循环）\n快捷键：Ctrl+Shift+Tab（等同点此按钮）"));
    m_tabNextBtn->setToolTip(tr("下一个网络 Tab（循环）\n快捷键：Ctrl+Tab（等同点此按钮）"));

    auto* addTabBtn = new ElaPushButton(QStringLiteral("+"), tabCorner);
    addTabBtn->setFixedWidth(28);
    addTabBtn->setFixedHeight(28);
    addTabBtn->setToolTip(tr("新建网络 Tab"));
    addTabBtn->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    tabCornerLay->addWidget(m_tabPrevBtn);
    tabCornerLay->addWidget(m_tabNextBtn);
    tabCornerLay->addWidget(addTabBtn);
    m_canvasTabs->setCornerWidget(tabCorner, Qt::TopRightCorner);

    connect(m_tabPrevBtn, &ElaIconButton::clicked, this, [this]() {
        activateAdjacentCanvasTab(true);
    });
    connect(m_tabNextBtn, &ElaIconButton::clicked, this, [this]() {
        activateAdjacentCanvasTab(false);
    });

    viewFrameLay->addWidget(m_canvasTabs);
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

    connect(addTabBtn, &ElaPushButton::clicked, this, [this]() {
        createCanvasTab();
    });
    connect(m_canvasTabs, &QTabWidget::tabCloseRequested, this, [this](int index) {
        closeCanvasTab(index);
    });
    connect(m_canvasTabs, &QTabWidget::currentChanged, this, [this](int) {
        clearPlaceMode();
        refreshCurrentCanvasContext();
    });
    connect(m_canvasTabs, &QTabWidget::tabBarDoubleClicked, this, [this](int index) {
        if (!m_canvasTabs || index < 0 || index >= m_canvasTabs->count()) {
            return;
        }
        QWidget* page = m_canvasTabs->widget(index);
        if (!page) {
            return;
        }
        bool ok = false;
        const QString currentName = m_canvasTabs->tabText(index);
        const QString newName = BookCanvasUi::promptLineText(this,
                                                             tr("重命名网络"),
                                                             tr("网络名称"),
                                                             currentName,
                                                             &ok)
                                    .trimmed();
        if (!ok) {
            return;
        }
        if (newName.isEmpty()) {
            QMessageBox::warning(this, tr("重命名失败"), tr("网络名称不能为空。"));
            return;
        }
        m_canvasTabs->setTabText(index, newName);
        const QString scopeToken = page->property(kCanvasTabScopeProperty).toString();
        if (!scopeToken.isEmpty()) {
            savePersistedTabName(scopeToken, newName);
        }
    });

    installNetworkTabKeyboardShortcuts();
    createCanvasTab();

    connect(showBtn, &ShowButton::toggled, this, [this](bool visible) {
        if (auto* scene = currentScene()) {
            scene->setAllEdgeWeightsVisible(visible);
        }
    });
    connect(clearCanvasBtn, &ElaPushButton::clicked, this, [this]() {
        clearPlaceMode();
        if (m_scene) {
            m_scene->clearAllContent();
        }
    });
    connect(smartRenumberBtn, &ElaPushButton::clicked, this, [this]() {
        if (!m_scene) {
            return;
        }
        if (!m_scene->renumberAllNodesFromZero()) {
            QMessageBox::warning(this, tr("智能重编号失败"), tr("重编号过程中出现编号冲突，请重试。"));
        }
    });
    connect(pruneUnconnectedBtn, &ElaPushButton::clicked, this, [this]() {
        if (!m_scene) {
            return;
        }
        m_scene->removeUnconnectedNodes();
    });
    connect(importNetworkBtn, &ElaPushButton::clicked, this, [this]() {
        const QString filePath
            = QFileDialog::getOpenFileName(this,
                                           tr("选择拓扑网络文件"),
                                           QDir::homePath(),
                                           tr("所有文件 (*)"));
        if (filePath.isEmpty()) {
            return;
        }
        if (!m_scene) {
            QMessageBox::warning(this, tr("恢复失败"), tr("画布场景未初始化。"));
            return;
        }
        clearPlaceMode();
        QString errorMessage;
        if (!m_scene->importGraph(filePath, &errorMessage)) {
            QMessageBox::warning(this,
                                 tr("恢复失败"),
                                 errorMessage.isEmpty() ? tr("无法从文件恢复网络。") : errorMessage);
            return;
        }
        QMessageBox::information(this, tr("恢复成功"), tr("已从文件恢复网络:\n%1").arg(filePath));
    });

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
    auto* undoShortcut
        = new QShortcut(QKeySequence::Undo, this, nullptr, nullptr, Qt::WindowShortcut);
    connect(undoShortcut, &QShortcut::activated, this, [this]() {
        if (m_scene) {
            m_scene->undo();
        }
    });
    auto* redoShortcut
        = new QShortcut(QKeySequence::Redo, this, nullptr, nullptr, Qt::WindowShortcut);
    connect(redoShortcut, &QShortcut::activated, this, [this]() {
        if (m_scene) {
            m_scene->redo();
        }
    });
#ifdef Q_OS_MACOS
    auto* redoShortcutMacCommandY
        = new QShortcut(QKeySequence(Qt::META | Qt::Key_Y), this, nullptr, nullptr, Qt::WindowShortcut);
    connect(redoShortcutMacCommandY, &QShortcut::activated, this, [this]() {
        if (m_scene) {
            m_scene->redo();
        }
    });
#endif
    auto* newTabShortcutCtrlT
        = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_T), this, nullptr, nullptr, Qt::WindowShortcut);
    connect(newTabShortcutCtrlT, &QShortcut::activated, this, [this]() {
        createCanvasTab();
    });
#ifdef Q_OS_MACOS
    auto* newTabShortcutCommandT
        = new QShortcut(QKeySequence(Qt::META | Qt::Key_T), this, nullptr, nullptr, Qt::WindowShortcut);
    connect(newTabShortcutCommandT, &QShortcut::activated, this, [this]() {
        createCanvasTab();
    });
#endif

    auto bindCanvasShortcut = [this](const QKeySequence& seq, const std::function<void()>& fn) {
        auto* sc = new QShortcut(seq, this, nullptr, nullptr, Qt::WindowShortcut);
        connect(sc, &QShortcut::activated, this, [this, fn]() {
            if (m_graphView) {
                fn();
            }
        });
    };
    bindCanvasShortcut(QKeySequence(Qt::Key_Left), [this]() {
        m_graphView->panViewportBy(-56.0, 0.0);
    });
    bindCanvasShortcut(QKeySequence(Qt::Key_Right), [this]() {
        m_graphView->panViewportBy(56.0, 0.0);
    });
    bindCanvasShortcut(QKeySequence(Qt::Key_Up), [this]() {
        m_graphView->panViewportBy(0.0, -56.0);
    });
    bindCanvasShortcut(QKeySequence(Qt::Key_Down), [this]() {
        m_graphView->panViewportBy(0.0, 56.0);
    });
    bindCanvasShortcut(QKeySequence::ZoomIn, [this]() {
        m_graphView->zoomInStep();
    });
    bindCanvasShortcut(QKeySequence::ZoomOut, [this]() {
        m_graphView->zoomOutStep();
    });
    bindCanvasShortcut(QKeySequence(Qt::CTRL | Qt::Key_0), [this]() {
        m_graphView->resetZoomToDefault();
    });
#ifdef Q_OS_MACOS
    bindCanvasShortcut(QKeySequence(Qt::META | Qt::Key_0), [this]() {
        m_graphView->resetZoomToDefault();
    });
#endif

    auto centralWidget = new QWidget(this);
    centralWidget->setWindowTitle("Canvas");
    auto centerLayout = new QVBoxLayout(centralWidget);
    centerLayout->addLayout(mainRow);
    centerLayout->setContentsMargins(0, 0, 20, 0);
    addCentralWidget(centralWidget, true, true, 0);

    const auto applyCanvasChrome = [centralWidget, splitter]() {
        const auto mode = eTheme->getThemeMode();
        const QColor pageBg = ElaThemeColor(mode, WindowBase);
        const QColor handleBg = ElaThemeColor(mode, BasicBase);
        const QColor handleBorder = ElaThemeColor(mode, BasicBorderDeep);

        // ElaScrollPage 默认把中央页设为透明；显式回填背景，避免浅色主题下透出深色底。
        centralWidget->setAttribute(Qt::WA_StyledBackground, true);
        centralWidget->setStyleSheet(
            QStringLiteral("#ElaScrollPage_CentralPage { background-color: %1; }")
                .arg(pageBg.name(QColor::HexRgb)));
        if (QWidget* outerVp = centralWidget->parentWidget()) {
            outerVp->setAutoFillBackground(true);
            QPalette op = outerVp->palette();
            op.setColor(QPalette::Window, pageBg);
            outerVp->setPalette(op);
        }

        splitter->setStyleSheet(
            QStringLiteral(
                "QSplitter::handle:horizontal {"
                "  background: %1;"
                "  border-left: 1px solid %2;"
                "  border-right: 1px solid %2;"
                "}"
                "QSplitter::handle:horizontal:hover {"
                "  background: %3;"
                "}")
                .arg(handleBg.name(QColor::HexRgb),
                     handleBorder.name(QColor::HexRgb),
                     handleBg.lighter(105).name(QColor::HexRgb)));
    };
    applyCanvasChrome();
    connect(eTheme, &ElaTheme::themeModeChanged, this, [applyCanvasChrome](ElaThemeType::ThemeMode) {
        applyCanvasChrome();
    });
}

QMap<QString, QString> CanvasPage::globalConfig() const {
    return currentScene() ? currentScene()->getGlobalConfig() : QMap<QString, QString>{};
}

void CanvasPage::setGlobalConfig(const QMap<QString, QString>& config) {
    if (auto* scene = currentScene()) {
        scene->setGlobalConfig(config);
    }
}

QString CanvasPage::currentTopologyExportPath() const {
    return BooksimPaths::scopedExportPath(BooksimPaths::topologyExportPathFromSettings(),
                                          currentTabScopeToken());
}

QString CanvasPage::currentConfigExportPath() const {
    return BooksimPaths::scopedExportPath(BooksimPaths::configExportPathFromSettings(),
                                          currentTabScopeToken());
}

QMap<QString, QString> CanvasPage::mergedBooksimConfigForSimulationRecord() const {
    auto* scene = currentScene();
    if (!scene) {
        return {};
    }
    const QString cfgPath = currentConfigExportPath();
    const QString topoPath = currentTopologyExportPath();
    const QString netField = BooksimPaths::networkFileFieldForJson(topoPath, cfgPath);
    return scene->mergedGlobalConfigForExport(netField);
}

void CanvasPage::previewCurrentNetworkConfigJson() {
    auto* scene = currentScene();
    if (!scene) {
        BookCanvasUi::alertWarning(this, tr("预览"), tr("画布未就绪。"));
        return;
    }
    const QString cfgPath = currentConfigExportPath();
    const QString topoPath = currentTopologyExportPath();
    const QString netField = BooksimPaths::networkFileFieldForJson(topoPath, cfgPath);
    const QString body = scene->exportJSONConfigText(netField);

    QString title = tr("BookSim 配置 JSON");
    if (!cfgPath.isEmpty()) {
        title = tr("%1 — %2").arg(title, QFileInfo(cfgPath).fileName());
    } else {
        title = tr("%1（未设置 JSON 导出路径）").arg(title);
    }
    const QString tabName = canvasTabTitle(currentCanvasTabIndex()).trimmed();
    if (!tabName.isEmpty()) {
        title = tr("[%1] %2").arg(tabName, title);
    }
    BookCanvasUi::showReadOnlyTextPreview(this, title, body);
}

void CanvasPage::previewCurrentNetworkTopologyFile() {
    auto* scene = currentScene();
    if (!scene) {
        BookCanvasUi::alertWarning(this, tr("预览"), tr("画布未就绪。"));
        return;
    }
    const QString body = scene->exportTopologyFileText();
    QString title = tr("BookSim 网络拓扑");
    const QString topoPath = currentTopologyExportPath();
    if (!topoPath.isEmpty()) {
        title = tr("%1 — %2").arg(title, QFileInfo(topoPath).fileName());
    } else {
        title = tr("%1（未设置拓扑导出路径）").arg(title);
    }
    const QString tabName = canvasTabTitle(currentCanvasTabIndex()).trimmed();
    if (!tabName.isEmpty()) {
        title = tr("[%1] %2").arg(tabName, title);
    }
    BookCanvasUi::showReadOnlyTextPreview(this, title, body);
}

int CanvasPage::canvasTabCount() const {
    return m_canvasTabs ? m_canvasTabs->count() : 0;
}

int CanvasPage::currentCanvasTabIndex() const {
    return m_canvasTabs ? m_canvasTabs->currentIndex() : -1;
}

QString CanvasPage::canvasTabTitle(int zeroBasedIndex) const {
    if (!m_canvasTabs || zeroBasedIndex < 0 || zeroBasedIndex >= m_canvasTabs->count()) {
        return {};
    }
    return m_canvasTabs->tabText(zeroBasedIndex);
}

bool CanvasPage::exportTopologySilently(QString* errorMessage) {
    const QString path = currentTopologyExportPath();
    if (path.isEmpty()) {
        if (errorMessage) {
            *errorMessage = QObject::tr("未配置拓扑导出路径，请在「设置」中设置 BookSim 拓扑文件路径。");
        }
        return false;
    }
    if (!QDir().mkpath(QFileInfo(path).absolutePath())) {
        if (errorMessage) {
            *errorMessage = QObject::tr("无法创建拓扑目标目录。");
        }
        return false;
    }
    auto* scene = currentScene();
    if (!scene) {
        if (errorMessage) {
            *errorMessage = QObject::tr("画布场景未初始化。");
        }
        return false;
    }
    scene->exportGraph(path);
    return true;
}

bool CanvasPage::exportConfigJsonSilently(QString* errorMessage) {
    const QString cfgPath = currentConfigExportPath();
    const QString topoPath = currentTopologyExportPath();
    if (cfgPath.isEmpty()) {
        if (errorMessage) {
            *errorMessage = QObject::tr("未配置 JSON 导出路径，请在「设置」中设置 BookSim 配置文件路径。");
        }
        return false;
    }
    if (!QDir().mkpath(QFileInfo(cfgPath).absolutePath())) {
        if (errorMessage) {
            *errorMessage = QObject::tr("无法创建 JSON 目标目录。");
        }
        return false;
    }
    auto* scene = currentScene();
    if (!scene) {
        if (errorMessage) {
            *errorMessage = QObject::tr("画布场景未初始化。");
        }
        return false;
    }
    const QString netField = BooksimPaths::networkFileFieldForJson(topoPath, cfgPath);
    scene->exportJSONConfig(cfgPath, netField);
    return true;
}

void CanvasPage::exportConfigJson() {
    if (auto* scene = currentScene(); scene && scene->topologyBlockCount() > 1) {
        QMessageBox::warning(
            this,
            tr("导出提示"),
            tr("画布上存在多个 BookSim 拓扑块。仅当恰好有一个拓扑块时，「导出配置」才会把该块的 "
               "topology / k / n / c / routing_function 写入 JSON；本次将保留全局配置中的对应字段。"));
    }
    QString errorMessage;
    if (!exportConfigJsonSilently(&errorMessage)) {
        QMessageBox::warning(this, QObject::tr("导出失败"), errorMessage);
        return;
    }
    const QString cfgPath = currentConfigExportPath();
    QMessageBox::information(this, QObject::tr("导出成功"), QObject::tr("JSON 配置已导出到:\n%1").arg(cfgPath));
}

CanvasPage::~CanvasPage() = default;

void CanvasPage::setGlobalConfigSyncSource(GlobalConfigPage* page) {
    m_globalConfigSyncSource = page;
}

void CanvasPage::pullGlobalConfigIntoAllScenes() {
    if (!m_globalConfigSyncSource || !m_canvasTabs) {
        return;
    }
    const QMap<QString, QString> cfg = m_globalConfigSyncSource->collectCurrentConfig();
    for (int i = 0; i < m_canvasTabs->count(); ++i) {
        QWidget* page = m_canvasTabs->widget(i);
        if (!page) {
            continue;
        }
        if (GraphScene* sc = page->findChild<GraphScene*>()) {
            sc->setGlobalConfig(cfg);
            if (cfg.contains(QStringLiteral("chiplet_connect"))) {
                sc->setChipletMeshConnect(cfg.value(QStringLiteral("chiplet_connect")));
            }
        }
    }
}

void CanvasPage::ensureChipletMeshGlobalConfig() {
    if (!m_globalConfigSyncSource) {
        return;
    }
    m_globalConfigSyncSource->applyChipletMeshTopologyAndRouting();
    pullGlobalConfigIntoAllScenes();
}

void CanvasPage::activateAdjacentCanvasTab(bool backward) {
    if (!m_canvasTabs || m_canvasTabs->count() <= 1) {
        return;
    }
    const int n = m_canvasTabs->count();
    const int c = m_canvasTabs->currentIndex();
    m_canvasTabs->setCurrentIndex(backward ? (c - 1 + n) % n : (c + 1) % n);
}

void CanvasPage::activateCanvasTabByIndex(int zeroBasedIndex) {
    if (!m_canvasTabs || zeroBasedIndex < 0 || zeroBasedIndex >= m_canvasTabs->count()) {
        return;
    }
    m_canvasTabs->setCurrentIndex(zeroBasedIndex);
}

void CanvasPage::closeCurrentCanvasTab() {
    if (!m_canvasTabs) {
        return;
    }
    closeCanvasTab(m_canvasTabs->currentIndex());
}

QString CanvasPage::tabNameSettingKey(const QString& scopeToken) {
    return QStringLiteral("canvas/tabName/%1").arg(scopeToken);
}

QString CanvasPage::loadPersistedTabName(const QString& scopeToken, const QString& fallbackName) {
    if (scopeToken.isEmpty()) {
        return fallbackName;
    }
    const QString name = settings.value(tabNameSettingKey(scopeToken), fallbackName).toString().trimmed();
    return name.isEmpty() ? fallbackName : name;
}

void CanvasPage::savePersistedTabName(const QString& scopeToken, const QString& name) {
    if (scopeToken.isEmpty()) {
        return;
    }
    settings.setValue(tabNameSettingKey(scopeToken), name);
}

void CanvasPage::removePersistedTabName(const QString& scopeToken) {
    if (scopeToken.isEmpty()) {
        return;
    }
    settings.remove(tabNameSettingKey(scopeToken));
}

void CanvasPage::createCanvasTab() {
    if (!m_canvasTabs) {
        return;
    }
    const int tabId = m_nextCanvasTabId++;
    const QString scopeToken = QStringLiteral("tab_%1").arg(tabId);
    auto* tabPage = new QWidget(m_canvasTabs);
    tabPage->setProperty(kCanvasTabScopeProperty, scopeToken);

    auto* tabLayout = new QVBoxLayout(tabPage);
    tabLayout->setContentsMargins(0, 0, 0, 0);
    tabLayout->setSpacing(0);

    auto* scene = new GraphScene(tabPage);
    scene->setGlobalConfig(RouterGlobalConfigDialog::getDefaultConfig());
    auto* view = new GraphView(scene, tabPage);
    tabLayout->addWidget(view);
    view->installEventFilter(this);
    if (QWidget* vp = view->viewport()) {
        vp->installEventFilter(this);
    }

    connect(scene, &GraphScene::nodeConfigureRequested, this, [this, scene](GraphNode* node) {
        if (node && node->getType() == GraphNode::Router) {
            RouterConfigDialog dialog(node->getId(), this);
            dialog.setConfig(scene->getRouterConfig(node->getId()));
            if (dialog.exec() == QDialog::Accepted) {
                scene->setRouterConfig(node->getId(), dialog.getConfig());
            }
        }
    });
    connect(scene,
            &GraphScene::topologyBlockConfigureRequested,
            this,
            [this, scene](GraphTopologyBlock* block) {
                if (!block) {
                    return;
                }
                const BooksimTopologyParams cur = block->params();
                BooksimTopologyPlaceDialog dlg(cur.topologyId, cur.displayLabel, this);
                dlg.setParams(cur);
                if (dlg.exec() == QDialog::Accepted) {
                    scene->updateTopologyBlockParams(block, dlg.getParams());
                }
            });
    connect(scene,
            &GraphScene::chipletMeshGlobalConfigRequested,
            this,
            &CanvasPage::ensureChipletMeshGlobalConfig,
            Qt::DirectConnection);

    const QString defaultName = tr("网络 %1").arg(tabId);
    const QString tabName = loadPersistedTabName(scopeToken, defaultName);
    const int index = m_canvasTabs->addTab(tabPage, tabName);
    m_canvasTabs->setCurrentIndex(index);
    refreshCurrentCanvasContext();
}

void CanvasPage::closeCanvasTab(int index) {
    if (!m_canvasTabs || index < 0 || index >= m_canvasTabs->count()) {
        return;
    }
    if (m_canvasTabs->count() <= 1) {
        createCanvasTab();
        if (index >= m_canvasTabs->count()) {
            index = 0;
        }
    }
    QWidget* w = m_canvasTabs->widget(index);
    if (w) {
        const QString scopeToken = w->property(kCanvasTabScopeProperty).toString();
        removePersistedTabName(scopeToken);
    }
    m_canvasTabs->removeTab(index);
    if (w) {
        w->deleteLater();
    }
    refreshCurrentCanvasContext();
}

void CanvasPage::refreshCurrentCanvasContext() {
    m_scene = currentScene();
    m_graphView = currentGraphView();
    updateCanvasTabNavigateButtons();
}

void CanvasPage::updateCanvasTabNavigateButtons() {
    const bool canCycle = m_canvasTabs && m_canvasTabs->count() > 1;
    if (m_tabPrevBtn) {
        m_tabPrevBtn->setEnabled(canCycle);
    }
    if (m_tabNextBtn) {
        m_tabNextBtn->setEnabled(canCycle);
    }
}

void CanvasPage::clickCanvasTabNavigateButton(bool backward) {
    ElaIconButton* btn = backward ? m_tabPrevBtn : m_tabNextBtn;
    if (!btn || !btn->isEnabled()) {
        canvasDebugLog(QStringLiteral(
                           "[ctrl-tab-debug] clickCanvasTabNavigateButton skip backward=%1 btn=%2 enabled=%3 "
                           "tabCount=%4 current=%5")
                           .arg(backward)
                           .arg(btn ? QStringLiteral("ok") : QStringLiteral("null"))
                           .arg((btn && btn->isEnabled()) ? QStringLiteral("true")
                                                          : QStringLiteral("false"))
                           .arg(m_canvasTabs ? m_canvasTabs->count() : -1)
                           .arg(m_canvasTabs ? m_canvasTabs->currentIndex() : -1));
        return;
    }
    const int before = m_canvasTabs ? m_canvasTabs->currentIndex() : -1;
    btn->click();
    const int after = m_canvasTabs ? m_canvasTabs->currentIndex() : -1;
    canvasDebugLog(
        QStringLiteral("[ctrl-tab-debug] clickCanvasTabNavigateButton backward=%1 before=%2 after=%3 tabCount=%4")
            .arg(backward)
            .arg(before)
            .arg(after)
            .arg(m_canvasTabs ? m_canvasTabs->count() : -1));
}

void CanvasPage::triggerNetworkTabNavigateClick(bool backward) {
    canvasDebugLog(QStringLiteral("[ctrl-tab-debug] triggerNetworkTabNavigateClick backward=%1").arg(backward));
    clickCanvasTabNavigateButton(backward);
}

GraphScene* CanvasPage::currentScene() const {
    if (!m_canvasTabs) {
        return nullptr;
    }
    auto* page = m_canvasTabs->currentWidget();
    if (!page) {
        return nullptr;
    }
    return page->findChild<GraphScene*>();
}

GraphView* CanvasPage::currentGraphView() const {
    if (!m_canvasTabs) {
        return nullptr;
    }
    auto* page = m_canvasTabs->currentWidget();
    if (!page) {
        return nullptr;
    }
    return page->findChild<GraphView*>();
}

QString CanvasPage::currentTabScopeToken() const {
    if (!m_canvasTabs || !m_canvasTabs->currentWidget()) {
        return QStringLiteral("tab_0");
    }
    const QVariant scope = m_canvasTabs->currentWidget()->property(kCanvasTabScopeProperty);
    if (scope.isValid() && !scope.toString().isEmpty()) {
        return scope.toString();
    }
    return QStringLiteral("tab_0");
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
    pullGlobalConfigIntoAllScenes();
}

void CanvasPage::hideEvent(QHideEvent* event) {
    BasePage::hideEvent(event);
}

bool CanvasPage::eventFilter(QObject* watched, QEvent* event) {
    if ((event->type() == QEvent::KeyPress || event->type() == QEvent::ShortcutOverride) && m_graphView) {
        auto* keyEvent = dynamic_cast<QKeyEvent*>(event);
        if (keyEvent && this->isVisible()) {
            const int key = keyEvent->key();
            const Qt::KeyboardModifiers mods = keyEvent->modifiers();
            const bool hasZoomModifier
                = mods.testFlag(Qt::ControlModifier) || mods.testFlag(Qt::MetaModifier);
            const bool isKeyPhase = event->type() == QEvent::KeyPress;

            // 先在 ShortcutOverride 阶段“预留”按键，防止被其他控件（如方向键焦点导航）吞掉。
            if (!isKeyPhase) {
                if (key == Qt::Key_Left || key == Qt::Key_Right || key == Qt::Key_Up ||
                    key == Qt::Key_Down || (hasZoomModifier && key == Qt::Key_0) ||
                    keyEvent->matches(QKeySequence::ZoomIn) || keyEvent->matches(QKeySequence::ZoomOut) ||
                    (hasZoomModifier &&
                     (key == Qt::Key_Plus || key == Qt::Key_Equal || key == Qt::Key_Minus ||
                      key == Qt::Key_Underscore))) {
                    keyEvent->accept();
                    return true;
                }
                return BasePage::eventFilter(watched, event);
            }

            if (!hasZoomModifier) {
                if (key == Qt::Key_Left) {
                    m_graphView->panViewportBy(-56.0, 0.0);
                    return true;
                }
                if (key == Qt::Key_Right) {
                    m_graphView->panViewportBy(56.0, 0.0);
                    return true;
                }
                if (key == Qt::Key_Up) {
                    m_graphView->panViewportBy(0.0, -56.0);
                    return true;
                }
                if (key == Qt::Key_Down) {
                    m_graphView->panViewportBy(0.0, 56.0);
                    return true;
                }
            }

            if (hasZoomModifier && key == Qt::Key_0) {
                m_graphView->resetZoomToDefault();
                return true;
            }

            if (keyEvent->matches(QKeySequence::ZoomIn) ||
                (hasZoomModifier && (key == Qt::Key_Plus || key == Qt::Key_Equal))) {
                m_graphView->zoomInStep();
                return true;
            }

            if (keyEvent->matches(QKeySequence::ZoomOut) ||
                (hasZoomModifier && (key == Qt::Key_Minus || key == Qt::Key_Underscore))) {
                m_graphView->zoomOutStep();
                return true;
            }
        }
    }

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
