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
    QString parentSongId,previousParentSongId;
private:
    QStringList playedTracksIds; //collection of song ids which were already played in the current session
    store * store_manager= nullptr;

signals:
    void setSimilarTracks(QStringList);
    void setPlaylist(QStringList);
    void failedGetSimilarTracks();
    void clearList();
    void clearListKeepingPlayingTrack();

public slots:
    void addSimilarTracks(QString video_id, QString songId);
    void addPlaylist(QString data);
    void getNextTracksInPlaylist(QStringList trackListFromMainWindow);
private slots:
    bool isNumericStr(const QString str);
};

#endif // SIMILARTRACKS_H
