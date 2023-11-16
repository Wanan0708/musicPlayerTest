#ifndef MVSHOW_H
#define MVSHOW_H

#include <QWidget>
#include <QVideoWidget>
#include <QCloseEvent>
#include <QDesktopWidget>
#include <QIcon>
#include <QDebug>
#include <QShortcut>

namespace Ui {
class mvShow;
}

class mvShow : public  QVideoWidget
{
    Q_OBJECT

public:
    explicit mvShow(QWidget *parent = nullptr);
    ~mvShow();

//    void closeEvent(QCloseEvent * event); //关闭显示主窗口
//    void changeEvent(QEvent *event); //最大化最小化
//    void keyPressEvent(QKeyEvent *event); //esc退出全屏
    bool eventFilter(QObject *obj, QEvent *event); //事件过滤器

signals:
    void stopPlaySig(); //停止
    void clickToPlayPause();

private:
    Ui::mvShow *ui;
};

#endif // MVSHOW_H
