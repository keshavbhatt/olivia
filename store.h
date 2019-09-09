#ifndef STORE_H
#define STORE_H

#include <QObject>
#include <QDebug>
#include <QSqlDatabase>

class store : public QObject
{
    Q_OBJECT

public:
    explicit store(QObject *parent = 0,QString dbName="default");
    Q_INVOKABLE QString web_print_saved_tracks();
    Q_INVOKABLE QString web_print_local_saved_tracks();
    Q_INVOKABLE QString web_print_local_saved_videos();
    Q_INVOKABLE QString web_print_fav_radio_channels();
    Q_INVOKABLE QString web_print_saved_albums();
    Q_INVOKABLE QString web_print_saved_artists();
    Q_INVOKABLE QString web_print_album_tracks(QVariant albumId);
    Q_INVOKABLE void removeRadioChannelFromFavourite(QVariant channelId);

private:
    QSqlDatabase db;
    int storeVersion;

public slots:

    void setTrack(QStringList meta);
    void add_to_player_queue(QString);
    QStringList getTrack(QString trackId);
    void saveAlbumArt(QString albumId, QString base64);
    void saveArtist(QString id, QString title);
    void saveAlbum(QString id, QString title);
    void saveDominantColor(QString albumId, QString value);
    void saveytIds(QString trackId, QString ids);
    void update_track(QString entity, QString trackId, QString value);
    void saveStreamUrl(QString,QString,QString);
    QList<QStringList> getPlayerQueue();
    bool isDownloaded(QString);
    bool isInQueue(QString);
    bool getExpiry(QString trackId);
    QString getAlbumId(QString songId);
    QString getArtistId(QString songId);
    void removeFromQueue(QString songId);
    QString getOfflineUrl(QString trackId);
    void removeFromCollection(QString);
    bool isInCollection(QString trackId);
    QString getDominantColor(QString albumId);//was private before

    QString getArtist(QString artistId);//was private before
    QString getAlbum(QString albumId);//was private before
    QString getThumbnail(QString artId);//was private before
    QString getYoutubeIds(QString trackId);//was private before


    void delete_track_cache(const QString download_path);
    void setRadioChannelToFavourite(QStringList meta);
    QStringList getRadioStation(QString trackId);
private slots:

    void initStore(QString dbName);
    void createTable(QString dbName);
    void openDb(QString dbName, QString type);
    void closeDb(QString dbName);
    void saveArts(QString albumId, QString artId);
    void createTableVersion(QString dbName, int version);

    QList<QStringList> getAllFavStations();
    QList<QStringList> getAllTracks();
    QList<QStringList> getAllAlbums();
    QStringList getAlbumDetails(QString albumId);
    QList<QStringList> getAllArtists();
    QStringList getArtistDetails(QString artistId);
    QList<QStringList> getAlbumTrackList(QString albumId);
    QList<QStringList> getAllVideos();
};

#endif // STORE_H
