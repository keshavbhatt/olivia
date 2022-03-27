#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
  ui->setupUi(this);
  show_SysTrayIcon();
  init_app();
  init_webview();
  init_offline_storage();
  init_engine();
  init_settings();
  init_miniMode();
  init_smartMode();
  init_lyrics();
  init_soundCloud();
  init_backup();
  init_downloadWidget();
  database = "hjkfdsll";
  store_manager = new store(this, database);
  store_manager->setObjectName("store_manager");
  connect(store_manager, &store::removeSongFromYtDlQueue,
          [=](QString songId) { removeSongFromProcessQueue(songId); });
  init_similar_tracks();
  loadPlayerQueue();
  init_search_autoComplete();
  init_radio();
  init_appShortcuts();
  init_videoOption();
  browse();
  installEventFilters();
  loadSettings();
}

void MainWindow::init_engine() {
  engine = new Engine(this);

  connect(engine, &Engine::errorMessage,
          [=](QString errorMessage) { this->showError(errorMessage); });

  connect(engine, &Engine::engineCacheCleared, [=]() {
    if (settingsUi.loading_movie->movie() != nullptr) {
      settingsUi.loading_movie->movie()->stop();
    }
    settingsUi.loading_movie->setVisible(false);
  });

  connect(engine, &Engine::engineDownloadFailed,
          [=](QString errorMessage) { this->showError(errorMessage); });

  connect(engine, &Engine::engineDownloadSucceeded, [=]() {
    if (settingsUi.loading_movie->movie() != nullptr) {
      settingsUi.loading_movie->movie()->stop();
    }
    settingsUi.loading_movie->setVisible(false);
    settingsUi.download_engine->setEnabled(true);
  });

  connect(engine, &Engine::engineStatus,
          [=](QString status) { settingsUi.engine_status->setText(status); });

  connect(engine, &Engine::openSettingsAndClickDownload, [=]() {
    on_settings_clicked();
    engine->download_engine_clicked();
    settingsUi.download_engine->setEnabled(false);
    QMovie *movie = new QMovie(":/icons/others/load.gif");
    settingsUi.loading_movie->setMovie(movie);
    settingsUi.loading_movie->setVisible(true);
    movie->start();
  });
}

void MainWindow::showError(QString message) {
  QMessageBox::information(
      this, QApplication::applicationName() + " Information", message);
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::init_similar_tracks() {
  if (similarTracks == nullptr) {

    similarTracks = new SimilarTracks(
        this, settingsObj.value("similarTracksToLoad", 8).toInt());

    connect(similarTracks, &SimilarTracks::lodingStarted,
            [=]() { ui->similarTrackLoader->start(); });
    connect(similarTracks, &SimilarTracks::setSimilarTracks,
            [=](QStringList list) {
              similarTracks->isLoadingPLaylist = false;
              currentSimilarTrackList.clear();
              currentSimilarTrackList = list;
              currentSimilarTrackProcessing = 0;
              prepareSimilarTracks();
            });
    connect(similarTracks, &SimilarTracks::setPlaylist, [=](QStringList list) {
      if (!ui->recommWidget->isVisible())
        ui->show_hide_smart_list_button->click();
      similarTracks->isLoadingPLaylist = true;
      currentSimilarTrackList.clear();
      currentSimilarTrackList = list;
      currentSimilarTrackProcessing = 0;
      prepareSimilarTracks();
    });
    connect(similarTracks, &SimilarTracks::failedGetSimilarTracks, [=]() {
      notify("", "Failed to load smart playlist");
      ui->similarTrackLoader->stop();
    });
    connect(similarTracks, &SimilarTracks::clearList, [=]() {
      currentSimilarTrackList.clear();
      //            similarTracks->parentSongId.clear();
      //            similarTracks->previousParentSongId.clear();
      ui->smart_list->clear();
    });
    connect(similarTracks, &SimilarTracks::clearListKeepingPlayingTrack, [=]() {
      while (similarTracksListHasTrackToBeRemoved()) {
        for (int i = 0; i < ui->smart_list->count(); i++) {
          if (ui->smart_list->itemWidget(ui->smart_list->item(i))
                  ->findChild<QLabel *>("playing")
                  ->toolTip() != "playing...") {
            QListWidgetItem *item = ui->smart_list->takeItem(i);
            if (item != nullptr) {
              delete item;
            }
          }
        }
      }
      ui->previous->setEnabled(false);
    });
  }
}

void MainWindow::init_radio() {
  ui->radioVolumeSlider->setMinimum(0);
  ui->radioVolumeSlider->setMaximum(130);
  ui->radioVolumeSlider->setValue(100);
  saveTracksAfterBuffer = settingsObj.value("saveAfterBuffer", "true").toBool();
  ui->radioVolumeSlider->setValue(settingsObj.value("volume", "100").toInt());
  radio_manager =
      new radio(this, ui->radioVolumeSlider->value(), saveTracksAfterBuffer);
  connect(radio_manager, SIGNAL(radioProcessReady()), this,
          SLOT(radioProcessReady()));
  connect(radio_manager, SIGNAL(radioStatus(QString)), this,
          SLOT(radioStatus(QString)));
  connect(radio_manager, SIGNAL(radioPosition(int)), this,
          SLOT(radioPosition(int)));
  connect(radio_manager, SIGNAL(radioDuration(int)), this,
          SLOT(radioDuration(int)));
  connect(radio_manager, SIGNAL(demuxer_cache_duration_changed(double, double)),
          this, SLOT(radio_demuxer_cache_duration_changed(double, double)));
  connect(radio_manager, SIGNAL(saveTrack(QString)), this,
          SLOT(saveTrack(QString)));
  connect(radio_manager, &radio::icy_cover_changed, [=](QPixmap pix) {
    ui->cover->clear();
    ui->cover->setPixmap(
        pix.scaled(100, 100, Qt::KeepAspectRatio, Qt::SmoothTransformation));
  });
  connect(radio_manager, &radio::icy_title_changed, [=](QString title) {
    ui->nowP_title->setText(title);
    if (settingsObj.value("marquee", false).toBool()) {
      ui->playing->setText(title);
    } else {
      ui->playing->setText("");
    }
  });

  // set initial tempVolume for radio_manager so the first track will also
  // fadeIn
  radio_manager->tempVol = ui->radioVolumeSlider->value();

  connect(radio_manager, &radio::fadeOutVolume, [=]() {
    if (settingsObj.value("crossfade", "false").toBool()) {
      radio_manager->fading = true;
      QPropertyAnimation *a = new QPropertyAnimation(radio_manager, "volume_");
      a->setDuration(1000);
      oldVolume = ui->radioVolumeSlider->value();
      int volume = ui->radioVolumeSlider->value();
      a->setStartValue(volume);
      int stepsToReduce = volume - 40;
      if (stepsToReduce < 0) {
        while (stepsToReduce != 0) {
          stepsToReduce++;
        }
      }
      a->setEndValue(stepsToReduce);
      radio_manager->tempVol = stepsToReduce;
      a->setEasingCurve(QEasingCurve::Linear);
      a->start(QPropertyAnimation::DeleteWhenStopped);
    }
  });

  // shows only faded volume
  connect(radio_manager, &radio::volumeChanged, [=](int val) {
    Q_UNUSED(val);
    // qDebug()<<"Faded Volume:"<<val;
  });

  connect(radio_manager, &radio::fadeInVolume, [=]() {
    if (settingsObj.value("crossfade", "false").toBool()) {
      QPropertyAnimation *a = new QPropertyAnimation(radio_manager, "volume_");
      bool faded_quick;
      if (radio_manager->fadequick) {
        a->setDuration(1500);
        faded_quick = true;
      } else {
        a->setDuration(3000);
        faded_quick = false;
      }
      a->setStartValue(radio_manager->tempVol);
      a->setEndValue(oldVolume);
      a->setEasingCurve(QEasingCurve::Linear);
      a->start(QPropertyAnimation::DeleteWhenStopped);
      radio_manager->fading = false;
      if (faded_quick)
        radio_manager->fadequick = false;
    }
  });

  connect(radio_manager, &radio::showToast,
          [=](const QString str) { showToast(str); });
  connect(radio_manager, &radio::getTrackInfo, [=]() {
    const QString trackId = nowPlayingSongIdWatcher->getValue();
    // disable track
    QListWidget *list =
        this->findChild<QListWidget *>(getCurrentPlayerQueue(trackId));
    if (list != nullptr) {
      QWidget *itemWidget =
          list->findChild<QWidget *>("track-widget-" + trackId);
      itemWidget->setEnabled(false);
    }
    // refresh track
    getAudioStream(store_manager->getYoutubeIds(trackId), trackId);
    //      checkOtherTracksForExpiry();
  });

  radio_manager->startRadioProcess(saveTracksAfterBuffer, "", false);
  connect(ui->radioSeekSlider, &seekSlider::setPosition, [=](QPoint localPos) {
    ui->radioSeekSlider->blockSignals(true);
    int pos =
        ui->radioSeekSlider->minimum() +
        ((ui->radioSeekSlider->maximum() - ui->radioSeekSlider->minimum()) *
         localPos.x()) /
            ui->radioSeekSlider->width();
    QPropertyAnimation *a =
        new QPropertyAnimation(ui->radioSeekSlider, "value");
    a->setDuration(150);
    a->setStartValue(ui->radioSeekSlider->value());
    a->setEndValue(pos);
    a->setEasingCurve(QEasingCurve::Linear);
    a->start(QPropertyAnimation::DeleteWhenStopped);
    radio_manager->radioSeek(pos);
    ui->radioSeekSlider->blockSignals(false);
    QApplication::processEvents();
  });
  connect(ui->radioSeekSlider, &seekSlider::showToolTip, [=](QPoint localPos) {
    int pos =
        ui->radioSeekSlider->minimum() +
        ((ui->radioSeekSlider->maximum() - ui->radioSeekSlider->minimum()) *
         localPos.x()) /
            ui->radioSeekSlider->width();
    int seconds = (pos) % 60;
    int minutes = (pos / 60) % 60;
    int hours = (pos / 3600) % 24;
    QTime time(hours, minutes, seconds);
    QToolTip::showText(ui->radioSeekSlider->mapToGlobal(localPos),
                       "Seek: " + time.toString());
  });
  connect(ui->radioVolumeSlider, &volumeSlider::setPosition,
          [=](QPoint localPos) {
            int pos = ui->radioVolumeSlider->minimum() +
                      ((ui->radioVolumeSlider->maximum() -
                        ui->radioVolumeSlider->minimum()) *
                       localPos.x()) /
                          ui->radioVolumeSlider->width();
            if (!radio_manager->fading) {
              QPropertyAnimation *a =
                  new QPropertyAnimation(ui->radioVolumeSlider, "value");
              a->setDuration(100);
              a->setStartValue(ui->radioVolumeSlider->value());
              a->setEndValue(pos);
              a->setEasingCurve(QEasingCurve::Linear);
              a->start(QPropertyAnimation::DeleteWhenStopped);
            }
            QApplication::processEvents();
          });
  connect(
      ui->radioVolumeSlider, &volumeSlider::showToolTip, [=](QPoint localPos) {
        if (localPos.x() != 0) {
          int pos = ui->radioVolumeSlider->minimum() +
                    ((ui->radioVolumeSlider->maximum() -
                      ui->radioVolumeSlider->minimum()) *
                     localPos.x()) /
                        ui->radioVolumeSlider->width();
          if (pos > -1) {
            if (pos < 101)
              QToolTip::showText(ui->radioVolumeSlider->mapToGlobal(localPos),
                                 "Set volume: " + QString::number(pos));
            else if (pos < 131)
              QToolTip::showText(ui->radioVolumeSlider->mapToGlobal(localPos),
                                 "Set volume: " + QString::number(pos) +
                                     " (amplified)");
          }
        } else {
          int pos = ui->radioVolumeSlider->value();
          if (pos > -1) {
            if (pos < 101)
              QToolTip::showText(ui->radioVolumeSlider->mapToGlobal(localPos),
                                 "Volume: " + QString::number(pos));
            else if (pos < 131)
              QToolTip::showText(ui->radioVolumeSlider->mapToGlobal(localPos),
                                 "Volume: " + QString::number(pos) +
                                     " (amplified)");
          }
        }
      });

  if (settingsObj.value("mpris").toBool()) {
    init_mpris();
  }
}

void MainWindow::init_mpris() {
  if (dp == nullptr) {
    dp = new MprisPlugin(this);
    connect(dp, SIGNAL(Next()), ui->next, SLOT(click()));
    connect(dp, SIGNAL(PlayPause()), ui->play_pause, SLOT(click()));
    connect(dp, SIGNAL(Play()), ui->play_pause, SLOT(click()));
    connect(dp, SIGNAL(Pause()), ui->play_pause, SLOT(click()));
    connect(dp, SIGNAL(Stop()), ui->stop, SLOT(click()));
    connect(dp, &MprisPlugin::Raise, [=]() { this->show(); });

    connect(ui->state, &QLineEdit::textChanged, [=](const QString state) {
      QString statusStr;
      if (state == "playing") {
        statusStr = "Playing";
      } else if (state == "paused") {
        statusStr = "Paused";
      } else {
        statusStr = "Stopped";
      }
      dp->playerStatus = statusStr;
      emit dp->PlaybackStatusChanged(statusStr);
    });
  }
}

void MainWindow::volumeUp() {
  int vol = ui->radioVolumeSlider->value();
  if (vol == 125)
    return;
  ui->radioVolumeSlider->setValue(++vol);
}

void MainWindow::volumeDown() {
  int vol = ui->radioVolumeSlider->value();
  if (vol == 0)
    return;
  ui->radioVolumeSlider->setValue(--vol);
}

void MainWindow::playNextTrack() {
  if (!ui->next->isEnabled())
    return;
  ui->next->click();
}

void MainWindow::playPreviousTrack() {
  if (!ui->previous->isEnabled())
    return;
  ui->previous->click();
}

void MainWindow::toggleMute() {
  if (ui->radioVolumeSlider->value() == 0) {
    ui->radioVolumeSlider->setValue(mutedVolume);
  } else {
    mutedVolume = ui->radioVolumeSlider->value();
    ui->radioVolumeSlider->setValue(0);
  }
}

void MainWindow::seekForward() { radio_manager->quick_seek(true); }

void MainWindow::seekBackward() { radio_manager->quick_seek(false); }

void MainWindow::init_appShortcuts() {
  QShortcut *volumeUp = new QShortcut(Qt::Key_0, this, SLOT(volumeUp()));
  volumeUp->setContext(Qt::ApplicationShortcut);

  QShortcut *volumeDown = new QShortcut(Qt::Key_9, this, SLOT(volumeDown()));
  volumeDown->setContext(Qt::ApplicationShortcut);

  QShortcut *seekForward =
      new QShortcut(Qt::Key_Right, this, SLOT(seekForward()));
  seekForward->setContext(Qt::ApplicationShortcut);

  QShortcut *seekBackward =
      new QShortcut(Qt::Key_Left, this, SLOT(seekBackward()));
  seekBackward->setContext(Qt::ApplicationShortcut);

  QShortcut *mute = new QShortcut(Qt::Key_M, this, SLOT(toggleMute()));
  mute->setContext(Qt::ApplicationShortcut);

  QShortcut *next =
      new QShortcut(QKeySequence("Ctrl+Right"), this, SLOT(playNextTrack()));
  next->setContext(Qt::ApplicationShortcut);

  QShortcut *previous =
      new QShortcut(QKeySequence("Ctrl+Left"), this, SLOT(playPreviousTrack()));
  previous->setContext(Qt::ApplicationShortcut);

  QShortcut *quit =
      new QShortcut(QKeySequence("Ctrl+Q"), this, SLOT(quitApp()));
  quit->setContext(Qt::ApplicationShortcut);
}

void MainWindow::installEventFilters() {
  ui->playing->installEventFilter(this);
  ui->top_widget->installEventFilter(this);
  ui->windowControls->installEventFilter(this);
  ui->label_6->installEventFilter(this);
  ui->nowPlayingGrip->installEventFilter(this);
  // install event filter for all buttons derived from controlButton to prevent
  // their default tooltip event
  foreach (controlButton *btn, this->findChildren<controlButton *>()) {
    btn->installEventFilter(this);
  }
  smartModeWidget->installEventFilter(this);
}

void MainWindow::notify(QString title, QString message) {
  if (settingsObj.value("disable_notifications", false).toBool() == true) {
    return;
  }
  if (title.isEmpty())
    title = QApplication::applicationName();

  if (settingsObj.value("notificationCombo", 1).toInt() == 0 &&
      trayIcon != nullptr) {
    trayIcon->showMessage(title, message, QIcon(":/icons/app/icon-64.png"),
                          5000);
  } else {
    notificationPopup->present(title, message,
                               QPixmap(":/icons/app/icon-64.png"));
  }
}

void MainWindow::closeEvent(QCloseEvent *event) {

  if (QSystemTrayIcon::isSystemTrayAvailable() &&
      settingsObj.value("closeButtonActionCombo", 0).toInt() == 0) {
    this->hide();
    event->ignore();
    if (minimize_notified == false) {
      notify(QApplication::applicationName(),
             "Application is minimized to system tray.");
      minimize_notified = true;
    }

    return;
  }
  quitApp();
  QMainWindow::closeEvent(event);
}

// do not use qApp quit, use this instead.
void MainWindow::quitApp() {
  settingsObj.setValue("geometry", saveGeometry());
  settingsObj.setValue("windowState", saveState());
  settingsObj.setValue("volume", radio_manager->volume);
  settUtils->changeSystemTitlebar(settingsUi.systemTitlebar->isChecked());
  if (ytdlProcess != nullptr && ytdlQueue.count() > 0) {
    ytdlQueue.clear();
    ytdlProcess->close();
  }
  radio_manager->quitRadio();
  radio_manager->killRadioProcess();
  radio_manager->deleteLater();

  for (int i = 0; i < processIdList.count(); i++) {
    QProcess::execute(
        "pkill", QStringList() << "-P" << QString::number(processIdList.at(i)));
    processIdList.removeAt(i);
  }
  // clear converted_temp dir
  QDir dir(setting_path + "/converted_temp");
  dir.setNameFilters(QStringList() << "*.*");
  dir.setFilter(QDir::Files);
  QStringList entries = dir.entryList();

  for (int i = 0; i < entries.count(); i++) {
    auto dirFile = entries.at(i);
    dir.remove(dirFile);
  }
  // remove used socket file
  QFile fifo(radio_manager->used_fifo_file_path);
  fifo.remove();

  qApp->quit();
}

void MainWindow::init_videoOption() {
  if (videoOption == nullptr) {
    videoOption = new VideoOption(nullptr, store_manager, radio_manager);

    videoOption->setWindowTitle(QApplication::applicationName() +
                                " - Video Option");
    videoOption->setWindowFlags(
        Qt::Window | Qt::WindowCloseButtonHint | Qt::WindowMinimizeButtonHint |
        Qt::WindowMaximizeButtonHint | Qt::WindowFullscreenButtonHint);
    videoOption->setWindowModality(Qt::WindowModal);

    connect(videoOption, SIGNAL(downloadRequested(QStringList, QStringList)),
            this, SLOT(videoOptionDownloadRequested(QStringList, QStringList)));
  }
}

void MainWindow::init_settings() {
  settUtils = new settings(this);
  connect(settUtils, SIGNAL(dynamicTheme(bool)), this,
          SLOT(dynamicThemeChanged(bool)));

  settingsObj.setObjectName("settings");
  setting_path = QStandardPaths::writableLocation(QStandardPaths::DataLocation);

  if (!QDir(setting_path).exists()) {
    QDir d(setting_path);
    d.mkpath(setting_path);
  }

  settingsWidget = new QWidget(nullptr);
  settingsWidget->setObjectName("settingsWidget");
  settingsUi.setupUi(settingsWidget);
  settingsWidget->setWindowFlags(Qt::Dialog);
  settingsWidget->setWindowModality(Qt::WindowModal);
  settingsWidget->setMinimumWidth(
      settingsUi.mainWidget->minimumSizeHint().width() +
      settingsUi.scrollArea->minimumSizeHint().width() +
      settingsUi.scrollAreaWidgetContents->layout()->spacing());

  settingsUi.tracksToLoad->setRange(2, 10);

  // event filter to prevent wheel event on certain widgets
  foreach (QSlider *slider, settingsWidget->findChildren<QSlider *>()) {
    slider->installEventFilter(this);
  }
  foreach (QComboBox *box, settingsWidget->findChildren<QComboBox *>()) {
    box->installEventFilter(this);
  }
  foreach (QSpinBox *spinBox, settingsWidget->findChildren<QSpinBox *>()) {
    spinBox->installEventFilter(this);
  }

  settingsUi.loading_movie->setVisible(false);

  connect(settingsUi.donate, &QPushButton::clicked, [=]() {
    QDesktopServices::openUrl(QUrl("https://paypal.me/keshavnrj/5"));
    notify("", "Opening donation link in Sensible WebBrowser..");
  });

  connect(settingsUi.rate, &QPushButton::clicked, [=]() {
    QDesktopServices::openUrl(QUrl("snap://olivia"));
    notify("", "Opening Softwares App..");
  });

  connect(settingsUi.more_apps, &QPushButton::clicked, [=]() {
    QDesktopServices::openUrl(QUrl("https://snapcraft.io/search?q=keshavnrj"));
    notify("", "Opening Sensible WebBrowser..");
  });

  connect(settingsUi.open_tracks_cache_dir, &controlButton::clicked,
          [=]() { utils::desktopOpenUrl(setting_path + "/downloadedTracks"); });
  connect(settingsUi.open_video_cache_dir, &controlButton::clicked,
          [=]() { utils::desktopOpenUrl(setting_path + "/downloadedVideos"); });

  // TODO test dbOp algo before making it public
  settingsUi.optimizeDb->hide();
  connect(settingsUi.optimizeDb, &controlButton::clicked, [=]() {
    QMessageBox msgBox;
    msgBox.setText("This will remove unneccesary tracks meta data from databse "
                   "and make it smaller and faster.");
    msgBox.setIconPixmap(
        QPixmap(":/icons/sidebar/info.png")
            .scaled(42, 42, Qt::KeepAspectRatio, Qt::SmoothTransformation));

    msgBox.setInformativeText("Optimize Database ?");
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::Yes);
    int ret = msgBox.exec();
    switch (ret) {
    case QMessageBox::Yes: {
      store_manager->cleanUp();
      utils *util = new utils(this);
      settingsUi.database_size->setText(
          util->refreshCacheSize(setting_path + "/storeDatabase/" + database));
      util->deleteLater();
      clear_queue();
      loadPlayerQueue();
      break;
    }
    case QMessageBox::No:
      break;
    }
  });

  connect(settingsUi.download_engine, &controlButton::clicked,
          [=]() { emit engine->openSettingsAndClickDownload(); });
  connect(settingsUi.clear_engine_cache, &controlButton::clicked, [=]() {
    engine->clearEngineCache();
    QMovie *movie = new QMovie(":/icons/others/load.gif");
    settingsUi.loading_movie->setMovie(movie);
    settingsUi.loading_movie->setVisible(true);
    movie->start();
  });

  connect(settingsUi.tracksToLoad,
          static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
          [=](int value) {
            similarTracks->numberOfSimilarTracksToLoad = value;
            settingsObj.setValue("similarTracksToLoad", value);
          });

  connect(settingsUi.saveAfterBuffer, SIGNAL(toggled(bool)), settUtils,
          SLOT(changeSaveAfterSetting(bool)));

  connect(settingsUi.showSearchSuggestion, SIGNAL(toggled(bool)), settUtils,
          SLOT(changeShowSearchSuggestion(bool)));

  connect(settingsUi.dynamicTheme, SIGNAL(toggled(bool)), settUtils,
          SLOT(changeDynamicTheme(bool)));

  connect(settingsUi.mpris_checkBox, SIGNAL(toggled(bool)), settUtils,
          SLOT(changeMpris(bool)));

  connect(settingsUi.crossFade, &QCheckBox::toggled, [=](bool checked) {
    settUtils->changeCrossFadeSetting(checked);
    radio_manager->crossFadeEnabled = checked;
  });

  connect(settingsUi.smart_playlist_checkBox, &QCheckBox::toggled,
          [=](bool checked) {
            settingsUi.tracksToLoad->setEnabled(checked);
            settUtils->changeSmartPlaylist(checked);
          });

  connect(settingsUi.marquee, &QCheckBox::toggled, [=](bool checked) {
    settUtils->changeMarqueeSetting(checked);
    if (checked) {
      if (ui->nowP_title->text() != "-")
        ui->playing->setText(ui->nowP_title->text());
    } else {
      ui->playing->setText("");
    }
  });

  connect(settingsUi.visualizer, &QCheckBox::toggled, [=](bool checked) {
    settUtils->changeVisualizerSetting(checked);
    ui->vis_widget->setVisible(checked);
    if (!nowPlayingSongIdWatcher->getValue().isEmpty() &&
        nowPlayingSongIdWatcher->getValue() != "0000000") {
      playSongById(QVariant(nowPlayingSongIdWatcher->getValue()));
    }
    ui->cover->setVisible(!checked);
  });

  connect(settingsUi.systemTitlebar, &QCheckBox::toggled, [=](bool checked) {
    if (checked) {
      hide();
      ui->windowControls_main->hide();
      ui->systemTitleBarSpacerWidget->show();
      this->setWindowFlags(
          Qt::Window | Qt::WindowTitleHint | Qt::WindowSystemMenuHint |
          Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint |
          Qt::WindowFullscreenButtonHint);
      show();
    } else {
      hide();
      this->setWindowFlags(Qt::FramelessWindowHint | Qt::CustomizeWindowHint |
                           Qt::WindowTitleHint);
      ui->systemTitleBarSpacerWidget->hide();
      ui->windowControls_main->show();
      show();
    }
  });

  settingsUi.disable_notifications->setChecked(
      settingsObj.value("disable_notifications", false).toBool());
  connect(settingsUi.disable_notifications, &QCheckBox::toggled,
          [=](bool checked) {
            settingsObj.setValue("disable_notifications", checked);
          });

  // initial settings for app transparency
  settingsUi.appTransperancySlider->setRange(50, 100);
  settingsUi.appTransperancyLabel->setText(
      QString::number(int(this->windowOpacity() * 100)));
  settingsUi.appTransperancySlider->setValue(int(this->windowOpacity() * 100));
  int labelWidth = settingsUi.appTransperancyLabel->fontMetrics()
                       .boundingRect(settingsUi.appTransperancyLabel->text())
                       .width();
  int oneCharWidth = labelWidth / 3;
  settingsUi.appTransperancyLabel->setMinimumWidth(oneCharWidth * 4);
  settingsUi.transperancyLabel->setMinimumWidth(oneCharWidth * 4);

  connect(settingsUi.appTransperancySlider, &QSlider::valueChanged,
          [=](int val) {
            settUtils->changeAppTransperancy(val);
            this->setWindowOpacity(qreal(val) / 100);
            settingsUi.appTransperancyLabel->setText(
                QString::number(settingsUi.appTransperancySlider->value()));
          });
  connect(settingsUi.miniModeTransperancySlider, &QSlider::valueChanged,
          [=](int val) {
            settUtils->changeMiniModeTransperancy(val);
            settingsUi.transperancyLabel->setText(QString::number(
                settingsUi.miniModeTransperancySlider->value()));
          });
  connect(settingsUi.miniModeStayOnTop, &QCheckBox::toggled, [=](bool checked) {
    settUtils->changeMiniModeStayOnTop(checked);
    miniModeWidget->deleteLater();
    init_miniMode();
  });

  connect(settingsUi.delete_offline_pages, &QPushButton::clicked, [=]() {
    utils *util = new utils(this);
    if (util->delete_cache(setting_path + "/paginator")) {
      settingsUi.offline_pages_size->setText(
          util->refreshCacheSize(setting_path + "/paginator"));
      util->deleteLater();
    }
  });
  connect(settingsUi.delete_tracks_cache, &QPushButton::clicked, [=]() {
    QMessageBox msgBox;
    msgBox.setText("This will delete all downloaded songs!");
    msgBox.setIconPixmap(
        QPixmap(":/icons/sidebar/info.png")
            .scaled(42, 42, Qt::KeepAspectRatio, Qt::SmoothTransformation));

    msgBox.setInformativeText("Delete all downloaded songs cache ?");
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::No);
    int ret = msgBox.exec();
    switch (ret) {
    case QMessageBox::Yes: {
      utils *util = new utils(this);
      store_manager->delete_track_cache(setting_path + "/downloadedTracks");
      settingsUi.cached_tracks_size->setText(
          util->refreshCacheSize(setting_path + "/downloadedTracks"));
      util->deleteLater();
      clear_queue();
      loadPlayerQueue();
      break;
    }
    case QMessageBox::No:
      break;
    }
  });

  connect(settingsUi.delete_videos_cache, &QPushButton::clicked, [=]() {
    QMessageBox msgBox;
    msgBox.setText("This will delete all downloaded videos!");
    msgBox.setIconPixmap(
        QPixmap(":/icons/sidebar/info.png")
            .scaled(42, 42, Qt::KeepAspectRatio, Qt::SmoothTransformation));

    msgBox.setInformativeText("Delete all downloaded videos cache ?");
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::No);
    int ret = msgBox.exec();
    switch (ret) {
    case QMessageBox::Yes: {
      utils *util = new utils(this);
      util->delete_cache(setting_path + "/downloadedVideos");
      settingsUi.cached_videos_size->setText(
          util->refreshCacheSize(setting_path + "/downloadedVideos"));
      util->deleteLater();
      loadPlayerQueue();
      break;
    }
    case QMessageBox::No:
      break;
    }
  });

  connect(settingsUi.drop_database, &QPushButton::clicked, [=]() {
    QMessageBox msgBox;
    msgBox.setText("This will clear your local database !!");
    msgBox.setIconPixmap(
        QPixmap(":/icons/sidebar/info.png")
            .scaled(42, 42, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    msgBox.setInformativeText("Delete local database ? \n\nThis only include "
                              "database, downloaded songs cache will remain.");
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::No);
    int ret = msgBox.exec();
    switch (ret) {
    case QMessageBox::Yes: {
      utils *util = new utils(this);
      if (util->delete_cache(setting_path + "/storeDatabase/" + database)) {
        settingsUi.database_size->setText(util->refreshCacheSize(
            setting_path + "/storeDatabase/" + database));
        util->deleteLater();
        clear_queue();
        restart_required();
      }
      break;
    }
    case QMessageBox::No:
      break;
    }
  });

  connect(settingsUi.plus, SIGNAL(clicked(bool)), this, SLOT(zoomin()));
  connect(settingsUi.minus, SIGNAL(clicked(bool)), this, SLOT(zoomout()));

  connect(settingsUi.backup_restore, &QPushButton::clicked,
          [=]() { openBackupUtil(); });

  connect(
      settingsUi.closeButtonActionCombo,
      static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
      [=](int index) {
        settingsObj.setValue("closeButtonActionCombo", index);
      });

  connect(
      settingsUi.notificationCombo,
      static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
      [=](int index) { settingsObj.setValue("notificationCombo", index); });

  connect(settingsUi.tryNotification, &QPushButton::clicked, [=]() {
    notify(QApplication::applicationName(), tr("Test Notification"));
  });

  settingsUi.zoom->setText(QString::number(ui->webview->zoomFactor(), 'f', 2));
  add_colors_to_color_widget();

  // show hide vis
  ui->cover->setVisible(!settingsObj.value("visualizer").toBool());
  ui->vis_widget->setVisible(settingsObj.value("visualizer").toBool());

  settingsWidget->setMinimumWidth(680);
}

bool MainWindow::isChildOf(QObject *Of, QObject *self) {
  bool ischild = false;
  if (Of->findChild<QWidget *>(self->objectName())) {
    // qDebug()<<self->objectName()<<"is child of"<<Of;
    ischild = true;
  }
  return ischild;
}

void MainWindow::openBackupUtil() {
  if (settingsWidget->isVisible()) {
    settingsWidget->close();
  }
  backup->adjustSize();
  backup->setFixedSize(backup->size());
  backup->check_last_backup();
  backup->fixTheme();
  backup->showNormal();
}

void MainWindow::restart_required() {
  QMessageBox msgBox;
  msgBox.setText("Application need to restart!");
  msgBox.setIconPixmap(
      QPixmap(":/icons/sidebar/info.png")
          .scaled(42, 42, Qt::KeepAspectRatio, Qt::SmoothTransformation));
  msgBox.setInformativeText("Please restart application to load new changes.");
  msgBox.setStandardButtons(QMessageBox::Ok);
  msgBox.setDefaultButton(QMessageBox::Ok);
  msgBox.exec();
  quitApp();
}

void MainWindow::dynamicThemeChanged(bool enabled) {
  if (enabled) {
    QString color = store_manager->getDominantColor(
        store_manager->getAlbumId(nowPlayingSongIdWatcher->getValue()));
    int r, g, b;
    if (color.split(",").count() > 2) {
      r = color.split(",").at(0).toInt();
      g = color.split(",").at(1).toInt();
      b = color.split(",").at(2).toInt();
      set_app_theme(QColor(r, g, b));
      // qDebug()<<QColor(r,g,b);
    }
  }
  settingsUi.themesWidget->setEnabled(!enabled);
}

void MainWindow::loadSettings() {

  // load notification type
  settingsUi.notificationCombo->setCurrentIndex(
      settingsObj.value("notificationCombo", 1).toInt());

  // load close button action settings
  settingsUi.closeButtonActionCombo->setCurrentIndex(
      settingsObj.value("closeButtonActionCombo", 0).toInt());

  int repeat = settingsObj.value("repeat", 0).toInt();
  switch (repeat) {
  case 0:
    ui->repeat->setCheckState(Qt::Unchecked);
    ui->repeat->setIcon(QIcon(":/icons/p_repeat_off.png"));
    break;
  case 1:
    ui->repeat->setCheckState(Qt::PartiallyChecked);
    ui->repeat->setIcon(QIcon(":/icons/p_repeat.png"));
    // disable shuffle if repeat current song is on
    if (ui->shuffle->isChecked()) {
      ui->shuffle->setChecked(false);
    }
    break;
  case 2:
    ui->repeat->setCheckState(Qt::Checked);
    ui->repeat->setIcon(QIcon(":/icons/p_repeat_queue.png"));
    break;
  default:
    break;
  }

  ui->shuffle->setChecked(settingsObj.value("shuffle").toBool());
  // disable shuffle if repeat current song is on
  if (ui->repeat->checkState() == Qt::PartiallyChecked) {
    ui->shuffle->setChecked(false);
  }
  ui->shuffle->setIcon(QIcon(ui->shuffle->isChecked()
                                 ? ":/icons/shuffle_button.png"
                                 : ":/icons/shuffle_button_disabled.png"));

  settingsUi.saveAfterBuffer->setChecked(
      settingsObj.value("saveAfterBuffer", "true").toBool());
  settingsUi.showSearchSuggestion->setChecked(
      settingsObj.value("showSearchSuggestion", "true").toBool());
  settingsUi.miniModeStayOnTop->setChecked(
      settingsObj.value("miniModeStayOnTop", "false").toBool());
  settingsUi.crossFade->setChecked(
      settingsObj.value("crossfade", "false").toBool());
  settingsUi.visualizer->setChecked(
      settingsObj.value("visualizer", "false").toBool());
  settingsUi.mpris_checkBox->setChecked(
      settingsObj.value("mpris", "false").toBool());
  settingsUi.smart_playlist_checkBox->setChecked(
      settingsObj.value("smart_playlist", "false").toBool());
  settingsUi.tracksToLoad->setValue(
      settingsObj.value("similarTracksToLoad", 8).toInt());

  // set value for similar tracks class
  similarTracks->numberOfSimilarTracksToLoad =
      settingsUi.tracksToLoad->text().toInt();
  settingsUi.dynamicTheme->setChecked(
      settingsObj.value("dynamicTheme", "false").toBool());
  settingsUi.marquee->setChecked(settingsObj.value("marquee", false).toBool());

  // restore mini mode tranparency
  settingsUi.miniModeTransperancySlider->setValue(
      settingsObj.value("miniModeTransperancy", 95).toInt());
  settingsUi.transperancyLabel->setText(
      QString::number(settingsUi.miniModeTransperancySlider->value()));

  // restore app tranparency
  settingsUi.appTransperancySlider->setValue(
      settingsObj.value("appTransperancy", 100).toInt());
  settingsUi.appTransperancyLabel->setText(
      QString::number(settingsUi.appTransperancySlider->value()));

  // keep this after init of settings widget
  if (settingsObj.value("dynamicTheme").toBool() == false) {
    QString rgbhash = settingsObj.value("customTheme", "#3BBAC6").toString();
    set_app_theme(QColor(rgbhash));
    if (color_list.contains(rgbhash, Qt::CaseInsensitive)) {
      if (settingsWidget->findChild<QPushButton *>(rgbhash.toUpper())) {
        settingsWidget->findChild<QPushButton *>(rgbhash.toUpper())
            ->setText("*");
      }
    }
  } else {
    QString rgbhash = settingsObj.value("customTheme", "#3BBAC6").toString();
    set_app_theme(QColor(rgbhash));
  }

  ui->tabWidget->setCurrentIndex(
      settingsObj.value("currentQueueTab", "0").toInt());
  restoreGeometry(settingsObj.value("geometry").toByteArray());
  restoreState(settingsObj.value("windowState").toByteArray());

  settingsUi.systemTitlebar->setChecked(
      settingsObj.value("systemTitlebar", "true").toBool());
  // load the system titlebar settings if its set to false, this is not
  // following the connection we already having cause
  //  toggle is not being triggered by default on checkbox due to its initial
  //  false state.
  if (!settingsObj.value("systemTitlebar", "true").toBool()) {
    this->setWindowFlags(Qt::FramelessWindowHint | Qt::CustomizeWindowHint |
                         Qt::WindowTitleHint);
    ui->windowControls_main->show();
    ui->systemTitleBarSpacerWidget->hide();
  }
}

void MainWindow::add_colors_to_color_widget() {

  color_list << "fakeitem"
             << "#FF0034"
             << "#2A82DA"
             << "#029013"
             << "#D22298"
             << "#FF901F"
             << "#2B2929";

  QObject *layout =
      settingsWidget->findChild<QObject *>("themeHolderGridLayout");
  int row = 0;
  int numberOfButtons = 0;
  while (numberOfButtons <= color_list.count()) {
    for (int f2 = 0; f2 < 3; f2++) {
      numberOfButtons++;
      if (numberOfButtons > color_list.count() - 1)
        break;
      QPushButton *pb = new QPushButton();
      pb->setObjectName(color_list.at(numberOfButtons));
      pb->setStyleSheet("background-color:" + color_list.at(numberOfButtons) +
                        ";border:1px;padding:2px;");
      connect(pb, &QPushButton::clicked, [=]() {
        set_app_theme(QColor(pb->objectName()));

        for (int i(0); i < color_list.count(); i++) {
          if (settingsWidget->findChild<QPushButton *>(color_list.at(i))) {
            settingsWidget->findChild<QPushButton *>(color_list.at(i))
                ->setText("");
          }
        }
        pb->setText("*");
      });
      static_cast<QGridLayout *>(layout)->addWidget(pb, row, f2);
    }
    row++;
  }
  QPushButton *pb = new QPushButton();
  pb->setIcon(QIcon(":/icons/picker.png"));
  pb->setIconSize(QSize(15, 15));
  pb->setObjectName("custom_color");
  pb->setText("Select color");
  pb->setToolTip("Choose custom color from color dialog.");
  connect(pb, SIGNAL(clicked(bool)), this, SLOT(customColor()));
  static_cast<QGridLayout *>(layout)->addWidget(pb, row + 1, 0);
}

void MainWindow::set_app_theme(QColor rgb) {
  settingsObj.setValue("customTheme", rgb);

  QString r = QString::number(rgb.red());
  QString g = QString::number(rgb.green());
  QString b = QString::number(rgb.blue());

  if (!color_list.contains(rgb.name(), Qt::CaseInsensitive)) {
    for (int i(0); i < color_list.count(); i++) {
      if (settingsWidget->findChild<QPushButton *>(color_list.at(i))) {
        settingsWidget->findChild<QPushButton *>(color_list.at(i))->setText("");
      }
    }
  }

  themeColor = r + "," + g + "," + b + "," + "0.2";

  this->setStyleSheet("QMainWindow{"
                      "background-color:rgba(" +
                      r + "," + g + "," + b + "," + "0.1" +
                      ");"
                      "background-image:url(:/icons/texture.png), "
                      "linear-gradient(hsla(0,0%,32%,.99), hsla(0,0%,27%,.95));"
                      "}");

  QString rgba = r + "," + g + "," + b + "," + "0.2";
  ui->webview->page()->mainFrame()->evaluateJavaScript("changeBg('" + rgba +
                                                       "')");
  QString widgetStyle = "background-color:"
                        "qlineargradient(spread:pad, x1:0, y1:0, x2:0, y2:1,"
                        "stop:0.129213 rgba(" +
                        r + ", " + g + ", " + b +
                        ", 30),"
                        "stop:0.38764 rgba(" +
                        r + ", " + g + ", " + b +
                        ", 120),"
                        "stop:0.679775 rgba(" +
                        r + ", " + g + ", " + b +
                        ", 84),"
                        "stop:1 rgba(" +
                        r + ", " + g + ", " + b + ", 30));";

  ui->left_panel->setStyleSheet("QWidget#left_panel{" + widgetStyle + "}");
  ui->right_panel->setStyleSheet("QWidget#right_panel{" + widgetStyle + "}");

  ui->recommWidget->setStyleSheet("QWidget#recommWidget{" + widgetStyle +
                                  ";border:none;}");

  ui->show_hide_smart_list_button->setStyleSheet(
      "QPushButton#show_hide_smart_list_button{" + widgetStyle +
      ";border:none;padding-top:3px;padding-bottom:3px;}");
  ui->clear->setStyleSheet("QPushButton#clear{" + widgetStyle +
                           ";border:none;padding-top:3px;padding-bottom:3px;}");

  smartModeWidget->setStyleSheet(
      ui->left_panel->styleSheet().replace("#left_panel", "#smartModeWidget"));

  miniModeWidget->setStyleSheet(
      ui->left_panel->styleSheet().replace("#left_panel", "#miniModeWidget"));

  ui->search->setStyleSheet(widgetStyle + "border:none;border-radius:0px;");
  ui->label_5->setStyleSheet(widgetStyle + "border:none;border-radius:0px;");

  settingsWidget->setStyleSheet("QWidget#settingsWidget{" + widgetStyle + "}");
  ui->tabWidget->setStyleSheet("QTabWidget#tabWidget{" + widgetStyle +
                               "}"); // to remove style set by designer

  ui->stream_info->setStyleSheet("QWidget#stream_info{" + widgetStyle +
                                 "}"); // to remove style set by designer

  QString btn_style =
      "QPushButton{color: silver; background-color: #45443F; border:1px solid "
      "#272727; padding-top: 3px; padding-bottom: 3px; padding-left: 3px; "
      "padding-right: 3px; border-radius: 2px; outline: none;}"
      "QPushButton:disabled { background-color: #45443F; border:1px solid "
      "#272727; padding-top: 3px; padding-bottom: 3px; padding-left: 5px; "
      "padding-right: 5px; /*border-radius: 2px;*/ color: #636363;}"
      "QPushButton:hover{border: 1px solid #272727;background-color:#5A584F; "
      "color:silver ;}"
      "QPushButton:pressed {background-color: #45443F;color: "
      "silver;padding-bottom:1px;}";

  ui->show_hide_smart_list_button->setStyleSheet(
      widgetStyle + ";border:none;padding-top:3px;padding-bottom:3px;");
  ui->ytdlRefreshAll->setStyleSheet(btn_style);
  ui->ytdlStopAll->setStyleSheet(btn_style);

  if (eq != nullptr) {
    eq->setStyleSheet("QWidget#equalizer{" + ui->search->styleSheet() + "}" +
                      "QFrame{" + ui->search->styleSheet() + "}");
    eq->removeStyle();
  }
  if (backup != nullptr) {
    backup->setStyleSheet("QWidget#Backup{" + ui->search->styleSheet() + "}" +
                          "QFrame{" + ui->search->styleSheet() + "}");
  }
}

void MainWindow::customColor() {
  SelectColorButton *s = new SelectColorButton();
  connect(s, SIGNAL(setCustomColor(QColor)), this, SLOT(set_app_theme(QColor)));
  s->changeColor();
}

void SelectColorButton::updateColor() { emit setCustomColor(color); }

void SelectColorButton::changeColor() {
  QColor newColor =
      QColorDialog::getColor(color, parentWidget(), "Choose theme color",
                             QColorDialog::DontUseNativeDialog);
  if (newColor != color) {
    setColor(newColor);
  }
}

void SelectColorButton::setColor(const QColor &color) {
  this->color = color;
  updateColor();
}

const QColor &SelectColorButton::getColor() { return color; }

// set up app #1
void MainWindow::init_app() {
  ui->playlistLoaderButtton->hide();

  ui->favourite->setChecked(false);
  ui->favourite->setEnabled(false);
  ui->favourite->setIcon(QIcon(":/icons/sidebar/liked_disabled.png"));

  ui->olivia_list->setDragDropMode(QAbstractItemView::NoDragDrop);
  ui->youtube_list->setDragDropMode(QAbstractItemView::NoDragDrop);
  ui->smart_list->setDragDropMode(QAbstractItemView::NoDragDrop);

  ui->debug_widget->hide();
  // sets search icon in label
  ui->label_5->resize(ui->label_5->width(), ui->search->height());
  ui->label_5->setPixmap(
      QPixmap(":/icons/sidebar/search.png")
          .scaled(18, 18, Qt::KeepAspectRatio, Qt::SmoothTransformation));
  qApp->setQuitOnLastWindowClosed(false);

  // init pagination class
  pagination_manager = new paginator(this);
  connect(pagination_manager, SIGNAL(reloadRequested(QString, QString)), this,
          SLOT(reloadREquested(QString, QString)));

  // init youtube class
  youtube = new Youtube(this, ui->webview, pagination_manager);
  youtube->setObjectName("youtube");
  connect(youtube, SIGNAL(setCountry(QString)), this,
          SLOT(setCountry(QString)));

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

  connect(split2, &QSplitter::splitterMoved, [=](int pos) {
    if (animationRunning == false) {
      left_panel_width = pos;
    }
  });

  QSplitter *split3 = new QSplitter;
  split3->setObjectName("split3");
  split3->addWidget(ui->queueHolder);
  split3->addWidget(ui->smart_playlist_holder);
  split3->setOrientation(Qt::Vertical);

  ui->verticalLayout_3->addWidget(split3);

  split3->setCollapsible(1, false);

  ui->similarTrackLoader->setRoundness(70.0);
  ui->similarTrackLoader->setMinimumTrailOpacity(15.0);
  ui->similarTrackLoader->setTrailFadePercentage(70.0);
  ui->similarTrackLoader->setNumberOfLines(10);
  ui->similarTrackLoader->setLineLength(5);
  ui->similarTrackLoader->setLineWidth(2);
  ui->similarTrackLoader->setInnerRadius(2);
  ui->similarTrackLoader->setRevolutionsPerSecond(1);
  ui->similarTrackLoader->setColor(QColor(227, 222, 222));
  ui->similarTrackLoader->installEventFilter(this);

  ui->webview_loader->setRoundness(70.0);
  ui->webview_loader->setMinimumTrailOpacity(15.0);
  ui->webview_loader->setTrailFadePercentage(70.0);
  ui->webview_loader->setNumberOfLines(10);
  ui->webview_loader->setLineLength(5);
  ui->webview_loader->setLineWidth(2);
  ui->webview_loader->setInnerRadius(2);
  ui->webview_loader->setRevolutionsPerSecond(1);
  ui->webview_loader->setColor(QColor(227, 222, 222));
  ui->webview_loader->installEventFilter(this);

  // hide recommWidget this code is  from
  // on_show_hide_smart_list_button_clicked()
  split3->widget(1)->setMaximumHeight(
      ui->show_hide_smart_list_button->height());
  split3->setSizes(QList<int>() << 300 << 0);
  split3->handle(1)->setEnabled(false);
  ui->recommWidget->hide();
  ui->show_hide_smart_list_button->setToolTip("Show");

  setWindowIcon(QIcon(":/icons/olivia.png"));
  setWindowTitle(QApplication::applicationName());

  ElidedLabel *artist = new ElidedLabel("-", nullptr);
  ElidedLabel *album = new ElidedLabel("-", nullptr);

  album->setObjectName("nowP_album");
  artist->setObjectName("nowP_artist");

  album->setAlignment(Qt::AlignHCenter);
  artist->setAlignment(Qt::AlignHCenter);

  ui->artist_horizontalLayout->addWidget(artist);
  ui->album_horizontalLayout->addWidget(album);

  QString btn_style_2 =
      "QPushButton{background-color:transparent ;border:0px;}"
      "QPushButton:disabled { background-color: transparent; border-bottom:1px "
      "solid #727578;"
      "padding-top: 3px; padding-bottom: 3px; padding-left: 5px; "
      "padding-right: 5px;color: #636363;}"
      "QPushButton:pressed "
      "{padding-bottom:0px;background-color:transparent;border:0px;}"
      "QPushButton:hover "
      "{border:none;padding-bottom:1px;background-color:transparent;border:0px;"
      "}";

  foreach (QPushButton *button,
           ui->windowControls->findChildren<QPushButton *>()) {
    button->setStyleSheet(btn_style_2);
  }

  QString btn_style_3 =
      "QPushButton{background-color:transparent ;border:0px;padding-top: 3px; "
      "padding-bottom: 3px;}"
      "QPushButton:disabled {padding-top: 3px; padding-bottom:3px;}"
      "QPushButton:pressed {padding-bottom:0px;border:0px;}"
      "QPushButton:hover "
      "{padding-bottom:1px;background-color:transparent;border:0px;}";

  foreach (QPushButton *button,
           ui->controls_widget->findChildren<QPushButton *>()) {
    button->setStyleSheet(btn_style_3);
  }
  ui->state->hide();
  connect(ui->state, &QLineEdit::textChanged, [=]() {
    utils *util = new utils(this);
    ui->visible_state->setTextFormat(Qt::RichText);
    ui->visible_state->setText(
        "<p align=\"center\" ><span style=\"font-size:11pt;\">" +
        util->toCamelCase(ui->state->text()) + "</span></p>");
    util->deleteLater();
  });

  // init nowPlayingSongId change watcher
  nowPlayingSongIdWatcher = new stringChangeWatcher(this);
  connect(nowPlayingSongIdWatcher, &stringChangeWatcher::valueChanged, [=]() {
    QString songId = nowPlayingSongIdWatcher->getValue();
    ui->webview->page()->mainFrame()->evaluateJavaScript("setNowPlaying('" +
                                                         songId + "')");
    qDebug() << "nowPlayingSongIdWatcher nowPlayingId=" << songId
             << "isRadioStation=" << nowPlayingSongIdWatcher->isRadioStation;
  });
}

void MainWindow::show_SysTrayIcon() {
  QAction *minimizeAction = new QAction(QObject::tr("&Hide"), this);
  this->connect(minimizeAction, SIGNAL(triggered()), this, SLOT(hide()));

  QAction *restoreAction = new QAction(QObject::tr("&Restore"), this);
  this->connect(restoreAction, SIGNAL(triggered()), this, SLOT(show()));

  QAction *quitAction = new QAction(QObject::tr("&Quit"), this);
  this->connect(quitAction, SIGNAL(triggered()), this, SLOT(quitApp()));

  QMenu *trayIconMenu = new QMenu(this);
  trayIconMenu->addAction(restoreAction);
  trayIconMenu->addAction(minimizeAction);
  trayIconMenu->addSeparator();
  trayIconMenu->addAction(quitAction);

  trayIcon = new QSystemTrayIcon(this);
  trayIcon->setContextMenu(trayIconMenu);
  trayIconMenu->setObjectName("trayIconMenu");

  trayIcon->setIcon(QIcon(":/icons/app/icon-32.png"));
  connect(trayIconMenu, SIGNAL(aboutToShow()), this,
          SLOT(check_window_state()));
  if (trayIcon->isSystemTrayAvailable()) {
    trayIcon->show();
  } else {
    trayIcon->deleteLater();
    trayIcon = nullptr;
    QMessageBox::information(
        this, "System tray:",
        "System tray not found, Please install a system tray in your system.",
        QMessageBox::Ok);
  }
  // init widget notification
  notificationPopup = new NotificationPopup(0);
  notificationPopup->setWindowFlags(Qt::ToolTip | Qt::WindowStaysOnTopHint |
                                    Qt::FramelessWindowHint);
  notificationPopup->setWindowModality(Qt::NonModal);
  notificationPopup->adjustSize();
  connect(notificationPopup, &NotificationPopup::notification_clicked, [=]() {
    this->show();
    this->setWindowState((this->windowState() & ~Qt::WindowMinimized) |
                         Qt::WindowActive);
    this->raise();
  });
}

// check window state and set tray menus
void MainWindow::check_window_state() {
  QObject *tray_icon_menu = this->findChild<QObject *>("trayIconMenu");
  if (tray_icon_menu != nullptr) {
    if (this->isVisible()) {
      ((QMenu *)(tray_icon_menu))->actions().at(1)->setDisabled(false);
      ((QMenu *)(tray_icon_menu))->actions().at(0)->setDisabled(true);
    } else {
      ((QMenu *)(tray_icon_menu))->actions().at(1)->setDisabled(true);
      ((QMenu *)(tray_icon_menu))->actions().at(0)->setDisabled(false);
    }
  }
}

// used to set county settings in youtube right now
void MainWindow::setCountry(QString country) {
  if (pageType == "youtube") {
    ui->webview->page()
        ->mainFrame()
        ->findFirstElement("#currentCountry")
        .setInnerXml(country);
    ui->webview->page()
        ->mainFrame()
        ->findFirstElement("#home")
        .evaluateJavaScript("this.click()");
  }
}

// set up webview #2
void MainWindow::init_webview() {

  connect(ui->webview, SIGNAL(loadFinished(bool)), this,
          SLOT(webViewLoaded(bool)));
  connect(ui->webview, &QWebView::loadStarted,
          [=]() { ui->webview_loader->start(); });

  // websettings---------------------------------------------------------------
  ui->webview->page()->setLinkDelegationPolicy(QWebPage::DelegateAllLinks);
  ui->webview->page()->settings()->setAttribute(
      QWebSettings::LocalContentCanAccessRemoteUrls, true);
  ui->webview->page()->settings()->setAttribute(
      QWebSettings::LocalContentCanAccessFileUrls, true);
  connect(ui->webview->page(), &QWebPage::linkClicked, [=](QUrl url) {
    QString link = url.toString();
    if (link.contains("://paypal.me/keshavnrj/5")) {
      showPayPalDonationMessageBox();
    } else if (link.contains("how-to-create-youtube-api-key-2020")) {
      QDesktopServices::openUrl(url);
    } else {
      ui->webview->load(url);
    }
  });
  QString setting_path =
      QStandardPaths::writableLocation(QStandardPaths::DataLocation);
  QString cookieJarPath;
  if (setting_path.split("/").last().isEmpty()) {
    cookieJarPath = setting_path + "/cookiejar_olivia.dat";
  } else {
    cookieJarPath = setting_path + "cookiejar_olivia.dat";
  }

#ifdef QT_DEBUG
  QWebSettings::globalSettings()->setAttribute(
      QWebSettings::DeveloperExtrasEnabled, true);
#else
  QWebSettings::globalSettings()->setAttribute(
      QWebSettings::DeveloperExtrasEnabled, false);
  ui->webview->setContextMenuPolicy(Qt::NoContextMenu);
#endif

  ui->webview->settings()->setAttribute(QWebSettings::LocalStorageEnabled,
                                        true);
  ui->webview->settings()->enablePersistentStorage(setting_path);

  QWebSettings::globalSettings()->setAttribute(QWebSettings::JavascriptEnabled,
                                               true);
  QWebSettings::globalSettings()->setAttribute(QWebSettings::PluginsEnabled,
                                               false);
  ui->webview->page()->settings()->setMaximumPagesInCache(0);
  ui->webview->page()->settings()->setAttribute(QWebSettings::PluginsEnabled,
                                                false);
  QNetworkDiskCache *diskCache = new QNetworkDiskCache(this);
  diskCache->setCacheDirectory(setting_path);
  ui->webview->page()->networkAccessManager()->setCache(diskCache);
  ui->webview->page()->networkAccessManager()->setCookieJar(new CookieJar(
      cookieJarPath, ui->webview->page()->networkAccessManager()));

  horizontalDpi = QGuiApplication::primaryScreen()->logicalDotsPerInchX();

  if (!settingsObj.value("zoom").isValid()) {
    zoom = 100.0;
    setZoom(zoom);
  } else {
    zoom = settingsObj.value("zoom", "100.0").toReal();
    setZoom(zoom);
  }
}

void MainWindow::showPayPalDonationMessageBox() {
  QMessageBox msgBox;
  QString btn_style =
      "QPushButton{color: silver; background-color: #45443F; border:1px solid "
      "#272727; padding-top: 3px; padding-bottom: 3px; padding-left: 3px; "
      "padding-right: 3px; border-radius: 2px; outline: none;}"
      "QPushButton:disabled { background-color: #45443F; border:1px solid "
      "#272727; padding-top: 3px; padding-bottom: 3px; padding-left: 5px; "
      "padding-right: 5px; /*border-radius: 2px;*/ color: #636363;}"
      "QPushButton:hover{border: 1px solid #272727;background-color:#5A584F; "
      "color:silver ;}"
      "QPushButton:pressed {background-color: #45443F;color: "
      "silver;padding-bottom:1px;}";

  msgBox.setWindowTitle(qApp->applicationName() + "- Donation");
  msgBox.setText("You clicked donation button,");
  msgBox.setIconPixmap(
      QPixmap(":/icons/sidebar/info.png")
          .scaled(42, 42, Qt::KeepAspectRatio, Qt::SmoothTransformation));
  msgBox.setInformativeText(
      "Do you want proceed to load donation page in Olivia? "
      "<center>or</center> Do you want to donate from a external browser by "
      "copying donation link ?");
  QAbstractButton *donateHereBtn =
      msgBox.addButton(tr(" &Donate here"), QMessageBox::ActionRole);
  QAbstractButton *copyLinkBtn = msgBox.addButton(
      tr(" &Open in external brwoser"), QMessageBox::ActionRole);
  donateHereBtn->setIcon(QIcon(":/icons/micro/redo.png"));
  copyLinkBtn->setIcon(QIcon(":/icons/micro/url.png"));
  donateHereBtn->setStyleSheet(btn_style);
  copyLinkBtn->setStyleSheet(btn_style);
  msgBox.addButton(tr("Close"), QMessageBox::RejectRole)->hide();

  msgBox.exec();
  if (msgBox.clickedButton() == donateHereBtn) {
    ui->webview->load(QUrl("https://paypal.me/keshavnrj/5"));
    notify("", "Opening doantion link please wait");
  } else if (msgBox.clickedButton() == copyLinkBtn) {
    QClipboard *clipboard = QGuiApplication::clipboard();
    clipboard->setText("https://paypal.me/keshavnrj/5");
    notify("", "Donation link copied, opening in browser");
    QDesktopServices::openUrl(QUrl("https://paypal.me/keshavnrj/5"));
  }
}

void MainWindow::init_offline_storage() {
  QString setting_path =
      QStandardPaths::writableLocation(QStandardPaths::DataLocation);
  QDir albumArt(setting_path + "/albumArts");
  if (!albumArt.exists()) {
    if (albumArt.mkdir(albumArt.path())) {
      qDebug() << "created albumArts dir";
    }
  }
  QDir downloaded(setting_path + "/downloadedTracks");
  if (!downloaded.exists()) {
    if (downloaded.mkdir(downloaded.path())) {
      qDebug() << "created downloadedTracks dir";
    }
  }
  QDir downloadedVideo(setting_path + "/downloadedVideos");
  if (!downloadedVideo.exists()) {
    if (downloadedVideo.mkdir(downloadedVideo.path())) {
      qDebug() << "created downloadedVideos dir";
    }
  }
  QDir downloadedTemp(setting_path + "/downloadedTemp");
  if (!downloadedTemp.exists()) {
    if (downloadedTemp.mkdir(downloadedTemp.path())) {
      qDebug() << "created downloadedTemp dir";
    }
  }
  QDir database(setting_path + "/storeDatabase");
  if (!database.exists()) {
    if (database.mkdir(database.path())) {
      qDebug() << "created storeDatabase dir";
    }
  }
  QDir convert(setting_path + "/converted_temp");
  if (!convert.exists()) {
    if (convert.mkdir(convert.path())) {
      qDebug() << "created convert dir";
    }
  }
}

void MainWindow::loadPlayerQueue() { //  #7
  QString setting_path =
      QStandardPaths::writableLocation(QStandardPaths::DataLocation);
  foreach (QStringList trackMetaList, store_manager->getPlayerQueue()) {
    QString id, title, artist, album, base64, dominantColor, songId, albumId,
        artistId, url;
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

    QString plainTitle = htmlToPlainText(title);

    QWidget *track_widget = new QWidget(ui->olivia_list);
    track_widget->setToolTip(htmlToPlainText(title));
    track_widget->setObjectName("track-widget-" + songId);
    track_ui.setupUi(track_widget);

    // update ytid for track if the don't exists
    if (id.isEmpty()) {
      QStringList trackmeta = store_manager->getTrack(songId);
      QString title = trackmeta.at(1);
      QString artist = trackmeta.at(5);
      QString query =
          title.replace("N/A", "") + " - " + artist.replace("N/A", "");
      prepareTrack(
          songId, query, "0",
          this->findChild<QListWidget *>(getCurrentPlayerQueue(songId)));
    }

    // update author for youtube videos who don't have author/artist from web
    if (artist.isEmpty() &&
        albumId.contains("undefined", Qt::CaseInsensitive)) {
      request = new Request(this);
      request->setObjectName(songId);
      connect(request, &Request::requestFinished,
              [=](QString uid, QString reply) {
                if (uid == songId) {
                  qDebug() << "updated track's author";
                  store_manager->update_track("artistId", songId, reply);
                  store_manager->saveArtist(reply, reply);
                  updateTrackInQueues(songId);
                }
                this->findChild<Request *>(uid)->deleteLater();
              });
      request->get(
          QUrl(
              "http://ktechpit.com/USS/Olivia/invidio_get_video.php?video_id=" +
              songId + "&author"),
          songId);
    }

    // set track meta icon
    if (albumId.contains("soundcloud")) {
      track_widget->setStyleSheet(
          "QWidget#track-widget-" + songId +
          "{background: transparent url(':/icons/micro/soundcloud_micro.png') "
          "no-repeat bottom right}");
    }

    track_ui.meta->hide();

    QFont font("Ubuntu");
    font.setPixelSize(12);

    ElidedLabel *titleLabel = new ElidedLabel(plainTitle, track_widget);
    titleLabel->setFont(font);
    titleLabel->setObjectName("title_elided");
    track_ui.verticalLayout_2->addWidget(titleLabel);

    ElidedLabel *artistLabel = new ElidedLabel(artist, track_widget);
    artistLabel->setObjectName("artist_elided");
    artistLabel->setFont(font);
    track_ui.verticalLayout_2->addWidget(artistLabel);

    ElidedLabel *albumLabel = new ElidedLabel(album, track_widget);
    albumLabel->setObjectName("album_elided");
    albumLabel->setFont(font);
    track_ui.verticalLayout_2->addWidget(albumLabel);

    track_ui.dominant_color->setText(dominantColor);
    track_ui.songId->setText(songId);
    track_ui.playing->setPixmap(QPixmap(":/icons/blank.png")
                                    .scaled(track_ui.playing->size(),
                                            Qt::KeepAspectRatio,
                                            Qt::SmoothTransformation));

    track_ui.songId->hide();
    track_ui.dominant_color->hide();
    track_ui.id->hide();
    if (store_manager->isDownloaded(songId)) {
      track_ui.id->setText(store_manager->getYoutubeIds(songId));
      track_ui.url->setText("file://" + setting_path + "/downloadedTracks/" +
                            songId);
      track_ui.offline->setPixmap(QPixmap(":/icons/offline.png")
                                      .scaled(track_ui.offline->size(),
                                              Qt::KeepAspectRatio,
                                              Qt::SmoothTransformation));
    } else {
      track_ui.url->setText(url);
    }
    track_ui.url->hide();
    track_ui.option->setObjectName(songId + "optionButton");
    connect(track_ui.option, SIGNAL(clicked(bool)), this,
            SLOT(showTrackOption()));

    base64 = base64.split("base64,").last();
    QByteArray ba = base64.toUtf8();
    QPixmap image;
    image.loadFromData(QByteArray::fromBase64(ba));
    if (!image.isNull()) {
      track_ui.cover->setPixmap(image);
    }

    if (albumId.contains("undefined-")) {
      track_ui.id->setText(id);
      QListWidgetItem *item;
      item = new QListWidgetItem(ui->youtube_list);
      // set size for track widget
      track_ui.cover->setMaximumSize(149, 90);
      track_ui.cover->setMinimumSize(149, 79);
      track_ui.widget->adjustSize();
      item->setSizeHint(track_widget->minimumSizeHint());
      ui->youtube_list->setItemWidget(item, track_widget);

      // checks if url is expired and updates item with new url which can be
      // streamed .... until keeps the track item disabled.
      if (url.isEmpty() || (store_manager->getExpiry(songId) &&
                            track_ui.url->text().contains("http"))) {
        // track_ui.loading->setPixmap(QPixmap(":/icons/url_issue.png").scaled(track_ui.loading->size(),Qt::KeepAspectRatio,Qt::SmoothTransformation));
        ui->youtube_list->itemWidget(item)->setEnabled(false);
        if (!track_ui.id->text().isEmpty()) {
          getAudioStream(track_ui.id->text().trimmed(),
                         track_ui.songId->text().trimmed());
        }
      }
      ui->youtube_list->addItem(item);
    } else {
      track_ui.id->setText(id);
      QListWidgetItem *item;
      item = new QListWidgetItem(ui->olivia_list);
      item->setSizeHint(track_widget->minimumSizeHint());
      ui->olivia_list->setItemWidget(item, track_widget);
      // checks if url is expired and updates item with new url which can be
      // streamed .... until keeps the track item disabled.
      if (url.isEmpty() || (store_manager->getExpiry(songId) &&
                            track_ui.url->text().contains("http"))) {
        ui->olivia_list->itemWidget(item)->setEnabled(false);
        if (!track_ui.id->text().isEmpty()) {
          getAudioStream(track_ui.id->text().trimmed(),
                         track_ui.songId->text().trimmed());
        } else {
          QString query =
              title.replace("N/A", "") + " - " + artist.replace("N/A", "");
          prepareTrack(songId, query, "0", ui->olivia_list);
        }
      }
      ui->olivia_list->addItem(item);
    }
  }
  ui->tabWidget->resize(ui->tabWidget->size().width() - 1,
                        ui->tabWidget->size().height());
  ui->tabWidget->resize(ui->tabWidget->size().width() + 1,
                        ui->tabWidget->size().height());
}

void MainWindow::keyPressEvent(QKeyEvent *event) {
  if (event->key() == Qt::Key_D) {
    if (!ui->debug_widget->isVisible())
      ui->debug_widget->show();
    else
      ui->debug_widget->hide();
  }
  if (event->key() == Qt::Key_F11) {
    if (this->isFullScreen()) {
      this->showMaximized();
    } else {
      this->showFullScreen();
    }
  }
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event) {

  if (isChildOf(settingsWidget, obj)) {
    if (event->type() == QEvent::Wheel) {
      return true;
    }
  }

  if (obj == ui->playing) {
    if (event->type() == QEvent::Enter) {
      ui->playing->resume();
    }
    if (event->type() == QEvent::Leave) {
      ui->playing->pause();
    }
  }

  if (obj == ui->similarTrackLoader) {
    switch (event->type()) {
    case QEvent::MouseButtonPress:
      ui->similarTrackLoader->stop();
      break;
    default:
      break;
    }
  }

  if ((obj == ui->nowPlayingGrip || obj == ui->top_widget ||
       obj == ui->windowControls || obj == ui->label_6) &&
      (event->type() == QEvent::MouseMove)) {
    const QMouseEvent *const me = static_cast<const QMouseEvent *>(event);
    if (me->buttons() & Qt::LeftButton) {
      if (obj == ui->nowPlayingGrip) {
        miniModeWidget->move(me->globalPos() - oldPosMiniWidget);
      } else {
        move(me->globalPos() - oldPos);
      }
    }
    return true;
  }

  if (obj == ui->nowPlayingGrip || obj == ui->top_widget ||
      obj == ui->windowControls || obj == ui->label_6) {
    if (event->type() == QEvent::MouseButtonPress) {
      const QMouseEvent *const me = static_cast<const QMouseEvent *>(event);
      if (me->button() == Qt::LeftButton) {
        if (obj == ui->nowPlayingGrip) {
          oldPosMiniWidget =
              QCursor::pos() - miniModeWidget->frameGeometry().topLeft();
        } else {
          oldPos = me->globalPos() - frameGeometry().topLeft();
        }
      }
      return true;
    }
  }

  if (obj == ui->top_widget || obj == ui->label_6) {
    if (event->type() == QEvent::MouseButtonDblClick) {
      const QMouseEvent *const me = static_cast<const QMouseEvent *>(event);
      if (me->button() == Qt::LeftButton) {
        if (this->isMaximized()) {
          this->setWindowState(Qt::WindowNoState);
        } else {
          this->setWindowState(Qt::WindowMaximized);
        }
      }
      return true;
    }
  }

  if (obj == ui->search) {
    return false;
  }
  if (obj == smartModeWidget && event->type() == QEvent::Close) {
    ui->smartMode->click();
    return false;
  }

  return obj->eventFilter(obj, event);
}

void MainWindow::init_search_autoComplete() {
  _onlineSearchSuggestion_ = new onlineSearchSuggestion(ui->search);
  ui->search->installEventFilter(_onlineSearchSuggestion_);
}

void MainWindow::setPlayerPosition(qint64 position) {

  int seconds = (position / 1000) % 60;
  int minutes = (position / 60000) % 60;
  int hours = (position / 3600000) % 24;
  QTime time(hours, minutes, seconds);
  ui->position->setText(time.toString());

  ui->radioSeekSlider->setValue(static_cast<int>(position));
  QApplication::processEvents();
}

void MainWindow::on_play_pause_clicked() {
  if (radio_manager->radioState == "paused") {
    radio_manager->resumeRadio();
  } else if (radio_manager->radioState == "playing") {
    radio_manager->pauseRadio();
  }
}

void MainWindow::on_radioVolumeSlider_valueChanged(int value) {
  if (radio_manager) {
    radio_manager->changeVolume(value);
  }
}

void MainWindow::on_radioSeekSlider_sliderMoved(int position) {
  ui->radioSeekSlider->blockSignals(true);
  radio_manager->radioSeek(position);
  ui->radioSeekSlider->blockSignals(false);
  QApplication::processEvents();
}

void MainWindow::on_stop_clicked() {
  ui->playing->setText("");
  radio_manager->stop();
}

void MainWindow::webViewLoaded(bool loaded) {

  ui->webview_loader->stop();

  if (loaded) {
    ui->webview->page()->mainFrame()->addToJavaScriptWindowObject(
        QString("mainwindow"), this);
    ui->webview->page()->mainFrame()->addToJavaScriptWindowObject(
        QString("youtube"), youtube);
    ui->webview->page()->mainFrame()->addToJavaScriptWindowObject(
        QString("paginator"), pagination_manager);
    ui->webview->page()->mainFrame()->evaluateJavaScript("changeBg('" +
                                                         themeColor + "')");
    if (!nowPlayingSongIdWatcher->getValue().isEmpty()) {
      ui->webview->page()->mainFrame()->evaluateJavaScript(
          "NowPlayingTrackId='" + nowPlayingSongIdWatcher->getValue() + "'");
      ui->webview->page()->mainFrame()->evaluateJavaScript(
          "setNowPlaying('" + nowPlayingSongIdWatcher->getValue() + "')");
    }
    QWebSettings::globalSettings()->clearMemoryCaches();
    // ui->webview->history()->clear();
    ui->webview->setFocus();
  }

  if (pageType == "recently_played") {
    ui->webview->page()->mainFrame()->addToJavaScriptWindowObject(
        QString("store"), store_manager);
    ui->webview->page()->mainFrame()->evaluateJavaScript(
        "open_recently_tracks();");
  }

  if (pageType == "liked_songs") {
    ui->webview->page()->mainFrame()->addToJavaScriptWindowObject(
        QString("store"), store_manager);
    ui->webview->page()->mainFrame()->evaluateJavaScript(
        "open_liked_tracks();");
  }

  if (pageType == "local_saved_songs") {
    ui->webview->page()->mainFrame()->addToJavaScriptWindowObject(
        QString("store"), store_manager);
    ui->webview->page()->mainFrame()->evaluateJavaScript(
        " open_local_saved_tracks();");
  }
  if (pageType == "local_saved_videos") {
    ui->webview->page()->mainFrame()->addToJavaScriptWindowObject(
        QString("store"), store_manager);
    ui->webview->page()->mainFrame()->evaluateJavaScript(
        "open_local_saved_videos();");
  }

  if (pageType == "radio") {
    ui->webview->page()->mainFrame()->addToJavaScriptWindowObject(
        QString("store"), store_manager);
    ui->webview->page()->mainFrame()->evaluateJavaScript(
        "loadTopStations('most-played');");
  }

  if (pageType == "goto_album") {
    ui->webview->page()->mainFrame()->addToJavaScriptWindowObject(
        QString("mainwindow"), this);
    ui->webview->page()->mainFrame()->evaluateJavaScript("album_view('" +
                                                         gotoAlbumId + "')");
  }

  if (pageType == "goto_artist") {
    ui->webview->page()->mainFrame()->addToJavaScriptWindowObject(
        QString("mainwindow"), this);
    ui->webview->page()->mainFrame()->evaluateJavaScript("artist_view('" +
                                                         gotoArtistId + "')");
  }

  if (pageType == "goto_youtube_channel") {
    leftListChangeCurrentRow(12);
    ui->webview->page()->mainFrame()->addToJavaScriptWindowObject(
        QString("mainwindow"), this);
    ui->webview->page()->mainFrame()->evaluateJavaScript("get_channel('" +
                                                         youtubeVideoId + "')");
  }

  if (pageType == "goto_youtube_recommendation" && !youtubeVideoId.isEmpty()) {
    leftListChangeCurrentRow(12);
    ui->webview->page()->mainFrame()->addToJavaScriptWindowObject(
        QString("mainwindow"), this);
    QString trackTitle = QString(store_manager->getTrack(youtubeVideoId).at(1))
                             .remove("'")
                             .remove("\"");
    youtubeVideoId = store_manager->getYoutubeIds(youtubeVideoId)
                         .split("<br>")
                         .first()
                         .trimmed();
    ui->webview->page()->mainFrame()->evaluateJavaScript(
        "show_related('" + youtubeVideoId + "','" + trackTitle + "')");
    youtubeVideoId.clear();
  }

  if (loaded && pageType == "recommendation") {
    leftListChangeCurrentRow(2);
    // if the function is called from track action menu
    if (!recommendationSongId.isEmpty()) {
      QString trackTitle = store_manager->getTrack(recommendationSongId).at(1);
      ui->webview->page()->mainFrame()->evaluateJavaScript(
          "$('#manual_search').val('" + trackTitle + "');setPlaylistBaseId('" +
          recommendationSongId + "');");
    }
    recommendationSongId.clear();
  }

  if (loaded && pageType == "browse") {
    ui->webview->page()->mainFrame()->evaluateJavaScript("overview()");
  }

  // youtube recommendation stuff
  if (loaded && pageType == "youtube" && !youtubeSearchTerm.isEmpty()) {
    ui->webview->page()->mainFrame()->evaluateJavaScript(
        "$('.ui-content').fadeOut('fast');$('#manual_search').val('" +
        youtubeSearchTerm + "');manual_youtube_search('" + youtubeSearchTerm +
        "');");
    youtubeSearchTerm.clear();
  }
  if (loaded && pageType == "youtube" && youtubeSearchTerm.isEmpty()) {
    ui->webview->page()->mainFrame()->evaluateJavaScript("load_history();");
    youtubeSearchTerm.clear();
  }

  if (loaded && pageType == "soundcloud") {
    ui->webview->page()->mainFrame()->evaluateJavaScript(
        "client_id=\"" + soundcloud->cid + "\"");
    ui->webview->page()->mainFrame()->evaluateJavaScript(
        "load_history();loadGenre();");
  }
  if (loaded && pageType == "youtube_playlist") {
    ui->webview->page()->mainFrame()->addToJavaScriptWindowObject(
        QString("store"), store_manager);
    ui->webview->page()->mainFrame()->evaluateJavaScript("load_history();");
  }

  if (pageType == "search") {
    if (!ui->search->text().isEmpty() && loaded) {
      ui->left_list->setCurrentRow(3);
      QString term = ui->search->text();
      term = term.replace(" ", "+");
      term = term.replace("'", "%27");
      search(term);
    }
  }
}

void MainWindow::setSearchTermAndOpenYoutube(QVariant term) {
  youtubeSearchTerm = term.toString();
  ui->left_list->setCurrentRow(12); // set youtube page
}

// returns search query to webend
QString MainWindow::getTerm() {
  QString term = ui->search->text();
  return term.replace(" ", "+");
}

void MainWindow::prepareSimilarTracks() {
  QString videoId, title, artist, album, coverUrl, songId, albumId, artistId,
      millis;
  if (currentSimilarTrackProcessing < currentSimilarTrackList.count()) {
    QStringList arr =
        QString(currentSimilarTrackList.at(currentSimilarTrackProcessing))
            .split("!=-=!");
    title = arr[0];
    artist = arr[1];
    album = arr[2];
    coverUrl = arr[3];
    songId = arr[4];
    albumId = arr[5];
    artistId = arr[6];
    millis = arr[7];
    if (albumId.contains("undefined")) { // yt case
      videoId = arr[4];
    }
    ui->webview->page()->mainFrame()->evaluateJavaScript(
        "getBase64andDominantColor('" + coverUrl + "')");
    currentSimilarTrackMeta = arr;
  }
}

void MainWindow::addToSimilarTracksQueue(
    const QVariant Base64andDominantColor) {

  if (!ui->recommWidget->isVisible())
    ui->show_hide_smart_list_button->click();
  QString Base64andDominantColorStr = Base64andDominantColor.toString();
  QString base64 = Base64andDominantColorStr.split("!=-=!").last();
  QString dominantColor = Base64andDominantColorStr.split("!=-=!").first();

  QString ytIds, title, artist, album, coverUrl, songId, albumId, artistId;

  // check is meta is available prevent ASSERT failure in QList<T>
  if (currentSimilarTrackMeta.isEmpty() && currentSimilarTrackMeta.count() < 7)
    return;

  title = currentSimilarTrackMeta.at(0);
  artist = currentSimilarTrackMeta.at(1);
  album = currentSimilarTrackMeta.at(2);
  coverUrl = currentSimilarTrackMeta.at(3);
  ytIds = currentSimilarTrackMeta.at(4);
  songId = currentSimilarTrackMeta.at(4);
  albumId = currentSimilarTrackMeta.at(5);
  artistId = currentSimilarTrackMeta.at(6);

  // prevent adding tracks with no songId it causes trouble
  if (songId.isEmpty())
    return;

  // add dominantColor and base64 to currenttrackmeta stringlist
  currentSimilarTrackMeta.append(dominantColor);
  currentSimilarTrackMeta.append(base64);

  QWidget *track_widget = new QWidget(ui->smart_list);
  track_widget->setToolTip(htmlToPlainText(title));
  track_widget->setObjectName("track-widget-" + songId);
  track_ui.setupUi(track_widget);

  // update author for youtube videos who don't have author/artist from web
  if (artist.isEmpty() && albumId.contains("undefined", Qt::CaseInsensitive)) {
    request = new Request(this);
    request->setObjectName(songId);
    connect(request, &Request::requestFinished,
            [=](QString uid, QString reply) {
              if (uid == songId) {
                qDebug() << "updated track's author";
                store_manager->update_track("artistId", songId, reply);
                store_manager->saveArtist(reply, reply);
                updateTrackInQueues(songId);
              }
              this->findChild<Request *>(uid)->deleteLater();
            });
    request->get(
        QUrl("http://ktechpit.com/USS/Olivia/invidio_get_video.php?video_id=" +
             songId + "&author"),
        songId);
  }

  QFont font("Ubuntu");
  font.setPixelSize(12);
  setFont(font);

  // to convert html sequence to plaintext
  QString plainTitle = htmlToPlainText(title);

  ElidedLabel *titleLabel = new ElidedLabel(plainTitle, track_widget);
  titleLabel->setFont(font);
  titleLabel->setObjectName("title_elided");
  track_ui.verticalLayout_2->addWidget(titleLabel);

  ElidedLabel *artistLabel =
      new ElidedLabel(htmlToPlainText(artist), track_widget);
  artistLabel->setObjectName("artist_elided");
  artistLabel->setFont(font);
  track_ui.verticalLayout_2->addWidget(artistLabel);

  ElidedLabel *albumLabel =
      new ElidedLabel(htmlToPlainText(album), track_widget);
  albumLabel->setObjectName("album_elided");
  albumLabel->setFont(font);
  track_ui.verticalLayout_2->addWidget(albumLabel);

  track_ui.id->setText(ytIds);

  track_ui.dominant_color->setText(dominantColor);
  track_ui.songId->setText(songId);
  track_ui.playing->setPixmap(QPixmap(":/icons/blank.png")
                                  .scaled(track_ui.playing->size(),
                                          Qt::KeepAspectRatio,
                                          Qt::SmoothTransformation));

  track_ui.songId->hide();
  track_ui.dominant_color->hide();
  track_ui.id->hide();
  track_ui.url->hide();
  track_ui.option->setObjectName(songId + "optionButton");

  // prepare meta str and add it to track meta lineEdit
  QString meta;
  foreach (QString str, currentSimilarTrackMeta) {
    meta.append(str + "!=-=!");
  }
  meta.chop(5); // remove last !=-=!
  track_ui.meta->setText(meta);
  track_ui.meta->hide();
  connect(track_ui.option, SIGNAL(clicked(bool)), this,
          SLOT(showTrackOption()));
  // LoadCover( QUrl(coverUrl),*track_ui.cover);
  base64 = base64.split("base64,").last();
  QByteArray ba = base64.toUtf8();
  QPixmap image;
  image.loadFromData(QByteArray::fromBase64(ba));
  if (!image.isNull()) {
    track_ui.cover->setPixmap(image);
  }

  if (albumId.contains("undefined-")) {
    track_ui.cover->setMaximumHeight(track_widget->height());
    track_ui.cover->setMaximumWidth(
        static_cast<int>(track_widget->height() * 1.15));
    track_ui.id->setText(songId + "<br>");
    store_manager->saveytIds(songId, songId + "<br>");
    QListWidgetItem *item;
    item = new QListWidgetItem(ui->smart_list);

    // set size for track widget
    track_ui.cover->setMaximumSize(149, 90);
    track_ui.cover->setMinimumSize(149, 79);
    track_ui.widget->adjustSize();
    item->setSizeHint(track_widget->minimumSizeHint());

    ui->smart_list->setItemWidget(item, track_widget);
    ui->smart_list->itemWidget(item)->setEnabled(
        false); // enable when finds a url
    ui->smart_list->addItem(item);

    // ui->smart_list->setCurrentRow(ui->smart_list->count()-1);
    if (store_manager->isDownloaded(songId)) {
      ui->smart_list->itemWidget(item)->setEnabled(true);
      ui->similarTrackLoader->stop();
      if (!ui->recommWidget->isVisible())
        ui->show_hide_smart_list_button->click();
      track_ui.url->setText("file://" + setting_path + "/downloadedTracks/" +
                            songId);
      track_ui.offline->setPixmap(QPixmap(":/icons/offline.png")
                                      .scaled(track_ui.offline->size(),
                                              Qt::KeepAspectRatio,
                                              Qt::SmoothTransformation));
    } else {
      if (store_manager->getYoutubeIds(songId).isEmpty()) {
        //                QString millis = ytIds; // we were given ytids as
        //                millis from webend
        QString query =
            title.replace("N/A", "") + " - " + artist.replace("N/A", "");
        prepareTrack(songId, query, "0", ui->smart_list);
      } else {
        // checks if url is expired and updates item with new url which can be
        // streamed .... until keeps the track item disabled.
        if (store_manager->getExpiry(songId)) {
          ui->smart_list->itemWidget(item)->setEnabled(false);
          getAudioStream(store_manager->getYoutubeIds(songId), songId);
        } else {
          ui->smart_list->itemWidget(item)->setEnabled(true);
          ui->similarTrackLoader->stop();
          if (!ui->recommWidget->isVisible())
            ui->show_hide_smart_list_button->click();
          track_ui.url->setText(store_manager->getOfflineUrl(songId));
        }
      }
    }
    similarTracksProcessHelper();
  } else {
    QListWidgetItem *item;
    item = new QListWidgetItem(ui->smart_list);
    item->setSizeHint(track_widget->minimumSizeHint());
    ui->smart_list->setItemWidget(item, track_widget);
    ui->smart_list->itemWidget(item)->setEnabled(
        false); // enable when finds a url
    ui->smart_list->addItem(item);

    // ui->smart_list->setCurrentRow(ui->smart_list->count()-1);
    if (store_manager->isDownloaded(songId)) {
      track_ui.id->setText(store_manager->getYoutubeIds(songId));
      ui->smart_list->itemWidget(item)->setEnabled(true);
      ui->similarTrackLoader->stop();
      if (!ui->recommWidget->isVisible())
        ui->show_hide_smart_list_button->click();

      track_ui.url->setText("file://" + setting_path + "/downloadedTracks/" +
                            songId);
      track_ui.offline->setPixmap(QPixmap(":/icons/offline.png")
                                      .scaled(track_ui.offline->size(),
                                              Qt::KeepAspectRatio,
                                              Qt::SmoothTransformation));
    } else {
      if (store_manager->getYoutubeIds(songId).isEmpty()) {
        //              QString millis = ytIds; // we were given ytids as millis
        //              from webend
        QString query =
            title.replace("N/A", "") + " - " + artist.replace("N/A", "");
        prepareTrack(songId, query, "0", ui->smart_list);
      } else {
        // checks if url is expired and updates item with new url which can be
        // streamed .... until keeps the track item disabled.
        if (store_manager->getExpiry(songId)) {
          ui->smart_list->itemWidget(item)->setEnabled(false);
          QNetworkAccessManager *m_netwManager =
              new QNetworkAccessManager(this);
          connect(m_netwManager, &QNetworkAccessManager::finished,
                  [=](QNetworkReply *rep) {
                    if (rep->error() == QNetworkReply::NoError) {
                      QString ytIds = rep->readAll();
                      track_ui.id->setText(ytIds.trimmed());
                      // SAVE DATA TO LOCAL DATABASE
                      store_manager->saveytIds(songId, ytIds);
                      getAudioStream(ytIds, songId);
                      similarTracksProcessHelper();
                      rep->deleteLater();
                      m_netwManager->deleteLater();
                    } else {
                      qDebug() << "error processing track"
                               << rep->request().url().toString();
                    }
                  });
          QString query =
              title.replace("N/A", "") + " - " + artist.replace("N/A", "");
          QUrl url(
              "http://ktechpit.com/USS/Olivia/youtube.php?millis=0&query=" +
              query);
          QNetworkRequest request(url);
          m_netwManager->get(request);
        } else {
          ui->smart_list->itemWidget(item)->setEnabled(true);
          ui->similarTrackLoader->stop();
          if (!ui->recommWidget->isVisible())
            ui->show_hide_smart_list_button->click();
          track_ui.url->setText(store_manager->getOfflineUrl(songId));
        }
      }
    }
    similarTracksProcessHelper();
  }

  //    if(store_manager->getTrack(songId).isEmpty()){
  // SAVE DATA TO LOCAL DATABASE
  base64.remove("data:image/jpeg;base64,");
  base64.remove("data:image/jpg;base64,");
  base64.remove("data:image/png;base64,");
  store_manager->saveAlbumArt(albumId, base64);
  store_manager->saveArtist(artistId, artist);
  store_manager->saveAlbum(albumId, album);
  store_manager->saveDominantColor(albumId, dominantColor);
  store_manager->setTrack(QStringList()
                          << songId << albumId << artistId << title);
  //    }

  for (int i = 0; i < currentSimilarTrackList.count(); i++) {
    if (currentSimilarTrackList.at(i).contains(songId)) {
      currentSimilarTrackList.removeAt(i);
    }
  }
}

void MainWindow::similarTracksProcessHelper() {
  // clear meta of currentSimilarTrack and free it for next track
  currentSimilarTrackMeta.clear();
  // process next track in currentSimilarTrackList list;
  if (similarTracks->isLoadingPLaylist) {
    if (currentSimilarTrackProcessing <
        settingsObj.value("similarTracksToLoad", 8)
            .toInt() /*currentSimilarTrackList.count()-1*/) {
      if (currentSimilarTrackProcessing + 1 ==
          settingsObj.value("similarTracksToLoad", 8).toInt())
        return;
      currentSimilarTrackProcessing++;
      prepareSimilarTracks();
    }
  } else {
    if (currentSimilarTrackProcessing <
        similarTracks
            ->numberOfSimilarTracksToLoad /*currentSimilarTrackList.count()-1*/) {
      if (currentSimilarTrackProcessing < currentSimilarTrackList.count()) {
        if (currentSimilarTrackProcessing + 1 ==
            settingsObj.value("similarTracksToLoad", 8).toInt())
          return;
        currentSimilarTrackProcessing++;
        prepareSimilarTracks();
      }
    }
  }
  // added to assing next when loading local songs
  nextPreviousHelper(ui->smart_list);
}

// INVOKABLE method to add track to one of the player queue
void MainWindow::addToQueue(QString ytIds, QString title, QString artist,
                            QString album, QString base64,
                            QString dominantColor, QString songId,
                            QString albumId, QString artistId) {

  QString setting_path =
      QStandardPaths::writableLocation(QStandardPaths::DataLocation);
  // prevent adding tracks with no songId it causes trouble
  if (songId.isEmpty())
    return;

  // check if track is in player queue already if found prevent further
  // execution
  if (store_manager->isInQueue(songId)) {
    findTrackInQueue(songId);
    return;
  } else { // add track to one of the list
    QWidget *track_widget = new QWidget(ui->olivia_list);
    track_widget->setToolTip(htmlToPlainText(title));
    track_widget->setObjectName("track-widget-" + songId);
    track_ui.setupUi(track_widget);

    // update author for youtube videos who don't have author/artist from web
    if (artist.isEmpty() &&
        albumId.contains("undefined", Qt::CaseInsensitive)) {
      request = new Request(this);
      request->setObjectName(songId);
      connect(request, &Request::requestFinished,
              [=](QString uid, QString reply) {
                if (uid == songId) {
                  qDebug() << "updated track's author";
                  store_manager->update_track("artistId", songId, reply);
                  store_manager->saveArtist(reply, reply);
                  updateTrackInQueues(songId);
                }
                this->findChild<Request *>(uid)->deleteLater();
              });
      request->get(
          QUrl(
              "http://ktechpit.com/USS/Olivia/invidio_get_video.php?video_id=" +
              songId + "&author"),
          songId);
    }

    // set track meta icon
    if (albumId.contains("soundcloud")) {
      track_widget->setStyleSheet(
          "QWidget#track-widget-" + songId +
          "{background: transparent url(':/icons/micro/soundcloud_micro.png') "
          "no-repeat bottom right}");
    }

    QFont font("Ubuntu");
    font.setPixelSize(12);
    setFont(font);

    // to convert html sequence to plaintext
    QString plainTitle = htmlToPlainText(title);

    ElidedLabel *titleLabel = new ElidedLabel(plainTitle, track_widget);
    titleLabel->setFont(font);
    titleLabel->setObjectName("title_elided");
    track_ui.verticalLayout_2->addWidget(titleLabel);

    ElidedLabel *artistLabel =
        new ElidedLabel(htmlToPlainText(artist), track_widget);
    artistLabel->setObjectName("artist_elided");
    artistLabel->setFont(font);
    track_ui.verticalLayout_2->addWidget(artistLabel);

    ElidedLabel *albumLabel =
        new ElidedLabel(htmlToPlainText(album), track_widget);
    albumLabel->setObjectName("album_elided");
    albumLabel->setFont(font);
    track_ui.verticalLayout_2->addWidget(albumLabel);

    track_ui.dominant_color->setText(dominantColor);
    track_ui.songId->setText(songId);
    track_ui.playing->setPixmap(QPixmap(":/icons/blank.png")
                                    .scaled(track_ui.playing->size(),
                                            Qt::KeepAspectRatio,
                                            Qt::SmoothTransformation));

    // hide meta widgets
    track_ui.meta->hide();
    track_ui.songId->hide();
    track_ui.id->hide();
    track_ui.url->hide();
    track_ui.dominant_color->hide();

    // set option button
    track_ui.option->setObjectName(songId + "optionButton");
    connect(track_ui.option, SIGNAL(clicked(bool)), this,
            SLOT(showTrackOption()));

    base64 = base64.split("base64,").last();
    QByteArray ba = base64.toUtf8();
    QPixmap image;
    image.loadFromData(QByteArray::fromBase64(ba));
    if (!image.isNull()) {
      track_ui.cover->setPixmap(image);
    }

    // check track type and decide where it will be added
    if (albumId.contains("undefined-")) { // youtube track
      qDebug() << ytIds;
      track_ui.id->setText(songId + "<br>");
      store_manager->saveytIds(songId, songId + "<br>");
      QListWidgetItem *item;
      item = new QListWidgetItem(ui->youtube_list);
      // set size for track widget
      track_ui.cover->setMaximumSize(149, 90);
      track_ui.cover->setMinimumSize(149, 79);
      track_ui.widget->adjustSize();
      item->setSizeHint(track_widget->minimumSizeHint());
      ui->youtube_list->setItemWidget(item, track_widget);
      ui->youtube_list->itemWidget(item)->setEnabled(
          false); // enable when finds a url
      ui->youtube_list->addItem(item);
      ui->youtube_list->setCurrentRow(ui->youtube_list->count() - 1);

      if (store_manager->isDownloaded(songId)) {
        ui->youtube_list->itemWidget(item)->setEnabled(true);
        track_ui.url->setText("file://" + setting_path + "/downloadedTracks/" +
                              songId);
        track_ui.offline->setPixmap(QPixmap(":/icons/offline.png")
                                        .scaled(track_ui.offline->size(),
                                                Qt::KeepAspectRatio,
                                                Qt::SmoothTransformation));
      } else {
        if (store_manager->getYoutubeIds(songId).isEmpty()) {
          qDebug() << "youtubeIds are empty";
          QString millis = ytIds; // we were given ytids as millis from webend
          QString query =
              title.replace("N/A", "") + " - " + artist.replace("N/A", "");
          prepareTrack(songId, query, millis, ui->youtube_list);
        } else {
          // checks if url is expired and updates item with new url which can be
          // streamed .... until keeps the track item disabled.
          if (store_manager->getExpiry(songId)) {
            ui->youtube_list->itemWidget(item)->setEnabled(false);
            getAudioStream(store_manager->getYoutubeIds(songId), songId);
            reverseYtdlProcessList();
          } else {
            ui->youtube_list->itemWidget(item)->setEnabled(true);
            track_ui.url->setText(store_manager->getOfflineUrl(songId));
          }
        }
      }
      ui->tabWidget->setCurrentWidget(ui->tab_2);
      ui->youtube_list->scrollToItem(item);
      store_manager->saveytIds(songId, ytIds);
    } else { // not a youtube track prepare it(find ytids and url) and add it to
             // olivia list
      QListWidgetItem *item;
      item = new QListWidgetItem(ui->olivia_list);
      item->setSizeHint(track_widget->minimumSizeHint());
      ui->olivia_list->setItemWidget(item, track_widget);
      ui->olivia_list->itemWidget(item)->setEnabled(
          false); // enable when finds a url
      ui->olivia_list->addItem(item);
      ui->olivia_list->setCurrentRow(ui->olivia_list->count() - 1);

      if (store_manager->isDownloaded(songId)) {
        ui->olivia_list->itemWidget(item)->setEnabled(true);
        track_ui.url->setText("file://" + setting_path + "/downloadedTracks/" +
                              songId);
        track_ui.offline->setPixmap(QPixmap(":/icons/offline.png")
                                        .scaled(track_ui.offline->size(),
                                                Qt::KeepAspectRatio,
                                                Qt::SmoothTransformation));
      } else {
        // check if track is from soundcloud
        if (albumId.contains("soundcloud")) {
          track_ui.id->setText("https://w.soundcloud.com/player/?url=https://"
                               "api.soundcloud.com/tracks/" +
                               songId);
          store_manager->saveytIds(songId, track_ui.id->text());
          // checks if url is expired and updates item with new url which can be
          // streamed .... until keeps the track item disabled.
          if (store_manager->getExpiry(songId)) {
            ui->olivia_list->itemWidget(item)->setEnabled(false);
            getAudioStream("https://w.soundcloud.com/player/?url=https://"
                           "api.soundcloud.com/tracks/" +
                               songId,
                           songId);
            reverseYtdlProcessList();
          } else {
            ui->olivia_list->itemWidget(item)->setEnabled(true);
            track_ui.url->setText(store_manager->getOfflineUrl(songId));
          }
        } else {
          // normal track processing
          if (store_manager->getYoutubeIds(songId).isEmpty()) {
            QString millis = ytIds; // we were given ytids as millis from webend
            QString query =
                title.replace("N/A", "") + " - " + artist.replace("N/A", "");
            prepareTrack(songId, query, millis, ui->olivia_list);
          } else {
            // checks if url is expired and updates item with new url which can
            // be streamed .... until keeps the track item disabled.
            if (store_manager->getExpiry(songId)) {
              ui->olivia_list->itemWidget(item)->setEnabled(false);
              getAudioStream(store_manager->getYoutubeIds(songId), songId);
              reverseYtdlProcessList();
            } else {
              ui->olivia_list->itemWidget(item)->setEnabled(true);
              track_ui.url->setText(store_manager->getOfflineUrl(songId));
            }
          }
        }
      }
      ui->tabWidget->setCurrentWidget(ui->tab);
      ui->olivia_list->scrollToItem(item);
    }

    // SAVE DATA TO LOCAL DATABASE
    base64.remove("data:image/jpeg;base64,");
    base64.remove("data:image/jpg;base64,");
    base64.remove("data:image/png;base64,");
    store_manager->saveAlbumArt(albumId, base64);
    store_manager->saveArtist(artistId, artist);
    store_manager->saveAlbum(albumId, album);
    store_manager->saveDominantColor(albumId, dominantColor);
    store_manager->setTrack(QStringList()
                            << songId << albumId << artistId << title);
    store_manager->add_to_player_queue(songId);
  }
}

void MainWindow::updateTrackInQueues(QString songId) {
  QListWidget *queue =
      this->findChild<QListWidget *>(getCurrentPlayerQueue(songId));
  if (queue != nullptr) {
    QWidget *itemWidget = queue->findChild<QWidget *>("track-widget-" + songId);
    if (itemWidget != nullptr)
      itemWidget->findChild<ElidedLabel *>("artist_elided")
          ->setText(
              store_manager->getArtist(store_manager->getArtistId(songId)));
  }
}

// jumps to track in one of player queue where the given songId is present
void MainWindow::findTrackInQueue(QString songId) {
  ui->console->append("Song - " + songId + " Already in queue");
  QListWidget *listWidget =
      ui->right_panel->findChild<QListWidget *>(getCurrentPlayerQueue(songId));

  // if listWidget not found in right_panel. this happens when player is
  // switched to smart mode;
  if (listWidget == nullptr) {
    listWidget = ui->recommHolder->findChild<QListWidget *>(
        getCurrentPlayerQueue(songId));
  }
  if (listWidget != nullptr) {
    // switch to listView
    QString listName = listWidget->objectName();
    if (listName == "olivia_list")
      ui->tabWidget->setCurrentWidget(ui->tab);
    if (listName == "youtube_list")
      ui->tabWidget->setCurrentWidget(ui->tab_2);
    if (listName == "smart_list" && !ui->recommWidget->isVisible())
      ui->show_hide_smart_list_button->click();

    QWidget *listWidgetItem =
        listWidget->findChild<QWidget *>("track-widget-" + songId);
    QPoint pos = listWidgetItem->pos();
    listWidget->setCurrentItem(listWidget->itemAt(pos));
    listWidget->scrollToItem(listWidget->itemAt(pos));
  }
}

// get track's ytIds and playable source url
void MainWindow::prepareTrack(QString songId, QString query, QString millis,
                              QListWidget *list) {
  qDebug() << "prepearing track:songid" << songId << query << millis;
  QNetworkAccessManager *m_netwManager = new QNetworkAccessManager(this);
  connect(m_netwManager, &QNetworkAccessManager::finished,
          [=](QNetworkReply *rep) {
            if (rep->error() == QNetworkReply::NoError) {
              QString ytIds = rep->readAll().trimmed();
              qDebug() << "PREPARED TRACK:" << songId << ytIds;
              QWidget *listWidgetItem =
                  list->findChild<QWidget *>("track-widget-" + songId);
              if (listWidgetItem != nullptr) { // check if track is not removed
                                               // before this request finish
                listWidgetItem->findChild<QLineEdit *>("id")->setText(ytIds);
              }
              // save ytids to store
              if (!ytIds.isEmpty()) {
                // do both save and update its a multipurpose function
                store_manager->saveytIds(songId, ytIds);
                qDebug() << "ytdids recieved:" << songId << ytIds;
                if (listWidgetItem != nullptr && !listWidgetItem->isEnabled()) {
                  getAudioStream(ytIds, songId);
                  if (similarTracks->isLoadingPLaylist == false) {
                    reverseYtdlProcessList();
                  }
                  if (ytdlQueue.count() > 3) {
                    reverseYtdlProcessList();
                  }
                }
              }
              rep->deleteLater();
              m_netwManager->deleteLater();
            } else {
              qDebug() << "error processing track"
                       << rep->request().url().toString();
            }
          });
  QUrl url("http://ktechpit.com/USS/Olivia/youtube.php?millis=" + millis +
           "&query=" + query);
  qDebug() << url;
  QNetworkRequest request(url);
  m_netwManager->get(request);
}

// reverse the process queue, so that recently added song can get first chance
// to process
void MainWindow::reverseYtdlProcessList() {
  // temp changes
  bool isSmartList = false;
  if (ytdlQueue.count() > 0) {
    if (getCurrentPlayerQueue(ytdlQueue.first().at(1)) ==
        "smart_list") { // prevent reversing queue if first track to be
                        // proccessed is from smart_list
      isSmartList = true;
      qDebug() << "not reversing queue";
    }
  }
  if (ytdlQueue.count() > 1 &&
      (!smartMode && !isSmartList)) { // prevent reversing if ytDlqueue onlly
                                      // have one track and if not smartMode
    ytdlQueue.insert(1, ytdlQueue.takeAt(ytdlQueue.count() - 1));
  }
}

void MainWindow::showTrackOption() {

  QPushButton *senderButton = qobject_cast<QPushButton *>(sender());

  QString songId = senderButton->objectName().remove("optionButton").trimmed();
  QString listname = getCurrentPlayerQueue(songId);
  if (!listname.isEmpty()) {
    QListWidget *list = this->findChild<QListWidget *>(listname);
    QWidget *listWidgetItem =
        list->findChild<QWidget *>("track-widget-" + songId);
    QPoint pos = listWidgetItem->pos();
    int index = list->row(list->itemAt(pos));
    list->setCurrentRow(index);
  }
  qDebug() << "trackOption for:" << songId;
  QString albumId = store_manager->getAlbumId(songId);
  QString artistId = store_manager->getArtistId(songId);

  qDebug() << "albumId:" << albumId;

  QAction *showRecommendation = new QAction("Spotify Recommendations", nullptr);
  QAction *watchVideo = new QAction("Video Options", nullptr);
  QAction *showLyrics = new QAction("Show Lyrics", nullptr);
  QAction *openChannel = new QAction("Open Channel", nullptr);
  QAction *youtubeShowRecommendation =
      new QAction("Youtube Recommendations", nullptr);
  QAction *gotoArtist = new QAction("Go to Artist", nullptr);
  QAction *gotoAlbum = new QAction("Go to Album", nullptr);
  QAction *removeSong = new QAction("Remove from queue", nullptr);
  QAction *deleteSongCache = new QAction("Delete song cache", nullptr);
  QAction *properties = new QAction("Track Properties", nullptr);
  QAction *downloadTrack = new QAction("Download audio", nullptr);
  QAction *getRemixes = new QAction("Go to Song Remixes", nullptr);

  QAction *addToLikedSongs = new QAction("Add to liked songs", nullptr);
  QAction *removeFromLikedSongs =
      new QAction("Remove from liked songs", nullptr);
  QAction *playNext = new QAction("Play next", nullptr);

  bool isDownloaded = store_manager->isDownloaded(songId);

  deleteSongCache->setEnabled(isDownloaded);
  properties->setEnabled(isDownloaded);
  downloadTrack->setEnabled(!isDownloaded);

  // setIcons
  youtubeShowRecommendation->setIcon(QIcon(":/icons/sidebar/youtube.png"));
  showRecommendation->setIcon(QIcon(":/icons/sidebar/spotify.png"));
  watchVideo->setIcon(QIcon(":/icons/sidebar/video.png"));
  gotoArtist->setIcon(QIcon(":/icons/sidebar/artist.png"));
  gotoAlbum->setIcon(QIcon(":/icons/sidebar/album.png"));
  showLyrics->setIcon(QIcon(":/icons/sidebar/playlist.png"));
  openChannel->setIcon(QIcon(":/icons/sidebar/artist.png"));
  removeSong->setIcon(QIcon(":/icons/sidebar/remove.png"));
  deleteSongCache->setIcon(QIcon(":/icons/sidebar/delete.png"));
  addToLikedSongs->setIcon(QIcon(":/icons/sidebar/liked.png"));
  removeFromLikedSongs->setIcon(QIcon(":/icons/sidebar/not_liked.png"));
  playNext->setIcon(QIcon(":/icons/sidebar/next.png"));
  properties->setIcon(QIcon(":/icons/sidebar/info.png"));
  downloadTrack->setIcon(QIcon(":/icons/others/donwload2.png"));
  getRemixes->setIcon(QIcon(":/icons/sidebar/remix.png"));

  connect(getRemixes, &QAction::triggered,
          [=]() { similarTracks->addRemixes(songId); });

  connect(downloadTrack, &QAction::triggered, [=]() {
    downloadWidget->trackId = songId;
    QStringList trackmeta = store_manager->getTrack(songId);
    QString title = trackmeta.at(1);

    // remove html notations from title
    QString plainTitle = htmlToPlainText(title);
    downloadWidget->trackTitle = plainTitle + " [Audio]";

    //      QString url_str =
    //      "https://www.youtube.com/watch?v="+ytIds.split("<br>").first();
    QString url_str = trackmeta.at(7);
    downloadWidget->startWget(url_str, downloadWidget->downloadLocationAudio,
                              QStringList() << "bestaudio", "audio");
    ui->tabWidget->setCurrentIndex(2); // switch to task tab
  });

  connect(properties, &QAction::triggered,
          [=]() { showTrackProperties(songId); });

  connect(addToLikedSongs, &QAction::triggered, [=]() {
    store_manager->add_to_liked(songId);
    setFavouriteButton(true);
  });

  connect(removeFromLikedSongs, &QAction::triggered, [=]() {
    removeFromFavourite(songId);
    setFavouriteButton(false);
  });

  connect(showLyrics, &QAction::triggered, [=]() {
    QString songTitle, artistTitle, lyrics_search_string;
    songTitle = store_manager->getTrack(songId).at(1);
    if (store_manager->getAlbum(albumId).contains("undefined")) {
      lyrics_search_string = songTitle;
    } else {
      artistTitle = store_manager->getArtist(artistId);
      lyrics_search_string = songTitle + " - " + artistTitle;
    }
    lyricsWidget->setStyleSheet("");
    lyricsWidget->setCustomStyle(ui->search->styleSheet(),
                                 ui->olivia_list->styleSheet(),
                                 this->styleSheet());
    lyricsWidget->setQueryString(htmlToPlainText(lyrics_search_string));
  });

  connect(showRecommendation, &QAction::triggered, [=]() {
    ui->webview->load(QUrl("qrc:///web/recommendation/recommendation.html"));
    pageType = "recommendation";
    recommendationSongId = songId;
  });

  connect(youtubeShowRecommendation, &QAction::triggered, [=]() {
    ui->webview->load(QUrl("qrc:///web/youtube/youtube.html"));
    pageType = "goto_youtube_recommendation";
    youtubeVideoId = songId;
  });

  connect(watchVideo, &QAction::triggered, [=]() {
    if (videoOption == nullptr) {
      init_videoOption();
    } else {
      QString btn_style =
          "QPushButton{color: silver; background-color: #45443F; border:1px "
          "solid #272727; padding-top: 3px; padding-bottom: 3px; padding-left: "
          "3px; padding-right: 3px; border-radius: 2px; outline: none;}"
          "QPushButton:disabled { background-color: #45443F; border:1px solid "
          "#272727; padding-top: 3px; padding-bottom: 3px; padding-left: 5px; "
          "padding-right: 5px; /*border-radius: 2px;*/ color: #636363;}"
          "QPushButton:hover{border: 1px solid "
          "#272727;background-color:#5A584F; color:silver ;}"
          "QPushButton:pressed {background-color: #45443F;color: "
          "silver;padding-bottom:1px;}";

      videoOption->setStyleSheet("");
      videoOption->setStyleSheet("QWidget#VideoOption{" +
                                 ui->search->styleSheet() + "}" + "QFrame{" +
                                 ui->search->styleSheet() + "}" + btn_style);
      videoOption->setMeta(songId);
      videoOption->removeStyle();
      videoOption->adjustSize();
      videoOption->show();
    }
  });

  connect(openChannel, &QAction::triggered,
          [=]() { open_youtube_channel_for_video(QVariant(songId)); });

  connect(gotoAlbum, &QAction::triggered, [=]() {
    ui->webview->load(QUrl("qrc:///web/goto/album.html"));
    pageType = "goto_album";
    gotoAlbumId = albumId;
  });

  connect(gotoArtist, &QAction::triggered, [=]() {
    ui->webview->load(QUrl("qrc:///web/goto/artist.html"));
    pageType = "goto_artist";
    gotoArtistId = artistId;
  });

  connect(deleteSongCache, &QAction::triggered,
          [=]() { delete_song_cache(songId); });

  // find the list widget which requested menu
  QObject *listObj = senderButton->parent();
  while (!listObj->objectName().contains("list")) {
    listObj = listObj->parent();
  }
  QListWidget *list = qobject_cast<QListWidget *>(listObj);

  connect(playNext, &QAction::triggered, [=]() {
    QWidget *listWidgetItem =
        list->findChild<QWidget *>("track-widget-" + songId);
    QPoint pos = listWidgetItem->pos();
    int index = list->row(list->itemAt(pos));
    assignNextTrack(list, index);
  });

  connect(removeSong, &QAction::triggered, [=]() {
    if (list->objectName() == "smart_list") {
      remove_song_from_smart_playlist(songId);
    } else {
      remove_song(songId);
    }
    nextPreviousHelper(list);
  });

  QMenu menu;

  // start radio
  QAction *startRadio = new QAction("Go to Song Radio", nullptr);
  startRadio->setIcon(QIcon(":/icons/sidebar/radio.png"));
  // radio connections
  QString ytIds = store_manager->getYoutubeIds(songId).split("<br>").first();
  connect(startRadio, &QAction::triggered, [=]() {
    notify("", "Loading songs for radio...");
    similarTracks->isLoadingPLaylist = false;
    getRecommendedTracksForAutoPlayHelper(ytIds, songId);
  });

  if (store_manager->is_liked_track(songId)) {
    menu.addAction(removeFromLikedSongs);
  } else {
    menu.addAction(addToLikedSongs);
  }

  // add playnnext to menu if something is playing
  if ((!nowPlayingSongIdWatcher->getValue().isEmpty() &&
       nowPlayingSongIdWatcher->getValue() != "0000000")) {
    menu.addAction(playNext);
  }

  if (!albumId.contains("undefined")) { // do not add gotoalbum and gotoartist
                                        // actions to youtube streams
    bool isSoundcloudTrack = albumId.contains("soundcloud");
    menu.addAction(showLyrics);
    menu.addAction(downloadTrack);
    menu.addAction(getRemixes);
    if (!isSoundcloudTrack) {
      menu.addAction(watchVideo);
    }
    menu.addSeparator();
    menu.addAction(startRadio);
    menu.addAction(getRemixes);

    if (!isNumericStr(songId) &&
        (!albumId.contains("undefined") &&
         !albumId.contains("soundcloud"))) // spotify song ids are not numeric
    {
      menu.addAction(showRecommendation);
    }
    // added youtube recommendation fallback for itunes tracks ids
    if (!isSoundcloudTrack) {
      menu.addAction(youtubeShowRecommendation);
    }
    menu.addSeparator();
    if (!isSoundcloudTrack) { // no album for soundcloud track
      menu.addAction(gotoAlbum);
    }
  } else {
    menu.addSeparator();
    menu.addAction(showLyrics);
    menu.addAction(openChannel);
    menu.addAction(downloadTrack);
    menu.addAction(watchVideo);
    menu.addSeparator();
    menu.addAction(youtubeShowRecommendation);
    menu.addAction(startRadio);
    menu.addAction(getRemixes);
  }

  menu.addSeparator();
  menu.addAction(removeSong);
  menu.addAction(deleteSongCache);
  menu.addSeparator();
  menu.addAction(properties);
  menu.setStyleSheet(menuStyle());
  menu.exec(QCursor::pos());
}

void MainWindow::open_youtube_channel_for_video(QVariant videoId) {
  ui->webview->load(QUrl("qrc:///web/youtube/youtube.html"));
  pageType = "goto_youtube_channel";
  youtubeVideoId = videoId.toString();
}

// similar to remove_song but used only for smart playlist
void MainWindow::remove_song_from_smart_playlist(QVariant track_id) {
  QString songId = track_id.toString().remove("<br>").trimmed();
  QListWidget *queue = ui->smart_list;
  for (int i = 0; i < queue->count(); i++) {
    QString songIdFromWidget =
        static_cast<QLineEdit *>(
            queue->itemWidget(queue->item(i))->findChild<QLineEdit *>("songId"))
            ->text()
            .trimmed();
    if (songIdFromWidget.contains(songId)) {
      queue->takeItem(i);
    }
  }
}

void MainWindow::remove_song(QVariant track_id) {
  QString songId = track_id.toString().remove("<br>").trimmed();
  QListWidget *queue =
      this->findChild<QListWidget *>(getCurrentPlayerQueue(songId));
  if (queue != nullptr) {
    QWidget *listWidgetItem =
        queue->findChild<QWidget *>("track-widget-" + songId);
    if (listWidgetItem != nullptr) {
      for (int i = 0; i < queue->count(); i++) {
        QString songIdFromWidget =
            static_cast<QLineEdit *>(queue->itemWidget(queue->item(i))
                                         ->findChild<QLineEdit *>("songId"))
                ->text()
                .trimmed();
        if (songIdFromWidget.contains(songId)) {
          queue->takeItem(i);
        }
      }
      store_manager->removeFromQueue(songId);
    }
  }
}

void MainWindow::removeSongFromProcessQueue(QString songId) {
  // remove song from ytdlqueue cuase its no longer needed right now
  for (int i = 0; i < ytdlQueue.count(); i++) {
    if (ytdlQueue.at(i).contains(songId, Qt::CaseInsensitive)) {
      qDebug() << "removed:" << ytdlQueue.at(i);
      ytdlQueue.removeAt(i);
    } else {
      qDebug() << "track not found in ytdlProcessQueue";
    }
  }
}

void MainWindow::delete_song_cache(QVariant track_id) {
  QString songId = track_id.toString().remove("<br>").trimmed();
  QString setting_path =
      QStandardPaths::writableLocation(QStandardPaths::DataLocation);
  QFile cache(setting_path + "/downloadedTracks/" + songId);
  cache.remove();
  store_manager->update_track("downloaded", songId, "0");
  for (int i = 0; i < ui->olivia_list->count(); i++) {
    QString songIdFromWidget =
        static_cast<QLineEdit *>(
            ui->olivia_list->itemWidget(ui->olivia_list->item(i))
                ->findChild<QLineEdit *>("songId"))
            ->text()
            .trimmed();
    if (songId == songIdFromWidget) {
      ui->olivia_list->itemWidget(ui->olivia_list->item(i))
          ->findChild<QLabel *>("offline")
          ->setPixmap(QPixmap(":/icons/blank.png"));
      ui->olivia_list->itemWidget(ui->olivia_list->item(i))
          ->findChild<QLineEdit *>("url")
          ->setText(store_manager->getOfflineUrl(songId));
      if (store_manager->getExpiry(songId)) {
        ui->olivia_list->itemWidget(ui->olivia_list->item(i))
            ->setEnabled(false);
        getAudioStream(store_manager->getYoutubeIds(songId), songId);
      }
      break;
    }
  }
  for (int i = 0; i < ui->youtube_list->count(); i++) {
    QString songIdFromWidget =
        static_cast<QLineEdit *>(
            ui->youtube_list->itemWidget(ui->youtube_list->item(i))
                ->findChild<QLineEdit *>("songId"))
            ->text()
            .trimmed();
    if (songId == songIdFromWidget) {
      ui->youtube_list->itemWidget(ui->youtube_list->item(i))
          ->findChild<QLabel *>("offline")
          ->setPixmap(QPixmap(":/icons/blank.png"));
      ui->youtube_list->itemWidget(ui->youtube_list->item(i))
          ->findChild<QLineEdit *>("url")
          ->setText(store_manager->getOfflineUrl(songId));
      if (store_manager->getExpiry(songId)) {
        ui->youtube_list->itemWidget(ui->youtube_list->item(i))
            ->setEnabled(false);
        getAudioStream(store_manager->getYoutubeIds(songId), songId);
      }
      break;
    }
  }

  for (int i = 0; i < ui->smart_list->count(); i++) {
    QString songIdFromWidget =
        static_cast<QLineEdit *>(
            ui->smart_list->itemWidget(ui->smart_list->item(i))
                ->findChild<QLineEdit *>("songId"))
            ->text()
            .trimmed();
    if (songId == songIdFromWidget) {
      ui->smart_list->itemWidget(ui->smart_list->item(i))
          ->findChild<QLabel *>("offline")
          ->setPixmap(QPixmap(":/icons/blank.png"));
      ui->smart_list->itemWidget(ui->smart_list->item(i))
          ->findChild<QLineEdit *>("url")
          ->setText(store_manager->getOfflineUrl(songId));
      if (store_manager->getExpiry(songId)) {
        ui->smart_list->itemWidget(ui->youtube_list->item(i))
            ->setEnabled(false);
        getAudioStream(store_manager->getYoutubeIds(songId), songId);
      }
      break;
    }
  }
}

void MainWindow::delete_video_cache(QVariant track_id) {
  QString songId = track_id.toString().remove("<br>").trimmed();
  QString setting_path =
      QStandardPaths::writableLocation(QStandardPaths::DataLocation);
  QStringList exts;
  exts << +".webm"
       << ".mp4"
       << ".mpeg"
       << ".mkv"
       << ".avi"
       << ".flv"
       << ".ogv"
       << ".ogg";
  QString ext;
  foreach (ext, exts) {
    QFile cache(setting_path + "/downloadedVideos/" + songId + ext);
    if (QFileInfo(cache).exists()) {
      cache.remove();
      break;
    }
  }
}

void MainWindow::web_watch_video(QVariant data) {
  QString btn_style =
      "QPushButton{color: silver; background-color: #45443F; border:1px solid "
      "#272727; padding-top: 3px; padding-bottom: 3px; padding-left: 3px; "
      "padding-right: 3px; border-radius: 2px; outline: none;}"
      "QPushButton:disabled { background-color: #45443F; border:1px solid "
      "#272727; padding-top: 3px; padding-bottom: 3px; padding-left: 5px; "
      "padding-right: 5px; /*border-radius: 2px;*/ color: #636363;}"
      "QPushButton:hover{border: 1px solid #272727;background-color:#5A584F; "
      "color:silver ;}"
      "QPushButton:pressed {background-color: #45443F;color: "
      "silver;padding-bottom:1px;}";

  videoOption->setStyleSheet("");
  videoOption->setStyleSheet("QWidget#VideoOption{" + ui->search->styleSheet() +
                             "}" + "QFrame{" + ui->search->styleSheet() + "}" +
                             btn_style);

  videoOption->setMetaFromWeb(data);
  videoOption->removeStyle();
  videoOption->adjustSize();
  videoOption->show();
}

void MainWindow::getAudioStream(QString ytIds, QString songId) {

  if (engine->checkEngine() == false)
    return;

  ytdlQueue.append(QStringList() << ytIds << songId);

  // update ytdlQueueLabel
  ui->ytdlQueueLabel->setText("Processing " +
                              QString::number(ytdlQueue.count()) + " tracks..");

  if (ytdlProcess == nullptr && ytdlQueue.count() > 0) {
    processYtdlQueue();
  }
}

void MainWindow::processYtdlQueue() {

  if (ytdlQueue.count() > 0) {

    // update ytdlQueueLabel
    ui->ytdlQueueLabel->setText(
        "Processing " + QString::number(ytdlQueue.count()) + " tracks..");

    QString ytIds = QString(ytdlQueue.at(0).at(0).split(",").first());
    QString songId = QString(ytdlQueue.at(0).at(1).split(",").last());

    ytdlQueue.removeAt(0);
    if (ytdlProcess == nullptr) {
      ytdlProcess = new QProcess(this);
      ytdlProcess->setObjectName(songId);

      QStringList urls = ytIds.split("<br>");
      QStringList urlsFinal;
      if (!ytIds.contains("soundcloud")) {
        for (int i = 0; i < urls.count(); i++) {
          if (!urls.at(i).isEmpty()) {
            urlsFinal.append("https://www.youtube.com/watch?v=" + urls.at(i));
          }
        }
      } else { // ytIds in case of soundcloud track is URL
        urlsFinal.append(ytIds);
      }

      QString addin_path =
          QStandardPaths::writableLocation(QStandardPaths::DataLocation);
      ytdlProcess->setProcessChannelMode(QProcess::MergedChannels);
      ytdlProcess->start("python3", QStringList()
                                        << addin_path + "/core"
                                        << "--force-ipv4"
                                        << "--get-url"
                                        << "-i"
                                        << "--extract-audio" << urlsFinal);
      ytdlProcess->waitForStarted();
      connect(ytdlProcess, SIGNAL(readyRead()), this, SLOT(ytdlReadyRead()));
      connect(ytdlProcess, SIGNAL(finished(int)), this,
              SLOT(ytdlFinished(int)));
    }
  } else {
    ui->ytdlQueueLabel->setText("no task");
    ui->similarTrackLoader->stop();
  }

  // update stream info buttons
  if (ytdlProcess != nullptr) {
    ui->ytdlStopAll->setEnabled(true);
    ui->ytdlRefreshAll->setEnabled(false);
  }
}

void MainWindow::ytdlFinished(int code) {
  Q_UNUSED(code);
  ytdlProcess->close();
  ytdlProcess = nullptr;

  if (ytdlQueue.count() > 0) {
    int count = ytdlQueue.count();
    if (count > 1) {
      ui->ytdlQueueLabel->setText("Processing " + QString::number(count) +
                                  " tracks..");
    } else {
      ui->ytdlQueueLabel->setText("Processing " + QString::number(count) +
                                  " track..");
    }
    processYtdlQueue();
  } else {
    ui->ytdlQueueLabel->setText("no task");
  }
  // update stream info buttons
  if (ytdlProcess == nullptr) {
    ui->ytdlStopAll->setEnabled(false);
    ui->ytdlRefreshAll->setEnabled(true);
  }
}

void MainWindow::ytdlReadyRead() {

  QProcess *senderProcess = qobject_cast<QProcess *>(sender());
  QString songId = senderProcess->objectName().trimmed();

  QByteArray b;
  b.append(senderProcess->readAll());
  QString s_data = QTextCodec::codecForMib(106)->toUnicode(b).trimmed();

  // qDebug()<<s_data;
  if (s_data.contains("This video is not available.", Qt::CaseInsensitive)) {
    QStringList trackMeta(store_manager->getTrack(songId));
    QString title = trackMeta.at(1);
    // QString artist = trackMeta.at(5);
    // we will remove artist here cause this will considered special case
    QString query = title; /*+" - "+artist.replace("N/A","");*/
    prepareTrack(songId, query, "0",
                 this->findChild<QListWidget *>(getCurrentPlayerQueue(songId)));
  }

  if (!s_data.isEmpty()) {
    QList<QWidget *> listWidgetItems =
        ui->right_panel->findChildren<QWidget *>("track-widget-" + songId);
    // if listwidgetItems not found in right_panel. this happens when player is
    // switched to smart mode;
    if (listWidgetItems.count() == 0) {
      listWidgetItems =
          ui->recommHolder->findChildren<QWidget *>("track-widget-" + songId);
    }
    if (listWidgetItems.isEmpty()) {
      qDebug() << "TRACK NOT FOUND IN LIST";
      senderProcess->close();
      if (senderProcess != nullptr)
        senderProcess->deleteLater();
    } else {
      foreach (QWidget *listWidgetItem, listWidgetItems) {
        QListWidget *listWidget =
            static_cast<QListWidget *>(listWidgetItem->parent()->parent());
        QString listName = listWidget->objectName();
        if (s_data.contains("https") &&
            !s_data.contains("ERROR:", Qt::CaseInsensitive)) {
          if (s_data.contains("manifest/dash/")) {
            // qDebug()<<"MENIFEAT URL:"<<s_data;
            ManifestResolver *mfr =
                new ManifestResolver(s_data.trimmed(), this);
            connect(mfr, &ManifestResolver::m4aAvailable, [=](QString m48url) {
              listWidgetItem->setEnabled(true);
              QLineEdit *url = listWidgetItem->findChild<QLineEdit *>("url");
              static_cast<QLineEdit *>(url)->setText(m48url);
              store_manager->saveStreamUrl(songId, m48url,
                                           getExpireTime(m48url));
              mfr->deleteLater();
            });
            connect(mfr, &ManifestResolver::error, [=]() {
              qDebug() << "error resolving audio node";
              return;
            });

          } else {
            listWidgetItem->setEnabled(true);
            QLineEdit *url = listWidgetItem->findChild<QLineEdit *>("url");
            QString url_str = s_data.trimmed();
            static_cast<QLineEdit *>(url)->setText(url_str);
            store_manager->saveStreamUrl(songId, url_str,
                                         getExpireTime(url_str));
            // track requested to play and url returned 403 error, so play it as
            // it is available now
            //                        QPoint pos = listWidgetItem->pos();
            //                        if(songId ==
            //                        nowPlayingSongIdWatcher->previousSongId){
            //                            listItemDoubleClicked(listWidget,listWidget->itemAt(pos));
            //                        }
          }

          // algo to assign next/previous song if current processed track is
          // next to currently playing track.
          int row;
          if (!listName.isEmpty()) {
            for (int i = 0; i < listWidget->count(); i++) {
              if (listWidget->itemWidget(listWidget->item(i))->objectName() ==
                  listWidgetItem->objectName()) {
                row = i;
                if (row + 1 <= listWidget->count()) {
                  if (row != 0) {
                    if (listWidget->itemWidget(listWidget->item(row - 1))
                            ->objectName()
                            .contains(nowPlayingSongIdWatcher->getValue())) {
                      assignNextTrack(listWidget, row);
                      ui->next->setEnabled(true);
                    }
                  }
                  if (row + 1 != listWidget->count()) {
                    if (listWidget->itemWidget(listWidget->item(row + 1))
                            ->objectName()
                            .contains(nowPlayingSongIdWatcher->getValue())) {
                      assignPreviousTrack(listWidget, row);
                      ui->previous->setEnabled(true);
                    }
                  }
                }
                break;
              }
            }
          }
          // END algo to assign next/previous song if current processed track is
          // next to currently playing track.

          // stop spinner
          if (listName == "smart_list") {
            ui->similarTrackLoader->stop();
          }
          // delete process/task
          senderProcess->close();
          if (senderProcess != nullptr)
            senderProcess->deleteLater();
        }
      }
    }
  }
}

QString MainWindow::getExpireTime(const QString urlStr) {
  QString expiryTime = QUrlQuery(QUrl::fromPercentEncoding(urlStr.toUtf8()))
                           .queryItemValue("expire")
                           .trimmed();

  if (expiryTime.isEmpty() || !isNumericStr(expiryTime)) {
    expiryTime =
        QUrlQuery(QUrl(urlStr.toUtf8())).queryItemValue("expire").trimmed();
  }

  if (expiryTime.isEmpty() || !isNumericStr(expiryTime)) {
    expiryTime = urlStr.split("/expire/").last().split("/").first().trimmed();
  }

  if (expiryTime.isEmpty() || !isNumericStr(expiryTime)) {
    expiryTime = urlStr.split("expire=").last().split("&").first();
  }

  if (expiryTime.isEmpty() || !isNumericStr(expiryTime)) {
    // set expiry time for soundcloud links
    if (urlStr.contains("media.sndcdn")) {
      expiryTime = QString::number(
          (QDateTime::currentMSecsSinceEpoch() / 1000) +
          2400); // 40 minutes (these urls expires in 50 minutes)
    } else {
      expiryTime = QString::number(
          (QDateTime::currentMSecsSinceEpoch() / 1000) + 18000); // 5 hours
    }
  }
  return expiryTime;
}

bool MainWindow::isNumericStr(const QString str) {
  bool isNumeric;
  str.toInt(&isNumeric, 16);
  str.toInt(&isNumeric, 10);
  return isNumeric;
}

void MainWindow::on_left_list_currentRowChanged(int currentRow) {
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
    show_local_saved_songs();
    break;
  case 7:
    show_local_saved_videos();
    break;
  case 8:
    show_liked_songs();
    break;
  case 9:
    show_playlists();
    break;
  case 10:
    show_recently_played();
    break;
  case 12:
    browse_youtube();
    break;
  case 13:
    browse_youtube_playlist();
    break;
  case 14:
    browse_soundcloud();
    break;
  case 15:
    internet_radio();
    break;
  }
}

void MainWindow::browse_youtube() {
  pageType = "youtube";
  ui->webview->load(QUrl("qrc:///web/youtube/youtube.html"));
}

void MainWindow::browse_youtube_playlist() {
  pageType = "youtube_playlist";
  ui->webview->load(QUrl("qrc:///web/youtube/playlist.html"));
}

void MainWindow::recommendations() {
  pageType = "recommendation";
  ui->webview->load(QUrl("qrc:///web/recommendation/recommendation.html"));
}

void MainWindow::clear_youtubeSearchTerm() { youtubeSearchTerm.clear(); }

void MainWindow::on_search_returnPressed() { search(""); }

void MainWindow::browse() {
  pageType = "browse";
  ui->left_list->setCurrentRow(1);
  //     ui->webview->load(QUrl("file:///tmp/index.html"));
  ui->webview->load(QUrl("qrc:///web/browse/browse.html"));
}

void MainWindow::search(QString offset) {
  pageType = "search";
  if (offset.isEmpty()) {
    ui->webview->load(QUrl("qrc:///web/search/search.html"));
  } else { // search page loaded perform js
    QString term = ui->search->text();
    term = term.replace(" ", "+");
    term = term.replace("'", "%27");
    if (offset.contains(term)) {
      offset = offset.remove(term);
    }
    term.append(offset);
    ui->webview->page()->mainFrame()->evaluateJavaScript("track_search('" +
                                                         term + "')");
  }
}

void MainWindow::show_top() {
  pageType = "top";
  ui->webview->load(QUrl("qrc:///web/top/top.html"));
}

void MainWindow::show_local_saved_songs() {
  pageType = "local_saved_songs";
  ui->webview->load(QUrl("qrc:///web/local_songs/local_songs.html"));
}

void MainWindow::show_local_saved_videos() {
  pageType = "local_saved_videos";
  ui->webview->load(QUrl("qrc:///web/local_videos/local_videos.html"));
}

void MainWindow::show_liked_songs() {
  pageType = "liked_songs";
  ui->webview->load(QUrl("qrc:///web/liked_songs/liked_songs.html"));
}

void MainWindow::show_playlists() {
  pageType = "playlists";
  ui->webview->load(QUrl("qrc:///web/playlists/playlists.html"));
}

void MainWindow::show_recently_played() {
  pageType = "recently_played";
  ui->webview->load(QUrl("qrc:///web/recently_played/recent.html"));
}

void MainWindow::internet_radio() {
  pageType = "radio";
  ui->webview->load(QUrl("qrc:///web/radio/radio.html"));
}

void MainWindow::browse_soundcloud() {
  if (soundcloud->cidEmpty()) {
    soundcloud->getHome();
  }
  pageType = "soundcloud";
  ui->webview->load(QUrl("qrc:///web/soundcloud/soundcloud.html"));
}

// PLAY TRACK ON ITEM DOUBLE
// CLICKED////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::on_olivia_list_itemDoubleClicked(QListWidgetItem *item) {
  playingSongRadio = false;
  listItemDoubleClicked(ui->olivia_list, item);
}

void MainWindow::on_smart_list_itemDoubleClicked(QListWidgetItem *item) {
  if (!settingsObj.value("smart_playlist", true).toBool()) {
    playingSongRadio = true;
  }
  listItemDoubleClicked(ui->smart_list, item);
}

void MainWindow::on_youtube_list_itemDoubleClicked(QListWidgetItem *item) {
  playingSongRadio = false;
  listItemDoubleClicked(ui->youtube_list, item);
}

void MainWindow::listItemDoubleClicked(QListWidget *list,
                                       QListWidgetItem *item) {

  listClearSelection(list->objectName());
  // clear debug console
  ui->console->clear();
  // hide console
  if (ui->debug_widget->isVisible())
    ui->debug_widget->hide();

  // prevent crash while playing song which is not in queue or removed by user
  if (item == nullptr)
    return;
  if (!list->itemWidget(item)->isEnabled())
    return;

  QString url = list->itemWidget(item)->findChild<QLineEdit *>("url")->text();
  QString songId =
      list->itemWidget(item)->findChild<QLineEdit *>("songId")->text();

  ElidedLabel *title =
      list->itemWidget(item)->findChild<ElidedLabel *>("title_elided");
  QString titleStr = static_cast<ElidedLabel *>(title)->text();

  ElidedLabel *artist =
      list->itemWidget(item)->findChild<ElidedLabel *>("artist_elided");
  QString artistStr = static_cast<ElidedLabel *>(artist)->text();

  ElidedLabel *album =
      list->itemWidget(item)->findChild<ElidedLabel *>("album_elided");
  QString albumStr = static_cast<ElidedLabel *>(album)->text();

  QString dominant_color =
      list->itemWidget(item)->findChild<QLineEdit *>("dominant_color")->text();

  QLabel *cover = list->itemWidget(item)->findChild<QLabel *>("cover");
  ui->cover->setPixmap(
      QPixmap(*cover->pixmap())
          .scaled(100, 100, Qt::KeepAspectRatio, Qt::SmoothTransformation));
  QPixmap(*cover->pixmap())
      .save(QString(setting_path + "/albumArts/" + "currentArt.png"), 0);

  setTrackItemNowPlaying();

  // show now playing on current track
  QLabel *playing = list->itemWidget(item)->findChild<QLabel *>("playing");
  playing->setPixmap(QPixmap(":/icons/now_playing.png")
                         .scaled(playing->size(), Qt::KeepAspectRatio,
                                 Qt::SmoothTransformation));
  playing->setToolTip("playing...");

  if (list->currentRow() == list->count() - 1) {
    // set loading playlist = false if the track are being played from normal
    // queues
    if (list->objectName() == "olivia_list" ||
        list->objectName() == "youtube_list") {
      currentSimilarTrackProcessing = 0;
      similarTracks->isLoadingPLaylist = false;
    }
    if (settingsObj.value("smart_playlist", true).toBool() ||
        playingSongRadio) {
      qDebug() << "called starter" << songId;
      startGetRecommendedTrackForAutoPlayTimer(songId);
    } else {
      // clear the list to respect the disabled smartplaylist in runtime.
      similarTracks->clearList();
      // alternative of calling similarTracks->failedGetSimilarTracks(); cause
      // we don't want to show toast
      similarTracks->parentSongId = "";
      ui->similarTrackLoader->stop();
      // hide smart playlist if visible
      if (ui->recommWidget->isVisible())
        ui->show_hide_smart_list_button->click();
    }
  }

  nowPlayingSongIdWatcher->isRadioStation = false;

  nowPlayingSongIdWatcher->setValue(songId);

  // decides next and previous song
  nextPreviousHelper(list);

  ui->webview->page()->mainFrame()->evaluateJavaScript(
      "NowPlayingTrackId='" + nowPlayingSongIdWatcher->getValue() + "'");

  ui->nowplaying_widget->setImage(*cover->pixmap());

  ui->state->setText("loading");

  ui->nowP_title->setText(titleStr);
  if (settingsObj.value("marquee", false).toBool()) {
    ui->playing->setText(titleStr);
  } else {
    ui->playing->setText("");
  }

  ElidedLabel *artist2 = this->findChild<ElidedLabel *>("nowP_artist");
  static_cast<ElidedLabel *>(artist2)->setText(artistStr);

  ElidedLabel *album2 = this->findChild<ElidedLabel *>("nowP_album");
  static_cast<ElidedLabel *>(album2)->setText(albumStr);

  if (store_manager->isDownloaded(songId)) {
    radio_manager->playRadio(false, QUrl(url));
  } else {
    saveTracksAfterBuffer =
        settingsObj.value("saveAfterBuffer", "true").toBool();
    // do not save tracks if playing from similar tracks list
    if (list->objectName() == "smart_list") {
      radio_manager->playRadio(false, QUrl(url));
    } else {
      radio_manager->playRadio(saveTracksAfterBuffer, QUrl(url));
    }
  }

  // update metadata of MPRIS interface
  if (dp != nullptr) {
    mpris_song_meta.insert("mpris:length", 500000);
    mpris_song_meta.insert("xesam:album", albumStr);
    mpris_song_meta.insert("mpris:artUrl",
                           setting_path + "/albumArts/" + "currentArt.png");
    mpris_song_meta.insert("xesam:title", titleStr);
    mpris_song_meta.insert("xesam:artist", artistStr);
    mpris_song_meta.insert("mpris:trackid", songId);
    QVariantList artistlist;
    artistlist.append(QVariant(artistStr));
    mpris_song_meta.insert("xesam:albumArtist", artistlist);
    mpris_song_meta.insert("xesam:url", url);
    emit dp->currentSongChanged(mpris_song_meta);
  }

  // TODO this is making app slow when the list of tracks if huge
  if (settingsObj.value("dynamicTheme").toBool() == true) {
    QString r, g, b;
    QStringList colors = dominant_color.split(",");
    if (colors.count() == 3) {
      r = colors.at(0);
      g = colors.at(1);
      b = colors.at(2);
    } else {
      r = "98";
      g = "164";
      b = "205";
    }
    set_app_theme(QColor(r.toInt(), g.toInt(), b.toInt()));
  }

  if (list->itemWidget(item)->rect().contains(
          list->itemWidget(item)->mapFromGlobal(QCursor::pos()))) {
    notify("", "Playing: " + titleStr);
  }

  // add to recentlyPlayed tracks
  store_manager->add_recently_played(songId);
  // reload recent page if page is recently_played
  if (pageType == "recently_played") {
    ui->webview->page()->mainFrame()->evaluateJavaScript(
        "open_recently_tracks()");
  }
  // check favourite and set favourite icon
  if (songId.isEmpty() || songId == "0000000") {
    ui->favourite->setChecked(false);
    ui->favourite->setEnabled(false);
    ui->favourite->setIcon(QIcon(":/icons/sidebar/liked_disabled.png"));
  } else {
    ui->favourite->setEnabled(true);
    // check if track is favourite and set button accordingly
    setFavouriteButton(
        store_manager->is_liked_track(nowPlayingSongIdWatcher->getValue()));
  }
}
// END PLAY TRACK ON ITEM DOUBLE
// CLICKED////////////////////////////////////////////////////////////////////////////////////////

void MainWindow::nextPreviousHelper(QListWidget *list) {

  // assign previous track to previous button
  if (list->currentRow() == 0) {
    ui->previous->setEnabled(false);
  } else {
    // assign previous track in list if next item in list is valid
    for (int i = list->currentRow() - 1; i > -1; i--) {
      if (list->itemWidget(list->item(i))->isEnabled()) {
        ui->previous->setEnabled(true);
        assignPreviousTrack(list, i);
        break;
      }
    }
  }

  // assign next track to next button
  if (!ui->shuffle->isChecked() &&
      (list->currentRow() == list->count() - 1 &&
       ui->repeat->checkState() !=
           Qt::PartiallyChecked)) { // is last track and not repeating current
                                    // song
    ui->next->setEnabled(false);
  } else if (ui->repeat->checkState() ==
             Qt::PartiallyChecked) { // repeat current song
    QListWidget *list = this->findChild<QListWidget *>(
        getCurrentPlayerQueue(nowPlayingSongIdWatcher->getValue()));
    if (list != nullptr) {
      QWidget *listWidgetItem = list->findChild<QWidget *>(
          "track-widget-" + nowPlayingSongIdWatcher->getValue());
      if (listWidgetItem != nullptr) {
        QPoint pos = listWidgetItem->pos();
        int index = list->row(list->itemAt(pos));
        assignNextTrack(list, index);
      }
    }
  } else {
    // assign next track in list if next item in list is valid
    for (int i = list->currentRow() + 1; i < list->count(); i++) {
      if (list->itemWidget(list->item(i))->isEnabled()) {
        ui->next->setEnabled(true);
        assignNextTrack(list, i);
        break;
      } else {
        ui->next->setEnabled(false);
      }
    }
  }
}

// app menu to hide show sidebar
void MainWindow::on_menu_clicked() {
  QSplitter *split2 = this->findChild<QSplitter *>("split2");
  if (static_cast<QSplitter *>(split2)->sizes()[0] == 0) { // closed state
    QPropertyAnimation *animation =
        new QPropertyAnimation(ui->left_panel, "geometry");
    animation->setDuration(100);
    QRect startRect = ui->left_panel->rect();
    QRect finalRect = startRect;
    finalRect.setWidth(left_panel_width);
    animation->setStartValue(startRect);
    animation->setEndValue(finalRect);
    animation->setEasingCurve(QEasingCurve::InCurve);
    animation->start(QAbstractAnimation::DeleteWhenStopped);
    animationRunning = true;
    connect(animation, &QPropertyAnimation::valueChanged, [=](QVariant var) {
      static_cast<QSplitter *>(split2)->setSizes(
          QList<int>() << var.toRect().width()
                       << static_cast<QSplitter *>(split2)->sizes()[1] -
                              var.toRect().width());
    });
    connect(animation, &QPropertyAnimation::finished,
            [=]() { animationRunning = false; });
    ui->menu->setIcon(QIcon(":/icons/close.png"));
  } else {
    QPropertyAnimation *animation =
        new QPropertyAnimation(ui->left_panel, "geometry");
    animation->setDuration(100);
    QRect startRect = ui->left_panel->rect();
    QRect finalRect = startRect;
    finalRect.setWidth(0);
    animation->setStartValue(startRect);
    animation->setEndValue(finalRect);
    animation->setEasingCurve(QEasingCurve::OutCurve);
    animation->start(QAbstractAnimation::DeleteWhenStopped);
    animationRunning = true;
    connect(animation, &QPropertyAnimation::valueChanged, [=](QVariant var) {
      static_cast<QSplitter *>(split2)->setSizes(
          QList<int>() << var.toRect().width()
                       << static_cast<QSplitter *>(split2)->sizes()[1] +
                              var.toRect().width());
    });
    connect(animation, &QPropertyAnimation::finished,
            [=]() { animationRunning = false; });
    ui->menu->setIcon(QIcon(":/icons/menu.png"));
  }
}

void MainWindow::on_settings_clicked() {
  if (!settingsWidget->isVisible()) {
    settingsWidget->setStyleSheet("QWidget#settingsWidget{" +
                                  ui->search->styleSheet() + "}");
    // refresh cache sizes
    QString setting_path =
        QStandardPaths::writableLocation(QStandardPaths::DataLocation);
    utils *util = new utils(this);
    settingsUi.cached_tracks_size->setText(
        util->refreshCacheSize(setting_path + "/downloadedTracks"));
    settingsUi.cached_videos_size->setText(
        util->refreshCacheSize(setting_path + "/downloadedVideos"));
    settingsUi.offline_pages_size->setText(
        util->refreshCacheSize(setting_path + "/paginator"));
    settingsUi.database_size->setText(
        util->refreshCacheSize(setting_path + "/storeDatabase/" + database));
    util->deleteLater();

    settingsWidget->showNormal();
  }
}

void MainWindow::resizeEvent(QResizeEvent *resizeEvent) {
  left_panel_width = ui->left_panel->width();
  ui->tabWidget->resize(ui->tabWidget->size().width() - 1,
                        ui->tabWidget->size().height());
  ui->tabWidget->resize(ui->tabWidget->size().width() + 1,
                        ui->tabWidget->size().height());
  QMainWindow::resizeEvent(resizeEvent);
}

// called from common.js
void MainWindow::showAjaxError() {
  qDebug() << "NETWORK ERROR OR USER CANCELED REQUEST";
}
// called from common.js
void MainWindow::hidePlaylistButton() { ui->playlistLoaderButtton->hide(); }
// called from common.js
void MainWindow::checkForPlaylist() {
  QVariant ulCount = ui->webview->page()->mainFrame()->evaluateJavaScript(
      "document.querySelector('.ui-page-active').querySelectorAll('ul')."
      "length");
  if (ulCount.isValid()) {
    int totalUl = ulCount.toInt();
    // find ul which contains gettrackinfo
    for (int i = 0; i < totalUl; i++) {
      QVariant validList = ui->webview->page()->mainFrame()->evaluateJavaScript(
          "document.querySelector('.ui-page-active').querySelectorAll('ul')[" +
          QString::number(i) + "].innerHTML.includes('gettrackinfo')");
      if (validList.isValid() && validList.toBool() == true) {
        ui->webview->page()->mainFrame()->evaluateJavaScript(
            "var playlist = "
            "document.querySelector('.ui-page-active').querySelectorAll('ul')"
            "[" +
            QString::number(i) + "]");
        // this shows add all buttton in webview.
        QString js =
            "playlist.innerHTML = \"<a id='playall' class='ui-btn ui-shadow "
            "ui-corner-all ui-btn-icon-left ui-icon-playall' "
            "onclick='mainwindow.on_playlistLoaderButtton_clicked();' "
            "data-role='button'>Play All</a>\" + playlist.innerHTML";
        ui->webview->page()->mainFrame()->evaluateJavaScript(js);
        // QToolTip::showText( ui->playlistLoaderButtton->mapToGlobal( QPoint(
        // 0, 0 ) ),
        // ui->playlistLoaderButtton->toolTip(),ui->playlistLoaderButtton,ui->playlistLoaderButtton->rect(),3000);
        ui->playlistLoaderButtton->show();
        break;
      }
    }
  }
}

void MainWindow::showAddToSmartPlaylist() { ui->playlistLoaderButtton->show(); }

void MainWindow::playAllLocalSongs() { similarTracks->addLocalSongs(); }

// returns theme color from common.js
void MainWindow::setThemeColor(QString color) { themeColor = color; }

////////////////////////////////////////////////////////////RADIO///////////////////////////////////////////////////////

void MainWindow::radioStatus(QString radioState) {
  if (radioState == "playing") {
    ui->stop->setEnabled(true);
    ui->play_pause->setEnabled(true);
    ui->play_pause->setIcon(QIcon(":/icons/p_pause.png"));
  } else if (radioState == "paused") {
    ui->stop->setEnabled(true);
    ui->play_pause->setEnabled(true);
    ui->play_pause->setIcon(QIcon(":/icons/p_play.png"));
  } else if (radioState == "eof") {
    ui->playing->setText("");
    ui->stop->setEnabled(false);
    ui->play_pause->setEnabled(false);
    ui->play_pause->setIcon(QIcon(":/icons/p_play.png"));
    // remove nowplaying from central widget
    setTrackItemNowPlaying();
    nowPlayingSongIdWatcher->previousSongId =
        nowPlayingSongIdWatcher->getValue();
    nowPlayingSongIdWatcher->setValue("0000000"); // null

    // play next track
    if (ui->next->isEnabled() /*&& !smartMode*/) {
      ui->next->click();
    } else {
      // assign next track from smart playlist if the smart mode is on
      // also populate smart playlist when smart mode is enabled
      if (smartMode) {
        // assign next track in list if next item in list is valid
        for (int i = 0; i < ui->smart_list->count(); i++) {
          if (ui->smart_list->itemWidget(ui->smart_list->item(i))
                  ->isEnabled()) {
            ui->next->setEnabled(true);
            assignNextTrack(ui->smart_list, i);
            break;
          } else {
            ui->next->setEnabled(false);
          }
        }
        if (ui->next->isEnabled()) {
          ui->next->click();
        }
        return;
      }

      // proceed with normal smart play mode
      if (!settingsObj.value("shuffle").toBool()) {
        qDebug() << "List ended play related songs";
        // show smart list if enabled
        if (settingsObj.value("smart_playlist", true).toBool() &&
            !ui->recommWidget->isVisible()) {
          ui->show_hide_smart_list_button->click();
        }
        // check if last track is playing from a playlist if thats the case
        // prevent from reasigning smae track again
        if (this->similarTracks->isLoadingPLaylist &&
            currentSimilarTrackList.count() < 1) {
          radio_manager->radioPlaybackTimer->stop();
          ui->state->setText(radioState);
          ui->previous->setEnabled(false);
          return;
        }
        // assign next track in list if next item in list is valid
        for (int i = 0; i < ui->smart_list->count(); i++) {
          if (ui->smart_list->itemWidget(ui->smart_list->item(i))
                  ->isEnabled()) {
            ui->next->setEnabled(true);
            assignNextTrack(ui->smart_list, i);
            break;
          } else {
            ui->next->setEnabled(false);
          }
        }
        if (ui->next->isEnabled()) {
          ui->next->click();
        }
      }
    }
  } else if (radioState == "stopped") {
    ui->playing->setText("");
    setTrackItemNowPlaying();
    nowPlayingSongIdWatcher->setValue("0000000");
    ui->stop->setEnabled(false);
    ui->play_pause->setEnabled(false);
    ui->play_pause->setIcon(QIcon(":/icons/p_play.png"));
  }
  if (radioState == "failed") {
    ui->playing->setText("");
    setTrackItemNowPlaying();
    nowPlayingSongIdWatcher->setValue("0000000");
    if (!ui->debug_widget->isVisible())
      ui->debug_widget->show();
  }
  ui->state->setText(radioState);
}

// removes playing icon and tooltips from lists trackItem
void MainWindow::setTrackItemNowPlaying() {
  QString previousNowPlayingTrackId = nowPlayingSongIdWatcher->getValue();
  QList<QWidget *> listWidgetItems = ui->right_panel->findChildren<QWidget *>(
      "track-widget-" + previousNowPlayingTrackId);
  // if listwidgetItems not found in right_panel. this happens when player is
  // witched to smart mode;
  //       if(listWidgetItems.count()==0){
  listWidgetItems.append(ui->recommHolder->findChildren<QWidget *>(
      "track-widget-" + previousNowPlayingTrackId));
  //       }
  // check if the track with similarId is also present in smaratplaylist too
  // if found remove nowplaying icon from it too
  foreach (QWidget *listWidgetItem, listWidgetItems) {
    listWidgetItem->findChild<QLabel *>("playing")->setToolTip("");
    listWidgetItem->findChild<QLabel *>("playing")->setPixmap(
        QPixmap(":/icons/blank.png"));
  }
}

void MainWindow::radioPosition(int pos) {
  int seconds = (pos) % 60;
  int minutes = (pos / 60) % 60;
  int hours = (pos / 3600) % 24;
  QTime time(hours, minutes, seconds);
  ui->position->setText(time.toString());
  ui->radioSeekSlider->setValue(pos);

  // update mpris position
  if (dp != nullptr) {
    dp->playerPosition = static_cast<qlonglong>(pos * 1000000);
  }
}

void MainWindow::radioDuration(int dur) {
  int seconds = (dur) % 60;
  int minutes = (dur / 60) % 60;
  int hours = (dur / 3600) % 24;
  QTime time(hours, minutes, seconds);
  ui->duration->setText(time.toString());
  ui->radioSeekSlider->setMaximum(dur);
  qlonglong duration = static_cast<qlonglong>(dur * 1000000);

  if (dp != nullptr && duration != mpris_song_meta.value("mpris:length")) {
    mpris_song_meta.remove("mpris:length");
    mpris_song_meta.insert("mpris:length", duration);
    emit dp->currentSongChanged(mpris_song_meta);
  }
}

void MainWindow::radio_demuxer_cache_duration_changed(
    double seconds_available, double radio_playerPosition) {
  if (ui->radioSeekSlider->maximum() != 0 &&
      qFuzzyCompare(seconds_available, 0) == false &&
      qFuzzyCompare(radio_playerPosition, 0) == false) {
    double totalSeconds = seconds_available + radio_playerPosition;
    double width = static_cast<double>(totalSeconds) /
                   static_cast<double>(ui->radioSeekSlider->maximum());
    ui->radioSeekSlider->subControlWidth = static_cast<double>(width * 100);
  }
}

// wrapper function to playLocalTrack function which finds song in queue and
// play it if found or add and play it if not found
void MainWindow::playSongById(QVariant songIdVar) { playLocalTrack(songIdVar); }

// invokable method used to only add track to player
void MainWindow::addToQueueFromLocal(QVariant songIdVar) {
  QString url, songId, title, album, artist, base64;
  songId = songIdVar.toString();
  qDebug() << songId;
  // prevent adding tracks with no songId it causes trouble
  if (songId.isEmpty())
    return;
  QStringList tracskMeta;
  tracskMeta = store_manager->getTrack(songId);

  title = tracskMeta[1];
  album = tracskMeta[3];
  artist = tracskMeta[5];
  base64 = tracskMeta[6];
  QString setting_path =
      QStandardPaths::writableLocation(QStandardPaths::DataLocation);
  url = "file://" + setting_path + "/downloadedTracks/" + songId;

  // if track is not in lists add it to queue
  if (!store_manager->isInQueue(songIdVar.toString())) {
    QStringList trackDetails = store_manager->getTrack(songId);
    QString id, title, artist, album, base64, dominantColor, albumId, artistId,
        url;
    title = trackDetails.at(1);
    albumId = trackDetails.at(2);
    album = trackDetails.at(3);
    artistId = trackDetails.at(4);
    artist = trackDetails.at(5);
    base64 = trackDetails.at(6);
    url = trackDetails.at(7);
    id = trackDetails.at(8);
    dominantColor = trackDetails.at(9);
    addToQueue(id, title, artist, album, base64, dominantColor, songId, albumId,
               artistId);
  }
}

// this method plays tracks from webpage
void MainWindow::playLocalTrack(QVariant songIdVar) {
  QString songId = songIdVar.toString();
  // if song is not in quque
  addToQueueFromLocal(songIdVar);
  // else find and play the track
  for (int i = 0; i < ui->olivia_list->count(); i++) {
    QString songIdFromWidget =
        static_cast<QLineEdit *>(
            ui->olivia_list->itemWidget(ui->olivia_list->item(i))
                ->findChild<QLineEdit *>("songId"))
            ->text()
            .trimmed();
    if (songId == songIdFromWidget) {
      ui->tabWidget->setCurrentWidget(ui->tab);
      ui->olivia_list->setCurrentRow(i);
      if (ui->olivia_list->itemWidget(ui->olivia_list->item(i))->isEnabled()) {
        listItemDoubleClicked(ui->olivia_list, ui->olivia_list->item(i));
      }
      ui->olivia_list->scrollToItem(ui->olivia_list->item(i));
      break;
    }
  }
  for (int i = 0; i < ui->youtube_list->count(); i++) {
    QString songIdFromWidget =
        static_cast<QLineEdit *>(
            ui->youtube_list->itemWidget(ui->youtube_list->item(i))
                ->findChild<QLineEdit *>("songId"))
            ->text()
            .trimmed();
    if (songId == songIdFromWidget) {
      ui->tabWidget->setCurrentWidget(ui->tab_2);
      ui->youtube_list->setCurrentRow(i);
      if (ui->youtube_list->itemWidget(ui->youtube_list->item(i))
              ->isEnabled()) {
        listItemDoubleClicked(ui->youtube_list, ui->youtube_list->item(i));
      }
      ui->youtube_list->scrollToItem(ui->youtube_list->item(i));
      break;
    }
  }
  similarTracks->clearList();
}

void MainWindow::saveRadioChannelToFavourite(QVariant channelInfo) {
  store_manager->setRadioChannelToFavourite(channelInfo.toStringList());
}

void MainWindow::playRadioFromWeb(QVariant streamDetails) {
  // clear playing icon from player queue
  setTrackItemNowPlaying();
  ui->console->clear();
  QString url, title, country, language, base64, stationId;
  QStringList list = streamDetails.toString().split("=,=");
  stationId = list.at(0);
  url = list.at(1);
  title = list.at(2);
  country = list.at(3);
  language = list.at(4);
  if (list.count() > 5) {
    base64 = list.at(5);
    base64 = base64.split("base64,").last();
    QByteArray ba = base64.toUtf8();
    QPixmap image;
    image.loadFromData(QByteArray::fromBase64(ba));
    if (!image.isNull()) {
      ui->cover->setPixmap(QPixmap(image).scaled(100, 100, Qt::KeepAspectRatio,
                                                 Qt::SmoothTransformation));
    }
  } else {
    ui->cover->setPixmap(QPixmap(":/web/radio/station_cover.jpg"));
  }

  QString titleStr = htmlToPlainText(title);
  ui->nowP_title->setText(titleStr);
  notify("", "Playing: " + titleStr);
  if (settingsObj.value("marquee", false).toBool()) {
    ui->playing->setText(titleStr);
  } else {
    ui->playing->setText("");
  }

  this->findChild<ElidedLabel *>("nowP_artist")->setText(language);

  this->findChild<ElidedLabel *>("nowP_album")->setText(country);

  // always false as we don't want record radio for now
  saveTracksAfterBuffer = false;
  radio_manager->playRadio(saveTracksAfterBuffer, QUrl(url.trimmed()));
  nowPlayingSongIdWatcher->setValue(stationId);
  nowPlayingSongIdWatcher->isRadioStation = true;
  ui->next->setEnabled(false);
  ui->previous->setEnabled(false);

  // check if channel is in favourite list
  if (stationId.isEmpty() || stationId == "0000000") {
    ui->favourite->setChecked(false);
    ui->favourite->setEnabled(false);
    ui->favourite->setIcon(QIcon(":/icons/sidebar/liked_disabled.png"));
  } else {
    ui->favourite->setEnabled(true);
    // check if channel is favourite and set button accordingly
    setFavouriteButton(store_manager->is_favourite_station(
        nowPlayingSongIdWatcher->getValue()));
  }
}

void MainWindow::saveTrack(QString format) {
  Q_UNUSED(format);
  QString download_Path =
      QStandardPaths::writableLocation(QStandardPaths::DataLocation) +
      "/downloadedTracks/";

  QString downloadTemp_Path =
      QStandardPaths::writableLocation(QStandardPaths::DataLocation) +
      "/downloadedTemp/";
  QFile file(downloadTemp_Path + "current.temp"); //+"."+format

  if (file.copy(download_Path + nowPlayingSongIdWatcher->getValue())) {
    file.close();
    //    qDebug()<<"saved"<<nowPlayingSongIdWatcher->getValue()<<"as"<<format<<"in"<<file.fileName();
    store_manager->update_track("downloaded",
                                nowPlayingSongIdWatcher->getValue(), "1");
  }
  updateTrack(nowPlayingSongIdWatcher->getValue(), download_Path);
}

// updates track after downloaded
void MainWindow::updateTrack(QString trackId, QString download_Path) {
  QString listName = getCurrentPlayerQueue(trackId);
  if (!listName.isEmpty()) {
    QListWidget *listWidget = this->findChild<QListWidget *>(listName);
    QWidget *listWidgetItem =
        listWidget->findChild<QWidget *>("track-widget-" + trackId);
    QLabel *offline = listWidgetItem->findChild<QLabel *>("offline");
    static_cast<QLabel *>(offline)->setPixmap(
        QPixmap(":/icons/offline.png")
            .scaled(track_ui.offline->size(), Qt::KeepAspectRatio,
                    Qt::SmoothTransformation));
    listWidgetItem->findChild<QLineEdit *>("url")->setText(
        "file://" + download_Path + trackId);
  }
}

////////////////////////////////////////////////////////////END
/// RADIO///////////////////////////////////////////////////////

// Filter
// Lists///////////////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::on_filter_olivia_textChanged(const QString &arg1) {
  filterList(arg1, ui->olivia_list); // olivia
}

void MainWindow::on_filter_youtube_textChanged(const QString &arg1) {
  filterList(arg1, ui->youtube_list); // youtube
}

void MainWindow::filterList(const QString &arg1, QListWidget *list) {
  hideListItems(list);

  OliviaMetaList.clear();
  fillOliviaMetaList(list);

  for (int i = 0; i < OliviaMetaList.count(); i++) {
    if (QString(OliviaMetaList.at(i)).contains(arg1, Qt::CaseInsensitive)) {
      QListWidgetItem *item = list->item(i);
      item->setHidden(false);
    }
  }
}
void MainWindow::fillOliviaMetaList(QListWidget *list) {
  for (int i = 0; i < list->count(); i++) {
    QListWidgetItem *item = list->item(i);

    ElidedLabel *title =
        list->itemWidget(item)->findChild<ElidedLabel *>("title_elided");
    QString titleStr = static_cast<ElidedLabel *>(title)->text();

    ElidedLabel *artist =
        list->itemWidget(item)->findChild<ElidedLabel *>("artist_elided");
    QString artistStr = static_cast<ElidedLabel *>(artist)->text();

    ElidedLabel *album =
        list->itemWidget(item)->findChild<ElidedLabel *>("album_elided");
    QString albumStr = static_cast<ElidedLabel *>(album)->text();
    // QString albumStr = album->text();
    OliviaMetaList.append(titleStr + " " + artistStr + "  " + albumStr);
  }
}

void MainWindow::hideListItems(QListWidget *list) {
  for (int i = 0; i < list->count(); i++) {
    list->item(i)->setHidden(true);
  }
}
// End Filter
// Lists///////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Webview dpi
// set///////////////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::zoomin() {
  zoom = zoom - 1.0;
  setZoom(zoom);
  settingsUi.zoom->setText(QString::number(ui->webview->zoomFactor(), 'f', 2));
}

void MainWindow::zoomout() {
  zoom = zoom + 1.0;
  setZoom(zoom);
  settingsUi.zoom->setText(QString::number(ui->webview->zoomFactor(), 'f', 2));
}

// use this only when page is loaded in webview
QString MainWindow::htmlEntityDecode(const QString &str) {
  QVariant result = ui->webview->page()->mainFrame()->evaluateJavaScript(
      "he.decode('" + str + "')");
  return result.toString();
}

void MainWindow::setZoom(qreal val) {
  ui->webview->setZoomFactor(horizontalDpi / val);
  settingsObj.setValue("zoom", zoom);
}

void MainWindow::init_backup() {
  backup = new Backup(this, setting_path, &settingsObj);
  backup->setWindowTitle(utils::toCamelCase(QApplication::applicationName()) +
                         " | " + "BackUp and Restore");
  backup->setWindowFlags(Qt::Dialog);
  backup->setWindowModality(Qt::NonModal);
  connect(backup, &Backup::app_restart_required, [=]() { restart_required(); });
}

void MainWindow::init_lyrics() {
  lyricsWidget = new Lyrics(this);
  lyricsWidget->setWindowFlags(Qt::Dialog);
  lyricsWidget->setWindowModality(Qt::NonModal);
}

void MainWindow::init_soundCloud() { soundcloud = new SoundCloud(this); }

void MainWindow::init_smartMode() {
  smartModeWidget = new QWidget(this);
  smartMode_ui.setupUi(smartModeWidget);
  smartModeWidget->setObjectName("smartModeWidget");
  smartModeWidget->setWindowTitle(
      utils::toCamelCase(QApplication::applicationName()) + " | Smart Mode");
  smartModeWidget->setWindowFlags(
      this->windowFlags() |
      Qt::Window /*| Qt::CustomizeWindowHint*/ /*| Qt::FramelessWindowHint*/);
  smartModeWidget->setWindowModality(Qt::NonModal);
  smartModeWidget->adjustSize();
}

void MainWindow::init_miniMode() {
  miniModeWidget = new QWidget(this);
  miniMode_ui.setupUi(miniModeWidget);
  miniModeWidget->setObjectName("miniModeWidget");
  if (settingsObj.value("miniModeStayOnTop", "false").toBool() == true) {
    miniModeWidget->setWindowFlags(Qt::Window | Qt::CustomizeWindowHint |
                                   Qt::WindowStaysOnTopHint |
                                   Qt::FramelessWindowHint);
  } else {
    miniModeWidget->setWindowFlags(Qt::Window | Qt::CustomizeWindowHint |
                                   Qt::FramelessWindowHint);
  }
  miniModeWidget->setWindowModality(Qt::ApplicationModal);
  miniModeWidget->adjustSize();
}

void MainWindow::on_miniMode_clicked() {
  if (!miniModeWidget->isVisible()) {
    ui->miniMode->setIcon(QIcon(":/icons/restore_mini_mode.png"));
    ui->miniMode->setToolTip("Restore back to full mode");
    miniMode_ui.nowPlayingLayout->addWidget(ui->nowplaying_widget);
    miniMode_ui.seekSliderLyout->addWidget(ui->position);
    miniMode_ui.seekSliderLyout->addWidget(ui->radioSeekSlider);
    miniMode_ui.seekSliderLyout->addWidget(ui->duration);
    ui->radioVolumeSlider->setMaximumWidth(16777215);
    miniMode_ui.volumeSliderLayout->addWidget(ui->radioVolumeSlider);
    miniMode_ui.controlLayout->addWidget(ui->controls_widget);
    miniModeWidget->move(ui->miniMode->mapToGlobal(
        QPoint(QPoint(-miniModeWidget->width() + ui->miniMode->width(), 30))));
    ui->smartMode->setEnabled(false);
    ui->line->hide();
    this->hide();
    miniModeWidget->setMaximumHeight(miniModeWidget->height());
    miniModeWidget->setWindowOpacity(
        qreal(settingsObj.value("miniModeTransperancy", "98").toReal() / 100));
    miniModeWidget->setStyleSheet(
        ui->left_panel->styleSheet().replace("#left_panel", "#miniModeWidget"));
    miniModeWidget->showNormal();
  } else {
    // restore
    miniModeWidget->hide();
    ui->smartMode->setEnabled(true);
    ui->line->show();
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

void MainWindow::on_tabWidget_currentChanged(int index) {
  Q_UNUSED(index);
  ui->tabWidget->resize(ui->tabWidget->size().width() - 1,
                        ui->tabWidget->size().height());
  ui->tabWidget->resize(ui->tabWidget->size().width() + 1,
                        ui->tabWidget->size().height());
  settingsObj.setValue("currentQueueTab", index);
}

void MainWindow::on_close_clicked() { this->close(); }

void MainWindow::on_minimize_clicked() {
  // setWindowState(Qt::WindowMinimized);
  this->showMinimized();
}

void MainWindow::on_maximize_clicked() {
  if (windowState() == Qt::WindowMaximized) {
    setWindowState(Qt::WindowNoState);
  } else {
    setWindowState(Qt::WindowMaximized);
  }
}

void MainWindow::on_fullScreen_clicked() {
  setWindowState(windowState() ^ Qt::WindowFullScreen);
}

void MainWindow::reloadREquested(QString dataType, QString query) {
  // if the function contains two arguments they are seperated by '<==>'
  // here on if we are checking that and passing the given dataType function two
  // arguments by splitting query else we do handle single arg function.
  QString arg1, arg2;
  if (query.contains("<==>")) {
    arg1 = query.split("<==>").first();
    arg2 = query.split("<==>").last();
    ui->webview->page()->mainFrame()->evaluateJavaScript(
        dataType + "(\"" + arg1 + "\",\"" + arg2 + "\")");
  } else {
    if (dataType == "track_search") {
      ui->webview->page()->mainFrame()->evaluateJavaScript(
          "$('#tracks_result').html('')");
    }
    ui->webview->page()->mainFrame()->evaluateJavaScript(dataType + "(\"" +
                                                         query + "\")");
  }
}

// currently used by shuffle track fucntion only
void MainWindow::getEnabledTracks(QListWidget *currentListWidget) {

  if (settingsObj.value("shuffle", false).toBool()) {
    shuffledPlayerQueue.clear();
    for (int i = 0; i < currentListWidget->count(); i++) {
      if (currentListWidget->itemWidget(currentListWidget->item(i))
              ->isEnabled()) {
        QString songId =
            currentListWidget->itemWidget(currentListWidget->item(i))
                ->findChild<QLineEdit *>("songId")
                ->text()
                .trimmed();
        if (songId !=
            nowPlayingSongIdWatcher
                ->getValue()) // prevent adding nowPlaying songId to shuffled
                              // list so it wont repeat song immediatly
          shuffledPlayerQueue.append(songId);
      }
    }
  }
}

void MainWindow::assignNextTrack(QListWidget *list, int index) {

  if (list == nullptr) // prevent if list is not valid pointer
    return;

  // shuffled track assignment
  // shuffle is disabled in smartMode bcoz songs are already coming shuffled
  if (settingsObj.value("shuffle", false).toBool() && !smartMode) {
    getEnabledTracks(list);
    if (!shuffledPlayerQueue.isEmpty()) {
      QString shuffledSongId;
      int shuffledIndex = qrand() % ((shuffledPlayerQueue.count() + 1) - 1) + 0;

      shuffledSongId = shuffledPlayerQueue.at(shuffledIndex);

      if (!shuffledSongId.isEmpty()) {
        ui->next->setToolTip(
            htmlToPlainText(store_manager->getTrack(shuffledSongId).at(1)));
      } else {
        ui->next->setToolTip("");
      }
      ui->next->disconnect();
      connect(ui->next, &QPushButton::clicked, [=]() {
        list->setCurrentRow(shuffledIndex);
        listItemDoubleClicked(list, list->item(shuffledIndex));
        list->scrollToItem(list->item(shuffledIndex));
      });
      return;
    }
  }

  // normal next track algo
  if (list->itemWidget(list->item(index)) ==
      nullptr) // if itemwidget is not valid prevent assingment
    return;
  QString songId = list->itemWidget(list->item(index))
                       ->findChild<QLineEdit *>("songId")
                       ->text()
                       .trimmed();
  if (!songId.isEmpty()) {
    QString tooltip = store_manager->getTrack(songId).at(1);
    if (!tooltip.isEmpty()) {
      ui->next->setToolTip(htmlToPlainText(tooltip));
      if (sender() != nullptr &&
          sender()->objectName().contains("optionButton")) {
        notify("", "Next: " + tooltip);
      }
      ui->next->setEnabled(true);
    } else {
      // dig title of track
      ElidedLabel *title = list->itemWidget(list->item(index))
                               ->findChild<ElidedLabel *>("title_elided");
      QString titleStr = static_cast<ElidedLabel *>(title)->text();
      if (!titleStr.isEmpty()) {
        ui->next->setToolTip(htmlToPlainText(titleStr));
        if (sender() != nullptr &&
            sender()->objectName().contains("optionButton")) {
          notify("", "Next: " + tooltip);
        }
        ui->next->setEnabled(true);
      } else {
        ui->next->setToolTip("");
        ui->next->setEnabled(false);
      }
    }
  } else {
    ui->next->setToolTip("");
    ui->next->setEnabled(false);
  }
  ui->next->disconnect();
  connect(ui->next, &QPushButton::clicked, [=]() {
    list->setCurrentRow(index);
    listItemDoubleClicked(list, list->item(index));
    list->scrollToItem(list->item(index));
  });
}

void MainWindow::startGetRecommendedTrackForAutoPlayTimer(QString songId) {
  QTimer *timer = new QTimer(this);
  timer->setInterval(2000);
  timer->setSingleShot(true);
  connect(timer, &QTimer::timeout, [=]() {
    getRecommendedTracksForAutoPlay(songId);
    timer->deleteLater();
  });
  timer->start();
}

// call this anytime to load similar tracks of any song in smart-playlist
void MainWindow::getRecommendedTracksForAutoPlayHelper(QString videoId,
                                                       QString songId) {
  if (videoId.isEmpty()) {
    QStringList trackmeta = store_manager->getTrack(songId);
    QString title = trackmeta.at(1);
    QString artist = trackmeta.at(5);
    QString query =
        title.replace("N/A", "") + " - " + artist.replace("N/A", "");
    prepareTrack(songId, query, "0",
                 this->findChild<QListWidget *>(getCurrentPlayerQueue(songId)));
  }
  qDebug() << "SONG RADIO for video:" << videoId << "songId:" << songId;
  currentSimilarTrackProcessing = 0;
  // keep now playing song and remove others
  while (similarTracksListHasTrackToBeRemoved()) {
    for (int i = 0; i < ui->smart_list->count(); i++) {
      if (ui->smart_list->itemWidget(ui->smart_list->item(i))
              ->findChild<QLabel *>("playing")
              ->toolTip() != "playing...") {
        QListWidgetItem *item = ui->smart_list->takeItem(i);
        if (item != nullptr) {
          delete item;
        }
      }
      ui->previous->setEnabled(false);
    }
  }

  if (similarTracks->isLoadingPLaylist && !similarTracks->isLoadingLocalSongs) {
    similarTracks->getNextTracksInPlaylist(currentSimilarTrackList);
  } else if (similarTracks->isLoadingPLaylist &&
             similarTracks->isLoadingLocalSongs) {
    similarTracks->pullMoreLocalTracks();
  } else {
    qDebug() << "addSimilarTracks";
    similarTracks->addSimilarTracks(videoId, songId);
  }
}

void MainWindow::getRecommendedTracksForAutoPlay(QString songId) {
  QString videoId =
      store_manager->getYoutubeIds(songId).split("<br>").first().trimmed();
  if (videoId.trimmed().isEmpty()) {
    QWidget *listWidgetItem =
        ui->smart_list->findChild<QWidget *>("track-widget-" + songId);
    if (listWidgetItem != nullptr) {
      QString id = listWidgetItem->findChild<QLineEdit *>("id")->text();
      videoId = id.split("<br>").first();
    }
  }
  qDebug() << ui->repeat->checkState();
  if (ui->repeat->checkState() == Qt::PartiallyChecked) {
    notify("", ":force: Will repeat current song..");
  }
  if (ui->repeat->checkState() == Qt::Checked) {
    notify("", ":force: Will repeat current queue..");
  }

  // load tracks in smart playlist if smart mode is enabled
  // prevent getting related songs in case if the track is repeating
  if (smartMode && ui->repeat->checkState() != Qt::PartiallyChecked) {
    if (ui->repeat->checkState() == Qt::Checked) {
      // repeat queue
      if (!ui->next->isEnabled()) {
        assignNextTrack(this->findChild<QListWidget *>(getCurrentPlayerQueue(
                            nowPlayingSongIdWatcher->getValue())),
                        0);
      } else {
        nextPreviousHelper(this->findChild<QListWidget *>(
            getCurrentPlayerQueue(nowPlayingSongIdWatcher->getValue())));
      }
      return;
    }

    qDebug() << "Prepare related songs list for videoId" << videoId << songId;
    getRecommendedTracksForAutoPlayHelper(videoId, songId);
    return;
  }

  if (ui->repeat->checkState() != Qt::Checked &&
      ui->repeat->checkState() != Qt::PartiallyChecked) {
    // load smart playlist if song is last
    if (!ui->next->isEnabled()) {
      qDebug() << "Prepare related songs list for videoId" << videoId << songId;
      getRecommendedTracksForAutoPlayHelper(videoId, songId);
    }
  } else if (ui->repeat->checkState() == Qt::Checked) {
    // repeat queue
    if (!ui->next->isEnabled()) {
      assignNextTrack(this->findChild<QListWidget *>(getCurrentPlayerQueue(
                          nowPlayingSongIdWatcher->getValue())),
                      0);
    } else {
      nextPreviousHelper(this->findChild<QListWidget *>(
          getCurrentPlayerQueue(nowPlayingSongIdWatcher->getValue())));
    }
  }
}

bool MainWindow::similarTracksListHasTrackToBeRemoved() {
  bool has = false;
  for (int i = 0; i < ui->smart_list->count(); i++) {
    if (ui->smart_list->itemWidget(ui->smart_list->item(i))
            ->findChild<QLabel *>("playing")
            ->toolTip() != "playing...") {
      has = true;
    }
    if (has)
      break;
  }
  return has;
}

void MainWindow::assignPreviousTrack(QListWidget *list, int index) {
  // shuffled track assignment
  // shuffle is disabled in smartMode bcoz songs are already coming shuffled
  if (settingsObj.value("shuffle").toBool() && !smartMode) {
    getEnabledTracks(list);
    if (!shuffledPlayerQueue.isEmpty()) {
      QString shuffledSongId;
      int shuffledIndex = qrand() % ((shuffledPlayerQueue.count() + 1) - 1) + 0;

      shuffledSongId = shuffledPlayerQueue.at(shuffledIndex);

      if (!shuffledSongId.isEmpty()) {
        ui->previous->setToolTip(
            htmlToPlainText(store_manager->getTrack(shuffledSongId).at(1)));
      } else {
        ui->previous->setToolTip("");
      }
      ui->previous->disconnect();
      connect(ui->previous, &QPushButton::clicked, [=]() {
        list->setCurrentRow(shuffledIndex);
        listItemDoubleClicked(list, list->item(shuffledIndex));
        list->scrollToItem(list->item(shuffledIndex));
      });
      return;
    }
  }

  // normal previous track algo
  QString songId = list->itemWidget(list->item(index))
                       ->findChild<QLineEdit *>("songId")
                       ->text()
                       .trimmed();
  if (!songId.isEmpty()) {
    QString tooltip = store_manager->getTrack(songId).at(1);
    if (!tooltip.isEmpty()) {
      ui->previous->setToolTip(htmlToPlainText(tooltip));
    } else {
      // dig title of track
      ElidedLabel *title = list->itemWidget(list->item(index))
                               ->findChild<ElidedLabel *>("title_elided");
      QString titleStr = static_cast<ElidedLabel *>(title)->text();
      if (!titleStr.isEmpty())
        ui->previous->setToolTip(htmlToPlainText(titleStr));
      else
        ui->previous->setToolTip("");
    }
  } else {
    ui->previous->setToolTip("");
  }
  ui->previous->disconnect();
  connect(ui->previous, &QPushButton::clicked, [=]() {
    list->setCurrentRow(index);
    listItemDoubleClicked(list, list->item(index));
    list->scrollToItem(list->item(index));
  });
}

void MainWindow::on_ytdlStopAll_clicked() {
  if (ytdlProcess != nullptr) {
    ytdlQueue.clear();
    ytdlProcess->close();
    ytdlProcess = nullptr;
  }
  if (ytdlProcess == nullptr) {
    ui->ytdlStopAll->setEnabled(false);
    ui->ytdlRefreshAll->setEnabled(true);
  }
}

void MainWindow::on_ytdlRefreshAll_clicked() {
  // process Olivia queue
  for (int i = 0; i < ui->olivia_list->count(); i++) {
    // get disabled items
    if (ui->olivia_list->itemWidget(ui->olivia_list->item(i))->isEnabled() ==
        false) {
      // get id and songId
      QString id, songId;
      id = ui->olivia_list->itemWidget(ui->olivia_list->item(i))
               ->findChild<QLineEdit *>("id")
               ->text();
      songId = ui->olivia_list->itemWidget(ui->olivia_list->item(i))
                   ->findChild<QLineEdit *>("songId")
                   ->text();
      // check if ytids are present
      if (!id.isEmpty()) {
        // add to ytdlProcessQueue
        getAudioStream(id, songId);
      }
    }
  }
  // process Youtube queue
  for (int i = 0; i < ui->youtube_list->count(); i++) {
    // get disabled items
    if (ui->youtube_list->itemWidget(ui->youtube_list->item(i))->isEnabled() ==
        false) {
      // get id and songId
      QString id, songId;
      id = ui->youtube_list->itemWidget(ui->youtube_list->item(i))
               ->findChild<QLineEdit *>("id")
               ->text();
      songId = ui->youtube_list->itemWidget(ui->youtube_list->item(i))
                   ->findChild<QLineEdit *>("songId")
                   ->text();
      // check if ytids are present
      if (!id.isEmpty()) {
        // add to ytdlProcessQueue
        getAudioStream(id, songId);
      }
    }
  }
}

//=========================================Track item click
// handler==========================================
void MainWindow::on_youtube_list_itemClicked(QListWidgetItem *item) {
  trackItemClicked(ui->youtube_list, item);
}

void MainWindow::on_olivia_list_itemClicked(QListWidgetItem *item) {
  trackItemClicked(ui->olivia_list, item);
}

void MainWindow::on_smart_list_itemClicked(QListWidgetItem *item) {
  trackItemClicked(ui->smart_list, item);
}

void MainWindow::trackItemClicked(QListWidget *listWidget,
                                  QListWidgetItem *item) {
  // check if track is not enabled
  if (listWidget->itemWidget(item)->isEnabled() == false) {
    QAction *updateTrack = new QAction("Refresh Track", nullptr);
    QAction *getYtIds = new QAction("Search on Youtube", nullptr);
    QAction *removeTrack =
        new QAction("Search on Youtube && Remove track", nullptr);
    QAction *removeTrack2 = new QAction("Remove track", nullptr);

    // set Icons
    updateTrack->setIcon(QIcon(":/icons/sidebar/refresh.png"));
    getYtIds->setIcon(QIcon(":/icons/sidebar/search.png"));
    removeTrack->setIcon(QIcon(":/icons/sidebar/remove.png"));
    removeTrack2->setIcon(QIcon(":/icons/sidebar/remove.png"));

    // not enabled, decide menu option to popup
    QString ytIds = listWidget->itemWidget(item)
                        ->findChild<QLineEdit *>("id")
                        ->text()
                        .trimmed();
    QString songId = listWidget->itemWidget(item)
                         ->findChild<QLineEdit *>("songId")
                         ->text()
                         .trimmed();

    connect(removeTrack, &QAction::triggered, [=]() {
      // youtube search
      QString songTitle = listWidget->itemWidget(item)
                              ->findChild<ElidedLabel *>("title_elided")
                              ->text();
      QString songArtist = listWidget->itemWidget(item)
                               ->findChild<ElidedLabel *>("artist_elided")
                               ->text();
      QString term = songTitle + " - " + songArtist;
      if (pageType == "youtube") {
        youtubeSearchTerm = term;
        ui->webview->page()->mainFrame()->evaluateJavaScript(
            "$('.ui-content').fadeOut('fast');$('#manual_search').val('" +
            youtubeSearchTerm + "');manual_youtube_search('" +
            youtubeSearchTerm + "');");
        youtubeSearchTerm.clear();
      } else {
        setSearchTermAndOpenYoutube(QVariant(term));
      }
      // remove
      listWidget->removeItemWidget(item);
      delete item;
      store_manager->removeFromQueue(songId);
    });

    connect(removeTrack2, &QAction::triggered, [=]() {
      listWidget->removeItemWidget(item);
      delete item;
      store_manager->removeFromQueue(songId);
    });

    // if no ytIds set for track
    if (ytIds.isEmpty()) {
      updateTrack->setEnabled(false);
      getYtIds->setEnabled(true);
      connect(getYtIds, &QAction::triggered, [=]() {
        // open youtube page and find track using track's metadata
        QString songTitle = listWidget->itemWidget(item)
                                ->findChild<ElidedLabel *>("title_elided")
                                ->text();
        QString songArtist = listWidget->itemWidget(item)
                                 ->findChild<ElidedLabel *>("artist_elided")
                                 ->text();
        QString term = songTitle + " - " + songArtist;
        if (pageType == "youtube") {
          youtubeSearchTerm = term;
          ui->webview->page()->mainFrame()->evaluateJavaScript(
              "$('.ui-content').fadeOut('fast');$('#manual_search').val('" +
              youtubeSearchTerm + "');manual_youtube_search('" +
              youtubeSearchTerm + "');");
          youtubeSearchTerm.clear();
        } else {
          setSearchTermAndOpenYoutube(QVariant(term));
        }
      });
    } else {
      getYtIds->setEnabled(false);
    }
    // if ytdlProcess is not running
    if (ytdlProcess == nullptr && !ytIds.isEmpty()) {
      updateTrack->setEnabled(true);
      connect(updateTrack, &QAction::triggered,
              [=]() { getAudioStream(ytIds, songId); });
    }

    // show menu
    QMenu menu;
    menu.addAction(updateTrack);
    menu.addAction(getYtIds);
    menu.addSeparator();
    menu.addAction(removeTrack);
    menu.addAction(removeTrack2);
    menu.setStyleSheet(menuStyle());
    menu.exec(QCursor::pos());
  }
}
//=========END Track item click handler=========

// returns Qmenu stylesheet according to current theme
QString MainWindow::menuStyle() {
  // set theme for menu
  QStringList rgba = themeColor.split(",");
  QString r, g, b, a;
  r = rgba.at(0);
  g = rgba.at(1);
  b = rgba.at(2);
  a = rgba.at(3);
  QString menuStyle("QMenu:icon{"
                    "padding-left:4px;"
                    "}"
                    "QMenu::item{"
                    "background-color:rgba(" +
                    r + "," + g + "," + b + "," + a +
                    ");"
                    "color: rgb(255, 255, 255);"
                    "}"

                    "QMenu::item:selected{"
                    "border:none;"
                    "background-color:"
                    "qlineargradient(spread:pad, x1:0, y1:0, x2:0, y2:1,"
                    "stop:0.129213 rgba(" +
                    r + ", " + g + ", " + b +
                    ", 35),"
                    "stop:0.38764 rgba(" +
                    r + ", " + g + ", " + b +
                    ", 120),"
                    "stop:0.679775 rgba(" +
                    r + ", " + g + ", " + b +
                    ", 110),"
                    "stop:1 rgba(" +
                    r + ", " + g + ", " + b +
                    ", 35));"
                    "color: rgb(255, 255, 255);"
                    "}"

                    "QMenu::item:disabled{"
                    "background-color:rgba(" +
                    r + "," + g + "," + b +
                    ",0.1);"
                    "color: grey;"
                    "}");
  return menuStyle;
}

// create and show queue option
void MainWindow::on_olivia_queue_options_clicked() {
  queueShowOption(ui->olivia_list);
}

void MainWindow::on_youtube_queue_options_clicked() {
  queueShowOption(ui->youtube_list);
}

void MainWindow::queueShowOption(QListWidget *queue) {

  QAction *clearQueue = new QAction(QIcon(":/icons/sidebar/remove.png"),
                                    "Clear this queue", nullptr);
  QAction *clearUnCached = new QAction(QIcon(":/icons/sidebar/remove.png"),
                                       "Clear un-cached songs", nullptr);
  QAction *refreshDeadTracks = new QAction(QIcon(":/icons/sidebar/refresh.png"),
                                           "Refresh dead tracks", nullptr);

  // connect
  if (queue->count() > 0) {

    clearQueue->setEnabled(true);
    refreshDeadTracks->setEnabled(hasDeadTracks(queue));
    clearUnCached->setEnabled(hasUnCachedTracks(queue));

    connect(clearQueue, &QAction::triggered, [=]() {
      for (int i = 0; i < queue->count(); i++) {
        QString songIdFromWidget =
            static_cast<QLineEdit *>(queue->itemWidget(queue->item(i))
                                         ->findChild<QLineEdit *>("songId"))
                ->text()
                .trimmed();
        store_manager->removeFromQueue(songIdFromWidget);
      }
      queue->clear();
    });
    connect(clearUnCached, &QAction::triggered, [=]() {
      while (hasUnCachedTracks(queue)) {
        for (int i = 0; i < queue->count(); i++) {
          QString songIdFromWidget =
              static_cast<QLineEdit *>(queue->itemWidget(queue->item(i))
                                           ->findChild<QLineEdit *>("songId"))
                  ->text()
                  .trimmed();
          songIdFromWidget = songIdFromWidget.remove("<br>").trimmed();
          if(!store_manager->isDownloaded(songIdFromWidget) /*&& !trackIsBeingProcessed(songIdFromWidget)*/){
            store_manager->removeFromQueue(songIdFromWidget);
            // queue->removeItemWidget(queue->item(i));
            QListWidgetItem *item = queue->takeItem(i);
            if (item != nullptr) {
              delete item;
            }
          }
        }
      }
    });

    connect(refreshDeadTracks, &QAction::triggered, [=]() {
      for (int i = 0; i < queue->count(); i++) {
        QString songIdFromWidget =
            static_cast<QLineEdit *>(queue->itemWidget(queue->item(i))
                                         ->findChild<QLineEdit *>("songId"))
                ->text()
                .trimmed();
        if (!queue->itemWidget(queue->item(i))->isEnabled() &&
            !store_manager->isDownloaded(songIdFromWidget)) {
          getAudioStream(store_manager->getYoutubeIds(songIdFromWidget),
                         songIdFromWidget);
        }
      }
    });
  } else {
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

bool MainWindow::hasDeadTracks(QListWidget *queue) {
  bool has = false;
  for (int i = 0; i < queue->count(); i++) {
    QString songIdFromWidget =
        static_cast<QLineEdit *>(
            queue->itemWidget(queue->item(i))->findChild<QLineEdit *>("songId"))
            ->text()
            .trimmed();
    if (store_manager->getExpiry(songIdFromWidget) &&
        !store_manager->isDownloaded(songIdFromWidget)) {
      has = true;
    }
    if (has)
      break;
  }
  return has;
}

bool MainWindow::hasUnCachedTracks(QListWidget *queue) {
  bool has = false;
  for (int i = 0; i < queue->count(); i++) {
    QString songIdFromWidget =
        static_cast<QLineEdit *>(
            queue->itemWidget(queue->item(i))->findChild<QLineEdit *>("songId"))
            ->text()
            .trimmed();
    if (!store_manager->isDownloaded(songIdFromWidget)) {
      has = true;
    }
    if (has)
      break;
  }
  return has;
}

// unused right now
bool MainWindow::trackIsBeingProcessed(QString songId) {
  bool isProcessing = false;
  QList<QProcess *> ytDlProcessList;
  ytDlProcessList = this->findChildren<QProcess *>();
  foreach (QProcess *process, ytDlProcessList) {
    if (process->objectName().trimmed() == songId) {
      isProcessing = true;
    }
    if (isProcessing)
      break;
  }
  return isProcessing;
}

void MainWindow::clear_queue() {
  ui->stop->click();
  ui->olivia_list->clear();
  ui->youtube_list->clear();
}

// change app transparency
void MainWindow::transparency_changed(int value) {
  setWindowOpacity(qreal(value) / 100);
}

//================================Equalizer=========================================================
void MainWindow::on_eq_clicked() {
  QString btn_style =
      "QPushButton{color: silver; background-color: #45443F; border:1px solid "
      "#272727; padding-top: 3px; padding-bottom: 3px; padding-left: 3px; "
      "padding-right: 3px; border-radius: 2px; outline: none;}"
      "QPushButton:disabled { background-color: #45443F; border:1px solid "
      "#272727; padding-top: 3px; padding-bottom: 3px; padding-left: 5px; "
      "padding-right: 5px; /*border-radius: 2px;*/ color: #636363;}"
      "QPushButton:hover{border: 1px solid #272727;background-color:#5A584F; "
      "color:silver ;}"
      "QPushButton:pressed {background-color: #45443F;color: "
      "silver;padding-bottom:1px;}";

  eq->setStyleSheet("");
  eq->setStyleSheet("QWidget#equalizer{" + ui->search->styleSheet() + "}" +
                    "QFrame{" + ui->search->styleSheet() + "}" + btn_style);
  eq->removeStyle();
  eq->show();
}

// called on every radio restart while eq class is initialzed once
void MainWindow::init_eq() {

  if (eq == nullptr) {
    eq = new equalizer(this);
    eq->setWindowTitle(QApplication::applicationName() + " - Equalizers");
    eq->setWindowFlags(Qt::Dialog);
    eq->setWindowModality(Qt::NonModal);
  }

  connect(eq, SIGNAL(update_eq(QString)), this, SLOT(set_eq(QString)));
  connect(eq, SIGNAL(disable_eq()), this, SLOT(disable_eq()));

  ui->eq->setIcon(eq->eq_enabled() ? QIcon(":/icons/eq_button.png")
                                   : QIcon(":/icons/eq_button_disabled.png"));

  eq->setRange();
  eq->loadSettings();
  QTimer::singleShot(500, eq, SLOT(triggerEq()));
}

void MainWindow::set_eq(QString eq_args) {
  if (!eq_args.isEmpty()) {
    QProcess *fifo = new QProcess(this);
    connect(fifo, SIGNAL(finished(int)), this, SLOT(deleteProcess(int)));
    fifo->start("bash",
                QStringList()
                    << "-c"
                    << "echo '{\"command\": [\"set_property\" ,\"af\",\"" +
                           eq_args + "\"]}' | socat - " +
                           radio_manager->used_fifo_file_path);
    processIdList.append(fifo->processId());
    ui->eq->setIcon(QIcon(":/icons/eq_button.png"));
    ui->eq->setToolTip("Equalizer (Enabled)");
  }
}

void MainWindow::disable_eq() {
  QProcess *fifo = new QProcess(this);
  connect(fifo, SIGNAL(finished(int)), this, SLOT(deleteProcess(int)));
  fifo->start("bash", QStringList() << "-c"
                                    << "echo '{\"command\": [\"af\",\"set\",\""
                                       "\"]}' | socat - " +
                                           radio_manager->used_fifo_file_path);
  processIdList.append(fifo->processId());
  ui->eq->setIcon(QIcon(":/icons/eq_button_disabled.png"));
  ui->eq->setToolTip("Equalizer (Disabled)");
}

void MainWindow::deleteProcess(int code) {
  QProcess *process = qobject_cast<QProcess *>(sender());
  if (code == 0) {
    process->deleteLater();
  }
}

void MainWindow::radioProcessReady() {
  if (eq == nullptr) {
    init_eq();
  } else {
    eq->setRange();
    eq->loadSettings();
    QTimer::singleShot(500, eq, SLOT(triggerEq()));
  }
}
//================================Equalizer=========================================================

//================================Shuffle===========================================================
void MainWindow::on_shuffle_toggled(bool checked) {
  settingsObj.setValue("shuffle", checked);
  if (checked) {
    // disable repeat current song if shuffle is on
    notify("", "Repeat current song is not allowed while shuffle");
    if (ui->repeat->checkState() == Qt::PartiallyChecked) {
      ui->repeat->setCheckState(Qt::Unchecked);
      notify("", "Shuffle: On, Repeat: Off");
    } else {
      notify("", "Shuffle: On");
    }
    ui->shuffle->setIcon(QIcon(":/icons/shuffle_button.png"));
    ui->shuffle->setToolTip("Shuffle (Enabled)");
  } else {
    notify("", "Shuffle: Off");
    ui->shuffle->setIcon(QIcon(":/icons/shuffle_button_disabled.png"));
    ui->shuffle->setToolTip("Shuffle (Disabled)");
  }
}
//================================Shuffle===========================================================

void MainWindow::on_hideDebug_clicked() { ui->debug_widget->hide(); }

void MainWindow::init_downloadWidget() {
  downloadWidget = new Widget(this);
  downloadWidget->setWindowFlags(Qt::Widget);
  downloadWidget->downloadLocation = setting_path + "/downloadedVideos";
  downloadWidget->downloadLocationAudio = setting_path + "/downloadedTracks/";
  ui->downloadWidgetVLayout->addWidget(downloadWidget);
  connect(qApp, SIGNAL(aboutToQuit()), downloadWidget,
          SLOT(on_pauseAll_clicked()));
  connect(downloadWidget, &Widget::updateTrack,
          [=](QString trackId, QString downloadLocation) {
            this->updateTrack(trackId, downloadLocation);
          });
}

void MainWindow::videoOptionDownloadRequested(
    QStringList trackMetaData,
    QStringList formats) { // videoformat<<audioformat
  ui->tabWidget->setCurrentIndex(2);
  QString ytIds, title, artist, album, coverUrl, songId;
  songId = trackMetaData.at(0);
  title = trackMetaData.at(1);
  album = trackMetaData.at(2);
  artist = trackMetaData.at(3);
  coverUrl = trackMetaData.at(4);
  ytIds = trackMetaData.at(5);

  downloadWidget->trackId = songId;

  // remove html notations from title

  QString plainTitle = htmlToPlainText(title);
  downloadWidget->trackTitle = plainTitle + " [Video]";

  QString url_str =
      "https://www.youtube.com/watch?v=" + ytIds.split("<br>").first();
  downloadWidget->startWget(url_str, downloadWidget->downloadLocation, formats,
                            "video+audio");
}

// switcher for menu listwidget used to change row without calling corresponding
// function
void MainWindow::leftListChangeCurrentRow(int row) {
  ui->left_list->blockSignals(true);
  ui->left_list->setCurrentRow(row);
  ui->left_list->blockSignals(false);
}

// INVOKABLE play video function called from webend
void MainWindow::playVideo(QString trackId) {
  QDir dir(setting_path + "/downloadedVideos/");
  QStringList filter;
  filter << trackId + "*";
  QFileInfoList files = dir.entryInfoList(filter);
  if (store_manager != nullptr) {
    QString titleLabel = store_manager->getTrack(trackId).at(1);
    QProcess *player = new QProcess(this);
    player->setObjectName("player");
    player->start("mpv",
                  QStringList()
                      << "--geometry=50%:50%"
                      << "--autofit=70%"
                      << "--title=MPV for Olivia - " + titleLabel << "--no-ytdl"
                      << files.at(0).filePath()
                      << "--volume=" + QString::number(radio_manager->volume));
    if (this->radio_manager->radioState == "playing") {
      this->radio_manager->pauseRadio();
    }
  }
}

// jump to queue of nowPlaying track scroll to it and select it
void MainWindow::on_jump_to_nowplaying_clicked() {
  // clear any filter if applied to any listviews and load all tracks to
  // listviews
  ui->filter_olivia->clear();
  ui->filter_youtube->clear();

  // identify the player queue name
  QString listName = getCurrentPlayerQueue(nowPlayingSongIdWatcher->getValue());
  QListWidget *listWidget = this->findChild<QListWidget *>(listName);
  QWidget *listWidgetItem = listWidget->findChild<QWidget *>(
      "track-widget-" + nowPlayingSongIdWatcher->getValue());

  listClearSelection(listName);

  // check if listWidgetItem found in one of list
  if (listWidgetItem == nullptr) {
    qDebug() << "CANNOT JUMP, TRACK NOT FOUND IN LIST";
    listName = "";
    return;
  } else {
    // switch tab where track is found
    if (listName == "olivia_list")
      ui->tabWidget->setCurrentWidget(ui->tab);
    if (listName == "youtube_list")
      ui->tabWidget->setCurrentWidget(ui->tab_2);
    if (listName == "smart_list" && !ui->recommWidget->isVisible())
      ui->show_hide_smart_list_button->click();

    // select current track
    for (int i = 0; i < listWidget->count(); i++) {
      QString songIdFromWidget =
          static_cast<QLineEdit *>(listWidget->itemWidget(listWidget->item(i))
                                       ->findChild<QLineEdit *>("songId"))
              ->text()
              .trimmed();
      if (songIdFromWidget.contains(nowPlayingSongIdWatcher->getValue())) {
        listWidget->scrollToItem(listWidget->item(i));
        listWidget->setCurrentRow(i);
      }
    }
  }
}

// clears the selection player queues
void MainWindow::listClearSelection(QString listName) {
  if (listName != ("olivia_list")) {
    ui->olivia_list->clearSelection();
  }
  if (listName != ("youtube_list")) {
    ui->youtube_list->clearSelection();
  }
  if (listName != ("smart_list")) {
    ui->smart_list->clearSelection();
  }
}

// show and hide smart list button clicked method
void MainWindow::on_show_hide_smart_list_button_clicked() {
  QSplitter *split3 = this->findChild<QSplitter *>("split3");
  if (!ui->recommWidget->isVisible()) {
    if (smartMode) {
      split3->widget(1)->setMaximumHeight(16777215);
      split3->setSizes(QList<int>() << 500 << 500);
      split3->handle(1)->setEnabled(true);
      ui->recommWidget->show();
      ui->jump_to_nowplaying->click();
    } else {
      split3->widget(1)->setMaximumHeight(16777215);
      split3->setSizes(QList<int>() << 300 << 300);
      split3->handle(1)->setEnabled(true);
      ui->recommWidget->show();
      ui->jump_to_nowplaying->click();
    }

    ui->show_hide_smart_list_button->setToolTip("Hide");
  } else {
    split3->widget(1)->setMaximumHeight(
        ui->show_hide_smart_list_button->height());
    split3->setSizes(QList<int>() << 300 << 0);
    split3->handle(1)->setEnabled(false);
    ui->recommWidget->hide();
    ui->show_hide_smart_list_button->setToolTip("Show");
  }
  ui->recommWidget->isVisible()
      ? ui->show_hide_smart_list_button->setIcon(
            QIcon(":/icons/sidebar/hideRecommend.png"))
      : ui->show_hide_smart_list_button->setIcon(
            QIcon(":/icons/sidebar/showRecommend.png"));
}

// add playlist from web interface
void MainWindow::addPlaylistByData(QString data) {
  similarTracks->addPlaylist(data);
}

void MainWindow::on_playlistLoaderButtton_clicked() {
  QVariant ulCount = ui->webview->page()->mainFrame()->evaluateJavaScript(
      "$.mobile.activePage.find('ul').length");
  if (ulCount.isValid()) {
    int totalUl = ulCount.toInt();
    // find ul which contains gettrackinfo
    for (int i = 0; i < totalUl; i++) {
      QVariant validList = ui->webview->page()->mainFrame()->evaluateJavaScript(
          "$($.mobile.activePage.find('ul')[" + QString::number(i) +
          "]).html().includes('gettrackinfo')");
      if (validList.isValid() && validList.toBool() == true) {
        ui->webview->page()->mainFrame()->evaluateJavaScript(
            "mainwindow.addPlaylistByData($($.mobile.activePage.find('ul')[" +
            QString::number(i) + "]).html())");
        break;
      }
    }
  }
  if (getCurrentPlayerQueue(nowPlayingSongIdWatcher->getValue()) ==
      "smart_list") {
    ui->next->setEnabled(false);
  }
}

// returns the player queue where the player is playing track
QString MainWindow::getCurrentPlayerQueue(QString songId) {
  QString listName;
  QList<QWidget *> listWidgetItems =
      ui->right_panel->findChildren<QWidget *>("track-widget-" + songId);
  // if listwidgetItems not found in right_panel. this happens when player is
  // witched to smart mode;
  if (listWidgetItems.count() == 0) {
    listWidgetItems =
        ui->recommHolder->findChildren<QWidget *>("track-widget-" + songId);
  }
  if (listWidgetItems.isEmpty()) {
    qDebug() << "PLAYER QUEUE NOT FOUND";
  } else {
    foreach (QWidget *listWidgetItem, listWidgetItems) {
      QListWidget *listWidget =
          static_cast<QListWidget *>(listWidgetItem->parent()->parent());
      if (listWidget != nullptr) {
        listName = listWidget->objectName();
      } else {
        qDebug() << "PLAYER QUEUE NOT FOUND";
      }
    }
  }
  return listName;
}
//======================track
// properties==================================================
void MainWindow::showTrackProperties(QString songId) {
  if (trackProperties == nullptr) {
    trackProperties = new TrackProperties(0, setting_path);
    connect(
        trackProperties, &TrackProperties::delete_song_cache,
        [=](QString trackId) { this->delete_song_cache(QVariant(trackId)); });
  }
  QStringList meta;
  meta = store_manager->getTrack(songId);
  trackProperties->setValues(meta);
  trackProperties->adjustSize();
  trackProperties->setFixedSize(340, trackProperties->height() +
                                         2); // 2px for progressbar
  trackProperties->show();
}

//======================start favourite
// button============================================ fake toggle programmatic
void MainWindow::setFavouriteButton(bool checked) {
  if (checked) {
    ui->favourite->setIcon(QIcon(":/icons/sidebar/liked_2.png"));
    ui->favourite->setToolTip("Remove from favourite");
    store_manager->add_to_liked(nowPlayingSongIdWatcher->getValue());
  } else {
    ui->favourite->setIcon(QIcon(":/icons/sidebar/liked_disabled.png"));
    ui->favourite->setToolTip("Add to Favourite");
  }
  ui->favourite->blockSignals(true);
  ui->favourite->setChecked(checked);
  ui->favourite->blockSignals(false);
}

// real toggle by user
void MainWindow::on_favourite_toggled(bool checked) {
  if (nowPlayingSongIdWatcher->isRadioStation == false) {
    if (checked) {
      ui->favourite->setIcon(QIcon(":/icons/sidebar/liked_2.png"));
      ui->favourite->setToolTip("Remove from favourite");
      store_manager->add_to_liked(nowPlayingSongIdWatcher->getValue());
    } else {
      ui->favourite->setIcon(QIcon(":/icons/sidebar/liked_disabled.png"));
      ui->favourite->setToolTip("Add to Favourite");
      removeFromFavourite(nowPlayingSongIdWatcher->getValue());
    }
    // reload liked tracks if page is liked_tracks
    if (pageType == "liked_songs") {
      ui->webview->page()->mainFrame()->evaluateJavaScript(
          "open_liked_tracks()");
    }
  } else {
    QMessageBox msgBox;
    msgBox.setText(
        "Add or remove radio station to favourite from Internet radio page.");
    msgBox.setIconPixmap(
        QPixmap(":/icons/sidebar/info.png")
            .scaled(42, 42, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.setDefaultButton(QMessageBox::Ok);
    msgBox.exec();
    // set fav to previous state
    ui->favourite->blockSignals(true);
    ui->favourite->setChecked(!checked);
    ui->favourite->blockSignals(false);
  }
}
// remove from favourite with msgbox asks user to remove song cache too
void MainWindow::removeFromFavourite(QString songId) {
  if (store_manager->isDownloaded(songId)) {
    QMessageBox msgBox;
    msgBox.setText("Keep track in Saved Music ?");
    msgBox.setIconPixmap(
        QPixmap(":/icons/sidebar/info.png")
            .scaled(42, 42, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    msgBox.setInformativeText("Do you also want to delete track's cache too ?");
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::Yes);
    int ret = msgBox.exec();
    switch (ret) {
    case QMessageBox::Yes:
      delete_song_cache(songId);
      break;
    case QMessageBox::No:
      msgBox.close();
      break;
    }
  }
  store_manager->remove_from_liked(songId);
}
//======================end favourite button===========

void MainWindow::on_smartMode_clicked() {
  if (!smartModeWidget->isVisible()) {
    smartMode = true;
    // disable next button to prevent/break normal next track assingment algo
    if (getCurrentPlayerQueue(nowPlayingSongIdWatcher->getValue()) !=
        "smart_list") {
      ui->next->disconnect();
      ui->next->setEnabled(false);
    }
    // CHECK THIS
    if (similarTracks->parentSongId != nowPlayingSongIdWatcher->getValue()) {
      ui->next->disconnect();
      ui->next->setEnabled(false);
    }
    smartModeShuffleState = ui->shuffle->isChecked();
    // disbale shuffle cause track are coming shuffled
    ui->shuffle->setChecked(false);
    ui->shuffle->hide();
    // populate smart playlist with currently playing track
    // check if related songs of songId are already loaded
    if (similarTracks->parentSongId == nowPlayingSongIdWatcher->getValue()) {
      similarTracks->previousParentSongId = nowPlayingSongIdWatcher->getValue();
    }
    if (similarTracks->previousParentSongId.isEmpty()) {
      similarTracks->parentSongId = nowPlayingSongIdWatcher->getValue();
      startGetRecommendedTrackForAutoPlayTimer(
          nowPlayingSongIdWatcher->getValue());
      // similarTracks->previousParentSongId =
      // nowPlayingSongIdWatcher->getValue();
    } else if (nowPlayingSongIdWatcher->getValue() !=
               similarTracks->previousParentSongId) {
      startGetRecommendedTrackForAutoPlayTimer(
          nowPlayingSongIdWatcher->getValue());
      similarTracks->previousParentSongId = nowPlayingSongIdWatcher->getValue();
    }
    if (!ui->recommWidget->isVisible()) {
      ui->show_hide_smart_list_button->click();
    }
    // disbale smart playlist show hide button
    ui->show_hide_smart_list_button->blockSignals(true);
    ui->smartMode->setIcon(QIcon(":/icons/restore_mini_mode.png"));
    ui->smartMode->setToolTip("Restore back to full mode");
    smartMode_ui.nowPlayingLayout->addWidget(ui->nowplaying_widget);
    smartMode_ui.seekSliderLyout->addWidget(ui->position);
    smartMode_ui.seekSliderLyout->addWidget(ui->radioSeekSlider);
    smartMode_ui.seekSliderLyout->addWidget(ui->duration);
    ui->radioVolumeSlider->setMaximumWidth(16777215);
    smartMode_ui.volumeSliderLayout->addWidget(ui->radioVolumeSlider);
    smartMode_ui.playlistLayout->addWidget(ui->recommHolder);
    smartMode_ui.controlLayout->addWidget(ui->controls_widget);
    smartModeWidget->move(ui->smartMode->mapToGlobal(QPoint(
        QPoint(-smartModeWidget->width() + ui->smartMode->width(), 30))));
    ui->miniMode->setEnabled(false);
    ui->line->hide();
    this->hide();
    smartModeWidget->setWindowOpacity(
        qreal(settingsObj.value("miniModeTransperancy", "98").toReal() / 100));
    smartModeWidget->setStyleSheet(ui->left_panel->styleSheet().replace(
        "#left_panel", "#smartModeWidget"));
    smartModeWidget->resize(smartModeWidget->width(), 500);
    smartModeWidget->showNormal();
  } else {
    smartMode = false;
    nextPreviousHelper(this->findChild<QListWidget *>(
        getCurrentPlayerQueue(nowPlayingSongIdWatcher->getValue())));
    // show shuffle button and restore its state
    ui->shuffle->show();
    ui->shuffle->setChecked(smartModeShuffleState);
    // reEnable smart playlist show hide button
    ui->show_hide_smart_list_button->blockSignals(false);
    // restore
    smartModeWidget->hide();
    ui->miniMode->setEnabled(true);
    ui->line->show();
    ui->smartMode->setToolTip("Switch to Smart Mode");
    ui->radioVolumeSlider->setMaximumWidth(200);
    ui->smartMode->setIcon(QIcon(":/icons/olivia_mini_icon.png"));
    ui->left_panel->layout()->addWidget(ui->nowplaying_widget);
    ui->horizontalLayout_11->addWidget(ui->radioVolumeSlider);
    ui->horizontalLayout_11->layout()->addWidget(ui->position);
    ui->horizontalLayout_11->layout()->addWidget(ui->radioSeekSlider);
    ui->horizontalLayout_11->layout()->addWidget(ui->duration);
    ui->smart_playlist_holder->layout()->addWidget(ui->recommHolder);
    ui->centralWidget->layout()->addWidget(ui->controls_widget);
    smartModeWidget->setMaximumHeight(16777215);
    this->showNormal();
  }
}

void MainWindow::on_repeat_stateChanged(int arg1) {
  settingsObj.setValue("repeat", arg1);
  QListWidget *list = this->findChild<QListWidget *>(
      getCurrentPlayerQueue(nowPlayingSongIdWatcher->getValue()));
  if (list == nullptr && list->objectName() != "downloadList")
    return;

  switch (arg1) {
  case 0: {
    // unchecked
    // repeat off
    ui->repeat->setToolTip("Repeat is off");
    notify("", "Repeat Off");
    ui->repeat->setIcon(QIcon(":/icons/p_repeat_off.png"));
    nextPreviousHelper(list);
    break;
  }
  case 1: {
    // partial
    // repeat current song
    // disable shuffle if repeat current song is on
    if (ui->shuffle->isChecked()) {
      ui->shuffle->setChecked(false);
      notify("", "Repeat current song, Shuffle: Off");
    }

    ui->repeat->setToolTip("Repeating current song");
    notify("", "Repeat current song");
    ui->repeat->setIcon(QIcon(":/icons/p_repeat.png"));
    if (list != nullptr) {
      QWidget *listWidgetItem = list->findChild<QWidget *>(
          "track-widget-" + nowPlayingSongIdWatcher->getValue());
      if (listWidgetItem != nullptr) {
        QPoint pos = listWidgetItem->pos();
        int index = list->row(list->itemAt(pos));
        assignNextTrack(list, index);
      }
    }
    break;
  }
  case 2:
    // checked
    // repeat queue
    ui->repeat->setToolTip("Repeating current queue");
    notify("", "Repeat current queue");
    ui->repeat->setIcon(QIcon(":/icons/p_repeat_queue.png"));
    // repeat queue
    if (!ui->next->isEnabled() || list->currentRow() == list->count() - 1) {
      assignNextTrack(this->findChild<QListWidget *>(getCurrentPlayerQueue(
                          nowPlayingSongIdWatcher->getValue())),
                      0);
    } else {
      nextPreviousHelper(list);
    }
    break;
  default:
    break;
  }
}

void MainWindow::showToast(QString message) {

  if (message.contains(":force:", Qt::CaseInsensitive)) {
    qDebug() << "loading smart playilist";
  } else if (message.contains("Playing") ||
             !this->isVisible()) // only show notification toast when player is
                                 // in normal mode.
    return;

  QWidget *widget = this->findChild<QWidget *>("toastWidget");
  if (widget != nullptr) {
    widget->close();
    widget->deleteLater();
  }

  QWidget *toastWidget = new QWidget(this, Qt::ToolTip);
  toastWidget->setObjectName("toastWidget");
  toastWidget->setWindowModality(Qt::NonModal);
  toastWidget->setWindowOpacity(qreal(QVariant("95").toReal() / 100));
  toastWidget->setStyleSheet("QWidget#toastWidget{border:none;background-color:"
                             "rgba(26,128,166,197);}");
  toast.setupUi(toastWidget);
  toast.messageLabel->setText(message.remove(":force:"));
  connect(toast.close, &QPushButton::clicked, toastWidget,
          [=]() { toastWidget->close(); });

  QTimer *toastTimer = new QTimer(toastWidget);
  toastTimer->setInterval(4000);
  connect(toastTimer, &QTimer::timeout, toastWidget, [=]() {
    if (toastWidget != nullptr && toastWidget->isVisible()) {
      toastWidget->close();
      toastWidget->deleteLater();
    }
  });
  toastTimer->start();
  toastWidget->adjustSize();
  int x =
      QApplication::desktop()->geometry().width() - (toastWidget->width() + 10);
  int y = 40;

  QPropertyAnimation *a = new QPropertyAnimation(toastWidget, "pos");
  a->setDuration(200);
  a->setStartValue(QCursor::pos());
  a->setEndValue(QApplication::desktop()->mapToGlobal(QPoint(x, y)));
  a->setEasingCurve(QEasingCurve::Linear);
  a->start(QPropertyAnimation::DeleteWhenStopped);
  toastWidget->show();
}

void MainWindow::on_clear_clicked() {
  similarTracks->clearListKeepingPlayingTrack();
}
