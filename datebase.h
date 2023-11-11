#ifndef DATEBASE_H
#define DATEBASE_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QMutex>
#include <QMutexLocker>
#include <QtDebug>
#include <QDateTime>
#include <QDir>

class Datebase  : public QObject
{
    Q_OBJECT
public:
    static Datebase * instance();

    bool createDB();
    bool updateVolumeToDB(int volume);
    bool selectVolumeFromDB(int &volume);

private:
    Datebase();
    static Datebase* m_instance;
    QSqlDatabase db;
};

#endif // DATEBASE_H
