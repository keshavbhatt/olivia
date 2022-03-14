#include "download_widget.h"
#include "elidedlabel.h"
#include "settings.h"
#include "store.h"
#include "ui_download_widget.h"
#include "utils.h"
#include <QDateTime>
#include <QDebug>
#include <QDesktopServices>
#include <QDir>
#include <QRegExp>
#include <QStandardPaths>
#include <QRandomGenerator>

Widget::Widget(QWidget *parent) : QWidget(parent), ui(new Ui::Widget) {
  ui->setupUi(this);

  ui->removeSelected->setEnabled(false);
  ui->startSelected->setEnabled(false);
  ui->pauseSelected->setEnabled(false);

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

  foreach (QPushButton *btn, this->findChildren<QPushButton *>()) {
    btn->setStyleSheet(btn_style);
  }

  color << "background-color: rgb(204, 0, 0);"    // 0 error
        << "background-color: rgb(0, 204, 0);"    // 1 running
        << "background-color: rgb(239, 167, 75);" // 2 paused
        << "background-color:rgb(110, 179, 228);" // 3 finished
      ;

  ui->already_added->hide();
  update_stats(); // update the download info counter

  setting_path = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
  if (!QDir(setting_path).exists()) {
    QDir d(setting_path);
    d.mkpath(setting_path);
  }

  // prepare database dir and file
  QDir dir(returnPath("videoDownloadHistory"));
  dbFile.setFileName(dir.path() + "/database.db");
  // prepare history dir
  QDir(returnPath("videoDownloadHistory"));
  read_history();
}

// on demand path maker
QString Widget::returnPath(QString pathname) {
  if (!QDir(setting_path + "/" + pathname).exists()) {
    QDir d(setting_path + "/" + pathname);
    d.mkpath(setting_path + "/" + pathname);
  }
  return setting_path + "/" + pathname + "/";
}

Widget::~Widget() { delete ui; }

void Widget::insertDownloadWidget(QString title, QString downloadLocation,
                                  QString uuid, bool from_history,
                                  QStringList item_info, QString processId) {
  QWidget *download_widget = new QWidget(this);
  download_widget->setWindowFlags(Qt::Widget);
  downloadItemUi.setupUi(download_widget);

  downloadItemUi.uuid->hide();
  downloadItemUi.destination->hide();

  if (from_history) {
    downloadItemUi.title->setText(title);
  } else {
    downloadItemUi.title->setText("Title: " + title);
    downloadItemUi.title->setToolTip(title);
  }

  downloadItemUi.destination->setText("Destination: " + downloadLocation);
  downloadItemUi.args->setText(downloadArgs);
  downloadItemUi.uuid->setText(uuid);
  downloadItemUi.args->hide();
  downloadArgs.clear();

  if (from_history) {
    downloadItemUi.progressBar->setMaximum(100);
    downloadItemUi.progressBar->setMinimum(0);
    downloadItemUi.progressBar->setValue(item_info.at(3).toInt());
    downloadItemUi.downloaded->setText(item_info.at(7));
    downloadItemUi.size->setText(item_info.at(4));
    downloadItemUi.status->setText(item_info.at(2));
    downloadItemUi.speed->setText("Speed: ~");
    downloadItemUi.eta->setText("ETA: ~");
    downloadItemUi.color->setStyleSheet(item_info.at(8));
    downloadItemUi.percentage->setText(item_info.at(6));
  } else {
    downloadItemUi.progressBar->setMaximum(0);
    downloadItemUi.progressBar->setMinimum(0);
  }
  download_widget->adjustSize();

  QListWidgetItem *item;
  item = new QListWidgetItem(ui->downloadList);
  ui->downloadList->addItem(item);
  download_widget->setObjectName("processWidgetObject#" + processId);

  item->setSizeHint(download_widget->minimumSizeHint());

  ui->downloadList->setItemWidget(item, download_widget);
  QGraphicsOpacityEffect *eff = new QGraphicsOpacityEffect(this);
  ui->downloadList->itemWidget(item)->setGraphicsEffect(eff);
  QPropertyAnimation *a = new QPropertyAnimation(eff, "opacity");
  a->setDuration(500);
  a->setStartValue(0);
  a->setEndValue(1);
  a->setEasingCurve(QEasingCurve::InCirc);
  a->start(QPropertyAnimation::DeleteWhenStopped);

  int currentRow = ui->downloadList->currentRow();
  ui->removeSelected->setEnabled(currentRow > -1);
  ui->downloadList->setCurrentRow(ui->downloadList->count() - 1);

  if (!from_history) {
    // write item info to dbFIle
    if (dbFile.open(QFile::ReadWrite | QIODevice::Append)) {
      QString urlstr =
          downloadItemUi.args->text().split(this->trackId + " ").last();
      QTextStream out(&dbFile);
      out << uuid + " >> " + urlstr.toUtf8().toBase64() << endl;
    } else {
      qDebug() << "cannot open database file for write";
    }
    dbFile.close();
    save_download_item(processId);
  }
  // progress

  update_ui_btns(processId);
}

// history
// database================================================================================================================
void Widget::save_download_item(QString processId) {
  QWidget *itemWidget = ui->downloadList->findChild<QWidget *>(
      "processWidgetObject#" + processId);
  QProcess *p = this->findChild<QProcess *>("downloadProcess#" + processId);

  if (p) {
    ElidedLabel *uuidLabel = itemWidget->findChild<ElidedLabel *>("uuid");
    ElidedLabel *argLabel = itemWidget->findChild<ElidedLabel *>("args");
    ElidedLabel *statusLabel = itemWidget->findChild<ElidedLabel *>("status");
    ElidedLabel *sizeLabel = itemWidget->findChild<ElidedLabel *>("size");
    ElidedLabel *titleLabel = itemWidget->findChild<ElidedLabel *>("title");
    QProgressBar *progress =
        itemWidget->findChild<QProgressBar *>("progressBar");
    ElidedLabel *percentageLabel =
        itemWidget->findChild<ElidedLabel *>("percentage");
    ElidedLabel *downloadedLabel =
        itemWidget->findChild<ElidedLabel *>("downloaded");
    ElidedLabel *colorLabel = itemWidget->findChild<ElidedLabel *>("color");

    QString args = argLabel->text().split("https").first();
    QString urlstr = argLabel->text().split("https").last().prepend("https");

    // prepare database
    QDir dir(returnPath("videoDownloadHistory"));
    QFile item_history(dir.path() + "/" + uuidLabel->text());
    if (item_history.open(QFile::ReadWrite | QIODevice::Truncate)) {
      QTextStream out(&item_history);
      out << args + ">>" + urlstr + ">>" << endl;
      out << uuidLabel->text() << endl;
      out << statusLabel->text() << endl;
      out << progress->value() << endl;
      out << sizeLabel->text() << endl;
      out << titleLabel->text() << endl;
      out << percentageLabel->text() << endl;
      out << downloadedLabel->text() << endl;
      out << colorLabel->styleSheet() << endl;
    } else {
      qDebug() << "cannot open database file for write";
    }
    item_history.close();

  } else {
    qDebug() << "process deleted";
  }
}

void Widget::read_history() {
  QStringList dbItems;
  if (dbFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
    QTextStream in(&dbFile);
    while (!in.atEnd()) {
      dbItems.append(in.readLine());
    }

  } else {
    qDebug() << "cannot read history database file";
  }
  dbFile.close();

  add_history_items_to_ui(dbItems);
}

void Widget::add_history_items_to_ui(QStringList dbItems) {
  for (int i = 0; i < dbItems.count(); i++) {
    QDir dir(returnPath("videoDownloadHistory"));
    QString fileName = dbItems.at(i).split(" >> ").first().trimmed();
    qDebug() << fileName;
    QStringList info;
    QFile item_history(dir.path() + "/" + fileName);
    // prevent crash if history item does not exist
    if (QFileInfo(item_history).exists() == false) {
      qDebug() << "FILE" << item_history.fileName() << "notfound";
      item_history.deleteLater();
    } else {
      if (item_history.open(QFile::ReadOnly | QIODevice::Text)) {
        QTextStream in(&item_history);
        while (!in.atEnd()) {
          info.append(in.readLine().remove("\n"));
        }
      } else {
        qDebug() << "cannot open database file for write";
      }
      item_history.close();

      // startProcess
      start_process(info);
      // insert item
    }
  }
}

void Widget::start_process(QStringList iteminfo) {

  QString program = "python3";
  QStringList arguments;
  arguments.append(QString(iteminfo.at(0).split(" >>").first()).split(" "));
  arguments.append(iteminfo.at(0).split(" >>").last().split(">>").first());

  downloadArgs = arguments.join(" ");

  QString uuid = iteminfo.at(1);
  QString title = iteminfo.at(5);
  QString downloadLocation_ = this->downloadLocation;

  QProcess *downloadProcess = new QProcess(this);
  downloadProcess->setProgram(program);
  downloadProcess->setArguments(arguments);
  downloadList.append(downloadProcess);
  QString processId = utils::generateRandomId(5);
  downloadProcess->setObjectName("downloadProcess#" + processId);

  connect(downloadProcess, SIGNAL(readyRead()), this,
          SLOT(downloadProcessReadyRead()));
  connect(downloadProcess, SIGNAL(finished(int)), this,
          SLOT(downloadProcessFinished(int)));
  connect(downloadProcess, SIGNAL(stateChanged(QProcess::ProcessState)), this,
          SLOT(downloadProcessStateChanged(QProcess::ProcessState)));

  bool from_history = true;

  insertDownloadWidget(title, downloadLocation_, uuid, from_history, iteminfo,
                       processId);
}

QString Widget::make_database_file(QUrl url) {
  // random string from url
  const QString possibleCharacters(
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789" +
      url.toString().remove(QRegExp("[^a-zA-Z\\d\\s]")));
  const int randomStringLength = 8;
  QString randomString;
  QDateTime cd = QDateTime::currentDateTime();

  for (int i = 0; i < randomStringLength; ++i) {
    int index = QRandomGenerator::system()->generate() % possibleCharacters.length();
    QChar nextChar = possibleCharacters.at(index);
    randomString.append(nextChar);
  }
  QString randomString_final =
      randomString.trimmed().simplified() +
      QString::number(cd.currentDateTime().time().msec())
          .trimmed()
          .simplified();
  randomString_final = randomString_final.trimmed().simplified().remove(" ");
  if (!check_if_url_already_exist(url.toString())) {
    // prepare database
    QDir dir(returnPath("videoDownloadHistory"));
    QFile item_history(dir.path() + "/" + randomString_final);
    if (item_history.open(QFile::ReadWrite | QIODevice::Truncate)) {
      item_history.write("");
    } else {
      qDebug() << "cannot open database file for write";
    }
    item_history.close();
    return randomString_final;
  } else {
    return "false"; // prevents url to be added to download list
  }
}

// read dbFile and check if url exist
bool Widget::check_if_url_already_exist(QString urlstr) {
  bool urlfound = false;
  QStringList urls;
  if (dbFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
    QTextStream in(&dbFile);
    while (!in.atEnd()) {
      urls.append(in.readLine());
    }
  } else {
    qDebug() << "cannot read history database file";
  }
  dbFile.close();
  bool present = false;
  for (int i = 0; i < urls.count(); i++) {
    QString url_from_line = QByteArray::fromBase64(
        QString(urls.at(i)).split(">> ").last().toUtf8());
    url_from_line.remove("\n");
    if (url_from_line == urlstr) {
      present = true;
      i = urls.count();
      break;
    } else {
      present = false;
    }
  }
  urlfound = present;
  return urlfound;
}

// from history
void Widget::remove_download_item(QString uuid) {
  // prepare database
  QDir dir(returnPath("videoDownloadHistory"));
  // remove history item
  QFile item_history(dir.path() + "/" + uuid);
  item_history.remove();

  // remove from dbFile
  QStringList lines;
  QStringList full_lines;
  if (dbFile.open(QFile::ReadWrite | QIODevice::Text)) {
    QTextStream in(&dbFile);
    while (!in.atEnd()) {
      QString line = in.readLine();
      lines.append(line.split(" >>").first());
      full_lines.append(line);
    }
    int at = lines.indexOf(uuid);
    if (at != -1) {
      //            qDebug()<<"found";
      lines.removeAt(at);
      full_lines.removeAt(at);
      dbFile.close();
      update_db_file(full_lines);
    }
  } else {
    qDebug() << "cannot open database file for write";
  }
}

void Widget::update_db_file(QStringList lines) {
  //    qDebug()<<"updatecalled"<<lines;
  if (dbFile.open(QFile::ReadWrite | QIODevice::Truncate)) {
    QTextStream out(&dbFile);
    for (int i = 0; i < lines.count(); i++) {
      out << lines.at(i) << endl;
    }
  } else {
    qDebug() << "cannot open database file for write";
  }
  dbFile.close();
}
// EOF history
// database================================================================================================================

void Widget::startWget(QString url_str, QString downloadLocation,
                       QStringList formats, QString type) {

  QString title = this->trackTitle;
  QString uuid = make_database_file(QUrl(url_str));
  if (uuid != "false") {
    QString program = "python3";
    QStringList arguments;
    arguments.append(setting_path + "/core");
    arguments.append("-f");
    if (type == "audio") {
      arguments.append(formats.at(0));
    } else {
      arguments.append(formats.at(0) + "+" + formats.at(1));
    }
    QString miscArgs;

    QString location_suffix;
    if (downloadLocation.at(downloadLocation.length() - 1) != QString("/")) {
      location_suffix = "/" + trackId;
    } else {
      location_suffix = trackId;
    }

    miscArgs = "-o " + downloadLocation + location_suffix;

    arguments.append(miscArgs.split(" "));

    arguments << url_str;

    downloadArgs = arguments.join(" ");

    QProcess *downloadProcess = new QProcess(this);
    QString processId = utils::generateRandomId(5);
    downloadProcess->setObjectName("downloadProcess#" + processId);
    downloadProcess->start(program, arguments);
    downloadList.append(downloadProcess);
    connect(downloadProcess, SIGNAL(readyRead()), this,
            SLOT(downloadProcessReadyRead()));
    connect(downloadProcess, SIGNAL(finished(int)), this,
            SLOT(downloadProcessFinished(int)));
    connect(downloadProcess, SIGNAL(stateChanged(QProcess::ProcessState)), this,
            SLOT(downloadProcessStateChanged(QProcess::ProcessState)));
    bool from_history = false;
    QStringList blank_;
    insertDownloadWidget(title, downloadLocation, uuid, from_history, blank_,
                         processId);
  } else {
    QGraphicsOpacityEffect *eff = new QGraphicsOpacityEffect(this);
    ui->already_added->setGraphicsEffect(eff);
    QPropertyAnimation *a = new QPropertyAnimation(eff, "opacity");
    a->setDuration(500);
    a->setStartValue(0);
    a->setEndValue(1);
    a->setEasingCurve(QEasingCurve::InCurve);
    a->start(QPropertyAnimation::DeleteWhenStopped);
    ui->already_added->show();

    hide_already_inList();
  }
}

void Widget::hide_already_inList() {
  QGraphicsOpacityEffect *eff = new QGraphicsOpacityEffect(this);
  ui->already_added->setGraphicsEffect(eff);
  QPropertyAnimation *a = new QPropertyAnimation(eff, "opacity");
  a->setDuration(5000);
  a->setStartValue(1);
  a->setEndValue(0);
  a->setEasingCurve(QEasingCurve::Linear);
  a->start(QPropertyAnimation::DeleteWhenStopped);
  connect(a, &QPropertyAnimation::finished,a,
          [ this] { ui->already_added->hide(); });
}

// download Process updates
// ==============================================================================================================
void Widget::downloadProcessReadyRead() {

  QProcess *senderProcess =
      qobject_cast<QProcess *>(sender()); // retrieve the sender Process
  QString processName = senderProcess->objectName();

  QString processId = processName.split("#").last();

  save_download_item(processId);

  //[download]   0.0% of 2.43GiB at 10.25KiB/s ETA 68:57:55
  QString output = senderProcess->readAll();

  QObject *itemObject = ui->downloadList->findChild<QObject *>(
      "processWidgetObject#" + processId);

  QObject *speedLabel = itemObject->findChild<QObject *>("speed");
  QObject *statusLabel = itemObject->findChild<QObject *>("status");
  QObject *sizeLabel = itemObject->findChild<QObject *>("size");
  QObject *etaLabel = itemObject->findChild<QObject *>("eta");
  //  QObject *titleLabel  = itemObject->findChild<QObject*>("title");
  QObject *progress = itemObject->findChild<QObject *>("progressBar");
  QObject *colorLabel = itemObject->findChild<QObject *>("color");
  QObject *percentageLabel = itemObject->findChild<QObject *>("percentage");
  QObject *downloadedLabel = itemObject->findChild<QObject *>("downloaded");

  ((ElidedLabel *)(speedLabel))->setText("Speed: ~");
  ((ElidedLabel *)(sizeLabel))->setText("Size: ~");
  ((ElidedLabel *)(etaLabel))->setText("ETA: ~");
  ((ElidedLabel *)(percentageLabel))->setText("Progress: ~");
  ((ElidedLabel *)(downloadedLabel))->setText("Downloaded: ~");
  ((ElidedLabel *)(colorLabel))->setStyleSheet(color.at(1));

  QString progressVal;
  QString progressValExact;

  if ((output.contains(QString("[download]"))) &&
      (!output.contains("[download] Destination:")) &&
      (!output.contains("Merging formats into")) &&
      (!output.contains("Resuming download")) &&
      (!output.contains("[ffmpeg]")) && !output.contains("fragments")) {
    QRegExp rx("(\\d+\\.\\d+%)");
    rx.indexIn(output);
    if (!rx.cap(0).isEmpty()) {
      progressVal = rx.cap(0);
      progressVal.chop(3);
    }
    // exact percent value
    QRegExp rxe("(\\d.+%)");
    rxe.indexIn(output);
    if (!rxe.cap(0).isEmpty()) {
      progressValExact = rxe.cap(0).remove("%");
    }
    QString downspeed =
        QString(QString(output.split(" at ").last()).split(" ETA").first())
            .remove("[download]");
    QString eta = QString(output.split("ETA ").last()).remove("[download]");
    QString size =
        QString(QString(output.split("% of ").last()).split(" at ").first())
            .remove("[download]");

    // set values
    ((ElidedLabel *)(speedLabel))->setText("Speed: " + downspeed);
    ((ElidedLabel *)(statusLabel))->setText("Status: Downloading");
    ((ElidedLabel *)(sizeLabel))->setText("Size: " + size);
    ((ElidedLabel *)(etaLabel))->setText("ETA: " + eta);

    ((QProgressBar *)(progress))->setMaximum(100);
    ((QProgressBar *)(progress))->setMinimum(0);
    ((QProgressBar *)(progress))->setValue(progressVal.toInt());
    ((ElidedLabel *)(percentageLabel))
        ->setText("Progress: " + QString::number(progressValExact.toDouble()) +
                  " %");

    QRegExp rxp("([A-Za-z]+)");
    QString match;
    rxp.indexIn(size);
    if (!rxp.cap(0).isEmpty()) {
      match = rxp.cap(0);
    }
    double tot_size = QString(size.split(match).first()).toDouble();
    double downloadedSize = (progressValExact.toDouble() / 100) * tot_size;

    ((ElidedLabel *)(downloadedLabel))
        ->setText("Downloaded: " + QString::number(downloadedSize, 'f', 3) +
                  match);
  }
}

void Widget::downloadProcessFinished(int exitCode) {

  QProcess *senderProcess =
      qobject_cast<QProcess *>(sender()); // retrieve the sender Process
  QString processName = senderProcess->objectName();
  QString processId = processName.split("#").last();
  update_ui_btns(processId);

  QObject *itemObject = ui->downloadList->findChild<QObject *>(
      "processWidgetObject#" + processId);

  QObject *speedLabel = itemObject->findChild<QObject *>("speed");
  QObject *statusLabel = itemObject->findChild<QObject *>("status");
  QObject *sizeLabel = itemObject->findChild<QObject *>("size");
  QObject *etaLabel = itemObject->findChild<QObject *>("eta");
  QObject *progress = itemObject->findChild<QObject *>("progressBar");
  QObject *colorLabel = itemObject->findChild<QObject *>("color");
  QObject *percentageLabel = itemObject->findChild<QObject *>("percentage");
  QObject *downloadedLabel = itemObject->findChild<QObject *>("downloaded");
  QObject *titleLabel = itemObject->findChild<QObject *>("title");

  if (exitCode == QProcess::NormalExit) {
    // set values
    ((ElidedLabel *)(speedLabel))->setText("Speed: Finished");
    ((ElidedLabel *)(etaLabel))->setText("ETA: Finished");
    ((ElidedLabel *)(statusLabel))->setText("Status: Finished");
    ((ElidedLabel *)(downloadedLabel))->setText("Downloaded: 100%");
    ((ElidedLabel *)(sizeLabel))->setText("Size: Finished");
    ((ElidedLabel *)(percentageLabel))->setText("Progress: 100%");
    ((ElidedLabel *)(colorLabel))->setStyleSheet(color.at(3));

    ((QProgressBar *)(progress))->setMaximum(100);
    ((QProgressBar *)(progress))->setMinimum(0);
    ((QProgressBar *)(progress))->setValue(100);
    ui->startSelected->setEnabled(false);
    save_download_item(processId);

    QObject *itemObject = ui->downloadList->findChild<QObject *>(
        "processWidgetObject#" + processId);
    ElidedLabel *argLabel = itemObject->findChild<ElidedLabel *>("args");
    QString videoId =
        argLabel->text().split(" https://").first().split("/").last();

    // update the track in queue
    if (((ElidedLabel *)(titleLabel))->text().contains(" [Audio]")) {
      QObject *parentMainWindow = this->parent();
      while (!parentMainWindow->objectName().contains("MainWindow")) {
        parentMainWindow = parentMainWindow->parent();
      }
      store *store_manager =
          parentMainWindow->findChild<store *>("store_manager");
      if (store_manager != nullptr) {
        store_manager->update_track("downloaded", videoId, "1");
        emit updateTrack(trackId, downloadLocationAudio);
      }
    }
  } else {
    ((ElidedLabel *)(statusLabel))->setText("Status: Error");
    ((QProgressBar *)(progress))->setMaximum(100);
    ((QProgressBar *)(progress))->setMinimum(0);
    ((QProgressBar *)(progress))->setValue(0);
    save_download_item(processId);
  }
  update_stats();
}

void Widget::downloadProcessStateChanged(QProcess::ProcessState processState) {
  QProcess *senderProcess =
      qobject_cast<QProcess *>(sender()); // retrieve the sender Process
  // qDebug()<<senderProcess->arguments();
  QString processName = senderProcess->objectName();
  QString processId = processName.split("#").last();
  update_ui_btns(processId);

  QObject *itemObject = ui->downloadList->findChild<QObject *>(
      "processWidgetObject#" + processId);

  QObject *colorLabel = itemObject->findChild<QObject *>("color");
  if (processState == QProcess::Running) {
    ((ElidedLabel *)(colorLabel))->setStyleSheet(color.at(1));
    save_download_item(processId);
  }
  if (processState == QProcess::NotRunning) {
    ((ElidedLabel *)(colorLabel))->setStyleSheet(color.at(0));
    save_download_item(processId);
  }

  update_stats();
}

// EOFs download Process updates
// ==============================================================================================================

void Widget::on_downloadList_currentRowChanged(int currentRow) {
  if (currentRow > -1) {
    ui->removeSelected->setEnabled(true);
    ui->startSelected->setEnabled(true);
    ui->pauseSelected->setEnabled(true);
    ui->removeAll->setEnabled(true);
    ui->startAll->setEnabled(true);
    ui->pauseAll->setEnabled(true);
    QListWidgetItem *item = ui->downloadList->item(currentRow);
    QString processId =
        ui->downloadList->itemWidget(item)->objectName().split("#").last();
    update_ui_btns(processId); // widget number and process number are same;
  } else {
    ui->removeSelected->setEnabled(false);
    ui->startSelected->setEnabled(false);
    ui->pauseSelected->setEnabled(false);
    ui->removeAll->setEnabled(false);
    ui->startAll->setEnabled(false);
    ui->pauseAll->setEnabled(false);
  }
}

void Widget::update_ui_btns(QString processId) {

  // enable pause btn if process state running
  QProcess *p = this->findChild<QProcess *>("downloadProcess#" + processId);
  QWidget *widget = ui->downloadList->findChild<QWidget *>(
      "processWidgetObject#" + processId);
  QListWidgetItem *item = ui->downloadList->itemAt(widget->pos());
  if (item != nullptr) {
    ElidedLabel *statusLabel =
        ui->downloadList->itemWidget(item)->findChild<ElidedLabel *>("status");

    if (p->state() == QProcess::Running) {
      ui->pauseSelected->setEnabled(true);
    } else {
      ui->pauseSelected->setEnabled(false);
    }

    // enable start btn if process state stopped
    if (p->state() == QProcess::NotRunning &&
        !statusLabel->text().contains("Finished")) {
      ui->startSelected->setEnabled(true);
    } else {
      ui->startSelected->setEnabled(false);
    }
  }
  update_stats();
}

void Widget::update_stats() {
  // calculate stats
  int running = 0, paused = 0, stopped = 0, finished = 0;
  int total = ui->downloadList->count();
  if (total <= 0) {
    ui->downloadList->hide();
    QGraphicsOpacityEffect *eff = new QGraphicsOpacityEffect(this);
    ui->no_download_label->setGraphicsEffect(eff);
    QPropertyAnimation *a = new QPropertyAnimation(eff, "opacity");
    a->setDuration(500);
    a->setStartValue(0);
    a->setEndValue(1);
    a->setEasingCurve(QEasingCurve::Linear);
    a->start(QPropertyAnimation::DeleteWhenStopped);
    ui->no_download_label->show();
  } else {
    ui->downloadList->show();
    ui->no_download_label->hide();
  }
  for (int i = 0; i < downloadList.count(); i++) {

    QListWidgetItem *item = ui->downloadList->item(i);

    QObject *statusLabel =
        ui->downloadList->itemWidget(item)->findChild<QObject *>("status");

    if (downloadList.at(i)->state() == QProcess::Running) {
      running++;
    }

    if (((ElidedLabel *)(statusLabel))->text().contains("Paused")) {
      paused++;
    }

    if (((ElidedLabel *)(statusLabel))->text().contains("Error")) {
      stopped++;
    }

    if (((ElidedLabel *)(statusLabel))->text().contains("Finished")) {
      finished++;
    }
  }
  QString src =
      "<html><head/><body><p><span style='font-size:9pt; font-weight:600; "
      "color:#babdb6;'/><span "
      "style=' font-size:9pt; font-weight:600; color:#00cc00;'>running: " +
      QString::number(running) +
      "</span><span style=' font-size:9pt;"
      "font-weight:600; color:#babdb6;'> | </span><span style=' font-size:9pt; "
      "font-weight:600; color:#EFA74B;'"
      ">paused: " +
      QString::number(paused) +
      " </span><span style=' font-size:9pt; font-weight:600; color:#babdb6;'>| "
      "</span><span style='"
      "font-size:9pt; font-weight:600; color:#cc0000;'>stopped: " +
      QString::number(stopped) +
      "</span><span style=' font-size:9pt; "
      "font-weight:600; color:#babdb6;'> | </span><span style='"
      "font-size:9pt; font-weight:600; color:#6EB3E4;'>finished: " +
      QString::number(finished) + "</span></p></body></html>";
  ui->stats->setText(src);
  ui->startAll->setEnabled(total > 1 && running < total);
  ui->pauseAll->setEnabled(running > 0 && total > 1);
  ui->removeAll->setEnabled(total > 0);
}

// Buttons slots
// ==============================================================================================================
void Widget::on_removeSelected_clicked() {
  QListWidgetItem *item = ui->downloadList->currentItem();
  QString processId =
      ui->downloadList->itemWidget(item)->objectName().split("#").last();
  QProcess *p = this->findChild<QProcess *>("downloadProcess#" + processId);
  p->close(); // close connections
  if (p) {
    disconnect(p, SIGNAL(finished(int, QProcess::ExitStatus)), this,
               SLOT(downloadProcessFinished(int)));
    disconnect(p, SIGNAL(readyRead()), this, SLOT(downloadProcessReadyRead()));
    disconnect(p, SIGNAL(stateChanged(QProcess::ProcessState)), this,
               SLOT(downloadProcessStateChanged(QProcess::ProcessState)));
    p->close();
    ElidedLabel *uuid =
        ui->downloadList->itemWidget(item)->findChild<ElidedLabel *>("uuid");
    if (p->state() == QProcess::Running || p->state() == QProcess::Starting) {
      remove_download_item(uuid->text());
      downloadList.removeOne(p);
      p->deleteLater();
      delete ui->downloadList->takeItem(ui->downloadList->currentRow());
    } else if (p->state() == QProcess::NotRunning) {
      remove_download_item(uuid->text());
      downloadList.removeOne(p);
      p->deleteLater();
      delete ui->downloadList->takeItem(ui->downloadList->currentRow());
    }
  }
  update_stats();
}

void Widget::on_startSelected_clicked() {
  QListWidgetItem *item = ui->downloadList->currentItem();
  QObject *progressBar =
      ui->downloadList->itemWidget(item)->findChild<QObject *>("progressBar");
  QObject *statusLabel =
      ui->downloadList->itemWidget(item)->findChild<QObject *>("status");

  QString processId =
      ui->downloadList->itemWidget(item)->objectName().split("#").last();
  QProcess *p = this->findChild<QProcess *>("downloadProcess#" + processId);
  // start process
  if (p != nullptr) {
    ((ElidedLabel *)(statusLabel))->setText("Status: Starting");
    ((QProgressBar *)(progressBar))->setMinimum(0);
    ((QProgressBar *)(progressBar))->setMaximum(0);
    p->start();
    save_download_item(processId);
  }
  update_stats();
}

void Widget::on_pauseSelected_clicked() {
  // get progress
  QListWidgetItem *item = ui->downloadList->currentItem();
  if (item == nullptr) {
    return;
  }
  QString processId =
      ui->downloadList->itemWidget(item)->objectName().split("#").last();

  QProgressBar *progressBar =
      ui->downloadList->itemWidget(item)->findChild<QProgressBar *>(
          "progressBar");
  int progress = progressBar->value();

  ElidedLabel *speedLabel =
      ui->downloadList->itemWidget(item)->findChild<ElidedLabel *>("speed");
  ElidedLabel *etaLabel =
      ui->downloadList->itemWidget(item)->findChild<ElidedLabel *>("eta");
  ElidedLabel *statusLabel =
      ui->downloadList->itemWidget(item)->findChild<ElidedLabel *>("status");
  ElidedLabel *colorLabel =
      ui->downloadList->itemWidget(item)->findChild<ElidedLabel *>("color");

  // stop process
  QProcess *p = this->findChild<QProcess *>("downloadProcess#" + processId);
  if (p == nullptr)
    return;
  p->close();
  p->waitForFinished();
  // set progress back
  progressBar->setValue(progress);
  statusLabel->setText("Status: Paused");
  speedLabel->setText("Speed: ~");
  etaLabel->setText("ETA: ~");
  colorLabel->setStyleSheet(color.at(2));
  save_download_item(processId);
  update_stats();
}

void Widget::on_startAll_clicked() {
  for (int i = 0; i < downloadList.count(); i++) {
    QListWidgetItem *item = ui->downloadList->item(i);
    QObject *progressBar =
        ui->downloadList->itemWidget(item)->findChild<QObject *>("progressBar");
    int progress = ((QProgressBar *)(progressBar))->value();
    if (progress != 100) {
      if (downloadList.at(i)->state() != QProcess::Running) {
        ((QProgressBar *)(progressBar))->setMinimum(0);
        ((QProgressBar *)(progressBar))->setMaximum(0);
        downloadList.at(i)->start();
        QString processId = downloadList.at(i)->objectName().split("#").last();
        save_download_item(processId);
      }
    }
  }
  update_stats();
}

void Widget::on_pauseAll_clicked() {
  for (int i = 0; i < downloadList.count(); i++) {
    QListWidgetItem *item = ui->downloadList->item(i);
    QObject *progressBar =
        ui->downloadList->itemWidget(item)->findChild<QObject *>("progressBar");
    QObject *speedLabel =
        ui->downloadList->itemWidget(item)->findChild<QObject *>("speed");
    QObject *etaLabel =
        ui->downloadList->itemWidget(item)->findChild<QObject *>("eta");
    QObject *statusLabel =
        ui->downloadList->itemWidget(item)->findChild<QObject *>("status");
    QObject *colorLabel =
        ui->downloadList->itemWidget(item)->findChild<QObject *>("color");

    int progress = ((QProgressBar *)(progressBar))->value();
    if (progress != 100) {
      if (downloadList.at(i)->state() == QProcess::Running ||
          downloadList.at(i)->state() == QProcess::Starting) {
        downloadList.at(i)->close();
        downloadList.at(i)->waitForFinished();
        // set progress back
        ((QProgressBar *)(progressBar))->setValue(progress);
        ((ElidedLabel *)(statusLabel))->setText("Status: Paused");
        ((ElidedLabel *)(speedLabel))->setText("Speed: ~");
        ((ElidedLabel *)(etaLabel))->setText("ETA: ~");
        ((ElidedLabel *)(colorLabel))->setStyleSheet(color.at(2));
        QString processId = downloadList.at(i)->objectName().split("#").last();
        save_download_item(processId);
      }
    }
  }
  update_stats();
}

void Widget::on_removeAll_clicked() {
  while (ui->downloadList->count() > 0) {
    ui->downloadList->setCurrentRow(ui->downloadList->count() - 1);
    if (ui->removeSelected->isEnabled()) {
      ui->removeSelected->click();
    }
  }
  update_stats();
}

void Widget::on_downloadList_itemDoubleClicked(QListWidgetItem *item) {
  QWidget *itemObject = ui->downloadList->itemWidget(item);
  ElidedLabel *argLabel = itemObject->findChild<ElidedLabel *>("args");
  ElidedLabel *titleLabel = itemObject->findChild<ElidedLabel *>("title");
  QString videoId =
      argLabel->text().split(" https://").first().split("/").last();

  settings *sett = new settings(this);
  int volume = sett->settingsObj.value("volume", 100).toInt();

  QDir dir;

  if (titleLabel->text().contains(" [Audio]")) {
    dir.setPath(setting_path + "/downloadedTracks/");
  } else {
    dir.setPath(setting_path + "/downloadedVideos/");
  }

  QString fileName = videoId;
  QString destination_path = dir.path();
  QFileInfoList files;
  foreach (QFileInfo info, dir.entryInfoList()) {
    if (info.fileName().contains(fileName))
      files.append(QList<QFileInfo>() << info);
  }
  if (files.count() > 0) {
    // remove .jpg file from result
    QString filenameFound = files.at(0).filePath();
    if (files.count() == 2 &&
        files.at(0).fileName().contains(".jpg", Qt::CaseInsensitive)) {
      filenameFound = files.at(1).filePath();
    }
    QProcess *player = new QProcess(this);
    player->setObjectName("player");
    player->start("mpv", QStringList()
                             << "--force-window"
                             << "--geometry=800x600+0+0"
                             << "--title=MPV for " +
                                    QApplication::applicationName() + " - " +
                                    titleLabel->text().toUtf8()
                             << "--no-ytdl" << filenameFound
                             << "--volume=" + QString::number(volume));
  } else {
    unableToLocateFile(fileName, destination_path);
  }
  sett->deleteLater();
}

void Widget::unableToLocateFile(QString filename, QString directory) {
  QMessageBox msgBox;
  msgBox.setText(QApplication::applicationName() +
                 " is unable to locate file:");
  msgBox.setIconPixmap(
      QPixmap(":/icons/sidebar/info.png")
          .scaled(42, 42, Qt::KeepAspectRatio, Qt::SmoothTransformation));
  msgBox.setStandardButtons(QMessageBox::Cancel);
  msgBox.setInformativeText("<i>" + directory + "/" + filename +
                            "</i> <br><br>What you would like to do ?");
  QPushButton *browse = new QPushButton("Browse", nullptr);
  msgBox.addButton(browse, QMessageBox::NoRole);
  connect(browse, &QPushButton::clicked,
          [=]() { QDesktopServices::openUrl(QUrl(directory)); });
  msgBox.exec();
}
