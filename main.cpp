#include "mainwindow.h"
#include "src/data/dbmanager.h" // 1. 引入头文件
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    // 2. 尝试初始化数据库 (这行代码会让数据库文件生成)
    DBManager::getInstance().initDB();
    MainWindow w;
    w.show();
    return a.exec();
}
