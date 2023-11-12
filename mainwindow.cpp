#include "mainwindow.h"
#include "ui_mainwindow.h"


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowFlag(Qt::FramelessWindowHint);
    setFixedSize(this->width(), this->height());
    setWindowIcon(QIcon(":/new/prefix1/image/title.png"));

    QPalette pal(this->palette());
    pal.setColor(QPalette::Background, Qt::white); //设置背景白色
    this->setAutoFillBackground(true);
    this->setPalette(pal);

    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(this);
    //设置阴影距离
    shadow->setOffset(10, 10);
    //设置阴影颜色
    shadow->setColor(QColor("#140444"));
    //设置阴影圆角
    shadow->setBlurRadius(30);
    //给嵌套QWidget设置阴影
    this->setGraphicsEffect(shadow);
    //给垂直布局器设置边距(此步很重要, 设置宽度为阴影的宽度)
    this->setContentsMargins(1,1,1,1);

    init();
}

MainWindow::~MainWindow()
{
    delete ui;
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
    playlist->setPlaybackMode(QMediaPlaylist::CurrentItemOnce); //歌曲循环模式
    connect(player, &QMediaPlayer::positionChanged, this, &MainWindow::slotSongSliderPosChanged);

    //读取数据库内容并填充
    if (!Datebase::instance()->createDB())
    {
//        QMessageBox::information(this, "数据库读取失败", "fialed to create database");
    }
    int iVolume = 0;
    if(Datebase::instance()->selectVolumeFromDB(iVolume))
    {
        player->setVolume(iVolume);
        iCurVolume = iVolume;
        ui->lineEdit_initialV->setText(QString::number(iVolume));
    }

    networkManager = new QNetworkAccessManager();
    qDebug() << __LINE__ << networkManager->supportedSchemes();
    networkRequest = new QNetworkRequest();
    connect(networkManager, &QNetworkAccessManager::finished, this, &MainWindow::databack);

    ui->horizontalSlider_songSlider->setMaximum(0);
    ui->horizontalSlider_volumeSlider->setMaximum(100);
    ui->horizontalSlider_volumeSlider->setValue(iVolume);

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

    ui->pushButton_lastPage->setVisible(false);
    ui->pushButton_nextPage->setVisible(false);
    ui->pushButton_lastPage->setToolTip("上一页");
    ui->pushButton_nextPage->setToolTip("下一页");
    ui->pushButton_lastSong->setToolTip("上一曲");
    ui->pushButton_nextSong->setToolTip("下一曲");
    ui->pushButton_playPause->setToolTip("播放");
    ui->pushButton_volume->setToolTip("静音");
    ui->pushButton_quit->setToolTip("关闭");
    ui->pushButton_minimize->setToolTip("最小化");

    ui->tableView_search->setShowGrid(false);

    connect(&lyric, &lyricShow::beHide, this, [=](){
        this->show();
    });

    //播放完成后切换下一曲
    connect(player, &QMediaPlayer::stateChanged, this, [=](QMediaPlayer::State state){
        if (state == QMediaPlayer::StoppedState) {
            qDebug() << "music play finished." << playlist->nextIndex(1) << playlist->previousIndex(1);
            ui->pushButton_playPause->setStyleSheet("QPushButton {border-image: url(':/new/prefix1/image/newPlay.png');}");
            ui->pushButton_playPause->setToolTip("播放");
//            playlist->next();

//            QModelIndex curIndex = ui->tableView_search->currentIndex();
//            int curRow = curIndex.row();
//            qDebug() << __LINE__ << curRow;
//            if (curRow < 0 || curRow > 12)
//            {
//                return;
//            }
//            ui->tableView_search->selectRow(curRow+1);
//            refreshCurrentMusic(curRow+1);
//            QStandardItemModel* tmpModel = new QStandardItemModel();
//            QModelIndex indexID =model->index(curRow+1,5);  // 第5列是musicID
//            on_tableView_search_doubleClicked(indexID);

//            on_pushButton_nextSong_clicked();
        }
    });

    //播放失败
    connect(player, QOverload<QMediaPlayer::Error>::of(&QMediaPlayer::error), this,
        [=](QMediaPlayer::Error error){
        qDebug () << "playError: " << error;
        ui->label_songName->setText("undefined error!");
        allLyricMap.clear();
        ui->label_lyric->clear();
        ui->pushButton_playPause->setStyleSheet("QPushButton {border-image: url(':/new/prefix1/image/newPlay.png');}");
        ui->pushButton_playPause->setToolTip("播放");
        player->stop();
    });

//    connect(playlist, &QMediaPlaylist::currentIndexChanged, this, &MainWindow::refreshCurrentMusic);

    connect(this, &MainWindow::changeLyric, &lyric, &lyricShow::slotChangeLyric); //切换歌曲后歌词也切换
}

void MainWindow::getWeight(int id)
{
    qDebug() << id;
    ui->stackedWidget->setCurrentIndex(id);
    if (1 == id)
    {
        QString str = QString("http://music.163.com/api/playlist/detail?id=3778678");

//        QString str = QString("http://music.163.com/api/search/get/web?csrf_token=hlpretag=&hlposttag=&s={}&type=1000&offset=0&total=true&limit=8");

        networkRequest->setUrl(QUrl(str));
        networkManager->get(*networkRequest);
    }
}

//关闭软件
void MainWindow::on_pushButton_quit_clicked()
{
    this->close();
}

//最小化
void MainWindow::on_pushButton_minimize_clicked()
{
    this->showMinimized();
}

//搜索栏
void MainWindow::on_lineEdit_search_returnPressed()
{
    if (ui->lineEdit_search->text().isEmpty())
    {
        return;
    }

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
//    qDebug() << __LINE__ << searchInfo;
    QJsonParseError err;               //错误信息对象
    QJsonDocument json_recv = QJsonDocument::fromJson(searchInfo,&err);//将json文本转换为 json 文件对象
    if(err.error != QJsonParseError::NoError)             //判断是否符合语法
    {
        qDebug() <<"搜索歌曲Json获取格式错误"<< err.errorString();
        return;
    }
    QJsonObject totalObject = json_recv.object();
    qDebug() << __LINE__ << json_recv;
    QStringList keys = totalObject.keys();    // 列出json里所有的key
    qDebug() << __LINE__ << keys;
    if(keys.contains("result"))                 //如果有结果
    {
        //在 json 文本中 {}花括号里面是QJsonObject对象, []方括号里面是QJsonArray
        QJsonObject resultObject = totalObject["result"].toObject();     //就将带 result 的内容提取后转换为对象
        QStringList resultKeys = resultObject.keys();      //保存所有key
        qDebug() << __LINE__ << resultKeys;
        if(resultKeys.contains("songs"))                    //如果 key 为songs ,代表找到了歌曲
        {
            ui->pushButton_lastPage->setVisible(true);
            ui->pushButton_nextPage->setVisible(true); //搜到歌曲后允许上下翻页
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

                model->item(k,1)->setToolTip(musicName); //鼠标悬停时显示信息
                model->item(k,2)->setToolTip(singerName);
                model->item(k,3)->setToolTip(albumName);
                model->item(k,4)->setToolTip(time);
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
    else if(keys.contains("lrc")) //处理歌词
    {
        QJsonObject lrcObject = totalObject["lrc"].toObject();     //就将带 msg 的内容提取后转换为对象
        QStringList resultKeys = lrcObject.keys();      //保存所有key
        qDebug() << __LINE__ << resultKeys;
        if (resultKeys.contains("lyric"))
        {
            allLyricMap.clear();

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
            refrashLyric(); //获得新歌词后刷新歌词
        }
    }
    else if(keys.contains("msg")) //处理歌单
    {
        QJsonObject msgObject = totalObject["msg"].toObject();     //就将带 msg 的内容提取后转换为对象
        QStringList msgKeys = msgObject.keys();      //保存所有key
        qDebug() << __LINE__ << msgKeys;
    }
}

//播放暂停
void MainWindow::on_pushButton_playPause_clicked()
{
    QModelIndex curIndex = ui->tableView_search->currentIndex();
    qDebug() << Q_FUNC_INFO << __LINE__ << ui->tableView_search->verticalHeader()->count();
    if (model->item(0, 0) == NULL || model->item(0,0)->text().trimmed() == "")
    {
        return;
    }

    int row=curIndex.row();//获得当前行索引
    if (row < 0 && QMediaPlayer::StoppedState == player->state()) //Table未选中则播放第一首歌
    {
        on_pushButton_nextSong_clicked();
        return;
    }

    //播放三种状态
    if (QMediaPlayer::PlayingState == player->state())
    {
        player->pause();
        ui->pushButton_playPause->setStyleSheet("QPushButton {border-image: url(':/new/prefix1/image/newPlay.png');}");
        ui->pushButton_playPause->setToolTip("播放");
    }
    else if (QMediaPlayer::PausedState == player->state())
    {
        int iValue = ui->horizontalSlider_songSlider->value();
        player->setPosition(iValue * 1000); //暂停状态时修改了进度条从修改完毕处播放
        player->play();
        ui->pushButton_playPause->setStyleSheet("QPushButton {border-image: url(':/new/prefix1/image/newPause.png');}");
        ui->pushButton_playPause->setToolTip("暂停");
    }
    else if (QMediaPlayer::StoppedState == player->state())
    {
        qDebug() << "state1: " << player->state();
        on_tableView_search_doubleClicked(curIndex);
    }
}

void MainWindow::on_tableView_search_doubleClicked(const QModelIndex &index)
{
    qDebug() << "index: " << index;
    int row=index.row();//获得当前行索引
    ui->tableView_search->selectRow(row); //选中列
    QModelIndex indexID =model->index(row,5);  // 第5列是musicID
    QString strMusicID = model->data(indexID).toString();//获取索引对应位置的数据转为字符串
    qDebug() << "clicked: " << strMusicID;

    player->setMedia(new QMediaPlaylist());
    playlist->clear();//清除播放列表
    QString url;
    url=QString("https://music.163.com/song/media/outer/url?id=%0").arg(strMusicID);
    playlist->addMedia(QUrl(url));                     //添加返回的音乐到播放列表中
//    player->setMedia(playlist);                        //将列表设置到播放器中
    player->setPlaylist(playlist);
//    playlist->setCurrentIndex(row);
    player->play();                                  //播放
    ui->pushButton_playPause->setStyleSheet("QPushButton {border-image: url(':/new/prefix1/image/newPause.png');}");
    ui->pushButton_playPause->setToolTip("暂停");

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
    ui->horizontalSlider_songSlider->setMaximum(strMinute.toInt() * 60 + strSeconds.toInt()); //歌曲进度条

//    refreshCurrentMusic(row);

    allLyricMap.clear();
    GetSongLyricBySongId(strMusicID);//获取歌词

    QModelIndex indexImg = model->index(row, 6);
    QString strMusicImg = model->data(indexImg).toString();
    qDebug() << __LINE__ << strMusicImg;
    networkRequest->setUrl(QUrl(strMusicImg)); //获取专辑图片
    networkManager->get(*networkRequest);
}

void MainWindow::slotSongSliderPosChanged(qint64 pos)
{
//    qDebug() << Q_FUNC_INFO << pos / 1000 << allLyricMap.keys();
    ui->horizontalSlider_songSlider->setValue(pos/1000);
    ui->label_curSongTime->setText(QString("%1").arg(pos/1000/60, 2, 10, QLatin1Char('0')) + ":" + QString("%2").arg(pos/1000%60, 2, 10, QLatin1Char('0')));

    int iValue = pos / 1000;
    if(allLyricMap.contains(iValue))
    {
        qDebug() << __LINE__ << allLyricMap.value(iValue);
        ui->label_lyric->setText(allLyricMap.value(iValue));
    }
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
    ui->pushButton_playPause->setStyleSheet("QPushButton {border-image: url(':/new/prefix1/image/newPause.png');}");
    ui->pushButton_playPause->setToolTip("暂停");

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

//    refreshCurrentMusic(row);

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
    playlist->addMedia(QUrl(url));  //添加返回的音乐到播放列表中
    player->setMedia(playlist);  //将列表设置到播放器中
    player->play();  //播放
    ui->pushButton_playPause->setStyleSheet("QPushButton {border-image: url(':/new/prefix1/image/newPause.png');}");
    ui->pushButton_playPause->setToolTip("暂停");

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

//    refreshCurrentMusic(row);

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
    qDebug() << Q_FUNC_INFO << ui->horizontalSlider_songSlider->value();
}

void MainWindow::on_horizontalSlider_songSlider_sliderReleased()
{
    connect(player, &QMediaPlayer::positionChanged, this, &MainWindow::slotSongSliderPosChanged);
    qDebug() << __LINE__ << ui->horizontalSlider_songSlider->value();
}

//设置是否静音
void MainWindow::on_pushButton_volume_clicked()
{
    if (!player->isMuted())
    {
        iCurVolume = player->volume();
        player->setMuted(true);
        ui->pushButton_volume->setStyleSheet("QPushButton {border-image: url(':/new/prefix1/image/newMute.png');}");
    }
    else
    {
        player->setMuted(false);
        ui->pushButton_volume->setStyleSheet("QPushButton {border-image: url(':/new/prefix1/image/newVolume.png');}");
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
    networkRequest->setUrl(QUrl(strUrl));
    networkManager->get(*networkRequest);
}

//刷新左下角单行显示歌词
void MainWindow::refrashLyric()
{
    QFile file("./lyric.txt");
    QTextStream in(&file);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        while (!in.atEnd())
        {
            QString strLineStream = in.readLine();
            analysisLyricsFile(strLineStream);//解析歌词文件内容

            QCoreApplication::processEvents();
        }
        file.close();
    }
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
//    playlist->clear();//清除播放列表
    QString str = ui->lineEdit_search->text();
    m_offset -= 13;
    str=QString("http://music.163.com/api/search/get/web?csrf_token=hlpretag=&hlposttag=&s={%1}&type=1&offset=%2&total=true&limit=13").arg(str).arg(m_offset);
    networkRequest->setUrl(QUrl(str));
    networkManager->get(*networkRequest);
}

//下一页
void MainWindow::on_pushButton_nextPage_clicked()
{
//    playlist->clear();//清除播放列表
    QString str = ui->lineEdit_search->text();
    m_offset += 13;
    str=QString("http://music.163.com/api/search/get/web?csrf_token=hlpretag=&hlposttag=&s={%1}&type=1&offset=%2&total=true&limit=13").arg(str).arg(m_offset);
    networkRequest->setUrl(QUrl(str));
    networkManager->get(*networkRequest);
}

//分析歌词
bool MainWindow::analysisLyricsFile(QString line)
{
    if(line == NULL || line.isEmpty()){
        qDebug()<<"thie line is empty!";
        return false;
    }
    QRegularExpression regularExpression("\\[(\\d+)?:(\\d+)?(\\.\\d+)?\\](.*)?");
    int index = 0;
    QRegularExpressionMatch match;
    match = regularExpression.match(line, index);
    if(match.hasMatch()) {
        int totalTime;
        totalTime = match.captured(1).toInt() * 60000 + match.captured(2).toInt() * 1000; //计算该时间点毫秒数
        QString currentText =QString::fromStdString(match.captured(4).toStdString()); //获取歌词文本
//        qDebug() << __LINE__ << currentText << totalTime;

        if (!currentText.isEmpty())
        {
            allLyricMap.insert(totalTime / 1000, currentText);
        }

        return true;
    }
    return false;
}

//点击歌曲进度条
void MainWindow::on_horizontalSlider_songSlider_actionTriggered(int action)
{
    qDebug() << __LINE__ << ui->horizontalSlider_songSlider->value() << action;
    if (ui->horizontalSlider_songSlider->value() >= ui->horizontalSlider_songSlider->maximum() || ui->horizontalSlider_songSlider->value() <= 0)
    {
        return;
    }

    int iValue = ui->horizontalSlider_songSlider->value();
    int iMaximum = ui->horizontalSlider_songSlider->maximum();
    if (3 == action) //点击进度条前面
    {
        qDebug() << __LINE__ << ui->horizontalSlider_songSlider->value() << iValue;
        if (iValue + 10 >= iMaximum) //步进为10，超出范围则设置为最大值
        {
            player->setPosition(iMaximum * 1000);
            emit player->positionChanged(iMaximum * 1000);
            ui->horizontalSlider_songSlider->setValue(iMaximum);
            ui->label_curSongTime->setText(QString("%1").arg(iMaximum/60, 2, 10, QLatin1Char('0')) + ":" + QString("%2").arg(iMaximum%60, 2, 10, QLatin1Char('0')));
        }
        else
        {
            player->setPosition(iValue * 1000 + 10000);
            emit player->positionChanged(iValue * 1000);
            ui->horizontalSlider_songSlider->setValue(iValue + 10);
            ui->label_curSongTime->setText(QString("%1").arg(iValue/60, 2, 10, QLatin1Char('0')) + ":" + QString("%2").arg(iValue%60, 2, 10, QLatin1Char('0')));
        }
    }
    else if (4 == action) //点击进度条后面
    {
        qDebug() << __LINE__ << ui->horizontalSlider_songSlider->value() << iValue;
        if (iValue <= 10) //步进为10，小于10则归零
        {
            player->setPosition(0);
            emit player->positionChanged(0);
            ui->horizontalSlider_songSlider->setValue(0);
            ui->label_curSongTime->setText(QString("%1").arg(0/60, 2, 10, QLatin1Char('0')) + ":" + QString("%2").arg(0%60, 2, 10, QLatin1Char('0')));
        }
        else
        {
            player->setPosition(iValue * 1000 - 10000);
            emit player->positionChanged(iValue * 1000);
            ui->horizontalSlider_songSlider->setValue(iValue - 10);
            ui->label_curSongTime->setText(QString("%1").arg(iValue/60, 2, 10, QLatin1Char('0')) + ":" + QString("%2").arg(iValue%60, 2, 10, QLatin1Char('0')));
        }
    }
    else
    {
        qDebug() << __LINE__ << ui->horizontalSlider_songSlider->value();
    }
}

//保存设置
void MainWindow::on_pushButton_saveSetting_clicked()
{
    int iVolume = ui->lineEdit_initialV->text().toInt();
    if (iVolume < 0 || iVolume > 100)
    {
        QMessageBox::information(this, "保存失败", "请输入数值为0~100");
        return;
    }
    Datebase::instance()->updateVolumeToDB(iVolume);
}

//切换歌后更新界面内容(未使用)
void MainWindow::refreshCurrentMusic(int iCurrentRow)
{
    qDebug() << "index: " << iCurrentRow;

    ui->tableView_search->selectRow(iCurrentRow); //选中列
    QModelIndex indexID =model->index(iCurrentRow,5);  // 第5列是musicID
    QString strMusicID = model->data(indexID).toString();//获取索引对应位置的数据转为字符串
    qDebug() << "clicked: " << strMusicID;

    QModelIndex indexSingerName = model->index(iCurrentRow, 1);
    QString strSongerName = model->data(indexSingerName).toString();
    QModelIndex indexSongName = model->index(iCurrentRow, 2);
    QString strMusicSongName = model->data(indexSongName).toString();
    ui->label_songName->setText(strMusicSongName + " - " + strSongerName); //歌曲名+歌手

    QModelIndex indexTime = model->index(iCurrentRow, 4);
    QString strMusicTime = model->data(indexTime).toString();
    ui->label_totalSongTime->setText(strMusicTime); // 歌曲时长

    QStringList timeList = strMusicTime.split(":");
    QString strMinute = timeList.at(0);
    QString strSeconds = timeList.at(1);
    musicDuration = strMinute.toInt() * 60 + strSeconds.toInt();
    qDebug() << "musicDuration: " << musicDuration;
    ui->horizontalSlider_songSlider->setMaximum(strMinute.toInt() * 60 + strSeconds.toInt()); //歌曲进度条
}

