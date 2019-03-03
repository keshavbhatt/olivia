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
#include <QCompleter>
#include <QAction>

#include "store.h"
#include "radio.h"
#include "onlinesearchsuggestion.h"
#include "seekslider.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    qApp->setQuitOnLastWindowClosed(true);
    init_app(); // #1
    init_webview();// #2
    init_offline_storage();//  #3

    init_settings();
    checkEngine();

    //sets search icon in label
    ui->label_5->setPixmap(QPixmap(":/icons/sidebar/search.png").scaled(18,18,Qt::KeepAspectRatio,Qt::SmoothTransformation));

    store_manager = new store(this,"hjkfdsll");// #6
    ui->debug_widget->hide();

    loadPlayerQueue();// #7 loads previous playing track queue
    connect(qApp,SIGNAL(aboutToQuit()),this,SLOT(quitApp()));
    init_search_autoComplete();

    ui->radioVolumeSlider->setMinimum(0);
    ui->radioVolumeSlider->setMaximum(130);

    ui->radioVolumeSlider->setValue(100);
    saveTracksAfterBuffer = true;
    radio_manager = new radio(this,ui->radioVolumeSlider->value(),saveTracksAfterBuffer);

    connect(radio_manager,SIGNAL(radioStatus(QString)),this,SLOT(radioStatus(QString)));
    connect(radio_manager,SIGNAL(radioPosition(int)),this,SLOT(radioPosition(int)));
    connect(radio_manager,SIGNAL(radioDuration(int)),this,SLOT(radioDuration(int)));
//    connect(radio_manager,SIGNAL(radioEOF(QString)),this,SLOT(radioEOF(QString)));
    connect(radio_manager,SIGNAL(demuxer_cache_duration_changed(double,double)),this,SLOT(radio_demuxer_cache_duration_changed(double,double)));
    connect(radio_manager,SIGNAL(saveTrack(QString)),this,SLOT(saveTrack(QString)));

   // QTimer::singleShot(1000,this,SLOT(evoke_engine_check()));

}

void MainWindow::init_settings(){
    settingsWidget = new QWidget(0);
    settingsUi.setupUi(settingsWidget);
    connect(settingsUi.download_engine,SIGNAL(clicked()),this,SLOT(download_engine_clicked()));
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

//    browse();

}

//set up webview #2
void MainWindow::init_webview(){
//    ui->webview->setZoomFactor(0.9);
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
    QWebSettings::globalSettings()->setAttribute(QWebSettings::PluginsEnabled, false);

    ui->webview->page()->settings()->setMaximumPagesInCache(0);
    ui->webview->page()->settings()->setAttribute(QWebSettings::PluginsEnabled, false);

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
    QDir downloadedTemp(setting_path+"/downloadedTemp");
    if(!downloadedTemp.exists()){
        if(downloadedTemp.mkdir(downloadedTemp.path())){
            qDebug()<<"created downloadedTemp dir";
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
        track_widget->setObjectName("track-widget-"+songId);
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
        track_ui.option->setObjectName(songId+"optionButton");
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
        if(url.isEmpty() || (store_manager->getExpiry(songId) && track_ui.url->text().contains("http"))){
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

bool MainWindow::eventFilter(QObject *obj, QEvent *event){
    if (obj == ui->search &&event->type() == QEvent::KeyPress) {
             QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
                Q_UNUSED(keyEvent);
             return QObject::eventFilter(obj, event);
         } else {
             // standard event processing
             return QObject::eventFilter(obj, event);
         }
}



void MainWindow::init_search_autoComplete(){
    ui->search->installEventFilter(this);
    _onlineSearchSuggestion_ = new onlineSearchSuggestion(ui->search);
}


void MainWindow::setPlayerPosition(qint64 position){
    ui->player_position->setText(QString::number(position));

    int seconds = (position/1000) % 60;
    int minutes = (position/60000) % 60;
    int hours = (position/3600000) % 24;
    QTime time(hours, minutes,seconds);
    ui->position->setText(time.toString());

    ui->radioSeekSlider->setValue(position);
}


void MainWindow::on_play_pause_clicked()
{
    qDebug()<<radio_manager->radioState;
    if(radio_manager->radioState=="paused"){
        radio_manager->resumeRadio();
    }else if(radio_manager->radioState=="playing"){
        radio_manager->pauseRadio();
    }
}

void MainWindow::on_radioVolumeSlider_valueChanged(int value)
{
    if(radio_manager){
         radio_manager->changeVolume(value);
    }
    ui->current_volume->setText(QString::number(value));
}

void MainWindow::on_radioSeekSlider_sliderReleased()
{
//    int pos= ui->radioSeekSlider->value(); //new value
//    radio_manager->radioSeek(pos);
}


void MainWindow::on_radioSeekSlider_sliderMoved(int position)
{
    ui->radioSeekSlider->setSliderPosition(position);
    radio_manager->radioSeek(position);
}


void MainWindow::on_stop_clicked()
{
    //radio stop
}


//NETWORK
void MainWindow::quitApp(){
    qDebug()<<"called";
    radio_manager->killRadioProcess();
    radio_manager->deleteProcess(0);
    radio_manager->deleteLater();
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::webViewLoaded(bool loaded){

    if(loaded){
        ui->webview->page()->mainFrame()->addToJavaScriptWindowObject(QString("mainwindow"),  this);
        ui->webview->page()->mainFrame()->evaluateJavaScript("changeBg('"+themeColor+"')");
        ui->webview->page()->mainFrame()->evaluateJavaScript("NowPlayingTrackId='"+nowPlayingSongId+"'");

        ui->webview->page()->mainFrame()->evaluateJavaScript("setNowPlaying('"+nowPlayingSongId+"')");
        QWebSettings::globalSettings()->clearMemoryCaches();
        ui->webview->history()->clear();
    }
    if(pageType=="saved_songs"){
        ui->webview->page()->mainFrame()->addToJavaScriptWindowObject(QString("store"), store_manager);
        ui->webview->page()->mainFrame()->evaluateJavaScript(" open_saved_tracks();");
    }
    if(pageType=="local_saved_songs"){
        ui->webview->page()->mainFrame()->addToJavaScriptWindowObject(QString("store"), store_manager);
        ui->webview->page()->mainFrame()->evaluateJavaScript(" open_local_saved_tracks();");
    }
    if(pageType=="saved_albums"){
        ui->webview->page()->mainFrame()->addToJavaScriptWindowObject(QString("store"), store_manager);
        ui->webview->page()->mainFrame()->evaluateJavaScript("open_saved_albums();");
    }
    if(pageType=="saved_artists"){
        ui->webview->page()->mainFrame()->addToJavaScriptWindowObject(QString("store"), store_manager);
        ui->webview->page()->mainFrame()->evaluateJavaScript("open_saved_artists();");
    }
    if(pageType=="radio"){
        ui->webview->page()->mainFrame()->addToJavaScriptWindowObject(QString("mainwindow"), this);
    }

    if(pageType=="goto_album"){
        ui->webview->page()->mainFrame()->addToJavaScriptWindowObject(QString("mainwindow"), this);
        ui->webview->page()->mainFrame()->evaluateJavaScript("album_view('"+gotoAlbumId+"')");
    }

    if(pageType=="goto_artist"){
        ui->webview->page()->mainFrame()->addToJavaScriptWindowObject(QString("mainwindow"), this);
        ui->webview->page()->mainFrame()->evaluateJavaScript("artist_view('"+gotoArtistId+"')");
    }
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

    QString setting_path =  QStandardPaths::writableLocation(QStandardPaths::DataLocation);

    if(store_manager->isInQueue(songId)){
        ui->console->append("Song - "+songId+" Already in queue");
        return;
    }else{
        QWidget *track_widget = new QWidget(ui->right_list);
        track_widget->setObjectName("track-widget-"+songId);
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
//        qDebug()<<"songId:"<<songId;
        track_ui.songId->setText(songId);
        track_ui.playing->setPixmap(QPixmap(":/icons/blank.png").scaled(track_ui.playing->size(),Qt::KeepAspectRatio,Qt::SmoothTransformation));

        track_ui.songId->hide();
        track_ui.dominant_color->hide();
        track_ui.id->hide();
        track_ui.url->hide();
        track_ui.option->setObjectName(songId+"optionButton");
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
        if(store_manager->isDownloaded(songId)){
            ui->right_list->itemWidget(item)->setEnabled(true);
            track_ui.url->setText("file://"+setting_path+"/downloadedTracks/"+songId);
            track_ui.offline->setPixmap(QPixmap(":/icons/offline.png").scaled(track_ui.offline->size(),Qt::KeepAspectRatio,Qt::SmoothTransformation));
        }else{
          getAudioStream(id,songId);
        }

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
}

void MainWindow::showTrackOption(){
    QPushButton* senderButton = qobject_cast<QPushButton*>(sender());
    QString songId = senderButton->objectName().remove("optionButton").trimmed();


    QAction *gotoArtist= new QAction("Go to Artist",0);
    QAction *gotoAlbum = new QAction("Go to Album",0);
    QAction *sepe = new QAction("",0);
    sepe->setSeparator(true);
    QAction *sepe2 = new QAction("",0);
    sepe2->setSeparator(true);
    QAction *removeSong = new QAction("Remove from queue",0);
//    QAction *deleteSong = new QAction("Remove from collection && queue",0);
    QAction *deleteSongCache = new QAction("Delete song cache",0);
    deleteSongCache->setEnabled(store_manager->isDownloaded(songId));
//    deleteSong->setEnabled(store_manager->isInCollection(songId));

    QString albumId = store_manager->getAlbumId(songId);
    QString artistId = store_manager->getArtistId(songId);


    connect(gotoAlbum,&QAction::triggered,[=](){
            qDebug()<<"goto Album :"<<albumId;
            ui->webview->load(QUrl("qrc:///web/goto/album.html"));
            pageType = "goto_album";
            gotoAlbumId = albumId;
    });

    connect(gotoArtist,&QAction::triggered,[=](){
            qDebug()<<"goto Artist :"<<artistId;
            ui->webview->load(QUrl("qrc:///web/goto/artist.html"));
            pageType = "goto_artist";
            gotoArtistId = artistId;
    });


    connect(deleteSongCache,&QAction::triggered,[=](){
        QString setting_path =  QStandardPaths::writableLocation(QStandardPaths::DataLocation);
            qDebug()<<"deleted Song cache :"<<songId;
            QFile cache(setting_path+"/downloadedTracks/"+songId);
            cache.remove();
            store_manager->update_track("downloaded",songId,"0");
            for (int i= 0;i<ui->right_list->count();i++) {
               QString songIdFromWidget = ((QLineEdit*) ui->right_list->itemWidget(ui->right_list->item(i))->findChild<QLineEdit*>("songId"))->text().trimmed();
                if(songId==songIdFromWidget){
                    ui->right_list->itemWidget(ui->right_list->item(i))->findChild<QLabel*>("offline")->setPixmap(QPixmap(":/icons/blank.png"));
                    ui->right_list->itemWidget(ui->right_list->item(i))->findChild<QLineEdit*>("url")->setText(store_manager->getOfflineUrl(songId));
                    break;
                }
            }
    });

//    connect(deleteSong,&QAction::triggered,[=](){
//            qDebug()<<"deleted Song from collection :"<<songId;
//            store_manager->removeFromCollection(songId);
//            for (int i= 0;i<ui->right_list->count();i++) {
//               QString songIdFromWidget = ((QLineEdit*) ui->right_list->itemWidget(ui->right_list->item(i))->findChild<QLineEdit*>("songId"))->text().trimmed();
//                if(songId==songIdFromWidget){
//                    ui->right_list->takeItem(i);
//                    store_manager->removeFromQueue(songId);
//                    break;
//                }
//            }
//    });

    connect(removeSong,&QAction::triggered,[=](){
            qDebug()<<"removed Song :"<<songId;
            for (int i= 0;i<ui->right_list->count();i++) {
               QString songIdFromWidget = ((QLineEdit*) ui->right_list->itemWidget(ui->right_list->item(i))->findChild<QLineEdit*>("songId"))->text().trimmed();
                if(songId==songIdFromWidget){
                    ui->right_list->takeItem(i);
                    store_manager->removeFromQueue(songId);
                    break;
                }
            }
    });

    QMenu menu;
    menu.addAction(gotoAlbum);
    menu.addAction(gotoArtist);
    menu.addAction(sepe);
    menu.addAction(removeSong);
//    menu.addAction(deleteSong);
    menu.addAction(sepe2);
    menu.addAction(deleteSongCache);

    menu.exec(QCursor::pos());
}


void MainWindow::getAudioStream(QString ytIds,QString songId){

    QTimer::singleShot(1000, [this]() {
        if(!checkEngine()){
            evoke_engine_check();
            return;
        }
    });


    ytdlQueue.append(QStringList()<<ytIds<<songId);

    if(ytdlProcess==nullptr){
        processYtdlQueue();
    }

}

void MainWindow::processYtdlQueue(){

    if(ytdlQueue.count()>0){
        QString ytIds = QString(ytdlQueue.at(0).at(0).split(",").first());
        QString songId = QString(ytdlQueue.at(0).at(1).split(",").last());

        qDebug()<<songId<<ytIds<<"....processing";

        ytdlQueue.takeAt(0);

        qDebug()<<"process queue called";

        if(ytdlProcess == nullptr){

            qDebug()<<"ENtered loop";
                ytdlProcess = new QProcess(this);
                ytdlProcess->setObjectName(songId);

                QStringList urls = ytIds.split("<br>");
                QStringList urlsFinal;
                for(int i=0; i < urls.count();i++){
                    if(!urls.at(i).isEmpty()){
                        urlsFinal.append("https://www.youtube.com/watch?v="+urls.at(i));
                    }
                }
                QString addin_path = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
                ytdlProcess->start("python",QStringList()<<addin_path+"/core"<<"--get-url" <<"-i"<< "--extract-audio"<<urlsFinal);
                ytdlProcess->waitForStarted();
                connect(ytdlProcess,SIGNAL(readyRead()),this,SLOT(ytdlReadyRead()));
                connect(ytdlProcess,SIGNAL(finished(int)),this,SLOT(ytdlFinished(int)));
        }
    }



}

void MainWindow::ytdlFinished(int code){
//    Q_UNUSED(code);
    ytdlProcess = nullptr;
    qDebug()<<"Process Finishned"<<code;
    if(ytdlQueue.count()>0){
        processYtdlQueue();
        qDebug()<<"YoutubedlQueueSize:"<<ytdlQueue.count();
    }
}

void MainWindow::ytdlReadyRead(){

    QProcess* senderProcess = qobject_cast<QProcess*>(sender());
    QString songId = senderProcess->objectName().trimmed();

    QByteArray b;
    b.append(senderProcess->readAll());
    QString s_data = QTextCodec::codecForMib(106)->toUnicode(b).trimmed();
    qDebug()<<s_data;

    if(!s_data.isEmpty()){
            QWidget *listWidget = ui->right_list->findChild<QWidget*>("track-widget-"+songId);
            listWidget->setEnabled(true);
            listWidget->findChild<QLabel*>("loading")->setPixmap(QPixmap(":/icons/blank.png").scaled(track_ui.loading->size(),Qt::KeepAspectRatio,Qt::SmoothTransformation));
            QLineEdit *url = listWidget->findChild<QLineEdit *>("url");
            QString url_str = s_data.trimmed();
            ((QLineEdit*)(url))->setText(url_str);
            QProcess* senderProcess = qobject_cast<QProcess*>(sender()); // retrieve the button clicked
            senderProcess->finished(0);
            QString expiryTime = QUrlQuery(QUrl::fromPercentEncoding(url_str.toUtf8())).queryItemValue("expire").trimmed();
            if(expiryTime.isEmpty()){
                expiryTime = url_str.split("/expire/").last().split("/").first().trimmed();
            }
            store_manager->saveStreamUrl(songId,url_str,expiryTime);
    }
}

void MainWindow::on_left_list_currentRowChanged(int currentRow)
{
    switch (currentRow) {
    case 1:
        // browse();
        break;
    case 3:
         search("");
        break;
    case 4:
         show_top();
        break;
    case 6:
         show_saved_songs();
        break;
    case 7:
         show_saved_albums();
        break;
    case 8:
         show_saved_artists();
        break;
    case 9:
         show_local_saved_songs();
        break;
    case 14:
         internet_radio();
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
     }
}

void MainWindow::show_top(){
    pageType = "top";
    ui->webview->load(QUrl("qrc:///web/top/top.html"));
}

void MainWindow::show_saved_songs(){
    pageType = "saved_songs";
    ui->webview->load(QUrl("qrc:///web/songs/songs.html"));
}

void MainWindow::show_local_saved_songs(){
    pageType = "local_saved_songs";
    ui->webview->load(QUrl("qrc:///web/local_songs/local_songs.html"));
}

void MainWindow::show_saved_albums(){
    pageType = "saved_albums";
    ui->webview->load(QUrl("qrc:///web/album/albums.html"));
}

void MainWindow::show_saved_artists(){
    pageType = "saved_artists";
    ui->webview->load(QUrl("qrc:///web/artist/artists.html"));
}

void MainWindow::internet_radio(){
    pageType = "radio";
    ui->webview->load(QUrl("qrc:///web/radio/radio.html"));
}

void MainWindow::on_right_list_itemDoubleClicked(QListWidgetItem *item)
{
    if(!ui->right_list->itemWidget(item)->isEnabled())
        return;
    QString id =  ui->right_list->itemWidget(item)->findChild<QLineEdit*>("id")->text();
    QString url = ui->right_list->itemWidget(item)->findChild<QLineEdit*>("url")->text();
    QString songId = ui->right_list->itemWidget(item)->findChild<QLineEdit*>("songId")->text();

    ui->webview->page()->mainFrame()->evaluateJavaScript("setNowPlaying("+songId+")");

   // radio_manager->quitRadio();

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
    qDebug()<<"THEME: "<<r<<g<<b;
    foreach (const QString com, colors) {
        if(com.toInt()>40){
            break;
        }else{
            r="69";
            g="69";
            b="69";
        }
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
    if(store_manager->isDownloaded(songId)){
        radio_manager->playRadio(false,QUrl(url));
    }else{
        //TODO get saveTracksAfterBuffer value from settings before calling this
        saveTracksAfterBuffer = true;
        radio_manager->playRadio(saveTracksAfterBuffer,QUrl(url));
    }
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


void MainWindow::on_settings_clicked()
{
    if(!settingsWidget->isVisible())
    {
        settingsWidget->setWindowFlags(Qt::Popup);
        settingsWidget->move(ui->settings->mapToGlobal(QPoint(QPoint(-settingsWidget->width()+ui->settings->width(),30))));
        settingsWidget->showNormal();
    }
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

//to fix resize bug in desabled tracks in player queue list
void MainWindow::resizeEvent(QResizeEvent *resizeEvent){
    QMainWindow::resizeEvent(resizeEvent);
    ui->right_list->resize(ui->right_list->size().width()+1,ui->right_list->size().height()+1);
    ui->right_list->resize(ui->right_list->size().width()-1,ui->right_list->size().height()-1);
}

void MainWindow::showAjaxError(){
    qDebug()<<"NETWORK ERROR";
}

//returns theme color from common.js
void MainWindow::setThemeColor(QString color){
    themeColor = color;
}


////////////////////////////////////////////////////////////RADIO///////////////////////////////////////////////////////

void MainWindow::radioStatus(QString radioState){
    if(radioState=="playing"){
        ui->play_pause->setIcon(QIcon(":/icons/p_pause.png"));
    }else if(radioState=="paused"||radioState=="stopped"){
        ui->play_pause->setIcon(QIcon(":/icons/p_play.png"));
    }else if(radioState=="stopped"){
        for (int i= 0;i<ui->right_list->count();i++) {
          ui->right_list->itemWidget(ui->right_list->item(i))->findChild<QLabel*>("playing")->setPixmap(QPixmap(":/icons/blank.png"));
        }
    }
    ui->state->setText(radioState);
}

void MainWindow::radioPosition(int pos){
   int seconds = (pos) % 60;
   int minutes = (pos/60) % 60;
   int hours = (pos/3600) % 24;
   QTime time(hours, minutes,seconds);
   ui->position->setText(time.toString());
   ui->radioSeekSlider->setValue(pos);
}


void MainWindow::radioDuration(int dur){
    int seconds = (dur) % 60;
    int minutes = (dur/60) % 60;
    int hours = (dur/3600) % 24;
    QTime time(hours, minutes,seconds);
    ui->duration->setText(time.toString());
    ui->radioSeekSlider->setMaximum(dur);
}

void MainWindow::radio_demuxer_cache_duration_changed(double seconds_available,double radio_playerPosition){
    if(ui->radioSeekSlider->maximum()!=0 && seconds_available != 0 && radio_playerPosition!=0){
        double totalSeconds = seconds_available+radio_playerPosition;
        double width =  (double)totalSeconds / (double)ui->radioSeekSlider->maximum() ;
        ui->radioSeekSlider->subControlWidth = (double)width*100;
    }
}

//void MainWindow::radioEOF(QString value){
//   if(value=="false"){
//   }
//}

void MainWindow::playRadioFromWeb(QVariant streamDetails){
    QString url,title,country,language;
    QStringList list = streamDetails.toString().split(",");
    url = list.at(0);
    title = list.at(1);
    country = list.at(2);
    language = list.at(3);

    this->findChild<ElidedLabel *>("nowP_title")->setText(title);

    this->findChild<ElidedLabel *>("nowP_artist")->setText(language);

    this->findChild<ElidedLabel *>("nowP_album")->setText(country);

    ui->cover->setPixmap(QPixmap(":/web/radio/station_cover.jpg"));

    saveTracksAfterBuffer=false;
    radio_manager->playRadio(saveTracksAfterBuffer,QUrl(url.trimmed()));

    for (int i= 0;i<ui->right_list->count();i++) {
      ui->right_list->itemWidget(ui->right_list->item(i))->findChild<QLabel*>("playing")->setPixmap(QPixmap(":/icons/blank.png"));
    }
}



//TODO
void MainWindow::saveTrack(QString format){
    QString download_Path =  QStandardPaths::writableLocation(QStandardPaths::DataLocation)+"/downloadedTracks/";

    QString downloadTemp_Path =  QStandardPaths::writableLocation(QStandardPaths::DataLocation)+"/downloadedTemp/";
    QFile file(downloadTemp_Path+"current.temp"); //+"."+format

    if(file.copy(download_Path+nowPlayingSongId)){
        file.close();
        qDebug()<<"saved"<<nowPlayingSongId<<"as"<<format<<"in"<<file.fileName();
        store_manager->update_track("downloaded",nowPlayingSongId,"1");
    }
      //show offline icon in track ui and change url to offline one
      for(int i = 0 ; i< ui->right_list->count();i++){
           if(ui->right_list->itemWidget(ui->right_list->item(i))->findChild<QLineEdit*>("songId")->text()==nowPlayingSongId){
              QLabel *offline = ui->right_list->itemWidget(ui->right_list->item(i))->findChild<QLabel*>("offline");
              ((QLabel*)(offline))->setPixmap(QPixmap(":/icons/offline.png").scaled(track_ui.offline->size(),Qt::KeepAspectRatio,Qt::SmoothTransformation));
              ui->right_list->itemWidget(ui->right_list->item(i))->findChild<QLineEdit*>("url")->setText("file://"+download_Path+nowPlayingSongId);
          }
      }
}
////////////////////////////////////////////////////////////END RADIO///////////////////////////////////////////////////////



//ENGINE STUFF

bool MainWindow::checkEngine(){
    QString setting_path = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
    QFileInfo checkFile(setting_path+"/core");
    bool present = false;
    if(checkFile.exists()&&checkFile.size()>0){
        settingsUi.engine_status->setText("Present");
        present = true;
    }else{
        settingsUi.engine_status->setText("Absent");
        present = false;
    }
    return present;
}

void MainWindow::download_engine_clicked()
{
    settingsUi.download_engine->setEnabled(false);
    settingsUi.engine_status->setText("Downloading core(1.4mb octet-stream)");
    QMovie *movie=new QMovie(":/icons/others/load.gif");
    settingsUi.loading_movie->setVisible(true);
    movie->start();
    settingsUi.loading_movie->setMovie(movie);
    QString addin_path = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
    QDir dir(addin_path);
    if (!dir.exists())
    dir.mkpath(addin_path);

    QString filename = "core";
    core_file =  new QFile(addin_path+"/"+filename ); //addin_path
    if(!core_file->open(QIODevice::ReadWrite | QIODevice::Truncate)){
        throw std::runtime_error("Could not open a file to write.");
    }

    QNetworkAccessManager *m_netwManager = new QNetworkAccessManager(this);
    QObject::connect(m_netwManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(slot_netwManagerFinished(QNetworkReply*)));
    QUrl url("https://yt-dl.org/downloads/latest/youtube-dl");
    QNetworkRequest request(url);
    m_netwManager->get(request);
}

void MainWindow::slot_netwManagerFinished(QNetworkReply *reply)
{
    reply->deleteLater();
    if(reply->error() == QNetworkReply::NoError){
        // Get the http status code
        int v = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (v >= 200 && v < 300) // Success
        {
            if(reply->error() == QNetworkReply::NoError){
                core_file->write(reply->readAll());
                core_file->close();
                checkEngine();
                settingsUi.loading_movie->movie()->stop();
                settingsUi.loading_movie->setVisible(false);
                }else{
                core_file->remove();
            }
            settingsUi.download_engine->setEnabled(true);
        }
        else if (v >= 300 && v < 400) // Redirection
        {
            // Get the redirection url
            QUrl newUrl = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
            // Because the redirection url can be relative  we need to use the previous one to resolve it
            newUrl = reply->url().resolved(newUrl);
//                qDebug()<<newUrl<<"redirected url";

            QNetworkAccessManager *manager = new QNetworkAccessManager();
            connect(manager, SIGNAL(finished(QNetworkReply*)),this, SLOT(slot_netwManagerFinished(QNetworkReply*))); //keeep requesting until reach final url
            manager->get(QNetworkRequest(newUrl));
        }
    }
    else //error
    {
        QString err = reply->errorString();
        if(err.contains("not")){ //to hide "Host yt-dl.org not found"
       settingsUi.engine_status->setText("Host not Found");}
        else if(err.contains("session")||err.contains("disabled")){
            settingsUi.engine_status->setText(err);
        }
        settingsUi.loading_movie->movie()->stop();
        settingsUi.loading_movie->setVisible(false);
        settingsUi.download_engine->setEnabled(true);

        reply->manager()->deleteLater();
    }
    connect(reply,SIGNAL(downloadProgress(qint64,qint64)),this,SLOT(down_progress(qint64,qint64)));

}
void MainWindow::down_progress(qint64 pos,qint64 tot){
    qDebug()<<pos<<tot<<"fuck";
}

void MainWindow::evoke_engine_check(){
    if(settingsUi.engine_status->text()=="Absent"){
        QMessageBox msgBox;
          msgBox.setText("Olivia component is missing");
          msgBox.setInformativeText("Olivia engine is missing, download now ?");
          msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
          msgBox.setDefaultButton(QMessageBox::Ok);
          int ret = msgBox.exec();
          switch (ret) {
            case QMessageBox::Ok:
                      on_settings_clicked();
                      settingsUi.download_engine->click();
              break;
            case  QMessageBox::Cancel:
              break;
          }
    }
}

