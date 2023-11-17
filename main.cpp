#include "mainwindow.h"
#include "datebase.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.addLibraryPath("./plugins");
    MainWindow w;
    w.show();
    return a.exec();
}
