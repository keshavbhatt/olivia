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
#include <QBuffer>
#include <QDateTime>
#include <QColorDialog>
#include <QPropertyAnimation>


#include "ui_track.h"
#include "store.h"
#include "radio.h"
#include "onlinesearchsuggestion.h"
#include "settings.h"
#include "paginator.h"
#include "youtube.h"

#include "ui_settings.h"
#include "ui_minimode.h"



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
    Q_INVOKABLE void playRadioFromWeb(QVariant streamDetails);
    Q_INVOKABLE void setSearchTermAndOpenYoutube(QVariant term);
    Q_INVOKABLE void clear_youtubeSearchTerm();
    Q_INVOKABLE void playLocalTrack(QVariant songId);

    QString youtubeSearchTerm;
    bool saveTracksAfterBuffer;

protected slots:
    void resizeEvent(QResizeEvent *resizeEvent);
    bool eventFilter(QObject *obj, QEvent *event);
    void closeEvent(QCloseEvent *event);



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

    void on_radioVolumeSlider_valueChanged(int value);
    void on_stop_clicked();
//    void on_radioSeekSlider_sliderReleased();
    void on_radioSeekSlider_sliderMoved(int position);
    void on_play_pause_clicked();
    void on_right_list_itemDoubleClicked(QListWidgetItem *item);
    void on_menu_clicked();
    void showTrackOption();
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
    //void quitApp();
    void radioPosition(int pos);
    void radioDuration(int dur);
//    void radioEOF(QString value);
    void radio_demuxer_cache_duration_changed(double, double radio_playerPosition);
    void init_search_autoComplete();
    void saveTrack(QString format);
    void ytdlFinished(int code);
    void processYtdlQueue();
    void on_settings_clicked();

    void init_settings();
    void init_miniMode();
    bool checkEngine();
    void download_engine_clicked();
    void slot_netwManagerFinished(QNetworkReply *reply);
//    void down_progress(qint64 pos, qint64 tot);
    void evoke_engine_check();
    void browse_youtube();
    void on_right_list_2_itemDoubleClicked(QListWidgetItem *item);
    void on_filter_olivia_textChanged(const QString &arg1);

    void hideListItems(QListWidget *list);
    void fillOliviaMetaList(QListWidget *list);
    void filterList(const QString &arg1, QListWidget *list);
    void on_filter_youtube_textChanged(const QString &arg1);



    void zoomin();
    void zoomout();
    void setZoom(float);
    void listItemDoubleClicked(QListWidget *list, QListWidgetItem *item);
    void on_miniMode_clicked();

    void add_colors_to_color_widget();
    void set_app_theme(QColor rgb);
    void customColor();
    void on_tabWidget_currentChanged(int index);

    void loadSettings();
    void dynamicThemeChanged(bool enabled);


    void on_close_clicked();
    void on_fullScreen_clicked();
    void on_minimize_clicked();
    void on_maximize_clicked();
    void reloadREquested(QString dataType, QString query);


    void assignNextTrack(QListWidget *list, int index);
    void assignPreviousTrack(QListWidget *list, int index);



    void setWavefrom(QString urlStr);
    void on_radioSeekSlider_valueChanged(int value);
    void setCountry(QString country);
    void setTrackItemNowPlaying();
    void on_ytdlStopAll_clicked();

    void on_ytdlRefreshAll_clicked();

    void on_right_list_2_itemClicked(QListWidgetItem *item);

    void on_right_list_itemClicked(QListWidgetItem *item);
    void trackItemClicked(QListWidget *listWidget, QListWidgetItem *item);

    void get_engine_version_info();
    void compare_versions(QString date, QString n_date);
    void check_engine_updates();
private:
    QString core_local_date,core_remote_date;
    float zoom;
    int horizontalDpi;
    QPoint oldPos,oldPosMiniWidget;

    Ui::MainWindow *ui;
    Ui::track track_ui;
    Ui::miniMode_form miniMode_ui;
    QWidget *miniModeWidget;
    Ui::settings settingsUi;
    QWidget *settingsWidget;
    settings *settUtils;
    Youtube *youtube;

   // QSettings * settings;
    QString pageType;
    int currentResultPage;
    bool isLoadingResults;
    QString offsetstr;

    QString gotoAlbumId,gotoArtistId;
    QString nowPlayingSongId;


    store *store_manager = nullptr;
    radio *radio_manager = nullptr;
    paginator *pagination_manager = nullptr;
    QString themeColor = "4,42,59,0.2";

    QStringList searchSuggestionList;

    onlineSearchSuggestion * _onlineSearchSuggestion_ = nullptr;

    QList<QStringList> ytdlQueue ;

    QProcess * ytdlProcess = nullptr;
    QFile *core_file;

    QList<QString> OliviaMetaList; //for search purpose

    int left_panel_width;
    bool animationRunning = false;

    QSettings settingsObj;
    QString setting_path;

    QStringList color_list ;

};

class SelectColorButton : public QPushButton
{
    Q_OBJECT
signals:

    void setCustomColor(QColor);

public:

    void setColor( const QColor& color );
    const QColor& getColor();

public slots:
    void updateColor();
    void changeColor();

private:
    QColor color;


};

#endif // MAINWINDOW_H
