#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QNetworkDiskCache>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkCookieJar>
#include <QWebHistory>
#include <QWebView>
#include <QWebFrame>
#include <QStandardPaths>
#include <QSettings>


#include <QStringListModel>
#include <QStandardItemModel>
#include <QFile>
#include <QTimer>
#include <QDir>
#include <QProgressBar>
#include <QMessageBox>
#include <QDesktopWidget>
#include <QFileDialog>

#include <QWebFrame>
#include <QSizePolicy>

#include <QDesktopServices>
#include <QStandardPaths>
#include <QTextCodec>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMovie>

#include <QWidget>
#include <QNetworkDiskCache>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkCookieJar>
#include <QWebHistory>
#include <QWebView>
#include <QWebFrame>
#include <QContextMenuEvent>
#include <QWebHitTestResult>
#include <QMenu>
#include <QWebElement>
#include <QWebView>
#include <QProcess>
#include <QMouseEvent>
#include <QClipboard>
#include <QListWidgetItem>
#include <QSettings>
#include <QMediaPlayer>
#include <QBuffer>
#include <QDateTime>
#include "ui_track.h"
#include "store.h"
#include "radio.h"
#include "onlinesearchsuggestion.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    Q_INVOKABLE void resultLoaded();
    Q_INVOKABLE void addToQueue(QString id, QString title, QString artist, QString album, QString base64, QString dominantColor, QString songId, QString albumId, QString artistId);
    Q_INVOKABLE QString getTerm();
    Q_INVOKABLE void showAjaxError();
    Q_INVOKABLE void setThemeColor(QString); //sets themeColor in mainWindow
    Q_INVOKABLE void playRadioFromWeb(QVariant urlVariant);


protected slots:
    void resizeEvent(QResizeEvent *resizeEvent);
    bool eventFilter(QObject *obj, QEvent *event);
private slots:
    void init_app();
    void init_webview();
    void on_left_list_currentRowChanged(int currentRow);

    void browse();
    void search(QString offset);
    void webViewLoaded(bool loaded);
    void getAudioStream(QString ytIds, QString songId);
    void on_search_returnPressed();
//    void youtubeDlFinished(int code, QProcess::ExitStatus exitStatus);
    void ytdlReadyRead();




    //MEDIAPLAYER
    void init_offline_storage();
    void setPlayerPosition(qint64 position);
    QString getMediaStatusString(QMediaPlayer::MediaStatus);
    QString getPlayerStateString(QMediaPlayer::State);
    void on_volumeSlider_valueChanged(int value);
    void on_stop_clicked();
    void on_seekSlider_sliderReleased();
    void on_seekSlider_sliderMoved(int position);


     void on_play_pause_clicked();




    void on_right_list_itemDoubleClicked(QListWidgetItem *item);


    void on_menu_clicked();


    void showTrackOption();


    void saveTrack(QString format);
    void getNowPlayingTrackId();


    void loadPlayerQueue();
    void keyPressEvent(QKeyEvent *event);
    void show_top();
    void show_saved_songs();
    void show_local_saved_songs();
    void show_saved_albums();
    void show_saved_artists();
    void internet_radio();
    void radioStatus(QString);
    void quitApp();
    void radioPosition(int pos);
    void radioDuration(int dur);
    void init_search_autoComplete();
private:
    Ui::MainWindow *ui;
    Ui::track track_ui;

    QSettings *settings;
    QString pageType;
    int currentResultPage;
    bool isLoadingResults;
    QString offsetstr;



    store *store_manager = nullptr;
    radio *radio_manager = nullptr;
    QString nowPlayingSongId;
    QString themeColor = "4,42,59,0.2";

    QStringList searchSuggestionList;

    onlineSearchSuggestion * _onlineSearchSuggestion_ = nullptr;
};

#endif // MAINWINDOW_H
