#include "store.h"

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
                 qDebug()<<"Store open for queries"<<setting_path+"/storeDatabase/"+dbName+".db";
             }else if(!db.isOpen()){
                 qDebug()<<"Store not Open , opening...";
                 openDb(dbName,"old");
             }
        }
}

//creates storeDatabase file with dbname given in defined pAths and opens it for transactions
void store::openDb(QString dbName,QString type){
    QString path = QStandardPaths::writableLocation(QStandardPaths::DataLocation)+"/storeDatabase/"+dbName+"/";
    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(path+dbName+".db");
    if(db.open()){
        if(type=="new"){
            createTable(dbName);
        }else{
            qDebug()<<"Store open for queries"<<path+dbName+".db";
        }
    }
}

// creates table in given DB name OR makes default structure of DB
void store::createTable(QString dbName){
    QString path = QStandardPaths::writableLocation(QStandardPaths::DataLocation)+"/storeDatabase/"+dbName+"/";
    //QString dbname = dbName.split("/").last().split(".db").first(); //default

    //tables
    QSqlQuery track;
    bool track_created = track.exec("create table tracks "
              "(trackId INTEGER PRIMARY KEY, "
              "albumId varchar(300), "
              "artistId varchar(300),"
              "title varchar(300),"
              "downloaded int(1))");
    QSqlQuery queue;
    bool queue_created = queue.exec("create table queue "
              "(id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,"
                "trackId integer )"
               );

    QSqlQuery artist;
    bool artist_created = artist.exec("create table artist "
              "(artistId INTEGER PRIMARY KEY,"
              "artistName varchar(300))"
               );

    QSqlQuery album;
    bool album_created = album.exec("create table album "
              "(albumId varchar(300) PRIMARY KEY,"
              "albumName varchar(300))"
               );

    QSqlQuery color;
    bool color_created = color.exec("create table color "
              "(albumId varchar(300) PRIMARY KEY,"
              "colorName varchar(300))"
               );

    QSqlQuery ytIds;
    bool ytIds_created = ytIds.exec("create table ytIds "
              "(trackId INTEGER PRIMARY KEY,"
              "ids varchar(500))"
               );

    QSqlQuery art;
    bool art_created = art.exec("create table arts "
              "(albumId varchar(300) PRIMARY KEY,"
              "artId varchar(300))"
               );

    QSqlQuery streamUrl;
    bool streamUrl_created = streamUrl.exec("create table stream_url "
              "(trackId INTEGER PRIMARY KEY,"
              "url varchar(500),"
              "timeOfExpiry varchar(70))"
               );

    if(track_created && queue_created && album_created && artist_created && color_created && ytIds_created && art_created && streamUrl_created){
        qDebug()<<"Database tables ready for"<<path+dbName+".db";
        initStore(dbName);
    }
}

//closes the DB
void store::closeDb(QString dbName){
    QString path = QStandardPaths::writableLocation(QStandardPaths::DataLocation)+"/storeDatabase/"+dbName;
    QSqlDatabase::removeDatabase(path+"/storeDatabase/"+dbName+".db");
    qDebug()<<"Acitive db connections:"<<QSqlDatabase::connectionNames();
}

//MODIFICATION METHODS
//saves tracks in db with all columns in tracks table in DB
void store::setTrack(QStringList meta){
    QString title = meta.at(3);
    QString albumId = meta.at(1);
    QSqlQuery query;
    query.exec("INSERT INTO tracks('trackId','albumId','artistId','title') "
               "VALUES('"+meta.at(0)+"','"+albumId.trimmed().remove("\"").replace("'","''").remove(QChar('\\')).remove("/")+"','"+meta.at(2)+"','"+title.remove("\"").replace("'","''")+"')");
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
    QSqlQuery query;
    query.exec("SELECT timeOfExpiry FROM stream_url WHERE trackId = '"+trackId+"'");
    bool expired = false;
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
        if(isDownloaded(songId)){
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
// returns json array string of local albums
QString store::web_print_saved_albums(){
    qDebug()<<"LOAD LOCAL SAVED ALBUMS";
    QJsonDocument json;
    QJsonArray recordsArray;
    foreach (QStringList albumList, getAllAlbums()) {
        QJsonObject recordObject;
        QString albumId,albumName,artistName,base64,dominantColor,artistId,tracksCount;
        albumId = albumList.at(0);
        albumName = albumList.at(1);
        base64 = albumList.at(2);
        dominantColor = albumList.at(3);
        artistName = albumList.at(4);
        artistId = albumList.at(5);
        tracksCount = albumList.at(6);

        recordObject.insert("albumId",albumId);
        recordObject.insert("albumName",albumName);
        recordObject.insert("base64",base64);
        recordObject.insert("dominantColor",dominantColor);
        recordObject.insert("artistName",artistName);
        recordObject.insert("artistId",artistId);
        recordObject.insert("tracksCount",tracksCount);
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

        recordObject.insert("artistId",artistId);
        recordObject.insert("artistName",artistName);
        recordObject.insert("tracksCount",tracksCount);
        recordsArray.push_back(recordObject);
    }
    json.setArray(recordsArray);
    return json.toJson();
}
