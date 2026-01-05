#include "src/ui/mainwindow.h"
#include <QApplication>
#include "src/network/weathermanager.h"
#include "src/utils/jsonhelper.h" // 引入头文件

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // 实例化网络管理对象
    WeatherManager *mgr = new WeatherManager();

    // 连接信号，接收数据并测试解析
    QObject::connect(mgr, &WeatherManager::weatherReceived, [](QString cityId, QByteArray data){

        // 1. 调用工具类解析
        TodayWeather weather = JsonHelper::parseWeatherJson(data);

        // 2. 打印 实况天气 (TodayWeather)
        qDebug() << "";
        qDebug() << "========== 解析结果 ==========";
        qDebug() << "城市名称:" << weather.city;
        qDebug() << "当前温度:" << weather.wendu << "℃";
        qDebug() << "当前天气:" << weather.type;   // 【修改】这里用 type
        qDebug() << "空气湿度:" << weather.shidu;
        qDebug() << "更新日期:" << weather.date;

        // 3. 打印 未来预报 (DayWeather List)
        qDebug() << "---------- 未来天气预报 ----------";
        if (weather.forecast.isEmpty()) {
            qDebug() << "警告：预报列表为空！";
        } else {
            for (const DayWeather &day : weather.forecast) {
                qDebug() << "日期:" << day.date << "(" << day.week << ")";
                qDebug() << "  天气:" << day.type; // 【修改】这里用 type
                qDebug() << "  高温:" << day.high << "℃";
                qDebug() << "  低温:" << day.low << "℃";
                qDebug() << "  风向:" << day.fengxiang << " 风力:" << day.fengli;
                qDebug() << "-------------------------------";
            }
        }
        qDebug() << "===============================";
    });

    // 连接错误信号，方便调试
    QObject::connect(mgr, &WeatherManager::errorOccurred, [](QString err){
        qDebug() << "发生错误:" << err;
    });

    // 发起请求 (建议使用拼音)
    qDebug() << "正在请求北京天气...";
    mgr->getWeather("beijing");

    return a.exec();
}
