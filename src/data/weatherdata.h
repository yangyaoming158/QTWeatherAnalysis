#ifndef WEATHERDATA_H
#define WEATHERDATA_H

#include <QString>
#include <QList>

/**
 * @brief 单日天气数据结构体
 * 用于存储每一天的具体天气详情，如今天、明天、后天
 */
struct DayWeather {
    QString date;       // 日期 (例如 "2026-01-05")
    QString week;       // 星期 (例如 "星期一")
    QString type;       // 天气类型 (例如 "多云")

    int high;           // 最高温 (例如 26)
    int low;            // 最低温 (例如 15)

    QString fengxiang;  // 风向
    QString fengli;     // 风力

    int aqi;            // 空气质量指数 (仅当天有)
};

/**
 * @brief 综合天气数据结构体
 * 包含城市信息、实时天气以及未来几天的预测列表
 */
struct TodayWeather {
    QString city;       // 城市名
    QString cityId;     // 城市ID
    QString date;       // 发布日期

    QString wendu;      // 实时温度
    QString shidu;      // 湿度
    QString pm25;       // PM2.5
    QString quality;    // 空气质量 (优/良)

    QString type;       // 实时天气类型
    QString ganmao;     // 感冒指数/建议

    // 未来天气列表 (包含今天、明天、后天...)
    QList<DayWeather> forecast;
};

#endif // WEATHERDATA_H
