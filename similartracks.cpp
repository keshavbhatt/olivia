#include "similartracks.h"
#include "elidedlabel.h"
#include <QDebug>
#include <QSqlQuery>

SimilarTracks::SimilarTracks(QObject *parent,int limit) : QObject(parent)
{
    numberOfSimilarTracksToLoad = limit;  //this is also hooked to setting's number spinner in MainWindow;
    store_manager = this->parent()->findChild<store*>("store_manager");

    QFile file(":/app_resources/remix_filter.dict");
    if(!file.open(QIODevice::ReadOnly)) {
        qDebug()<<"filenot opened";
    }
    else {
        remixData = QString(file.readAll()).trimmed().split(":DATA:").last().split("\"<<\"");
    }
    file.close();
}

void SimilarTracks::addRemixes(QString songId){
    //track meta from store
    QStringList trackMeta = store_manager->getTrack(songId);
    QString title = trackMeta.at(1);
    title = title.toUtf8();

    //do not remove bracket content from soundcloud tracks
    QString albumId = trackMeta.at(2);
    bool isSoundcloudTrack = albumId.contains("soundcloud");

    //replace content written inside brackets of title if its not (feat. ... or ( feat.....
    //this is to remove irrelivent content to make title clear.
    //do not remove if track is soundcloud cause there is artist infos there
    if( !isSoundcloudTrack && title.contains("(",Qt::CaseInsensitive) &&
            (!title.contains("(feat",Qt::CaseInsensitive)||!title.contains("( feat",Qt::CaseInsensitive))){
        title = title.split("(").first();
    }

    title = title+QString(+" remix mix").replace("  "," ");

    //remove remix from soundcloud track titles
    if(isSoundcloudTrack){
        title = title.remove("remix").remove("(remix)");
    }


    //add artist to title
    //    QString artist = trackMeta.at(5);
    //    artist = artist.remove("vevo",Qt::CaseInsensitive);
    //    title = title.remove(artist,Qt::CaseInsensitive);
    //    title = title +" "+ artist;

    emit lodingStarted();
    isLoadingPLaylist = true;
    isLoadingLocalSongs = false;

    QNetworkAccessManager *m_netwManager = new QNetworkAccessManager(this);
    connect(m_netwManager,&QNetworkAccessManager::finished,[=](QNetworkReply* rep){
        if(rep->error() == QNetworkReply::NoError){
            QString repStr = rep->readAll();
            QStringList list = repStr.split("gettrackinfo(");
            list.removeFirst();
            QStringList finalList;

            foreach (QString str, list) {
                QString title= str.split("!=-=!")[0];
                QString trackId = str.split("!=-=!")[4]; //track id of track being added to list
                //qDebug()<<title<<trackId;
                //prevent adding already played tracks or add tracks only which are not played in this session
                if(!playedTracksIds.contains(trackId) && trackId != songId &&
                        (remixIsValid(title))){
                    QString dataStr = QString(str.split(");\">").first()).remove("&quot;");
                    if(!dataStr.isEmpty() && (QString(dataStr.at(0))=="'"||QString(dataStr.at(0))=="\"")){
                        dataStr = dataStr.remove("'");
                        dataStr = dataStr.remove("\"");
                    }
                    finalList.append(dataStr);
                }
            }
            //qDebug()<<finalList;
            if(finalList.count()>0){
                emit setPlaylist(finalList);
//                emit setSimilarTracks(finalList);
            }else{
                qDebug()<<"List size is 0";
                parentSongId = ""; //empty the parentsongid so user can reload request
                emit failedGetSimilarTracks();
            }
        }else{
            parentSongId = ""; //empty the parentsongid so user can reload request
            emit failedGetSimilarTracks();
        }
    });
    QUrl url = "http://ktechpit.com/USS/Olivia/manual_youtube_search.php?query="+title;
    qDebug()<<"requested remix for"<<title;
    QNetworkRequest request(url);
    m_netwManager->get(request);
}

bool SimilarTracks::remixIsValid(QString title){
    bool valid = false;
    foreach (QString word, remixData) {
        QString word_ = word.trimmed();
        QStringList withSpaces;
        withSpaces<<" "+word_+" "<<word_+" "<<" "+word_;
        foreach (QString w, withSpaces) {
            if(title.contains(w,Qt::CaseInsensitive)){
                valid = true;
                break;
            }
        }
    }
    return valid;
}

void SimilarTracks::addSimilarTracks(QString video_id,QString songId){
    if(!parentSongId.isEmpty()){
        previousParentSongId = parentSongId;
    }
    parentSongId = songId;
    emit lodingStarted();
    isLoadingPLaylist = false;
    isLoadingLocalSongs = false;

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
                parentSongId = ""; //empty the parentsongid so user can reload request
                emit failedGetSimilarTracks();
            }
        }else{
            parentSongId = ""; //empty the parentsongid so user can reload request
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
    if(!data.contains("[LOCALTRACKS] ",Qt::CaseSensitive)){
        isLoadingLocalSongs = false;

    }

    isLoadingPLaylist = true;

    if(!isLoadingLocalSongs){
        emit clearList();
    }

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
        parentSongId = ""; //empty the parentsongid so user can reload request
        emit failedGetSimilarTracks();
    }
}


void SimilarTracks::getNextTracksInPlaylist(QStringList trackListFromMainWindow){

    isLoadingPLaylist = true;

    emit clearListKeepingPlayingTrack();

    if(trackListFromMainWindow.count()>0){
        emit setPlaylist(trackListFromMainWindow);
    }else{
        parentSongId = ""; //empty the parentsongid so user can reload request
        emit failedGetSimilarTracks();
    }
}


/*
* used to play all local tracks==============================================================
*/
void SimilarTracks::addLocalSongs(){
    emit clearListKeepingPlayingTrack();
    isLoadingLocalSongs = true;
    QString data = get_local_saved_tracks_data();
    if(data.isEmpty()){
        emit failedGetSimilarTracks();
        return;
    }
    addPlaylist(data);
}


//pull next page
void SimilarTracks::pullMoreLocalTracks()
{
    emit clearListKeepingPlayingTrack();
    isLoadingLocalSongs = true;
    currentPageNumber = currentPageNumber+1;
    addPlaylist(get_local_saved_tracks_PageNumber(currentPageNumber));
}

//main returns html string of local downloaded tracks
QString SimilarTracks::get_local_saved_tracks_data(){
    totalTracks = store_manager->getTrackCount("tracks","trackId");
    totalPages = totalTracks/numberOfSimilarTracksToLoad;
    int itemsLeft = totalTracks -(totalPages*numberOfSimilarTracksToLoad);
    if(itemsLeft>0){
        totalPages += 1;
    }
    currentPageNumber = 0;
    return get_local_saved_tracks_PageNumber(currentPageNumber);
}

//main result provider update query in getTrackCount also to get correct page numbers
QList<QStringList> SimilarTracks::get_local_saved_tracks_data_only(int offset){
    QSqlQuery query;
    QList<QStringList> trackList ;
    query.exec("SELECT trackId FROM tracks WHERE downloaded = 1 ORDER BY title ASC LIMIT "+QString::number(numberOfSimilarTracksToLoad)+" OFFSET "+QString::number(offset));
        while(query.next()){
             trackList.append(store_manager->getTrack(query.value("trackId").toString()));
        }
    return trackList;
}


QString SimilarTracks::get_local_saved_tracks_PageNumber(int pageNumber){
    currentPageNumber = pageNumber;
    //case for local_saved_tracks
    if(totalPages>0){
        QString html,li;
        int offset = pageNumber * numberOfSimilarTracksToLoad;
        foreach (QStringList trackList, get_local_saved_tracks_data_only(offset)) {
            QString id,title,artist,album,base64,dominantColor,songId,albumId,artistId,url;
            songId = trackList.at(0);
            title = trackList.at(1);
            albumId = trackList.at(2);
            album = trackList.at(3);
            artistId = trackList.at(4);
            artist = trackList.at(5);
            base64 = trackList.at(6);
            url = trackList.at(7);
            id = trackList.at(8);
            dominantColor = trackList.at(9);
            QString divider = "!=-=!";
            //list is like this to make it compliant with our online data providers
            li += "gettrackinfo(&quot;"+title+divider+artist+divider+album+divider+"data:image/png;base64,"+base64+divider+songId+divider+albumId+divider+artistId+divider+"0"+divider+songId+");\">";
        }
        html =   li;
        return "[LOCALTRACKS] "+html;
    }else{
        return "";
    }
}
/*
* used to play all local tracks==============================================================
*/


