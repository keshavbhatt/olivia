#include "analytics.h"
#include <QDateTime>
#include <QSysInfo>
#include <QFileInfo>
#include <QFile>
#include <QStandardPaths>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QByteArray>
#include "utils.h"

analytics::analytics(QObject *parent) : QObject(parent)
{
   setting_path = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
   startTime = QDateTime::currentMSecsSinceEpoch();
   QNetworkAccessManager *m_netwManager = new QNetworkAccessManager(0);
   connect(m_netwManager,&QNetworkAccessManager::finished,[=](QNetworkReply*rep){
       if(rep->error()==QNetworkReply::NoError){
            QByteArray buffer = rep->readAll();
            QJsonDocument jsonDoc(QJsonDocument::fromJson(buffer));
            QJsonObject jsonReply = jsonDoc.object();
            QString ip = jsonReply.value("ip").toString();
            QString  country = jsonReply.value("country").toString();
            qDebug() << data;

           addData(ip);//ip
           addData(country);
           addData(QSysInfo::currentCpuArchitecture());
           addData(QSysInfo::kernelType());
           addData(QSysInfo::kernelVersion());
           addData(QSysInfo::prettyProductName());

           //check if local user log file with old uid exists
           const QString oUid =getUidFromOldLogFile();
           if(!oUid.isEmpty()){
               //preserve old uid;
               uid = "_"+oUid.split("_").last();
           }else{
              uid = "_"+ip.replace(".","").replace(":","");
           }
           //generate random uid if no uid was formed
           if(uid.length()<4||uid.trimmed().isEmpty()){
               uid =  "_"+utils::generateRandomId(16);
           }

           //create with user file if not found with uid
           if(!QFileInfo(QFile(setting_path+"/alog"+uid)).exists()){
                  QFile file(setting_path+"/alog"+uid);
                  file.open(QIODevice::WriteOnly);
                  file.close();
                  //new user
                  getData();
                  processData();
           }
           emit analytics_ready();
       }
   });
   QNetworkRequest request(QUrl("https://ipapi.co/json"));
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
    addData(QString::number(appDuration/1000));
//    QFile file(setting_path+"/alog"+uid);
//    if (file.open(QIODevice::Append | QIODevice::Text)){
//        QTextStream out(&file);
//              out << data<< "\n";
//        file.close();
//    }
    return data;
    //"139.167.254.217<=>IN<=>x86_64<=>linux<=>4.4.0-157-generic<=>Ubuntu 16.04.6 LTS<=>1754332"
}

void analytics::headLessPush(){
    QUrl serviceUrl = QUrl("http://ktechpit.com/USS/Olivia/services/analytics.php");
    QByteArray postData;
    QUrlQuery query;
    query.addQueryItem("type","update");
    query.addQueryItem("uid",uid.split("_").last());
    query.addQueryItem("data",getData());
    postData = query.toString(QUrl::FullyEncoded).toUtf8();

    QProcess *process= new QProcess(0);
    QStringList args;
    args<<"--post-data"<<QString(postData)<<serviceUrl.toString()<<"--quiet";
    process->startDetached("wget",args);
    qDebug()<<"headless"<<postData;
}

void analytics::processData(){
    QFile file(setting_path+"/alog"+uid);
    QStringList data_list;
    if (!file.open(QIODevice::ReadWrite | QIODevice::Text))
        return;
    QTextStream in(&file);
    while(!in.atEnd()){
        QByteArray line = in.readLine().toUtf8();
        data_list = QString(line).split("<=>");
        preparePush(data_list);
    }
    file.close();
    //clear file data after processing
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return;
    file.close();
}

void analytics::preparePush(QStringList data_list){

        push("update",data_list);
}

QString analytics::getUidFromOldLogFile(){
    QDir d(setting_path);
    d.setNameFilters(QStringList()<<"alog_*");
    QString path_= "";
    foreach (QString path, d.entryList()) {
        if(path.contains("alog_")){
            path_ = path;
        }
    }
    qDebug()<<"alog path= "<<path_;
    return path_;
}

void analytics::push(QString type,QStringList data_list){
    qDebug()<<"push called";
    QUrl serviceUrl = QUrl("http://ktechpit.com/USS/Olivia/services/analytics.php");
    QByteArray postData;
    QUrlQuery query;
    if(type=="update"){
       query.addQueryItem("type",type);
       query.addQueryItem("uid",uid.split("_").last());
    }
    query.addQueryItem("data",data_list.join("<=>"));
    postData = query.toString(QUrl::FullyEncoded).toUtf8();
    QNetworkAccessManager *networkManager = new QNetworkAccessManager(this);
    connect(networkManager,&QNetworkAccessManager::finished,[=](QNetworkReply*rep){
        if(rep->error()==QNetworkReply::NoError){
            qDebug()<<"analytics: "<<"pushed.";
            qDebug()<<"analytics reply:"<<rep->readAll();
        }else{
            qDebug()<<"analytics: "<<rep->errorString();
        }
    });
    QNetworkRequest networkRequest(serviceUrl);
    networkRequest.setHeader(QNetworkRequest::ContentTypeHeader,"application/x-www-form-urlencoded");
    networkManager->post(networkRequest,postData);
}




