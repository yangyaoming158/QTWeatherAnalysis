#include "weathermanager.h"

WeatherManager::WeatherManager(QObject *parent)
    : QObject{parent}
{
    m_manager = new QNetworkAccessManager(this);
}

WeatherManager::~WeatherManager()
{
    // 自动销毁
}

void WeatherManager::getWeather(const QString &cityId)
{
    m_currentCityId = cityId;
    requestNowWeather();
}

// --- 阶段一：获取实况天气 (Now) ---
void WeatherManager::requestNowWeather()
{
    // 心知天气实况接口
    // 参数: key, location, language=zh-Hans(简体中文), unit=c(摄氏度)
    QString urlStr = QString("%1/v3/weather/now.json?key=%2&location=%3&language=zh-Hans&unit=c")
                         .arg(API_HOST, API_KEY, m_currentCityId);

    QNetworkRequest request(QUrl(urlStr)); // 注意这里要转 QUrl
    QNetworkReply *reply = m_manager->get(request);

    connect(reply, &QNetworkReply::finished, this, [=]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray data = reply->readAll();

            QJsonDocument doc = QJsonDocument::fromJson(data);

            // 【心知天气校验逻辑】检查是否有 "results" 数组
            if (doc.isObject() && doc.object().contains("results")) {
                QJsonArray results = doc.object()["results"].toArray();
                if (!results.isEmpty()) {
                    // 暂存结果中的第一个对象 (包含 location 和 now)
                    m_tempNowData = results[0].toObject();

                    // -> 进入下一阶段：查未来天气
                    requestDailyWeather();
                } else {
                    emit errorOccurred("API Error: Empty results");
                }
            } else {
                emit errorOccurred("API Error: Invalid format");
            }
        } else {
            emit errorOccurred("Network Error (Now): " + reply->errorString());
        }
        reply->deleteLater();
    });
}

// --- 阶段二：获取逐日预报 (Daily) 并合并 ---
void WeatherManager::requestDailyWeather()
{
    // 心知天气预报接口 (免费版通常支持 3 天)
    // start=0 (从今天开始), days=3 (查3天)
    QString urlStr = QString("%1/v3/weather/daily.json?key=%2&location=%3&language=zh-Hans&unit=c&start=0&days=3")
                         .arg(API_HOST, API_KEY, m_currentCityId);

    QNetworkRequest request(QUrl(urlStr));
    QNetworkReply *reply = m_manager->get(request);

    connect(reply, &QNetworkReply::finished, this, [=]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray data = reply->readAll();
            QJsonDocument doc = QJsonDocument::fromJson(data);

            if (doc.isObject() && doc.object().contains("results")) {
                QJsonArray results = doc.object()["results"].toArray();

                if (!results.isEmpty()) {
                    QJsonObject dailyData = results[0].toObject();

                    // 【数据合并】
                    // 我们构建一个统一的 JSON 结构传给 UI 和 数据库
                    // 结构:
                    // {
                    //    "location": {城市信息},
                    //    "now": {实况温度},
                    //    "daily": [预报列表]
                    // }

                    QJsonObject finalObj;

                    // 1. 放入位置信息 (从实况数据里取)
                    if(m_tempNowData.contains("location"))
                        finalObj["location"] = m_tempNowData["location"];

                    // 2. 放入实况 (now)
                    if(m_tempNowData.contains("now"))
                        finalObj["now"] = m_tempNowData["now"];

                    // 3. 放入预报 (daily)
                    if(dailyData.contains("daily"))
                        finalObj["daily"] = dailyData["daily"];

                    // 转为字符串发送
                    QJsonDocument finalDoc(finalObj);
                    QByteArray finalBytes = finalDoc.toJson(QJsonDocument::Compact);

                    qDebug() << "Data Fetch Success. Size:" << finalBytes.size();
                    emit weatherReceived(m_currentCityId, finalBytes);

                } else {
                    emit errorOccurred("API Error: Empty daily results");
                }
            } else {
                emit errorOccurred("API Error: Invalid daily format");
            }
        } else {
            emit errorOccurred("Network Error (Daily): " + reply->errorString());
        }
        reply->deleteLater();
    });
}
