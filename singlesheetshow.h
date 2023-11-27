#ifndef SINGLESHEETSHOW_H
#define SINGLESHEETSHOW_H

#include <QWidget>
#include <QDebug>
#include <QDateTime>
#include <QMouseEvent>
#include <QPixmap>

namespace Ui {
class SingleSheetShow;
}

class SingleSheetShow : public QWidget
{
    Q_OBJECT

public:
    explicit SingleSheetShow(QWidget *parent = nullptr);
    ~SingleSheetShow();

    bool eventFilter(QObject *obj, QEvent *event);//事件过滤器

    void setSheetValues(QString id, QString singerName, QString sheetName, QString imgAdd, QString createTime, QString playCount, QString trackCount);

    void setAlbumImage(QPixmap pixmap);

signals:
    void showSingleSheetSig(QString id);

public slots:

private:
    Ui::SingleSheetShow *ui;

    QString id;
    QString singerName;
    QString songSheetName;
    QString imgAdd;
    QString createTime;
    QString playCount;
    QString trackCount;
};

#endif // SINGLESHEETSHOW_H
