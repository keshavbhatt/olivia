#include "mainwindow.h"
#include "ui_mainwindow.h"


#include <QSplitter>
#include <QUrlQuery>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QGraphicsDropShadowEffect>
#include <QMovie>
#include <QCompleter>
#include <QAction>
#include <QToolTip>

#include "cookiejar.h"
#include "elidedlabel.h"
#include "store.h"
#include "radio.h"
#include "onlinesearchsuggestion.h"
#include "seekslider.h"
#include "settings.h"
#include "paginator.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->setWindowFlags(Qt::FramelessWindowHint | Qt::CustomizeWindowHint | Qt::WindowTitleHint);
    qApp->setQuitOnLastWindowClosed(true);

    init_app(); // #1
    init_webview();// #2
    init_offline_storage();//  #3

    init_settings();
    init_miniMode();

    checkEngine();

    //sets search icon in label
    ui->label_5->resize(ui->label_5->width(),ui->search->height());
    ui->label_5->setPixmap(QPixmap(":/icons/sidebar/search.png").scaled(18,18,Qt::KeepAspectRatio,Qt::SmoothTransformation));

    store_manager = new store(this,"hjkfdsll");// #6

    pagination_manager = new paginator(this);
    connect(pagination_manager,SIGNAL(reloadRequested(QString,QString)),this,SLOT(reloadREquested(QString,QString)));

    ui->debug_widget->hide();

    loadPlayerQueue();// #7 loads previous playing track queue
    init_search_autoComplete();

    ui->radioVolumeSlider->setMinimum(0);
    ui->radioVolumeSlider->setMaximum(130);

    ui->radioVolumeSlider->setValue(100);
    saveTracksAfterBuffer = settingsObj.value("saveAfterBuffer","true").toBool();
    radio_manager = new radio(this,ui->radioVolumeSlider->value(),saveTracksAfterBuffer);

    connect(radio_manager,SIGNAL(radioStatus(QString)),this,SLOT(radioStatus(QString)));
    connect(radio_manager,SIGNAL(radioPosition(int)),this,SLOT(radioPosition(int)));
    connect(radio_manager,SIGNAL(radioDuration(int)),this,SLOT(radioDuration(int)));
    connect(radio_manager,SIGNAL(demuxer_cache_duration_changed(double,double)),this,SLOT(radio_demuxer_cache_duration_changed(double,double)));
    connect(radio_manager,SIGNAL(saveTrack(QString)),this,SLOT(saveTrack(QString)));

    connect(ui->radioSeekSlider,&seekSlider::setPosition,[=](QPoint localPos){
        ui->radioSeekSlider->blockSignals(true);
        radio_manager->radioSeek(ui->radioSeekSlider->minimum() + ((ui->radioSeekSlider->maximum()-ui->radioSeekSlider->minimum()) * localPos.x()) / ui->radioSeekSlider->width());
        ui->radioSeekSlider->blockSignals(false);
    });



    connect(ui->radioSeekSlider,&seekSlider::showToolTip,[=](QPoint localPos){
        int pos = ui->radioSeekSlider->minimum() + ((ui->radioSeekSlider->maximum()-ui->radioSeekSlider->minimum()) * localPos.x()) / ui->radioSeekSlider->width();
        int seconds = (pos) % 60;
        int minutes = (pos/60) % 60;
        int hours = (pos/3600) % 24;
        QTime time(hours, minutes,seconds);
        QToolTip::showText(ui->radioSeekSlider->mapToGlobal(localPos), "Seek: "+time.toString());
     });

    connect(ui->radioVolumeSlider,&volumeSlider::setPosition,[=](QPoint localPos){
        ui->radioVolumeSlider->setValue(ui->radioVolumeSlider->minimum() + ((ui->radioVolumeSlider->maximum()-ui->radioVolumeSlider->minimum()) * localPos.x()) / ui->radioVolumeSlider->width());
    });

    connect(ui->radioVolumeSlider,&volumeSlider::showToolTip,[=](QPoint localPos){
        if(localPos.x()!=0){
            int pos = ui->radioVolumeSlider->minimum() + ((ui->radioVolumeSlider->maximum()-ui->radioVolumeSlider->minimum()) * localPos.x()) / ui->radioVolumeSlider->width();
            if(pos>-1){
                if(pos < 101)
                    QToolTip::showText(ui->radioVolumeSlider->mapToGlobal(localPos), "Set volume: "+QString::number(pos));
                else if(pos<131)
                    QToolTip::showText(ui->radioVolumeSlider->mapToGlobal(localPos), "Set volume: "+QString::number(pos)+" (amplified)");
            }
        }else{
            int pos = ui->radioVolumeSlider->value();
            if(pos>-1){
                if(pos < 101)
                    QToolTip::showText(ui->radioVolumeSlider->mapToGlobal(localPos), "Volume: "+QString::number(pos));
                else if(pos<131)
                    QToolTip::showText(ui->radioVolumeSlider->mapToGlobal(localPos), "Volume: "+QString::number(pos)+" (amplified)");
            }
        }
    });

    browse();

    ui->top_widget->installEventFilter(this);
    ui->windowControls->installEventFilter(this);
    ui->label_6->installEventFilter(this);
    ui->nowPlayingGrip->installEventFilter(this);


    QString btn_style_2= "QPushButton{background-color:transparent ;border:0px;}"
                         "QPushButton:disabled { background-color: transparent; border-bottom:1px solid #727578;"
                         "padding-top: 3px; padding-bottom: 3px; padding-left: 5px; padding-right: 5px;color: #636363;}"
                         "QPushButton:pressed {padding-bottom:0px;background-color:transparent;border:0px;}"
                         "QPushButton:hover {border:none;padding-bottom:1px;background-color:transparent;border:0px;}";
    ui->close->setStyleSheet(btn_style_2);
    ui->minimize->setStyleSheet(btn_style_2);
    ui->maximize->setStyleSheet(btn_style_2);
    ui->fullScreen->setStyleSheet(btn_style_2);

    loadSettings();
}

void MainWindow::init_settings(){

    settUtils = new settings(this);
    connect(settUtils,SIGNAL(dynamicTheme(bool)),this,SLOT(dynamicThemeChanged(bool)));

    settingsObj.setObjectName("settings");
    setting_path =  QStandardPaths::writableLocation(QStandardPaths::DataLocation); 

    if(!QDir(setting_path).exists()){
        QDir d(setting_path);
        d.mkpath(setting_path);
    }

    settingsWidget = new QWidget(0);
    settingsWidget->setObjectName("settingsWidget");
    settingsUi.setupUi(settingsWidget);
    settingsWidget->setWindowFlags(Qt::Dialog);
    settingsWidget->setWindowModality(Qt::ApplicationModal);
    settingsWidget->adjustSize();
    connect(settingsUi.download_engine,SIGNAL(clicked()),this,SLOT(download_engine_clicked()));
    connect(settingsUi.saveAfterBuffer,SIGNAL(toggled(bool)),settUtils,SLOT(changeSaveAfterSetting(bool)));
    connect(settingsUi.showSearchSuggestion,SIGNAL(toggled(bool)),settUtils,SLOT(changeShowSearchSuggestion(bool)));
    connect(settingsUi.dynamicTheme,SIGNAL(toggled(bool)),settUtils,SLOT(changeDynamicTheme(bool)));
    connect(settingsUi.miniModeTransperancySlider,&QSlider::valueChanged,[=](int val){
        settUtils->changeMiniModeTransperancy(val);
        settingsUi.transperancyLabel->setText(QString::number(settingsUi.miniModeTransperancySlider->value()));
    });
    connect(settingsUi.miniModeStayOnTop,&QCheckBox::toggled,[=](bool checked){
        settUtils->changeMiniModeStayOnTop(checked);
        miniModeWidget->deleteLater();
        init_miniMode();
    });

    horizontalDpi = QApplication::desktop()->screen()->logicalDpiX();
    zoom = 100.0;
    connect(settingsUi.plus,SIGNAL(clicked(bool)),this,SLOT(zoomin()));
    connect(settingsUi.minus,SIGNAL(clicked(bool)),this,SLOT(zoomout()));

    settingsUi.zoom->setText(QString::number(ui->webview->zoomFactor(),'f',2));
    add_colors_to_color_widget();

}

void MainWindow::dynamicThemeChanged(bool enabled){
    settingsUi.themesWidget->setEnabled(!enabled);
}

void MainWindow::loadSettings(){
    settingsUi.saveAfterBuffer->setChecked(settingsObj.value("saveAfterBuffer","true").toBool());
    settingsUi.showSearchSuggestion->setChecked(settingsObj.value("showSearchSuggestion","true").toBool());
    settingsUi.miniModeStayOnTop->setChecked(settingsObj.value("miniModeStayOnTop","false").toBool());

    settingsUi.dynamicTheme->setChecked(settingsObj.value("dynamicTheme","false").toBool());
    setZoom(settingsObj.value("zoom","100.0").toFloat());
    settingsUi.miniModeTransperancySlider->setValue(settingsObj.value("miniModeTransperancy","95").toInt());
    settingsUi.transperancyLabel->setText(QString::number(settingsUi.miniModeTransperancySlider->value()));

    //keep this after init of settings widget
    if(settingsObj.value("dynamicTheme").toBool()==false){
        QString rgbhash = settingsObj.value("customTheme","#3BBAC6").toString();
        set_app_theme(QColor(rgbhash));
        if(color_list.contains(rgbhash,Qt::CaseInsensitive)){
            if(settingsWidget->findChild<QPushButton*>(rgbhash.toUpper())){
                settingsWidget->findChild<QPushButton*>(rgbhash.toUpper())->setText("*");
            }
        }
    }

    ui->tabWidget->setCurrentIndex(settingsObj.value("currentQueueTab","0").toInt());
    restoreGeometry(settingsObj.value("geometry").toByteArray());
    restoreState(settingsObj.value("windowState").toByteArray());
}

void MainWindow::add_colors_to_color_widget(){

        color_list<<"fakeitem"<<"#FF0034"<<"#0070FF"<<"#029013"
                        <<"#D22298"<<"#FF901F"<<"#836C50";

        QObject *layout = settingsWidget->findChild<QObject*>("themeHolderGridLayout");
        int row=0;
        int numberOfButtons=0;
        while (numberOfButtons<=color_list.count())
        {
            for (int f2=0; f2<3; f2++)
            {   numberOfButtons++;
                if (numberOfButtons>color_list.count()-1)
                    break;
                QPushButton *pb =new QPushButton();
                pb->setObjectName(color_list.at(numberOfButtons));
                pb->setStyleSheet("background-color:"+color_list.at(numberOfButtons)+";border:1px;padding:2px;");
                connect(pb,&QPushButton::clicked,[=](){
                  set_app_theme(QColor(pb->objectName()));

                  for(int i(0);i<color_list.count();i++){
                     if(settingsWidget->findChild<QPushButton*>(color_list.at(i))){
                         settingsWidget->findChild<QPushButton*>(color_list.at(i))->setText("");
                     }
                  }
                  pb->setText("*");
                });
                ((QGridLayout *)(layout))->addWidget(pb, row, f2);
            }
            row++;
        }
        QPushButton *pb =new QPushButton();
        pb->setIcon(QIcon(":/icons/picker.png"));
        pb->setIconSize(QSize(22,22));
        pb->setObjectName("custom color");
        pb->setText("Select color");
        pb->setToolTip("Choose custom color from Color Dialog");
        connect(pb,SIGNAL(clicked(bool)),this,SLOT(customColor()));
        ((QGridLayout *)(layout))->addWidget(pb, row+1, 0);
        settingsWidget->adjustSize();
}

void MainWindow::set_app_theme(QColor rgb){

    settingsObj.setValue("customTheme",rgb);

    QString r = QString::number(rgb.red());
    QString g = QString::number(rgb.green());
    QString b = QString::number(rgb.blue());

    if(!color_list.contains(rgb.name(),Qt::CaseInsensitive)){
          for(int i(0);i<color_list.count();i++){
           if(settingsWidget->findChild<QPushButton*>(color_list.at(i))){
               settingsWidget->findChild<QPushButton*>(color_list.at(i))->setText("");
           }
        }
    }

    themeColor = r+","+g+","+b+","+"0.2";

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
    QString scrollbarStyle ="QScrollBar:vertical {"
                                "background-color: transparent;"
                                "border:none;"
                                "width: 10px;"
                                "margin: 22px 0 22px 0;"
                            "}"
                            "QScrollBar::handle:vertical {"
                                "background: grey;"
                                "min-height: 20px;"
                            "}";
    ui->left_panel->setStyleSheet("QWidget#left_panel{"+widgetStyle+"}");
    ui->right_panel->setStyleSheet("QWidget#right_panel{"+widgetStyle+"}");
    ui->right_list->setStyleSheet("QListWidget{"+widgetStyle+"}"+scrollbarStyle);
    ui->right_list_2->setStyleSheet("QListWidget{"+widgetStyle+"}"+scrollbarStyle);


    miniModeWidget->setStyleSheet ( ui->left_panel->styleSheet().replace("#left_panel","#miniModeWidget"));

    ui->search->setStyleSheet(widgetStyle+"border:none;border-radius:0px;");
    ui->label_5->setStyleSheet(widgetStyle+"border:none;border-radius:0px;");

    settingsWidget->setStyleSheet("QWidget#settingsWidget{"+widgetStyle+"}");


}

void MainWindow::customColor(){
        SelectColorButton *s=new SelectColorButton();
        connect(s,SIGNAL(setCustomColor(QColor)),this,SLOT(set_app_theme(QColor)));
        s->changeColor();
}

void SelectColorButton::updateColor(){
    emit setCustomColor(color);
}

void SelectColorButton::changeColor(){
   QColor newColor = QColorDialog::getColor(color,parentWidget());
   if ( newColor != color )
   {
           setColor( newColor );
   }
}

void SelectColorButton::setColor( const QColor& color ){
    this->color = color;
    updateColor();
}

const QColor& SelectColorButton::getColor(){
    return color;
}


//set up app #1
void MainWindow::init_app(){

    QSplitter *split1 = new QSplitter;
    split1->setObjectName("split1");

    split1->addWidget(ui->center_widget);
    split1->addWidget(ui->right_panel);
    split1->setOrientation(Qt::Horizontal);

    QSplitter *split2 = new QSplitter;
    split2->setObjectName("split2");
    split2->addWidget(ui->left_panel);
    split2->addWidget(split1);
    split2->setOrientation(Qt::Horizontal);

    ui->horizontalLayout_3->addWidget(split2);

    connect(split2,&QSplitter::splitterMoved,[=](int pos){
        if(animationRunning==false){
            left_panel_width = pos;
        }
    });

    setWindowIcon(QIcon(":/icons/olivia.png"));
    setWindowTitle(QApplication::applicationName());


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

        QTextDocument text;
        text.setHtml(title);
        QString plainTitle = text.toPlainText();

        QWidget *track_widget = new QWidget(ui->right_list);
        track_widget->setObjectName("track-widget-"+songId);
        track_ui.setupUi(track_widget);

        QFont font("Ubuntu");
        font.setPixelSize(12);
        setFont(font);

        ElidedLabel *titleLabel = new ElidedLabel(plainTitle,0);
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


        if(albumId.contains("undefined-")){

            QListWidgetItem* item;
            item = new QListWidgetItem(ui->right_list_2);

            QGraphicsOpacityEffect *eff = new QGraphicsOpacityEffect(this);

            item->setSizeHint(track_widget->minimumSizeHint());

            ui->right_list_2->setItemWidget(item, track_widget);

            track_ui.cover->setMaximumHeight(track_widget->height());
            track_ui.cover->setMaximumWidth((int)(track_widget->height()*1.15));

            ui->right_list_2->itemWidget(item)->setGraphicsEffect(eff);

            // checks if url is expired and updates item with new url which can be streamed .... until keeps the track item disabled.
            if(url.isEmpty() || (store_manager->getExpiry(songId) && track_ui.url->text().contains("http"))){
            //track_ui.loading->setPixmap(QPixmap(":/icons/url_issue.png").scaled(track_ui.loading->size(),Qt::KeepAspectRatio,Qt::SmoothTransformation));
                ui->right_list_2->itemWidget(item)->setEnabled(false);
                if(!track_ui.id->text().isEmpty()){
                    getAudioStream(track_ui.id->text().trimmed(),track_ui.songId->text().trimmed());
                }
            }else{
            //track_ui.loading->setPixmap(QPixmap(":/icons/blank.png").scaled(track_ui.loading->size(),Qt::KeepAspectRatio,Qt::SmoothTransformation));
            }
            QPropertyAnimation *a = new QPropertyAnimation(eff,"opacity");
            a->setDuration(500);
            a->setStartValue(0);
            a->setEndValue(1);
            a->setEasingCurve(QEasingCurve::InCirc);
            a->start(QPropertyAnimation::DeleteWhenStopped);
            ui->right_list_2->addItem(item);
        }else{
            QListWidgetItem* item;
            item = new QListWidgetItem(ui->right_list);

            QGraphicsOpacityEffect *eff = new QGraphicsOpacityEffect(this);

            item->setSizeHint(track_widget->minimumSizeHint());


            ui->right_list->setItemWidget(item, track_widget);
            ui->right_list->itemWidget(item)->setGraphicsEffect(eff);

            // checks if url is expired and updates item with new url which can be streamed .... until keeps the track item disabled.
            if(url.isEmpty() || (store_manager->getExpiry(songId) && track_ui.url->text().contains("http"))){
            //track_ui.loading->setPixmap(QPixmap(":/icons/url_issue.png").scaled(track_ui.loading->size(),Qt::KeepAspectRatio,Qt::SmoothTransformation));
                ui->right_list->itemWidget(item)->setEnabled(false);
                if(!track_ui.id->text().isEmpty()){
                    getAudioStream(track_ui.id->text().trimmed(),track_ui.songId->text().trimmed());
                }
            }else{
            //track_ui.loading->setPixmap(QPixmap(":/icons/blank.png").scaled(track_ui.loading->size(),Qt::KeepAspectRatio,Qt::SmoothTransformation));
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
    ui->tabWidget->resize(ui->tabWidget->size().width()-1,ui->tabWidget->size().height());
    ui->tabWidget->resize(ui->tabWidget->size().width()+1,ui->tabWidget->size().height());
}


void MainWindow::keyPressEvent(QKeyEvent *event){
    if( event->key() == Qt::Key_D )
       {
           if(!ui->debug_widget->isVisible())
               ui->debug_widget->show();
           else ui->debug_widget->hide();
       }
    if( event->key() == Qt::Key_F11 )
       {
        if(this->isFullScreen()){
            this->showMaximized();
        }else{
            this->showFullScreen();
        }
       }
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event){
    if ((obj == ui->nowPlayingGrip || ui->top_widget  || ui->windowControls || ui->label_6 ) && (event->type() == QEvent::MouseMove)) {
            const QMouseEvent* const me = static_cast<const QMouseEvent*>( event );
            if (me->buttons() & Qt::LeftButton) {
                if(obj==ui->nowPlayingGrip){
                    miniModeWidget->move(me->globalPos() - oldPosMiniWidget);
                }else{
                    move(me->globalPos() - oldPos);
                }
                event->accept();
            }
            return true;
    }
    if ((obj == ui->nowPlayingGrip || ui->top_widget  || ui->windowControls || ui->label_6) &&
        (event->type() == QEvent::MouseButtonPress)) {
            const QMouseEvent* const me = static_cast<const QMouseEvent*>( event );
            if (me->button() == Qt::LeftButton) {
                if(obj==ui->nowPlayingGrip){
                    oldPosMiniWidget = QCursor::pos() - miniModeWidget->frameGeometry().topLeft();
                }else{
                    oldPos = me->globalPos() - frameGeometry().topLeft();
                }
                event->accept();
            }
            return true;
    }
    return obj->eventFilter(obj, event);
}



void MainWindow::init_search_autoComplete(){
    ui->search->installEventFilter(this);
    _onlineSearchSuggestion_ = new onlineSearchSuggestion(ui->search);
}


void MainWindow::setPlayerPosition(qint64 position){

    int seconds = (position/1000) % 60;
    int minutes = (position/60000) % 60;
    int hours = (position/3600000) % 24;
    QTime time(hours, minutes,seconds);
    ui->position->setText(time.toString());

    ui->radioSeekSlider->setValue(position);
}


void MainWindow::on_play_pause_clicked()
{
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
}

void MainWindow::on_radioSeekSlider_sliderMoved(int position)
{
    ui->radioSeekSlider->blockSignals(true);
    ui->radioSeekSlider->setSliderPosition(position);
    radio_manager->radioSeek(position);
    ui->radioSeekSlider->blockSignals(false);
}


void MainWindow::on_stop_clicked()
{
    //radio stop
    radio_manager->stop();
}


//NETWORK
void MainWindow::quitApp(){
    settingsObj.setValue("geometry",saveGeometry());
    settingsObj.setValue("windowState", saveState());
    radio_manager->killRadioProcess();
    radio_manager->deleteProcess(0);
    radio_manager->deleteLater();
}

void MainWindow::closeEvent(QCloseEvent *event){
    settingsObj.setValue("geometry",saveGeometry());
    settingsObj.setValue("windowState", saveState());
    radio_manager->killRadioProcess(); //kill radio and all other processes
    qApp->quit();
    QMainWindow::closeEvent(event);
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::webViewLoaded(bool loaded){

    if(loaded){
        ui->webview->page()->mainFrame()->addToJavaScriptWindowObject(QString("mainwindow"),  this);
        ui->webview->page()->mainFrame()->addToJavaScriptWindowObject(QString("paginator"), pagination_manager);
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

    if( loaded && pageType == "youtube" && !youtubeSearchTerm.isEmpty()){
        ui->webview->page()->mainFrame()->evaluateJavaScript("$('.ui-content').fadeOut('slow');$('#manual_search').val('"+youtubeSearchTerm+"');manual_youtube_search('"+youtubeSearchTerm+"');");
        youtubeSearchTerm.clear();
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

void MainWindow::setSearchTermAndOpenYoutube(QVariant term){
    youtubeSearchTerm = term.toString();
    ui->left_list->setCurrentRow(14); //set youtube page
}

void MainWindow::resultLoaded(){
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
        //TODO provide user hint that track already in list
        return;
    }else{
        QWidget *track_widget = new QWidget(ui->right_list);
        track_widget->setObjectName("track-widget-"+songId);
        track_ui.setupUi(track_widget);

        QFont font("Ubuntu");
        font.setPixelSize(12);
        setFont(font);

        //to convert html sequence to plaintext
        QTextDocument text;
        text.setHtml(title);
        QString plainTitle = text.toPlainText();

        ElidedLabel *titleLabel = new ElidedLabel(plainTitle,0);
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

        if(albumId.contains("undefined-")){
            track_ui.cover->setMaximumHeight(track_widget->height());
            track_ui.cover->setMaximumWidth((int)(track_widget->height()*1.15));
            QListWidgetItem* item;
            item = new QListWidgetItem(ui->right_list_2);
            QGraphicsOpacityEffect *eff = new QGraphicsOpacityEffect(this);
            item->setSizeHint(track_widget->minimumSizeHint());
            ui->right_list_2->setItemWidget(item, track_widget);
            ui->right_list_2->itemWidget(item)->setGraphicsEffect(eff);
            ui->right_list_2->itemWidget(item)->setEnabled(false); //enable when finds a url

            QPropertyAnimation *a = new QPropertyAnimation(eff,"opacity");
            a->setDuration(500);
            a->setStartValue(0);
            a->setEndValue(1);
            a->setEasingCurve(QEasingCurve::InCirc);
            a->start(QPropertyAnimation::DeleteWhenStopped);
            ui->right_list_2->addItem(item);

            ui->right_list_2->setCurrentRow(ui->right_list_2->count()-1);
            if(store_manager->isDownloaded(songId)){
                ui->right_list_2->itemWidget(item)->setEnabled(true);
                track_ui.url->setText("file://"+setting_path+"/downloadedTracks/"+songId);
                track_ui.offline->setPixmap(QPixmap(":/icons/offline.png").scaled(track_ui.offline->size(),Qt::KeepAspectRatio,Qt::SmoothTransformation));
            }else{
              getAudioStream(id,songId);
            }
            ui->tabWidget->setCurrentWidget(ui->tab_2);
            ui->right_list_2->scrollToBottom();
        }else{
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
            ui->tabWidget->setCurrentWidget(ui->tab);
            ui->right_list->scrollToBottom();
        }

        //SAVE DATA TO LOCAL DATABASE
            store_manager->saveAlbumArt(albumId,base64);
            store_manager->saveArtist(artistId,artist);
            store_manager->saveAlbum(albumId,album);
            store_manager->saveDominantColor(albumId,dominantColor);
            store_manager->saveytIds(songId,id);
            store_manager->setTrack(QStringList()<<songId<<albumId<<artistId<<title);
            store_manager->add_to_player_queue(songId);
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
            for (int i= 0;i<ui->right_list_2->count();i++) {
               QString songIdFromWidget = ((QLineEdit*) ui->right_list_2->itemWidget(ui->right_list_2->item(i))->findChild<QLineEdit*>("songId"))->text().trimmed();
                if(songId==songIdFromWidget){
                    ui->right_list_2->itemWidget(ui->right_list_2->item(i))->findChild<QLabel*>("offline")->setPixmap(QPixmap(":/icons/blank.png"));
                    ui->right_list_2->itemWidget(ui->right_list_2->item(i))->findChild<QLineEdit*>("url")->setText(store_manager->getOfflineUrl(songId));
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
          //  qDebug()<<"removed Song :"<<songId;
            for (int i= 0;i<ui->right_list->count();i++) {
               QString songIdFromWidget = ((QLineEdit*) ui->right_list->itemWidget(ui->right_list->item(i))->findChild<QLineEdit*>("songId"))->text().trimmed();
                if(songId==songIdFromWidget){
                    ui->right_list->takeItem(i);
                    store_manager->removeFromQueue(songId);
                    break;
                }
            }
            for (int i= 0;i<ui->right_list_2->count();i++) {
               QString songIdFromWidget = ((QLineEdit*) ui->right_list_2->itemWidget(ui->right_list_2->item(i))->findChild<QLineEdit*>("songId"))->text().trimmed();
                if(songId==songIdFromWidget){
                    ui->right_list_2->takeItem(i);
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

    if(ytdlProcess==nullptr && ytdlQueue.count()>0){
        processYtdlQueue();
    }

}

void MainWindow::processYtdlQueue(){

    if(ytdlQueue.count()>0){
        QString ytIds = QString(ytdlQueue.at(0).at(0).split(",").first());
        QString songId = QString(ytdlQueue.at(0).at(1).split(",").last());

        ytdlQueue.removeAt(0);
        if(ytdlProcess == nullptr){
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
    Q_UNUSED(code);
    ytdlProcess = nullptr;
    if(ytdlQueue.count()>0){
        qDebug()<<"YoutubedlQueueSize:"<<ytdlQueue.count();
        processYtdlQueue();
    }
}

void MainWindow::ytdlReadyRead(){

    QProcess* senderProcess = qobject_cast<QProcess*>(sender());
    QString songId = senderProcess->objectName().trimmed();

    QByteArray b;
    b.append(senderProcess->readAll());
    QString s_data = QTextCodec::codecForMib(106)->toUnicode(b).trimmed();
  //  qDebug()<<s_data;

    if(!s_data.isEmpty()){
            QWidget *listWidget = ui->right_list->findChild<QWidget*>("track-widget-"+songId);
            if(listWidget==nullptr){
                listWidget= ui->right_list_2->findChild<QWidget*>("track-widget-"+songId);
            }
            if(s_data.contains("http")){
                listWidget->setEnabled(true);
//              listWidget->findChild<QLabel*>("loading")->setPixmap(QPixmap(":/icons/blank.png").scaled(track_ui.loading->size(),Qt::KeepAspectRatio,Qt::SmoothTransformation));
                QLineEdit *url = listWidget->findChild<QLineEdit *>("url");
                QString url_str = s_data.trimmed();
                ((QLineEdit*)(url))->setText(url_str);

                QProcess* senderProcess = qobject_cast<QProcess*>(sender()); // retrieve the button clicked
                senderProcess->kill();

                QString expiryTime = QUrlQuery(QUrl::fromPercentEncoding(url_str.toUtf8())).queryItemValue("expire").trimmed();
                if(expiryTime.isEmpty()){
                    expiryTime = url_str.split("/expire/").last().split("/").first().trimmed();
                }
                store_manager->saveStreamUrl(songId,url_str,expiryTime);
            }
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
         browse_youtube();
        break;
    case 15:
         internet_radio();
        break;
    case 17:
         qApp->quit();
        break;
    default:
        break;
    }
}

void MainWindow::browse_youtube(){
    pageType="youtube";
    ui->webview->load(QUrl("qrc:///web/youtube/youtube.html"));
}

void MainWindow::clear_youtubeSearchTerm(){
    youtubeSearchTerm.clear();
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

//PLAY TRACK ON ITEM DOUBLE CLICKED////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::on_right_list_itemDoubleClicked(QListWidgetItem *item)
{
    listItemDoubleClicked(ui->right_list,item);

    //hide playing labels in other list
    QList<QLabel*> playing_label_list_other;
    playing_label_list_other = ui->right_list_2->findChildren<QLabel*>("playing");
    foreach (QLabel *playing, playing_label_list_other) {
        playing->setPixmap(QPixmap(":/icons/blank.png").scaled(playing->size(),Qt::KeepAspectRatio,Qt::SmoothTransformation));
        playing->setToolTip("");
    }
}

void MainWindow::on_right_list_2_itemDoubleClicked(QListWidgetItem *item)
{
    listItemDoubleClicked(ui->right_list_2,item);

    //hide playing labels in other list
    QList<QLabel*> playing_label_list_other;
    playing_label_list_other = ui->right_list->findChildren<QLabel*>("playing");
    foreach (QLabel *playing, playing_label_list_other) {
        playing->setPixmap(QPixmap(":/icons/blank.png").scaled(playing->size(),Qt::KeepAspectRatio,Qt::SmoothTransformation));
        playing->setToolTip("");
    }
}

void MainWindow::listItemDoubleClicked(QListWidget *list,QListWidgetItem *item){

    if(!list->itemWidget(item)->isEnabled())
        return;
    QString id =  list->itemWidget(item)->findChild<QLineEdit*>("id")->text();
    QString url = list->itemWidget(item)->findChild<QLineEdit*>("url")->text();
    QString songId = list->itemWidget(item)->findChild<QLineEdit*>("songId")->text();

    ui->webview->page()->mainFrame()->evaluateJavaScript("setNowPlaying('"+songId+"')");

    Q_UNUSED(id);

    ElidedLabel *title = list->itemWidget(item)->findChild<ElidedLabel *>("title_elided");
    QString titleStr = ((ElidedLabel*)(title))->text();

    ElidedLabel *artist = list->itemWidget(item)->findChild<ElidedLabel *>("artist_elided");
    QString artistStr = ((ElidedLabel*)(artist))->text();

    ElidedLabel *album = list->itemWidget(item)->findChild<ElidedLabel *>("album_elided");
    QString albumStr = ((ElidedLabel*)(album))->text();

    QString dominant_color = list->itemWidget(item)->findChild<QLineEdit*>("dominant_color")->text();

    QLabel *cover = list->itemWidget(item)->findChild<QLabel *>("cover");
    ui->cover->setPixmap(QPixmap(*cover->pixmap()).scaled(100,100,Qt::KeepAspectRatio,Qt::SmoothTransformation));

   //hide playing labels in current list
    QList<QLabel*> playing_label_list_;
    playing_label_list_ = list->findChildren<QLabel*>("playing");
    foreach (QLabel *playing, playing_label_list_) {
        playing->setPixmap(QPixmap(":/icons/blank.png").scaled(playing->size(),Qt::KeepAspectRatio,Qt::SmoothTransformation));
        playing->setToolTip("");
    }

    //show now playing on current track
    QLabel *playing = list->itemWidget(item)->findChild<QLabel *>("playing");
    playing->setPixmap(QPixmap(":/icons/now_playing.png").scaled(playing->size(),Qt::KeepAspectRatio,Qt::SmoothTransformation));
    playing->setToolTip("playing...");

    getNowPlayingTrackId();

    ui->nowplaying_widget->setImage(*cover->pixmap());

    if(!ui->state->isVisible())
        ui->state->show();
    ui->state->setText("Connecting...");


    if(settingsObj.value("dynamicTheme").toBool()==true){
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
        QString scrollbarStyle ="QScrollBar:vertical {"
                                    "background-color: transparent;"
                                    "border:none;"
                                    "width: 10px;"
                                    "margin: 22px 0 22px 0;"
                                "}"
                                "QScrollBar::handle:vertical {"
                                    "background: grey;"
                                    "min-height: 20px;"
                                "}";
        ui->left_panel->setStyleSheet("QWidget#left_panel{"+widgetStyle+"}");
        ui->right_panel->setStyleSheet("QWidget#right_panel{"+widgetStyle+"}");
        ui->right_list->setStyleSheet("QListWidget{"+widgetStyle+"}"+scrollbarStyle);
        ui->right_list_2->setStyleSheet("QListWidget{"+widgetStyle+"}"+scrollbarStyle);


        miniModeWidget->setStyleSheet ( ui->left_panel->styleSheet().replace("#left_panel","#miniModeWidget"));

        ui->search->setStyleSheet(widgetStyle+"border:none;border-radius:0px;");
        ui->label_5->setStyleSheet(widgetStyle+"border:none;border-radius:0px;");

        settingsWidget->setStyleSheet("QWidget#settingsWidget{"+widgetStyle+"}");


    }

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
        saveTracksAfterBuffer =  settingsObj.value("saveAfterBuffer","true").toBool();
        radio_manager->playRadio(saveTracksAfterBuffer,QUrl(url));
    }
}
//END PLAY TRACK ON ITEM DOUBLE CLICKED////////////////////////////////////////////////////////////////////////////////////////



//app menu to hide show sidebar
void MainWindow::on_menu_clicked()
{
    QSplitter *split2= this->findChild<QSplitter*>("split2");
    if(((QSplitter*)(split2))->sizes()[0]==0){ //closed state
        QPropertyAnimation *animation = new QPropertyAnimation(ui->left_panel, "geometry");
        animation->setDuration(100);
        QRect startRect=ui->left_panel->rect();
        QRect finalRect=startRect;
        finalRect.setWidth(left_panel_width);
        animation->setStartValue(startRect);
        animation->setEndValue(finalRect);
        animation->setEasingCurve(QEasingCurve::InCurve);
        animation->start(QAbstractAnimation::DeleteWhenStopped);
        animationRunning = true;
        connect(animation,&QPropertyAnimation::valueChanged, [=](QVariant var){
            ((QSplitter*)(split2))->setSizes(QList<int>()<<var.toRect().width()<<((QSplitter*)(split2))->sizes()[1]-var.toRect().width());
        });
        connect(animation,&QPropertyAnimation::finished, [=](){
            animationRunning = false;
        });
        ui->menu->setIcon(QIcon(":/icons/close.png"));
    }else{
        QPropertyAnimation *animation = new QPropertyAnimation(ui->left_panel, "geometry");
        animation->setDuration(100);
        QRect startRect=ui->left_panel->rect();
        QRect finalRect=startRect;
        finalRect.setWidth(0);
        animation->setStartValue(startRect);
        animation->setEndValue(finalRect);
        animation->setEasingCurve(QEasingCurve::OutCurve);
        animation->start(QAbstractAnimation::DeleteWhenStopped);
        animationRunning = true;
        connect(animation,&QPropertyAnimation::valueChanged, [=](QVariant var){
              ((QSplitter*)(split2))->setSizes(QList<int>()<<var.toRect().width()<<((QSplitter*)(split2))->sizes()[1]+var.toRect().width());
        });
        connect(animation,&QPropertyAnimation::finished, [=](){
            animationRunning = false;
        });
        ui->menu->setIcon(QIcon(":/icons/menu.png"));
    }
}


void MainWindow::on_settings_clicked()
{
    if(!settingsWidget->isVisible())
    {
        settingsWidget->setGeometry(
            QStyle::alignedRect(
                Qt::LeftToRight,
                Qt::AlignCenter,
                settingsWidget->size(),
                qApp->desktop()->availableGeometry()
            )
        );
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
    for(int i = 0 ; i< ui->right_list_2->count();i++){
         if(ui->right_list_2->itemWidget(ui->right_list_2->item(i))->findChild<QLabel*>("playing")->toolTip()=="playing..."){
            //get songId of visible track
            QLineEdit *songId = ui->right_list_2->itemWidget(ui->right_list_2->item(i))->findChild<QLineEdit*>("songId");
            nowPlayingSongId = ((QLineEdit*)(songId))->text();
        }
    }
}

void MainWindow::resizeEvent(QResizeEvent *resizeEvent){
    left_panel_width = ui->left_panel->width();
    ui->tabWidget->resize(ui->tabWidget->size().width()-1,ui->tabWidget->size().height());
    ui->tabWidget->resize(ui->tabWidget->size().width()+1,ui->tabWidget->size().height());
    QMainWindow::resizeEvent(resizeEvent);
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
        ui->stop->setEnabled(true);
        ui->play_pause->setIcon(QIcon(":/icons/p_pause.png"));
    }else if(radioState=="paused"){
        ui->stop->setEnabled(true);
        ui->play_pause->setIcon(QIcon(":/icons/p_play.png"));
    }else if(radioState=="stopped"){
        ui->stop->setEnabled(false);
        ui->console->setText("stopped");
        ui->play_pause->setIcon(QIcon(":/icons/p_play.png"));
        for (int i= 0;i<ui->right_list->count();i++) {
          ui->right_list->itemWidget(ui->right_list->item(i))->findChild<QLabel*>("playing")->setPixmap(QPixmap(":/icons/blank.png"));
        }
        for (int i= 0;i<ui->right_list_2->count();i++) {
          ui->right_list_2->itemWidget(ui->right_list_2->item(i))->findChild<QLabel*>("playing")->setPixmap(QPixmap(":/icons/blank.png"));
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


//this method plays tracks from webpage
void MainWindow::playLocalTrack(QVariant songIdVar){

    QString url,songId,title,album,artist,base64;
    songId = songIdVar.toString();
    QStringList tracskList ;
    tracskList = store_manager->getTrack(songId);

    title = tracskList[1];
    album = tracskList[3];
    artist = tracskList[5];
    base64 = tracskList[6];
    nowPlayingSongId = songId;
    QString setting_path = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
    url = "file://"+setting_path+"/downloadedTracks/"+songId;
    QVariant data = url+"=,="+title+"=,="+album+"=,="+artist+"=,="+base64;
    playRadioFromWeb(data);
    ui->webview->page()->mainFrame()->evaluateJavaScript("setNowPlaying('"+songId+"')");


    //if track is not in lists add it to queue
    if(!store_manager->isInQueue(songIdVar.toString())){
        QStringList trackDetails = store_manager->getTrack(songId);
        QString id,title,artist,album,base64,dominantColor,albumId,artistId,url;
        title = trackDetails.at(1);
        albumId = trackDetails.at(2);
        album = trackDetails.at(3);
        artistId = trackDetails.at(4);
        artist = trackDetails.at(5);
        base64 = trackDetails.at(6);
        url = trackDetails.at(7);
        id = trackDetails.at(8);
        dominantColor = trackDetails.at(9);
        addToQueue(id,title,artist,album,base64,dominantColor,songId,albumId,artistId);
    }else{//else find and play the track
        for (int i= 0;i<ui->right_list->count();i++) {
           QString songIdFromWidget = ((QLineEdit*) ui->right_list->itemWidget(ui->right_list->item(i))->findChild<QLineEdit*>("songId"))->text().trimmed();
            if(songId==songIdFromWidget){
                ui->tabWidget->setCurrentWidget(ui->tab);
                ui->right_list->setCurrentRow(i);
                listItemDoubleClicked(ui->right_list,ui->right_list->item(i));
                ui->right_list->scrollToItem(ui->right_list->item(i));
                 break;
            }
        }
        for (int i= 0;i<ui->right_list_2->count();i++) {
           QString songIdFromWidget = ((QLineEdit*) ui->right_list_2->itemWidget(ui->right_list_2->item(i))->findChild<QLineEdit*>("songId"))->text().trimmed();
            if(songId==songIdFromWidget){
                ui->tabWidget->setCurrentWidget(ui->tab_2);
                ui->right_list_2->setCurrentRow(i);
                listItemDoubleClicked(ui->right_list_2,ui->right_list_2->item(i));
                ui->right_list_2->scrollToItem(ui->right_list_2->item(i));
                break;
            }
        }
    }
}

void MainWindow::playRadioFromWeb(QVariant streamDetails){
    QString url,title,country,language,base64;
    QStringList list = streamDetails.toString().split("=,=");
    url = list.at(0);
    title = list.at(1);
    country = list.at(2);
    language = list.at(3);
    if(list.count()>4){
        base64 = list.at(4);
        base64 = base64.split("base64,").last();
        QByteArray ba = base64.toUtf8();
        QPixmap image;
        image.loadFromData(QByteArray::fromBase64(ba));
        if(!image.isNull()){
            ui->cover->setPixmap(QPixmap(image).scaled(100,100,Qt::KeepAspectRatio,Qt::SmoothTransformation));
        }
    }else{
        ui->cover->setPixmap(QPixmap(":/web/radio/station_cover.jpg"));
    }

    QTextDocument text;
    text.setHtml(title);
    QString plainTitle = text.toPlainText();

    this->findChild<ElidedLabel *>("nowP_title")->setText(plainTitle);

    this->findChild<ElidedLabel *>("nowP_artist")->setText(language);

    this->findChild<ElidedLabel *>("nowP_album")->setText(country);

    //always false as we don't want record radio for now
    saveTracksAfterBuffer=false;
    radio_manager->playRadio(saveTracksAfterBuffer,QUrl(url.trimmed()));

    //clear playing icon from player queue
    for (int i= 0;i<ui->right_list->count();i++) {
      ui->right_list->itemWidget(ui->right_list->item(i))->findChild<QLabel*>("playing")->setPixmap(QPixmap(":/icons/blank.png"));
    }
    for (int i= 0;i<ui->right_list_2->count();i++) {
      ui->right_list_2->itemWidget(ui->right_list_2->item(i))->findChild<QLabel*>("playing")->setPixmap(QPixmap(":/icons/blank.png"));
    }
}


//TODO
void MainWindow::saveTrack(QString format){
    Q_UNUSED(format);
    QString download_Path =  QStandardPaths::writableLocation(QStandardPaths::DataLocation)+"/downloadedTracks/";

    QString downloadTemp_Path =  QStandardPaths::writableLocation(QStandardPaths::DataLocation)+"/downloadedTemp/";
    QFile file(downloadTemp_Path+"current.temp"); //+"."+format

    if(file.copy(download_Path+nowPlayingSongId)){
        file.close();
    //    qDebug()<<"saved"<<nowPlayingSongId<<"as"<<format<<"in"<<file.fileName();
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
      for(int i = 0 ; i< ui->right_list_2->count();i++){
           if(ui->right_list_2->itemWidget(ui->right_list_2->item(i))->findChild<QLineEdit*>("songId")->text()==nowPlayingSongId){
              QLabel *offline = ui->right_list_2->itemWidget(ui->right_list_2->item(i))->findChild<QLabel*>("offline");
              ((QLabel*)(offline))->setPixmap(QPixmap(":/icons/offline.png").scaled(track_ui.offline->size(),Qt::KeepAspectRatio,Qt::SmoothTransformation));
              ui->right_list_2->itemWidget(ui->right_list_2->item(i))->findChild<QLineEdit*>("url")->setText("file://"+download_Path+nowPlayingSongId);
          }
      }
}
////////////////////////////////////////////////////////////END RADIO///////////////////////////////////////////////////////



//ENGINE STUFF//////////////////////////////////////////////////////////////////////////////////////////////////////////////
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
    settingsUi.engine_status->setText("Downloading core(1.4mb)");
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
        qDebug()<<"Could not open a file to write.";
    }

    QNetworkAccessManager *m_netwManager = new QNetworkAccessManager(this);
    connect(m_netwManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(slot_netwManagerFinished(QNetworkReply*)));
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
//END ENGINE STUFF//////////////////////////////////////////////////////////////////////////////////////////////////////////////




//Filter Lists///////////////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::on_filter_olivia_textChanged(const QString &arg1){
    filterList(arg1,ui->right_list); //olivia
}

void MainWindow::on_filter_youtube_textChanged(const QString &arg1){
    filterList(arg1,ui->right_list_2); //youtube
}

void MainWindow::filterList(const QString &arg1,QListWidget *list)
{
    hideListItems(list);

    OliviaMetaList.clear();
    fillOliviaMetaList(list);

    for(int i= 0 ; i < OliviaMetaList.count(); i++){
        if(QString(OliviaMetaList.at(i)).contains(arg1,Qt::CaseInsensitive)){
            QListWidgetItem *item = list->item(i);
            item->setHidden(false);
        }
    }
}
void MainWindow::fillOliviaMetaList(QListWidget *list){
    for(int i= 0 ; i <list->count(); i++){
        QListWidgetItem *item = list->item(i);

        ElidedLabel *title = list->itemWidget(item)->findChild<ElidedLabel *>("title_elided");
        QString titleStr = ((ElidedLabel*)(title))->text();

        ElidedLabel *artist = list->itemWidget(item)->findChild<ElidedLabel *>("artist_elided");
        QString artistStr = ((ElidedLabel*)(artist))->text();

        ElidedLabel *album = list->itemWidget(item)->findChild<ElidedLabel *>("album_elided");
        QString albumStr = ((ElidedLabel*)(album))->text();

        OliviaMetaList.append(titleStr+" "+artistStr+"  "+albumStr);
    }
}

void MainWindow::hideListItems(QListWidget *list){
    for(int i = 0 ; i < list->count();i++){
        list->item(i)->setHidden(true);
    }
}
//End Filter Lists///////////////////////////////////////////////////////////////////////////////////////////////////////////////


//Webview dpi set///////////////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::zoomin(){
    zoom = zoom - 1.0;
    setZoom(zoom);
    settingsUi.zoom->setText(QString::number(ui->webview->zoomFactor(),'f',2));
    settingsObj.setValue("zoom",zoom);
}

void MainWindow::zoomout(){
    zoom = zoom + 1.0;
    setZoom(zoom);
    settingsUi.zoom->setText(QString::number(ui->webview->zoomFactor(),'f',2));
    settingsObj.setValue("zoom",zoom);
}

void MainWindow::setZoom(float val){
    ui->webview->setZoomFactor( horizontalDpi / val);
}

void MainWindow::init_miniMode(){
    miniModeWidget = new QWidget(this);
    miniMode_ui.setupUi(miniModeWidget);
    miniModeWidget->setObjectName("miniModeWidget");
    if(settingsObj.value("miniModeStayOnTop","false").toBool()==true){
        miniModeWidget->setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    }else{
        miniModeWidget->setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    }
    miniModeWidget->adjustSize();
}

void MainWindow::on_miniMode_clicked()
{
    if(!miniModeWidget->isVisible())
    {
        ui->miniMode->setIcon(QIcon(":/icons/restore_mini_mode.png"));
        ui->miniMode->setToolTip("Restore back to main window");

        miniMode_ui.nowPlayingLayout->addWidget(ui->nowplaying_widget);

        miniMode_ui.seekSliderLyout->addWidget(ui->position);
        miniMode_ui.seekSliderLyout->addWidget(ui->radioSeekSlider);
        miniMode_ui.seekSliderLyout->addWidget(ui->duration);

        ui->radioVolumeSlider->setMaximumWidth(16777215);
        miniMode_ui.volumeSliderLayout->addWidget(ui->radioVolumeSlider);

        miniMode_ui.controlLayout->addWidget(ui->controls_widget);
        miniModeWidget->move(ui->miniMode->mapToGlobal(QPoint(QPoint(-miniModeWidget->width()+ui->miniMode->width(),30))));
        this->hide();
        miniModeWidget->setMaximumHeight(miniModeWidget->height());

       // miniModeWidget->setWindowOpacity(qreal(95)/100);
        miniModeWidget->setWindowOpacity(qreal(settingsObj.value("miniModeTransperancy","98").toReal()/100));

        miniModeWidget->setStyleSheet ( ui->left_panel->styleSheet().replace("#left_panel","#miniModeWidget"));

        miniModeWidget->showNormal();
    }else{
        //restore
        miniModeWidget->hide();
        ui->miniMode->setToolTip("Switch to Mini Mode");
        ui->radioVolumeSlider->setMaximumWidth(200);
        ui->miniMode->setIcon(QIcon(":/icons/mini_mode.png"));
        ui->left_panel->layout()->addWidget(ui->nowplaying_widget);
        ui->horizontalLayout_11->addWidget(ui->radioVolumeSlider);
        ui->horizontalLayout_11->layout()->addWidget(ui->position);
        ui->horizontalLayout_11->layout()->addWidget(ui->radioSeekSlider);
        ui->horizontalLayout_11->layout()->addWidget(ui->duration);
        ui->centralWidget->layout()->addWidget(ui->controls_widget);
        miniModeWidget->setMaximumHeight(16777215);
        this->show();
    }
}

void MainWindow::on_tabWidget_currentChanged(int index)
{
    Q_UNUSED(index);
    ui->tabWidget->resize(ui->tabWidget->size().width()-1,ui->tabWidget->size().height());
    ui->tabWidget->resize(ui->tabWidget->size().width()+1,ui->tabWidget->size().height());
    settingsObj.setValue("currentQueueTab",index);
}

void MainWindow::on_close_clicked()
{
    this->close();
}

void MainWindow::on_minimize_clicked()
{
    this->setWindowState(Qt::WindowMinimized);
}

void MainWindow::on_maximize_clicked()
{
    if(this->windowState()==Qt::WindowMaximized){
        this->setWindowState(Qt::WindowNoState);
    }else
        this->setWindowState(Qt::WindowMaximized);
}

void MainWindow::on_fullScreen_clicked()
{
    if(this->windowState()==Qt::WindowFullScreen){
        this->setWindowState(Qt::WindowNoState);
    }else
        this->setWindowState(Qt::WindowFullScreen);
}


void MainWindow::reloadREquested(QString dataType,QString query){
    ui->webview->page()->mainFrame()->evaluateJavaScript(dataType+"(\""+query+"\")");
}
