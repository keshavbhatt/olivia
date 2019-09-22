#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QSplitter>
#include <QUrlQuery>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QGraphicsDropShadowEffect>
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
#include "youtube.h"
#include "lyrics.h"
#include "manifest_resolver.h"
#include "utils.h"
#include "plugins/mpris/mprisplugin.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->debug_widget->hide();
    //sets search icon in label
    ui->label_5->resize(ui->label_5->width(),ui->search->height());
    ui->label_5->setPixmap(QPixmap(":/icons/sidebar/search.png").scaled(18,18,Qt::KeepAspectRatio,Qt::SmoothTransformation));

    qApp->setQuitOnLastWindowClosed(true);

    init_app();
    init_webview();
    init_offline_storage();
    init_settings();
    init_miniMode();
    init_lyrics();

    init_downloadWidget();


    QTimer::singleShot(1000, [this]() {
        if(!checkEngine()){
            evoke_engine_check();
            return;
        }else{
            check_engine_updates();
        }
    });

    database = "hjkfdsll";
    store_manager = new store(this,database);

    pagination_manager = new paginator(this);
    connect(pagination_manager,SIGNAL(reloadRequested(QString,QString)),this,SLOT(reloadREquested(QString,QString)));

    loadPlayerQueue();
    init_search_autoComplete();
    init_radio();
    init_videoOption();

    browse();
    installEventFilters();
    loadSettings();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::init_radio(){
    ui->radioVolumeSlider->setMinimum(0);
    ui->radioVolumeSlider->setMaximum(130);
    ui->radioVolumeSlider->setValue(100);
    saveTracksAfterBuffer = settingsObj.value("saveAfterBuffer","true").toBool();
    ui->radioVolumeSlider->setValue(settingsObj.value("volume","100").toInt());
    radio_manager = new radio(this,ui->radioVolumeSlider->value(),saveTracksAfterBuffer);
    connect(radio_manager,SIGNAL(radioProcessReady()),this,SLOT(radioProcessReady()));
    connect(radio_manager,SIGNAL(radioStatus(QString)),this,SLOT(radioStatus(QString)));
    connect(radio_manager,SIGNAL(radioPosition(int)),this,SLOT(radioPosition(int)));
    connect(radio_manager,SIGNAL(radioDuration(int)),this,SLOT(radioDuration(int)));
    connect(radio_manager,SIGNAL(demuxer_cache_duration_changed(double,double)),this,SLOT(radio_demuxer_cache_duration_changed(double,double)));
    connect(radio_manager,SIGNAL(saveTrack(QString)),this,SLOT(saveTrack(QString)));
    connect(radio_manager,&radio::icy_cover_changed,[=](QPixmap pix){
        ui->cover->clear();
        ui->cover->setPixmap(pix.scaled(100,100,Qt::KeepAspectRatio,Qt::SmoothTransformation));
    });
    radio_manager->startRadioProcess(saveTracksAfterBuffer,"",false);
    connect(ui->radioSeekSlider,&seekSlider::setPosition,[=](QPoint localPos){
        ui->radioSeekSlider->blockSignals(true);
        int pos = ui->radioSeekSlider->minimum() + ((ui->radioSeekSlider->maximum()-ui->radioSeekSlider->minimum()) * localPos.x()) / ui->radioSeekSlider->width();

        QPropertyAnimation *a = new QPropertyAnimation(ui->radioSeekSlider,"value");
        a->setDuration(150);
        a->setStartValue(ui->radioSeekSlider->value());
        a->setEndValue(pos);
        a->setEasingCurve(QEasingCurve::InCurve);
        a->start(QPropertyAnimation::DeleteWhenStopped);

        radio_manager->radioSeek(pos);
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
        int pos = ui->radioVolumeSlider->minimum() + ((ui->radioVolumeSlider->maximum()-ui->radioVolumeSlider->minimum()) * localPos.x()) / ui->radioVolumeSlider->width();
        QPropertyAnimation *a = new QPropertyAnimation(ui->radioVolumeSlider,"value");
        a->setDuration(150);
        a->setStartValue(ui->radioVolumeSlider->value());
        a->setEndValue(pos);
        a->setEasingCurve(QEasingCurve::InCurve);
        a->start(QPropertyAnimation::DeleteWhenStopped);
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

    init_mpris();
}

void MainWindow::init_mpris(){
    if(dp==nullptr){
        dp = new MprisPlugin(this);
        connect(dp,SIGNAL(Next()),ui->next,SLOT(click()));
        connect(dp,SIGNAL(PlayPause()),ui->play_pause,SLOT(click()));
        connect(dp,SIGNAL(Play()),ui->play_pause,SLOT(click()));
        connect(dp,SIGNAL(Pause()),ui->play_pause,SLOT(click()));
        connect(dp,SIGNAL(Stop()),ui->stop,SLOT(click()));
        connect(dp,&MprisPlugin::Raise,[=](){
            this->show();
        });

        connect(ui->state,&QLineEdit::textChanged,[=](const QString state){
            QString statusStr;
            if(state=="playing"){
                statusStr = "Playing";
            }else if (state=="paused") {
                statusStr = "Paused";
            }else {
                statusStr = "Stopped";
            }
            dp->playerStatus = statusStr;
            emit dp->PlaybackStatusChanged(statusStr);
        });
    }
}

void MainWindow::installEventFilters(){
    ui->top_widget->installEventFilter(this);
    ui->windowControls->installEventFilter(this);
    ui->label_6->installEventFilter(this);
    ui->nowPlayingGrip->installEventFilter(this);
    ui->next->installEventFilter(this);
    ui->previous->installEventFilter(this);
}

void MainWindow::closeEvent(QCloseEvent *event){
    settingsObj.setValue("geometry",saveGeometry());
    settingsObj.setValue("windowState", saveState());
    settingsObj.setValue("volume",radio_manager->volume);
    settUtils->changeSystemTitlebar(settingsUi.systemTitlebar->isChecked());
    if(ytdlProcess!=nullptr && ytdlQueue.count()>0){
        ytdlQueue.clear();
        ytdlProcess->close();
    }
    radio_manager->quitRadio();
    radio_manager->killRadioProcess();
    radio_manager->deleteLater();

    for(int i=0;i<processIdList.count();i++){
        QProcess::execute("pkill",QStringList()<<"-P"<<QString::number(processIdList.at(i)));
        processIdList.removeAt(i);
    }
    QMainWindow::closeEvent(event);
}


void MainWindow::init_videoOption(){
    if(videoOption == nullptr){
        videoOption = new VideoOption(nullptr,store_manager,radio_manager->used_fifo_file_path);

        videoOption->setWindowTitle(QApplication::applicationName()+" - Video Option");
        videoOption->setWindowFlags(Qt::Window | Qt::WindowCloseButtonHint | Qt::WindowMinimizeButtonHint
                                    | Qt::WindowMaximizeButtonHint | Qt::WindowFullscreenButtonHint);
        videoOption->setWindowModality(Qt::WindowModal);

        connect(videoOption,SIGNAL(downloadRequested(QStringList,QStringList)),this,SLOT(videoOptionDownloadRequested(QStringList,QStringList)));
    }
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

    settingsWidget = new QWidget(nullptr);
    settingsWidget->setObjectName("settingsWidget");
    settingsUi.setupUi(settingsWidget);
    settingsWidget->setWindowFlags(Qt::Dialog);
    settingsWidget->setWindowModality(Qt::ApplicationModal);
    settingsWidget->adjustSize();
    connect(settingsUi.download_engine,SIGNAL(clicked()),this,SLOT(download_engine_clicked()));
    connect(settingsUi.saveAfterBuffer,SIGNAL(toggled(bool)),settUtils,SLOT(changeSaveAfterSetting(bool)));
    connect(settingsUi.showSearchSuggestion,SIGNAL(toggled(bool)),settUtils,SLOT(changeShowSearchSuggestion(bool)));
    connect(settingsUi.dynamicTheme,SIGNAL(toggled(bool)),settUtils,SLOT(changeDynamicTheme(bool)));

    connect(settingsUi.equalizer,&QCheckBox::toggled,[=](bool checked){
       settUtils->changeEqualizerSetting(checked);
       ui->eq->setVisible(checked);
    });

    connect(settingsUi.visualizer,&QCheckBox::toggled,[=](bool checked){
       settUtils->changeVisualizerSetting(checked);
       ui->vis_widget->setVisible(checked);
       if(!nowPlayingSongId.isEmpty()){
           playSongById(QVariant(nowPlayingSongId));
       }
       ui->cover->setVisible(!checked);
    });

    connect(settingsUi.systemTitlebar,&QCheckBox::toggled,[=](bool checked){
        if(checked){
            hide();
            ui->windowControls_main->hide();
            ui->systemTitleBarSpacerWidget->show();
            this->setWindowFlags(Qt::Window | Qt::WindowTitleHint|Qt::WindowSystemMenuHint
                                 |Qt::WindowMinMaxButtonsHint|Qt::WindowCloseButtonHint|Qt::WindowFullscreenButtonHint);
            show();
        }else{
            hide();
            this->setWindowFlags(Qt::FramelessWindowHint | Qt::CustomizeWindowHint | Qt::WindowTitleHint);
            ui->systemTitleBarSpacerWidget->hide();
            ui->windowControls_main->show();
            show();
        }
    });

    //initial settings for app transparency
    settingsUi.appTransperancySlider->setRange(50,100);
    settingsUi.appTransperancyLabel->setText(QString::number(int(this->windowOpacity()*100)));
    settingsUi.appTransperancySlider->setValue(int(this->windowOpacity()*100));
    int labelWidth = settingsUi.appTransperancyLabel->fontMetrics().boundingRect(settingsUi.appTransperancyLabel->text()).width();
    int oneCharWidth = labelWidth/3;
    settingsUi.appTransperancyLabel->setMinimumWidth(oneCharWidth*4);
    settingsUi.transperancyLabel->setMinimumWidth(oneCharWidth*4);

    connect(settingsUi.appTransperancySlider,&QSlider::valueChanged,[=](int val){
        settUtils->changeAppTransperancy(val);
        this->setWindowOpacity(qreal(val)/100);
        settingsUi.appTransperancyLabel->setText(QString::number(settingsUi.appTransperancySlider->value()));
    });
    connect(settingsUi.miniModeTransperancySlider,&QSlider::valueChanged,[=](int val){
        settUtils->changeMiniModeTransperancy(val);
        settingsUi.transperancyLabel->setText(QString::number(settingsUi.miniModeTransperancySlider->value()));
    });
    connect(settingsUi.miniModeStayOnTop,&QCheckBox::toggled,[=](bool checked){
        settUtils->changeMiniModeStayOnTop(checked);
        miniModeWidget->deleteLater();
        init_miniMode();
    });

    connect(settingsUi.delete_offline_pages,&QPushButton::clicked,[=](){
        utils* util = new utils(this);
        if(util->delete_cache(setting_path+"/paginator")){
            settingsUi.offline_pages_size->setText(util->refreshCacheSize(setting_path+"/paginator"));
            util->deleteLater();
        }
    });
    connect(settingsUi.donate,&QPushButton::clicked,[=](){
        settingsWidget->close();
        ui->webview->load(QUrl("https://paypal.me/keshavnrj/5"));
    });
    connect(settingsUi.delete_tracks_cache,&QPushButton::clicked,[=](){
          QMessageBox msgBox;
          msgBox.setText("This will delete all downloaded songs!");
          msgBox.setIcon(QMessageBox::Information);
          msgBox.setInformativeText("Delete all downloaded songs cache ?");
          msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
          msgBox.setDefaultButton(QMessageBox::No);
          int ret = msgBox.exec();
          switch (ret) {
            case QMessageBox::Yes:{
                  utils* util = new utils(this);

                      store_manager->delete_track_cache(setting_path+"/downloadedTracks");
                      settingsUi.cached_tracks_size->setText(util->refreshCacheSize(setting_path+"/downloadedTracks"));
                      util->deleteLater();
                      clear_queue();
                      loadPlayerQueue();
              break;}
            case  QMessageBox::No:
              break;
          }
    });

    connect(settingsUi.drop_database,&QPushButton::clicked,[=](){
          QMessageBox msgBox;
          msgBox.setText("This will clear your local database !!");
          msgBox.setIcon(QMessageBox::Information);
          msgBox.setInformativeText("Delete local database ? \n\nThis only include database, downloaded songs cache will remain.");
          msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
          msgBox.setDefaultButton(QMessageBox::No);
          int ret = msgBox.exec();
          switch (ret) {
            case QMessageBox::Yes:{
                  utils* util = new utils(this);
                  if(util->delete_cache(setting_path+"/storeDatabase/"+database)){
                      settingsUi.database_size->setText(util->refreshCacheSize(setting_path+"/storeDatabase/"+database));
                      util->deleteLater();
                      clear_queue();
                      restart_required();
                  }
              break;}
            case  QMessageBox::No:
              break;
          }
    });

    connect(settingsUi.plus,SIGNAL(clicked(bool)),this,SLOT(zoomin()));
    connect(settingsUi.minus,SIGNAL(clicked(bool)),this,SLOT(zoomout()));

    settingsUi.zoom->setText(QString::number(ui->webview->zoomFactor(),'f',2));
    add_colors_to_color_widget();

    //show hide eq
    ui->eq->setVisible(settingsObj.value("equalizer").toBool());

    //show hide vis
    ui->cover->setVisible(!settingsObj.value("visualizer").toBool());
    ui->vis_widget->setVisible(settingsObj.value("visualizer").toBool());
}

void MainWindow::restart_required(){
    QMessageBox msgBox;
    msgBox.setText("Application need to restart!");
    msgBox.setIcon(QMessageBox::Information);
    msgBox.setInformativeText("Please restart application for new changes.");
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.setDefaultButton(QMessageBox::Ok);
    msgBox.exec();
    qApp->quit();
}

void MainWindow::dynamicThemeChanged(bool enabled){
    if(enabled){
        QString color = store_manager->getDominantColor(store_manager->getAlbumId(nowPlayingSongId));
        int r,g,b;
        if(color.split(",").count()>2){
            r = color.split(",").at(0).toInt();
            g = color.split(",").at(1).toInt();
            b = color.split(",").at(2).toInt();
            set_app_theme(QColor(r,g,b));
           // qDebug()<<QColor(r,g,b);
        }

    }
    settingsUi.themesWidget->setEnabled(!enabled);
}

void MainWindow::loadSettings(){

    ui->shuffle->setChecked(settingsObj.value("shuffle").toBool());
    ui->shuffle->setIcon(QIcon(settingsObj.value("shuffle").toBool()?":/icons/shuffle_button.png":":/icons/shuffle_button_disabled.png"));

    settingsUi.saveAfterBuffer->setChecked(settingsObj.value("saveAfterBuffer","true").toBool());
    settingsUi.showSearchSuggestion->setChecked(settingsObj.value("showSearchSuggestion","true").toBool());
    settingsUi.miniModeStayOnTop->setChecked(settingsObj.value("miniModeStayOnTop","false").toBool());
    settingsUi.equalizer->setChecked(settingsObj.value("equalizer","false").toBool());
    settingsUi.visualizer->setChecked(settingsObj.value("visualizer","false").toBool());


    settingsUi.dynamicTheme->setChecked(settingsObj.value("dynamicTheme","false").toBool());

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
    }else{
        QString rgbhash = settingsObj.value("customTheme","#3BBAC6").toString();
        set_app_theme(QColor(rgbhash));
    }

    ui->tabWidget->setCurrentIndex(settingsObj.value("currentQueueTab","0").toInt());
    restoreGeometry(settingsObj.value("geometry").toByteArray());
    restoreState(settingsObj.value("windowState").toByteArray());

    settingsUi.systemTitlebar->setChecked(settingsObj.value("systemTitlebar","true").toBool());
    //load the system titlebar settings if its set to false, this is not following the connection we already having cause
    // toggle is not being triggered by default on checkbox due to its initial false state.
    if(!settingsObj.value("systemTitlebar","true").toBool()){
        this->setWindowFlags(Qt::FramelessWindowHint | Qt::CustomizeWindowHint | Qt::WindowTitleHint);
        ui->windowControls_main->show();
        ui->systemTitleBarSpacerWidget->hide();
    }
}

void MainWindow::add_colors_to_color_widget(){

        color_list<<"fakeitem"<<"#FF0034"<<"#2A82DA"<<"#029013"
                        <<"#D22298"<<"#FF901F"<<"#565655"
                        <<"#2B2929"<<"#E95420"<<"#6C2164";

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
                static_cast<QGridLayout*>(layout)->addWidget(pb, row, f2);
            }
            row++;
        }
        QPushButton *pb =new QPushButton();
        pb->setIcon(QIcon(":/icons/picker.png"));
        pb->setIconSize(QSize(22,22));
        pb->setObjectName("custom_color");
        pb->setText("Select color");
        pb->setToolTip("Choose custom color from color dialog.");
        connect(pb,SIGNAL(clicked(bool)),this,SLOT(customColor()));
        static_cast<QGridLayout*>(layout)->addWidget(pb, row+1, 0);
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

    this->setStyleSheet("QMainWindow{"
                            "background-color:rgba("+r+","+g+","+b+","+"0.1"+");"
                            "background-image:url(:/icons/texture.png), linear-gradient(hsla(0,0%,32%,.99), hsla(0,0%,27%,.95));"
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
    ui->tabWidget->setStyleSheet("QTabWidget#tabWidget{"+widgetStyle+"}"); //to remove style set by designer

    ui->stream_info->setStyleSheet("QWidget#stream_info{"+widgetStyle+"}"); //to remove style set by designer

   QString btn_style ="QPushButton{color: silver; background-color: #45443F; border:1px solid #272727; padding-top: 3px; padding-bottom: 3px; padding-left: 3px; padding-right: 3px; border-radius: 2px; outline: none;}"
   "QPushButton:disabled { background-color: #45443F; border:1px solid #272727; padding-top: 3px; padding-bottom: 3px; padding-left: 5px; padding-right: 5px; /*border-radius: 2px;*/ color: #636363;}"
   "QPushButton:hover{border: 1px solid #272727;background-color:#5A584F; color:silver ;}"
   "QPushButton:pressed {background-color: #45443F;color: silver;padding-bottom:1px;}";

    ui->ytdlRefreshAll->setStyleSheet(btn_style);
    ui->ytdlStopAll->setStyleSheet(btn_style);

    if(eq != nullptr){
        eq->setStyleSheet("QWidget#equalizer{"+ui->search->styleSheet()+"}"
                                         +"QFrame{"+ui->search->styleSheet()+"}");
        eq->removeStyle();
    }

    settingsUi.download_engine->setStyleSheet(btn_style);
    settingsUi.plus->setStyleSheet(btn_style);
    settingsUi.minus->setStyleSheet(btn_style);
    settingsUi.delete_offline_pages->setStyleSheet(btn_style);
    settingsUi.delete_tracks_cache->setStyleSheet(btn_style);
    settingsUi.drop_database->setStyleSheet(btn_style);
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
    //init youtube class
    youtube = new Youtube(this);
    connect(youtube,SIGNAL(setCountry(QString)),this,SLOT(setCountry(QString)));

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


    ElidedLabel *title = new ElidedLabel("-",nullptr);
    ElidedLabel *artist = new ElidedLabel("-",nullptr);
    ElidedLabel *album = new ElidedLabel("-",nullptr);

    title->setObjectName("nowP_title");
    album->setObjectName("nowP_album");
    artist->setObjectName("nowP_artist");

    title->setAlignment(Qt::AlignHCenter);
    album->setAlignment(Qt::AlignHCenter);
    artist->setAlignment(Qt::AlignHCenter);

    ui->title_horizontalLayout->addWidget(title);
    ui->artist_horizontalLayout->addWidget(artist);
    ui->album_horizontalLayout->addWidget(album);

    QString btn_style_2= "QPushButton{background-color:transparent ;border:0px;}"
                         "QPushButton:disabled { background-color: transparent; border-bottom:1px solid #727578;"
                         "padding-top: 3px; padding-bottom: 3px; padding-left: 5px; padding-right: 5px;color: #636363;}"
                         "QPushButton:pressed {padding-bottom:0px;background-color:transparent;border:0px;}"
                         "QPushButton:hover {border:none;padding-bottom:1px;background-color:transparent;border:0px;}";

    foreach (QPushButton *button, ui->windowControls->findChildren<QPushButton*>()) {
        button->setStyleSheet(btn_style_2);
    }

     QString btn_style_3 = "QPushButton{background-color:transparent ;border:0px;padding-top: 3px; padding-bottom: 3px;}"
                           "QPushButton:disabled {padding-top: 3px; padding-bottom:3px;}"
                           "QPushButton:pressed {padding-bottom:0px;border:0px;}"
                           "QPushButton:hover {padding-bottom:1px;background-color:transparent;border:0px;}";

     foreach (QPushButton *button, ui->controls_widget->findChildren<QPushButton*>()) {
         button->setStyleSheet(btn_style_3);
     }
     ui->state->hide();
     connect(ui->state,&QLineEdit::textChanged,[=](){
         utils* util = new utils(this);
         ui->visible_state->setTextFormat(Qt::RichText);
         ui->visible_state->setText("<p align=\"center\" ><span style=\"font-size:11pt;\">"+util->toCamelCase(ui->state->text())+"</span></p>");
         util->deleteLater();
     });
}

//used to set county settings in youtube right now
void MainWindow::setCountry(QString country){
    if(pageType=="youtube"){
        ui->webview->page()->mainFrame()->findFirstElement("#currentCountry").setInnerXml(country);
        ui->webview->page()->mainFrame()->findFirstElement("#home").evaluateJavaScript("this.click()");
    }
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

    #ifdef QT_DEBUG
      QWebSettings::globalSettings()->setAttribute(QWebSettings::DeveloperExtrasEnabled, true);
    #else
      QWebSettings::globalSettings()->setAttribute(QWebSettings::DeveloperExtrasEnabled, false);
      ui->webview->setContextMenuPolicy(Qt::NoContextMenu);
    #endif

    ui->webview->settings()->setAttribute(QWebSettings::LocalStorageEnabled, true);
    ui->webview->settings()->enablePersistentStorage(setting_path);

    QWebSettings::globalSettings()->setAttribute(QWebSettings::JavascriptEnabled, true);
    QWebSettings::globalSettings()->setAttribute(QWebSettings::PluginsEnabled, false);
    ui->webview->page()->settings()->setMaximumPagesInCache(0);
    ui->webview->page()->settings()->setAttribute(QWebSettings::PluginsEnabled, false);
    QNetworkDiskCache* diskCache = new QNetworkDiskCache(this);
    diskCache->setCacheDirectory(setting_path);
    ui->webview->page()->networkAccessManager()->setCache(diskCache);
    ui->webview->page()->networkAccessManager()->setCookieJar(new CookieJar(cookieJarPath, ui->webview->page()->networkAccessManager()));

    horizontalDpi = QApplication::desktop()->screen()->logicalDpiX();

    if(!settingsObj.value("zoom").isValid()){
        zoom = 100.0;
        setZoom(zoom);
    }else{
        zoom =  settingsObj.value("zoom","100.0").toReal();
        setZoom(zoom);
    }
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
    QDir downloadedVideo(setting_path+"/downloadedVideos");
    if(!downloadedVideo.exists()){
        if(downloadedVideo.mkdir(downloadedVideo.path())){
            qDebug()<<"created downloadedVideos dir";
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
    foreach (QStringList trackMetaList, store_manager->getPlayerQueue()) {
        QString id,title,artist,album,base64,dominantColor,songId,albumId,artistId,url;
        songId = trackMetaList.at(0);
        title = trackMetaList.at(1);
        albumId = trackMetaList.at(2);
        album = trackMetaList.at(3);
        artistId = trackMetaList.at(4);
        artist = trackMetaList.at(5);
        base64 = trackMetaList.at(6);
        url = trackMetaList.at(7);
        id = trackMetaList.at(8);
        dominantColor = trackMetaList.at(9);

        QTextDocument text;
        text.setHtml(title);
        QString plainTitle = text.toPlainText();

        QWidget *track_widget = new QWidget(ui->right_list);
        track_widget->setToolTip(title);
        track_widget->setObjectName("track-widget-"+songId);
        track_ui.setupUi(track_widget);

        QFont font("Ubuntu");
        font.setPixelSize(12);

        ElidedLabel *titleLabel = new ElidedLabel(plainTitle,nullptr);
        titleLabel->setFont(font);
        titleLabel->setObjectName("title_elided");
        track_ui.verticalLayout_2->addWidget(titleLabel);

        ElidedLabel *artistLabel = new ElidedLabel(artist,nullptr);
        artistLabel->setObjectName("artist_elided");
        artistLabel->setFont(font);
        track_ui.verticalLayout_2->addWidget(artistLabel);

        ElidedLabel *albumLabel = new ElidedLabel(album,nullptr);
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
            track_ui.cover->setMaximumWidth(static_cast<int>(track_widget->height()*1.15));

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
            item->setData(playerQueue::IdentifierRole,true);
             ui->right_list->addItem(item,true);
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
    if ((obj == ui->nowPlayingGrip || obj == ui->top_widget  || obj == ui->windowControls || obj == ui->label_6 ) && (event->type() == QEvent::MouseMove)) {
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

    if (obj == ui->nowPlayingGrip || obj == ui->top_widget  || obj == ui->windowControls || obj == ui->label_6){
        if (event->type() == QEvent::MouseButtonPress){
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
    }

    if (obj ==  ui->top_widget || obj == ui->label_6) {
        if(event->type() == QEvent::MouseButtonDblClick){
            const QMouseEvent* const me = static_cast<const QMouseEvent*>( event );
            if (me->button() == Qt::LeftButton) {
                if(this->isMaximized()){
                    this->setWindowState(Qt::WindowNoState);
                }else{
                    this->setWindowState(Qt::WindowMaximized);
                }
                 event->accept();
            }
            return true;
        }
    }

    if(obj==ui->search){
        return false;
    }
        return obj->eventFilter(obj, event);
}



void MainWindow::init_search_autoComplete(){
    _onlineSearchSuggestion_ = new onlineSearchSuggestion(ui->search);
    ui->search->installEventFilter(_onlineSearchSuggestion_);
}


void MainWindow::setPlayerPosition(qint64 position){

    int seconds = (position/1000) % 60;
    int minutes = (position/60000) % 60;
    int hours = (position/3600000) % 24;
    QTime time(hours, minutes,seconds);
    ui->position->setText(time.toString());

    ui->radioSeekSlider->setValue(static_cast<int>(position));
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
    radio_manager->radioSeek(position);
//    ui->radioSeekSlider->setSliderPosition(position);
    ui->radioSeekSlider->blockSignals(false);
}

void MainWindow::on_stop_clicked()
{
    radio_manager->stop();
}


void MainWindow::webViewLoaded(bool loaded){

    if(loaded){
        ui->webview->page()->mainFrame()->addToJavaScriptWindowObject(QString("mainwindow"),  this);
        ui->webview->page()->mainFrame()->addToJavaScriptWindowObject(QString("paginator"), pagination_manager);
        ui->webview->page()->mainFrame()->evaluateJavaScript("changeBg('"+themeColor+"')");
        if(!nowPlayingSongId.isEmpty() && pageType!="browse"){
            ui->webview->page()->mainFrame()->evaluateJavaScript("NowPlayingTrackId='"+nowPlayingSongId+"'");
            ui->webview->page()->mainFrame()->evaluateJavaScript("setNowPlaying('"+nowPlayingSongId+"')");
        }
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
    if(pageType=="local_saved_videos"){
        ui->webview->page()->mainFrame()->addToJavaScriptWindowObject(QString("store"), store_manager);
        ui->webview->page()->mainFrame()->evaluateJavaScript("open_local_saved_videos();");
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
        qDebug()<<"radio";
        ui->webview->page()->mainFrame()->addToJavaScriptWindowObject(QString("store"), store_manager);
        ui->webview->page()->mainFrame()->evaluateJavaScript("loadTopStations('most-played');");
    }

    if(pageType=="goto_album"){
        leftListChangeCurrentRow(7);
        ui->webview->page()->mainFrame()->addToJavaScriptWindowObject(QString("mainwindow"), this);
        ui->webview->page()->mainFrame()->evaluateJavaScript("album_view('"+gotoAlbumId+"')");
    }

    if(pageType=="goto_artist"){
        leftListChangeCurrentRow(8);
        ui->webview->page()->mainFrame()->addToJavaScriptWindowObject(QString("mainwindow"), this);
        ui->webview->page()->mainFrame()->evaluateJavaScript("artist_view('"+gotoArtistId+"')");
    }

    if(pageType=="goto_youtube_channel"){
        leftListChangeCurrentRow(15);
        ui->webview->page()->mainFrame()->addToJavaScriptWindowObject(QString("youtube"),  youtube);
        ui->webview->page()->mainFrame()->addToJavaScriptWindowObject(QString("mainwindow"), this);
        ui->webview->page()->mainFrame()->evaluateJavaScript("get_channel('"+youtubeVideoId+"')");
    }

    if(pageType=="goto_youtube_recommendation"){
        leftListChangeCurrentRow(15);
        if(!youtubeVideoId.isEmpty()){

            ui->webview->page()->mainFrame()->addToJavaScriptWindowObject(QString("youtube"),  youtube);
            ui->webview->page()->mainFrame()->addToJavaScriptWindowObject(QString("mainwindow"), this);
            QString trackTitle = QString(store_manager->getTrack(youtubeVideoId).at(1)).remove("'").remove("\"");
            youtubeVideoId = store_manager->getYoutubeIds(youtubeVideoId).split("<br>").first().trimmed();
            ui->webview->page()->mainFrame()->evaluateJavaScript("show_related('"+youtubeVideoId+"','"+trackTitle+"')");
        }
        youtubeVideoId.clear();
    }

    if( loaded && pageType == "recommendation"){
        leftListChangeCurrentRow(2);
        ui->webview->page()->mainFrame()->addToJavaScriptWindowObject(QString("youtube"),  youtube);

        //if the function is called from track action menu
        if(!recommendationSongId.isEmpty()){
            QString trackTitle = store_manager->getTrack(recommendationSongId).at(1);
            ui->webview->page()->mainFrame()->evaluateJavaScript("$('#manual_search').val('"+trackTitle+"');setPlaylistBaseId('"+recommendationSongId+"');");
        }
        recommendationSongId.clear();
     }

    if( loaded && pageType == "youtube" && !youtubeSearchTerm.isEmpty()){
        ui->webview->page()->mainFrame()->addToJavaScriptWindowObject(QString("youtube"),  youtube);
        ui->webview->page()->mainFrame()->evaluateJavaScript("$('.ui-content').fadeOut('fast');$('#manual_search').val('"+youtubeSearchTerm+"');manual_youtube_search('"+youtubeSearchTerm+"');");
        youtubeSearchTerm.clear();
    }
    if( loaded && pageType == "youtube" && youtubeSearchTerm.isEmpty()){
        ui->webview->page()->mainFrame()->addToJavaScriptWindowObject(QString("youtube"),  youtube);
        ui->webview->page()->mainFrame()->evaluateJavaScript("load_history();");
        youtubeSearchTerm.clear();
    }

    if(pageType=="search"){
        if(!ui->search->text().isEmpty() && loaded && !offsetstr.contains("offset")){
            ui->left_list->setCurrentRow(3);
            QString term = ui->search->text();
            term = term.replace(" ","+");
            term = term.replace("'","%27");
            search(term);
            isLoadingResults=false;
        }else{
            isLoadingResults = false;
        }
    }
}

void MainWindow::setSearchTermAndOpenYoutube(QVariant term){
    youtubeSearchTerm = term.toString();
    ui->left_list->setCurrentRow(15); //set youtube page
}

void MainWindow::resultLoaded(){
  isLoadingResults =false;
}

//returns search query to webend
QString MainWindow::getTerm(){
    QString term = ui->search->text();
    return  term.replace(" ","+");
}

void MainWindow::addToQueue(QString id,QString title,
                            QString artist,QString album,QString base64,
                            QString dominantColor,QString songId,QString albumId,QString artistId){

    id = id.remove("<br>");
    QString setting_path =  QStandardPaths::writableLocation(QStandardPaths::DataLocation);

    if(store_manager->isInQueue(songId)){
        ui->console->append("Song - "+songId+" Already in queue");
        QWidget *listWidgetItem = ui->right_list_2->findChild<QWidget*>("track-widget-"+songId);
        if(listWidgetItem!=nullptr){
            QListWidget * listWidget =  ui->right_list_2;
            for (int i = 0; i < listWidget->count(); ++i) {
                if(listWidget->itemWidget(listWidget->item(i))->objectName()==listWidgetItem->objectName()){
                    listWidget->setCurrentItem(listWidget->item(i));
                    ui->right_list_2->scrollToItem(ui->right_list_2->item(i));
                    break;
                }
            }
        }else{
            QWidget *listWidgetItem = ui->right_list->findChild<QWidget*>("track-widget-"+songId);
            QListWidget * listWidget =  ui->right_list;
            for (int i = 0; i < listWidget->count(); ++i) {
                if(listWidget->itemWidget(listWidget->item(i))->objectName()==listWidgetItem->objectName()){
                    listWidget->setCurrentItem(listWidget->item(i));
                    ui->right_list->scrollToItem(ui->right_list->item(i));
                    break;
                }
            }
        }
        return;
    }else{
        QWidget *track_widget = new QWidget(ui->right_list);
        track_widget->setToolTip(title);
        track_widget->setObjectName("track-widget-"+songId);
        track_ui.setupUi(track_widget);

        QFont font("Ubuntu");
        font.setPixelSize(12);
        setFont(font);

        //to convert html sequence to plaintext
        QTextDocument text;
        text.setHtml(htmlToPlainText(title));
        QString plainTitle = text.toPlainText();



        ElidedLabel *titleLabel = new ElidedLabel(plainTitle,nullptr);
        titleLabel->setFont(font);
        titleLabel->setObjectName("title_elided");
        track_ui.verticalLayout_2->addWidget(titleLabel);

        ElidedLabel *artistLabel = new ElidedLabel(htmlToPlainText(artist),nullptr);
        artistLabel->setObjectName("artist_elided");
        artistLabel->setFont(font);
        track_ui.verticalLayout_2->addWidget(artistLabel);

        ElidedLabel *albumLabel = new ElidedLabel(htmlToPlainText(album),nullptr);
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
            track_ui.cover->setMaximumWidth(static_cast<int>(track_widget->height()*1.15));
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
              //reverse the process queue, so that recently added song can get first chance to process
              if(ytdlQueue.count()>1){
                  ytdlQueue.insert(1,  ytdlQueue.takeAt(ytdlQueue.count()-1));
              }
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
              //reverse the process queue, so that recently added song can get first chance to process
              if(ytdlQueue.count()>1){
                  ytdlQueue.insert(1,  ytdlQueue.takeAt(ytdlQueue.count()-1));
              }
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
    QString albumId = store_manager->getAlbumId(songId);
    QString artistId = store_manager->getArtistId(songId);

    QAction *showRecommendation = new QAction("Spotify Recommendations",nullptr);
    QAction *watchVideo = new QAction("Watch Video",nullptr);
    QAction *showLyrics = new QAction("Show Lyrics",nullptr);
    QAction *openChannel = new QAction("Open Channel",nullptr);
    QAction *youtubeShowRecommendation= new QAction("Youtube Recommendations",nullptr);
    QAction *gotoArtist= new QAction("Go to Artist",nullptr);
    QAction *gotoAlbum = new QAction("Go to Album",nullptr);   
    QAction *removeSong = new QAction("Remove from queue",nullptr);
    QAction *deleteSongCache = new QAction("Delete song cache",nullptr);
    deleteSongCache->setEnabled(store_manager->isDownloaded(songId));

    //setIcons
    youtubeShowRecommendation->setIcon(QIcon(":/icons/sidebar/youtube.png"));
    showRecommendation->setIcon(QIcon(":/icons/sidebar/spotify.png"));
    watchVideo->setIcon(QIcon(":/icons/sidebar/video.png"));
    gotoArtist->setIcon(QIcon(":/icons/sidebar/artist.png"));
    gotoAlbum->setIcon(QIcon(":/icons/sidebar/album.png"));
    showLyrics->setIcon(QIcon(":/icons/sidebar/playlist.png"));
    openChannel->setIcon(QIcon(":/icons/sidebar/artist.png"));
    removeSong->setIcon(QIcon(":/icons/sidebar/remove.png"));
    deleteSongCache->setIcon(QIcon(":/icons/sidebar/delete.png"));


    connect(showLyrics,&QAction::triggered,[=](){
        QString songTitle, artistTitle,lyrics_search_string;
        songTitle = store_manager->getTrack(songId).at(1);
       if(store_manager->getAlbum(albumId).contains("undefined")){
            lyrics_search_string = songTitle;
       }else{
            artistTitle = store_manager->getArtist(artistId);
            lyrics_search_string = songTitle +" - "+  artistTitle;
       }
       lyricsWidget->setStyleSheet("");
       lyricsWidget->setCustomStyle(ui->search->styleSheet(),ui->right_list->styleSheet(),this->styleSheet());
       lyricsWidget->setQueryString(htmlToPlainText(lyrics_search_string));
    });


    connect(showRecommendation,&QAction::triggered,[=](){
        ui->webview->load(QUrl("qrc:///web/recommendation/recommendation.html"));
        pageType = "recommendation";
        recommendationSongId = songId;
    });

    connect(youtubeShowRecommendation,&QAction::triggered,[=](){
        ui->webview->load(QUrl("qrc:///web/youtube/youtube.html"));
        pageType = "goto_youtube_recommendation";
        youtubeVideoId = songId;
    });

    connect(watchVideo,&QAction::triggered,[=](){
        if(videoOption==nullptr){
            init_videoOption();
        }else{
            QString btn_style ="QPushButton{color: silver; background-color: #45443F; border:1px solid #272727; padding-top: 3px; padding-bottom: 3px; padding-left: 3px; padding-right: 3px; border-radius: 2px; outline: none;}"
            "QPushButton:disabled { background-color: #45443F; border:1px solid #272727; padding-top: 3px; padding-bottom: 3px; padding-left: 5px; padding-right: 5px; /*border-radius: 2px;*/ color: #636363;}"
            "QPushButton:hover{border: 1px solid #272727;background-color:#5A584F; color:silver ;}"
            "QPushButton:pressed {background-color: #45443F;color: silver;padding-bottom:1px;}";

            videoOption->setStyleSheet("");
            videoOption->setStyleSheet("QWidget#VideoOption{"+ui->search->styleSheet()+"}"
                                             +"QFrame{"+ui->search->styleSheet()+"}"
                                             +btn_style);
            videoOption->setMeta(songId);
            videoOption->removeStyle();
            videoOption->adjustSize();
            videoOption->show();
        }
    });

    connect(openChannel,&QAction::triggered,[=](){
        ui->webview->load(QUrl("qrc:///web/youtube/youtube.html"));
        pageType = "goto_youtube_channel";
        youtubeVideoId = songId;
    });

    connect(gotoAlbum,&QAction::triggered,[=](){
        ui->webview->load(QUrl("qrc:///web/goto/album.html"));
        pageType = "goto_album";
        gotoAlbumId = albumId;
    });

    connect(gotoArtist,&QAction::triggered,[=](){
        ui->webview->load(QUrl("qrc:///web/goto/artist.html"));
        pageType = "goto_artist";
        gotoArtistId = artistId;
    });

    connect(deleteSongCache,&QAction::triggered,[=](){
        delete_song_cache(songId);
    });

    connect(removeSong,&QAction::triggered,[=](){
        remove_song(songId);
    });

    QMenu menu;
    if(!albumId.contains("undefined")){// do not add gotoalbum and gotoartist actions to youtube streams
        menu.addAction(showLyrics);
        menu.addAction(watchVideo);
        if(!isNumericStr(songId)) //spotify song ids are not numeric
        {
            menu.addAction(showRecommendation);
        }
        //added youtube recommendation fallback for itunes tracks ids
        menu.addAction(youtubeShowRecommendation);
        menu.addAction(gotoAlbum);
        menu.addAction(gotoArtist);
    }else{
        menu.addSeparator();
        menu.addAction(showLyrics);
        menu.addAction(openChannel);
        menu.addAction(watchVideo);
        menu.addAction(youtubeShowRecommendation);
    }
    menu.addSeparator();
    menu.addAction(removeSong);
    menu.addSeparator();
    menu.addAction(deleteSongCache);
    menu.setStyleSheet(menuStyle());
    menu.exec(QCursor::pos());
}

void MainWindow::remove_song(QVariant track_id){
    QString songId = track_id.toString().remove("<br>").trimmed();
    for (int i= 0;i<ui->right_list->count();i++) {
       QString songIdFromWidget = static_cast<QLineEdit*>(ui->right_list->itemWidget(ui->right_list->item(i))->findChild<QLineEdit*>("songId"))->text().trimmed();
        if(songId==songIdFromWidget){
            ui->right_list->takeItem(i);
            store_manager->removeFromQueue(songId);
            break;
        }
    }
    for (int i= 0;i<ui->right_list_2->count();i++) {
       QString songIdFromWidget = static_cast<QLineEdit*>(ui->right_list_2->itemWidget(ui->right_list_2->item(i))->findChild<QLineEdit*>("songId"))->text().trimmed();
        if(songId==songIdFromWidget){
            ui->right_list_2->takeItem(i);
            store_manager->removeFromQueue(songId);
            break;
        }
    }
}

void MainWindow::delete_song_cache(QVariant track_id){
    QString songId = track_id.toString().remove("<br>").trimmed();
    QString setting_path =  QStandardPaths::writableLocation(QStandardPaths::DataLocation);
    QFile cache(setting_path+"/downloadedTracks/"+songId);
    cache.remove();
    store_manager->update_track("downloaded",songId,"0");
    for (int i= 0;i<ui->right_list->count();i++) {
       QString songIdFromWidget = static_cast<QLineEdit*>(ui->right_list->itemWidget(ui->right_list->item(i))->findChild<QLineEdit*>("songId"))->text().trimmed();
        if(songId==songIdFromWidget){
            ui->right_list->itemWidget(ui->right_list->item(i))->findChild<QLabel*>("offline")->setPixmap(QPixmap(":/icons/blank.png"));
            ui->right_list->itemWidget(ui->right_list->item(i))->findChild<QLineEdit*>("url")->setText(store_manager->getOfflineUrl(songId));
            if(store_manager->getExpiry(songId)){
                ui->right_list->itemWidget(ui->right_list->item(i))->setEnabled(false);
                getAudioStream(store_manager->getYoutubeIds(songId),songId);
            }
            break;
        }
    }
    for (int i= 0;i<ui->right_list_2->count();i++) {
       QString songIdFromWidget = static_cast<QLineEdit*>(ui->right_list_2->itemWidget(ui->right_list_2->item(i))->findChild<QLineEdit*>("songId"))->text().trimmed();
        if(songId==songIdFromWidget){
            ui->right_list_2->itemWidget(ui->right_list_2->item(i))->findChild<QLabel*>("offline")->setPixmap(QPixmap(":/icons/blank.png"));
            ui->right_list_2->itemWidget(ui->right_list_2->item(i))->findChild<QLineEdit*>("url")->setText(store_manager->getOfflineUrl(songId));
            if(store_manager->getExpiry(songId)){
                ui->right_list_2->itemWidget(ui->right_list_2->item(i))->setEnabled(false);
                getAudioStream(store_manager->getYoutubeIds(songId),songId);
            }
            break;
        }
    }
}

void MainWindow::web_watch_video(QVariant data){
    QString btn_style ="QPushButton{color: silver; background-color: #45443F; border:1px solid #272727; padding-top: 3px; padding-bottom: 3px; padding-left: 3px; padding-right: 3px; border-radius: 2px; outline: none;}"
    "QPushButton:disabled { background-color: #45443F; border:1px solid #272727; padding-top: 3px; padding-bottom: 3px; padding-left: 5px; padding-right: 5px; /*border-radius: 2px;*/ color: #636363;}"
    "QPushButton:hover{border: 1px solid #272727;background-color:#5A584F; color:silver ;}"
    "QPushButton:pressed {background-color: #45443F;color: silver;padding-bottom:1px;}";

    videoOption->setStyleSheet("");
    videoOption->setStyleSheet("QWidget#VideoOption{"+ui->search->styleSheet()+"}"
                                     +"QFrame{"+ui->search->styleSheet()+"}"
                                     +btn_style);

    videoOption->setMetaFromWeb(data);
    videoOption->removeStyle();
    videoOption->adjustSize();
    videoOption->show();
}


void MainWindow::getAudioStream(QString ytIds,QString songId){

    if(!checkEngine())
        return;

    ytdlQueue.append(QStringList()<<ytIds<<songId);

    //update ytdlQueueLabel
    ui->ytdlQueueLabel->setText("Processing "+QString::number(ytdlQueue.count())+" tracks..");


    if(ytdlProcess==nullptr && ytdlQueue.count()>0){
        processYtdlQueue();
    }
}

void MainWindow::processYtdlQueue(){

    if(ytdlQueue.count()>0){

        //update ytdlQueueLabel
        ui->ytdlQueueLabel->setText("Processing "+QString::number(ytdlQueue.count())+" tracks..");

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
                ytdlProcess->start("python",QStringList()<<addin_path+"/core"<<"--force-ipv4"<<"--get-url" <<"-i"<< "--extract-audio"<<urlsFinal);
                ytdlProcess->waitForStarted();
                connect(ytdlProcess,SIGNAL(readyRead()),this,SLOT(ytdlReadyRead()));
                connect(ytdlProcess,SIGNAL(finished(int)),this,SLOT(ytdlFinished(int)));
        }
    }else{
        ui->ytdlQueueLabel->setText("no task");
    }

    //update stream info buttons
    if(ytdlProcess!=nullptr){
        ui->ytdlStopAll->setEnabled(true);
        ui->ytdlRefreshAll->setEnabled(false);
    }
}

void MainWindow::ytdlFinished(int code){
    Q_UNUSED(code);
    ytdlProcess->close();
    ytdlProcess = nullptr;

    if(ytdlQueue.count()>0){
        ui->ytdlQueueLabel->setText("Processing "+QString::number(ytdlQueue.count())+" tracks..");
        processYtdlQueue();
    }else{
        ui->ytdlQueueLabel->setText("no task");
    }
    //update stream info buttons
    if(ytdlProcess==nullptr){
        ui->ytdlStopAll->setEnabled(false);
        ui->ytdlRefreshAll->setEnabled(true);
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
            QString listName;
            QListWidget *listWidget;
            QWidget *listWidgetItem = ui->right_list->findChild<QWidget*>("track-widget-"+songId);
            listName = "olivia";
            listWidget = ui->right_list;
            if(listWidgetItem==nullptr){
                listWidgetItem= ui->right_list_2->findChild<QWidget*>("track-widget-"+songId);
                listName = "youtube";
                listWidget = ui->right_list_2;
            }
            //check if listWidgetItem found in one of list
            if(listWidgetItem==nullptr){
                qDebug()<<"TRACK NOT FOUND IN LIST";
                listName = "";
                QProcess* senderProcess = qobject_cast<QProcess*>(sender());
                senderProcess->close();
                if(senderProcess != nullptr)
                senderProcess->deleteLater();
            }else{
                //algo to assign next/previous song if current processed track is next to currently playing track.
                int row;
                if(!listName.isEmpty()){
                    for(int i=0;i<listWidget->count();i++){
                        if(listWidget->itemWidget(listWidget->item(i))->objectName()==listWidgetItem->objectName()){
                            row = i;
                            if( row+1 <= listWidget->count()){
                             if(row != 0){
                                if(listWidget->itemWidget(listWidget->item(row-1))->objectName().contains(nowPlayingSongId)){
                                    assignNextTrack(listWidget,row);
                                    ui->next->setEnabled(true);
                                }
                             }
                             if(row+1 != listWidget->count()){
                                if(listWidget->itemWidget(listWidget->item(row+1))->objectName().contains(nowPlayingSongId)){
                                    assignPreviousTrack(listWidget,row);
                                    ui->previous->setEnabled(true);
                                }
                             }
                            }
                          break;
                        }
                    }
                }
                //END algo to assign next/previous song if current processed track is next to currently playing track.

                if(s_data.contains("https")){
                    if(s_data.contains("manifest/dash/")){
                        qDebug()<<"MENIFEAT URL:"<<s_data;
                        ManifestResolver *mfr= new ManifestResolver(s_data.trimmed(),this);
                        connect(mfr,&ManifestResolver::m4aAvailable,[=](QString m48url){
                            listWidgetItem->setEnabled(true);
                            QLineEdit *url = listWidgetItem->findChild<QLineEdit *>("url");
                            static_cast<QLineEdit*>(url)->setText(m48url);
                            qDebug()<<"NEW URL:"<<m48url;
                            //TODO
                            store_manager->saveStreamUrl(songId,m48url,getExpireTime(m48url));
                            mfr->deleteLater();
                        });
                        connect(mfr,&ManifestResolver::error,[=](){
                                qDebug()<<"error resolving audio node";
                                return;
                        });

                    }else{
                        listWidgetItem->setEnabled(true);
                        //listWidgetItem->findChild<QLabel*>("loading")->setPixmap(QPixmap(":/icons/blank.png").scaled(track_ui.loading->size(),Qt::KeepAspectRatio,Qt::SmoothTransformation));
                        QLineEdit *url = listWidgetItem->findChild<QLineEdit *>("url");
                        QString url_str = s_data.trimmed();
                        static_cast<QLineEdit*>(url)->setText(url_str);
                        store_manager->saveStreamUrl(songId,url_str,getExpireTime(url_str));
                    }
                    //delete process/task
                    QProcess* senderProcess = qobject_cast<QProcess*>(sender());
                    senderProcess->close();
                    if(senderProcess != nullptr)
                    senderProcess->deleteLater();
                }
            }
    }
}

QString MainWindow::getExpireTime(const QString urlStr){
    QString expiryTime = QUrlQuery(QUrl::fromPercentEncoding(urlStr.toUtf8())).queryItemValue("expire").trimmed();

    if(expiryTime.isEmpty() || !isNumericStr(expiryTime)){
        QString expiryTime = QUrlQuery(QUrl(urlStr.toUtf8())).queryItemValue("expire").trimmed();
    }

    if(expiryTime.isEmpty() || !isNumericStr(expiryTime)){
        expiryTime = urlStr.split("/expire/").last().split("/").first().trimmed();
    }

    if(expiryTime.isEmpty() || !isNumericStr(expiryTime)){
        expiryTime = urlStr.split("expire=").last().split("&").first();
    }

    if(expiryTime.isEmpty() || !isNumericStr(expiryTime)){
        expiryTime = QString::number((QDateTime::currentMSecsSinceEpoch()/1000)+18000);
    }

    return expiryTime;
}

bool MainWindow::isNumericStr(const QString str){
    bool isNumeric;
     str.toInt(&isNumeric, 16);
     str.toInt(&isNumeric, 10);
     return  isNumeric;
}

void MainWindow::queue_currentItemChanged(QListWidget *queue,QListWidgetItem *current, QListWidgetItem *previous)
{
    Q_UNUSED(queue);
    Q_UNUSED(current);
    Q_UNUSED(previous);
    //qDebug()<<queue->objectName()<<current<<previous;
}

void MainWindow::on_right_list_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous)
{
    queue_currentItemChanged(ui->right_list,current,previous);
}
void MainWindow::on_right_list_2_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous)
{
    queue_currentItemChanged(ui->right_list_2,current,previous);
}


void MainWindow::on_left_list_currentRowChanged(int currentRow)
{
    switch (currentRow) {
    case 1:
        browse();
        break;
    case 2:
        recommendations();
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
    case 10:
         show_local_saved_videos();
        break;
    case 15:
         browse_youtube();
        break;
    case 16:
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

void MainWindow::recommendations(){
    pageType = "recommendation";
    ui->webview->load(QUrl("qrc:///web/recommendation/recommendation.html"));
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
        term = term.replace(" ","+");
        term = term.replace("'","%27");
        if(offset.contains(term)){
           offset = offset.remove(term);
        }
        term.append(offset);
        offsetstr = term;

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

void MainWindow::show_local_saved_videos(){
    pageType = "local_saved_videos";
    ui->webview->load(QUrl("qrc:///web/local_videos/local_videos.html"));
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
    //clear debug console
    ui->console->clear();
    //hide console
    if(ui->debug_widget->isVisible())
        ui->debug_widget->hide();

    if(!list->itemWidget(item)->isEnabled())
        return;
    QString id =  list->itemWidget(item)->findChild<QLineEdit*>("id")->text();
    QString url = list->itemWidget(item)->findChild<QLineEdit*>("url")->text();
    QString songId = list->itemWidget(item)->findChild<QLineEdit*>("songId")->text();

    Q_UNUSED(id);

    ElidedLabel *title = list->itemWidget(item)->findChild<ElidedLabel *>("title_elided");
    QString titleStr = static_cast<ElidedLabel*>(title)->text();

    ElidedLabel *artist = list->itemWidget(item)->findChild<ElidedLabel *>("artist_elided");
    QString artistStr = static_cast<ElidedLabel*>(artist)->text();

    ElidedLabel *album = list->itemWidget(item)->findChild<ElidedLabel *>("album_elided");
    QString albumStr = static_cast<ElidedLabel*>(album)->text();

    QString dominant_color = list->itemWidget(item)->findChild<QLineEdit*>("dominant_color")->text();

    QLabel *cover = list->itemWidget(item)->findChild<QLabel *>("cover");
    ui->cover->setPixmap(QPixmap(*cover->pixmap()).scaled(100,100,Qt::KeepAspectRatio,Qt::SmoothTransformation));
    QPixmap(*cover->pixmap()).save(QString(setting_path+"/albumArts/"+"currentArt.png"),0);


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


    //assign next track to next button
    if(list->currentRow()==0){
        ui->previous->setEnabled(false);
    }else{
        //assign next track in list if next item in list is valid
        for(int i=list->currentRow()-1;i>-1;i--){
            if(list->itemWidget(list->item(i))->isEnabled()){
                ui->previous->setEnabled(true);
                assignPreviousTrack(list,i);
                break;
            }
        }
    }


    if(list->currentRow()==list->count()-1){
        ui->next->setEnabled(false);
    }else{
        //assign next track in list if next item in list is valid
        for(int i=list->currentRow()+1;i<list->count();i++){
            if(list->itemWidget(list->item(i))->isEnabled()){
                ui->next->setEnabled(true);
                assignNextTrack(list,i);
                break;
            }else {
                ui->next->setEnabled(false);
            }
        }
    }

    getNowPlayingTrackId();

    ui->nowplaying_widget->setImage(*cover->pixmap());

    ui->state->setText("loading");

    ElidedLabel *title2 = this->findChild<ElidedLabel *>("nowP_title");
    static_cast<ElidedLabel*>(title2)->setText(titleStr);

    ElidedLabel *artist2 = this->findChild<ElidedLabel *>("nowP_artist");
    static_cast<ElidedLabel*>(artist2)->setText(artistStr);

    ElidedLabel *album2 = this->findChild<ElidedLabel *>("nowP_album");
    static_cast<ElidedLabel*>(album2)->setText(albumStr);


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

    //update metadata of MPRIS interface
    if(dp!=nullptr){
        mpris_song_meta.insert("mpris:length", 500000);
        mpris_song_meta.insert("xesam:album", albumStr);
        mpris_song_meta.insert("mpris:artUrl", setting_path+"/albumArts/"+"currentArt.png");
        mpris_song_meta.insert("xesam:title", titleStr);
        mpris_song_meta.insert("xesam:artist", artistStr);
        mpris_song_meta.insert("mpris:trackid", songId);
        QVariantList artistlist;
        artistlist.append(QVariant(artistStr));
        mpris_song_meta.insert("xesam:albumArtist", artistlist);
        mpris_song_meta.insert("xesam:url", url);
        emit dp->currentSongChanged(mpris_song_meta);
    }

    //TODO this is making app slow when the list of tracks if huge
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
        //change the color of main window according to album cover bug here
        this->setStyleSheet(+"QMainWindow{"
                               "background-color:rgba("+r+","+g+","+b+","+"0.1"+");"
                               "background-image:url(:/icons/texture.png), linear-gradient(hsla(0,0%,32%,.99), hsla(0,0%,27%,.95));"
                               "}");

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

        miniModeWidget->setStyleSheet (ui->left_panel->styleSheet().replace("#left_panel","#miniModeWidget"));

        ui->search->setStyleSheet(widgetStyle+"border:none;border-radius:0px;");
        ui->label_5->setStyleSheet(widgetStyle+"border:none;border-radius:0px;");

        //settingsUi theme is set when it is opened;
        ui->stream_info->setStyleSheet("QWidget#stream_info{"+widgetStyle+"}"); //to remove style set by designer

        if(eq != nullptr){
            eq->setStyleSheet("QWidget#equalizer{"+ui->search->styleSheet()+"}"
                                             +"QFrame{"+ui->search->styleSheet()+"}");
            eq->removeStyle();
        }

        QString rgba = r+","+g+","+b+","+"0.2";
        ui->webview->page()->mainFrame()->evaluateJavaScript("changeBg('"+rgba+"')");
    }
    ui->webview->page()->mainFrame()->evaluateJavaScript("setNowPlaying('"+songId+"')");
}
//END PLAY TRACK ON ITEM DOUBLE CLICKED////////////////////////////////////////////////////////////////////////////////////////

//app menu to hide show sidebar
void MainWindow::on_menu_clicked()
{
    QSplitter *split2= this->findChild<QSplitter*>("split2");
    if(static_cast<QSplitter*>(split2)->sizes()[0]==0){ //closed state
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
            static_cast<QSplitter*>(split2)->setSizes(QList<int>()<<var.toRect().width()<<static_cast<QSplitter*>(split2)->sizes()[1]-var.toRect().width());
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
              static_cast<QSplitter*>(split2)->setSizes(QList<int>()<<var.toRect().width()<<static_cast<QSplitter*>(split2)->sizes()[1]+var.toRect().width());
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
        settingsWidget->setStyleSheet("QWidget#settingsWidget{"+ui->search->styleSheet()+"}");
        //refresh cache sizes
        QString setting_path =  QStandardPaths::writableLocation(QStandardPaths::DataLocation);

        utils* util = new utils(this);

        settingsUi.cached_tracks_size->setText(util->refreshCacheSize(setting_path+"/downloadedTracks"));
        settingsUi.offline_pages_size->setText(util->refreshCacheSize(setting_path+"/paginator"));
        settingsUi.database_size->setText(util->refreshCacheSize(setting_path+"/storeDatabase/"+database));

        util->deleteLater();

        settingsWidget->showNormal();
    }
}

void MainWindow::getNowPlayingTrackId(){
    for(int i = 0 ; i< ui->right_list->count();i++){
         if(ui->right_list->itemWidget(ui->right_list->item(i))->findChild<QLabel*>("playing")->toolTip()=="playing..."){
            //get songId of visible track
            QLineEdit *songId = ui->right_list->itemWidget(ui->right_list->item(i))->findChild<QLineEdit*>("songId");
            nowPlayingSongId = static_cast<QLineEdit*>(songId)->text();
        }
    }
    for(int i = 0 ; i< ui->right_list_2->count();i++){
         if(ui->right_list_2->itemWidget(ui->right_list_2->item(i))->findChild<QLabel*>("playing")->toolTip()=="playing..."){
            //get songId of visible track
            QLineEdit *songId = ui->right_list_2->itemWidget(ui->right_list_2->item(i))->findChild<QLineEdit*>("songId");
            nowPlayingSongId = static_cast<QLineEdit*>(songId)->text();
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
    qDebug()<<"NETWORK ERROR OR USER CANCELED REQUEST";
}

//returns theme color from common.js
void MainWindow::setThemeColor(QString color){
    themeColor = color;
}


////////////////////////////////////////////////////////////RADIO///////////////////////////////////////////////////////

void MainWindow::radioStatus(QString radioState){
    if(radioState=="playing"){
        ui->stop->setEnabled(true);
        ui->play_pause->setEnabled(true);
        ui->play_pause->setIcon(QIcon(":/icons/p_pause.png"));
    }else if(radioState=="paused"){
        ui->stop->setEnabled(true);
        ui->play_pause->setEnabled(true);
        ui->play_pause->setIcon(QIcon(":/icons/p_play.png"));
    }else if (radioState=="eof") {
        ui->stop->setEnabled(false);
        ui->play_pause->setEnabled(false);
        ui->play_pause->setIcon(QIcon(":/icons/p_play.png"));

        //remove nowplaying from central widget
        ui->webview->page()->mainFrame()->evaluateJavaScript("setNowPlaying('0000000')");
        setTrackItemNowPlaying();

        // play next track
        if(ui->next->isEnabled()){
            ui->next->click();
        }

    }
    else if(radioState=="stopped"){
        ui->webview->page()->mainFrame()->evaluateJavaScript("setNowPlaying('0000000')");
        ui->stop->setEnabled(false);
        ui->play_pause->setEnabled(false);
        ui->play_pause->setIcon(QIcon(":/icons/p_play.png"));
        setTrackItemNowPlaying();
    }
    if(radioState=="failed"){
        ui->webview->page()->mainFrame()->evaluateJavaScript("setNowPlaying('0000000')");
        if(!ui->debug_widget->isVisible())
            ui->debug_widget->show();
    }
    ui->state->setText(radioState);
}


void MainWindow::setTrackItemNowPlaying(){ //removes playing icon from lists
    for (int i= 0;i<ui->right_list->count();i++) {
      ui->right_list->itemWidget(ui->right_list->item(i))->findChild<QLabel*>("playing")->setPixmap(QPixmap(":/icons/blank.png"));
    }
    for (int i= 0;i<ui->right_list_2->count();i++) {
      ui->right_list_2->itemWidget(ui->right_list_2->item(i))->findChild<QLabel*>("playing")->setPixmap(QPixmap(":/icons/blank.png"));
    }
}

void MainWindow::radioPosition(int pos){
   int seconds = (pos) % 60;
   int minutes = (pos/60) % 60;
   int hours = (pos/3600) % 24;
   QTime time(hours, minutes,seconds);
   ui->position->setText(time.toString());
   ui->radioSeekSlider->setValue(pos);

   //update mpris position
   if(dp!=nullptr){
       dp->playerPosition = static_cast<qlonglong>(pos * 1000000);
   }
}


void MainWindow::radioDuration(int dur){
    int seconds = (dur) % 60;
    int minutes = (dur/60) % 60;
    int hours = (dur/3600) % 24;
    QTime time(hours, minutes,seconds);
    ui->duration->setText(time.toString());
    ui->radioSeekSlider->setMaximum(dur);
    qlonglong duration = static_cast<qlonglong>(dur * 1000000);

    if(dp!=nullptr && duration != mpris_song_meta.value("mpris:length")){
        mpris_song_meta.remove("mpris:length");
        mpris_song_meta.insert("mpris:length", duration);
        emit dp->currentSongChanged(mpris_song_meta);
    }
}

void MainWindow::radio_demuxer_cache_duration_changed(double seconds_available,double radio_playerPosition){
    if(ui->radioSeekSlider->maximum()!=0 && qFuzzyCompare(seconds_available,0) == false
            && qFuzzyCompare(radio_playerPosition,0)==false){
        double totalSeconds = seconds_available+radio_playerPosition;
        double width =  static_cast<double>(totalSeconds) / static_cast<double>(ui->radioSeekSlider->maximum()) ;
        ui->radioSeekSlider->subControlWidth = static_cast<double>(width*100);
    }
}

//wrapper function to playLocalTrack function which finds song in queue and play it if found or add and play it if not found
void MainWindow::playSongById(QVariant songIdVar){
    playLocalTrack(songIdVar);
}

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
           QString songIdFromWidget = static_cast<QLineEdit*>(ui->right_list->itemWidget(ui->right_list->item(i))->findChild<QLineEdit*>("songId"))->text().trimmed();
            if(songId==songIdFromWidget){
                ui->tabWidget->setCurrentWidget(ui->tab);
                ui->right_list->setCurrentRow(i);
                listItemDoubleClicked(ui->right_list,ui->right_list->item(i));
                ui->right_list->scrollToItem(ui->right_list->item(i));
                break;
            }
        }
        for (int i= 0;i<ui->right_list_2->count();i++) {
           QString songIdFromWidget = static_cast<QLineEdit*>(ui->right_list_2->itemWidget(ui->right_list_2->item(i))->findChild<QLineEdit*>("songId"))->text().trimmed();
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

void MainWindow::saveRadioChannelToFavourite(QVariant channelInfo){
    store_manager->setRadioChannelToFavourite(channelInfo.toStringList());
}

void MainWindow::playRadioFromWeb(QVariant streamDetails){
    ui->console->clear();
    QString url,title,country,language,base64,stationId;
    QStringList list = streamDetails.toString().split("=,=");
    stationId = list.at(0);
    qDebug()<<stationId;
    url = list.at(1);
    title = list.at(2);
    country = list.at(3);
    language = list.at(4);
    if(list.count()>5){
        base64 = list.at(5);
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

    this->findChild<ElidedLabel *>("nowP_title")->setText(htmlToPlainText(title));

    this->findChild<ElidedLabel *>("nowP_artist")->setText(language);

    this->findChild<ElidedLabel *>("nowP_album")->setText(country);

    //always false as we don't want record radio for now
    saveTracksAfterBuffer=false;
    radio_manager->playRadio(saveTracksAfterBuffer,QUrl(url.trimmed()));
    ui->webview->page()->mainFrame()->evaluateJavaScript("setNowPlaying('"+stationId+"')");

    //clear playing icon from player queue
    setTrackItemNowPlaying();
}


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
              static_cast<QLabel*>(offline)->setPixmap(QPixmap(":/icons/offline.png").scaled(track_ui.offline->size(),Qt::KeepAspectRatio,Qt::SmoothTransformation));
              ui->right_list->itemWidget(ui->right_list->item(i))->findChild<QLineEdit*>("url")->setText("file://"+download_Path+nowPlayingSongId);
          }
      }
      for(int i = 0 ; i< ui->right_list_2->count();i++){
           if(ui->right_list_2->itemWidget(ui->right_list_2->item(i))->findChild<QLineEdit*>("songId")->text()==nowPlayingSongId){
              QLabel *offline = ui->right_list_2->itemWidget(ui->right_list_2->item(i))->findChild<QLabel*>("offline");
              static_cast<QLabel*>(offline)->setPixmap(QPixmap(":/icons/offline.png").scaled(track_ui.offline->size(),Qt::KeepAspectRatio,Qt::SmoothTransformation));
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
    if(reply->error() == QNetworkReply::NoError){
        // Get the http status code
        int v = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (v >= 200 && v < 300) // Success
        {
            if(reply->error() == QNetworkReply::NoError){
                core_file->write(reply->readAll());
                core_file->close();
                get_engine_version_info();
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
    reply->deleteLater();
}

//writes core_version file with version info after core downloaded
void MainWindow::get_engine_version_info(){
    QNetworkAccessManager *m_netwManager = new QNetworkAccessManager(this);
    connect(m_netwManager,&QNetworkAccessManager::finished,[=](QNetworkReply* rep){
        if(rep->error() == QNetworkReply::NoError){
            QString addin_path = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
            QDir dir(addin_path);
            if (!dir.exists())
            dir.mkpath(addin_path);

            QString filename = "core_version";
            QFile *core_version_file =  new QFile(addin_path+"/"+filename ); //addin_path
            if(!core_version_file->open(QIODevice::ReadWrite | QIODevice::Truncate)){
                qDebug()<<"Could not open a core_version_file to write.";
            }
            core_version_file->write(rep->readAll());
            core_version_file->close();
            core_version_file->deleteLater();
        }
        rep->deleteLater();
        m_netwManager->deleteLater();
    });
    QUrl url("https://rg3.github.io/youtube-dl/update/LATEST_VERSION");
    QNetworkRequest request(url);
    m_netwManager->get(request);
}

void MainWindow::check_engine_updates(){

    //read version from local core_version file
    QString addin_path = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
    QFile *core_version_file =  new QFile(addin_path+"/"+"core_version" );
    if (!core_version_file->open(QIODevice::ReadOnly | QIODevice::Text)){
        core_local_date = "2019.01.01";
        core_remote_date = QDate::currentDate().toString(Qt::ISODate);
        compare_versions(core_local_date,core_remote_date);
        return;
    }
    core_local_date  = core_version_file->readAll().trimmed();

    //read version from remote
    QNetworkAccessManager *m_netwManager = new QNetworkAccessManager(this);
    connect(m_netwManager,&QNetworkAccessManager::finished,[=](QNetworkReply* rep){
        if(rep->error() == QNetworkReply::NoError){
             core_remote_date = rep->readAll().trimmed();
             if(!core_local_date.isNull() && !core_remote_date.isNull()){
                compare_versions(core_local_date,core_remote_date);
             }
        }
        rep->deleteLater();
        m_netwManager->deleteLater();
    });
    QUrl url("https://rg3.github.io/youtube-dl/update/LATEST_VERSION");
    QNetworkRequest request(url);
    m_netwManager->get(request);
}

void MainWindow::compare_versions(QString date,QString n_date){

    int year,month,day,n_year,n_month,n_day;

    year = QDate::fromString(date,Qt::ISODate).year();
    month = QDate::fromString(date,Qt::ISODate).month();
    day = QDate::fromString(date,Qt::ISODate).day();

    n_year = QDate::fromString(n_date,Qt::ISODate).year();
    n_month = QDate::fromString(n_date,Qt::ISODate).month();
    n_day = QDate::fromString(n_date,Qt::ISODate).day();

    bool update= false;

    if(n_year>year || n_month>month || n_day>day ){
       update=true;
    }

    if(update){
        QMessageBox msgBox;
          msgBox.setText("Olivia Engine update (<b>ver: "+n_date+"</b>) available !");
          msgBox.setIcon(QMessageBox::Information);
          msgBox.setInformativeText("You are having an outdated engine (<b>ver: "+date+"</b>), please update to latest engine for better performance. Update now ?");
          msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
          msgBox.setDefaultButton(QMessageBox::Ok);

          int ret = msgBox.exec();
          switch (ret) {
            case QMessageBox::Ok:
                  on_settings_clicked();
                  settingsUi.download_engine->click();
              break;
            case  QMessageBox::Cancel:
                  check_engine_updates();
              break;
          }
    }
}

void MainWindow::evoke_engine_check(){
    if(settingsUi.engine_status->text()=="Absent"){
        QMessageBox msgBox;
          msgBox.setText("Olivia need to download its engine which is responsible for finding music online!");
          msgBox.setIcon(QMessageBox::Information);
          msgBox.setInformativeText("Olivia engine (1.4Mb in size) is youtube-dl with some modifications, without this the app will not work properly, Download now ?");
          msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
          QPushButton *p = new QPushButton("Quit",nullptr);
          msgBox.addButton(p,QMessageBox::NoRole);
          msgBox.setDefaultButton(QMessageBox::Ok);

          int ret = msgBox.exec();
          switch (ret) {
            case QMessageBox::Ok:
                  on_settings_clicked();
                  settingsUi.download_engine->click();
              break;
            case  QMessageBox::Cancel:
                  evoke_engine_check();
              break;
            default:
                qApp->quit();
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
        QString titleStr = static_cast<ElidedLabel*>(title)->text();

        ElidedLabel *artist = list->itemWidget(item)->findChild<ElidedLabel *>("artist_elided");
        QString artistStr = static_cast<ElidedLabel*>(artist)->text();

        ElidedLabel *album = list->itemWidget(item)->findChild<ElidedLabel *>("album_elided");
        QString albumStr = static_cast<ElidedLabel*>(album)->text();
        //QString albumStr = album->text();
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
}

void MainWindow::zoomout(){
    zoom = zoom + 1.0;
    setZoom(zoom);
    settingsUi.zoom->setText(QString::number(ui->webview->zoomFactor(),'f',2));
}

void MainWindow::setZoom(qreal val){
    ui->webview->setZoomFactor(horizontalDpi / val);
    settingsObj.setValue("zoom",zoom);
}

void MainWindow::init_lyrics(){
    lyricsWidget = new Lyrics(this);
    lyricsWidget->setWindowFlags(Qt::Dialog);
    lyricsWidget->setWindowModality(Qt::NonModal);
}

void MainWindow::init_miniMode(){
    miniModeWidget = new QWidget(this);
    miniMode_ui.setupUi(miniModeWidget);
    miniModeWidget->setObjectName("miniModeWidget");
    if(settingsObj.value("miniModeStayOnTop","false").toBool()==true){
        miniModeWidget->setWindowFlags(Qt::Window  | Qt::CustomizeWindowHint  | Qt::WindowStaysOnTopHint  | Qt::FramelessWindowHint );
    }else{
        miniModeWidget->setWindowFlags(Qt::Window | Qt::CustomizeWindowHint | Qt::FramelessWindowHint  );
    }
    miniModeWidget->setWindowModality(Qt::ApplicationModal);
    miniModeWidget->adjustSize();
}

void MainWindow::on_miniMode_clicked()
{
    if(!miniModeWidget->isVisible())
    {
        ui->miniMode->setIcon(QIcon(":/icons/restore_mini_mode.png"));
        ui->miniMode->setToolTip("Restore back to full mode");

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
    //if the function contains two arguments they are seperated by '<==>'
    //here on if we are checking that and passing the given dataType function two arguments by splitting query
    //else we do handle single arg function.
    QString arg1,arg2;
    if(query.contains("<==>")){
        arg1 = query.split("<==>").first();
        arg2 = query.split("<==>").last();
        ui->webview->page()->mainFrame()->evaluateJavaScript(dataType+"(\""+arg1+"\",\""+arg2+"\")");
    }else{
        if(dataType == "track_search"){
            ui->webview->page()->mainFrame()->evaluateJavaScript("$('#tracks_result').html('')");
        }
        ui->webview->page()->mainFrame()->evaluateJavaScript(dataType+"(\""+query+"\")");
    }

}

//currently used by suffle track fucntion only
void MainWindow::getEnabledTracks(QListWidget *currentListWidget){

    if(settingsObj.value("shuffle").toBool()){
        shuffledPlayerQueue.clear();
        for(int i=0;i<currentListWidget->count();i++){
            if(currentListWidget->itemWidget(currentListWidget->item(i))->isEnabled()){
                QString songId = currentListWidget->itemWidget(currentListWidget->item(i))->findChild<QLineEdit *>("songId")->text().trimmed();
                shuffledPlayerQueue.append(songId);
            }
        }
    }
}

void MainWindow::assignNextTrack(QListWidget *list ,int index){

    //shuffled track assignment
    if(settingsObj.value("shuffle").toBool()){
        getEnabledTracks(list);
        if(!shuffledPlayerQueue.isEmpty()){
            QString shuffledSongId;
            int shuffledIndex = qrand() % ((shuffledPlayerQueue.count() + 1) - 1) + 0;

            shuffledSongId = shuffledPlayerQueue.at(shuffledIndex);

            if(!shuffledSongId.isEmpty()){
                ui->next->setToolTip(htmlToPlainText(store_manager->getTrack(shuffledSongId).at(1)));
            }else{
                ui->next->setToolTip("");
            }
            ui->next->disconnect();
            connect(ui->next,&QPushButton::clicked,[=](){
                list->setCurrentRow(shuffledIndex);
                listItemDoubleClicked(list,list->item(shuffledIndex));
                list->scrollToItem(list->item(shuffledIndex));
            });
            return;
        }
    }

    //normal next track algo
    QString songId;

    if(settingsObj.value("shuffle",false).toBool()){

    }else{
         songId = list->itemWidget(list->item(index))->findChild<QLineEdit *>("songId")->text().trimmed();
    }

    if(!songId.isEmpty()){
        ui->next->setToolTip(htmlToPlainText(store_manager->getTrack(songId).at(1)));
    }else{
        ui->next->setToolTip("");
    }
    ui->next->disconnect();
    connect(ui->next,&QPushButton::clicked,[=](){
        list->setCurrentRow(index);
        listItemDoubleClicked(list,list->item(index));
        list->scrollToItem(list->item(index));
    });
}


void MainWindow::assignPreviousTrack(QListWidget *list ,int index)
{
    //shuffled track assignment
    if(settingsObj.value("shuffle").toBool()){
        getEnabledTracks(list);
        if(!shuffledPlayerQueue.isEmpty()){
            QString shuffledSongId;
            int shuffledIndex = qrand() % ((shuffledPlayerQueue.count() + 1) - 1) + 0;

            shuffledSongId = shuffledPlayerQueue.at(shuffledIndex);

            if(!shuffledSongId.isEmpty()){
                ui->previous->setToolTip(htmlToPlainText(store_manager->getTrack(shuffledSongId).at(1)));
            }else{
                ui->previous->setToolTip("");
            }
            ui->previous->disconnect();
            connect(ui->previous,&QPushButton::clicked,[=](){
                list->setCurrentRow(shuffledIndex);
                listItemDoubleClicked(list,list->item(shuffledIndex));
                list->scrollToItem(list->item(shuffledIndex));
            });
            return;
        }
    }

    //normal previous track algo
    QString songId = list->itemWidget(list->item(index))->findChild<QLineEdit *>("songId")->text().trimmed();
    if(!songId.isEmpty()){
        ui->previous->setToolTip(htmlToPlainText(store_manager->getTrack(songId).at(1)));
    }else{
        ui->previous->setToolTip("");
    }
    ui->previous->disconnect();
    connect(ui->previous,&QPushButton::clicked,[=](){
        list->setCurrentRow(index);
        listItemDoubleClicked(list,list->item(index));
        list->scrollToItem(list->item(index));
    });
}



void MainWindow::on_ytdlStopAll_clicked()
{
   // qDebug()<<ytdlQueue.count()<<ytdlProcess;
    if(ytdlProcess!=nullptr){
        ytdlQueue.clear();
        ytdlProcess->close();
        ytdlProcess=nullptr;
    }
    if(ytdlProcess==nullptr){
        ui->ytdlStopAll->setEnabled(false);
        ui->ytdlRefreshAll->setEnabled(true);
    }    
}

void MainWindow::on_ytdlRefreshAll_clicked()
{
    //process Olivia queue
    for (int i= 0;i<ui->right_list->count();i++) {
        //get disabled items
        if(ui->right_list->itemWidget(ui->right_list->item(i))->isEnabled()==false){
            //get id and songId
            QString id,songId;
            id = ui->right_list->itemWidget(ui->right_list->item(i))->findChild<QLineEdit*>("id")->text();
            songId = ui->right_list->itemWidget(ui->right_list->item(i))->findChild<QLineEdit*>("songId")->text();
            //check if ytids are present
            if(!id.isEmpty()){
                //add to ytdlProcessQueue
                getAudioStream(id,songId);
            }
        }
    }
    //process Youtube queue
    for (int i= 0;i<ui->right_list_2->count();i++) {
        //get disabled items
        if(ui->right_list_2->itemWidget(ui->right_list_2->item(i))->isEnabled()==false){
            //get id and songId
            QString id,songId;
            id = ui->right_list_2->itemWidget(ui->right_list_2->item(i))->findChild<QLineEdit*>("id")->text();
            songId = ui->right_list_2->itemWidget(ui->right_list_2->item(i))->findChild<QLineEdit*>("songId")->text();
            //check if ytids are present
            if(!id.isEmpty()){
                //add to ytdlProcessQueue
                getAudioStream(id,songId);
            }
        }
    }
}


//=========================================Track item click handler==========================================
void MainWindow::on_right_list_2_itemClicked(QListWidgetItem *item)
{
    trackItemClicked(ui->right_list_2,item);
}

void MainWindow::on_right_list_itemClicked(QListWidgetItem *item)
{
    trackItemClicked(ui->right_list,item);
}

void MainWindow::trackItemClicked(QListWidget *listWidget,QListWidgetItem *item){
    //check if track is not enabled
    if(listWidget->itemWidget(item)->isEnabled()==false){
        QAction *updateTrack= new QAction("Refresh Track",nullptr);
        QAction *getYtIds = new QAction("Search on Youtube",nullptr);
        QAction *removeTrack = new QAction("Search on Youtube && Remove track",nullptr);
        QAction *removeTrack2 = new QAction("Remove track",nullptr);


        //set Icons
        updateTrack->setIcon(QIcon(":/icons/sidebar/refresh.png"));
        getYtIds->setIcon(QIcon(":/icons/sidebar/search.png"));
        removeTrack->setIcon(QIcon(":/icons/sidebar/remove.png"));
        removeTrack2->setIcon(QIcon(":/icons/sidebar/remove.png"));


        //not enabled, decide menu option to popup
        QString ytIds = listWidget->itemWidget(item)->findChild<QLineEdit*>("id")->text().trimmed();
        QString songId = listWidget->itemWidget(item)->findChild<QLineEdit*>("songId")->text().trimmed();

        connect(removeTrack,&QAction::triggered,[=](){

            //youtube search
            QString  songTitle =  listWidget->itemWidget(item)->findChild<ElidedLabel*>("title_elided")->text();
            QString  songArtist =  listWidget->itemWidget(item)->findChild<ElidedLabel*>("artist_elided")->text();
            QString term = songTitle+" - "+songArtist;
            if(pageType=="youtube"){
                youtubeSearchTerm = term;
                ui->webview->page()->mainFrame()->evaluateJavaScript("$('.ui-content').fadeOut('fast');$('#manual_search').val('"+youtubeSearchTerm+"');manual_youtube_search('"+youtubeSearchTerm+"');");
                youtubeSearchTerm.clear();
            }else{
                setSearchTermAndOpenYoutube(QVariant(term));
            }
            //remove
            listWidget->removeItemWidget(item);
            delete item;
            store_manager->removeFromQueue(songId);
        });

         connect(removeTrack2,&QAction::triggered,[=](){
             listWidget->removeItemWidget(item);
             delete item;
             store_manager->removeFromQueue(songId);
         });

        //qDebug()<<songId<<ytIds;
        //if no ytIds set for track
        if(ytIds.isEmpty()){
             updateTrack->setEnabled(false);
             getYtIds->setEnabled(true);
             connect(getYtIds,&QAction::triggered,[=](){
                //open youtube page and find track using track's metadata
                QString  songTitle =  listWidget->itemWidget(item)->findChild<ElidedLabel*>("title_elided")->text();
                QString  songArtist =  listWidget->itemWidget(item)->findChild<ElidedLabel*>("artist_elided")->text();
                QString term = songTitle+" - "+songArtist;
                if(pageType=="youtube"){
                    youtubeSearchTerm = term;
                    ui->webview->page()->mainFrame()->evaluateJavaScript("$('.ui-content').fadeOut('fast');$('#manual_search').val('"+youtubeSearchTerm+"');manual_youtube_search('"+youtubeSearchTerm+"');");
                    youtubeSearchTerm.clear();
                }else{
                    setSearchTermAndOpenYoutube(QVariant(term));
                }
             });
        }else{
            getYtIds->setEnabled(false);
        }
        //if ytdlProcess is not running
        if(ytdlProcess==nullptr && !ytIds.isEmpty()){
            updateTrack->setEnabled(true);
            connect(updateTrack,&QAction::triggered,[=](){
               getAudioStream(ytIds,songId);
            });
        }else if(ytdlProcess!=nullptr && ytdlQueue.count()>0){ // if ytdlProcess is running set the track to upcoming process in ytdlQueue
            //move this to upcoming ytdlProcess
        }
        //show menu
        QMenu menu;
        menu.addAction(updateTrack);
        menu.addAction(getYtIds);
        menu.addSeparator();
        menu.addAction(removeTrack);
        menu.addAction(removeTrack2);
        menu.setStyleSheet(menuStyle());
        menu.exec(QCursor::pos());
    }else{
//        QString songId = listWidget->itemWidget(item)->findChild<QLineEdit*>("songId")->text().trimmed();
//        QPushButton* optionButton = listWidget->itemWidget(item)->findChild<QPushButton*>(songId+"optionButton");
//        optionButton->click();
    }
}
//=========================================END Track item click handler==========================================


//returns Qmenu stylesheet according to current theme
QString MainWindow::menuStyle(){
    //set theme for menu
    QStringList rgba = themeColor.split(",");
    QString r,g,b,a;
    r= rgba.at(0);
    g= rgba.at(1);
    b= rgba.at(2);
    a= rgba.at(3);
    QString  menuStyle(
             "QMenu:icon{"
              "padding-left:4px;"
             "}"
             "QMenu::item{"
                "background-color:rgba("
                 +r+","+g+","+b+","+a+");"
                "color: rgb(255, 255, 255);"
             "}"

             "QMenu::item:selected{"
                "border:none;"
                "background-color:"
                "qlineargradient(spread:pad, x1:0, y1:0, x2:0, y2:1,"
                "stop:0.129213 rgba("+r+", "+g+", "+b+", 35),"
                "stop:0.38764 rgba("+r+", "+g+", "+b+", 120),"
                "stop:0.679775 rgba("+r+", "+g+", "+b+", 110),"
                "stop:1 rgba("+r+", "+g+", "+b+", 35));"
             "color: rgb(255, 255, 255);"
             "}"

            "QMenu::item:disabled{"
              "background-color:rgba("
               +r+","+g+","+b+",0.1);"
              "color: grey;"
            "}"
          );
    return menuStyle;
}

//create and show queue option
void MainWindow::on_olivia_queue_options_clicked()
{
    queueShowOption(ui->right_list);
}

void MainWindow::on_youtube_queue_options_clicked()
{
    queueShowOption(ui->right_list_2);
}

void MainWindow::queueShowOption(QListWidget *queue){

    QAction *clearQueue  = new QAction(QIcon(":/icons/sidebar/remove.png"),"Clear this queue",nullptr);
    QAction *clearUnCached = new QAction(QIcon(":/icons/sidebar/remove.png"),"Clear un-cached songs",nullptr);
    QAction *refreshDeadTracks = new QAction(QIcon(":/icons/sidebar/refresh.png"),"Refresh dead tracks",nullptr);

    //connect
    if(queue->count()>0){

        clearQueue->setEnabled(true);
        refreshDeadTracks->setEnabled(hasDeadTracks(queue));
        clearUnCached->setEnabled(hasUnCachedTracks(queue));

        connect(clearQueue,&QAction::triggered,[=](){
            for (int i=0; i<queue->count();i++) {
                QString songIdFromWidget = static_cast<QLineEdit*>(queue->itemWidget(queue->item(i))->findChild<QLineEdit*>("songId"))->text().trimmed();
                store_manager->removeFromQueue(songIdFromWidget);
            }
            queue->clear();

        });
        connect(clearUnCached,&QAction::triggered,[=](){
            for (int i=0; i<queue->count();i++) {
                QString songIdFromWidget = static_cast<QLineEdit*>(queue->itemWidget(queue->item(i))->findChild<QLineEdit*>("songId"))->text().trimmed();
                if(!store_manager->isDownloaded(songIdFromWidget) /*&& !trackIsBeingProcessed(songIdFromWidget)*/){
                    store_manager->removeFromQueue(songIdFromWidget);
                    queue->removeItemWidget(queue->item(i));
                    if(queue->item(i) != nullptr){
                        delete queue->item(i);
                    }
                }
            }
        });

        connect(refreshDeadTracks,&QAction::triggered,[=](){
            for (int i=0; i<queue->count();i++) {
                QString songIdFromWidget = static_cast<QLineEdit*>( queue->itemWidget(queue->item(i))->findChild<QLineEdit*>("songId"))->text().trimmed();
                if( !queue->itemWidget(queue->item(i))->isEnabled() && !store_manager->isDownloaded(songIdFromWidget)){
                    getAudioStream(store_manager->getYoutubeIds(songIdFromWidget),songIdFromWidget);
                }
            }
        });
    }else{
        clearQueue->setEnabled(false);
        clearUnCached->setEnabled(false);
        refreshDeadTracks->setEnabled(false);
    }

    QMenu menu;
    menu.addAction(clearQueue);
    menu.addAction(clearUnCached);
    menu.addSeparator();
    menu.addAction(refreshDeadTracks);
    menu.setStyleSheet(menuStyle());
    menu.exec(QCursor::pos());

}

bool MainWindow::hasDeadTracks(QListWidget *queue){
    bool has = false;
     for (int i=0; i<queue->count();i++) {
         QString songIdFromWidget = static_cast<QLineEdit*>(queue->itemWidget(queue->item(i))->findChild<QLineEdit*>("songId"))->text().trimmed();
         if(store_manager->getExpiry(songIdFromWidget) && !store_manager->isDownloaded(songIdFromWidget)){
             has = true;
         }
         if(has)break;
     }
     return has;
}

bool MainWindow::hasUnCachedTracks(QListWidget *queue){
    bool has = false;
     for (int i=0; i<queue->count();i++) {
         QString songIdFromWidget = static_cast<QLineEdit*>(queue->itemWidget(queue->item(i))->findChild<QLineEdit*>("songId"))->text().trimmed();
         if(!store_manager->isDownloaded(songIdFromWidget)){
             has = true;
         }
         if(has)break;
     }
     return has;
}

//unused right now
bool MainWindow::trackIsBeingProcessed(QString songId){
    bool isProcessing = false;
    QList<QProcess*> ytDlProcessList;
    ytDlProcessList = this->findChildren<QProcess*>();
    foreach (QProcess *process, ytDlProcessList) {
        if(process->objectName().trimmed()==songId){
            isProcessing = true;
        }
        if(isProcessing)break;
    }
    return isProcessing;
}


void MainWindow::clear_queue(){
    ui->stop->click();
    ui->right_list->clear();
    ui->right_list_2->clear();
}

//change app transparency
void MainWindow::transparency_changed(int value)
{
    setWindowOpacity(qreal(value)/100);
}


//================================Equalizer=========================================================
void MainWindow::on_eq_clicked()
{
    QString btn_style ="QPushButton{color: silver; background-color: #45443F; border:1px solid #272727; padding-top: 3px; padding-bottom: 3px; padding-left: 3px; padding-right: 3px; border-radius: 2px; outline: none;}"
    "QPushButton:disabled { background-color: #45443F; border:1px solid #272727; padding-top: 3px; padding-bottom: 3px; padding-left: 5px; padding-right: 5px; /*border-radius: 2px;*/ color: #636363;}"
    "QPushButton:hover{border: 1px solid #272727;background-color:#5A584F; color:silver ;}"
    "QPushButton:pressed {background-color: #45443F;color: silver;padding-bottom:1px;}";

    eq->setStyleSheet("");
    eq->setStyleSheet("QWidget#equalizer{"+ui->search->styleSheet()+"}"
                                     +"QFrame{"+ui->search->styleSheet()+"}"
                                     +btn_style);
    eq->removeStyle();
    eq->show();
}

//called on every radio restart while eq class is initialzed once
void MainWindow::init_eq(){

    if(eq == nullptr){
        eq = new equalizer(this);

        eq->setWindowTitle(QApplication::applicationName()+" - Equalizers");
        eq->setWindowFlags(Qt::Dialog);
        eq->setWindowModality(Qt::NonModal);
    }

    connect(eq,SIGNAL(update_eq(QString)),this,SLOT(set_eq(QString)));
    connect(eq,SIGNAL(disable_eq()),this,SLOT(disable_eq()));

    ui->eq->setIcon(eq->eq_enabled()?QIcon(":/icons/eq_button.png"):QIcon(":/icons/eq_button_disabled.png"));

    eq->setRange();
    eq->loadSettings();
    QTimer::singleShot(500,eq,SLOT(triggerEq()));
}

void MainWindow::set_eq(QString eq_args){
    if(!eq_args.isEmpty()){
        QProcess *fifo = new QProcess(this);
        connect(fifo, SIGNAL(finished(int)), this, SLOT(deleteProcess(int)) );
        fifo->start("bash",QStringList()<<"-c"<< "echo '{\"command\": [\"set_property\" ,\"af\",\""+eq_args+"\"]}' | socat - "+ radio_manager->used_fifo_file_path);
        processIdList.append(fifo->processId());
        ui->eq->setIcon(QIcon(":/icons/eq_button.png"));
        ui->eq->setToolTip("Equalizer (Enabled)");
    }
}

void MainWindow::disable_eq(){
    QProcess *fifo = new QProcess(this);
    connect(fifo, SIGNAL(finished(int)), this, SLOT(deleteProcess(int)) );
    fifo->start("bash",QStringList()<<"-c"<< "echo '{\"command\": [\"af\",\"set\",\"""\"]}' | socat - "+ radio_manager->used_fifo_file_path);
    processIdList.append(fifo->processId());
    ui->eq->setIcon(QIcon(":/icons/eq_button_disabled.png"));
    ui->eq->setToolTip("Equalizer (Disabled)");
}

void MainWindow::deleteProcess(int code){
    QProcess *process = qobject_cast<QProcess*>(sender());
    if(code==0){
        process->deleteLater();
    }
}

void MainWindow::radioProcessReady(){
    if(eq==nullptr){
        init_eq();
    }else{
        eq->setRange();
        eq->loadSettings();
        QTimer::singleShot(500,eq,SLOT(triggerEq()));
    }
}
//================================Equalizer=========================================================


//================================Shuffle===========================================================
void MainWindow::on_shuffle_toggled(bool checked)
{
    settingsObj.setValue("shuffle",checked);
    if(checked){
        ui->shuffle->setIcon(QIcon(":/icons/shuffle_button.png"));
        ui->shuffle->setToolTip("Shuffle (Enabled)");
    }else{
        ui->shuffle->setIcon(QIcon(":/icons/shuffle_button_disabled.png"));
        ui->shuffle->setToolTip("Shuffle (Disabled)");
    }
}
//================================Shuffle===========================================================


void MainWindow::on_hideDebug_clicked()
{
    ui->debug_widget->hide();
}

void MainWindow::init_downloadWidget(){
    downloadWidget = new Widget(0);
    downloadWidget->setWindowFlags(Qt::Widget);
    downloadWidget->downloadLocation =setting_path+"/downloadedVideos";
    ui->downloadWidgetVLayout->addWidget(downloadWidget);
    connect(qApp,SIGNAL(aboutToQuit()),downloadWidget,SLOT(on_pauseAll_clicked()));
}

void MainWindow::videoOptionDownloadRequested(QStringList trackMetaData,QStringList formats){ //videoformat<<audioformat
    ui->tabWidget->setCurrentIndex(2);
    QString ytIds,title,artist,album,coverUrl,songId;
    songId = trackMetaData.at(0);
    title = trackMetaData.at(1);
    album = trackMetaData.at(2);
    artist = trackMetaData.at(3);
    coverUrl = trackMetaData.at(4);
    ytIds = trackMetaData.at(5);

    downloadWidget->trackId = songId;

    //remove html notations from title
    QTextDocument text;
    text.setHtml(title);
    QString plainTitle = text.toPlainText();
    downloadWidget->trackTitle = plainTitle;
    text.deleteLater();

    QString url_str = "https://www.youtube.com/watch?v="+ytIds.split("<br>").first();
    downloadWidget->startWget(url_str,downloadWidget->downloadLocation,formats);
}

void MainWindow::leftListChangeCurrentRow(int row){
    ui->left_list->blockSignals(true);
     ui->left_list->setCurrentRow(row);
    ui->left_list->blockSignals(false);
}

void MainWindow::playVideo(QString trackId){
    QDir dir(setting_path+"/downloadedVideos/");
    QStringList filter;
    filter<< trackId+"*";
    QFileInfoList files = dir.entryInfoList(filter);
    //store *store_manager  = this->findChild<store*>("store_manager");
    if(store_manager!=nullptr){
        QString titleLabel =store_manager->getTrack(trackId).at(1);
        QProcess *player = new QProcess(this);
        player->setObjectName("player");
        player->start("mpv",QStringList()<<"--title=MPV for Olivia - "+
                      titleLabel<<"--no-ytdl"<<files.at(0).filePath()<<"--volume"<<QString::number(radio_manager->volume)
                      );
    }else{
        qDebug()<<"StoreManager not found";
    }
}

//jump to queue of nowPlaying track scroll to it and select it
void MainWindow::on_jump_to_nowplaying_clicked()
{
    //identify the player queue name
    QString listName;
    QListWidget *listWidget;
    QWidget *listWidgetItem = ui->right_list->findChild<QWidget*>("track-widget-"+nowPlayingSongId);
    listWidget = ui->right_list;
    listName = "olivia";
    if(listWidgetItem==nullptr){
        listWidgetItem= ui->right_list_2->findChild<QWidget*>("track-widget-"+nowPlayingSongId);
        listWidget = ui->right_list_2;
        listName = "youtube";
    }
    //check if listWidgetItem found in one of list
    if(listWidgetItem==nullptr){
        qDebug()<<"CANNOT JUMP, TRACK NOT FOUND IN LIST";
        listName="";
        return;
    }else{
        // select current track
        if(listName=="olivia")
             ui->tabWidget->setCurrentWidget(ui->tab);
        if(listName=="youtube")
             ui->tabWidget->setCurrentWidget(ui->tab_2);

        //switch tab where track is found
        for (int i= 0;i<listWidget->count();i++) {
            QString songIdFromWidget = static_cast<QLineEdit*>(listWidget->itemWidget(listWidget->item(i))->findChild<QLineEdit*>("songId"))->text().trimmed();
             if(songIdFromWidget.contains(nowPlayingSongId)){
                listWidget->scrollToItem(listWidget->item(i));
                listWidget->setCurrentRow(i);
             }
        }
    }
}
