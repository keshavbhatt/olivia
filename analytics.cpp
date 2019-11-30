#include "analytics.h"
#include <QDateTime>
#include <QSysInfo>

analytics::analytics(QObject *parent) : QObject(parent)
{
   startTime = QDateTime::currentMSecsSinceEpoch();
   QNetworkAccessManager *m_netwManager = new QNetworkAccessManager(0);
   connect(m_netwManager,&QNetworkAccessManager::finished,[=](QNetworkReply*rep){
       if(rep->error()==QNetworkReply::NoError){
           addData(rep->readAll());
           addData(QSysInfo::currentCpuArchitecture());
           addData(QSysInfo::kernelType());
           addData(QSysInfo::kernelVersion());
           addData(QSysInfo::prettyProductName());
       }
   });
   QNetworkRequest request(QUrl("https://api.ipify.org/")); //https://ipapi.co/json/
   m_netwManager->get(request);
}

void analytics::addData(QString arg1){
    if(data.isEmpty()){
        data.append(arg1);
    }else{
        data.append("<=>").append(arg1);
    }
}

QString analytics::getData(){
    qint64 appDuration = QDateTime::currentMSecsSinceEpoch() - startTime;
    addData(QString::number(appDuration));
    return data;
//  "139.167.254.217<=>x86_64<=>linux<=>4.4.0-157-generic<=>Ubuntu 16.04.6 LTS<=>1754332"
}
