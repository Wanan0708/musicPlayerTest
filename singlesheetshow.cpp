#include "singlesheetshow.h"
#include "ui_singlesheetshow.h"

SingleSheetShow::SingleSheetShow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SingleSheetShow)
{
    ui->setupUi(this);
    this->installEventFilter(this);

    ui->label_songSheetName->setWordWrap(true);
    ui->label_songSheetName->setAlignment(Qt::AlignHCenter);
}

SingleSheetShow::~SingleSheetShow()
{
    delete ui;
}

bool SingleSheetShow::eventFilter(QObject *obj, QEvent *event)
{
    qDebug() << __LINE__ << event->type();
    if (obj == this) // 你要过滤的对象
    {
        if (event->type() == QEvent::MouseButtonPress)
        {
            QMouseEvent * mouseEvent = static_cast<QMouseEvent *>(event);  // 类型转换
            if (mouseEvent->button() == Qt::LeftButton)
            {
                emit showSingleSheetSig(this->id);
            }
        }
        else if (event->type() == QEvent::LanguageChange)
        {
            ui->retranslateUi(this);
        }
        else
        {
            return false;
        }
        return true; // 注意这里一定要返回true，表示你要过滤该事件原本的实现
    }
    return QWidget::event(event);
}

void SingleSheetShow::setSheetValues(QString id, QString singerName, QString sheetName, QString imgAdd, QString createTime, QString playCount, QString trackCount)
{
    QString spath = ":/new/prefix1/image/playCounts.png";
    QPixmap picPixmap;
    picPixmap.load(spath);
    //按照比例缩放
    QPixmap TempPixmap = picPixmap.scaled(ui->label_playCountImage->height(), ui->label_playCountImage->height(),Qt::KeepAspectRatio, Qt::SmoothTransformation);
    ui->label_playCountImage->setScaledContents(true);
    ui->label_playCountImage->setPixmap(TempPixmap);

    QString strPath = ":/new/prefix1/image/songsCount.png";
    QPixmap pixmap;
    pixmap.load(strPath);
    //按照比例缩放
    QPixmap playPixmap = pixmap.scaled(ui->label_songsCountImage->height(), ui->label_songsCountImage->height(),Qt::KeepAspectRatio, Qt::SmoothTransformation);
    ui->label_songsCountImage->setScaledContents(true);
    ui->label_songsCountImage->setPixmap(playPixmap);


    qDebug() << Q_FUNC_INFO << id << singerName << sheetName;
    this->id = id;
    this->singerName = singerName;
    this->songSheetName = sheetName;
    this->imgAdd =imgAdd;
    this->createTime = createTime;
    this->playCount = playCount;
    this->trackCount = trackCount;

    QDateTime time = QDateTime::fromMSecsSinceEpoch(createTime.toLongLong()); //时间戳-毫秒级
    QString strStartTime = time.toString("yyyy-mm-dd hh:mm:ss"); //未使用

    ui->label_creator->setText(singerName);
    ui->label_playCount->setText(playCount);
    ui->label_songSheetName->setText(sheetName);
    ui->label_songsCount->setText(trackCount);
}

void SingleSheetShow::setAlbumImage(QPixmap pixmap)
{
    pixmap = pixmap.scaled(ui->label_image->width(), ui->label_image->height(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    ui->label_image->setPixmap(pixmap);
}
