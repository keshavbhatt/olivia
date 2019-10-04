#ifndef SIMILARTRACKS_H
#define SIMILARTRACKS_H

#include <QObject>
#include <QtNetwork>

class SimilarTracks : public QObject
{
    Q_OBJECT
public:
    explicit SimilarTracks(QObject *parent = nullptr);



signals:
    void addToSimilarTracksList(QStringList);

public slots:
    void addSimilarTracks(QString video_id);
};

#endif // SIMILARTRACKS_H
