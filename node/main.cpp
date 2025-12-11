#include "mainwindow.h"

#include <QApplication>
#include <QIcon>
#include <QDir>
#include <QFile>
#include "noderegistry.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    
    // Set application icon (try multiple paths)
    QString iconPath = QCoreApplication::applicationDirPath() + "/icon/icon.png";
    if (!QFile::exists(iconPath)) {
        iconPath = "icon/icon.png";
    }
    if (!QFile::exists(iconPath)) {
        iconPath = "../icon/icon.png";
    }
    a.setWindowIcon(QIcon(iconPath));
    
    // Register all nodes using the centralized registry
    NodeRegistry::instance().registerNodes();
    
    MainWindow w;
    w.show();
    return a.exec();
}
