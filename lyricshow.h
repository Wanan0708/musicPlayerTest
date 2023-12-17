#ifndef LYRICSHOW_H
#define LYRICSHOW_H

#include <QWidget>
#include <QFile>
#include <QDebug>
#include <QDateTime>
#include <QBitmap>
#include <QPainter>
#include <QTextCursor>

namespace Ui {
class lyricShow;
}

class lyricShow : public QWidget
{
    Q_OBJECT

public:
    explicit lyricShow(QWidget *parent = nullptr);
    ~lyricShow();

    bool event(QEvent *event);

signals:
    void beHide();

public slots:
    void slotChangeSongImage(QPixmap pixmap);
    void slotModifiedSongInfor(QMap<QString, QString> map);
    void slotGetLyric(QMap<int, QString> map);
    void slotChangeSingleLyric(int pos);

private slots:
    void on_pushButton_hide_clicked();

private:
    Ui::lyricShow *ui;
    QMap<int, QString> lyricMap;
    QTextCursor cursor;
};

#endif // LYRICSHOW_H
