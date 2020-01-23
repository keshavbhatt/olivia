#include "youtube.h"
#include <QProcess>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTextDocument>


Youtube::Youtube(QObject *parent,QWebView *webview) : QObject(parent)
{
    setParent(parent);
    view = webview;
    setting_path =  QStandardPaths::writableLocation(QStandardPaths::DataLocation);
}

QString Youtube::getCurrentCountry(){
    return settings.value("country").toString().toUpper();
}

void Youtube::saveGeo(QString country){
    settings.setValue("country",country.toLower());
    emit setCountry(settings.value("country").toString().toUpper());
}

void Youtube::flat_playlist(QVariant playlist_id){
   QString p_id = playlist_id.toString();
   QString url_str = "https://m.youtube.com/playlist?list="+p_id;
   QProcess *flatter = new QProcess(this);
   connect(flatter,SIGNAL(finished(int)),this,SLOT(flatterFinished(int)));
   flatter->start("python",QStringList()<<setting_path+"/core"<<"--dump-single-json"<<"--flat-playlist"<<url_str);
   flatter->waitForStarted();
   qDebug()<<"flatter started with:"<<flatter->pid();
}

void Youtube::flatterFinished(int exitCode){
    view->page()->mainFrame()->evaluateJavaScript("hideUiLoader()");
    qDebug()<<"flatter exited with code:"<<exitCode;
    if(exitCode==0){
        QProcess *flatter = static_cast<QProcess*>(sender());
        if(flatter!=nullptr){
            QJsonDocument jsonResponse = QJsonDocument::fromJson(QString(flatter->readAll()).toUtf8());

            QJsonObject jsonObject = jsonResponse.object();

            QString playlist_id = jsonObject.value("id").toString();
            QString playlist_name = jsonObject.value("title").toString();
            QString uploader_name = jsonObject.value("uploader").toString();
            QString uploader_id  = jsonObject.value("uploader_id").toString();
            qDebug()<<uploader_id;
            QString uploader_url ;//= jsonObject.value("uploader_url").toString();
            uploader_url = "search.php?channelId="+uploader_id+"&channelTitle="+QString(uploader_name.replace(" ","+"));
            uploader_url = QUrl::toPercentEncoding(uploader_url);
            QString html,list_items;

            int videoAdded = 0;
            QJsonArray jsonArray = jsonObject["entries"].toArray();
            if(jsonArray.count()>0){
                foreach (const QJsonValue & value, jsonArray) {
                    QJsonObject obj = value.toObject();
                    QString videoId ,title ,artist,album ,thumb
                            ,songId ,artistId ,albumId ,trackTimeMillis;
                    videoId = obj.value("id").toString();
                    title = obj.value("title").toString();
                    artist = uploader_name;
                    album = "undefined";
                    thumb = "https://i.ytimg.com/vi/"+videoId+"/mqdefault.jpg";
                    songId = videoId;
                    artistId = artist;
                    albumId = "undefined-"+videoId;
                    trackTimeMillis ="0";

                    title= title.replace("'"," ");
                    artist=artist.replace("'"," ").replace(" ","&nbsp;");
                    album=album.replace("'"," ");

                    if(title.contains("[Deleted video]",Qt::CaseInsensitive) || title.contains("[Private video]",Qt::CaseInsensitive)){
                        qDebug()<<title;
                    }else{
                        videoAdded++;
                        list_items.append("<li>");
                        list_items.append("<a data-channelhref='"+uploader_url+"' onclick='gettrackinfo(\""+title+"!=-=!"+artist+"!=-=!"+album+"!=-=!"+thumb+"!=-=!"+songId+"!=-=!"+albumId+"!=-=!"+artistId+"!=-=!"+trackTimeMillis+"!=-=!"+videoId+"\")'>");
                        list_items.append("<img id='"+videoId+"' style='max-width:250px  !important ;max-height:143px' src='"+thumb+"'>");
                        list_items.append("<p style='margin-left:13.5em  !important;height:100px;'>");
                        list_items.append("<b>"+title+"</b><br>");
                        list_items.append("<br><i class='ellipsis'>Added to: "+playlist_name+". <br> by:"+artist+"</i><br></p></a>");
                        list_items.append("<a href='#' onclick='track_option(\""+videoId+"\")'>More Options</a>");
                        list_items.append("<span>");
                        list_items.append("<a style='right: 3.8em;' onclick='watch_video(\""+videoId+"\")' id='"+videoId+"_watchVideo' title='Watch Video' class='ui-li-count my-watch-video-button' href='#' data-role='button' data-iconpos='notext' data-icon='video' data-theme='b'>");
                        list_items.append("Watch</a></span>");
                        list_items.append("</li>");
                    }
                }
                QString header = "<div id='HEADER_DIV' style='height:250px;background-image:url("+QString("\"channel.png\"")+"); background-size: cover;"
                                 "text-align:center; background-position: center;text-shadow: 1px 3px 14px rgb(0, 0, 0);"
                                 "box-shadow:rgb(0, 0, 0) 0px 0px 30px;'>"
                                 "   <div id='HEADER_WRAPPER' style='height:100%;background-color:rgba(0, 0, 0, 0.27);' >"
                                 "      <h2 style='font-size: 2em;position: relative;top: 35%;margin: 0px auto;"
                                 "         background-color: rgba(0, 0, 0, 0.49);padding: 20px 0px;'>"+playlist_name+"</h2>"
                                 "   </div>"
                                 "</div>"
                                 "<br>";
                html.append(header);
                html.append("<i style='display: block;text-align:center;' class='ellipsis'>Video count: "+QString::number(videoAdded)+" </i>");
                html.append("<br>");
                html.append("<ul class='list' id='manul_youtube_page_result'  data-role='listview' data-split-icon='bars' data-split-theme='b' data-inset='true'>");
                html.append(list_items);
                html.append("</ul>");
                html= html.toUtf8();
                html = html.toHtmlEscaped();
                view->page()->mainFrame()->evaluateJavaScript("setPlaylistVideos(\""+html+"\")");
            }
        }
    }
}

