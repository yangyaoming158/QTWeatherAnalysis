#ifndef DBMANAGER_H
#define DBMANAGER_H
#include <QObject>
class dbmanager : public QObject
{
    Q_OBJECT
public:
    explicit dbmanager(QObject *parent = nullptr);

signals:
};

#endif // DBMANAGER_H
