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

signals:

private:
    QSqlDatabase db;


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
    bool getExpiry(QString trackId);
private slots:
    void initStore(QString dbName);
    void createTable(QString dbName);
    void openDb(QString dbName, QString type);
    void closeDb(QString dbName);
    void saveArts(QString albumId, QString artId);
    QString getArtist(QString artistId);
    QString getAlbum(QString albumId);
    QString getThumbnail(QString artId);
    QString getOfflineUrl(QString trackId);
    QString getYoutubeIds(QString trackId);
    QString getDominantColor(QString albumId);
};

#endif // STORE_H
