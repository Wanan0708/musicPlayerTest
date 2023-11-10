#ifndef LYRICSHOW_H
#define LYRICSHOW_H

#include <QWidget>
#include <QFile>
#include <QDebug>
#include <QDateTime>

namespace Ui {
class lyricShow;
}

class lyricShow : public QWidget
{
    Q_OBJECT

public:
    explicit lyricShow(QWidget *parent = nullptr);
    ~lyricShow();

signals:
    void beHide();

public slots:
    void slotChangeLyric();

private slots:
    void on_pushButton_hide_clicked();

private:
    Ui::lyricShow *ui;
};

#endif // LYRICSHOW_H
