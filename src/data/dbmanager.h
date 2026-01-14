#ifndef DBMANAGER_H
#define DBMANAGER_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>
#include <QDebug>
#include <QCoreApplication>
#include "weatherdata.h"

class DBManager : public QObject
{
    Q_OBJECT
public:
    // 单例模式获取实例
    static DBManager& getInstance();

    // 初始化数据库 (连接 + 建表)
    bool initDB();

    // 保存/更新天气缓存
    // 参数: 城市ID, 城市名称, JSON原始字符串
    bool cacheWeather(const QString &cityId, const QString &cityName, const QByteArray &jsonData);

    // 读取缓存
    // 参数: 城市ID
    // 返回: 缓存的JSON数据 (如果不存在或过期，返回空字节数组)
    QByteArray getWeatherCache(const QString &cityId);

    // 【新增】插入单条历史天气数据
    // 返回 true 表示插入或更新成功
    bool insertHistoryData(const QString &cityId, const QString &date, int high, int low);

    // 【新增】查询某个城市的历史趋势（按日期排序）
    // 返回结构体列表，用于画图
    QList<DayWeather> getHistoryData(const QString &cityId);

    // 【新增】获取数据库连接对象的接口
    QSqlDatabase getDatabase();

private:
    explicit DBManager(QObject *parent = nullptr);
    ~DBManager();

    // 禁止拷贝
    DBManager(const DBManager&) = delete;
    DBManager& operator=(const DBManager&) = delete;

    QSqlDatabase m_db;

    // 设置缓存过期时间 (例如 1 小时 = 3600 秒)
    const int CACHE_EXPIRE_SECONDS = 3600;


};

#endif // DBMANAGER_H
