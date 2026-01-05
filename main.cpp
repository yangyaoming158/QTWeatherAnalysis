#include "mainwindow.h"
//#include "src/data/dbmanager.h" // 1. 引入头文件
#include <QApplication>
#include "src/network/weathermanager.h" // 引入头文件

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    // 2. 尝试初始化数据库 (这行代码会让数据库文件生成)
    //DBManager::getInstance().initDB();
    // --- 测试代码开始 ---
    WeatherManager *mgr = new WeatherManager();

    // 连接信号看结果
    QObject::connect(mgr, &WeatherManager::weatherReceived, [](QString cityId, QByteArray data){
        qDebug() << "Success! City:" << cityId;
        qDebug() << "Data:" << data;
    });

    QObject::connect(mgr, &WeatherManager::errorOccurred, [](QString err){
        qDebug() << "Error:" << err;
    });

    // 请求北京天气
    mgr->getWeather("beijing"); // 试着用拼音
    // --- 测试代码结束 ---
    //MainWindow w;
    //w.show();
    return a.exec();
}
