#include "lyricshow.h"
#include "ui_lyricshow.h"

lyricShow::lyricShow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::lyricShow)
{
    ui->setupUi(this);
    setWindowFlag(Qt::FramelessWindowHint);
    this->setWindowTitle("歌词显示");
    QPalette pal(this->palette());
    pal.setColor(QPalette::Background, Qt::white); //设置背景白色
    this->setAutoFillBackground(true);
    this->setPalette(pal);
    ui->textEdit_lyric->setReadOnly(true); //只读
}

lyricShow::~lyricShow()
{
    delete ui;
}

void lyricShow::on_pushButton_hide_clicked()
{
    this->hide();
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
            qDebug() << "split: " << listUseLyric;
            QString strUseLyric = listUseLyric.last();
            ui->textEdit_lyric->append(strUseLyric);
            ui->textEdit_lyric->setAlignment(Qt::AlignCenter);  //水平居中
        }
        file.close();
    }
}
