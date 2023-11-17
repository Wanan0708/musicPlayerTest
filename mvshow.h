#ifndef MVSHOW_H
#define MVSHOW_H

#include <QWidget>
#include <QVideoWidget>
#include <QCloseEvent>
#include <QDesktopWidget>
#include <QIcon>
#include <QDebug>
#include <QShortcut>
#include <QMouseEvent>

namespace Ui {
class mvShow;
}

class mvShow : public  QVideoWidget
{
    Q_OBJECT

public:
    explicit mvShow(QWidget *parent = nullptr);
    ~mvShow();

    bool eventFilter(QObject *obj, QEvent *event); //事件过滤器

signals:
    void stopPlaySig(); //停止
    void clickToPlayPause();

private:
    Ui::mvShow *ui;
};

#endif // MVSHOW_H
