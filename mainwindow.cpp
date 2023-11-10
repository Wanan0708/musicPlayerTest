#include "mainwindow.h"
#include "ui_mainwindow.h"


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowFlag(Qt::FramelessWindowHint);
    setFixedSize(this->width(), this->height());

    lyricWeight = new Ui::lyricShow();

    init();
}

MainWindow::~MainWindow()
{
    delete ui;
    delete lyricWeight;
    delete player;
    delete playlist;
}

void MainWindow::init()
{
    iCurVolume = 30;
    m_offset = 0;

    player=new QMediaPlayer();
    player->setVolume(30);
    playlist = new QMediaPlaylist();
    connect(player, &QMediaPlayer::positionChanged, this, &MainWindow::slotSongSliderPosChanged);

    networkManager = new QNetworkAccessManager();
    qDebug() << __LINE__ << networkManager->supportedSchemes();
    networkRequest = new QNetworkRequest();
    connect(networkManager, &QNetworkAccessManager::finished, this, &MainWindow::databack);

    ui->horizontalSlider_volumeSlider->setMaximum(100);
    ui->horizontalSlider_volumeSlider->setValue(30);

    ui->buttonGroup->setId(ui->pushButton_search,0);
    ui->buttonGroup->setId(ui->pushButton_songSheet,1);
    ui->buttonGroup->setId(ui->pushButton_rankList,2);
    ui->buttonGroup->setId(ui->pushButton_set,3);
    ui->buttonGroup->setId(ui->pushButton_login,4);
    connect(ui->buttonGroup, QOverload<int>::of(&QButtonGroup::buttonClicked), this, &MainWindow::getWeight);

    QStringList headerList;
    headerList << "#" << "歌曲名称" << "歌手名" << "专辑名" << "时长";
    QHeaderView *hearview = new QHeaderView(Qt::Horizontal);
    model = new QStandardItemModel;
    model->setHorizontalHeaderLabels(headerList);
    hearview->setModel(model);
    ui->tableView_search->setHorizontalHeader(hearview);
    ui->tableView_search->setColumnWidth(0, 10);
    ui->tableView_search->setColumnWidth(1, 230);
    ui->tableView_search->setColumnWidth(2, 150);
    ui->tableView_search->setColumnWidth(3, 181);
    ui->tableView_search->setColumnWidth(4, 120);
    ui->tableView_search->verticalHeader()->setHidden(true);
    ui->tableView_search->setEditTriggers(QAbstractItemView::NoEditTriggers); //设置只读

    ui->tableView_search->setShowGrid(false);

    connect(&lyric, &lyricShow::beHide, this, [=](){
        this->show();
    });
    connect(this, &MainWindow::changeLyric, &lyric, &lyricShow::slotChangeLyric); //切换歌曲后歌词也切换
}

void MainWindow::getWeight(int id)
{
    qDebug() << id;
    ui->stackedWidget->setCurrentIndex(id);
}


void MainWindow::on_pushButton_quit_clicked()
{
    this->close();
}


void MainWindow::on_pushButton_minimize_clicked()
{
    this->showMinimized();
}


void MainWindow::on_lineEdit_search_returnPressed()
{
    QString str = ui->lineEdit_search->text();
    m_offset = 0;
//    str="http://music.163.com/api/search/get/web?csrf_token=hlpretag=&hlposttag=&s={"+str+"}&type=1&offset=%1&total=true&limit=13";//接入网易云API并且传入需要搜索的数据和返回的数量
    str=QString("http://music.163.com/api/search/get/web?csrf_token=hlpretag=&hlposttag=&s={%1}&type=1&offset=%2&total=true&limit=13").arg(str).arg(m_offset);
//    str = "http://mobilecdn.kugou.com/api/v3/search/song?format=json&keyword=%1&page=%2&pagesize=%3";

    networkRequest->setUrl(QUrl(str));
    networkManager->get(*networkRequest);
}

//处理数据
void MainWindow::databack(QNetworkReply *reply)
{
    QVariant nCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    qDebug() << __LINE__ << nCode << reply->errorString();
    searchInfo = reply->readAll();
    qDebug() << __LINE__ << searchInfo;
    QJsonParseError err;               //错误信息对象
    QJsonDocument json_recv = QJsonDocument::fromJson(searchInfo,&err);//将json文本转换为 json 文件对象
    if(err.error != QJsonParseError::NoError)             //判断是否符合语法
    {
        qDebug() <<"搜索歌曲Json获取格式错误"<< err.errorString();
        return;
    }
    QJsonObject totalObject = json_recv.object();
    QStringList keys = totalObject.keys();    // 列出json里所有的key
    qDebug() << __LINE__ << keys;
    if(keys.contains("result"))                 //如果有结果
    {       //在 json 文本中 {}花括号里面是QJsonObject对象, []方括号里面是QJsonArray

        QJsonObject resultObject = totalObject["result"].toObject();     //就将带 result 的内容提取后转换为对象
        QStringList resultKeys = resultObject.keys();      //保存所有key
        if(resultKeys.contains("songs"))                    //如果 key 为songs ,代表找到了歌曲
        {
            int k = 0;
            QJsonArray array = resultObject["songs"].toArray();
            for(const auto &i : qAsConst(array))                   //开始获取歌曲中的信息
            {
                QJsonObject object = i.toObject();
                musicId = object["id"].toInt();                         // 音乐id
                musicDuration = object["duration"].toInt();             // 音乐长度
                qDebug() << "musicDuration: " << musicDuration;
                musicName = object["name"].toString();                  // 音乐名
                mvId = object["mvid"].toInt();                          // mvid
                QStringList artistsKeys = object.keys();
                qDebug() << "artistsKeys: " << artistsKeys;
                if(artistsKeys.contains("artists"))                //如果result中包含了 artists
                {
                    QJsonArray artistsArray = object["artists"].toArray();   //将 artist 的内容提取后保存
                    for(const auto &j : qAsConst(artistsArray))
                    {
                        QJsonObject object2 = j.toObject();
                        singerName = object2["name"].toString();         // 歌手名
                        imgAddress = object2["img1v1Url"].toString(); //专辑图片路径
                        qDebug() << __LINE__ << object2.keys() << imgAddress;
                    }
                }
                if(artistsKeys.contains("album"))                //包含了专辑
                {
                    QJsonObject albumObjct = object["album"].toObject();
                    albumName = albumObjct["name"].toString();            // 专辑名
                    QStringList artistList = albumObjct.keys();
                    if(artistList.contains("artist"))
                    {
                        QJsonArray artistArray = albumObjct["artist"].toArray();
                        qDebug() << __LINE__ << artistList << artistArray;
                        for (const auto &j : qAsConst(artistArray))
                        {
                            QJsonObject tmpObj = j.toObject();
                            imgAddress = tmpObj["img1v1Url"].toString(); //第二种方式拿专辑图片路径
                            qDebug() << __LINE__ << "imgAddress: " << imgAddress;
                        }
                    }
                }

                qDebug() << "musicId: " << musicId;
                qDebug() << "musicName: " << musicName;
                qDebug() << "singerName: " << singerName;
                qDebug() << "musicDuration: " << musicDuration;
                qDebug() << "albumName: " << albumName;
                qDebug() << "imgAddress: " << imgAddress;

                model->setItem(k,0, new QStandardItem(QString::number(k+1)));
                model->setItem(k,1,new QStandardItem(musicName)); //音乐名称
                model->setItem(k,2,new QStandardItem(singerName)); //歌手名
                model->setItem(k,3,new QStandardItem(albumName)); //专辑名
                QString time = QString("%1:%2").arg(musicDuration/1000/60, 2, 10, QLatin1Char('0')).arg(musicDuration/1000%60, 2, 10, QLatin1Char('0'));
                model->setItem(k,4,new QStandardItem(time)); //歌曲时长
                model->setItem(k,5,new QStandardItem(QString::number(musicId))); //歌曲ID
                model->setItem(k,6,new QStandardItem(imgAddress)); //歌曲图片路径
                model->item(k,0)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
                model->item(k,1)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
                model->item(k,2)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
                model->item(k,3)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
                model->item(k,4)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
                qDebug() << "time: " << time << "k: " << k;
                k++;

                QString url;
                url=QString("https://music.163.com/song/media/outer/url?id=%0").arg(musicId);
                playlist->addMedia(QUrl(url));                     //添加返回的音乐到播放列表中
            }
            ui->tableView_search->setModel(model);
            ui->tableView_search->setColumnWidth(0, 10);
            ui->tableView_search->setColumnWidth(1, 230);
            ui->tableView_search->setColumnWidth(2, 150);
            ui->tableView_search->setColumnWidth(3, 176);
            ui->tableView_search->setColumnWidth(4, 120);
            ui->tableView_search->hideColumn(5);
            ui->tableView_search->hideColumn(6);
        }
    }
    else if(keys.contains("lrc"))
    {
        QJsonObject lrcObject = totalObject["lrc"].toObject();     //就将带 msg 的内容提取后转换为对象
        QStringList resultKeys = lrcObject.keys();      //保存所有key
        qDebug() << __LINE__ << resultKeys;
        if (resultKeys.contains("lyric"))
        {
            QString strLyric = lrcObject["lyric"].toString();
            QStringList lyricList = strLyric.split("\n");

            QFile file("./lyric.txt");
            if (file.open(QIODevice::ReadWrite | QIODevice::Text | QIODevice::Truncate))
            {
                QTextStream stream(&file);
                stream.seek(file.size());
                for (auto& i : lyricList)
                {
                    QString strLineLyric;
                    strLineLyric.append(i);
                    stream << strLineLyric << "\n";
                }
                file.close();

                emit changeLyric();
            }
        }
    }
}

void MainWindow::on_pushButton_playPause_clicked()
{
    qDebug() << "state0: " << player->state();
    if (QMediaPlayer::PlayingState == player->state())
    {
        player->pause();
    }
    else if (QMediaPlayer::PausedState == player->state() || QMediaPlayer::StoppedState == player->state())
    {
        player->play();
    }
    else
    {
        qDebug() << "state1: " << player->state();
    }
}

void MainWindow::on_tableView_search_doubleClicked(const QModelIndex &index)
{
    qDebug() << "index: " << index;
    int row=index.row();//获得当前行索引
//    int col=index.column();//获得当前列索引
    ui->tableView_search->selectRow(row); //选中列
    QModelIndex indexID =model->index(row,5);  // 第5列是musicID
    QString strMusicID = model->data(indexID).toString();//获取索引对应位置的数据转为字符串
    qDebug() << "clicked: " << strMusicID;

    player->setMedia(new QMediaPlaylist());
    playlist->clear();//清除播放列表
    QString url;
    url=QString("https://music.163.com/song/media/outer/url?id=%0").arg(strMusicID);
    playlist->addMedia(QUrl(url));                     //添加返回的音乐到播放列表中
    player->setMedia(playlist);                        //将列表设置到播放器中
    player->play();                                  //播放

    QModelIndex indexSingerName = model->index(row, 1);
    QString strSongerName = model->data(indexSingerName).toString();
    QModelIndex indexSongName = model->index(row, 2);
    QString strMusicSongName = model->data(indexSongName).toString();
    ui->label_songName->setText(strMusicSongName + " - " + strSongerName); //歌曲名+歌手

    QModelIndex indexTime = model->index(row, 4);
    QString strMusicTime = model->data(indexTime).toString();
    ui->label_totalSongTime->setText(strMusicTime); // 歌曲时长

    QModelIndex indexImg = model->index(row, 6);
    QString strMusicImg = model->data(indexImg).toString();

    QStringList timeList = strMusicTime.split(":");
    QString strMinute = timeList.at(0);
    QString strSeconds = timeList.at(1);
    musicDuration = strMinute.toInt() * 60 + strSeconds.toInt();
    qDebug() << "musicDuration: " << musicDuration;
    ui->horizontalSlider_songSlider->setMaximum(strMinute.toInt() * 60 + strSeconds.toInt()); //歌曲进度条

    GetSongLyricBySongId(strMusicID);//获取歌词

    qDebug() << __LINE__ << strMusicImg;
    networkRequest->setUrl(QUrl(strMusicImg)); //获取专辑图片
    networkManager->get(*networkRequest);
}

void MainWindow::slotSongSliderPosChanged(qint64 pos)
{
//    qDebug() << Q_FUNC_INFO;
    ui->horizontalSlider_songSlider->setValue(pos/1000);
    ui->label_curSongTime->setText(QString("%1").arg(pos/1000/60, 2, 10, QLatin1Char('0')) + ":" + QString("%2").arg(pos/1000%60, 2, 10, QLatin1Char('0')));
}

//拖动音量进度条
void MainWindow::on_horizontalSlider_volumeSlider_valueChanged(int value)
{
    qDebug() << __LINE__ << "value: " << value;
    player->setVolume(value);
    iCurVolume = value;
}

//上一曲
void MainWindow::on_pushButton_lastSong_clicked()
{
    qDebug();
    QModelIndex lastIndex = ui->tableView_search->currentIndex();
    int row = lastIndex.row();//获得当前行索引
    if (row <= 0)
    {
        return;
    }
    QModelIndex curIndex = ui->tableView_search->model()->index(--row, 0, QModelIndex());
    ui->tableView_search->setCurrentIndex(curIndex); //点击后切换行
    ui->tableView_search->selectRow(row);

    QModelIndex indexID = model->index(row,5);  // 第5列是musicID
    QString strMusicID = model->data(indexID).toString();//获取索引对应位置的数据转为字符串
    qDebug() << "clicked: " << strMusicID;

    player->setMedia(new QMediaPlaylist());
    playlist->clear();//清除播放列表
    QString url;
    url=QString("https://music.163.com/song/media/outer/url?id=%0").arg(strMusicID);
    playlist->addMedia(QUrl(url));                     //添加返回的音乐到播放列表中
    player->setMedia(playlist);                        //将列表设置到播放器中
    player->play();                                  //播放

    QModelIndex indexSingerName = model->index(row, 1);
    QString strSongerName = model->data(indexSingerName).toString();
    QModelIndex indexSongName = model->index(row, 2);
    QString strMusicSongName = model->data(indexSongName).toString();
    ui->label_songName->setText(strMusicSongName + " - " + strSongerName); //歌曲名+歌手

    QModelIndex indexTime = model->index(row, 4);
    QString strMusicTime = model->data(indexTime).toString();
    ui->label_totalSongTime->setText(strMusicTime); // 歌曲时长

    QStringList timeList = strMusicTime.split(":");
    QString strMinute = timeList.at(0);
    QString strSeconds = timeList.at(1);
    musicDuration = strMinute.toInt() * 60 + strSeconds.toInt();
    qDebug() << "musicDuration: " << musicDuration;
    ui->horizontalSlider_songSlider->setMaximum(strMinute.toInt() * 60 + strSeconds.toInt());

    GetSongLyricBySongId(strMusicID);
    emit changeLyric(); //更换歌词
}

//下一曲
void MainWindow::on_pushButton_nextSong_clicked()
{
    qDebug();
    QModelIndex lastIndex = ui->tableView_search->currentIndex();
    int row = lastIndex.row();//获得当前行索引
    if (row >= 12)
    {
        return;
    }
    QModelIndex curIndex = ui->tableView_search->model()->index(++row, 0, QModelIndex());
    ui->tableView_search->setCurrentIndex(curIndex); //点击后切换行
    ui->tableView_search->selectRow(row);

    QModelIndex indexID = model->index(row,5);  // 第5列是musicID
    QString strMusicID = model->data(indexID).toString();//获取索引对应位置的数据转为字符串
    qDebug() << "clicked: " << strMusicID;

    player->setMedia(new QMediaPlaylist());
    playlist->clear();//清除播放列表
    QString url;
    url=QString("https://music.163.com/song/media/outer/url?id=%0").arg(strMusicID);
    playlist->addMedia(QUrl(url));                     //添加返回的音乐到播放列表中
    player->setMedia(playlist);                        //将列表设置到播放器中
    player->play();                                  //播放

    QModelIndex indexSingerName = model->index(row, 1);
    QString strSongerName = model->data(indexSingerName).toString();
    QModelIndex indexSongName = model->index(row, 2);
    QString strMusicSongName = model->data(indexSongName).toString();
    ui->label_songName->setText(strMusicSongName + " - " + strSongerName); //歌曲名+歌手

    QModelIndex indexTime = model->index(row, 4);
    QString strMusicTime = model->data(indexTime).toString();
    ui->label_totalSongTime->setText(strMusicTime); // 歌曲时长

    QStringList timeList = strMusicTime.split(":");
    QString strMinute = timeList.at(0);
    QString strSeconds = timeList.at(1);
    musicDuration = strMinute.toInt() * 60 + strSeconds.toInt();
    qDebug() << "musicDuration: " << musicDuration;
    ui->horizontalSlider_songSlider->setMaximum(strMinute.toInt() * 60 + strSeconds.toInt());

    GetSongLyricBySongId(strMusicID);
    emit changeLyric(); //更换歌词
}

//拖动歌曲进度条
void MainWindow::on_horizontalSlider_songSlider_sliderMoved(int position)
{
    qDebug() << "position: " << position << player->duration();
    player->setPosition(position * 1000);
    ui->label_curSongTime->setText(QString("%1").arg(position/60, 2, 10, QLatin1Char('0')) + ":" + QString("%2").arg(position%60, 2, 10, QLatin1Char('0')));
}


void MainWindow::on_horizontalSlider_songSlider_sliderPressed()
{
    disconnect(player, &QMediaPlayer::positionChanged, this, &MainWindow::slotSongSliderPosChanged);
}


void MainWindow::on_horizontalSlider_songSlider_sliderReleased()
{
    connect(player, &QMediaPlayer::positionChanged, this, &MainWindow::slotSongSliderPosChanged);
}

//设置是否静音
void MainWindow::on_pushButton_volume_clicked()
{
    if (0 != player->volume())
    {
        iCurVolume = player->volume();
        player->setVolume(0);
    }
    else
    {
        player->setVolume(iCurVolume);
    }
}

//获取歌词
void MainWindow::GetSongLyricBySongId(QString musicId)
{
    QModelIndex curIndex = ui->tableView_search->currentIndex();
    int row = curIndex.row();//获得当前行索引
    if (row < 0 || row > 12)
    {
        return;
    }

    QString strUrl = QString("http://music.163.com/api/song/lyric?id=%1&lv=1&kv=1&tv=-1").arg(musicId);
    qDebug() << "QSslSocket=" << QSslSocket::sslLibraryBuildVersionString(); //电脑没配置https
    qDebug() << "OpenSSL支持情况:" << QSslSocket::supportsSsl();

    networkRequest->setUrl(QUrl(strUrl));
    networkManager->get(*networkRequest);
}

//打开歌词窗口
void MainWindow::on_pushButton_minPhoto_clicked()
{
    lyric.show();
    this->hide();
}

//上一页
void MainWindow::on_pushButton_lastPage_clicked()
{
    if (m_offset < 13)
    {
        return;
    }
    QString str = ui->lineEdit_search->text();
    m_offset -= 13;
    str=QString("http://music.163.com/api/search/get/web?csrf_token=hlpretag=&hlposttag=&s={%1}&type=1&offset=%2&total=true&limit=13").arg(str).arg(m_offset);
    networkRequest->setUrl(QUrl(str));
    networkManager->get(*networkRequest);
}

//下一页
void MainWindow::on_pushButton_nextPage_clicked()
{
    QString str = ui->lineEdit_search->text();
    m_offset += 13;
    str=QString("http://music.163.com/api/search/get/web?csrf_token=hlpretag=&hlposttag=&s={%1}&type=1&offset=%2&total=true&limit=13").arg(str).arg(m_offset);
    networkRequest->setUrl(QUrl(str));
    networkManager->get(*networkRequest);
}

