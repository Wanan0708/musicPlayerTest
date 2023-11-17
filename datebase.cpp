#include "datebase.h"

Datebase* Datebase::m_instance = nullptr;

Datebase::Datebase()
{

}

Datebase *Datebase::instance()
{
    if (m_instance == nullptr)
    {
        m_instance = new Datebase();
    }
    return m_instance;
}

bool Datebase::createDB()
{
    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("settings.db");     //如果本目录下没有该文件,则会在本目录下生成,否则连接该文件
    db.setUserName("admin");
    db.setPassword("admin");
    if (!db.open())
    {
        qDebug() << db.lastError().text();
        return false;
    }

    QSqlQuery query;
    QString strSql = "CREATE TABLE setting (id INTEGER PRIMARY KEY AUTOINCREMENT, "
                                            "volume INTEGER);";

    if(query.exec(strSql))
    {
        QString insertSql = "insert into setting(id,volume) VALUES('1','45');";
        query.exec(insertSql);
        return true;
    }
    qDebug() << query.lastError();

    return false;
}

bool Datebase::updateVolumeToDB(int volume)
{
    if (volume < 0 || volume > 100)
    {
        return false;
    }

    QString update_sql = "update setting set volume = :volume where id = 1;";
    QSqlQuery query(db);
    query.prepare(update_sql);
    query.bindValue(":volume", volume);
//    query.bindValue(":id", 1);
    if(query.exec())
    {
        return true;
    }
    qDebug() << query.lastError();

    return false;
}

bool Datebase::selectVolumeFromDB(int &volume)
{
    QSqlQuery query(db);
    query.exec("SELECT volume FROM setting where id = 1;");

    while(query.next())
    {
        volume = query.value(0).toInt();
    }

    if(volume >= 0 && volume <= 100)
    {
        return true;
    }
    qDebug() << __LINE__ << query.lastError();

    return false;
}
