#include "jsonhelper.h"

JsonHelper::JsonHelper() {}

TodayWeather JsonHelper::parseWeatherJson(const QByteArray &data)
{
    TodayWeather today;

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(data, &err);

    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        qDebug() << "JSON Parse Error:" << err.errorString();
        return today;
    }

    QJsonObject root = doc.object();

    // 1. 解析城市信息 (location)
    if (root.contains("location")) {
        QJsonObject loc = root["location"].toObject();
        today.city = loc["name"].toString();
        today.cityId = loc["id"].toString();
    }

    // 2. 解析实况天气 (now)
    if (root.contains("now")) {
        QJsonObject now = root["now"].toObject();
        today.wendu = now["temperature"].toString(); // 心知天气返回的是字符串
        today.text = now["text"].toString();
        today.type = now["code"].toString(); // 图标代码

        // 今天的日期暂时用系统日期，或者从 daily 列表里取
        today.date = QDateTime::currentDateTime().toString("yyyy-MM-dd");
    }

    // 3. 解析未来天气 (daily)
    if (root.contains("daily") && root["daily"].isArray()) {
        QJsonArray dailyArr = root["daily"].toArray();

        for (int i = 0; i < dailyArr.size(); ++i) {
            QJsonObject dayObj = dailyArr[i].toObject();
            DayWeather day;

            day.date = dayObj["date"].toString();
            day.text_day = dayObj["text_day"].toString();
            day.code_day = dayObj["code_day"].toString();

            // 注意：心知返回的温度是字符串，需要转 int 方便后续画图
            day.high = dayObj["high"].toString().toInt();
            day.low = dayObj["low"].toString().toInt();

            day.fengxiang = dayObj["wind_direction"].toString();
            day.fengli = dayObj["wind_scale"].toString();

            // 简单的星期计算
            QDate dateObj = QDate::fromString(day.date, "yyyy-MM-dd");
            day.week = dateObj.toString("dddd"); // "星期X"

            today.forecast.append(day);
        }
    }

    return today;
}
