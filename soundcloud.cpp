#include "soundcloud.h"
#include "request.h"
#include <QDebug>

SoundCloud::SoundCloud(QObject *parent) : QObject(parent)
{
    getHome();
}

bool SoundCloud::cidEmpty(){
    return cid.trimmed().isEmpty();
}

void SoundCloud::extract_cid(QString rep)
{
    cid = rep.split("client_id:\"").last().split("\"").first();
}

void SoundCloud::getHome()
{
    Request *req = new Request(this);
    connect(req,&Request::requestFinished,[=](QString uid,QString rep)
    {
        Q_UNUSED(uid);
        getJs(rep);
        req->deleteLater();
    });
    req->get(home,"0");
}

void SoundCloud::getJs(const QString res){
    QRegExp regExp("src=*\"([^\"]+.js)");
    jsList.clear();
    int pos = 0;
    while ((pos = regExp.indexIn(res, pos)) != -1)
    {
        jsList << regExp.cap(1);
        pos += regExp.matchedLength();
    }
    //reverse list
    for(int k=0, s=jsList.size(), max=(s/2); k<max; k++) jsList.swap(k,s-(1+k));
    getCid();
}

void SoundCloud::getCid(){
    if(cidjs.trimmed().isEmpty())
    {
        Request *req = new Request(this);
        connect(req,&Request::requestFinished,[=](QString uid,QString rep){
            if(rep.contains("client_id:\"")){
                qDebug()<<"CLIENT ID FOUND IN"<<uid;
                cidjs = rep;
                extract_cid(rep);
            }else{
                if(jsId<jsList.count()){
                    jsId = jsId + 1;
                    getCid();
                }
            }
            req->deleteLater();
        });
        qDebug()<<"REQUEST:"<<jsId<<jsList.at(jsId);
        req->get(jsList.at(jsId),QString::number(jsId));
    }
}



