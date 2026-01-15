#include "dbmanager.h"

DBManager::DBManager(QObject *parent) : QObject(parent)
{
    // 构造函数留空，初始化逻辑放在 initDB
}

DBManager::~DBManager()
{
    if (m_db.isOpen()) {
        m_db.close();
    }
}

DBManager &DBManager::getInstance()
{
    static DBManager instance;
    return instance;
}

bool DBManager::initDB()
{
    if (m_db.isOpen()) {
        return true;
    }

    // 1. 连接数据库
    m_db = QSqlDatabase::addDatabase("QSQLITE");
    // 数据库文件路径：在可执行文件同级目录下生成 weather.db
    QString dbPath = QCoreApplication::applicationDirPath() + "/weather.db";
    m_db.setDatabaseName(dbPath);

    if (!m_db.open()) {
        qDebug() << "Error: Failed to open database" << m_db.lastError();
        return false;
    }

    // 2. 创建缓存表
    // 字段: 城市ID(主键), 城市名, JSON内容, 最后更新时间
    QSqlQuery query;
    QString sql = "CREATE TABLE IF NOT EXISTS WeatherCache ("
                  "city_id TEXT PRIMARY KEY, "
                  "city_name TEXT, "
                  "json_data TEXT, "
                  "last_update DATETIME)";

    if (!query.exec(sql)) {
        qDebug() << "Error: Failed to create table" << query.lastError();
        return false;
    }

    // 2. 【新增】创建历史数据表
    // 关键点：PRIMARY KEY(city_id, date) —— 这就是联合主键！
    QString sqlHistory = "CREATE TABLE IF NOT EXISTS WeatherHistory ("
                         "city_id TEXT, "
                         "date TEXT, "
                         "high INTEGER, "
                         "low INTEGER, "
                         "PRIMARY KEY(city_id, date))"; // <--- 联合主键，防止重复

    if (!query.exec(sqlHistory)) {
        qDebug() << "创建历史表失败:" << query.lastError();
        return false;
    }

    qDebug() << "Database init success! Path:" << dbPath;
    return true;
}

bool DBManager::cacheWeather(const QString &cityId, const QString &cityName, const QByteArray &jsonData)
{
    if (!m_db.isOpen() && !initDB()) return false;

    QSqlQuery query;
    // 使用 REPLACE INTO: 如果 ID 存在则替换，不存在则插入
    QString sql = "INSERT OR REPLACE INTO WeatherCache (city_id, city_name, json_data, last_update) "
                  "VALUES (:id, :name, :json, :time)";

    query.prepare(sql);
    query.bindValue(":id", cityId);
    query.bindValue(":name", cityName);
    query.bindValue(":json", QString::fromUtf8(jsonData)); // 存为文本
    query.bindValue(":time", QDateTime::currentDateTime()); // 记录当前时间

    if (!query.exec()) {
        qDebug() << "Error: Failed to cache weather" << query.lastError();
        return false;
    }
    return true;
}

QByteArray DBManager::getWeatherCache(const QString &cityId)
{
    if (!m_db.isOpen() && !initDB()) return QByteArray();

    QSqlQuery query;
    query.prepare("SELECT json_data, last_update FROM WeatherCache WHERE city_id = :id");
    query.bindValue(":id", cityId);

    if (query.exec() && query.next()) {
        QDateTime lastUpdate = query.value("last_update").toDateTime();
        QDateTime now = QDateTime::currentDateTime();

        // 计算时间差 (秒)
        qint64 diff = lastUpdate.secsTo(now);

        // 如果缓存时间超过设定值 (比如1小时)，则视为过期，返回空，强制联网刷新
        if (diff > CACHE_EXPIRE_SECONDS) {
            qDebug() << "Cache expired for city:" << cityId << "Diff:" << diff << "s";
            return QByteArray();
        }

        // 没过期，返回缓存的 JSON 数据
        QString jsonStr = query.value("json_data").toString();
        qDebug() << "Hit cache for city:" << cityId;
        return jsonStr.toUtf8();
    }

    // 没查到数据
    return QByteArray();
}

// 【新增】插入逻辑
bool DBManager::insertHistoryData(const QString &cityId, const QString &date, int high, int low)
{
    if (!m_db.isOpen() && !initDB()) return false;

    QSqlQuery query;
    QString sql = "INSERT OR REPLACE INTO WeatherHistory (city_id, date, high, low) "
                  "VALUES (:cityid, :date, :high, :low)";

    query.prepare(sql);
    query.bindValue(":cityid", cityId);
    query.bindValue(":date", date);
    query.bindValue(":high", high);
    query.bindValue(":low", low);

    bool success = query.exec(); // 执行

    // 【新增调试代码】
    if (!success) {
        // 如果插入失败，打印具体原因！
        qDebug() << "❌ 插入历史失败! ID:" << cityId << " Date:" << date
                 << " Error:" << query.lastError().text();
    } else {
        // 如果成功，也打印一下，确认真的插进去了
        qDebug() << "✅ 插入成功: " << cityId << date;
    }

    return success;
}

// 【新增】查询逻辑
QList<DayWeather> DBManager::getHistoryData(const QString &cityId)
{
    QList<DayWeather> list;
    if (!m_db.isOpen() && !initDB()) return list;

    QSqlQuery query;
    // 按日期升序排列，这样画图时线是顺的
    query.prepare("SELECT date, high, low FROM WeatherHistory WHERE city_id = :id ORDER BY date ASC");
    query.bindValue(":id", cityId);

    if (query.exec()) {
        while (query.next()) {
            DayWeather day;
            day.date = query.value("date").toString();
            day.high = query.value("high").toInt();
            day.low = query.value("low").toInt();
            // 注意：历史表里没存 week 和 type，如果需要也可以加上，这里主要为了画图
            list.append(day);
        }
    }
    return list;
}

QSqlDatabase DBManager::getDatabase()
{
    return m_db;
}


// dbmanager.cpp

QList<DayWeather> DBManager::getRecentHistory(const QString &cityId)
{
    QList<DayWeather> list;
    if (!m_db.isOpen() && !initDB()) return list;

    QSqlQuery query;

    // 获取今天的日期 (yyyy-MM-dd)
    QString today = QDateTime::currentDateTime().toString("yyyy-MM-dd");

    // 【核心 SQL 语句】
    // 1. 内层查询：WHERE date < :today (只找以前的)，ORDER BY date DESC (找离今天最近的)，LIMIT 6 (只取6条)
    // 2. 外层查询：把找出来的这 6 条，重新 ORDER BY date ASC (按时间正序排列，方便画图)
    QString sql = "SELECT * FROM ("
                  "  SELECT date, high, low FROM WeatherHistory "
                  "  WHERE city_id = :cityid AND date < :today "
                  "  ORDER BY date DESC LIMIT 6"
                  ") ORDER BY date ASC";

    query.prepare(sql);
    query.bindValue(":cityid", cityId);
    query.bindValue(":today", today); // 排除今天和未来

    if (query.exec()) {
        while (query.next()) {
            DayWeather day;
            day.date = query.value("date").toString();
            day.high = query.value("high").toInt();
            day.low = query.value("low").toInt();
            list.append(day);
        }
    } else {
        qDebug() << "查询历史失败:" << query.lastError();
    }
    return list;
}

// 1. 实现获取中文名
QString DBManager::getCityName(const QString &cityId)
{
    if (!m_db.isOpen() && !initDB()) return cityId;

    QSqlQuery query;
    // 从 WeatherCache 表中查找 city_name
    query.prepare("SELECT city_name FROM WeatherCache WHERE city_id = :id");
    query.bindValue(":id", cityId);

    if (query.exec() && query.next()) {
        QString name = query.value("city_name").toString();
        if (!name.isEmpty()) return name;
    }
    return cityId; // 如果查不到（或者没缓存），就这就返回拼音
}
