#include <QtCore/QCoreApplication>
#include "mainserver.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    MainServer server;

    return a.exec();
}
