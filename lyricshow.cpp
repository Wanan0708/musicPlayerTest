#include "lyricshow.h"
#include "ui_lyricshow.h"

lyricShow::lyricShow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::lyricShow)
{
    ui->setupUi(this);
    setWindowFlag(Qt::FramelessWindowHint);
    setWindowIcon(QIcon(":/new/prefix1/image/dinosaur.png"));
    this->setWindowTitle("歌词显示");
    QPalette pal(this->palette());
    pal.setColor(QPalette::Background, Qt::white); //设置背景白色
    this->setAutoFillBackground(true);
    this->setPalette(pal);
    ui->textEdit_lyric->setReadOnly(true); //只读

    QBitmap bmp(this->size());
    bmp.fill();
    QPainter p(&bmp);
    p.setPen(Qt::NoPen);
    p.setBrush(Qt::black);
    p.drawRoundedRect(bmp.rect(),5,5);//像素为5的圆角
    setMask(bmp);
}

lyricShow::~lyricShow()
{
    delete ui;
}

void lyricShow::on_pushButton_hide_clicked()
{
    emit beHide();
}

void lyricShow::slotChangeLyric()
{
    qDebug() << Q_FUNC_INFO;
    ui->textEdit_lyric->clear();
    QFile file("./lyric.txt");
    QTextStream in(&file);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        while (!in.atEnd())
        {
            QString strLineStream = in.readLine();
            QStringList listUseLyric = strLineStream.split("]");
            QString strUseLyric = listUseLyric.last();
            ui->textEdit_lyric->append(strUseLyric);
            ui->textEdit_lyric->setAlignment(Qt::AlignCenter);  //水平居中
        }
        file.close();
    }
}

void lyricShow::slotChangeSongImage(QPixmap pixmap)
{
    if (!pixmap.isNull())
    {
        pixmap.scaled(ui->label_songImage->size(), Qt::KeepAspectRatio);
        ui->label_songImage->setScaledContents(true);
        ui->label_songImage->setPixmap(pixmap);
        qDebug() << "设置成功";
    }
}

void lyricShow::slotModifiedSongInfor(QMap<QString, QString> map)
{
    if (map.isEmpty())
    {
        return;
    }

    ui->label_singerName->setText(map.value("singerName"));
    ui->label_songName->setText(map.value("songName"));
    ui->label_albumName->setText(map.value("songAlbumName"));
}
