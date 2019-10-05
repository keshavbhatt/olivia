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
private:
    QStringList playedTracksIds;

signals:
    void setSimilarTracks(QStringList);
    void failedGetSimilarTracks();

public slots:
    void addSimilarTracks(QString video_id);
};

#endif // SIMILARTRACKS_H
