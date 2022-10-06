//QT4:#include <QtGui/QApplication>
#include <QtWidgets/QApplication>
#include "TwoServosGUI.h"
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    TwoServosGUI w;
    w.show();
    
    return a.exec();
}
