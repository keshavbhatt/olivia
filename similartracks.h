#ifndef SIMILARTRACKS_H
#define SIMILARTRACKS_H

#include <QObject>
#include <QtNetwork>

class SimilarTracks : public QObject
{
    Q_OBJECT
public:
    explicit SimilarTracks(QObject *parent = nullptr, int limit = 5);
    int numberOfSimilarTracksToLoad;
    bool isLoadingPLaylist = false;
private:
    QStringList playedTracksIds;

signals:
    void setSimilarTracks(QStringList);
    void setPlaylist(QStringList);
    void failedGetSimilarTracks();
    void clearList();

public slots:
    void addSimilarTracks(QString video_id);
    void addPlaylist(QString data);
};

#endif // SIMILARTRACKS_H
