#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "cookiejar.h"
#include "elidedlabel.h"
#include <QSplitter>
#include <QUrlQuery>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QGraphicsDropShadowEffect>
#include <QMovie>
#include "store.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    init_app(); // #1
    init_webview();// #2
    init_offline_storage();//  #3
    initMediaPlayer(); // #4
    initNetworkManager();// #5
    store_manager = new store(0,"hjkfds");// #6
    ui->debug_widget->hide();
   // ui->state->hide();
    loadPlayerQueue();// #7 loads previous playing track queue
}

//set up app #1
void MainWindow::init_app(){

    QSplitter *split1 = new QSplitter;
    split1->setObjectName("split1");
    split1->addWidget(ui->center_widget);
    split1->addWidget(ui->right_panel);
    split1->setOrientation(Qt::Horizontal);

    ui->horizontalLayout_3->addWidget(split1);

    setWindowIcon(QIcon(":/icons/olivia.png"));
    setWindowTitle(QApplication::applicationName());

    //setting path init
    QString setting_path =  QStandardPaths::writableLocation(QStandardPaths::DataLocation);
    QString myinifilepath(setting_path+"/OliviaSettings.ini");
    settings = new QSettings(myinifilepath,QSettings::IniFormat);
    settings->beginGroup("Settings");
    settings->setValue("AppActivation-Code","A_scNo.2335");


    ElidedLabel *title = new ElidedLabel("-",0);
    ElidedLabel *artist = new ElidedLabel("-",0);
    ElidedLabel *album = new ElidedLabel("-",0);

    title->setObjectName("nowP_title");
    album->setObjectName("nowP_album");
    artist->setObjectName("nowP_artist");

    title->setAlignment(Qt::AlignHCenter);
    album->setAlignment(Qt::AlignHCenter);
    artist->setAlignment(Qt::AlignHCenter);

    ui->title_horizontalLayout->addWidget(title);
    ui->artist_horizontalLayout->addWidget(artist);
    ui->album_horizontalLayout->addWidget(album);

    browse();

}

//set up webview #2
void MainWindow::init_webview(){
    ui->webview->setZoomFactor(0.9);
    connect(ui->webview,SIGNAL(loadFinished(bool)),this,SLOT(webViewLoaded(bool)));

    //websettings---------------------------------------------------------------
    ui->webview->page()->settings()->setAttribute(QWebSettings::LocalContentCanAccessRemoteUrls,true);
    ui->webview->page()->settings()->setAttribute(QWebSettings::LocalContentCanAccessFileUrls,true);
    QString setting_path =  QStandardPaths::writableLocation(QStandardPaths::DataLocation);
    QString cookieJarPath ;
    if(setting_path.split("/").last().isEmpty()){
       cookieJarPath  =  setting_path+"/cookiejar_olivia.dat";
    }else{
       cookieJarPath  =  setting_path+"cookiejar_olivia.dat";
    }
    ui->webview->settings()->setAttribute(QWebSettings::LocalStorageEnabled, true);
    ui->webview->settings()->enablePersistentStorage(setting_path);
    QWebSettings::globalSettings()->setAttribute(QWebSettings::DeveloperExtrasEnabled, true);
    QWebSettings::globalSettings()->setAttribute(QWebSettings::JavascriptEnabled, true);
    QWebSettings::globalSettings()->setAttribute(QWebSettings::PluginsEnabled, true);

    ui->webview->page()->settings()->setMaximumPagesInCache(10);

    QNetworkDiskCache* diskCache = new QNetworkDiskCache(this);
    diskCache->setCacheDirectory(setting_path);
    ui->webview->page()->networkAccessManager()->setCache(diskCache);
    ui->webview->page()->networkAccessManager()->setCookieJar(new CookieJar(cookieJarPath, ui->webview->page()->networkAccessManager()));

}

void MainWindow::init_offline_storage(){
    QString setting_path =  QStandardPaths::writableLocation(QStandardPaths::DataLocation);
    QDir albumArt(setting_path+"/albumArts");
    if(!albumArt.exists()){
        if(albumArt.mkdir(albumArt.path())){
            qDebug()<<"created albumArts dir";
        }
    }
    QDir downloaded(setting_path+"/downloadedTracks");
    if(!downloaded.exists()){
        if(downloaded.mkdir(downloaded.path())){
            qDebug()<<"created downloadedTracks dir";
        }
    }
    QDir database(setting_path+"/storeDatabase");
    if(!database.exists()){
        if(database.mkdir(database.path())){
            qDebug()<<"created storeDatabase dir";
        }
    }

}

void MainWindow::loadPlayerQueue(){ //  #7
    QString setting_path =  QStandardPaths::writableLocation(QStandardPaths::DataLocation);
    foreach (QStringList trackList, store_manager->getPlayerQueue()) {
        QString id,title,artist,album,base64,dominantColor,songId,albumId,artistId,url;
        songId = trackList.at(0);
        title = trackList.at(1);
        albumId = trackList.at(2);
        album = trackList.at(3);
        artistId = trackList.at(4);
        artist = trackList.at(5);
        base64 = trackList.at(6);
        url = trackList.at(7);
        id = trackList.at(8);
        dominantColor = trackList.at(9);

        QWidget *track_widget = new QWidget(ui->right_list);
        track_widget->setObjectName("track-widget-"+id);
        track_ui.setupUi(track_widget);

        QFont font("Ubuntu");
        font.setPixelSize(12);
        setFont(font);

        ElidedLabel *titleLabel = new ElidedLabel(title,0);
        titleLabel->setFont(font);
        titleLabel->setObjectName("title_elided");
        track_ui.verticalLayout_2->addWidget(titleLabel);

        ElidedLabel *artistLabel = new ElidedLabel(artist,0);
        artistLabel->setObjectName("artist_elided");
        artistLabel->setFont(font);
        track_ui.verticalLayout_2->addWidget(artistLabel);

        ElidedLabel *albumLabel = new ElidedLabel(album,0);
        albumLabel->setObjectName("album_elided");
        albumLabel->setFont(font);
        track_ui.verticalLayout_2->addWidget(albumLabel);

        track_ui.id->setText(id);
        track_ui.dominant_color->setText(dominantColor);
        track_ui.songId->setText(songId);
//      track_ui.playing->hide();
        track_ui.playing->setPixmap(QPixmap(":/icons/blank.png").scaled(track_ui.playing->size(),Qt::KeepAspectRatio,Qt::SmoothTransformation));

        track_ui.songId->hide();
        track_ui.dominant_color->hide();
        track_ui.id->hide();
        if(store_manager->isDownloaded(songId)){
            track_ui.url->setText("file://"+setting_path+"/downloadedTracks/"+songId);
            track_ui.offline->setPixmap(QPixmap(":/icons/offline.png").scaled(track_ui.offline->size(),Qt::KeepAspectRatio,Qt::SmoothTransformation));
        }else{
            track_ui.url->setText(url);
        }
        track_ui.url->hide();
        track_ui.option->setObjectName(id+"optionButton");
        connect(track_ui.option,SIGNAL(clicked(bool)),this,SLOT(showTrackOption()));

        base64 = base64.split("base64,").last();
        QByteArray ba = base64.toUtf8();
        QPixmap image;
        image.loadFromData(QByteArray::fromBase64(ba));
        if(!image.isNull()){
            track_ui.cover->setPixmap(image);
        }
        QListWidgetItem* item;
        item = new QListWidgetItem(ui->right_list);

        QGraphicsOpacityEffect *eff = new QGraphicsOpacityEffect(this);

        item->setSizeHint(track_widget->minimumSizeHint());
        ui->right_list->setItemWidget(item, track_widget);
        ui->right_list->itemWidget(item)->setGraphicsEffect(eff);

        // checks if url is expired and updates item with new url which can be streamed .... until keeps the track item disabled.
        if(url.isEmpty() || (store_manager->getExpiry(songId) && track_ui.url->text().contains("https"))){
            track_ui.loading->setPixmap(QPixmap(":/icons/url_issue.png").scaled(track_ui.loading->size(),Qt::KeepAspectRatio,Qt::SmoothTransformation));
            ui->right_list->itemWidget(item)->setEnabled(false);
            if(!track_ui.id->text().isEmpty()){
                getAudioStream(track_ui.id->text().trimmed(),track_ui.songId->text().trimmed());
            }
        }else{
            track_ui.loading->setPixmap(QPixmap(":/icons/blank.png").scaled(track_ui.loading->size(),Qt::KeepAspectRatio,Qt::SmoothTransformation));
        }
        QPropertyAnimation *a = new QPropertyAnimation(eff,"opacity");
        a->setDuration(500);
        a->setStartValue(0);
        a->setEndValue(1);
        a->setEasingCurve(QEasingCurve::InCirc);
        a->start(QPropertyAnimation::DeleteWhenStopped);
        ui->right_list->addItem(item);
    }
}

void MainWindow::keyPressEvent(QKeyEvent *event){
    if( event->key() == Qt::Key_D )
       {
           if(!ui->debug_widget->isVisible())
               ui->debug_widget->show();
           else ui->debug_widget->hide();
       }
}

void MainWindow::initMediaPlayer(){
    ui->volumeSlider->setMinimum(0);
    ui->volumeSlider->setMaximum(100);
    mediaPlayer = new QMediaPlayer(this,QMediaPlayer::StreamPlayback);
    connect(ui->volumeSlider,SIGNAL(valueChanged(int)),mediaPlayer,SLOT(setVolume(int)));
    connect(mediaPlayer,SIGNAL(positionChanged(qint64)),this,SLOT(setPlayerPosition(qint64)));
    connect(ui->secondsAvailable,SIGNAL(valueChanged(int)),ui->buffer_size,SLOT(setValue(int)));

    connect(mediaPlayer, &QMediaPlayer::stateChanged, [=](){
        ui->playerState->setText(getPlayerStateString(mediaPlayer->state()));
        if(!ui->state->isVisible())
            ui->state->setVisible(true);
            ui->state->setText(getPlayerStateString(mediaPlayer->state()));
        ui->play_pause->setIcon(mediaPlayer->state()==QMediaPlayer::PlayingState?QIcon(":/icons/p_pause.png"):QIcon(":/icons/p_play.png"));
        if(mediaPlayer->state()==QMediaPlayer::PlayingState){

            if(!playBackTimer->isActive()&& bufferFull == false)
                playBackTimer->start(400);
        }
     });

    connect(mediaPlayer,&QMediaPlayer::durationChanged,[=](){
            ui->player_duration->setText(QString::number(mediaPlayer->duration()));

            qint64 duration = mediaPlayer->duration();
            int seconds = (duration/1000) % 60;
            int minutes = (duration/60000) % 60;
            int hours = (duration/3600000) % 24;
            QTime time(hours, minutes,seconds);
            ui->duration->setText(time.toString());

            ui->seekSlider->setMaximum(mediaPlayer->duration());
            ui->secondsAvailable->setMaximum(ui->seekSlider->maximum());
            ui->buffer_size->setMaximum(ui->secondsAvailable->maximum());
    });

    connect(mediaPlayer, &QMediaPlayer::mediaStatusChanged, [=](){
        bytesPerSecond = ui->bufferSlider->maximum()/ui->seekSlider->maximum();
        ui->mediaStatus->setText(getMediaStatusString(mediaPlayer->mediaStatus()));
        if(mediaPlayer->mediaStatus()==QMediaPlayer::EndOfMedia||mediaPlayer->mediaStatus()==QMediaPlayer::InvalidMedia){
            ui->seekSlider->setEnabled(false);
        }else{
             ui->seekSlider->setEnabled(true);
        }
    });

    emit ui->volumeSlider->setValue(100);
}

void MainWindow::setPlayerPosition(qint64 position){
    ui->player_position->setText(QString::number(position));

    int seconds = (position/1000) % 60;
    int minutes = (position/60000) % 60;
    int hours = (position/3600000) % 24;
    QTime time(hours, minutes,seconds);
    ui->position->setText(time.toString());

    ui->seekSlider->setValue(position);
}


void MainWindow::tryStopMediaPlayer(){
    if(mediaPlayer->state()==QMediaPlayer::PlayingState){
        mediaPlayer->stop();
    }
}
void MainWindow::tryToPlay(int pos){
        //record buffer size here check it before calling this function in timeout loop
        if(buffer!=nullptr)
            lastBufferSize = buffer->size();
        else
            lastBufferSize = 0;
        userStoppedPlayback = false;
        mediaPlayer->setMedia(QMediaContent(),buffer);
        mediaPlayer->setPosition(pos);
        mediaPlayer->play();
        if(playBackTimer!=nullptr)
        if(!playBackTimer->isActive() && bufferFull == false)
        playBackTimer->start(400);//playback timer timeout
        tried =true;
}

void MainWindow::on_play_pause_clicked()
{
    if(mediaPlayer->state()==QMediaPlayer::PlayingState){
        mediaPlayer->pause();
        userStoppedPlayback = true;
    }else if(mediaPlayer->state()==QMediaPlayer::PausedState){
        mediaPlayer->play();
        userStoppedPlayback = false;
    }
    else if(mediaPlayer->state()==QMediaPlayer::StoppedState){
        tryToPlay(0);
    }
}

void MainWindow::on_volumeSlider_valueChanged(int value)
{
     ui->current_volume->setText(QString::number(value));
}

void MainWindow::on_seekSlider_sliderReleased()
{
    int pos= ui->seekSlider->value(); //new value

    if(mediaPlayer->state()==QMediaPlayer::PlayingState && pos<ui->secondsAvailable->value()){
        tryToPlay(pos); //not seekable
     }else{
        ui->seekSlider->setValue(mediaPlayer->position());
    }

}

void MainWindow::on_stop_clicked()
{
    mediaPlayer->stop();
    playBackTimer->stop();
    userStoppedPlayback = true;
}

void MainWindow::on_seekSlider_sliderMoved(int position)
{
//    getNowPlayingTrackId();
    if(!store_manager->isDownloaded(nowPlayingSongId)){
        if(position>ui->secondsAvailable->value()){
            ui->seekSlider->setSliderPosition(secondsAvailable);
        }
    }else{
           mediaPlayer->setPosition(position);
        }
   // positionChnagedConnected = false;
   // QObject::disconnect(mediaPlayer, &QMediaPlayer::positionChanged, this, nullptr);
}


QString MainWindow::getMediaStatusString(QMediaPlayer::MediaStatus e)
{
    switch (e) {
    case QMediaPlayer::LoadedMedia: return "Loaded Media"; break;
    case QMediaPlayer::NoMedia: return "No Media"; break;
    case QMediaPlayer::LoadingMedia: return "Loading Media"; break;
    case QMediaPlayer::StalledMedia: return "Stalled Media"; break;
    case QMediaPlayer::UnknownMediaStatus: return "Unknown Media Status"; break;
    case QMediaPlayer::BufferedMedia: return "Buffered Media"; break;
    case QMediaPlayer::BufferingMedia: return "Buffering Media"; break;
    case QMediaPlayer::EndOfMedia: return "EndOf Media"; break;
    case QMediaPlayer::InvalidMedia: return "Invalid Media"; break;
    default: return "Unknown Media Status";
        break;
    }

}

QString MainWindow::getPlayerStateString(QMediaPlayer::State e)
{
    switch (e) {
    case QMediaPlayer::StoppedState: return "Stopped State"; break;
    case QMediaPlayer::PlayingState: return "Playing State"; break;
    case QMediaPlayer::PausedState: return "Paused State"; break;
    default: return "Unknown PLayer State";
        break;
    }

}


//NETWORK
void MainWindow::initNetworkManager(){
    nm = new QNetworkAccessManager(this);
    nm->setCache(ui->webview->page()->networkAccessManager()->cache());
    reply = nullptr;
}

void MainWindow::contentSizeMetaChanged(){
    if (content_size->operation() == QNetworkAccessManager::HeadOperation){
            int content_length = content_size->header(QNetworkRequest::ContentLengthHeader).toInt();
            if(content_length>0){
                qDebug()<<"GOT CONTENT LENGTH FROM URL STRING: "<<content_length;
            }
            if(content_length==0){
                QString queries = content_size->request().url().query(QUrl::FullyDecoded);
                content_length = QUrlQuery(queries).queryItemValue("clen").toInt();
                ui->console->append("GOT CONTENT LENGTH FROM URL: "+QString::number(content_length));
                qDebug()<<"GOT CONTENT LENGTH FROM QURLQUERY:"<<content_length;
            }
            if(content_length==0){
               content_length = content_size->request().url().toString().split("/clen/").last().split("/").first().toInt();
               qDebug()<<"GOT CONTENT LENGTH FROM URL STRING WITH /:"<<content_length;
            }
            ui->bufferSlider->setMaximum(content_length);
            ui->total_buffer->setText(QString::number(content_length));
            stream(content_size->request().url());
    }else{
        ui->console->append("yes");
    }
}

void MainWindow::getStreamSize(const QUrl url){
    lastBufferSize=0;
    if(reply!=nullptr){
        reply->abort();
    }
    if(content_size!=nullptr){
        content_size->abort();
    }



    if(buffer!=nullptr){
        qDebug()<<"buffer deleted";
        buffer->close();
        buffer->deleteLater();
    }

    if(byteArray!=nullptr){
        qDebug()<<"bytearray cleared";
        byteArray->clear();
    }

    byteArray = new QByteArray();
    buffer = new QBuffer(byteArray);
    buffer->open(QBuffer::ReadOnly);


    ui->console->clear();
    ui->console->append("Requested to stream : "+url.toString()+"\n");
    ui->loaded_buffer->setText("current");
    ui->bufferSlider->setValue(0);
    content_size = nm->head(QNetworkRequest(url));
    connect(content_size, SIGNAL(metaDataChanged()), this, SLOT(contentSizeMetaChanged()));
}

void MainWindow::stream(const QUrl url){
    QNetworkRequest req(url);
    req.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferCache);
    req.setRawHeader("User-Agent","Mozilla/5.0 (X11; Linux x86_64; rv:65.0) Gecko/20100101 Firefox/65.0");
        reply = nm->get(req);
        connect(reply, SIGNAL(readyRead()), this, SLOT(networkReadyRead()));
        connect(reply, &QNetworkReply::finished,[=](){
            QVariant possibleRedirectUrl = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
            if(possibleRedirectUrl.isValid()){
                qDebug()<<"REDIRECTED NETWORK";
                ui->console->append(QString(possibleRedirectUrl.toString()));
                reply->abort();
                stream(QUrl(possibleRedirectUrl.toString()));
                return;
            }else{
                secondsAvailable = mediaPlayer->duration();
                bufferFull = true;
            }

            QString url = reply->request().url().toString();

            QString format =QUrlQuery(QUrl::fromPercentEncoding(reply->request().url().toString().toUtf8())).queryItemValue("mime").split("/").last();

            if(format.isEmpty()){ //try to get mime type with regexp
                format = url.split("audio%2F").last().split("&").first().trimmed();
                if(format.isEmpty()){
                    format = url.split("audio%2F").last().split("/").first().trimmed();
                }
            }
            if(format.contains("/")){
                format = format.split("/").first();
            }

            if(reply->error()==QNetworkReply::NoError){
                qDebug()<<"FINISHED NETWORK";
                if(!store_manager->isDownloaded(nowPlayingSongId)){
                    saveTrack(format.trimmed());
                }
            }else{
                ui->state->show();
                ui->state->setText(reply->errorString().split(" ").first());
            }
        });
}

void MainWindow::saveTrack(QString format){ //saves track to offline storage

    QString download_Path =  QStandardPaths::writableLocation(QStandardPaths::DataLocation)+"/downloadedTracks/";
    QFile file(download_Path+nowPlayingSongId); //+"."+format
    if (!file.open(QIODevice::WriteOnly))
        return;
           file.write(buffer->data());
         file.close();
      qDebug()<<"saved"<<nowPlayingSongId<<"as"<<format<<"in"<<file.fileName();
      store_manager->update_track("downloaded",nowPlayingSongId,"1");

      //show offline icon in track ui and change url to offline one
      for(int i = 0 ; i< ui->right_list->count();i++){
           if(ui->right_list->itemWidget(ui->right_list->item(i))->findChild<QLineEdit*>("songId")->text()==nowPlayingSongId){
              QLabel *offline = ui->right_list->itemWidget(ui->right_list->item(i))->findChild<QLabel*>("offline");
              ((QLabel*)(offline))->setPixmap(QPixmap(":/icons/offline.png").scaled(track_ui.offline->size(),Qt::KeepAspectRatio,Qt::SmoothTransformation));
              ui->right_list->itemWidget(ui->right_list->item(i))->findChild<QLineEdit*>("url")->setText("file://"+download_Path+nowPlayingSongId);
          }
      }
}

void MainWindow::networkReadyRead(){
    byteArray->append(reply->readAll());
    if(lastBufferSize!=0 && bytesPerSecond !=0 && !bufferFull)
    secondsAvailable = buffer->size()/bytesPerSecond;
    buffer->reset();
    ui->loaded_buffer->setText(QString::number(buffer->size()));
    ui->bufferSlider->setValue(buffer->size());
    ui->console->append("downloaded "+QString::number(buffer->size()) +" of "+ ui->total_buffer->text());
    if(store_manager->isDownloaded(nowPlayingSongId)){
        tryToPlay(0);
    }else{
        if(!tried)
        tryToPlay(0);
    }

}

void MainWindow::on_bufferSlider_valueChanged(int value)
{
    if(value==ui->bufferSlider->maximum()){
        //load full buffer
        if(!userStoppedPlayback){
            tryToPlay(mediaPlayer->position());
            playBackTimer->stop();
            bufferFull = true;
        }
    }
}



void MainWindow::on_cache_slider_sliderMoved(int position)
{
    PLAYER_CACHE_SIZE = position*10000; /*position 1 means 10kb*/
    ui->cacheSize->setText(QString::number(PLAYER_CACHE_SIZE/1000)+"Kb");
}




MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::webViewLoaded(bool loaded){
//    qDebug()<<"page loaded"<<loaded;
    if(loaded)
    ui->webview->page()->mainFrame()->addToJavaScriptWindowObject(QString("mainwindow"),  this);

    if(pageType=="search"){
        if(!ui->search->text().isEmpty() && loaded && !offsetstr.contains("offset")){
            ui->left_list->setCurrentRow(3);
            QString term = ui->search->text();
            term.replace(" ","+");
            search(term);
            isLoadingResults=false;
        }else{
            isLoadingResults = false;
        }
    }
}

void MainWindow::resultLoaded(){
// qDebug()<<"resultLoaded for page"+QString::number(currentResultPage);
  isLoadingResults =false;
}

//returns search query to webend
QString MainWindow::getTerm(){
    QString term = ui->search->text();
    return  term.replace(" ","+");
}

void MainWindow::addToQueue(QString id,QString title,QString artist,QString album,QString base64,QString dominantColor,QString songId,QString albumId,QString artistId){

    QWidget *track_widget = new QWidget(ui->right_list);
    track_widget->setObjectName("track-widget-"+id);
    track_ui.setupUi(track_widget);

    QFont font("Ubuntu");
    font.setPixelSize(12);
    setFont(font);

    ElidedLabel *titleLabel = new ElidedLabel(title,0);
    titleLabel->setFont(font);
    titleLabel->setObjectName("title_elided");
    track_ui.verticalLayout_2->addWidget(titleLabel);

    ElidedLabel *artistLabel = new ElidedLabel(artist,0);
    artistLabel->setObjectName("artist_elided");
    artistLabel->setFont(font);
    track_ui.verticalLayout_2->addWidget(artistLabel);

    ElidedLabel *albumLabel = new ElidedLabel(album,0);
    albumLabel->setObjectName("album_elided");
    albumLabel->setFont(font);
    track_ui.verticalLayout_2->addWidget(albumLabel);

    track_ui.id->setText(id);
    track_ui.dominant_color->setText(dominantColor);
    qDebug()<<"songId:"<<songId;
    track_ui.songId->setText(songId);

   // track_ui.playing->hide();

    track_ui.playing->setPixmap(QPixmap(":/icons/blank.png").scaled(track_ui.playing->size(),Qt::KeepAspectRatio,Qt::SmoothTransformation));


    track_ui.songId->hide();
    track_ui.dominant_color->hide();
    track_ui.id->hide();
    track_ui.url->hide();
    track_ui.option->setObjectName(id+"optionButton");
    connect(track_ui.option,SIGNAL(clicked(bool)),this,SLOT(showTrackOption()));

    base64 = base64.split("base64,").last();
    QByteArray ba = base64.toUtf8();
    QPixmap image;
    image.loadFromData(QByteArray::fromBase64(ba));
    if(!image.isNull()){
        track_ui.cover->setPixmap(image);
    }
    QListWidgetItem* item;
    item = new QListWidgetItem(ui->right_list);

    QGraphicsOpacityEffect *eff = new QGraphicsOpacityEffect(this);

    item->setSizeHint(track_widget->minimumSizeHint());
    ui->right_list->setItemWidget(item, track_widget);
    ui->right_list->itemWidget(item)->setGraphicsEffect(eff);
    ui->right_list->itemWidget(item)->setEnabled(false); //enable when finds a url

    QPropertyAnimation *a = new QPropertyAnimation(eff,"opacity");
    a->setDuration(500);
    a->setStartValue(0);
    a->setEndValue(1);
    a->setEasingCurve(QEasingCurve::InCirc);
    a->start(QPropertyAnimation::DeleteWhenStopped);
    ui->right_list->addItem(item);

    ui->right_list->setCurrentRow(ui->right_list->count()-1);
    getAudioStream(id,songId);


//SAVE DATA TO LOCAL DATABASE

    store_manager->saveAlbumArt(albumId,base64);
    store_manager->saveArtist(artistId,artist);
    store_manager->saveAlbum(albumId,album);
    store_manager->saveDominantColor(albumId,dominantColor);
    store_manager->saveytIds(songId,id);
    store_manager->setTrack(QStringList()<<songId<<albumId<<artistId<<title);
    store_manager->add_to_player_queue(songId);

    ui->right_list->scrollToBottom();

}

void MainWindow::showTrackOption(){
    QPushButton* senderButton = qobject_cast<QPushButton*>(sender());
    qDebug()<<"Clicked"<<senderButton->objectName();
}

void MainWindow::getAudioStream(QString id,QString songId){
    QProcess *ytdl = new QProcess(this);
    ytdl->setObjectName(id+"!==&songId==!"+songId);

    QStringList urls = id.split("<br>");
    QStringList urlsFinal;
    for(int i=0; i < urls.count();i++){
        if(!urls.at(i).isEmpty()){
            urlsFinal.append("https://www.youtube.com/watch?v="+urls.at(i));
        }
    }
    ytdl->start("youtube-dl",QStringList()<<"--get-url" <<"-i"<< "--extract-audio"<<urlsFinal);
    ytdl->waitForStarted();
    connect(ytdl,SIGNAL(readyRead()),this,SLOT(ytdlReadyRead()));
    connect(ytdl,SIGNAL(finished(int,QProcess::ExitStatus)),this,SLOT(youtubeDlFinished(int,QProcess::ExitStatus)));
}

void MainWindow::youtubeDlFinished(int code,QProcess::ExitStatus exitStatus){
    QProcess* senderProcess = qobject_cast<QProcess*>(sender());
    QString processName = senderProcess->objectName();
    Q_UNUSED(exitStatus);
    Q_UNUSED(processName);
    Q_UNUSED(code);
//    qDebug()<<processName<<"Youtubedl Process exit code-"<<code<<exitStatus;
}
void MainWindow::ytdlReadyRead(){

    QProcess* senderProcess = qobject_cast<QProcess*>(sender());
    QString processName = senderProcess->objectName().split("!==&songId==!").first().trimmed();//"!==&songId==!"+songId
    QString songId = senderProcess->objectName().split("!==&songId==!").last().trimmed();

    QByteArray b;
    b.append(senderProcess->readAll());
    QString s_data = QTextCodec::codecForMib(106)->toUnicode(b).trimmed();
//    qDebug()<<s_data;

    if(!s_data.isEmpty()){
            QWidget *listWidget = ui->right_list->findChild<QWidget*>("track-widget-"+processName);
            listWidget->setEnabled(true);
            listWidget->findChild<QLabel*>("loading")->setPixmap(QPixmap(":/icons/blank.png").scaled(track_ui.loading->size(),Qt::KeepAspectRatio,Qt::SmoothTransformation));
            QLineEdit *url = listWidget->findChild<QLineEdit *>("url");
            QString url_str = s_data.trimmed();
            ((QLineEdit*)(url))->setText(url_str);
            QProcess* senderProcess = qobject_cast<QProcess*>(sender()); // retrieve the button clicked
            senderProcess->kill();
            QString expiryTime = QUrlQuery(QUrl::fromPercentEncoding(url_str.toUtf8())).queryItemValue("expire").trimmed();
            store_manager->saveStreamUrl(songId,url_str,expiryTime);
    }
}

void MainWindow::on_left_list_currentRowChanged(int currentRow)
{
    switch (currentRow) {
    case 1:
         browse();
        break;
    case 3:
         search("");
        break;
    case 4:
         show_top();
        break;
    case 15:
         qApp->quit();
        break;
    default:
        break;
    }
}

void MainWindow::on_search_returnPressed()
{
    currentResultPage = 1;
    isLoadingResults=false;
    offsetstr="";
    search("");
}

void MainWindow::browse(){
     pageType="browse";
     ui->left_list->setCurrentRow(1);
     ui->webview->load(QUrl("qrc:///web/browse/browse.html"));
}

void MainWindow::search(QString offset){
     pageType = "search";
    if(offset.isEmpty()){
         ui->webview->load(QUrl("qrc:///web/search/search.html"));
        isLoadingResults=false;
    }else{ //search page loaded perform js
         QString term = ui->search->text();
        term.replace(" ","+");
        if(offset.contains(term)){
           offset = offset.remove(term);
        }
        term.append(offset);
        offsetstr= term;
        ui->webview->page()->mainFrame()->evaluateJavaScript("track_search('"+term+"')");
        isLoadingResults=true;
//        qDebug()<<"term"<<term;
    }
}

void MainWindow::show_top(){
    pageType = "top";
    ui->webview->load(QUrl("qrc:///web/top/top.html"));
}

void MainWindow::on_right_list_itemDoubleClicked(QListWidgetItem *item)
{

    if(!ui->right_list->itemWidget(item)->isEnabled())
        return;

    QString id =  ui->right_list->itemWidget(item)->findChild<QLineEdit*>("id")->text();
    QString url = ui->right_list->itemWidget(item)->findChild<QLineEdit*>("url")->text();
    QString songId = ui->right_list->itemWidget(item)->findChild<QLineEdit*>("songId")->text();

    ui->webview->page()->mainFrame()->evaluateJavaScript("setNowPlaying("+songId+")");

    tryStopMediaPlayer();  //stops mediaplayer if it is playing

    Q_UNUSED(id);


    ElidedLabel *title = ui->right_list->itemWidget(item)->findChild<ElidedLabel *>("title_elided");
    QString titleStr = ((ElidedLabel*)(title))->text();

    ElidedLabel *artist = ui->right_list->itemWidget(item)->findChild<ElidedLabel *>("artist_elided");
    QString artistStr = ((ElidedLabel*)(artist))->text();

    ElidedLabel *album = ui->right_list->itemWidget(item)->findChild<ElidedLabel *>("album_elided");
    QString albumStr = ((ElidedLabel*)(album))->text();

    QString dominant_color = ui->right_list->itemWidget(item)->findChild<QLineEdit*>("dominant_color")->text();

    QLabel *cover = ui->right_list->itemWidget(item)->findChild<QLabel *>("cover");
    ui->cover->setPixmap(QPixmap(*cover->pixmap()).scaled(100,100,Qt::KeepAspectRatio,Qt::SmoothTransformation));

   //hide all playing labels
    QList<QLabel*> playing_label_list_;
    playing_label_list_ = ui->right_list->findChildren<QLabel*>("playing");
    foreach (QLabel *playing, playing_label_list_) {
        playing->setPixmap(QPixmap(":/icons/blank.png").scaled(playing->size(),Qt::KeepAspectRatio,Qt::SmoothTransformation));
        playing->setToolTip("");
    }
    //show now playing on current track
    QLabel *playing = ui->right_list->itemWidget(item)->findChild<QLabel *>("playing");
    playing->setPixmap(QPixmap(":/icons/now_playing.png").scaled(playing->size(),Qt::KeepAspectRatio,Qt::SmoothTransformation));
    playing->setToolTip("playing...");

    getNowPlayingTrackId();

    if(!ui->state->isVisible())
        ui->state->show();
    ui->state->setText("Connecting...");
    ui->secondsAvailable->setValue(0);

    QString r,g,b;
    QStringList colors = dominant_color.split(",");
    if(colors.count()==3){
        r=colors.at(0);
        g=colors.at(1);
        b=colors.at(2);
    }else{
        r="98";
        g="164";
        b="205";
    }

    ui->nowplaying_widget->setImage(*cover->pixmap());
    ui->nowplaying_widget->setColor(QColor(r.toInt(),g.toInt(),b.toInt()));
    //change the color of main window according to album cover
    this->setStyleSheet(this->styleSheet()+"QMainWindow{"
                            "background-color:rgba("+r+","+g+","+b+","+"0.1"+");"
                            "}");
    QString rgba = r+","+g+","+b+","+"0.2";

    ui->webview->page()->mainFrame()->evaluateJavaScript("changeBg('"+rgba+"')");
    QString widgetStyle= "background-color:"
                         "qlineargradient(spread:pad, x1:0, y1:0, x2:0, y2:1,"
                         "stop:0.129213 rgba("+r+", "+g+", "+b+", 30),"
                         "stop:0.38764 rgba("+r+", "+g+", "+b+", 120),"
                         "stop:0.679775 rgba("+r+", "+g+", "+b+", 84),"
                         "stop:1 rgba("+r+", "+g+", "+b+", 30));";
    ui->left_panel->setStyleSheet("QWidget#left_panel{"+widgetStyle+"}");
    ui->right_panel->setStyleSheet("QWidget#right_panel{"+widgetStyle+"}");
//    QString sliderStyle = ;


    ElidedLabel *title2 = this->findChild<ElidedLabel *>("nowP_title");
    ((ElidedLabel*)(title2))->setText(titleStr);

    ElidedLabel *artist2 = this->findChild<ElidedLabel *>("nowP_artist");
    ((ElidedLabel*)(artist2))->setText(artistStr);


    ElidedLabel *album2 = this->findChild<ElidedLabel *>("nowP_album");
    ((ElidedLabel*)(album2))->setText(albumStr);


    QList<ElidedLabel*> label_list_;
    QList<QGraphicsDropShadowEffect*> shadow_list_;

    // Get all UI labels and apply shadows
    label_list_ = ui->nowplaying_widget->findChildren<ElidedLabel*>();
    foreach(ElidedLabel *lbl, label_list_) {
        shadow_list_.append(new QGraphicsDropShadowEffect);
        shadow_list_.back()->setBlurRadius(5);
        shadow_list_.back()->setOffset(1, 1);
        shadow_list_.back()->setColor(QColor("#292929"));
        lbl->setGraphicsEffect(shadow_list_.back());
    }

    getStreamSize(QUrl(url));
    tried =false;

        userStoppedPlayback = false;
        bufferFull = false;
        lastBufferSize = 0;
        bytesPerSecond = 0;
        secondsAvailable = 0;


//    // exit if already running
       if(playBackTimer!= nullptr)
          return;
       playBackTimer = new QTimer(this);
       connect(playBackTimer, &QTimer::timeout, [=](){
//                qDebug()<<"Playing"<<"lastbuffersize:"<<lastBufferSize<<"bytespersecond:"<<bytesPerSecond<<"secondsavailable"<<secondsAvailable;

                if(secondsAvailable>0 && !bufferFull){
                    ui->secondsAvailable->setValue(secondsAvailable);
                    ui->secondsLabel->setText(QString::number(secondsAvailable));
                }

                if((mediaPlayer->position()<mediaPlayer->duration()|| mediaPlayer->duration()==-1)&& !userStoppedPlayback  ){
                    if(mediaPlayer->mediaStatus()==QMediaPlayer::EndOfMedia||mediaPlayer->mediaStatus()==QMediaPlayer::InvalidMedia){ //loads when buffer reached end
                        //get pos, load buffer, set pos, play buffer

                        if(bufferFull == false){
                            if(buffer->size()>lastBufferSize+PLAYER_CACHE_SIZE){
                                qDebug()<<"try to play";
                                tryToPlay(mediaPlayer->position());
                            }else{
                                if(!ui->state->isVisible())
                                    ui->state->show();
                                ui->state->setText("Buffering...");
                                ui->console->append("WATING FOR CACHE BUFFER TO REACH-"+QString::number(PLAYER_CACHE_SIZE+lastBufferSize-buffer->size())+" BYTES");
                            }
                        }else{
                            tryToPlay(mediaPlayer->position());
                            playBackTimer->stop();
                        }
                    }else if(mediaPlayer->state()==QMediaPlayer::PausedState){ //load media even if plalyer is paused
                        mediaPlayer->setMedia(QMediaContent(),buffer);
                        mediaPlayer->setPosition(mediaPlayer->position());
                        ui->console->append("BUFFER LOADED IN PAUSED STATE");
                    }
                }
       });
}

//app menu to hide show sidebar
void MainWindow::on_menu_clicked()
{
    if(ui->left_panel->maximumWidth()==0){
        QPropertyAnimation *animation = new QPropertyAnimation(ui->left_panel, "maximumWidth");
        animation->setDuration(500);
        animation->setStartValue(0);
        animation->setEndValue(500);
        animation->setEasingCurve(QEasingCurve::InQuart);
        animation->start();
        ui->menu->setIcon(QIcon(":/icons/close.png"));
    }else{
        QPropertyAnimation *animation = new QPropertyAnimation(ui->left_panel, "maximumWidth");
        animation->setDuration(500);
        animation->setStartValue(500);
        animation->setEndValue(0);
        animation->setEasingCurve(QEasingCurve::OutQuart);
        animation->start();
        ui->menu->setIcon(QIcon(":/icons/menu.png"));
    }
}

void MainWindow::on_right_list_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous){
//    qDebug()<<current<<previous;
}

void MainWindow::getNowPlayingTrackId(){
    for(int i = 0 ; i< ui->right_list->count();i++){
         if(ui->right_list->itemWidget(ui->right_list->item(i))->findChild<QLabel*>("playing")->toolTip()=="playing..."){
            //get songId of visible track
            QLineEdit *songId = ui->right_list->itemWidget(ui->right_list->item(i))->findChild<QLineEdit*>("songId");
            nowPlayingSongId = ((QLineEdit*)(songId))->text();
        }
    }
}

void MainWindow::resizeEvent(QResizeEvent *resizeEvent){
    QMainWindow::resizeEvent(resizeEvent);
    ui->right_list->resize(ui->right_list->size().width()+1,ui->right_list->size().height()+1);
    ui->right_list->resize(ui->right_list->size().width()-1,ui->right_list->size().height()-1);
}



