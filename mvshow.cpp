#include "mvshow.h"
#include "ui_mvshow.h"

mvShow::mvShow(QWidget *parent) :
    QVideoWidget(parent),
    ui(new Ui::mvShow)
{
    ui->setupUi(this);
    setWindowIcon(QIcon(":/new/prefix1/image/dinosaur.png"));

    this->installEventFilter(this); //安装事件过滤器
}

mvShow::~mvShow()
{
    delete ui;
}

//void mvShow::closeEvent(QCloseEvent *event)
//{
//    emit stopPlaySig();
//    event->accept();
//}

////响应全屏(修改为事件过滤器)
//void mvShow::changeEvent(QEvent *event)
//{
//    qDebug() << "event->type: " << event->type() << this->windowState();
//    if(event->type()!=QEvent::WindowStateChange) return;
//    if(this->windowState() == Qt::WindowMaximized)
//    {
//        this->showFullScreen();
//    }
//    else if(this->windowState() == Qt::WindowMinimized)
//    {
//        this->showMinimized();
//    }
//}

////按下esc恢复窗口
//void mvShow::keyPressEvent(QKeyEvent *event)
//{
//   switch(event->key())
//   {
//   case Qt::Key_Escape:
//   {
//       this->showNormal();
//       break;
//   }
//   default:
//       QWidget::keyPressEvent(event);
//   }
//}


bool mvShow::eventFilter(QObject *obj, QEvent *event)
{
    qDebug() << __LINE__ << event->type();
    if (obj == this) // 你要过滤的对象
    {
        if (event->type() == QEvent::MouseButtonDblClick) //无法分辨单击双击信号
        {
            QMouseEvent * mouseEvent = static_cast<QMouseEvent *>(event);  // 类型转换
            if (mouseEvent->button() == Qt::RightButton)
            {
                if (this->windowState() == Qt::WindowFullScreen)
                {
                    this->showNormal();
                }
                else
                {
                    this->showFullScreen();
                }
            }
        }
        else if (event->type() == QEvent::MouseButtonPress)
        {
            QMouseEvent * mouseEvent = static_cast<QMouseEvent *>(event);  // 类型转换
            if (mouseEvent->button() == Qt::LeftButton)
            {
                emit clickToPlayPause();
            }
        }
        else if(event->type() == QEvent::WindowStateChange)
        {
            if(this->windowState() == Qt::WindowMaximized)
            {
                this->showFullScreen();
            }
        }
        else if(event->type() == QEvent::KeyPress)
        {
            QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);  // 类型转换
            switch(keyEvent->key())
            {
            case Qt::Key_Escape:
            {
                this->showNormal();
                break;
            }
            default:
                QWidget::keyPressEvent(keyEvent);
            }
        }
        else if(event->type() == QEvent::Close)
        {
            QCloseEvent *closeEvent = static_cast<QCloseEvent*>(event);
            emit stopPlaySig();
            closeEvent->accept();
        }
        else
        {
            return false;
        }
        return true; // 注意这里一定要返回true，表示你要过滤该事件原本的实现
    }
    return false; // 返回false表示不过滤，还按默认的来
}
