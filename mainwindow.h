#ifndef MAINWINDOW_H
#define MAINWINDOW_H


#include <QClipboard>
#include <QColorDialog>
#include <QDateTime>
#include <QDesktopServices>
#include <QDesktopWidget>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QListWidgetItem>
#include <QMainWindow>
#include <QMenu>
#include <QMessageBox>
#include <QMouseEvent>
#include <QMovie>
#include <QNetworkAccessManager>
#include <QNetworkCookieJar>
#include <QNetworkDiskCache>
#include <QNetworkReply>
#include <QProcess>
#include <QProgressBar>
#include <QPropertyAnimation>
#include <QSettings>
#include <QSizePolicy>
#include <QStandardPaths>
#include <QTextCodec>
#include <QTimer>
#include <QWebElement>
#include <QWebFrame>
#include <QWebHistory>
#include <QWebView>
#include <QWidget>

#include "equalizer.h"
#include "lyrics.h"
#include "onlinesearchsuggestion.h"
#include "paginator.h"
#include "radio.h"
#include "settings.h"
#include "store.h"
#include "videooption.h"
#include "youtube.h"
#include "download_widget.h"
#include "plugins/mpris/mprisplugin.h"
#include "stringchangewatcher.h"
#include "similartracks.h"
#include "trackproperties.h"
#include <analytics.h>
#include <other/connect.h>


#include "ui_minimode.h"
#include "ui_settings.h"
#include "ui_track.h"
#include "ui_smart_mode.h"
#include "ui_toast.h"




namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    Q_INVOKABLE QString getTerm();
    Q_INVOKABLE void addToQueue(QString id, QString title, QString artist, QString album, QString base64, QString dominantColor, QString songId, QString albumId, QString artistId);
    Q_INVOKABLE void clear_youtubeSearchTerm();
    Q_INVOKABLE void playLocalTrack(QVariant songId);
    Q_INVOKABLE void addToQueueFromLocal(QVariant songIdVar);
    Q_INVOKABLE void playSongById(QVariant songIdVar);
    Q_INVOKABLE void playRadioFromWeb(QVariant streamDetails);
    Q_INVOKABLE void saveRadioChannelToFavourite(QVariant channelInfo);
    Q_INVOKABLE void setSearchTermAndOpenYoutube(QVariant term);
    Q_INVOKABLE void setThemeColor(QString); //sets themeColor in mainWindow
    Q_INVOKABLE void showAjaxError();
    Q_INVOKABLE void web_watch_video(QVariant data);
    Q_INVOKABLE void playVideo(QString trackId);
    Q_INVOKABLE void browse_youtube();
    Q_INVOKABLE void browse_soundcloud();
    Q_INVOKABLE void delete_song_cache(QVariant track_id);
    Q_INVOKABLE void remove_song(QVariant track_id);
    Q_INVOKABLE void addToSimilarTracksQueue(const QVariant Base64andDominantColor);
    Q_INVOKABLE void addPlaylistByData(QString data);
    Q_INVOKABLE void checkForPlaylist();
    Q_INVOKABLE void hidePlaylistButton();
    Q_INVOKABLE void on_playlistLoaderButtton_clicked();
    Q_INVOKABLE void removeFromFavourite(QString songId);
    Q_INVOKABLE void showTrackProperties(QString songId);
    Q_INVOKABLE void delete_video_cache(QVariant track_id);




    QString youtubeSearchTerm;
    bool saveTracksAfterBuffer;

public slots:
    void set_eq(QString eq_args);
    void disable_eq();
    void showToast(QString message);

protected slots:
    void closeEvent(QCloseEvent *event);
    bool eventFilter(QObject *obj, QEvent *event);
    void keyPressEvent(QKeyEvent *event);
    void resizeEvent(QResizeEvent *resizeEvent);

private slots:
    bool checkEngine();
    bool hasDeadTracks(QListWidget *queue);
    bool hasUnCachedTracks(QListWidget *queue);
    bool isNumericStr(const QString str);
    bool trackIsBeingProcessed(QString songId);
    QString getExpireTime(const QString urlStr);
    QString menuStyle();
    void add_colors_to_color_widget();
    void assignNextTrack(QListWidget *list, int index);
    void assignPreviousTrack(QListWidget *list, int index);
    void browse();
    void check_engine_updates();
    void clear_queue();
    void compare_versions(QString date, QString n_date);
    void customColor();
    void deleteProcess(int code);
    void download_engine_clicked();
    void dynamicThemeChanged(bool enabled);
    void evoke_engine_check();
    void fillOliviaMetaList(QListWidget *list);
    void filterList(const QString &arg1, QListWidget *list);
    void get_engine_version_info();
    void getAudioStream(QString ytIds, QString songId);
    void hideListItems(QListWidget *list);
    void init_app();
    void init_eq();
    void init_lyrics();
    void init_miniMode();
    void init_offline_storage();
    void init_radio();
    void init_search_autoComplete();
    void init_settings();
    void init_webview();
    void init_videoOption();
    void installEventFilters();
    void internet_radio();
    void listItemDoubleClicked(QListWidget *list, QListWidgetItem *item);
    void loadPlayerQueue();
    void loadSettings();
    void on_close_clicked();
    void on_eq_clicked();
    void on_filter_olivia_textChanged(const QString &arg1);
    void on_filter_youtube_textChanged(const QString &arg1);
    void on_fullScreen_clicked();
    void on_left_list_currentRowChanged(int currentRow);
    void on_maximize_clicked();
    void on_menu_clicked();
    void on_minimize_clicked();
    void on_miniMode_clicked();
    void on_olivia_queue_options_clicked();
    void on_play_pause_clicked();
    void on_radioSeekSlider_sliderMoved(int position);
    void on_radioVolumeSlider_valueChanged(int value);
    void on_youtube_list_itemClicked(QListWidgetItem *item);
    void on_youtube_list_itemDoubleClicked(QListWidgetItem *item);
    void on_olivia_list_itemClicked(QListWidgetItem *item);
    void on_smart_list_itemClicked(QListWidgetItem *item);
    void on_olivia_list_itemDoubleClicked(QListWidgetItem *item);
    void on_search_returnPressed();
    void on_settings_clicked();
    void on_stop_clicked();
    void on_tabWidget_currentChanged(int index);
    void on_youtube_queue_options_clicked();
    void on_ytdlRefreshAll_clicked();
    void on_ytdlStopAll_clicked();
    void processYtdlQueue();
    void queueShowOption(QListWidget *queue);
    void radio_demuxer_cache_duration_changed(double, double radio_playerPosition);
    void radioDuration(int dur);
    void radioPosition(int pos);
    void radioProcessReady();
    void radioStatus(QString);
    void recommendations();
    void reloadREquested(QString dataType, QString query);
    void restart_required();
    void saveTrack(QString format);
    void search(QString offset);
    void set_app_theme(QColor rgb);
    void setCountry(QString country);
    void setPlayerPosition(qint64 position);
    void setTrackItemNowPlaying();
    void setZoom(qreal);
    void show_local_saved_songs();
    void show_top();
    void showTrackOption();
    void slot_netwManagerFinished(QNetworkReply *reply);
    void trackItemClicked(QListWidget *listWidget, QListWidgetItem *item);
    void transparency_changed(int value); //from settings widget
    void webViewLoaded(bool loaded);
    void ytdlFinished(int code);
    void ytdlReadyRead();
    void zoomin();
    void zoomout();

    //return pain text version of text which contains html symbolic notations
    QString htmlToPlainText(QString html){
        QTextDocument text;
        text.setHtml(html.replace("\\\"","'"));
        return text.toPlainText();
    }

    void on_shuffle_toggled(bool checked);

    void on_hideDebug_clicked();

    void videoOptionDownloadRequested(QStringList metaData, QStringList formats);
    void init_downloadWidget();
    void leftListChangeCurrentRow(int row);
    void getEnabledTracks(QListWidget*);

    void init_mpris();

    void show_local_saved_videos();
    void on_jump_to_nowplaying_clicked();

    void getRecommendedTracksForAutoPlay(QString songId);
    void on_show_hide_smart_list_button_clicked();

    void init_similar_tracks();
    void on_smart_list_itemDoubleClicked(QListWidgetItem *item);
    void startGetRecommendedTrackForAutoPlayTimer(QString songId);
    bool similarTracksListHasTrackToBeRemoved();
    void showPayPalDonationMessageBox();
    void prepareSimilarTracks();
    void similarTracksProcessHelper();
    void listClearSelection(QString listName);
    void reverseYtdlProcessList();
    void prepareTrack(QString songId, QString query, QString millis, QListWidget *list);
    void findTrackInQueue(QString songId);
    void show_recently_played();

    void show_liked_songs();
    void show_playlists();
    QString getCurrentPlayerQueue(QString songId);
    void on_favourite_toggled(bool checked);

    void setFavouriteButton(bool checked);
    void remove_song_from_smart_playlist(QVariant track_id);
    void on_smartMode_clicked();

    void init_smartMode();
//    void nextTrackHelper(QWidget *listWidgetItem);
    void on_repeat_stateChanged(int arg1);

    void nextPreviousHelper(QListWidget *list);
    void getRecommendedTracksForAutoPlayHelper(QString videoId, QString songId);
    void removeSongFromProcessQueue(QString songId);
    void updateTrack(QString trackId, QString download_Path);
    void on_connect_clicked();


private:
    QStringList currentSimilarTrackMeta ,currentSimilarTrackList;
    int currentSimilarTrackProcessing = 0;

    SimilarTracks *similarTracks = nullptr;
    Widget *downloadWidget;
    bool animationRunning = false;
    equalizer *eq = nullptr;
    int left_panel_width;
    Lyrics *lyricsWidget;
    onlineSearchSuggestion * _onlineSearchSuggestion_ = nullptr;
    paginator *pagination_manager = nullptr;
    QFile *core_file;
    QList<qint64> processIdList;
    QList<QString> OliviaMetaList; //for search purpose
    QList<QStringList> ytdlQueue ;
    QPoint oldPos,oldPosMiniWidget;
    QProcess * ytdlProcess = nullptr;
    qreal horizontalDpi;
    qreal zoom;
    QSettings settingsObj;
    QString core_local_date,core_remote_date;
    QString database;
    QString gotoAlbumId,gotoArtistId,recommendationSongId,youtubeVideoId;//jumper vars
    QString pageType;
    QString previous_eqArg;
    QString setting_path;
    QString themeColor = "4,42,59,0.2";
    QStringList color_list ;
    QStringList searchSuggestionList;
    QWidget *miniModeWidget;
    QWidget *smartModeWidget;
    QWidget *settingsWidget;
    radio *radio_manager = nullptr;
    settings *settUtils;
    store *store_manager = nullptr;
    Ui::MainWindow *ui;
    Ui::miniMode_form miniMode_ui;
    Ui::smart_mode_form smartMode_ui;
    Ui::settings settingsUi;
    Ui::track track_ui;
    Ui::toastWidget_ toast;
    Youtube *youtube;
    VideoOption *videoOption = nullptr;
    QStringList shuffledPlayerQueue;

    MprisPlugin *dp = nullptr;
    analytics *analytic = nullptr;
    QVariantMap mpris_song_meta;
    stringChangeWatcher* nowPlayingSongIdWatcher;
    bool smartMode = false;
    bool smartModeShuffleState = false;
    int oldVolume;
    bool playingSongRadio;
    TrackProperties *trackProperties = nullptr;
    Connect *connect_ = nullptr;
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
