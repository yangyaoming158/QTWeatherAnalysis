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
