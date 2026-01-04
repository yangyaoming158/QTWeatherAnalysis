#ifndef WEATHERMANAGER_H
#define WEATHERMANAGER_H

#include <QObject>

class weathermanager : public QObject
{
    Q_OBJECT
public:
    explicit weathermanager(QObject *parent = nullptr);

signals:
};

#endif // WEATHERMANAGER_H
