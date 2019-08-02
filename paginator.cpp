#include "paginator.h"
#include <QDateTime>
#include <QWebView>
#include <QWebFrame>
#include <QWebPage>

paginator::paginator(QObject *parent) : QObject(parent)
{
    QString setting_path =  QStandardPaths::writableLocation(QStandardPaths::DataLocation);
    QDir paginator(setting_path+"/paginator");
    if(!paginator.exists()){
        if(paginator.mkdir(paginator.path())){
            qDebug()<<"created paginator dir";
        }
    }
    paginator_path = paginator.absolutePath();
}

//used to save data returned from function dataType of page pageType for query query
void paginator::save(QString pageType,QString dataType,QString query,QString data){
    QString path = createDir(pageType.trimmed()+"/"+dataType.trimmed());
    QFile file(path+query.trimmed());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
              return;
    QTextStream out(&file);
    QString loadOnlineBtn;
    if(pageType=="album_view"){
        loadOnlineBtn  = "<a style='font-size: 0.88em; padding-top: 10px;margin: 10px 0px;background-color: rgba(36, 142, 179, 0.66);border: none;' class='ui-shadow-icon ui-btn ui-shadow ui-corner-all ui-icon-clock ui-btn-icon-left' href='#' onclick='paginator.deleteCache(\""+pageType+"\",\""+dataType+"\",\""+query+"\")'> Saved on "+QDateTime::currentDateTime().toLocalTime().toString()+" - Reload</a>";
    }else{
        loadOnlineBtn  = "<a style='font-size: 0.88em; padding-top: 10px;margin: 10px 0px;background-color: rgba(36, 142, 179, 0.66);border: none;' class='ui-shadow-icon ui-btn ui-shadow ui-corner-all ui-icon-clock ui-btn-icon-left' href='#' onclick='paginator.deleteCache(\""+pageType+"\",\""+dataType+"\",\""+query+"\")'> Saved on "+QDateTime::currentDateTime().toLocalTime().toString()+" - Reload</a>";
    }
    out <<loadOnlineBtn<< data.trimmed();
}

//used to load data from offline page pageType for funcction dataType for query query
QString paginator::load(QString pageType,QString dataType,QString query){
    QFile file(getPath(pageType,dataType,query));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)){
        qDebug()<<"unable to open file";
    }
    QTextStream in(&file);
    return in.readAll();
}

//used to check if data is present in offline storage
bool paginator::isOffline(QString pageType,QString dataType,QString query){
   return QFileInfo(QFile(getPath(pageType,dataType,query))).exists();
}


//core
//used to create base path where query file will be saved
QString paginator::createDir(QString name){  // name without slashes
    QDir dir(paginator_path+"/"+name.trimmed());
    dir.mkpath(dir.path());
    return dir.path()+"/";
}

//used to get full path of query file
QString paginator::getPath(QString pageType,QString dataType,QString query){
    QDir file(paginator_path+"/"+pageType.trimmed()+"/"+dataType.trimmed()+"/"+query.trimmed());
    //qDebug()<<file.path();
    return file.path();
}

//load new data online
void paginator::deleteCache(QString pageType,QString dataType,QString query){
    qDebug()<<pageType<<dataType<<query;
    QString path = createDir(pageType.trimmed()+"/"+dataType.trimmed());
    QFile file(path+query.trimmed());
    file.remove();
    emit reloadRequested(dataType,query);
}


//load hostory

//currently used to get youtube_history (recent searches)
QString paginator::getList(QString page,QString dataType){
    QStringList files;
    files<<"fake-item"<<QDir(getPath(page,dataType,"")).entryList(QDir::Files,QDir::Time);

    QString html;
    int row=0;
    int numberOfButtons=0;
    QString col;
    while (numberOfButtons<=files.count())
    {
        for (int f2=0; f2<3; f2++)
        {   numberOfButtons++;
            if (numberOfButtons>files.count()-1)
                break;
            if(f2==0) col = "a";
            if(f2==1) col = "b";
            if(f2==2) col = "c";
            if(numberOfButtons>=10)
                break;
            html.append("<div class='ui-block-"+col+"'><a onclick='"+dataType+"(\""+files.at(numberOfButtons)+"\");$(\"#manual_search\").val(\""+files.at(numberOfButtons)+"\")' class='ui-shadow-icon ui-btn ui-shadow ui-corner-all ui-icon-clock ui-btn-icon-left ui-mini'>"+files.at(numberOfButtons)+"</a></div>");
        }
        row++;
    }
    return html;
}

//used to clear youtube search history
void paginator::clearRecentSearches(){
    QFileInfoList filesInfo;
    filesInfo<<QDir(getPath("youtube","manual_youtube_search","")).entryInfoList(QDir::Files,QDir::Time);
    foreach (QFileInfo fileName, filesInfo) {
        QFile(fileName.filePath()).remove();
    }
}
