#include "radio.h"


radio::radio(QObject *parent) : QObject(parent)
{
    this->setObjectName("radio-manager");

    setting_path =  QStandardPaths::writableLocation(QStandardPaths::DataLocation);

    QProcess *fifo = new QProcess(0);
    fifo->start("mkfifo",QStringList()<<setting_path+"/fifofile");

    radioPlaybackTimer = new QTimer(this);

    connect(radioPlaybackTimer, &QTimer::timeout, [=](){
        if(radioProcess->state()==QProcess::Running){
            QProcess *playbackState = new QProcess(0);
            playbackState->start("bash",QStringList()<<"-c"<<"echo pausing_keep_force get_property pause  >> "+ setting_path+"/fifofile");

            QProcess *playerPos = new QProcess(0);
            playerPos->start("bash",QStringList()<<"-c"<<"echo get_time_pos >> "+ setting_path+"/fifofile");

            QProcess *playerDur = new QProcess(0);
            playerDur->start("bash",QStringList()<<"-c"<<"echo get_time_length >> "+ setting_path+"/fifofile");


            QList<QProcess*> radio_process_list;
            radio_process_list = this->findChildren<QProcess*>("_radio_");
            qDebug()<<"RUNNING RADIO PROCESS:"<<radio_process_list.count();
        }
    });
}

void radio::playRadio(QUrl url,int volume){

    if(radioProcess != nullptr){
        pauseRadio();
//        quitRadio();
        killRadio();
    }

        radioProcess = new QProcess(this);
        radioProcess->setObjectName("_radio_");
        radioProcess->start("bash",QStringList()<<"-c"<<"mplayer -slave -quiet -input file="+setting_path+"/fifofile -volume "+QString::number(volume)+" '"+url.toString()+"'");
        radioProcess->waitForStarted();
        connect(radioProcess,SIGNAL(readyRead()),this,SLOT(radioReadyRead()));
        connect(radioProcess,SIGNAL(finished(int)),this,SLOT(radioFinished(int)));
}


void radio::radioReadyRead(){
    if(!radioPlaybackTimer->isActive()){
        radioPlaybackTimer->start(1000);
    }

    QTextBrowser *console =  this->parent()->findChild<QTextBrowser *>("console");
    ((QTextBrowser*)(console))->setText(radioProcess->readAll());
    if(((QTextBrowser*)(console))->toPlainText().contains("pause=no")){
        radioState ="playing";
        emit radioStatus(radioState);
    }
    if(((QTextBrowser*)(console))->toPlainText().contains("POSITION=")){
        QString position  = ((QTextBrowser*)(console))->toPlainText().split("POSITION=").last().split("\n").first().trimmed();
//        qDebug()<<"RADIO POS: "<<(int)position.toDouble();
                emit radioPosition((int)position.toDouble());
    }
    if(((QTextBrowser*)(console))->toPlainText().contains("LENGTH=")){
        QString duration  = ((QTextBrowser*)(console))->toPlainText().split("LENGTH=").last().trimmed();
//        qDebug()<<"RADIO DUR: "<<(int)duration.toDouble();
                emit radioDuration((int)duration.toDouble());
    }

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
    fifo->start("bash",QStringList()<<"-c"<<"echo seek "+QString::number(pos)+" 2 >> "+ setting_path+"/fifofile");
    fifo->waitForStarted();
}

void radio::pauseRadio()
{
      QProcess *fifo = new QProcess(0);
      fifo->start("bash",QStringList()<<"-c"<<"echo pause >> "+ setting_path+"/fifofile");
      fifo->waitForStarted();

      radioState = "paused";
      emit radioStatus(radioState);

      radioPlaybackTimer->stop();
}

void radio::resumeRadio()
{
    QProcess *fifo = new QProcess(0);
    fifo->start("bash",QStringList()<<"-c"<<"echo pause >> "+ setting_path+"/fifofile");
    fifo->waitForStarted();

    radioState = "playing";
    emit radioStatus(radioState);

    radioPlaybackTimer->start(1000);
}

void radio::changeVolume(int volume)
{
    QProcess *fifo = new QProcess(0);
    fifo->start("bash",QStringList()<<"-c"<<"echo set_property volume "+QString::number(volume)+" >> "+ setting_path+"/fifofile");
}

void radio::quitRadio()
{
    if(radioProcess!=nullptr){
        if(radioPlaybackTimer->isActive() || radioProcess->state()==QProcess::Running){
            QProcess *fifo = new QProcess(0);
            fifo->start("bash",QStringList()<<"-c"<<"echo quit >> "+ setting_path+"/fifofile");
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

