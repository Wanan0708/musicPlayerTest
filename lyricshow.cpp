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

void lyricShow::slotGetLyric(QMap<int, QString> map)
{
    lyricMap = map;
    ui->textEdit_lyric->clear();
    for (auto j : lyricMap.keys())
    {
        ui->textEdit_lyric->append(lyricMap.value(j));
        ui->textEdit_lyric->setAlignment(Qt::AlignCenter);  //水平居中
    }

//    ui->textEdit_lyric->moveCursor(QTextCursor::Start, QTextCursor::MoveAnchor);
    cursor = ui->textEdit_lyric->textCursor(); //移动歌词到开始
    cursor.movePosition(QTextCursor::Start);
    ui->textEdit_lyric->setTextCursor(cursor);
}

void lyricShow::slotChangeSingleLyric(int pos)
{
    int tmpLoop = 0;
    for (auto i : lyricMap.keys())
    {
        if (i == pos)
        {
            break;
        }
        tmpLoop++;
    }

    cursor.movePosition(QTextCursor::Start); //修改进度条之后也可以正常显示
    ui->textEdit_lyric->setTextCursor(cursor);

    for (int j = 0; j <= tmpLoop + 1; ++j)
    {
        cursor.movePosition(QTextCursor::Down);
        cursor.movePosition(QTextCursor::StartOfLine);
        ui->textEdit_lyric->setTextCursor(cursor);
    }

}
