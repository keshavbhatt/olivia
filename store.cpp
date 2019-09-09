#include "store.h"
#include "utils.h"

#include <QSqlQuery>
#include <QSqlResult>
#include <QSqlRecord>
#include <QSqlDatabase>
#include <QSqlError>
#include <QFile>
#include <QStandardPaths>
#include <QFileInfo>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QColor>
#include <QPixmap>
#include <QImage>
#include <QBuffer>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>




// init store class and creates defined directories
store::store(QObject *parent, QString dbName) : QObject(parent)
{
    storeVersion = 2;  // 2 is the initial version of database
    QString setting_path =  QStandardPaths::writableLocation(QStandardPaths::DataLocation)+"/storeDatabase/";
    QDir d(setting_path+dbName);
    if(!d.exists()){
        if(d.mkdir(d.path())){
            if(QFileInfo(QFile(setting_path+dbName)).exists()){
                initStore(dbName);
            }
        }
    }else{
        initStore(dbName);
    }
}

//tries to open DB for transactions
void store::initStore(QString dbName){ //takes dbname only dont include paths
        QString setting_path =  QStandardPaths::writableLocation(QStandardPaths::DataLocation)+"/storeDatabase/"+dbName;
        QString db_path ;
        if(QFileInfo(QFile(setting_path+"/"+dbName+".db")).exists()){
            db_path = setting_path+"/"+dbName+".db";
        }else{
            openDb(dbName,"new");
        }
        //open db if it was created and is valid
        if(!db_path.isEmpty()){
             if(db.isOpen()) {
                 qWarning()<<"Store open for queries"<<setting_path+"/storeDatabase/"+dbName+".db";
             }else if(!db.isOpen()){
                 qWarning()<<"Store not Open , opening...";
                 openDb(dbName,"old");
             }
        }
}

//creates storeDatabase file with dbname given in defined pAths and opens it for transactions
void store::openDb(QString dbName,QString type){
    QString path = QStandardPaths::writableLocation(QStandardPaths::DataLocation)+"/storeDatabase/"+dbName+"/";
    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(path+dbName+".db");

    //get local storeversion
    int localStoreVersion = 1;
    QFile *store_version_file =  new QFile(path+"store_version" );
    if (!store_version_file->open(QIODevice::ReadOnly | QIODevice::Text)){
        qWarning()<<"Could not open a store_version_file to read.";
    }else{
        localStoreVersion  = store_version_file->readAll().trimmed().toInt();
    }
    store_version_file->close();

    if(db.open()){
        if(type=="new"){
            createTable(dbName);
        }else if(localStoreVersion < storeVersion){
                for (int i=localStoreVersion;i<storeVersion;i++) {
                    createTableVersion(dbName,storeVersion);
                }
                initStore(dbName);
        }else{
            qWarning()<<"Store open for queries"<<path+dbName+".db";
        }
    }
    if(!store_version_file->open(QIODevice::ReadWrite | QIODevice::Truncate)){
        qWarning()<<"Could not open a store_version_file to write.";
    }
    store_version_file->write(QString::number(storeVersion).toUtf8());
    store_version_file->close();
    store_version_file->deleteLater();
}

// creates table in given DB name OR makes default structure of DB
void store::createTable(QString dbName){
    QString path = QStandardPaths::writableLocation(QStandardPaths::DataLocation)+"/storeDatabase/"+dbName+"/";
    //QString dbname = dbName.split("/").last().split(".db").first(); //default

    //tables
    QSqlQuery track;
    bool track_created = track.exec("create table tracks "
              "(trackId varchar(500) PRIMARY KEY, "
              "albumId varchar(300), "
              "artistId varchar(300),"
              "title varchar(500),"
              "downloaded int(1))");
    QSqlQuery queue;
    bool queue_created = queue.exec("create table queue "
              "(id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,"
                "trackId varchar(500) )"
               );

    QSqlQuery artist;
    bool artist_created = artist.exec("create table artist "
              "(artistId varchar(500) PRIMARY KEY,"
              "artistName varchar(500))"
               );

    QSqlQuery album;
    bool album_created = album.exec("create table album "
              "(albumId varchar(300) PRIMARY KEY,"
              "albumName varchar(500))"
               );

    QSqlQuery color;
    bool color_created = color.exec("create table color "
              "(albumId varchar(300) PRIMARY KEY,"
              "colorName varchar(300))"
               );

    QSqlQuery ytIds;
    bool ytIds_created = ytIds.exec("create table ytIds "
              "(trackId varchar(500) PRIMARY KEY,"
              "ids varchar(500))"
               );

    QSqlQuery art;
    bool art_created = art.exec("create table arts "
              "(albumId varchar(300) PRIMARY KEY,"
              "artId varchar(300))"
               );

    QSqlQuery streamUrl;
    bool streamUrl_created = streamUrl.exec("create table stream_url "
              "(trackId varchar(300) PRIMARY KEY,"
              "url varchar(500),"
              "timeOfExpiry varchar(70))"
               );

    if(track_created && queue_created && album_created && artist_created && color_created && ytIds_created && art_created && streamUrl_created){
        qDebug()<<"Database tables ready for"<<path+dbName+".db";
        initStore(dbName);
    }
}

//create or update store db according to requested version
void store::createTableVersion(QString dbName,int version){
    QString path = QStandardPaths::writableLocation(QStandardPaths::DataLocation)+"/storeDatabase/"+dbName+"/";
    switch (version) {
        // case 1 is default database, case 0 is nothing(updates started with case 2)
        case 2:{
            QSqlQuery radio;
            qWarning()<<"Update store with new update- "<<version<<" done.";
            radio.exec("create table radio_favourite "
                      "(channelId varchar(300) PRIMARY KEY,"
                      "url varchar(900),"
                      "title varchar(500),"
                      "lang varchar(500),"
                      "country varchar(800))"
                      );
        }
            break;
        //create cases here to update database table according to store version
    }
}


//closes the DB
void store::closeDb(QString dbName){
    QString path = QStandardPaths::writableLocation(QStandardPaths::DataLocation)+"/storeDatabase/"+dbName;
    QSqlDatabase::removeDatabase(path+"/storeDatabase/"+dbName+".db");
    qDebug()<<"Acitive db connections:"<<QSqlDatabase::connectionNames();
}

//MODIFICATION METHODS
//function to set NULL in downloaded cloumn , used by setting dialog for delete_cached_tracks;
void store::delete_track_cache(const QString download_path){
    QSqlQuery query;
    query.exec("SELECT trackId FROM tracks;");
    if(query.record().count()>1){
        while(query.next()){
            QString trackName  = query.value("trackId").toString();
            QFile file(download_path+"/"+trackName);
            if(file.remove()){
                 file.deleteLater();
                 QSqlQuery query2;
                 query2.exec("UPDATE tracks SET 'downloaded' = NULL WHERE trackId = '"+trackName+"';");
            }
        }
    }else{
        utils* util = new utils(this);
        if(util->delete_cache(download_path)){
            util->deleteLater();
        }
    }
}

//saves tracks in db with all columns in tracks table in DB
void store::setTrack(QStringList meta){
    QString title = meta.at(3);
    QString albumId = meta.at(1);
    QSqlQuery query;
    query.exec("INSERT INTO tracks('trackId','albumId','artistId','title') "
               "VALUES('"+meta.at(0)+"','"+albumId.trimmed().remove("\"").replace("'","''").remove(QChar('\\')).remove("/")+"','"+meta.at(2)+"','"+title.remove("\"").replace("'","''")+"')");
}

//saves radio channel to favourite in db with all columns in radio_favourite table in DB
void store::setRadioChannelToFavourite(QStringList meta){
    //("87600", "http://stream-uk1.radioparadise.com/aac-320", "Radio Paradise (320k)", "United States of America", "English\")")
    QString title = QString(meta.at(2)).remove("\"").replace("'","''").trimmed();
    QString country = QString(meta.at(3)).remove("\"").replace("'","''").trimmed();
    QString lang = QString(meta.at(4)).remove("\"").replace("'","''").trimmed();
    QSqlQuery query;
    query.exec("INSERT INTO radio_favourite('channelId','url','title','lang','country') "
               "VALUES('"+meta.at(0)+"','"+meta.at(1)+"','"+title+"','"+lang.remove(")").remove("(")+"','"+country+"')");
}

//remove
void store::removeRadioChannelFromFavourite(QVariant channelId){
    QSqlQuery query;
    query.exec("DELETE FROM radio_favourite WHERE channelId= '"+channelId.toString().trimmed()+"';");
}

//updates download info of a track of any other info in tracks table
void store::update_track(QString entity,QString trackId,QString value){
    QSqlQuery query;
    query.exec("UPDATE tracks SET "+entity.remove("\"")+" = '"+ value +"' WHERE trackId = '"+trackId+"';");
}

//saves currently added tracks from player queue to db just with their trackId
void store::add_to_player_queue(QString trackId){
    QSqlQuery query;
    query.exec("INSERT INTO queue('trackId') VALUES('"+trackId.trimmed()+"')");
}

//saves album title with album id
void store::saveAlbum(QString id,QString title){
    QSqlQuery query;
    query.exec("INSERT INTO album('albumId','albumName') VALUES('"+id.trimmed().remove("\"").replace("'","''").remove(QChar('\\')).remove("/")+"','"+title.trimmed().remove("\"").replace("'","''").remove(QChar('\\')).remove("/")+"')");

}

//saves artist title with artist id
void store::saveArtist(QString id,QString title){
    QSqlQuery query;
    query.exec("INSERT INTO artist('artistId','artistName') VALUES('"+id.trimmed()+"','"+title.trimmed().remove("\"").replace("'","''")+"')");
}

//saves dominant color of earch album from its art
void store::saveDominantColor(QString albumId,QString value){
    QSqlQuery query;
    query.exec("INSERT INTO color('albumId','colorName') VALUES('"+albumId.trimmed().trimmed().remove("\"").replace("'","''").remove(QChar('\\')).remove("/")+"','"+value.trimmed()+"')");
}

//function to save the id of tracks from YTServer
void store::saveytIds(QString trackId,QString ids){
    QSqlQuery query;
    query.exec("INSERT INTO ytIds('trackId','ids') VALUES('"+trackId.trimmed()+"','"+ids.trimmed()+"')");
}

//seperate function to save info about album arts
void store::saveArts(QString albumId,QString artId){
    QSqlQuery query;
    query.exec("INSERT INTO arts('artId','albumId') VALUES('"+albumId.trimmed()+"','"+artId.trimmed()+"')");
}

//FILESYSTEM
void store::saveAlbumArt(QString albumId,QString base64){ //save album art if not exists
    //add entry to db
    QString artId = "art-"+albumId.remove("\"").replace("'","''").remove(QChar('\\')).remove("/");
    saveArts(artId,albumId.remove("\"").replace("'","''").remove(QChar('\\')).remove("/"));
    //save to disk
    QString albumArtPath =  QStandardPaths::writableLocation(QStandardPaths::DataLocation)+"/albumArts/";
    QByteArray bytes =base64.toUtf8();
    QBuffer buffer(&bytes);
    QFile file(albumArtPath+artId);
    if (!file.open(QIODevice::WriteOnly))
        return;
           file.write(buffer.data());
         file.close();
}

//saves the url of track with expiryTime which is check by mAInWindow while loading track
void store::saveStreamUrl(QString songId, QString url_str, QString expiryTime){
    QSqlQuery query;
    bool saved = query.exec("INSERT INTO stream_url('trackId','url','timeOfExpiry') VALUES('"+songId.trimmed()+"','"+url_str.trimmed()+"','"+expiryTime.trimmed()+"')");
    //update track info if it already exist or any kind of error while inserting new entry
    if(!saved){
        QSqlQuery update;
        update.prepare("UPDATE stream_url SET url='"+url_str.trimmed()+"',timeOfExpiry='"+expiryTime.trimmed()+"' WHERE trackId='"+songId.trimmed()+"'");
        update.exec();
    }
}


//GETTERS METHODS
QList<QStringList> store::getPlayerQueue(){
    QSqlQuery query;
    QList<QStringList> trackList ;
    query.exec("SELECT trackId FROM queue ORDER BY id ASC");
    if(query.record().count()>0){
        while(query.next()){
             trackList.append(getTrack(query.value("trackId").toString()));
        }
    }
    return trackList;
}

QList<QStringList> store::getAllFavStations(){
    QSqlQuery query;
    QList<QStringList> stationsList ;
    query.exec("SELECT channelId FROM radio_favourite");
    if(query.record().count()>0){
        while(query.next()){
             stationsList.append(getRadioStation(query.value("channelId").toString()));
        }
    }
    return stationsList;
}

QList<QStringList> store::getAllTracks(){
    QSqlQuery query;
    QList<QStringList> trackList ;
    query.exec("SELECT trackId FROM tracks ORDER BY title ASC");
    if(query.record().count()>0){
        while(query.next()){
             trackList.append(getTrack(query.value("trackId").toString()));
        }
    }
    return trackList;
}

QList<QStringList> store::getAllVideos(){
    QList<QStringList> trackList ;
    QString setting_path =  QStandardPaths::writableLocation(QStandardPaths::DataLocation);
    QDir dir(setting_path+"/downloadedVideos/");
    QStringList filter;
    filter<< +"*.webm"<<"*.mp4"<<"*.mpeg"<<"*.mkv"<<"*.avi"<<"*.flv"<<"*.ogv"<<"*.ogg";
    QFileInfoList files = dir.entryInfoList(filter);
   // qDebug()<<dir.entryList(filter);

    foreach (QFileInfo fileInfo, files) {
//        qDebug()<<fileInfo.baseName();
        trackList.append(getTrack(fileInfo.baseName()));
    }
   // qDebug()<<trackList;
    return trackList;
}

QList<QStringList> store::getAllAlbums(){
    QSqlQuery query;
    QList<QStringList> albumList ;
    query.exec("SELECT * FROM album ORDER BY albumName ASC");
    if(query.record().count()>0){
        while(query.next()){
             albumList.append(getAlbumDetails(query.value("albumId").toString()));
        }
    }
    return albumList;
}

QStringList store::getAlbumDetails(QString albumId){
    //get album artist from tracks table
    QSqlQuery query;
    query.exec("SELECT artistId FROM tracks WHERE albumId = '"+albumId+"'");
    QString artistId,artistName,tracksCount;
    int count = 0;
    if(query.record().count()>0){
        while(query.next()){
            artistId =  query.value("artistId").toString();
            count++;
        }
    }
    tracksCount = QString::number(count);
    artistName = getArtist(artistId);
    return QStringList()<<albumId<<getAlbum(albumId)<<getThumbnail(albumId)<<getDominantColor(albumId)<<artistName<<artistId<<tracksCount;
}

QList<QStringList> store::getAllArtists(){
    QSqlQuery query;
    QList<QStringList> artistList ;
    query.exec("SELECT artistId FROM artist ORDER BY artistName ASC");
    if(query.record().count()>0){
        while(query.next()){
             artistList.append(getArtistDetails(query.value("artistId").toString()));
        }
    }
    return artistList;
}

QStringList store::getArtistDetails(QString artistId){
    //get album artist from tracks table
    QSqlQuery query;
    query.exec("SELECT artistId FROM tracks WHERE artistId = '"+artistId+"'");
    QString artistName,tracksCount;
    int count = 0;
    if(query.record().count()>0){
        while(query.next()){
            count++;
        }
    }
    tracksCount = QString::number(count);
    artistName = getArtist(artistId);
    return QStringList()<<artistId<<artistName<<tracksCount;
}

QStringList store::getTrack(QString trackId){
    QSqlQuery query;
    query.exec("SELECT * FROM tracks WHERE trackId = '"+trackId+"'");
    QString albumId,artistId,title;
    if(query.record().count()>0){
        while(query.next()){
            albumId = query.value("albumId").toString();
            artistId = query.value("artistId").toString();
            title= query.value("title").toString();
        }
    }
    return QStringList()<<trackId<<title<<albumId<<getAlbum(albumId)<<artistId<<getArtist(artistId)<<getThumbnail(albumId)<<getOfflineUrl(trackId)<<getYoutubeIds(trackId)<<getDominantColor(albumId);
}
QStringList store::getRadioStation(QString trackId){
    QSqlQuery query;
    query.exec("SELECT * FROM radio_favourite WHERE channelId = '"+trackId+"'");
    QString url,title,lang,country;
    if(query.record().count()>0){
        while(query.next()){
            url = query.value("url").toString();
            title= query.value("title").toString();
            lang= query.value("lang").toString();
            country= query.value("country").toString();
        }
    }
    return QStringList()<<trackId<<url<<title<<lang<<country<<"qrc:/web/radio/station.jpg";
}



QString store::getAlbum(QString albumId){
    QSqlQuery query;
    QString albumId_str;
    query.exec("SELECT albumName FROM album WHERE albumId = '"+albumId+"'");
    if(query.record().count()>0){
        while(query.next()){
             albumId_str = query.value("albumName").toString();
        }
    }
    return albumId_str;
}

QString store::getAlbumId(QString songId){
    QSqlQuery query;
    QString albumId;
    query.exec("SELECT albumId FROM tracks WHERE trackId = '"+songId+"'");
    if(query.record().count()>0){
        while(query.next()){
             albumId = query.value("albumId").toString();
        }
    }
    return albumId;
}

QString store::getArtistId(QString songId){
    QSqlQuery query;
    QString artistId;
    query.exec("SELECT artistId FROM tracks WHERE trackId = '"+songId+"'");
    if(query.record().count()>0){
        while(query.next()){
             artistId = query.value("artistId").toString();
        }
    }
    return artistId;
}

void store::removeFromQueue(QString songId){
    QSqlQuery query;
    query.exec("DELETE FROM queue WHERE trackId = '"+songId+"'");
}

void store::removeFromCollection(QString songId){
    QSqlQuery query;
    query.exec("DELETE FROM tracks WHERE trackId = '"+songId+"'");
}



QString store::getArtist(QString artistId){
    QSqlQuery query;
    QString artistId_str;
    query.exec("SELECT artistName FROM artist WHERE artistId = '"+artistId+"'");
    if(query.record().count()>0){
        while(query.next()){
             artistId_str = query.value("artistName").toString();
        }
    }
    return artistId_str;
}

QString store::getThumbnail(QString artId){
    QSqlQuery query;
    query.exec("SELECT artId FROM arts WHERE albumId = '"+artId+"'");
    QString artId_Str;
    if(query.record().count()>0){
        while(query.next()){
             artId_Str =  query.value("artId").toString();
        }
    }
    QString setting_path =  QStandardPaths::writableLocation(QStandardPaths::DataLocation);
    QDir albumArt(setting_path+"/albumArts");
    QFile thumbnailFile(albumArt.path()+"/"+artId_Str);
    if(!thumbnailFile.open(QIODevice::ReadOnly))
        return "";
    QTextStream in(&thumbnailFile);
    return in.readAll();
}

QString store::getOfflineUrl(QString trackId){
    QSqlQuery query;
    query.exec("SELECT url FROM stream_url WHERE trackId = '"+trackId+"'");
    QString url_str;
    if(query.record().count()>0){
        while(query.next()){
             url_str =  query.value("url").toString();
        }
    }
    return url_str;
}

QString store::getYoutubeIds(QString trackId){
    QSqlQuery query;
    query.exec("SELECT ids FROM ytids WHERE trackId = '"+trackId+"'");
    QString ids_str;
    if(query.record().count()>0){
        while(query.next()){
             ids_str =  query.value("ids").toString();
        }
    }
    return ids_str;
}

bool store::isInQueue(QString trackId){
    QSqlQuery query;
    query.exec("SELECT * FROM queue WHERE trackId = '"+trackId+"'");
    bool present = false;
    if(query.record().count()>0){
        while(query.next()){
            if(query.value("trackId").toString().trimmed()==trackId){
                present = true;
            }else{
                present =  false;
            }
        }
    }
    return present;
}

bool store::isDownloaded(QString trackId){
    QSqlQuery query;
    query.exec("SELECT downloaded FROM tracks WHERE trackId = '"+trackId+"'");
    bool downloaded = false;
    if(query.record().count()>0){
        while(query.next()){
            if(query.value("downloaded").toString().trimmed()=="1"){
                downloaded = true;
            }else{
                downloaded =  false;
            }
        }
    }
    return downloaded;
}

bool store::isInCollection(QString trackId){
    QSqlQuery query;
    query.exec("SELECT trackId FROM tracks WHERE trackId = '"+trackId+"'");
    bool inCollection = false;
    if(query.record().count()>0){
        while(query.next()){
            if(query.value("trackId").toString().trimmed()==trackId){
                inCollection = true;
            }else{
                inCollection =  false;
            }
        }
    }
    return inCollection;
}


QString store::getDominantColor(QString albumId){
    QSqlQuery query;
    query.exec("SELECT colorName FROM color WHERE albumId = '"+albumId+"'");
    QString ids_str;
    if(query.record().count()>0){
        while(query.next()){
             ids_str =  query.value("colorName").toString();
        }
    }
    return ids_str;
}


bool store::getExpiry(QString trackId){
    bool expired = false;
    if(!trackId.trimmed().isEmpty()){
        QSqlQuery query;
        query.exec("SELECT timeOfExpiry FROM stream_url WHERE trackId = '"+trackId+"'");

        QString timeOfExpiry;
        if(query.record().count()>0){
            while(query.next()){
                 timeOfExpiry =  query.value("timeOfExpiry").toString();
            }
        }
        //qDebug()<<QDateTime::currentMSecsSinceEpoch()/1000<<timeOfExpiry;
        if(timeOfExpiry.toInt() < QDateTime::currentMSecsSinceEpoch()/1000)
        {
            expired =  true;
        }else{
            expired = false;
        }
    }else{
        expired = true;
    }
    return expired;
}


// WEB=========================================================================

//returns json string of added tracks to library
QString store::web_print_saved_tracks(){
    qDebug()<<"LOAD SAVED TRACKS";
    QJsonDocument json;
    QJsonArray recordsArray;
    foreach (QStringList trackList, getAllTracks()) {
        QJsonObject recordObject;
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
        if(!songId.trimmed().isEmpty()){
            dominantColor = trackList.at(9);
            recordObject.insert("songId",songId);
            recordObject.insert("title",title);
            recordObject.insert("albumId",albumId);
            recordObject.insert("album",album);
            recordObject.insert("artistId",artistId);
            recordObject.insert("artist",artist);
            recordObject.insert("base64",base64);
            recordObject.insert("url",url);
            recordObject.insert("id",id);
            recordsArray.push_back(recordObject);
        }
    }
    json.setArray(recordsArray);
    return json.toJson();
}

// returns json array string of local downloaded tracks
QString store::web_print_fav_radio_channels(){
    qDebug()<<"LOAD FAVOURITE RADIO CHANNELS";
    QJsonDocument json;
    QJsonArray recordsArray;
    foreach (QStringList trackList, getAllFavStations()) {
        QJsonObject recordObject;
        QString id,url,title,lang,country,base64;
        id = trackList.at(0);
        url = trackList.at(1);
        title = QString(trackList.at(2)).remove("'");
        lang = trackList.at(3);
        country = trackList.at(4);
        base64 = trackList.at(5);

        recordObject.insert("channelId",id);
        recordObject.insert("title",title);
        recordObject.insert("lang",lang.remove("'"));
        recordObject.insert("country",country.remove("'"));
        recordObject.insert("base64",base64);
        recordObject.insert("url",url);
        recordsArray.push_back(recordObject);
    }
    json.setArray(recordsArray);
    return json.toJson();
}

// returns json array string of local downloaded tracks
QString store::web_print_local_saved_tracks(){
    qDebug()<<"LOAD LOCAL SAVED TRACKS";
    QJsonDocument json;
    QJsonArray recordsArray;
    foreach (QStringList trackList, getAllTracks()) {
        QJsonObject recordObject;
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
        if(!songId.trimmed().isEmpty() && isDownloaded(songId)){
            recordObject.insert("songId",songId);
            recordObject.insert("title",title);
            recordObject.insert("albumId",albumId);
            recordObject.insert("album",album);
            recordObject.insert("artistId",artistId);
            recordObject.insert("artist",artist);
            recordObject.insert("base64",base64);
            recordObject.insert("url",url);
            recordObject.insert("id",id);
            recordsArray.push_back(recordObject);
        }
    }
    json.setArray(recordsArray);
    return json.toJson();
}

// returns json array string of local downloaded videos
QString store::web_print_local_saved_videos(){
    qDebug()<<"LOAD LOCAL SAVED VIDEOS";
    QJsonDocument json;
    QJsonArray recordsArray;
    foreach (QStringList trackList, getAllVideos()) {
        QJsonObject recordObject;
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
        if(!songId.trimmed().isEmpty()){
            recordObject.insert("songId",songId);
            recordObject.insert("title",title);
            recordObject.insert("albumId",albumId);
            recordObject.insert("album",album);
            recordObject.insert("artistId",artistId);
            recordObject.insert("artist",artist);
            recordObject.insert("base64",base64);
            recordObject.insert("url",url);
            recordObject.insert("id",id);
            recordsArray.push_back(recordObject);
        }
    }
    json.setArray(recordsArray);
    return json.toJson();
}

QString store::web_print_saved_albums(){
    qDebug()<<"LOAD LOCAL SAVED ALBUMS";
    QJsonDocument json;
    QJsonArray recordsArray;
    QJsonObject recordObject;
    int untitledAlbumsRecordObjectCount =0;


    foreach (QStringList albumList, getAllAlbums()) {
        QString albumId,albumName,artistName,base64,dominantColor,artistId,tracksCount;
        albumId = albumList.at(0);
        albumName = albumList.at(1);
        base64 = albumList.at(2);
        dominantColor = albumList.at(3);
        artistName = albumList.at(4);
        artistId = albumList.at(5);
        tracksCount = albumList.at(6);

        if(albumId.contains("undefined") && !albumId.trimmed().isEmpty()){
            albumName="untitled";
            untitledAlbumsRecordObjectCount = untitledAlbumsRecordObjectCount+1 ;
            //qDebug()<<albumName;
        }

        if(!albumId.contains("undefined") && !albumId.trimmed().isEmpty()){
            recordObject.insert("albumId",albumId);
            recordObject.insert("albumName",albumName);
            recordObject.insert("base64",base64);
            recordObject.insert("dominantColor",dominantColor);
            recordObject.insert("artistName",artistName);
            recordObject.insert("artistId",artistId);
            recordObject.insert("tracksCount",tracksCount);
            recordsArray.push_back(recordObject);
        }
    }

    if(untitledAlbumsRecordObjectCount>0){
        recordObject.insert("albumId","untitled");
        recordObject.insert("albumName","Youtube");
        recordObject.insert("base64","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");
        recordObject.insert("dominantColor","75,245,85");
        recordObject.insert("artistName","Various");
        recordObject.insert("artistId","undefined");
        recordObject.insert("tracksCount",QString::number(untitledAlbumsRecordObjectCount));
        recordsArray.push_back(recordObject);
    }

    json.setArray(recordsArray);
    return json.toJson();
}

// returns json array string of local artists
QString store::web_print_saved_artists(){
    qDebug()<<"LOAD LOCAL SAVED ARTISTS";
    QJsonDocument json;
    QJsonArray recordsArray;
    foreach (QStringList artistList, getAllArtists()) {
        QJsonObject recordObject;
        QString artistId,artistName,tracksCount;

        artistId = artistList.at(0);
        artistName = artistList.at(1);
        tracksCount = artistList.at(2);
        if(!artistId.trimmed().isEmpty()){
            recordObject.insert("artistId",artistId);
            recordObject.insert("artistName",artistName);
            recordObject.insert("tracksCount",tracksCount);
            recordsArray.push_back(recordObject);
        }
    }
    json.setArray(recordsArray);
    return json.toJson();
}

//helper function for web_print_album_tracks
QList<QStringList> store::getAlbumTrackList(QString albumId){
    QList<QStringList> trackList ;
     if(albumId=="untitled"){
         albumId = "undefined";
         QSqlQuery query;
         query.exec("SELECT trackId FROM tracks WHERE albumId LIKE '%"+albumId+"%' ");
          if(query.record().count()>0){
             while(query.next()){
               trackList.append(getTrack(query.value("trackId").toString()));
             }
         }
     }else{
         QSqlQuery query;
         query.exec("SELECT trackId FROM tracks WHERE albumId = '"+albumId+"'");
          if(query.record().count()>0){
             while(query.next()){
               trackList.append(getTrack(query.value("trackId").toString()));
             }
         }
     }
    return trackList;
}

QString store::web_print_album_tracks(QVariant albumId){
    qDebug()<<"LOAD ALBUM TRACKS";
    QJsonDocument json;
    QJsonArray recordsArray;
    foreach (QStringList trackDetails, getAlbumTrackList(albumId.toString())) {
        QJsonObject recordObject;
        QString id,title,artist,album,base64,dominantColor,songId,albumId,artistId,url;
        songId = trackDetails.at(0);
        title = trackDetails.at(1);
        albumId = trackDetails.at(2);
        album = trackDetails.at(3);
        artistId = trackDetails.at(4);
        artist = trackDetails.at(5);
        base64 = trackDetails.at(6);
        url = trackDetails.at(7);
        id = trackDetails.at(8);
        dominantColor = trackDetails.at(9);
        recordObject.insert("songId",songId);
        recordObject.insert("title",title);
        recordObject.insert("albumId",albumId);
        recordObject.insert("album",album);
        recordObject.insert("artistId",artistId);
        recordObject.insert("artist",artist);
        recordObject.insert("base64",base64);
        recordObject.insert("url",url);
        recordObject.insert("id",id);
        recordsArray.push_back(recordObject);
    }
    json.setArray(recordsArray);
    return json.toJson();
}
