#include "ui/MainWindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    // Qt5 Windows 7 高DPI支持
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

    QApplication a(argc, argv);

    // 设置应用程序信息
    QApplication::setApplicationName("BookPdfCreator");
    QApplication::setApplicationVersion("1.0.0");
    QApplication::setOrganizationName("BookPdfCreator");

    // 创建并显示主窗口
    MainWindow w;
    w.show();

    return a.exec();
}
