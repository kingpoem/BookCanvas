#include "CanvasPage.h"
#include "component/BooksimTopologyPlaceDialog.h"
#include "component/DragButton.h"
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
        const QColor text = ElaThemeColor(eTheme->getThemeMode(), BasicText);
        QPalette pal = label->palette();
        pal.setColor(QPalette::WindowText, text);
        label->setPalette(pal);
#ifdef Q_OS_WINDOWS
        // Windows 下部分控件在透明/自绘容器里可能不跟随调色板前景色，显式设置 color 保证深色主题可读性。
        label->setStyleSheet(
            QStringLiteral("background: transparent; color: %1;").arg(text.name(QColor::HexRgb)));
#endif
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
#ifdef Q_OS_WINDOWS
        // Windows 下显式回填背景，避免透明链路导致面板保持深色底。
        w->setAttribute(Qt::WA_StyledBackground, true);
        w->setStyleSheet(QStringLiteral("background-color: %1;").arg(bg.name(QColor::HexRgb)));
#endif
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
    m_scene->setGlobalConfig(RouterGlobalConfigDialog::getDefaultConfig());
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

    auto* showBtn = new ShowButton(ElaIconType::Eye, "eye", stripInner);
    showBtn->setToolTip(tr("显示 / 隐藏非单位链路权重"));
    auto* clearCanvasBtn = new ElaIconButton(ElaIconType::TrashCan, 18, stripInner);
    clearCanvasBtn->setBorderRadius(8);
    clearCanvasBtn->setToolTip(tr("一键清除当前画布上的全部节点、链路与拓扑块"));

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
    ph.setBold(true);
    ph.setPointSize(10);
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

    stripLay->addStretch(1);

    auto* rightColumn = new QVBoxLayout();
    rightColumn->setSpacing(12);

    auto* viewFrame = new QFrame(this);
    viewFrame->setFrameShape(QFrame::StyledPanel);
    viewFrame->setObjectName(QStringLiteral("CanvasViewFrame"));
    viewFrame->setStyleSheet(QStringLiteral(
        "QFrame#CanvasViewFrame { border: 1px solid palette(mid); border-radius: 8px; background: transparent; }"));
    auto* viewFrameLay = new QVBoxLayout(viewFrame);
    viewFrameLay->setContentsMargins(6, 6, 6, 6);
    viewFrameLay->addWidget(m_graphView);

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
    connect(clearCanvasBtn, &ElaPushButton::clicked, this, [this]() {
        clearPlaceMode();
        if (m_scene) {
            m_scene->clearAllContent();
        }
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
            m_scene->updateTopologyBlockParams(block, dlg.getParams());
        }
    });

    auto centralWidget = new QWidget(this);
    centralWidget->setWindowTitle("Canvas");
    auto centerLayout = new QVBoxLayout(centralWidget);
    centerLayout->addLayout(mainRow);
    centerLayout->setContentsMargins(0, 0, 20, 0);
    addCentralWidget(centralWidget, true, true, 0);

#ifdef Q_OS_WINDOWS
    const auto applyWindowsCanvasChrome = [centralWidget, splitter]() {
        const auto mode = eTheme->getThemeMode();
        const QColor pageBg = ElaThemeColor(mode, WindowBase);
        const QColor handleBg = ElaThemeColor(mode, BasicBase);
        const QColor handleBorder = ElaThemeColor(mode, BasicBorderDeep);

        // ElaScrollPage 默认把中央页设为透明；在 Windows 下显式回填背景，避免透出深色底。
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
    applyWindowsCanvasChrome();
    connect(eTheme, &ElaTheme::themeModeChanged, this, [applyWindowsCanvasChrome](ElaThemeType::ThemeMode) {
        applyWindowsCanvasChrome();
    });
#endif
}

QMap<QString, QString> CanvasPage::globalConfig() const {
    return m_scene ? m_scene->getGlobalConfig() : QMap<QString, QString>{};
}

void CanvasPage::setGlobalConfig(const QMap<QString, QString>& config) {
    if (m_scene) {
        m_scene->setGlobalConfig(config);
    }
}

bool CanvasPage::exportTopologySilently(QString* errorMessage) {
    const QString path = BooksimPaths::topologyExportPathFromSettings();
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
    if (!m_scene) {
        if (errorMessage) {
            *errorMessage = QObject::tr("画布场景未初始化。");
        }
        return false;
    }
    m_scene->exportGraph(path);
    return true;
}

bool CanvasPage::exportConfigJsonSilently(QString* errorMessage) {
    const QString cfgPath = BooksimPaths::configExportPathFromSettings();
    const QString topoPath = BooksimPaths::topologyExportPathFromSettings();
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
    if (!m_scene) {
        if (errorMessage) {
            *errorMessage = QObject::tr("画布场景未初始化。");
        }
        return false;
    }
    const QString netField = BooksimPaths::networkFileFieldForJson(topoPath, cfgPath);
    m_scene->exportJSONConfig(cfgPath, netField);
    return true;
}

void CanvasPage::exportConfigJson() {
    if (m_scene && m_scene->topologyBlockCount() > 1) {
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
    const QString cfgPath = BooksimPaths::configExportPathFromSettings();
    QMessageBox::information(this, QObject::tr("导出成功"), QObject::tr("JSON 配置已导出到:\n%1").arg(cfgPath));
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
