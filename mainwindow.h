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
    void GetSongLyricBySongId(QString Id);

public:

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

private:
    Ui::MainWindow *ui;

    QNetworkAccessManager *networkManager;
    QNetworkRequest *networkRequest;

    QStandardItemModel* model;
    QString durationTime;
    QString positionTime;
    QNetworkAccessManager* manager;
    QMediaPlayer *player;
    QMediaPlaylist *playlist;
    QTextDocument* doc;
    QTextBlock textLine;
    int musicId;//音乐ID
    int musicDuration; //音乐时长（未处理）
    int mvId; //mv
    int iCurVolume; //当前音量
    bool bMute; //是否静音
    QString musicName,singerName,albumName;
    QByteArray searchInfo;

};
#endif // MAINWINDOW_H
