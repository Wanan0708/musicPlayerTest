#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDebug>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>
#include <QVariant>
#include <QByteArray>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QUrl>
#include <QPixmap>
#include <QSize>
#include <QMediaPlayer>
#include <QMediaPlaylist>
#include <QTextDocument>
#include <QTextBlock>
#include <QStandardItemModel>
#include <QThread>
#include <QList>
#include <QMessageBox>
#include <QGraphicsDropShadowEffect>
#include <QPainter>
#include <QBitmap>
#include <QtMath>
#include <QThread>
#include <QMovie>
#include <QFile>
#include <QVideoWindowControl>
#include <QVideoWidget>
#include <QMenu>
#include <QAction>
#include "lyricshow.h"
#include "ui_lyricshow.h"
#include "datebase.h"
#include "mvshow.h"
#include "singlesheetshow.h"

#define PAGE_SEARCH 0
#define PAGE_SONGSHEET 1
#define PAGE_RANKSHEET 2
#define PAGE_SETTING 3
#define PAGE_USERINFO 4

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void init();
    void initControlStylesheet();
    void GetSongLyricBySongId(QString musicId);
    void getSongPicBySongID(QString musicID);
    void refrashLyric();
    void setControlEnabled(bool bFlag, QPushButton *button);

signals:
    void changeLyric();
    void changeSongImage(QPixmap pixmap);
    void modifiedSongInfor(QMap<QString, QString> map);
    void setLyricSig(QMap<int, QString>);
    void changeSingleLyric(int pos);
    void repeatPlaySig();
    void setSheetImageSig();

private slots:
    void getWeight(int id);

    void on_pushButton_quit_clicked();

    void on_pushButton_minimize_clicked();

    void databack(QNetworkReply *reply);

    void on_pushButton_playPause_clicked();

    void on_lineEdit_search_returnPressed();

    void on_tableView_search_doubleClicked(const QModelIndex &index);

    void slotSongSliderPosChanged(qint64 pos);

    void on_horizontalSlider_volumeSlider_valueChanged(int value);

    void on_pushButton_lastSong_clicked();

    void on_pushButton_nextSong_clicked();

    void on_horizontalSlider_songSlider_sliderMoved(int position);

    void on_horizontalSlider_songSlider_sliderPressed();

    void on_horizontalSlider_songSlider_sliderReleased();

    void on_pushButton_volume_clicked();

    void on_pushButton_minPhoto_clicked();

    void on_pushButton_lastPage_clicked();

    void on_pushButton_nextPage_clicked();

    bool analysisLyricsFile(QString line);

    void on_horizontalSlider_songSlider_actionTriggered(int action);

    void on_pushButton_saveSetting_clicked();

    void refreshCurrentMusic(int iCurrentRow);

    void on_tableView_hotSongTable_doubleClicked(const QModelIndex &index);

    void slotDealSearchMenu(QPoint point);

    void slotPlayMV();

    void slotDealRankMenu(QPoint point);

    void slotDealRankAction();

    void slotDealSheetMenu(QPoint point);

    void slotBackAction();

    void slotShowSingleSheet(QString id);

    void on_tableView_singleSheet_doubleClicked(const QModelIndex &index);

private:
    Ui::MainWindow *ui;

    lyricShow lyric; //歌词界面
    mvShow *mvWidget; //MV
    SingleSheetShow *sheetShow; //歌单

    QNetworkAccessManager *networkManager;
    QNetworkRequest *networkRequest;

    QStandardItemModel* model; //填充搜到的歌曲tableview单元格
    QStandardItemModel* hotSongModel; //热歌榜model
    QStandardItemModel* songSheetModel; //歌单model
    QString durationTime; //音乐时长
    QString imgAddress; //专辑图片地址
    QMediaPlayer *player; //播放音乐
    QMediaPlaylist *searchPlaylist; //搜索页播放列表
    QMediaPlaylist *hotSongPlaylist; //热歌榜播放列表
    QMediaPlaylist *songSheetPlaylist; //歌单播放列表
    QTextDocument* doc;
    QTextBlock textLine;
    int musicId;//音乐ID
    int musicDuration; //音乐时长（未处理）
    int mvId; //mvID
    int iCurVolume; //当前音量
    int m_offset; //翻页
    bool bMute; //是否静音
    bool bSearchSongSheet; //避免重复搜索歌单
    bool bHotSongList; //避免重复搜索热歌榜
    bool bSongSheet; //避免重复搜索歌单
    QString musicName,singerName,albumName;
    QByteArray searchInfo; //存储接收到的信息
    QMap<int, QString> allLyricMap; //存储歌词 <时间，歌词>
    QStringList lyricList; //未处理过的歌词

    QMenu *popMenu; //搜索界面右键菜单
    QAction *playMVAction; //播放mv选项
    QMenu *rankMenu; //排行榜右键菜单
    QAction *newMenu;
    QAction *upMenu;
    QAction *hotMenu;
    QAction *dyMenu;
    QMenu *sheetMenu; //歌单右键菜单
    QAction *backMenu;

    QMediaPlayer *videoPlayer; //播放mv
    QMediaPlaylist *videoPlaylist; //mv播放列表
    QVideoWidget *videoWidget; //播放mv窗口

    QEventLoop loop;
};
#endif // MAINWINDOW_H
