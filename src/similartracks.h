#ifndef SIMILARTRACKS_H
#define SIMILARTRACKS_H

#include <QObject>
#include <QtNetwork>
#include "store.h"

class SimilarTracks : public QObject
{
    Q_OBJECT
public:
    explicit SimilarTracks(QObject *parent = nullptr, int limit = 1);
    int numberOfSimilarTracksToLoad = 0;
    bool isLoadingPLaylist = false;
    bool isLoadingLocalSongs = false;
    QString parentSongId,previousParentSongId;
private:
    QStringList playedTracksIds; //collection of song ids which were already played in the current session
    store * store_manager= nullptr;
    QStringList remixData;

    int totalTracks,totalPages,currentPageNumber;

signals:
    void setSimilarTracks(QStringList);
    void setPlaylist(QStringList);
    void failedGetSimilarTracks();
    void clearList();
    void clearListKeepingPlayingTrack();
    void lodingStarted();

public slots:
    void addSimilarTracks(QString video_id, QString songId);
    void addPlaylist(QString data);
    void getNextTracksInPlaylist(QStringList trackListFromMainWindow);
    void addRemixes(QString songId);
    void addLocalSongs();
    void pullMoreLocalTracks();

private slots:
    bool isNumericStr(const QString str);
    bool remixIsValid(QString title);
    QString get_local_saved_tracks_PageNumber(int pageNumber);
    QList<QStringList> get_local_saved_tracks_data_only(int offset);
    QString get_local_saved_tracks_data();
};

#endif // SIMILARTRACKS_H
