#include "CanvasPage.h"
#include <ElaIconButton.h>
#include <ElaImageCard.h>
#include <ElaText.h>
#include <ElaMessageBar.h>
#include <QApplication>
#include <QClipboard>
#include <QVBoxLayout>
#include <Version.h>
#include <ElaToolBar.h>
#include <ElaGraphicsScene.h>
#include <ElaGraphicsView.h>
#include "utils/Tools.h"

// 这里写所有的内容，调用其他函数
CanvasPage::CanvasPage(QWidget* parent) : BasePage(parent) {
    setWindowTitle("Canvas");
    auto *layout = new QHBoxLayout(this);

    // 左边工具栏
    // QAction* addElaIconAction(ElaIconType::IconName icon, const QString& text);
    auto *toolbar = new ElaToolBar(this);
    toolbar->addElaIconAction(ElaIconType::CircleInfo, "Circle");
    toolbar->addElaIconAction(ElaIconType::Square, "Square");

    // 中间画布
    auto *scene = new ElaGraphicsScene(this);
    auto *view = new ElaGraphicsView(scene);

    layout->addWidget(toolbar);
    layout->addWidget(view, 1);
    setLayout(layout);

    printObjectTree(layout);
}

ElaScrollPageArea* CanvasPage::createTextArea(QString label, QString content) {
    auto* contentText = new ElaText(content, this);
    contentText->setWordWrap(false);
    contentText->setTextPixelSize(15);
}
