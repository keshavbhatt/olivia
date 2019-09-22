#include "videooption.h"
#include "ui_videooption.h"
#include "elidedlabel.h"
#include "manifest_resolver.h"
#include "settings.h"




VideoOption::VideoOption(QWidget *parent,store *store,QString fifopath):
    QWidget(parent),
    ui(new Ui::VideoOption)
{
    ui->setupUi(this);
    if(store_manager==nullptr){
        store_manager = store;
    }
    used_fifo_file_path = fifopath.split(".fifo").first()+"_videoPlayer"+".fifo";
    playerTimer = new QTimer(nullptr);

    connect(playerTimer,&QTimer::timeout,[=](){
        QProcess *fifo = new QProcess(nullptr);
        connect(fifo, SIGNAL(finished(int)), this, SLOT(deleteProcess(int)) );
        connect(fifo,&QProcess::readyRead,[=](){
            QString out = fifo->readAll();
            if(out.contains("success")){
                out = out.split(",").first().split(":").last();
                if(out=="true"){
                    if(this->windowState()!=Qt::WindowFullScreen){
                        this->setWindowState(Qt::WindowFullScreen);
                    }
                }else{
                    if(this->windowState()==Qt::WindowFullScreen){
                        this->setWindowState(Qt::WindowNoState);
                        this->setCursor(Qt::ArrowCursor);
                     }
                }
            }
        });
        fifo->start("bash",QStringList()<<"-c"<<"echo '{\"command\":[\"get_property\" , \"fullscreen\"]}' | socat - "+ used_fifo_file_path);
    });
}

void VideoOption::toggleFullscreen()
{
   // QProcess *fifo = new QProcess(nullptr);
   // connect(fifo, SIGNAL(finished(int)), this, SLOT(deleteProcess(int)) );
   // fifo->start("bash",QStringList()<<"-c"<< "echo '{\"command\": [\"set_property\" ,\"volume\","+QString::number(volume)+"]}' | socat - "+ used_fifo_file_path);
}


void VideoOption::deleteProcess(int code){
     Q_UNUSED(code);
     QProcess *process = qobject_cast<QProcess*>(sender());
     if(process->state()==QProcess::Running){
         process->terminate();
         process->waitForFinished();
         process->deleteLater();
     }else{
         process->close();
         process->deleteLater();
     }
}

VideoOption::~VideoOption()
{
    delete ui;
}

void VideoOption::setMeta(QString songId){
    resetVars();
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

    currentTrackMeta.clear();
    currentTrackMeta<<songId<<title<<album<<artist<<base64<<ytIds;

    QTextDocument text;
    text.setHtml(title);
    QString plainTitle = text.toPlainText();
    currentTitle = plainTitle;

    QFont font("Ubuntu");
    font.setPixelSize(12);

    ui->titlle->setFont(font);
    ui->titlle->setText(plainTitle);

    ui->artist->setFont(font);
    ui->artist->setText(artist);

    ui->album->setFont(font);
    ui->album->setText(album);

    if(album=="undefined"){
        ui->cover->setMaximumHeight(ui->track_widget->height());
        ui->cover->setMaximumWidth(static_cast<int>(ui->track_widget->height()*1.50));
    }

    base64 = base64.split("base64,").last();
    QByteArray ba = base64.toUtf8();
    QPixmap image;
    image.loadFromData(QByteArray::fromBase64(ba));
    if(!image.isNull() && album=="undefined"){
        ui->cover->setPixmap(image);
        ui->cover->setMaximumSize(178,100);
        ui->cover->resize(178,100);
    }else{
        ui->cover->setPixmap(image);
        ui->cover->setMaximumSize(100,100);
        ui->cover->resize(100,100);
    }

    ui->progressBar->hide();
    ui->option_frame->hide();

    getVideoStream(ytIds,songId);

    //save track to store
    store_manager->saveAlbumArt(albumId,base64);
    store_manager->saveArtist(artistId,artist);
    store_manager->saveAlbum(albumId,album);
    store_manager->saveDominantColor(albumId,dominantColor);
    store_manager->saveytIds(songId,ytIds);
    store_manager->setTrack(QStringList()<<songId<<albumId<<artistId<<title);

}

void VideoOption::resetVars(){
    foreach(QRadioButton *rBtn,ui->quality_box->findChildren<QRadioButton*>()){
        rBtn->deleteLater();
    }
    ui->watch->setEnabled(false);
    ui->download->setEnabled(false);

    audioCode.clear();
    videoCode.clear();
    resolution_List.clear();
    audioUrl.clear();
    videoUrl.clear();
    currentUrl.clear();
    currentTitle.clear();
    ui->cover->setMaximumSize(100,100);
}

void VideoOption::setMetaFromWeb(QVariant data){

    resetVars();
    QStringList trackMetaList = data.toString().split("<==>");
    currentTrackMeta = trackMetaList;
    QString ytIds,title,artist,album,coverUrl,songId,base64,dominantColor,artistId,albumId;



    if(trackMetaList.count()>9){
        songId = trackMetaList.at(0);
        title = trackMetaList.at(1);
        album = trackMetaList.at(2);
        artist = trackMetaList.at(3);
        coverUrl = trackMetaList.at(4);
        ytIds = trackMetaList.at(5);
        base64 = trackMetaList.at(6);
        dominantColor= trackMetaList.at(7);
        artistId= trackMetaList.at(8);
        albumId= trackMetaList.at(9);

    }else{
        //TODO show error
    }

    QTextDocument text;
    text.setHtml(title);
    QString plainTitle = text.toPlainText();
    currentTitle = plainTitle;

    QFont font("Ubuntu");
    font.setPixelSize(12);

    ui->titlle->setFont(font);
    ui->titlle->setText(plainTitle);

    ui->artist->setFont(font);
    ui->artist->setText(artist);

    ui->album->setFont(font);
    ui->album->setText(album);

    if(album=="undefined"){
        ui->cover->setMaximumHeight(ui->track_widget->height());
        ui->cover->setMaximumWidth(static_cast<int>(ui->track_widget->height()*1.50));
    }

    LoadAvatar(coverUrl);

    ui->progressBar->hide();
    ui->option_frame->hide();

    getVideoStream(ytIds,songId);

  // qDebug()<<ytIds<<title<<artist<<album<<coverUrl<<songId<<dominantColor<<artistId<<albumId<<base64;
    //save track to store
    store_manager->saveAlbumArt(albumId,base64);
    store_manager->saveArtist(artistId,artist);
    store_manager->saveAlbum(albumId,album);
    store_manager->saveDominantColor(albumId,dominantColor);
    store_manager->saveytIds(songId,ytIds);
    store_manager->setTrack(QStringList()<<songId<<albumId<<artistId<<title);

}

void VideoOption::LoadAvatar(const QUrl &avatarUrl)
{
   QNetworkAccessManager manager;
   QNetworkDiskCache* diskCache = new QNetworkDiskCache(0);
   QString cache_path = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
   diskCache->setCacheDirectory(cache_path);
   manager.setCache(diskCache);
   QEventLoop loop;
   QNetworkReply *reply = manager.get(QNetworkRequest(avatarUrl));
   QObject::connect(reply, &QNetworkReply::finished, &loop, [&reply, this,&loop](){
    if (reply->error() == QNetworkReply::NoError)
    {
        QByteArray jpegData = reply->readAll();
        QPixmap pixmap;
        pixmap.loadFromData(jpegData);
        if (!pixmap.isNull())
        {
           ui->cover->setPixmap(pixmap);
        }
    }else{
       // cover load error
       //TODO load default cover
    }
    loop.quit();
  });
  loop.exec();
}

void VideoOption::removeStyle(){
    foreach(ElidedLabel *label, this->findChildren<ElidedLabel *>()){
        label->setStyleSheet("background-color:transparent");
    }
    ui->ignore1->setStyleSheet("background-color:transparent");
    ui->ignore2->setStyleSheet("background-color:transparent");
}

void VideoOption::closeEvent(QCloseEvent *event){
    QProcess *player = this->findChild<QProcess*>("player");
    if(player != nullptr && player->state()==QProcess::Running){
        player->terminate();
        player->deleteLater();
        event->ignore();
    }else {
        if(ytdlProcess != nullptr){
            ytdlProcess->close();
            ytdlProcess = nullptr;
        }
        event->accept();
    }
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
        ui->scrollArea->verticalScrollBar()->setValue(0);
        ui->scrollArea_2->verticalScrollBar()->setValue(0);
        ui->option_frame->show();
    }

    if(ytdlProcess!= nullptr){
        ytdlProcess->close();
        ytdlProcess = nullptr;
    }

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
    this->setWindowState(Qt::WindowNoState);
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

        ui->download->disconnect();
        connect(ui->download,&QPushButton::clicked,[=](){
            getUrlForFormatsAndDownload(audioCode,videoCode);
        });

        ui->watch->setEnabled(true);
        ui->download->setEnabled(true);

    }else{
        ui->watch->setEnabled(false);
        ui->download->setEnabled(false);

    }
}


void VideoOption::getUrlForFormatsAndPLay(QString audioFormat,QString videoFormat){
    QString addin_path = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
    QProcess *audioP = new QProcess(this);

    task = "play";
    connect(audioP,SIGNAL(finished(int)),this,SLOT(getUrlProcessFinished(int)));


    audioP->start("python",QStringList()<<addin_path+"/core"<<"-f"<<videoFormat+"+"+audioFormat<<"--get-url"<<currentUrl);
    ui->progressBar->show();
    ui->watch->setText("Merging formats...");
    ui->watch->setEnabled(false);
    ui->download->setEnabled(false);

    audioP->waitForStarted();
}

void VideoOption::getUrlForFormatsAndDownload(QString audioFormat,QString videoFormat){
    ui->progressBar->show();
    task ="download";
    addToDownload(videoFormat,audioFormat);
    ui->download->setEnabled(false);
    ui->progressBar->hide();
}

void VideoOption::getUrlProcessFinished(int code){
    if(code==0){
        QProcess *process = qobject_cast<QProcess*>(sender());
        QString output = process->readAll().trimmed();
        if(output.contains("http")){
            videoUrl = output.split("\n").first();
            audioUrl = output.split("\n").last();
            if(task=="play")
                mergeAndPlay(videoUrl,audioUrl);
            else
                addToDownload(videoUrl,audioUrl);//not being used
        }
        ui->progressBar->hide();
    }else{
//        QMessageBox msgBox;
//        msgBox.setText("ERROR: an error occured while performing this task");
//        msgBox.setIcon(QMessageBox::Information);
//        msgBox.setInformativeText("Error code: "+QString::number(code));
//        msgBox.setStandardButtons(QMessageBox::Ok);
//        msgBox.setDefaultButton(QMessageBox::Ok);
//        msgBox.exec();
    }
}

void VideoOption::addToDownload(QString videoFormat, QString audioFormat){

    QStringList downloadFormats;
    downloadFormats<<videoFormat<<audioFormat;

    if(currentTrackMeta.count()>4){
        emit downloadRequested(currentTrackMeta,downloadFormats);
    }else{
        //TODO show error
    }
}

void VideoOption::mergeAndPlay(QString videoUrlStr,QString audioUrlStr){
    settings *sett = new settings(this);
    int volume = sett->settingsObj.value("volume",100).toInt();
    QProcess *player = new QProcess(this);
    player->setObjectName("player");
    connect(player,SIGNAL(finished(int)),this,SLOT(getUrlProcessFinished(int)));
    player->start("mpv",QStringList()<<"-wid="+QString::number(this->winId())<<"--title=MPV for Olivia - "+
                  currentTitle<<"--no-ytdl"<<videoUrlStr<<"--audio-file="+audioUrlStr<<"--input-ipc-server="+used_fifo_file_path
                  <<"--volume"<<QString::number(volume));
    sett->deleteLater();
    ui->watch->setText("Opening Player...");
    connect(player,SIGNAL(finished(int)),this,SLOT(playerFinished(int)));
    connect(player,SIGNAL(readyRead()),this,SLOT(playerReadyRead()));
}

void VideoOption::playerFinished(int code){
    Q_UNUSED(code);
    ui->watch->setText("Watch");
    ui->watch->setEnabled(true);
    ui->download->setEnabled(true);

    this->setWindowTitle(QApplication::applicationName()+" - Video Option");
    if(this->windowState()==Qt::WindowFullScreen){
        this->setWindowState(Qt::WindowNoState);
    }
    playerTimer->stop();
}

void VideoOption::playerReadyRead(){
    ui->watch->setEnabled(false);
    ui->download->setEnabled(false);

    ui->watch->setText("Playing...");
    this->setWindowTitle("MPV for Olivia - "+currentTitle);
    if(!playerTimer->isActive()){
        playerTimer->start(500);
    }
}


