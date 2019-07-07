#include "videooption.h"
#include "ui_videooption.h"
#include "elidedlabel.h"
#include "manifest_resolver.h"

#include <QRadioButton>

VideoOption::VideoOption(QWidget *parent,store *store) :
    QWidget(parent),
    ui(new Ui::VideoOption)
{
    ui->setupUi(this);
    if(store_manager==nullptr){
        store_manager = store;
    }
}

VideoOption::~VideoOption()
{
    delete ui;
}

void VideoOption::setMeta(QString songId){

    foreach(QRadioButton *rBtn,ui->quality_box->findChildren<QRadioButton*>()){
        rBtn->deleteLater();
    }
    ui->watch->setEnabled(false);
    audioCode.clear();
    videoCode.clear();
    resolution_List.clear();
    audioUrl.clear();
    videoUrl.clear();
    currentUrl.clear();

    QStringList trackMetaList = store_manager->getTrack(songId);
    QString ytIds,title,artist,album,base64,dominantColor,albumId,artistId,url;
    title = trackMetaList.at(1);
    albumId = trackMetaList.at(2);
    album = trackMetaList.at(3);
    artistId = trackMetaList.at(4);
    artist = trackMetaList.at(5);
    base64 = trackMetaList.at(6);
    url = trackMetaList.at(7);
    ytIds = trackMetaList.at(8);
    dominantColor = trackMetaList.at(9);

    QTextDocument text;
    text.setHtml(title);
    QString plainTitle = text.toPlainText();

    QFont font("Ubuntu");
    font.setPixelSize(12);

    ui->titlle->setFont(font);
    ui->titlle->setText(plainTitle);

    ui->artist->setFont(font);
    ui->artist->setText(artist);

    ui->album->setFont(font);
    ui->album->setText(album);

    base64 = base64.split("base64,").last();
    QByteArray ba = base64.toUtf8();
    QPixmap image;
    image.loadFromData(QByteArray::fromBase64(ba));
    if(!image.isNull()){
        ui->cover->setPixmap(image);
    }

    ui->progressBar->hide();
    ui->option_frame->hide();

    getVideoStream(ytIds,songId);

}


void VideoOption::removeStyle(){
    foreach(ElidedLabel *label, this->findChildren<ElidedLabel *>()){
        label->setStyleSheet("background-color:transparent");
    }
}

void VideoOption::closeEvent(QCloseEvent *event){

    if(ytdlProcess != nullptr){
        ytdlProcess->close();
        ytdlProcess = nullptr;
    }
    QWidget::closeEvent(event);
}


void VideoOption::getVideoStream(QString ytIds,QString songId){

    //TODO
//    if(!checkEngine())
//        return;

    ytdlQueue.append(QStringList()<<ytIds<<songId);

    if(ytdlProcess==nullptr && ytdlQueue.count()>0){
        processYtdlQueue();
    }
}

void VideoOption::processYtdlQueue(){

    if(ytdlQueue.count()>0){

        QString ytIds = QString(ytdlQueue.at(0).at(0).split(",").first());
        QString songId = QString(ytdlQueue.at(0).at(1).split(",").last());

        ytdlQueue.removeAt(0);
        if(ytdlProcess == nullptr){
            ytdlProcess = new QProcess(this);
            ytdlProcess->setObjectName(songId);

            QString url = "https://www.youtube.com/watch?v="+ytIds.split("<br>").first();
            currentUrl = url;

            QString addin_path = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
            ytdlProcess->start("python",QStringList()<<addin_path+"/core"<<"-F"<<url);
            ui->progressBar->show();
            ytdlProcess->waitForStarted();
            connect(ytdlProcess,SIGNAL(finished(int)),this,SLOT(ytdlFinished(int)));
        }
    }

    //update stream info buttons
    if(ytdlProcess==nullptr){
        ui->progressBar->hide();
    }
}

void VideoOption::ytdlFinished(int code){
    QProcess* senderProcess = qobject_cast<QProcess*>(sender());

    if(code==0){
        QString songId = senderProcess->objectName().trimmed();

        QByteArray b;
        b.append(senderProcess->readAll());
        QString s_data = QTextCodec::codecForMib(106)->toUnicode(b).trimmed();

        QStringList formats;
        if(!s_data.isEmpty()){
              formats = s_data.split("resolution note").last().split("\n");
        }

        foreach(QString format,formats){
            if(format.contains("audio only") && !format.isEmpty()){
                 QString code,codec,resolution,size;
                 code = format.split(" ").first();
                 codec = format.split(code+" ").last().trimmed().split(" ").first();
                 resolution = format.split(codec+" ").at(1).trimmed().split(" ").first();
                 size = format.split(",").last().trimmed().split(" ").first();
                 QRadioButton *button = new QRadioButton(resolution+"("+size+")", this);
                 button->setObjectName("audio-"+code);
                 connect(button,SIGNAL(clicked(bool)),this,SLOT(update_audio_video_code(bool)));
                 ui->quality_box_layout_audio->addWidget(button);
            }

            if(format.contains("video only") && !format.isEmpty()){
                 QString code,codec,resolution,size;
                 code = format.split(" ").first();
                 codec = format.split(code+" ").last().trimmed().split(" ").first();
                 resolution = format.split(codec+" ").at(1).trimmed().split(" ").first();
                 size = format.split(",").last().trimmed().split(" ").first();
                 if(!resolution_List.contains(resolution)){
                     QRadioButton *button = new QRadioButton(resolution+"("+size+")", this);
                     button->setObjectName("video-"+code);
                     connect(button,SIGNAL(clicked(bool)),this,SLOT(update_audio_video_code(bool)));
                     ui->quality_box_layout_video->addWidget(button);
                     resolution_List.append(resolution);
                 }
            }
        }
        ui->option_frame->show();
    }

    ytdlProcess->close();
    ytdlProcess = nullptr;

    if(ytdlQueue.count()>0){
        processYtdlQueue();
    }

    //update
    if(ytdlProcess==nullptr){
        ui->progressBar->hide();
    }

    //delete process/task
    senderProcess->close();
    if(senderProcess != nullptr)
    senderProcess->deleteLater();
}

void VideoOption::update_audio_video_code(bool checked){
    QRadioButton *button = qobject_cast<QRadioButton*>(sender());
    if(button->objectName().contains("audio-")&&checked){
        audioCode = button->objectName().split("audio-").last();
    }
    if(button->objectName().contains("video-")&&checked){
        videoCode = button->objectName().split("video-").last();
    }

    if(!audioCode.isEmpty() && !videoCode.isEmpty()){
        ui->watch->disconnect();
        connect(ui->watch,&QPushButton::clicked,[=](){
            getUrlForFormatsAndPLay(audioCode,videoCode);
        });
        ui->watch->setEnabled(true);
    }else{
        ui->watch->setEnabled(false);
    }
}


void VideoOption::getUrlForFormatsAndPLay(QString audioFormat,QString videoFormat){
    QString addin_path = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
    QProcess *audioP = new QProcess(this);
    connect(audioP,SIGNAL(finished(int)),this,SLOT(getUrlProcessFinished(int)));
    audioP->start("python",QStringList()<<addin_path+"/core"<<"-f"<<videoFormat+"+"+audioFormat<<"--get-url"<<currentUrl);
    ui->progressBar->show();
    ui->watch->setText("Merging formats...");
    ui->watch->setEnabled(false);
    audioP->waitForStarted();
}

void VideoOption::getUrlProcessFinished(int code){
    if(code==0){
        QProcess *process = qobject_cast<QProcess*>(sender());
        QString output = process->readAll().trimmed();
        if(output.contains("http")){
            videoUrl = output.split("\n").first();
            audioUrl = output.split("\n").last();
            mergeAndPlay(videoUrl,audioUrl);
        }
        ui->progressBar->hide();

    }else{
     //TODO show error to user
    }
}

void VideoOption::mergeAndPlay(QString videoUrlStr,QString audioUrlStr){
    QProcess *player = new QProcess(this);
    connect(player,SIGNAL(finished(int)),this,SLOT(getUrlProcessFinished(int)));
    player->start("mpv",QStringList()<<"--title=MPV for Olivia"<<"--no-ytdl"<<videoUrlStr<<"--audio-file="+audioUrlStr);
    connect(player,SIGNAL(finished(int)),this,SLOT(playerFinished(int)));
    connect(player,SIGNAL(readyRead()),this,SLOT(playerReadyRead()));
}

void VideoOption::playerFinished(int code){
    Q_UNUSED(code);
    ui->watch->setText("Watch");
    ui->watch->setEnabled(true);
}

void VideoOption::playerReadyRead(){
    ui->watch->setEnabled(false);
    ui->watch->setText("Playing...");
}



