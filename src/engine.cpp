#include "engine.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

//"https://rg3.github.io/youtube-dl/update/LATEST_VERSION"
//"https://api.github.com/repos/yt-dlp/yt-dlp/releases/latest"
//"http://ktechpit.com/USS/engine/core_version"
QString ENGINE_VERSION_URL =
    "https://api.github.com/repos/yt-dlp/yt-dlp/releases/latest";

//"https://yt-dl.org/downloads/latest/youtube-dl"
//"https://github.com/yt-dlp/yt-dlp/releases/latest/download/yt-dlp"
//"http://ktechpit.com/USS/engine/core.eco"
QString ENGINE_DOWNLOAD_URL =
    "https://github.com/yt-dlp/yt-dlp/releases/latest/download/yt-dlp";

Engine::Engine(QObject *parent) : QObject(parent) {
  QTimer::singleShot(1000, [this]() {
    if (!checkEngine()) {
      evoke_engine_check();
      return;
    } else {
      EngineVersionFromEngine();
      check_engine_updates();
    }
  });
}

bool Engine::checkEngine() {
  QString setting_path =
      QStandardPaths::writableLocation(QStandardPaths::DataLocation);
  QFileInfo checkFile(setting_path + "/core");
  bool present = false;
  if (checkFile.exists() && checkFile.size() > 0) {
    emit engineStatus("Present");
    present = true;
  } else {
    emit engineStatus("Absent");
    present = false;
  }
  return present;
}

void Engine::download_engine_clicked() {

  QString addin_path =
      QStandardPaths::writableLocation(QStandardPaths::DataLocation);
  QDir dir(addin_path);
  if (!dir.exists())
    dir.mkpath(addin_path);

  QString filename = "core";
  core_file = new QFile(addin_path + "/" + filename);
  if (!core_file->open(QIODevice::ReadWrite | QIODevice::Truncate)) {
    qWarning() << "could not open engine file to write";
  }

  QNetworkAccessManager *m_netwManager = new QNetworkAccessManager(this);
  connect(m_netwManager, SIGNAL(finished(QNetworkReply *)), this,
          SLOT(slot_netwManagerFinished(QNetworkReply *)));
  QUrl url(ENGINE_DOWNLOAD_URL);
  QNetworkRequest request(url);

  emit engineStatus("Downloading...");
  qWarning() << "downloading engine...";

  m_netwManager->get(request);
}

QString Engine::enginePath() {
  QString addin_path =
      QStandardPaths::writableLocation(QStandardPaths::DataLocation);
  return addin_path + "/" + "core";
}

void Engine::slot_netwManagerFinished(QNetworkReply *reply) {
  if (reply->error() == QNetworkReply::NoError) {
    // Get the http status code
    int v = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (v >= 200 && v < 300) // Success
    {
      if (reply->error() == QNetworkReply::NoError) {
        core_file->write(reply->readAll());
        core_file->close();
        clearEngineCache(); // call clear engine cache after engine update
        get_engine_version_info();
        checkEngine();
        emit engineDownloadSucceeded();
        EngineVersionFromEngine(true);
      } else {
        core_file->remove();
      }

    } else if (v >= 300 && v < 400) // Redirection
    {
      // get the redirection url
      QUrl newUrl =
          reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
      // because the redirection url can be relative  we need to use the
      // previous one to resolve it
      newUrl = reply->url().resolved(newUrl);
      QNetworkAccessManager *manager = new QNetworkAccessManager();
      connect(manager, SIGNAL(finished(QNetworkReply *)), this,
              SLOT(slot_netwManagerFinished(
                  QNetworkReply *))); // keep requesting until reach final url
      manager->get(QNetworkRequest(newUrl));
    }
  } else // error
  {
    QString err = reply->errorString();
    if (err.contains("not found")) {
      emit engineStatus("Host not Found");
    } else if (err.contains("session") || err.contains("disabled")) {
      emit engineStatus(err);
    }
    emit engineDownloadFailed(err);
    emit engineDownloadSucceeded(); // fake UI fixer
    reply->manager()->deleteLater();
  }
  reply->deleteLater();
}

// function used to clear engine cache, to prevent 403 and 429 issue.
void Engine::clearEngineCache() {
  QProcess *clear_engine_cache = new QProcess(this);
  QString addin_path =
      QStandardPaths::writableLocation(QStandardPaths::DataLocation);
  connect(clear_engine_cache,
          static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(
              &QProcess::finished),
          [clear_engine_cache, this](int exitCode,
                                     QProcess::ExitStatus exitStatus) {
            Q_UNUSED(exitStatus);
            if (checkEngine() == true) {
              if (exitCode == 0) {
                emit engineCacheCleared();
              } else {
                emit errorMessage(QString(
                    QString("Error occured while clearing engine cache") +
                    clear_engine_cache->readAll()));
                emit engineCacheCleared(); // fake UI fixer
              }
            } else {
              emit errorMessage("Engine not present download engine first");
              emit engineStatus("Absent");
              emit engineCacheCleared(); // fake UI fixer
            }
          });

  clear_engine_cache->start("python3", QStringList() << addin_path + "/core"
                                                     << "--rm-cache-dir");
}

// writes core_version file with version info after core downloaded
void Engine::get_engine_version_info() {
  QNetworkAccessManager *m_netwManager = new QNetworkAccessManager(this);
  connect(
      m_netwManager, &QNetworkAccessManager::finished, [=](QNetworkReply *rep) {
        if (rep->error() == QNetworkReply::NoError) {
          QString addin_path =
              QStandardPaths::writableLocation(QStandardPaths::DataLocation);
          QDir dir(addin_path);
          if (!dir.exists())
            dir.mkpath(addin_path);

          QString filename = "core_version";
          QFile *core_version_file =
              new QFile(addin_path + "/" + filename); // addin_path
          if (!core_version_file->open(QIODevice::ReadWrite |
                                       QIODevice::Truncate)) {
            qDebug() << "Could not open a core_version_file to write.";
          }
          QString replyStr = rep->readAll();
          QString version;

          if (rep->request().url().toString().contains("api.")) {
            QJsonDocument doc = QJsonDocument::fromJson(replyStr.toUtf8());
            if (doc.isNull() == false) {
              QJsonObject obj = doc.object();
              version = obj.value("tag_name").toString().trimmed();
            }
          }
          // qDebug()<<"VERSION STR"<<version;
          core_version_file->write(version.toUtf8());
          core_version_file->close();
          core_version_file->deleteLater();
        }
        rep->deleteLater();
        m_netwManager->deleteLater();
      });
  QNetworkRequest request(ENGINE_VERSION_URL);
  m_netwManager->get(request);
}

void Engine::check_engine_updates() {
  // read version from local core_version file
  QString addin_path =
      QStandardPaths::writableLocation(QStandardPaths::DataLocation);
  QFile *core_version_file = new QFile(addin_path + "/" + "core_version");
  if (!core_version_file->open(QIODevice::ReadOnly | QIODevice::Text)) {
    core_local_date = "2019.01.01";
    core_remote_date = QDate::currentDate().toString(Qt::ISODate);
    compare_versions(core_local_date, core_remote_date);
    return;
  }
  core_local_date = core_version_file->readAll().trimmed();

  // read version from remote
  QNetworkAccessManager *m_netwManager = new QNetworkAccessManager(this);
  connect(m_netwManager, &QNetworkAccessManager::finished,
          [=](QNetworkReply *rep) {
            if (rep->error() == QNetworkReply::NoError) {
              QString replyStr = rep->readAll();
              if (rep->request().url().toString().contains("api.")) {
                QJsonDocument doc = QJsonDocument::fromJson(replyStr.toUtf8());
                if (doc.isNull() == false) {
                  QJsonObject obj = doc.object();
                  core_remote_date = obj.value("tag_name").toString().trimmed();
                }
              } else {
                core_remote_date = replyStr.trimmed();
              }

              if (!core_local_date.isNull() && !core_remote_date.isNull()) {
                compare_versions(core_local_date, core_remote_date);
              }
            } else {
              qWarning() << "update check failed";
            }
            rep->deleteLater();
            m_netwManager->deleteLater();
          });
  QUrl url(ENGINE_VERSION_URL);
  QNetworkRequest request(url);
  qWarning() << "checking for engine updates...";
  m_netwManager->get(request);
}

void Engine::compare_versions(QString date, QString n_date) {

  qWarning() << "installed version: " + date + " remote version: " + n_date;

  int year, month, day, n_year, n_month, n_day;

  year = QDate::fromString(date, Qt::ISODate).year();
  month = QDate::fromString(date, Qt::ISODate).month();
  day = QDate::fromString(date, Qt::ISODate).day();

  n_year = QDate::fromString(n_date, Qt::ISODate).year();
  n_month = QDate::fromString(n_date, Qt::ISODate).month();
  n_day = QDate::fromString(n_date, Qt::ISODate).day();

  if (n_year > year || n_month > month || n_day > day) {
    qWarning() << "engine update available( ver: " + n_date + ")";
    QMessageBox msgBox;
    msgBox.setText("" + QApplication::applicationName() +
                   " Engine update (<b>ver: " + n_date + "</b>) available!");
    msgBox.setIconPixmap(
        QPixmap(":/icons/information-line.png")
            .scaled(42, 42, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    msgBox.setInformativeText("You are having an outdated engine (<b>ver: " +
                              date + "</b>), please update to latest engine "
                                     "for better performance. Update now?");
    msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Ok);
    QPushButton *p = new QPushButton("Quit", nullptr);
    msgBox.addButton(p, QMessageBox::NoRole);
    int ret = msgBox.exec();
    switch (ret) {
    case QMessageBox::Ok:
      emit openSettingsAndClickDownload();
      break;
    case QMessageBox::Cancel:
      // check_engine_updates();
      break;
    default:
      qApp->quit();
      break;
    }
  }
}

void Engine::evoke_engine_check() {
  qWarning() << "looking for installed engine binary...";
  if (checkEngine() == false) {
    QMessageBox msgBox;
    msgBox.setText("" + QApplication::applicationName() +
                   " needs to download it's engine which is responsible for "
                   "finding media online");
    msgBox.setIconPixmap(
        QPixmap(":/icons/information-line.png")
            .scaled(42, 42, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    msgBox.setInformativeText("The " + QApplication::applicationName() +
                              " engine (1.4Mb & is based on youtube-dl with "
                              "some modifications), is missing, do you want to "
                              "download it now?");
    msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
    QPushButton *p = new QPushButton("Quit", nullptr);
    msgBox.addButton(p, QMessageBox::NoRole);
    msgBox.setDefaultButton(QMessageBox::Ok);

    switch (msgBox.exec()) {
    case QMessageBox::Ok:
      emit openSettingsAndClickDownload();
      break;
    case QMessageBox::Cancel:
      evoke_engine_check();
      break;
    default:
      qApp->quit();
      break;
    }
  }
}

void Engine::EngineVersionFromEngine(bool afterUpdate) {
  if (checkEngine() == false)
    return;
  QProcess *vprocess = new QProcess(this);
  connect(vprocess, &QProcess::readyRead, [=]() {
    m_engine_version = vprocess->readAll();
    if (afterUpdate) {
      qWarning() << "engine updated to version: " + m_engine_version;
    } else {
      qWarning() << "installed engine version: " + m_engine_version;
    }
  });
  connect(vprocess, static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(
                        &QProcess::finished),
          [=](int exitCode, QProcess::ExitStatus exitStatus) {
            Q_UNUSED(exitCode)
            if (exitStatus != QProcess::NormalExit) {
              qWarning() << "failed while getting engine version information";
            }
          });

  QString setting_path =
      QStandardPaths::writableLocation(QStandardPaths::DataLocation);
  qWarning() << "checking local engine version information";
  vprocess->start("python3", QStringList() << setting_path + "/core"
                                           << "--version");
}
