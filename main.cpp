#include "mainwindow.h"

#include <QApplication>
#include<QCoreApplication>
#include"Constants.h"
#include<QDir>

int main(int argc, char *argv[])
{
    QCoreApplication::setApplicationName("Pawns");
    QCoreApplication::setOrganizationName("Alamahant");
    QCoreApplication::setApplicationVersion("1.0.0");
    QDir().mkpath(PawnConstants::APPDIR);
    QDir().mkpath(PawnConstants::saveDirPath);
    QDir().mkpath(PawnConstants::saveHistoryDirPath);
    QDir().mkpath(PawnConstants::savePGNDirPath);

    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}
