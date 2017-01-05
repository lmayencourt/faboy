#include "qtboy.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QtBoy w;
    w.show();

    return a.exec();
}
