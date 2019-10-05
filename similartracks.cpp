#include "similartracks.h"
#include "elidedlabel.h"
#include <QDebug>

SimilarTracks::SimilarTracks(QObject *parent,int limit) : QObject(parent)
{
    numberOfSimilarTracksToLoad = limit;
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
                //prevent adding already played tracks or add tracks only which are not played in this session
                if(!playedTracksIds.contains(str.split("!=-=!")[4])){
                    finalList.append(QString(str.split(");\">").first()).remove("&quot;"));
                }
            }
            if(finalList.count()>0){
                emit setSimilarTracks(finalList);
                int tracksLeft;
                if(finalList.count()<numberOfSimilarTracksToLoad){ //set finalLeft tracks for the for loop below
                    tracksLeft = finalList.count();
                }else{
                    tracksLeft = numberOfSimilarTracksToLoad;
                }
                for (int i = 0; i < tracksLeft; i++) {
                   playedTracksIds.append(QString(finalList.at(i)).split("!=-=!")[4]);
                }
            }else{
                emit failedGetSimilarTracks();
            }
        }else{
            emit failedGetSimilarTracks();
        }
        rep->deleteLater();
        m_netwManager->deleteLater();
    });
    QUrl url("http://ktechpit.com/USS/Olivia/youtube_related_videos.php?video_id="+video_id);
    QNetworkRequest request(url);
    m_netwManager->get(request);
}
