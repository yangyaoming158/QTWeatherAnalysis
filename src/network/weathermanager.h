#ifndef WEATHERMANAGER_H
#define WEATHERMANAGER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray> // 新增
#include <QDebug>

class WeatherManager : public QObject
{
    Q_OBJECT
public:
    explicit WeatherManager(QObject *parent = nullptr);
    ~WeatherManager();

    // 对外接口：根据城市名或ID获取天气 (心知天气支持拼音如 "beijing" 或 ID)
    void getWeather(const QString &cityId);

signals:
    // 信号：当数据全部获取并合并完成后发送
    void weatherReceived(QString cityId, QByteArray combinedJson);
    void errorOccurred(QString errorMsg);

private:
    QNetworkAccessManager *m_manager;

    // 【修改点 1】 配置心知天气的 Key 和 Host
    // 你的私钥 (Private Key)
    const QString API_KEY = "Sn7ufFZTCm_nYO_bP";
    // 心知天气通用域名
    const QString API_HOST = "https://api.seniverse.com";

    // 临时存储变量
    QString m_currentCityId;
    QJsonObject m_tempNowData; // 暂存实况数据

    void requestNowWeather();
    void requestDailyWeather();
};

#endif // WEATHERMANAGER_H
