#include "radio.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>


radio::radio(QObject *parent,int volume) : QObject(parent)
{
    this->setObjectName("radio-manager");

    setting_path =  QStandardPaths::writableLocation(QStandardPaths::DataLocation);

    QProcess *fifo = new QProcess(0);
    fifo->start("mkfifo",QStringList()<<setting_path+"/fifofile");

    radioPlaybackTimer = new QTimer(this);

    radioProcess = new QProcess(this);
    radioProcess->setObjectName("_radio_");//
//    radioProcess->start("bash",QStringList()<<"-c"<<"mpv --audio-display=no --no-video --input-ipc-server="+setting_path+"/fifofile --volume "+QString::number(volume)+" '"+url.toString()+"'");
    radioProcess->start("bash",QStringList()<<"-c"<<"mpv --gapless-audio=yes --audio-display=no --no-video --input-ipc-server="+setting_path+"/fifofile --volume "+QString::number(volume)+" --idle");
    radioProcess->waitForStarted();

    connect(radioProcess,SIGNAL(readyRead()),this,SLOT(radioReadyRead()));
    connect(radioProcess,SIGNAL(finished(int)),this,SLOT(radioFinished(int)));
//    radioProcess->waitForReadyRead();

    connect(radioPlaybackTimer, &QTimer::timeout, [=](){
        if(radioProcess->state()==QProcess::Running){
            QProcess *playbackState = new QProcess(0);
            playbackState->start("bash",QStringList()<<"-c"<< "echo '{\"command\": [\"get_property\" ,\"pause\"]}' | socat - "+ setting_path+"/fifofile");
            playbackState->waitForStarted();
            playbackState->waitForReadyRead();
            QString state = QString(playbackState->readAll()).split("\"data\":").last().split(",").first();
            if(state=="false"){
                radioState = "playing";
            }else if (state=="true"){
                radioState = "paused";
            }
            emit radioStatus(radioState);


            QProcess *playerPos = new QProcess(0);
            playerPos->start("bash",QStringList()<<"-c"<< "echo '{\"command\": [\"get_property\" ,\"time-pos\"]}' | socat - "+ setting_path+"/fifofile");
            playerPos->waitForStarted();
            playerPos->waitForReadyRead();
            emit radioPosition((int)QString(playerPos->readAll()).split("\"data\":").last().split(",").first().toDouble());

            QProcess *playerDur = new QProcess(0);
            playerDur->start("bash",QStringList()<<"-c"<< "echo '{\"command\": [\"get_property\" ,\"duration\"]}' | socat - "+ setting_path+"/fifofile");
            playerDur->waitForStarted();
            playerDur->waitForReadyRead();
            emit radioDuration((int)QString(playerDur->readAll()).split("\"data\":").last().split(",").first().toDouble());


            QList<QProcess*> radio_process_list;
            radio_process_list = this->findChildren<QProcess*>("_radio_");
            qDebug()<<"RUNNING RADIO PROCESS:"<<radio_process_list.count();
        }
    });
}


void radio::playRadio(QUrl url){

    if(radioProcess != nullptr)
     loadMedia(url);

}

void radio::loadMedia(QUrl url){
    QProcess *fifo = new QProcess(0);
    fifo->start("bash",QStringList()<<"-c"<< "echo '{\"command\": [\"loadfile\" ,\""+url.toString()+"\""+"]}' | socat - "+ setting_path+"/fifofile");
    qDebug()<<fifo->program()<<fifo->arguments();
    fifo->waitForStarted();
}


void radio::radioReadyRead(){
    if(!radioPlaybackTimer->isActive()){
        radioPlaybackTimer->start(1000);
    }
    QTextBrowser *console =  this->parent()->findChild<QTextBrowser *>("console");
    ((QTextBrowser*)(console))->setText(radioProcess->readAll());
}

void radio::radioFinished(int code){
    if(code == 0){
        radioState = "exit";
        emit radioStatus("exit");
        radioProcess->deleteLater();
        radioPlaybackTimer->stop();
    }else{
        radioState = "crashed";
        emit radioStatus("crashed");
        radioProcess->deleteLater();
        radioPlaybackTimer->stop();
    }
}

void radio::radioSeek(int pos){
    QProcess *fifo = new QProcess(0);
    fifo->start("bash",QStringList()<<"-c"<< "echo '{\"command\": [\"set_property\" ,\"time-pos\","+QString::number(pos)+"]}' | socat - "+ setting_path+"/fifofile");
    fifo->waitForStarted();
}

void radio::pauseRadio()
{
      QProcess *fifo = new QProcess(0);
      fifo->start("bash",QStringList()<<"-c"<<"echo cycle pause >> "+ setting_path+"/fifofile");
      fifo->waitForStarted();

      radioState = "paused";
      emit radioStatus(radioState);

      radioPlaybackTimer->stop();
}

void radio::resumeRadio()
{
    QProcess *fifo = new QProcess(0);
    fifo->start("bash",QStringList()<<"-c"<<"echo cycle pause >> "+ setting_path+"/fifofile");
    fifo->waitForStarted();

    radioState = "playing";
    emit radioStatus(radioState);

    radioPlaybackTimer->start(1000);
}

void radio::changeVolume(int volume)
{
    QProcess *fifo = new QProcess(0);
    fifo->start("bash",QStringList()<<"-c"<< "echo '{\"command\": [\"set_property\" ,\"volume\","+QString::number(volume)+"]}' | socat - "+ setting_path+"/fifofile");
}

void radio::quitRadio()
{
    if(radioProcess!=nullptr){
        if(radioPlaybackTimer->isActive() || radioProcess->state()==QProcess::Running){
            QProcess *fifo = new QProcess(0);
            fifo->start("bash",QStringList()<<"-c"<<"echo '{\"command\": [\"quit\"]}' | socat - "+ setting_path+"/fifofile");
            fifo->waitForStarted();
            radioState="exit";
            emit radioStatus(radioState);
            radioPlaybackTimer->stop();
        }
    }
}

void radio::killRadio()
{
    QList<QProcess*> radio_process_list;
    radio_process_list = this->findChildren<QProcess*>("_radio_");
    foreach (QProcess *radio, radio_process_list) {
        qDebug()<<radio<<radio->parent();
        radio->close();
    }
    radioState="exit";
    emit radioStatus(radioState);
}

