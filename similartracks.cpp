#include "similartracks.h"
#include "elidedlabel.h"
#include <QDebug>


SimilarTracks::SimilarTracks(QObject *parent,int limit) : QObject(parent)
{
    numberOfSimilarTracksToLoad = limit;
    qDebug()<<"Related songs limit"<<limit;
    store_manager = this->parent()->findChild<store*>("store_manager");
}

void SimilarTracks::addSimilarTracks(QString video_id,QString songId){
    if(!parentSongId.isEmpty()){
        previousParentSongId = parentSongId;
    }
    parentSongId = songId;

    isLoadingPLaylist = false;
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
                    QString dataStr = QString(str.split(");\">").first()).remove("&quot;");
                    if(!dataStr.isEmpty() && (QString(dataStr.at(0))=="'"||QString(dataStr.at(0))=="\"")){
                        dataStr = dataStr.remove("'");
                        dataStr = dataStr.remove("\"");
                    }
                    finalList.append(dataStr);
                }
            }
          //  qDebug()<<finalList;
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
    QUrl url;
    if(store_manager->getAlbumId(songId).contains("undefined-") || isNumericStr(songId)){
         url = "http://ktechpit.com/USS/Olivia/youtube_related_videos.php?video_id="+video_id;
    }else{
         url = "http://ktechpit.com/USS/Olivia/spotify/spotify_related_seed_song.php?s_id="+songId+"&url=null";
    }
    QNetworkRequest request(url);
    m_netwManager->get(request);
}

bool SimilarTracks::isNumericStr(const QString str){
    bool isNumeric;
     str.toInt(&isNumeric, 16);
     str.toInt(&isNumeric, 10);
     return  isNumeric;
}

void SimilarTracks::addPlaylist(QString data){

    isLoadingPLaylist = true;

    emit clearList();

    QStringList list = data.split("gettrackinfo(");
    list.removeFirst();

    QStringList finalList;

    foreach (QString str, list) {
        QString dataStr = QString(str.split(");\">").first()).remove("&quot;");
        if(!dataStr.isEmpty() && (QString(dataStr.at(0))=="'"||QString(dataStr.at(0))=="\"")){
            dataStr = dataStr.remove("'");
            dataStr = dataStr.remove("\"");
        }
        finalList.append(dataStr);
    }

    if(finalList.count()>0){
        emit setPlaylist(finalList);
    }else{
        emit failedGetSimilarTracks();
    }
}

void SimilarTracks::getNextTracksInPlaylist(QStringList trackListFromMainWindow){

    isLoadingPLaylist = true;

    emit clearListKeepingPlayingTrack();

    if(trackListFromMainWindow.count()>0){
        emit setPlaylist(trackListFromMainWindow);
    }else{
        emit failedGetSimilarTracks();
    }
}

