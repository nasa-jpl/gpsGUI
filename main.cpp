#include "gpsgui.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    GpsGui w;
    w.show();
    return a.exec();
}
