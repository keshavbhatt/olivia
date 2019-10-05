#include "similartracks.h"
#include "elidedlabel.h"
#include <QDebug>

SimilarTracks::SimilarTracks(QObject *parent) : QObject(parent)
{

}

void SimilarTracks::addSimilarTracks(QString video_id){

    QNetworkAccessManager *m_netwManager = new QNetworkAccessManager(this);
    connect(m_netwManager,&QNetworkAccessManager::finished,[=](QNetworkReply* rep){
        if(rep->error() == QNetworkReply::NoError){
            QString repStr = rep->readAll();
            QStringList list = repStr.split("gettrackinfo(");
            list.removeFirst();
            QStringList finalList;
            foreach (QString str, list) {
               finalList.append(QString(str.split(");\">").first()).remove("&quot;"));
            }
            if(finalList.count()>0){
                for (int i = 0; i < 5;/*finalList.count();*/ i++) {
                    QString videoId,title,artist,album,coverUrl,songId,albumId,artistId,millis;
                    QStringList arr = QString(finalList.at(i)).split("!=-=!");
                    title = arr[0];
                    artist = arr[1];
                    album = arr[2];
                    coverUrl = arr[3];
                    songId = arr[4];
                    albumId = arr[5];
                    artistId= arr[6];
                    millis = arr[7];

                    if(albumId.contains("undefined")){ //yt case
                        videoId = arr[4];
                    }
                    emit addToSimilarTracksList(QStringList()<<videoId<<title<<artist<<album<<coverUrl<<songId<<albumId<<artistId);
                }
            }
        }
        rep->deleteLater();
        m_netwManager->deleteLater();
    });
    QUrl url("http://ktechpit.com/USS/Olivia/youtube_related_videos.php?video_id="+video_id);
    QNetworkRequest request(url);
    m_netwManager->get(request);
}
