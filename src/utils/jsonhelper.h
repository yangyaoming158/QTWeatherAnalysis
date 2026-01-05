#ifndef JSONHELPER_H
#define JSONHELPER_H

#include "weatherdata.h"
#include <QByteArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QDebug>

class JsonHelper
{
public:
    JsonHelper();

    // 核心函数：把字节数组解析为 WeatherData 结构体
    static TodayWeather parseWeatherJson(const QByteArray &data);
};

#endif // JSONHELPER_H
