#include "mainwindow.h"
#include <QApplication>
#include <RunGuard.h>
#include <QDialog>

int main(int argc, char *argv[])
{
    RunGuard guard("x_soundboard");

    QApplication a(argc, argv);

    if (!guard.tryToRun())
    {
        QMessageBox::warning(nullptr, "Failed to start", "The program is already running", QMessageBox::Ok);
        return 0;
    }

    MainWindow w;
    w.show();
    return a.exec();
}
