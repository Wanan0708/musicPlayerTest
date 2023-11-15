#ifndef MVSHOW_H
#define MVSHOW_H

#include <QWidget>
#include <QVideoWidget>
#include <QCloseEvent>

namespace Ui {
class mvShow;
}

class mvShow : public  QVideoWidget
{
    Q_OBJECT

public:
    explicit mvShow(QWidget *parent = nullptr);
    ~mvShow();

    void closeEvent(QCloseEvent * event);

signals:
    void stopPlsySig();

private:
    Ui::mvShow *ui;
};

#endif // MVSHOW_H
