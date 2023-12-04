#include "mainwindow.h"
#include "ui_mainwindow.h"


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowFlag(Qt::FramelessWindowHint);
    setFixedSize(this->width(), this->height());
    setWindowIcon(QIcon(":/new/prefix1/image/dinosaur.png"));

    QPalette pal(this->palette());
    pal.setColor(QPalette::Background, Qt::white); //设置背景白色
    this->setAutoFillBackground(true);
    this->setPalette(pal);

    QBitmap bmp(this->size());
    bmp.fill();
    QPainter p(&bmp);
    p.setPen(Qt::NoPen);
    p.setBrush(Qt::black);
    p.drawRoundedRect(bmp.rect(),5,5);//像素为5的圆角
    setMask(bmp);

    init();
}

MainWindow::~MainWindow()
{
    delete ui;
    delete player;
    delete searchPlaylist;
    delete hotSongModel;
    delete hotSongPlaylist;
    delete videoPlayer;
    delete videoPlaylist;
    delete popMenu;
    delete playMVAction;
}

void MainWindow::init()
{
    iCurVolume = 30;
    m_offset = 0;
    bHotSongList = false;
    bSongSheet = false;

    player=new QMediaPlayer();
    player->setVolume(30);
    searchPlaylist = new QMediaPlaylist();
    searchPlaylist->setPlaybackMode(QMediaPlaylist::CurrentItemOnce); //歌曲循环模式
    hotSongPlaylist = new QMediaPlaylist();
    hotSongPlaylist->setPlaybackMode(QMediaPlaylist::CurrentItemOnce); //歌曲循环模式
    connect(player, &QMediaPlayer::positionChanged, this, &MainWindow::slotSongSliderPosChanged);
    songSheetPlaylist = new QMediaPlaylist();
    songSheetPlaylist->setPlaybackMode(QMediaPlaylist::CurrentItemOnce); //歌曲循环模式

    videoPlayer = new QMediaPlayer();
    videoPlaylist = new QMediaPlaylist();
    mvWidget = new mvShow();
    videoPlayer->setVideoOutput(mvWidget);
    mvWidget->setAutoFillBackground(true);
    connect(mvWidget, &mvShow::stopPlaySig, videoPlayer, [=]() {
        videoPlayer->stop();
        this->show();
    });

    connect(mvWidget, &mvShow::clickToPlayPause, videoPlayer, [=]()
    {
        if (videoPlayer->state() == QMediaPlayer::PlayingState)
        {
            videoPlayer->pause();
        }
        else if (videoPlayer->state() == QMediaPlayer::PausedState)
        {
            videoPlayer->play();
        }
    });

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
    connect(networkManager, &QNetworkAccessManager::finished, this, &MainWindow::databack, Qt::QueuedConnection);

    ui->horizontalSlider_songSlider->setMaximum(0);
    ui->horizontalSlider_volumeSlider->setMaximum(100);
    ui->horizontalSlider_volumeSlider->setValue(iVolume);
    ui->horizontalSlider_volumeSlider->setToolTip(QString::number(ui->horizontalSlider_volumeSlider->value()));

    ui->tableView_search->setContextMenuPolicy(Qt::CustomContextMenu); //可弹出右键菜单
    popMenu = new QMenu(ui->tableView_search);
    playMVAction = new QAction();
    playMVAction->setText("播放MV");
    popMenu->addAction(playMVAction);
    connect(ui->tableView_search, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(slotDealSearchMenu(QPoint)));
    connect(playMVAction, &QAction::triggered, this, &MainWindow::slotPlayMV);

    ui->tableView_hotSongTable->setContextMenuPolicy(Qt::CustomContextMenu); //可弹出右键菜单
    rankMenu = new QMenu(ui->tableView_hotSongTable);
    newMenu = new QAction();
    upMenu = new QAction();
    hotMenu = new QAction();
    dyMenu = new QAction();
    newMenu->setText("新歌榜");
    upMenu->setText("飙升榜");
    hotMenu->setText("热歌榜");
    dyMenu->setText("抖音榜");
    rankMenu->addAction(newMenu);
    rankMenu->addAction(upMenu);
    rankMenu->addAction(hotMenu);
    rankMenu->addAction(dyMenu);
    connect(ui->tableView_hotSongTable, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(slotDealRankMenu(QPoint)));
    connect(newMenu, &QAction::triggered, this, &MainWindow::slotDealRankAction);
    connect(upMenu, &QAction::triggered, this, &MainWindow::slotDealRankAction);
    connect(hotMenu, &QAction::triggered, this, &MainWindow::slotDealRankAction);
    connect(dyMenu, &QAction::triggered, this, &MainWindow::slotDealRankAction);

    ui->tableView_singleSheet->setContextMenuPolicy(Qt::CustomContextMenu); //可弹出右键菜单
    sheetMenu = new QMenu(ui->tableView_singleSheet);
    backMenu = new QAction();
    backMenu->setText("返回歌单界面");
    sheetMenu->addAction(backMenu);
    connect(ui->tableView_singleSheet, &QTableView::customContextMenuRequested, this, &MainWindow::slotDealSheetMenu);
    connect(backMenu, &QAction::triggered, this, &MainWindow::slotBackAction);

    ui->stackedWidget->setCurrentIndex(0);

    initControlStylesheet(); //修改部分按键样式

    connect(ui->buttonGroup, QOverload<int>::of(&QButtonGroup::buttonClicked), this, &MainWindow::getWeight);

    ui->tableView_search->setShowGrid(false);
    ui->tableView_hotSongTable->setShowGrid(false);
    ui->tableView_singleSheet->setShowGrid(false);

    connect(&lyric, &lyricShow::beHide, this, [=](){
        this->show();
        lyric.hide();
    });

    //播放完成后切换下一曲
    connect(player, &QMediaPlayer::stateChanged, this, [=](QMediaPlayer::State state){
        if (state == QMediaPlayer::StoppedState) {
            qDebug() << "music play finished." << searchPlaylist->nextIndex(1) << searchPlaylist->previousIndex(1);
            ui->pushButton_playPause->setStyleSheet("QPushButton {border-image: url(':/new/prefix1/image/newPlay.png');}");
            ui->pushButton_playPause->setToolTip("播放");
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

    connect(videoPlayer, QOverload<QMediaPlayer::Error>::of(&QMediaPlayer::error), this,
            [=](QMediaPlayer::Error error)
    {
        qDebug () << "playError: " << error;
        videoPlayer->stop();
    });

//    connect(searchPlaylist, &QMediaPlaylist::currentIndexChanged, this, &MainWindow::refreshCurrentMusic);
    connect(hotSongPlaylist, &QMediaPlaylist::currentIndexChanged, this, &MainWindow::refreshCurrentMusic);
    connect(songSheetPlaylist, &QMediaPlaylist::currentIndexChanged, this, &MainWindow::refreshCurrentMusic);

    connect(this, &MainWindow::changeSongImage, &lyric, &lyricShow::slotChangeSongImage); //切换歌曲后图片也切换
    connect(this, &MainWindow::modifiedSongInfor, &lyric, &lyricShow::slotModifiedSongInfor);
    connect(this, &MainWindow::setLyricSig, &lyric, &lyricShow::slotGetLyric); //重新设置歌词
    connect(this, &MainWindow::changeSingleLyric, &lyric, &lyricShow::slotChangeSingleLyric); //修改单据歌词显示位置
}

//初始化部分样式
void MainWindow::initControlStylesheet()
{
    ui->buttonGroup->setId(ui->pushButton_search,0); //按钮组加ID
    ui->buttonGroup->setId(ui->pushButton_songSheet,1);
    ui->buttonGroup->setId(ui->pushButton_rankList,2);
    ui->buttonGroup->setId(ui->pushButton_setting,3);
    ui->buttonGroup->setId(ui->pushButton_login,4);

    QStringList headerList;
    headerList << "#" << "歌曲名称" << "歌手名" << "专辑名" << "时长";
    QHeaderView *hearview = new QHeaderView(Qt::Horizontal); //歌曲table
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
    ui->tableView_search->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableView_search->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tableView_search->setFocusPolicy(Qt::NoFocus);

    QStringList hotSongHeaderList;
    hotSongHeaderList << "#" << "歌曲名称" << "歌手名" << "专辑名" << "时长";
    QHeaderView *hotSongHearview = new QHeaderView(Qt::Horizontal); //歌曲table
    hotSongModel = new QStandardItemModel(); //热歌榜table
    hotSongModel->setHorizontalHeaderLabels(hotSongHeaderList);
    hotSongHearview->setModel(hotSongModel);
    ui->tableView_hotSongTable->setHorizontalHeader(hotSongHearview);
    ui->tableView_hotSongTable->setColumnWidth(0, 30);
    ui->tableView_hotSongTable->setColumnWidth(1, 230);
    ui->tableView_hotSongTable->setColumnWidth(2, 140);
    ui->tableView_hotSongTable->setColumnWidth(3, 181);
    ui->tableView_hotSongTable->setColumnWidth(4, 110);
    ui->tableView_hotSongTable->verticalHeader()->setHidden(true);
    ui->tableView_hotSongTable->setEditTriggers(QAbstractItemView::NoEditTriggers); //设置只读
    ui->tableView_hotSongTable->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->tableView_hotSongTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableView_hotSongTable->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tableView_hotSongTable->setFocusPolicy(Qt::NoFocus);

    QStringList songSheetHeaderList; //歌单table
    songSheetHeaderList << "#" << "歌曲名称" << "歌手名" << "专辑名" << "时长";
    QHeaderView *songSheetHearview = new QHeaderView(Qt::Horizontal); //歌曲table
    songSheetModel = new QStandardItemModel(); //热歌榜table
    songSheetModel->setHorizontalHeaderLabels(songSheetHeaderList);
    songSheetHearview->setModel(songSheetModel);
    ui->tableView_singleSheet->setHorizontalHeader(songSheetHearview);
    ui->tableView_singleSheet->setModel(songSheetModel);
    ui->tableView_singleSheet->setColumnWidth(0, 30);
    ui->tableView_singleSheet->setColumnWidth(1, 230);
    ui->tableView_singleSheet->setColumnWidth(2, 140);
    ui->tableView_singleSheet->setColumnWidth(3, 181);
    ui->tableView_singleSheet->setColumnWidth(4, 110);
    ui->tableView_singleSheet->verticalHeader()->setHidden(true);
    ui->tableView_singleSheet->setEditTriggers(QAbstractItemView::NoEditTriggers); //设置只读
    ui->tableView_singleSheet->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableView_singleSheet->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tableView_singleSheet->setFocusPolicy(Qt::NoFocus);
    ui->tableWidget_songSheet->horizontalHeader()->setVisible(false); //隐藏水平表头
    ui->tableWidget_songSheet->verticalHeader()->setVisible(false); //隐藏垂直表头
    ui->tableWidget_songSheet->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff); //水平滚动条
//    ui->tableWidget_songSheet->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff); //垂直滚动条

    ui->tableWidget_songSheet->horizontalHeader()->setDefaultSectionSize(234); //限制宽高
    ui->tableWidget_songSheet->verticalHeader()->setDefaultSectionSize(147);

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
    ui->pushButton_search->setToolTip("搜索");
    ui->pushButton_songSheet->setToolTip("歌单");
    ui->pushButton_rankList->setToolTip("排行榜");
    ui->pushButton_setting->setToolTip("设置");
    ui->pushButton_login->setToolTip("用户中心");

    QIcon searchIcon(":/new/prefix1/image/search.png"); // 加载左侧按键图标
    QPixmap searchPixmap = searchIcon.pixmap(QSize(40, 40)); // 转换为40x40大小的QPixmap
    ui->pushButton_search->setIcon(QIcon(searchPixmap));
    ui->pushButton_search->setIconSize(QSize(40, 40));

    QIcon sheetIcon(":/new/prefix1/image/songSheet.png");
    QPixmap sheetPixmap = sheetIcon.pixmap(QSize(40, 40));
    ui->pushButton_songSheet->setIcon(QIcon(sheetPixmap));
    ui->pushButton_songSheet->setIconSize(QSize(40, 40));

    QIcon rankIcon(":/new/prefix1/image/theList.png");
    QPixmap rankPixmap = rankIcon.pixmap(QSize(40, 40));
    ui->pushButton_rankList->setIcon(QIcon(rankPixmap));
    ui->pushButton_rankList->setIconSize(QSize(40, 40));

    QIcon settingIcon(":/new/prefix1/image/setting.png");
    QPixmap settingPixmap = settingIcon.pixmap(QSize(40, 40));
    ui->pushButton_setting->setIcon(QIcon(settingPixmap));
    ui->pushButton_setting->setIconSize(QSize(40, 40));

    QIcon userIcon(":/new/prefix1/image/userLogin.png");
    QPixmap userPixmap = userIcon.pixmap(QSize(45, 45));
    ui->pushButton_login->setIcon(QIcon(userPixmap));
    ui->pushButton_login->setIconSize(QSize(45, 45));

    QMovie *movie = new QMovie(":/new/prefix1/image/newloading.gif"); //热歌榜需要加载时间
    ui->label_rankLoading->setMovie(movie);  //将动态图加载进入label
    ui->label_sheetLoading->setMovie(movie);
    movie->setScaledSize(ui->label_rankLoading->size());   //将gif图设置成label大小
    movie->start();
}

void MainWindow::getWeight(int id)
{
    qDebug() << id;
    //点击后修改样式
    for (int i = 0; i < 5; ++i)
    {
        if (id == i)
        {
            ui->buttonGroup->button(i)->setStyleSheet("QPushButton{"
                                                      "background-color: rgb(240, 240, 240); "
                                                      "font: 13px 'Microsoft YaHei';"
                                                      "border-radius: 0px;"
                                                      "border: 0px solid rgb(250, 250, 250);"
                                                      "border-style:outset;"
                                                  "}"

                                                  "QPushButton:hover{"
                                                      "background-color: rgb(230, 230, 230);"
                                                  "}"

                                                  "QPushButton:pressed"
                                                  "{"
                                                      "padding-left:3px;"
                                                      "padding-top:3px;"
                                                  "}");
        }
        else
        {
            ui->buttonGroup->button(i)->setStyleSheet("QPushButton{"
                                                      "background-color: rgb(255, 255, 255); "
                                                      "font: 13px 'Microsoft YaHei';"
                                                      "border-radius: 0px;"
                                                      "border: 0px solid rgb(250, 250, 250);"
                                                      "border-style:outset;"
                                                  "}"

                                                  "QPushButton:hover{"
                                                      "background-color: rgb(230, 230, 230);"
                                                  "}"

                                                  "QPushButton:pressed"
                                                  "{"
                                                      "padding-left:3px;"
                                                      "padding-top:3px;"
                                                  "}");
        }

//        QCoreApplication::processEvents();
    }

    QString str;
    ui->stackedWidget->setCurrentIndex(id);
    if (1 == id && !bSongSheet) //歌单
    {
        bSongSheet = true;
//        ui->stackedWidget_songSheet->setCurrentIndex(0);
//        QString str = "http://api.yimian.xyz/msc/?type=playlist&id=2557908184"; //可用  每日推荐歌单
//        QString str = QString("http://music.163.com/api/playlist/detail?id=2557908184"); //不稳定
//        QString str = "http://music.cyrilstudio.top/recommend/resource"; //每日推荐歌单(需登录)
        QString str = "http://music.cyrilstudio.top/top/playlist/highquality";

        networkRequest->setUrl(QUrl(str));
        networkManager->get(*networkRequest);
    }
    else if (2 == id && !bHotSongList) //热歌榜
    {
        setControlEnabled(false, ui->pushButton_rankList);
        bHotSongList = true;
//        str = "http://api.wer.plus/api/wytop?t=4"; //t=1:原创榜,2:新歌榜,3:飙升榜,4:热歌榜
//        str = "http://music.163.com/api/playlist/detail?id=3778678"; //用此api信息更详细但不稳定
        str = "http://api.yimian.xyz/msc/?type=playlist&id=3778678";
        networkRequest->setUrl(QUrl(str));
        networkManager->get(*networkRequest);
    }
    else if(3 == id)
    {

    }
    else if(4 == id)
    {
//        QString str = QString("http://music.163.com/api/playlist/detail?id=3779629");
//        QString str = QString("http://music.163.com/api/search/get/web?csrf_token=hlpretag=&hlposttag=&s=李荣浩&type=1&offset=0&total=true&limit=8");
//        QString str = "http://api.yimian.xyz/msc/?type=single&id=29764564";
//        QString str = "http://api.yimian.xyz/msc/?type=cover&id=109951165182029540";
//        QString str = "http://p3.music.126.net/0uZ_bKtm4E188Uk9LFN1qg==/109951163187393370.jpg?param=300y300";
//        str = "http://api.wer.plus/api/wytop?t=1";
//        QString str = "http://music.163.com/api/mv/detail?id=365345&type=mp4"; //获取mv播放地址
//        QString str = "http://p2.music.126.net/iXldDnpjTRxGpGAvMGNbag==/109951167496316773.jpg";
//        networkRequest->setUrl(QUrl(str));
//        networkManager->get(*networkRequest);
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

    getWeight(0); //搜索后回到第一页
}

//处理数据
void MainWindow::databack(QNetworkReply *reply)
{
    loop.quit();
    QVariant nCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    qDebug() << __LINE__ << nCode << reply->errorString();
    searchInfo = reply->readAll();
//    qDebug() << __LINE__ << searchInfo;
    QJsonParseError err;  //错误信息对象
    QJsonDocument json_recv = QJsonDocument::fromJson(searchInfo,&err);//将json文本转换为 json 文件对象
    if(err.error != QJsonParseError::NoError)  //判断是否符合语法(图片也会走这里，暂不清楚原因,猜测是格式问题)
    {
        if (ui->stackedWidget->currentIndex() == PAGE_SONGSHEET && ui->stackedWidget_songSheet->currentIndex() == 0)
        {
            QPixmap pixmap; //处理图片
            pixmap.loadFromData(searchInfo);
            qDebug() << __LINE__ << searchInfo << pixmap;

            sheetShow->setAlbumImage(pixmap);
            return ;
        }
        QPixmap pixmap; //处理图片
        pixmap.loadFromData(searchInfo);
        qDebug() << __LINE__ << searchInfo << pixmap << json_recv.isNull();
        if (!pixmap.isNull())
        {
            emit changeSongImage(pixmap);

            pixmap = pixmap.scaled(ui->pushButton_minPhoto->width(), ui->pushButton_minPhoto->height(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
            ui->pushButton_minPhoto->setIcon(pixmap);
            ui->pushButton_minPhoto->setIconSize(QSize(ui->pushButton_minPhoto->width(), ui->pushButton_minPhoto->height()));
        }

        qDebug() <<"搜索歌曲Json获取格式错误"<< err.errorString();
        return;
    }
    if (json_recv.isArray()) //热歌榜和歌单 接收到的数据为QJsonArray
    {
        if (ui->stackedWidget->currentIndex() == PAGE_SONGSHEET) //歌单
        {
            songSheetPlaylist->clear();
            songSheetModel->clear();
            QJsonArray jsonArray = json_recv.array();
            qDebug() << __LINE__ << jsonArray.count();
            int k = 0;
            for (const auto &i : qAsConst(jsonArray))
            {
                QJsonObject jsonObject = i.toObject();
                int musicId = jsonObject["id"].toInt();
                QString musicName = jsonObject["name"].toString();
                QString singerName = jsonObject["artist"].toString();
                QString albumName = jsonObject["album"].toString();
                QString playAddress = jsonObject["url"].toString();
                QString imageAddress = jsonObject["cover"].toString();
                QString lyricAddress = jsonObject["lrc"].toString();

                qDebug() << "hotmusicName: " << musicName;
                qDebug() << "hotmusicid: " << musicId;
                qDebug() << "hotsingerName: " << singerName;
                qDebug() << "hotalbumName: " << albumName;
                qDebug() << "hotplayAddress: " << playAddress;
                qDebug() << "hotimageAddress: " << imageAddress;
                qDebug() << "hotlyricAddress: " << lyricAddress << "\n";

                songSheetModel->setItem(k,0, new QStandardItem(QString::number(k+1)));
                songSheetModel->setItem(k,1,new QStandardItem(musicName)); //音乐名称
                songSheetModel->setItem(k,2,new QStandardItem(singerName)); //歌手名
                songSheetModel->setItem(k,3,new QStandardItem(albumName)); //专辑名
                songSheetModel->setItem(k,4,new QStandardItem("undefined")); //时长
                songSheetModel->setItem(k,5,new QStandardItem(QString::number(musicId))); //音乐ID
                songSheetModel->setItem(k,6,new QStandardItem(imageAddress)); //歌曲播放地址
                songSheetModel->item(k,1)->setToolTip(musicName); //鼠标悬停时显示信息
                songSheetModel->item(k,2)->setToolTip(singerName); //鼠标悬停时显示信息
                songSheetModel->item(k,3)->setToolTip(albumName); //鼠标悬停时显示信息
                songSheetModel->item(k,4)->setToolTip("undefined"); //鼠标悬停时显示信息

                songSheetModel->item(k,0)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
                songSheetModel->item(k,1)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
                songSheetModel->item(k,2)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
                songSheetModel->item(k,3)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
                songSheetModel->item(k,4)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);

                ++k;

                QString url=QString("https://music.163.com/song/media/outer/url?id=%0").arg(musicId);
                songSheetPlaylist->addMedia(QUrl(url));                     //添加返回的音乐到播放列表中
            }
            ui->label_sheetLoading->hide();
            setControlEnabled(true, ui->pushButton_songSheet);

            ui->tableView_singleSheet->setColumnWidth(0, 30);
            ui->tableView_singleSheet->setColumnWidth(1, 229);
            ui->tableView_singleSheet->setColumnWidth(2, 140);
            ui->tableView_singleSheet->setColumnWidth(3, 181);
            ui->tableView_singleSheet->setColumnWidth(4, 105);
            ui->tableView_singleSheet->setColumnWidth(5, 0);
            ui->tableView_singleSheet->setColumnWidth(6, 0);
            ui->tableView_singleSheet->hideColumn(5);
            ui->tableView_singleSheet->hideColumn(6);

            QStringList songSheetHeaderList; //切换榜单后会更新hotSongModel
            songSheetHeaderList << "#" << "歌曲名称" << "歌手名" << "专辑名" << "时长";
            songSheetModel->setHorizontalHeaderLabels(songSheetHeaderList);

            ui->tableView_singleSheet->setModel(songSheetModel);
            return;
        }
        else if (ui->stackedWidget->currentIndex() == PAGE_RANKSHEET) //排行榜
        {
            hotSongPlaylist->clear();
            QJsonArray jsonArray = json_recv.array();
            qDebug() << __LINE__ << jsonArray.count();
            int k = 0;
            for (const auto &i : qAsConst(jsonArray))
            {
                QJsonObject jsonObject = i.toObject();
                int musicId = jsonObject["id"].toInt();
                QString musicName = jsonObject["name"].toString();
                QString singerName = jsonObject["artist"].toString();
                QString albumName = jsonObject["album"].toString();
                QString playAddress = jsonObject["url"].toString();
                QString imageAddress = jsonObject["cover"].toString();
                QString lyricAddress = jsonObject["lrc"].toString();

                qDebug() << "hotmusicName: " << musicName;
                qDebug() << "hotmusicid: " << musicId;
                qDebug() << "hotsingerName: " << singerName;
                qDebug() << "hotalbumName: " << albumName;
                qDebug() << "hotplayAddress: " << playAddress;
                qDebug() << "hotimageAddress: " << imageAddress;
                qDebug() << "hotlyricAddress: " << lyricAddress << "\n";

                hotSongModel->setItem(k,0, new QStandardItem(QString::number(k+1)));
                hotSongModel->setItem(k,1,new QStandardItem(musicName)); //音乐名称
                hotSongModel->setItem(k,2,new QStandardItem(singerName)); //歌手名
                hotSongModel->setItem(k,3,new QStandardItem(albumName)); //专辑名
                hotSongModel->setItem(k,4,new QStandardItem("undefined")); //时长
                hotSongModel->setItem(k,5,new QStandardItem(QString::number(musicId))); //音乐ID
                hotSongModel->setItem(k,6,new QStandardItem(imageAddress)); //歌曲播放地址
                hotSongModel->item(k,1)->setToolTip(musicName); //鼠标悬停时显示信息
                hotSongModel->item(k,2)->setToolTip(singerName); //鼠标悬停时显示信息
                hotSongModel->item(k,3)->setToolTip(albumName); //鼠标悬停时显示信息
                hotSongModel->item(k,4)->setToolTip("undefined"); //鼠标悬停时显示信息

                hotSongModel->item(k,0)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
                hotSongModel->item(k,1)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
                hotSongModel->item(k,2)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
                hotSongModel->item(k,3)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
                hotSongModel->item(k,4)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);

                ++k;

                QString url=QString("https://music.163.com/song/media/outer/url?id=%0").arg(musicId);
                hotSongPlaylist->addMedia(QUrl(url));                     //添加返回的音乐到播放列表中
            }
            ui->label_rankLoading->hide();
            setControlEnabled(true, ui->pushButton_rankList);

            ui->tableView_hotSongTable->setColumnWidth(0, 30);
            ui->tableView_hotSongTable->setColumnWidth(1, 230);
            ui->tableView_hotSongTable->setColumnWidth(2, 140);
            ui->tableView_hotSongTable->setColumnWidth(3, 181);
            ui->tableView_hotSongTable->setColumnWidth(4, 105);
            ui->tableView_hotSongTable->hideColumn(5);
            ui->tableView_hotSongTable->hideColumn(6);

            QStringList hotSongHeaderList; //切换榜单后会更新hotSongModel
            hotSongHeaderList << "#" << "歌曲名称" << "歌手名" << "专辑名" << "时长";
            hotSongModel->setHorizontalHeaderLabels(hotSongHeaderList);

            ui->tableView_hotSongTable->setModel(hotSongModel);

            return;
        }

    }
    else
    {
        QJsonObject totalObject = json_recv.object();
        qDebug() << __LINE__ << json_recv;
        QStringList keys = totalObject.keys();    // 列出json里所有的key
        if(keys.contains("result")) //如果有结果
        {
            //在 json 文本中 {}花括号里面是QJsonObject对象, []方括号里面是QJsonArray
            QJsonObject resultObject = totalObject["result"].toObject();     //就将带 result 的内容提取后转换为对象
            QStringList resultKeys = resultObject.keys();      //保存所有key
    //        qDebug() << __LINE__ << resultKeys;
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
                    qDebug() << "mvId: " << mvId;

                    model->setItem(k,0, new QStandardItem(QString::number(k+1)));
                    model->setItem(k,1,new QStandardItem(musicName)); //音乐名称
                    model->setItem(k,2,new QStandardItem(singerName)); //歌手名
                    model->setItem(k,3,new QStandardItem(albumName)); //专辑名
                    QString time = QString("%1:%2").arg(musicDuration/1000/60, 2, 10, QLatin1Char('0')).arg(musicDuration/1000%60, 2, 10, QLatin1Char('0'));
                    model->setItem(k,4,new QStandardItem(time)); //歌曲时长
                    model->setItem(k,5,new QStandardItem(QString::number(musicId))); //歌曲ID
                    model->setItem(k,6,new QStandardItem(imgAddress)); //歌曲图片路径
                    model->setItem(k,7,new QStandardItem(QString::number(mvId))); //MVID

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
                    searchPlaylist->addMedia(QUrl(url));                     //添加返回的音乐到播放列表中
                }
                ui->tableView_search->setColumnWidth(0, 10);
                ui->tableView_search->setColumnWidth(1, 230);
                ui->tableView_search->setColumnWidth(2, 150);
                ui->tableView_search->setColumnWidth(3, 176);
                ui->tableView_search->setColumnWidth(4, 120);
                ui->tableView_search->setColumnWidth(5, 0);
                ui->tableView_search->setColumnWidth(6, 0);
                ui->tableView_search->setColumnWidth(7, 0);
                ui->tableView_search->hideColumn(5);
                ui->tableView_search->hideColumn(6);
                ui->tableView_search->hideColumn(7);
                ui->tableView_search->setModel(model);
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
                lyricList = strLyric.split("\n");

    //            QFile file("./lyric.txt"); //也可以使用文件保存歌词
    //            file.remove();
    //            if (file.open(QIODevice::ReadWrite | QIODevice::Text | QIODevice::Truncate))
    //            {
    //                QTextStream stream(&file);
    //                stream.seek(file.size());
    //                for (auto& i : lyricList)
    //                {
    //                    QString strLineLyric;
    //                    strLineLyric.append(i);
    //                    stream << strLineLyric << "\n";

    ////                    QCoreApplication::processEvents();
    //                }
    //                file.close();

    ////                emit changeLyric();
    //            }
                refrashLyric(); //获得新歌词后刷新歌词
            }
        }
        else if(keys.contains("recommend") && keys.count() == 4) //处理歌单(未完成)
        {
            QJsonObject msgObject = totalObject["recommend"].toObject();     //就将带 recommend 的内容提取后转换为对象
            QStringList msgKeys = msgObject.keys();      //保存所有key
            qDebug() << __LINE__ << msgKeys;
            QJsonArray recommendArray = totalObject["recommend"].toArray();
            qDebug() << __LINE__ << recommendArray;
            int iPos = 0;
            ui->tableWidget_songSheet->setRowCount(recommendArray.size()/3);
            ui->tableWidget_songSheet->setColumnCount(3);
            for (const auto &i : qAsConst(recommendArray))
            {
                QJsonObject object = i.toObject();

                double strId = object["id"].toDouble();
                int strType = object["type"].toInt();
                QString strSongName = object["name"].toString();
                QString picUrl = object["picUrl"].toString();
                double playCount = object["playcount"].toDouble();
                double createTime = object["createTime"].toDouble();
                double trackCount = object["trackCount"].toDouble();

                QString singerName;
                QJsonObject creatorObj = object["creator"].toObject();
                singerName = creatorObj["nickname"].toString();

                qDebug() << "strId = " << strId << object["id"].type() << object["id"].toVariant() << QString::number(strId, 'f', 0);
                qDebug() << "strType = " << strType << object["type"].type() << QString::number(strType, 'f', 0);
                qDebug() << "strSongName = " << strSongName << object["name"].type();
                qDebug() << "picUrl = " << picUrl << object["picUrl"].type();
                qDebug() << "playCount = " << playCount << object["playcount"].type() << QString::number(playCount, 'f', 0);
                qDebug() << "createTime = " << createTime << object["createTime"].type() << QString::number(createTime, 'f', 0);
                qDebug() << "trackCount = " << trackCount << object["trackCount"].type() << QString::number(trackCount, 'f', 0);

                sheetShow = new SingleSheetShow();
                sheetShow->setSheetValues(QString::number(strId, 'f', 0), singerName, strSongName, picUrl, QString::number(createTime, 'f', 0), QString::number(playCount, 'f', 0), QString::number(trackCount, 'f', 0));

                ui->tableWidget_songSheet->setCellWidget(iPos/3, iPos%3, sheetShow);
                qDebug() << "rowCount==== " << ui->tableWidget_songSheet->rowCount() << ui->tableWidget_songSheet->columnCount() << iPos;
                iPos++;

                if (picUrl.contains("https"))
                {
                    picUrl = "http" + picUrl.mid(5, -1);
                }
                qDebug() << __LINE__ << picUrl;
                networkRequest->setUrl(QUrl(picUrl)); //获取专辑图片
                networkManager->get(*networkRequest);

                loop.exec(QEventLoop::ExcludeUserInputEvents); //保证每个widget图片对应上

                connect(sheetShow, &SingleSheetShow::showSingleSheetSig, this, &MainWindow::slotShowSingleSheet);
            }
        }
        else if (keys.contains("playlists") && keys.count() == 5) //精选歌单
        {
            QJsonArray listArray = totalObject["playlists"].toArray();
            qDebug() << __LINE__ << listArray;
            ui->tableWidget_songSheet->setRowCount(listArray.size()/3);
            ui->tableWidget_songSheet->setColumnCount(3);
            int iPos = 0;
            for (const auto &i : qAsConst(listArray))
            {
                QJsonObject object = i.toObject();

                double strId = object["id"].toDouble();
//                int strType = object["type"].toInt();
                QString strSongName = object["name"].toString();
                QString picUrl = object["coverImgUrl"].toString();
                double playCount = object["playCount"].toDouble();
                double createTime = object["createTime"].toDouble();
                double trackCount = object["trackCount"].toDouble();

                QString singerName;
                QJsonObject creatorObj = object["creator"].toObject();
                singerName = creatorObj["nickname"].toString();

                qDebug() << "strId = " << strId << object["id"].type() << object["id"].toVariant() << QString::number(strId, 'f', 0);
//                qDebug() << "strType = " << strType << object["type"].type() << QString::number(strType, 'f', 0);
                qDebug() << "strSongName = " << strSongName << object["name"].type();
                qDebug() << "picUrl = " << picUrl << object["picUrl"].type();
                qDebug() << "playCount = " << playCount << object["playCount"].type() << QString::number(playCount, 'f', 0);
                qDebug() << "createTime = " << createTime << object["createTime"].type() << QString::number(createTime, 'f', 0);
                qDebug() << "trackCount = " << trackCount << object["trackCount"].type() << QString::number(trackCount, 'f', 0);

                sheetShow = new SingleSheetShow();
                sheetShow->setSheetValues(QString::number(strId, 'f', 0), singerName, strSongName, picUrl, QString::number(createTime, 'f', 0), QString::number(playCount, 'f', 0), QString::number(trackCount, 'f', 0));

                ui->tableWidget_songSheet->setCellWidget(iPos/3, iPos%3, sheetShow);
                qDebug() << "rowCount==== " << ui->tableWidget_songSheet->rowCount() << ui->tableWidget_songSheet->columnCount() << iPos;
                iPos++;

                if (picUrl.contains("https"))
                {
                    picUrl = "http" + picUrl.mid(5, -1);
                }
                qDebug() << __LINE__ << picUrl;
                networkRequest->setUrl(QUrl(picUrl)); //获取专辑图片
                networkManager->get(*networkRequest);

                loop.exec(QEventLoop::ExcludeUserInputEvents); //保证每个widget图片对应上

                QCoreApplication::processEvents(); //防止阻塞界面

                connect(sheetShow, &SingleSheetShow::showSingleSheetSig, this, &MainWindow::slotShowSingleSheet);
            }
        }
        else if (keys.contains("subed") && keys.contains("data") && keys.count() == 7) //MV播放
        {
            QJsonObject dataObject = totalObject["data"].toObject();
            QStringList dataKeyList = dataObject.keys();
            QString songName = dataObject["name"].toString(); //歌曲名
            QString singerName = dataObject["artistName"].toString(); //歌手名
            if (dataKeyList.contains("brs"))
            {
                QJsonObject brsObject = dataObject["brs"].toObject();
                QStringList brsKeyList = brsObject.keys();
                QList<int> iKeyList; //保存最大分辨率
                for (const auto &i : qAsConst(brsKeyList))
                {
                    iKeyList.append(i.toInt());
                }
                qSort(iKeyList.begin(), iKeyList.end());
    //            brsKeyList.sort();
                qDebug() << __LINE__ <<brsObject["240"].toString();
                qDebug() << __LINE__ <<brsObject["480"].toString();
                qDebug() << __LINE__ <<brsObject["720"].toString() << iKeyList;

                QString mvRatio = brsObject[QString::number(iKeyList.last())].toString();
                qDebug() << __LINE__ << mvRatio << brsKeyList << (mvWidget == nullptr);

                player->pause(); //播放mv时暂停播放音乐
                ui->pushButton_playPause->setStyleSheet("QPushButton {border-image: url(':/new/prefix1/image/newPlay.png');}");
                ui->pushButton_playPause->setToolTip("播放");

                videoPlaylist->clear(); //播放列表只需要一个mv
                videoPlaylist->addMedia(QUrl(mvRatio)); //添加返回的音乐到播放列表中
                videoPlayer->setPlaylist(videoPlaylist);
                videoPlayer->play();
                mvWidget->resize(720, 480);
                mvWidget->setWindowTitle(songName + " - " + singerName + " - " + QString::number(iKeyList.last()) + "p");
                mvWidget->setAutoFillBackground(true);
                mvWidget->show();
                this->hide();
            }
        }
    }
    reply->deleteLater();
}

//播放暂停
void MainWindow::on_pushButton_playPause_clicked()
{
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
//        on_tableView_search_doubleClicked(curIndex);
        if (searchPlaylist->isEmpty() && hotSongPlaylist->isEmpty())
        {
            return;
        }
        player->play();
        ui->pushButton_playPause->setStyleSheet("QPushButton {border-image: url(':/new/prefix1/image/newPause.png');}");
        ui->pushButton_playPause->setToolTip("暂停");

        emit repeatPlaySig();
    }
}

//搜索tableview双击
void MainWindow::on_tableView_search_doubleClicked(const QModelIndex &index)
{
    qDebug() << "index: " << index;
    int row=index.row();//获得当前行索引
    ui->tableView_search->selectRow(row); //选中列
    QModelIndex indexID =model->index(row,5);  // 第5列是musicID
    QString strMusicID = model->data(indexID).toString();//获取索引对应位置的数据转为字符串
    qDebug() << "clicked: " << strMusicID;

    player->setMedia(new QMediaPlaylist());
    searchPlaylist->clear();//清除播放列表
    QString url;
    url=QString("https://music.163.com/song/media/outer/url?id=%0").arg(strMusicID);
    searchPlaylist->addMedia(QUrl(url)); //添加返回的音乐到播放列表中
//    player->setMedia(searchPlaylist); //将列表设置到播放器中
    player->setPlaylist(searchPlaylist);
//    searchPlaylist->setCurrentIndex(row);
    player->play(); //播放
    ui->pushButton_playPause->setStyleSheet("QPushButton {border-image: url(':/new/prefix1/image/newPause.png');}");
    ui->pushButton_playPause->setToolTip("暂停");

    getSongPicBySongID(strMusicID); //获取封面图片

    QModelIndex indexSingerName = model->index(row, 1);
    QString strSingerName = model->data(indexSingerName).toString();
    QModelIndex indexSongName = model->index(row, 2);
    QString strMusicSongName = model->data(indexSongName).toString();
    ui->label_songName->setText(strMusicSongName + " - " + strSingerName); //歌曲名+歌手

    QModelIndex indexAlbum = model->index(row, 3);
    QString strMusicAlbum = model->data(indexAlbum).toString(); //歌曲专辑名

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
    QMap<QString, QString> tmpMap; //刷新歌词界面的信息
    tmpMap.insert("singerName", strSingerName);
    tmpMap.insert("songName", strMusicSongName);
    tmpMap.insert("songAlbumName", strMusicAlbum);
    emit modifiedSongInfor(tmpMap);

    allLyricMap.clear();
    GetSongLyricBySongId(strMusicID);//获取歌词

    QModelIndex indexImg = model->index(row, 6);
    QString strMusicImg = model->data(indexImg).toString();
    qDebug() << __LINE__ << strMusicImg;
    networkRequest->setUrl(QUrl(strMusicImg)); //获取专辑图片
    networkManager->get(*networkRequest);
}

//歌曲进度条移动
void MainWindow::slotSongSliderPosChanged(qint64 pos)
{
//    qDebug() << Q_FUNC_INFO << pos << allLyricMap.keys();
    ui->horizontalSlider_songSlider->setValue(pos/1000);
    ui->label_curSongTime->setText(QString("%1").arg(pos/1000/60, 2, 10, QLatin1Char('0')) + ":" + QString("%2").arg(pos/1000%60, 2, 10, QLatin1Char('0')));

    int iValue = pos / 1000;
    if(allLyricMap.contains(iValue))
    {
        qDebug() << __LINE__ << allLyricMap.value(iValue);
        ui->label_lyric->setText(allLyricMap.value(iValue));

        emit changeSingleLyric(iValue);
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
    if ((player->playlist() == hotSongPlaylist) && (!hotSongPlaylist->isEmpty())) //排行榜
    {
        if (hotSongPlaylist->currentIndex() < 0)
        {
            QModelIndex curIndex = ui->tableView_hotSongTable->currentIndex();
            int row = curIndex.row();//获得当前行索引
            disconnect(hotSongPlaylist, &QMediaPlaylist::currentIndexChanged, this, &MainWindow::refreshCurrentMusic);
//            hotSongPlaylist->blockSignals(true);
            hotSongPlaylist->setCurrentIndex(row);
//            hotSongPlaylist->blockSignals(false);
            connect(hotSongPlaylist, &QMediaPlaylist::currentIndexChanged, this, &MainWindow::refreshCurrentMusic);
        }
        ui->tableView_hotSongTable->selectRow(hotSongPlaylist->currentIndex() - 1);
        hotSongPlaylist->setCurrentIndex(hotSongPlaylist->currentIndex() - 1);
        ui->pushButton_playPause->setStyleSheet("QPushButton {border-image: url(':/new/prefix1/image/newPause.png');}");
        ui->pushButton_playPause->setToolTip("暂停");
        player->play();
        return;
    }
    else if ((player->playlist() == songSheetPlaylist) && (!songSheetPlaylist->isEmpty())) //歌单
    {
        if (songSheetPlaylist->currentIndex() < 0)
        {
            QModelIndex curIndex = ui->tableView_singleSheet->currentIndex();
            int row = curIndex.row();//获得当前行索引
            disconnect(songSheetPlaylist, &QMediaPlaylist::currentIndexChanged, this, &MainWindow::refreshCurrentMusic);
            songSheetPlaylist->setCurrentIndex(row);
            connect(songSheetPlaylist, &QMediaPlaylist::currentIndexChanged, this, &MainWindow::refreshCurrentMusic);
        }
        ui->tableView_singleSheet->selectRow(songSheetPlaylist->currentIndex() - 1);
        songSheetPlaylist->setCurrentIndex(songSheetPlaylist->currentIndex() - 1);
        ui->pushButton_playPause->setStyleSheet("QPushButton {border-image: url(':/new/prefix1/image/newPause.png');}");
        ui->pushButton_playPause->setToolTip("暂停");
        player->play();
        return;
    }
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
    searchPlaylist->clear();//清除播放列表
    QString url;
    url=QString("https://music.163.com/song/media/outer/url?id=%0").arg(strMusicID);
    searchPlaylist->addMedia(QUrl(url));                     //添加返回的音乐到播放列表中
    player->setMedia(searchPlaylist);                        //将列表设置到播放器中
    player->play();                                  //播放
    ui->pushButton_playPause->setStyleSheet("QPushButton {border-image: url(':/new/prefix1/image/newPause.png');}");
    ui->pushButton_playPause->setToolTip("暂停");

    QModelIndex indexSingerName = model->index(row, 1);
    QString strSingerName = model->data(indexSingerName).toString();
    QModelIndex indexSongName = model->index(row, 2);
    QString strMusicSongName = model->data(indexSongName).toString();
    ui->label_songName->setText(strMusicSongName + " - " + strSingerName); //歌曲名+歌手

    QModelIndex indexAlbum = model->index(row, 3);
    QString strMusicAlbum = model->data(indexAlbum).toString(); //歌曲专辑名

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
    QMap<QString, QString> tmpMap; //刷新歌词界面的信息
    tmpMap.insert("singerName", strSingerName);
    tmpMap.insert("songName", strMusicSongName);
    tmpMap.insert("songAlbumName", strMusicAlbum);
    emit modifiedSongInfor(tmpMap);

    GetSongLyricBySongId(strMusicID);
//    emit changeLyric(); //更换歌词
}

//下一曲
void MainWindow::on_pushButton_nextSong_clicked()
{
    if ((player->playlist() == hotSongPlaylist) && (!hotSongPlaylist->isEmpty())) //排行榜
    {
        if (hotSongPlaylist->currentIndex() < 0)
        {
            QModelIndex lastIndex = ui->tableView_hotSongTable->currentIndex();
            int row = lastIndex.row();//获得当前行索引
            hotSongPlaylist->blockSignals(true);
            hotSongPlaylist->setCurrentIndex(row);
            hotSongPlaylist->blockSignals(false);
        }
        ui->tableView_hotSongTable->selectRow(hotSongPlaylist->currentIndex() + 1);
        hotSongPlaylist->setCurrentIndex(hotSongPlaylist->currentIndex() + 1);
        ui->pushButton_playPause->setStyleSheet("QPushButton {border-image: url(':/new/prefix1/image/newPause.png');}");
        ui->pushButton_playPause->setToolTip("暂停");
        player->play();
        return;
    }
    else if ((player->playlist() == songSheetPlaylist) && (!songSheetPlaylist->isEmpty())) //歌单
    {
        if (songSheetPlaylist->currentIndex() < 0)
        {
            QModelIndex curIndex = ui->tableView_singleSheet->currentIndex();
            int row = curIndex.row();//获得当前行索引
            disconnect(songSheetPlaylist, &QMediaPlaylist::currentIndexChanged, this, &MainWindow::refreshCurrentMusic);
            songSheetPlaylist->setCurrentIndex(row);
            connect(songSheetPlaylist, &QMediaPlaylist::currentIndexChanged, this, &MainWindow::refreshCurrentMusic);
        }
        ui->tableView_singleSheet->selectRow(songSheetPlaylist->currentIndex() + 1);
        songSheetPlaylist->setCurrentIndex(songSheetPlaylist->currentIndex() + 1);
        ui->pushButton_playPause->setStyleSheet("QPushButton {border-image: url(':/new/prefix1/image/newPause.png');}");
        ui->pushButton_playPause->setToolTip("暂停");
        player->play();
        return;
    }
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
    searchPlaylist->clear();//清除播放列表
    QString url;
    url=QString("https://music.163.com/song/media/outer/url?id=%0").arg(strMusicID);
    searchPlaylist->addMedia(QUrl(url));  //添加返回的音乐到播放列表中
    player->setMedia(searchPlaylist);  //将列表设置到播放器中
    player->play();  //播放
    ui->pushButton_playPause->setStyleSheet("QPushButton {border-image: url(':/new/prefix1/image/newPause.png');}");
    ui->pushButton_playPause->setToolTip("暂停");

    QModelIndex indexSingerName = model->index(row, 1);
    QString strSingerName = model->data(indexSingerName).toString();
    QModelIndex indexSongName = model->index(row, 2);
    QString strMusicSongName = model->data(indexSongName).toString();
    ui->label_songName->setText(strMusicSongName + " - " + strSingerName); //歌曲名+歌手

    QModelIndex indexAlbum = model->index(row, 3);
    QString strMusicAlbum = model->data(indexAlbum).toString(); //歌曲专辑名

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
    QMap<QString, QString> tmpMap; //刷新歌词界面的信息
    tmpMap.insert("singerName", strSingerName);
    tmpMap.insert("songName", strMusicSongName);
    tmpMap.insert("songAlbumName", strMusicAlbum);
    emit modifiedSongInfor(tmpMap);

    GetSongLyricBySongId(strMusicID);
//    emit changeLyric(); //更换歌词
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
    if (ui->stackedWidget->currentIndex() == 0)
    {
        QModelIndex curIndex = ui->tableView_search->currentIndex();
        int row = curIndex.row();//获得当前行索引
        if (row < 0 || row > model->rowCount())
        {
            return;
        }
    }
    else if (ui->stackedWidget->currentIndex() == 2)
    {
        QModelIndex curIndex = ui->tableView_hotSongTable->currentIndex();
        int row = curIndex.row();//获得当前行索引
        if (row < 0 || row > hotSongModel->rowCount())
        {
            return;
        }
    }

    allLyricMap.clear();
    QString strUrl = QString("http://music.163.com/api/song/lyric?id=%1&lv=1&kv=1&tv=-1").arg(musicId);
    networkRequest->setUrl(QUrl(strUrl));
    networkManager->get(*networkRequest);
}

//获取封面(有问题)
void MainWindow::getSongPicBySongID(QString musicID)
{
    QString str = QString("http://api.yimian.xyz/msc/?type=single&id=%1").arg(musicID);
    qDebug() << __LINE__ << str;
    networkRequest->setUrl(QUrl(str));
    networkManager->get(*networkRequest);
}

//通过歌词文件刷新歌词
void MainWindow::refrashLyric()
{
    for (const auto &i : qAsConst(lyricList))
    {
        analysisLyricsFile(i);//解析歌词文件内容
    }
    emit setLyricSig(allLyricMap);
}

//设置控件是否可用
void MainWindow::setControlEnabled(bool bFlag, QPushButton *button)
{
    QList<QPushButton*> buttons = this->findChildren<QPushButton*>();
    for (auto i : qAsConst(buttons))
    {
        i->setEnabled(bFlag);
    }

    QList<QLineEdit*> lines = this->findChildren<QLineEdit*>();
    for (auto j : qAsConst(lines))
    {
        j->setEnabled(bFlag);
    }

    QList<QSlider*> sliders = this->findChildren<QSlider*>();
    for (auto k : qAsConst(sliders))
    {
        k->setEnabled(bFlag);
    }

    ui->pushButton_minimize->setEnabled(true);
    ui->pushButton_quit->setEnabled(true);
    button->setEnabled(true);
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
//    searchPlaylist->clear();//清除播放列表
    QString str = ui->lineEdit_search->text();
    m_offset -= 13;
    str=QString("http://music.163.com/api/search/get/web?csrf_token=hlpretag=&hlposttag=&s={%1}&type=1&offset=%2&total=true&limit=13").arg(str).arg(m_offset);
    networkRequest->setUrl(QUrl(str));
    networkManager->get(*networkRequest);
}

//下一页
void MainWindow::on_pushButton_nextPage_clicked()
{
//    searchPlaylist->clear();//清除播放列表
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
        qDebug() << __LINE__ << currentText << totalTime << match.captured(1).toInt() << match.captured(2).toInt();

        durationTime = totalTime;
        if (player->playlist() == hotSongPlaylist || player->playlist() == songSheetPlaylist) //热歌榜的歌曲总时间从这里获取
        {
            ui->horizontalSlider_songSlider->setMaximum(totalTime/1000);
            QString time = QString("%1:%2").arg(totalTime/1000/60, 2, 10, QLatin1Char('0')).arg(totalTime/1000%60, 2, 10, QLatin1Char('0'));
            ui->label_totalSongTime->setText(time);
        }

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

//切换歌后更新界面内容
void MainWindow::refreshCurrentMusic(int iCurrentRow)
{
    QMediaPlaylist *playlist = qobject_cast<QMediaPlaylist *>(sender());
    qDebug()<<"lineEdit->objectName()=="<<(playlist==hotSongPlaylist) << iCurrentRow;
    ui->label_lyric->clear();

    if (playlist == hotSongPlaylist)
    {
        QModelIndex curIndex = ui->tableView_hotSongTable->currentIndex();
        int row = curIndex.row();//获得当前行索引
        qDebug() << __LINE__ << iCurrentRow << row << curIndex;
        if (iCurrentRow < 0)
        {
            ui->tableView_hotSongTable->selectRow(row);
            disconnect(hotSongPlaylist, &QMediaPlaylist::currentIndexChanged, this, &MainWindow::refreshCurrentMusic);
//            hotSongPlaylist->blockSignals(true); //未生效，暂不清楚原因
//            QSignalBlocker blocker(hotSongPlaylist); //临时屏蔽列表信号
            hotSongPlaylist->setCurrentIndex(row);
//            hotSongPlaylist->blockSignals(false);
            connect(hotSongPlaylist, &QMediaPlaylist::currentIndexChanged, this, &MainWindow::refreshCurrentMusic);
            return;
        }
        if (iCurrentRow != row) //经常会自己跳到第一行，暂不清楚原因，避免重复获取歌词
        {
            return;
        }

        QModelIndex indexID = hotSongModel->index(iCurrentRow,5);  // 第5列是musicID
        QString strMusicID = hotSongModel->data(indexID).toString();//获取索引对应位置的数据转为字符串
        qDebug() << __LINE__ << strMusicID << iCurrentRow;

        GetSongLyricBySongId(strMusicID); //热歌榜没有歌曲时间，需要重新获取

        QModelIndex indexSingerName = hotSongModel->index(iCurrentRow, 1);
        QString strSingerName = hotSongModel->data(indexSingerName).toString();
        QModelIndex indexSongName = hotSongModel->index(iCurrentRow, 2);
        QString strMusicSongName = hotSongModel->data(indexSongName).toString();
        qDebug() << __LINE__ << strMusicSongName << strSingerName;
        ui->label_songName->setText(strMusicSongName + " - " + strSingerName); //歌曲名+歌手

        QModelIndex indexAlbum = hotSongModel->index(iCurrentRow, 3);
        QString strMusicAlbum = hotSongModel->data(indexAlbum).toString(); //歌曲专辑名

        QMap<QString, QString> tmpMap; //刷新歌词界面的信息
        tmpMap.insert("singerName", strSingerName);
        tmpMap.insert("songName", strMusicSongName);
        tmpMap.insert("songAlbumName", strMusicAlbum);
        emit modifiedSongInfor(tmpMap);
    }
    else if (playlist == searchPlaylist)
    {
        ui->tableView_search->selectRow(iCurrentRow); //,选中列
        QModelIndex indexID =model->index(iCurrentRow,5);  // 第5列是musicID
        QString strMusicID = model->data(indexID).toString();//获取索引对应位置的数据转为字符串
        qDebug() << "clicked: " << strMusicID;

        QModelIndex indexSingerName = model->index(iCurrentRow, 1);
        QString strSingerName = model->data(indexSingerName).toString();
        QModelIndex indexSongName = model->index(iCurrentRow, 2);
        QString strMusicSongName = model->data(indexSongName).toString();
        ui->label_songName->setText(strMusicSongName + " - " + strSingerName); //歌曲名+歌手

        QModelIndex indexAlbum = model->index(iCurrentRow, 3);
        QString strMusicAlbum = model->data(indexAlbum).toString(); //歌曲专辑名

        QModelIndex indexTime = model->index(iCurrentRow, 4);
        QString strMusicTime = model->data(indexTime).toString();
        ui->label_totalSongTime->setText(strMusicTime); // 歌曲时长

        QStringList timeList = strMusicTime.split(":");
        QString strMinute = timeList.at(0);
        QString strSeconds = timeList.at(1);
        musicDuration = strMinute.toInt() * 60 + strSeconds.toInt();
        qDebug() << "musicDuration: " << musicDuration;
        ui->horizontalSlider_songSlider->setMaximum(strMinute.toInt() * 60 + strSeconds.toInt()); //歌曲进度条

        QMap<QString, QString> tmpMap; //刷新歌词界面的信息
        tmpMap.insert("singerName", strSingerName);
        tmpMap.insert("songName", strMusicSongName);
        tmpMap.insert("songAlbumName", strMusicAlbum);
        emit modifiedSongInfor(tmpMap);
    }
    else if (playlist == songSheetPlaylist)
    {
        QModelIndex curIndex = ui->tableView_singleSheet->currentIndex();
        int row = curIndex.row();//获得当前行索引
        qDebug() << __LINE__ << iCurrentRow << row << curIndex;
        if (iCurrentRow < 0)
        {
            ui->tableView_singleSheet->selectRow(row);
            disconnect(songSheetPlaylist, &QMediaPlaylist::currentIndexChanged, this, &MainWindow::refreshCurrentMusic);
            songSheetPlaylist->setCurrentIndex(row);
            connect(songSheetPlaylist, &QMediaPlaylist::currentIndexChanged, this, &MainWindow::refreshCurrentMusic);
            return;
        }
        if (iCurrentRow != row) //经常会自己跳到第一行，暂不清楚原因，避免重复获取歌词
        {
            return;
        }

        QModelIndex indexID = songSheetModel->index(iCurrentRow,5);  // 第5列是musicID
        QString strMusicID = songSheetModel->data(indexID).toString();//获取索引对应位置的数据转为字符串
        qDebug() << __LINE__ << strMusicID << iCurrentRow;

        GetSongLyricBySongId(strMusicID); //热歌榜没有歌曲时间，需要重新获取

        QModelIndex indexSingerName = songSheetModel->index(iCurrentRow, 1);
        QString strSingerName = songSheetModel->data(indexSingerName).toString();
        QModelIndex indexSongName = songSheetModel->index(iCurrentRow, 2);
        QString strMusicSongName = songSheetModel->data(indexSongName).toString();
        qDebug() << __LINE__ << strMusicSongName << strSingerName;
        ui->label_songName->setText(strMusicSongName + " - " + strSingerName); //歌曲名+歌手

        QModelIndex indexAlbum = songSheetModel->index(iCurrentRow, 3);
        QString strMusicAlbum = songSheetModel->data(indexAlbum).toString(); //歌曲专辑名

        QMap<QString, QString> tmpMap; //刷新歌词界面的信息
        tmpMap.insert("singerName", strSingerName);
        tmpMap.insert("songName", strMusicSongName);
        tmpMap.insert("songAlbumName", strMusicAlbum);
        emit modifiedSongInfor(tmpMap);
    }
    else
    {
        qDebug() << "check error!";
    }
}

//热歌榜歌曲双击播放
void MainWindow::on_tableView_hotSongTable_doubleClicked(const QModelIndex &index)
{
    qDebug() << "index: " << index;
    int row=index.row();//获得当前行索引
//    ui->tableView_hotSongTable->selectRow(row); //选中列
    QModelIndex indexID = hotSongModel->index(row,5);  // 第5列是musicID
    QString strMusicID = hotSongModel->data(indexID).toString();//获取索引对应位置的数据转为字符串
    qDebug() << "clicked: " << strMusicID << row;

    player->setPlaylist(hotSongPlaylist);
    hotSongPlaylist->setCurrentIndex(row);
    player->play(); //播放
    ui->pushButton_playPause->setStyleSheet("QPushButton {border-image: url(':/new/prefix1/image/newPause.png');}");
    ui->pushButton_playPause->setToolTip("暂停");

    QModelIndex indexImg = hotSongModel->index(row, 6);
    QString strMusicImg = hotSongModel->data(indexImg).toString();
    if (strMusicImg.contains("https"))
    {
        strMusicImg = "http" + strMusicImg.mid(5, -1);
    }
    qDebug() << __LINE__ << strMusicImg;
    networkRequest->setUrl(QUrl(strMusicImg)); //获取专辑图片
    networkManager->get(*networkRequest);
}

//显示搜索界面右键菜单
void MainWindow::slotDealSearchMenu(QPoint point)
{
    QModelIndex index = ui->tableView_search->indexAt(point);
    if (index.isValid())
    {
        popMenu->exec(QCursor::pos()); // 菜单出现的位置为当前鼠标的位置
    }
    qDebug() << __LINE__ << index;
}

//排行榜右鍵菜單
void MainWindow::slotDealRankMenu(QPoint point)
{
    QModelIndex index = ui->tableView_hotSongTable->indexAt(point);
    if (index.isValid())
    {
        rankMenu->exec(QCursor::pos()); // 菜单出现的位置为当前鼠标的位置
    }
    qDebug() << __LINE__ << index;
}

//切换榜单
void MainWindow::slotDealRankAction()
{
    QAction *action = qobject_cast<QAction *>(sender());
    qDebug() << "action->objectName()==" << action->objectName();
    long long rankId = 0;
    if (action == newMenu) //新歌榜
    {
        hotSongModel->clear();
        ui->tableView_hotSongTable->setModel(hotSongModel);
        rankId = 3779629;
    }
    else if (action == upMenu) //飙升榜
    {
        hotSongModel->clear();
        ui->tableView_hotSongTable->setModel(hotSongModel);
        rankId = 19723756;
    }
    else if (action == hotMenu) //热歌榜
    {
        hotSongModel->clear();
        ui->tableView_hotSongTable->setModel(hotSongModel);
        rankId = 3778678;
    }
    else if (action == dyMenu) //抖音榜
    {
        hotSongModel->clear();
        ui->tableView_hotSongTable->setModel(hotSongModel);
        rankId = 2250011882;
    }

    QString strUrl = QString("http://api.yimian.xyz/msc/?type=playlist&id=%1").arg(rankId);
    qDebug() << __LINE__ << strUrl;
    networkRequest->setUrl(QUrl(strUrl));
    networkManager->get(*networkRequest);

    ui->label_rankLoading->show();
}

//歌单右键菜单
void MainWindow::slotDealSheetMenu(QPoint point)
{
    QModelIndex index = ui->tableView_singleSheet->indexAt(point);
    if (index.isValid())
    {
        sheetMenu->exec(QCursor::pos()); // 菜单出现的位置为当前鼠标的位置
    }
    qDebug() << __LINE__ << index;
}

//返回歌单
void MainWindow::slotBackAction()
{
//    QString str = "http://music.cyrilstudio.top/recommend/resource"; //每日推荐歌单
    QString str = "http://music.cyrilstudio.top/top/playlist/highquality";

    networkRequest->setUrl(QUrl(str));
    networkManager->get(*networkRequest);

    ui->stackedWidget_songSheet->setCurrentIndex(0);
    songSheetModel->clear();
    ui->tableView_singleSheet->setModel(songSheetModel);
    ui->label_sheetLoading->show();
}

//单个歌单详情
void MainWindow::slotShowSingleSheet(QString id)
{
    qDebug() << __LINE__ << id;

    setControlEnabled(false, ui->pushButton_songSheet);

    QString strUrl = QString("http://api.yimian.xyz/msc/?type=playlist&id=%1").arg(id);
    qDebug() << __LINE__ << strUrl;
    networkRequest->setUrl(QUrl(strUrl));
    networkManager->get(*networkRequest);

    ui->stackedWidget_songSheet->setCurrentIndex(1);
}

//播放mv
void MainWindow::slotPlayMV()
{
    QModelIndex lastIndex = ui->tableView_search->currentIndex();
    int row = lastIndex.row();//获得当前行索引
    if (row < 0 || row > 12)
    {
        return;
    }

    QModelIndex MVIDIndex = model->index(row,7);  // 第7列是MVID
    QString strMVID = model->data(MVIDIndex).toString();//获取索引对应位置的数据转为字符串
    qDebug() << __LINE__ << row << MVIDIndex << strMVID;

    if (strMVID.toInt() != 0)
    {
        QString strUrl = QString("http://music.163.com/api/mv/detail?id=%1&type=mp4").arg(strMVID);

        networkRequest->setUrl(QUrl(strUrl));
        networkManager->get(*networkRequest);
        if (ui->label_lyric->text() == "未获取到MV")
        {
            ui->label_lyric->clear();
        }

    }
    else
    {
        ui->label_lyric->setText("未获取到MV");
//        ui->label_lyric->clear();
    }

    player->pause();
    ui->pushButton_playPause->setStyleSheet("QPushButton {border-image: url(':/new/prefix1/image/newPlay.png');}");
    ui->pushButton_playPause->setToolTip("播放");
}

//歌单歌曲播放
void MainWindow::on_tableView_singleSheet_doubleClicked(const QModelIndex &index)
{
    qDebug() << "index: " << index;
    int row=index.row();//获得当前行索引
    QModelIndex indexID = songSheetModel->index(row,5);  // 第5列是musicID
    QString strMusicID = songSheetModel->data(indexID).toString();//获取索引对应位置的数据转为字符串
    qDebug() << "clicked: " << strMusicID << row;

    player->setPlaylist(songSheetPlaylist);
    songSheetPlaylist->setCurrentIndex(row);
    player->play(); //播放
    ui->pushButton_playPause->setStyleSheet("QPushButton {border-image: url(':/new/prefix1/image/newPause.png');}");
    ui->pushButton_playPause->setToolTip("暂停");

    QModelIndex indexImg = songSheetModel->index(row, 6);
    QString strMusicImg = songSheetModel->data(indexImg).toString();
    if (strMusicImg.contains("https"))
    {
        strMusicImg = "http" + strMusicImg.mid(5, -1);
    }
    qDebug() << __LINE__ << strMusicImg;
    networkRequest->setUrl(QUrl(strMusicImg)); //获取专辑图片
    networkManager->get(*networkRequest);
}

