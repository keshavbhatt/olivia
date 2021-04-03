#ifndef MANIFEST_RESOLVER_H
#define MANIFEST_RESOLVER_H

#include <QString>
#include <QDomElement>
#include <QDomNode>
#include <QDomElement>
#include <QByteArray>
#include <QtNetwork>
#include <QObject>
#include <QMessageBox>
#include <QDebug>

class ManifestResolver : public QObject{
    Q_OBJECT

public:
    ManifestResolver(const QUrl& url, QObject* parent= nullptr);
    virtual ~ManifestResolver();
private slots:
    void downloadManifest(QUrl url);
    void getM4a(QString data);
    void slot_netwManagerFinished(QNetworkReply*);
signals:
    void m4aAvailable(QString);
    void error();
private:

};

//constructor
 inline ManifestResolver::ManifestResolver(const QUrl &url, QObject *parent)
     : QObject(parent){
     downloadManifest(url);
 }
//destructor
inline ManifestResolver::~ManifestResolver(){
     this->deleteLater();
 }

inline void ManifestResolver::downloadManifest(QUrl url){
    QNetworkAccessManager *m_netwManager = new QNetworkAccessManager(nullptr);
    connect(m_netwManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(slot_netwManagerFinished(QNetworkReply*)));
    QNetworkRequest request(url);
    m_netwManager->get(request);
}

inline void ManifestResolver::slot_netwManagerFinished(QNetworkReply *reply){
    if(reply->error() == QNetworkReply::NoError){
        int v = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (v >= 200 && v < 300) // Success
        {
            if(reply->error() == QNetworkReply::NoError){
                 getM4a(reply->readAll());
            }
        }
        else if (v >= 300 && v < 400) // Redirection
        {
            QUrl newUrl = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
            newUrl = reply->url().resolved(newUrl);
            if(newUrl.toString().contains("manifest/dash/")){
                QNetworkAccessManager *manager = new QNetworkAccessManager();
                connect(manager, SIGNAL(finished(QNetworkReply*)),this, SLOT(slot_netwManagerFinished(QNetworkReply*)));
                manager->get(QNetworkRequest(newUrl));
            }else{
                getM4a(reply->readAll());
            }
        }
    }
    else //error
    {
        emit error();
    }
    reply->deleteLater();
}

// dig xml
inline void ManifestResolver::getM4a(QString data){
    //qDebug()<<data;
    QString m4aUrl = "";
    //parse data
    QDomDocument domDocument;
    if(domDocument.setContent(data)){
        QDomNodeList nodes = domDocument.elementsByTagName("BaseURL");
        for(int i = 0; i < nodes.count(); i++) {
            QDomNode elm = nodes.at(i);
            if(elm.isElement())
            {
                if(elm.toElement().text().contains("audio%2Fmp4")||
                          elm.toElement().text().contains("audio/mp4")){
                        m4aUrl= elm.toElement().text();
                        emit m4aAvailable(m4aUrl);
                        break;
                }
            }
        }
    }else{
        qDebug()<<"ERROR";
        emit error();
    }
}

#endif // MANIFEST_RESOLVER_H


