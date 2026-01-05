#include "jsonhelper.h"
#include <QDateTime> // 记得包含这个

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

        // 【修改点】结构体里叫 wendu，类型是 String
        today.wendu = now["temperature"].toString();

        // 【修改点】结构体里叫 type，我们把 "晴" 这种文字存进去
        today.type = now["text"].toString();

        // 注意：你的结构体里没有专门存 icon 代码的字段，暂时忽略 code
        // today.code = now["code"].toString();

        today.shidu = now["humidity"].toString();
        today.pm25 = "0"; // 心知免费版now接口通常不含pm25，给个默认值
        today.quality = "无";

        today.date = QDateTime::currentDateTime().toString("yyyy-MM-dd");
    }

    // 3. 解析未来天气 (daily)
    if (root.contains("daily") && root["daily"].isArray()) {
        QJsonArray dailyArr = root["daily"].toArray();

        for (int i = 0; i < dailyArr.size(); ++i) {
            QJsonObject dayObj = dailyArr[i].toObject();
            DayWeather day;

            day.date = dayObj["date"].toString();

            // 【修改点】结构体里叫 type，把 "text_day" ("多云") 存进去
            day.type = dayObj["text_day"].toString();

            // 【修改点】你的结构体没有 code_day，这行删掉/注释掉
            // day.code_day = dayObj["code_day"].toString();

            day.high = dayObj["high"].toString().toInt();
            day.low = dayObj["low"].toString().toInt();

            day.fengxiang = dayObj["wind_direction"].toString();
            day.fengli = dayObj["wind_scale"].toString();

            // 补全星期
            QDate dateObj = QDate::fromString(day.date, "yyyy-MM-dd");
            day.week = dateObj.toString("dddd");

            today.forecast.append(day);
        }
    }

    return today;
}
