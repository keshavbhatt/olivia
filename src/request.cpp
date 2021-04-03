#include "request.h"

Request::Request(QObject *parent) : QObject(parent)
{
    setParent(parent);
}

Request::~Request()
{
    this->deleteLater();
}

void Request::get(const QUrl url,QString uid){
    QNetworkAccessManager *m_netwManager = new QNetworkAccessManager(this);
    connect(m_netwManager,&QNetworkAccessManager::finished,[=](QNetworkReply* rep){
        if(rep->error() == QNetworkReply::NoError){
            QString repStr = rep->readAll();
            emit requestFinished(uid,repStr);
        }
        rep->deleteLater();
        m_netwManager->deleteLater();
    });
    QNetworkRequest request(url);
    m_netwManager->get(request);
}
