#include "mainwindow.h"
#include "datebase.h"
#include <QTranslator>

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    QTranslator *qtTranslator = new QTranslator();
    QObject::connect(&w, &MainWindow::refreshLanguage, [&](int lan){
        if (lan == 2){
            qtTranslator->load(":/new/prefix1/trans_en_US.qm");
        }
        else if (lan == 1){
            qtTranslator->load(":/new/prefix1/trans_zh_CN.qm");
        }
        a.installTranslator(qtTranslator);
    });
    return a.exec();
}
