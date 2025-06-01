#include <QApplication>
#include <QDebug>
#include <QList>
#include <QMainWindow>

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    QMainWindow win;
    qDebug() << "win is hedden: " << win.isHidden();
    win.show();
    qDebug() << "win is hedden: " << win.isHidden();
    return app.exec();
}
