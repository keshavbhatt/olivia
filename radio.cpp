#include "radio.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>


radio::radio(QObject *parent,int volumeValue,bool saveTracksAfterBufferMode) : QObject(parent)
{
    this->setObjectName("radio-manager");
    setting_path =  QStandardPaths::writableLocation(QStandardPaths::DataLocation);

    QProcess *fifo = new QProcess(this);
    fifo->start("mkfifo",QStringList()<<setting_path+"/fifofile");
    connect(fifo, SIGNAL(finished(int)), this, SLOT(deleteProcess(int)) );

    radioPlaybackTimer = new QTimer(this);
    volume= volumeValue;
    saveTracksAfterBuffer = saveTracksAfterBufferMode;

    startRadioProcess(saveTracksAfterBuffer,"",false);

}

void radio::startRadioProcess(bool saveTracksAfterBufferMode, QString urlString, bool calledByCloseEvent){
    if(calledByCloseEvent){
        qDebug()<<"not starting radio process"<<calledByCloseEvent;
     return;
    }
    qDebug()<<"starting radio process"<<calledByCloseEvent;
    radioProcess = new QProcess(this);
    radioProcess->setProcessChannelMode(QProcess::MergedChannels);
    connect(radioProcess,SIGNAL(readyRead()),this,SLOT(radioReadyRead()));
    connect(radioProcess,SIGNAL(finished(int)),this,SLOT(radioFinished(int)));

    QString status_message_arg = "--term-status-msg='[olivia:][${=time-pos}][${=duration}][${=pause}][${=paused-for-cache}][${idle-active}][${cache-buffering-state}%][${=demuxer-cache-duration}][${=seekable}][${=audio-bitrate}][${seeking}]'";//[${=eof-reached}]

    radioProcess->setObjectName("_radio_");

    if(urlString.isEmpty()){
            radioProcess->start("bash",QStringList()<<"-c"<<"mpv "+status_message_arg+" --keep-open --keep-open-pause=no  --demuxer-max-back-bytes=5000000 --no-ytdl --gapless-audio=yes --audio-display=no --no-video --input-ipc-server="+setting_path+"/fifofile --volume "+QString::number(volume)+" --idle");
    }else{
        if(!saveTracksAfterBufferMode)
            radioProcess->start("bash",QStringList()<<"-c"<<"mpv "+status_message_arg+"  --keep-open --keep-open-pause=no --demuxer-max-back-bytes=5000000 --no-ytdl --gapless-audio=yes --audio-display=no --no-video --input-ipc-server="+setting_path+"/fifofile --volume "+QString::number(volume)+" --idle");
        else
            radioProcess->start("bash",QStringList()<<"-c"<<"wget -O - '"+urlString+"' | tee "+setting_path+"/downloadedTemp/current.temp"+" | mpv "+status_message_arg+" --keep-open --keep-open-pause=no --no-ytdl --gapless-audio=yes --audio-display=no --no-video --input-ipc-server="+setting_path+"/fifofile --volume "+QString::number(volume)+" --idle -");
    }
    radioProcess->waitForStarted();

    radioPlaybackTimer->disconnect();

    connect(radioPlaybackTimer, &QTimer::timeout, [=](){

        //check olivia's idle state
        QProcess *fifo = new QProcess(this);
        connect(fifo, SIGNAL(finished(int)), this, SLOT(deleteProcess(int)) );
        fifo->start("bash",QStringList()<<"-c"<<"echo '{\"command\":[\"get_property\" , \"idle-active\"]}' | socat - "+ setting_path+"/fifofile");
        fifo->waitForStarted();

        connect(fifo,&QProcess::readyRead,[=](){
            QString out = fifo->readAll();
            if(out.contains("success")){
                out = out.split(",").first().split(":").last();
                if(out=="true"){
                    radioState = "stopped";
                    emit radioStatus(radioState);
                    radioPlaybackTimer->stop();
                }
            }
        });

        //check olivia's EOF state
        QProcess *fifoEOF = new QProcess(this);
        connect(fifoEOF, SIGNAL(finished(int)), this, SLOT(deleteProcess(int)) );
        fifoEOF->start("bash",QStringList()<<"-c"<<"echo '{\"command\":[\"get_property\" , \"eof-reached\"]}' | socat - "+ setting_path+"/fifofile");
        fifoEOF->waitForStarted();

        connect(fifoEOF,&QProcess::readyRead,[=](){
            QString out = fifoEOF->readAll();
            QJsonDocument jsonResponse = QJsonDocument::fromJson(out.toUtf8());
            QJsonObject jsonObject = jsonResponse.object();
            QJsonValue eofVal = jsonObject.value("data");
                if(eofVal.isBool() && eofVal.toBool()==true && !eofVal.isUndefined()){
                    radioState = "eof";
                    emit radioStatus(radioState);
                    radioState = "playing";
                    radioPlaybackTimer->stop();
                }
        });



        if(radioProcess->state()==QProcess::Running){

//            QString state_line = this->parent()->findChild<QTextBrowser *>("console")->toPlainText().trimmed();

            QString position,duration,paused,paused_for_cache,idle_active,cache_buffering_state,
                    demuxer_cache_duration,seekable,audio_bitrate,seeking;
            QStringList items;
            items.append(state_line.split("]"));
            //replace [ from item value
            for (int i=0;i<items.count();i++) {
                if(items.at(i).contains("[")){
                    items.replace(i,QString(items.at(i)).remove("["));
                }
            }
            //assign items to vars
            if(items.count()==12){
                position                = items.at(1);
                duration                = items.at(2);
                paused                  = items.at(3);
                paused_for_cache        = items.at(4);
                idle_active             = items.at(5);
                cache_buffering_state   = items.at(6);
                demuxer_cache_duration  = items.at(7);
                seekable                = items.at(8);
                audio_bitrate           = items.at(9);
                seeking                 = items.at(10);
            }

            if(paused=="no"){
                radioState = "playing";
            }

            if(paused_for_cache=="yes"){
                radioState = "buffering";
            }

            if(paused!="yes"){
                if(seeking=="yes"){
                    radioState = "seeking";
                }
            }


            if(!demuxer_cache_duration.isEmpty()){
                emit demuxer_cache_duration_changed((double)demuxer_cache_duration.toDouble(),(double)position.toDouble());
            }


            if(!audio_bitrate.isEmpty()){
              //  emit radioAudioBitrate((int)audio_bitrate.toDouble());
            }


            int playerPosition = (int)position.toDouble();
            int playerDuration = (int)duration.toDouble();


           // qDebug()<<playerPosition;
            emit radioPosition(playerPosition);
            emit radioDuration(playerDuration);
            emit radioStatus(radioState);

        }
    });

    if(!radioPlaybackTimer->isActive())
       radioPlaybackTimer->start(500);
}


void radio::playRadio(bool saveTracksAfterBufferMode,QUrl url){

    state_line.clear();

    streamUrl = url.toString();
    saveTracksAfterBuffer = saveTracksAfterBufferMode;
    if(radioProcess!=nullptr){
        if(saveTracksAfterBufferMode)
            killRadioProcess();
        else
            loadMedia(url);
    }else{
         startRadioProcess(saveTracksAfterBuffer,streamUrl,false);
    }
}

void radio::loadMedia(QUrl url){
//    QTextBrowser *console =  this->parent()->findChild<QTextBrowser *>("console");
//    ((QTextBrowser*)(console))->clear();
    state_line.clear();

   // qDebug()<<"loadmedia called";
    QProcess *fifo = new QProcess(this);
    connect(fifo, SIGNAL(finished(int)), this, SLOT(deleteProcess(int)) );
    fifo->start("bash",QStringList()<<"-c"<< "echo '{\"command\": [\"loadfile\" ,\""+url.toString()+"\""+"]}' | socat - "+ setting_path+"/fifofile");
    fifo->waitForStarted();

    if(radioState=="paused"){
        resumeRadio();
    }
}

void radio::radioReadyRead(){
    if(!radioPlaybackTimer->isActive()){
        radioPlaybackTimer->start(500);
    }
    QString output = radioProcess->readAll();
    if(output.contains("written to stdout")){
        emit saveTrack(QString("webm"));
    }
    if(output.contains("[olivia:]")){
        // mpv sometimes sends output without ] in line this will append ] and fix it
        output = output.trimmed();
        if(output.at(output.count()-1)!=']')
            output.append("]");
            state_line = output.trimmed();
//        QTextBrowser *console =  this->parent()->findChild<QTextBrowser *>("console");
//        ((QTextBrowser*)(console))->setText(output);
    }else if(output.contains("failed",Qt::CaseInsensitive)){
        QTextBrowser *console =  this->parent()->findChild<QTextBrowser *>("console");
        ((QTextBrowser*)(console))->setText(output);
        radioState="failed";
        emit radioStatus(radioState);
    }
}


void radio::radioFinished(int code){
    if(code == 0){
        radioState = "exit";
        emit radioStatus("exit");
        radioProcess->deleteLater();
        radioPlaybackTimer->stop();
    }else{
        radioState = "loading";
        emit radioStatus("loading");
        radioProcess->deleteLater();
        radioPlaybackTimer->stop();
        startRadioProcess(saveTracksAfterBuffer,streamUrl,false);
    }
}

void radio::radioSeek(int pos){
    QProcess *fifo = new QProcess(this);
    connect(fifo, SIGNAL(finished(int)), this, SLOT(deleteProcess(int)) );
    fifo->start("bash",QStringList()<<"-c"<< "echo '{\"command\": [\"set_property\" ,\"time-pos\","+QString::number(pos)+"]}' | socat - "+ setting_path+"/fifofile");
    fifo->waitForStarted();
}

void radio::pauseRadio()
{
      QProcess *fifo = new QProcess(this);
      connect(fifo, SIGNAL(finished(int)), this, SLOT(deleteProcess(int)) );
      fifo->start("bash",QStringList()<<"-c"<<"echo '{\"command\": [\"cycle\" , \"pause\"]}' | socat - "+ setting_path+"/fifofile");
      fifo->waitForStarted();

      radioState = "paused";
      emit radioStatus(radioState);

      radioPlaybackTimer->stop();
}

void radio::resumeRadio()
{
    QProcess *fifo = new QProcess(this);
    connect(fifo, SIGNAL(finished(int)), this, SLOT(deleteProcess(int)) );
    fifo->start("bash",QStringList()<<"-c"<<"echo '{\"command\": [\"cycle\" , \"pause\"]}' | socat - "+ setting_path+"/fifofile");
    fifo->waitForStarted();

    radioState = "playing";
    emit radioStatus(radioState);

    radioPlaybackTimer->start(500);
}

void radio::changeVolume(int val)
{
    volume = val;
    QProcess *fifo = new QProcess(this);
    connect(fifo, SIGNAL(finished(int)), this, SLOT(deleteProcess(int)) );
    fifo->start("bash",QStringList()<<"-c"<< "echo '{\"command\": [\"set_property\" ,\"volume\","+QString::number(volume)+"]}' | socat - "+ setting_path+"/fifofile");
}

//will use to quit radio process when it will take too much RAM
void radio::quitRadio()
{
    if(radioProcess!=nullptr){
        if(radioPlaybackTimer->isActive() || radioProcess->state()==QProcess::Running){
            radioState="exit";
            emit radioStatus(radioState);
            radioPlaybackTimer->stop();
            startRadioProcess(saveTracksAfterBuffer,"http://google.com",true);
        }
    }
}

void radio::deleteProcess(int code){
    QList<QProcess*> radio_process_list;
    radio_process_list = this->findChildren<QProcess*>();
//    qDebug()<<"NUMBER OF PROCESS:"<<radio_process_list.count();
    Q_UNUSED(code);
    const QObject *sen = sender();
    delete sen;
}

void radio::killRadioProcess(){
    if(radioProcess->state()==QProcess::Running){
      QProcess::execute("pkill",QStringList()<<"-P"<<QString::number(radioProcess->processId()));
      delete radioProcess;
    }
}

void radio::stop(){
    QProcess *fifo = new QProcess(this);
    connect(fifo, SIGNAL(finished(int)), this, SLOT(deleteProcess(int)) );
    fifo->start("bash",QStringList()<<"-c"<< "echo '{\"command\": [\"stop\"]}' | socat - "+ setting_path+"/fifofile");
    fifo->waitForStarted();
    radioPlaybackTimer->stop();
}

