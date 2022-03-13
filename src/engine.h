#ifndef ENGINE_H
#define ENGINE_H

#include <QApplication>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>
#include <QObject>
#include <QProcess>
#include <QPushButton>
#include <QSettings>
#include <QStandardPaths>
#include <QtNetwork>

#include "utils.h"

class Engine : public QObject {
  Q_OBJECT
public:
  explicit Engine(QObject *parent = nullptr);

signals:
  void engineStatus(QString status);

  void engineCacheCleared();

  void errorMessage(QString errorMessage);

  void openSettingsAndClickDownload();

  void engineDownloadFailed(QString errorMessage);

  void engineDownloadSucceeded();

  void showSettingsTab(bool scrollToEngine);

public slots:
  void clearEngineCache();

  void download_engine_clicked();

  static QString enginePath();

  bool checkEngine();
private slots:

  void slot_netwManagerFinished(QNetworkReply *reply);

  void get_engine_version_info();

  void check_engine_updates();

  void compare_versions(QString date, QString n_date);

  void evoke_engine_check();

  void EngineVersionFromEngine(bool afterUpdate = false);

private:
  QFile *core_file = nullptr;

  QString core_local_date, core_remote_date;

  QString m_engine_version;
};

#endif // ENGINE_H
