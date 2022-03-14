#ifndef RADIO_H
#define RADIO_H

#include <QDebug>
#include <QEventLoop>
#include <QLabel>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QObject>
#include <QProcess>
#include <QStandardPaths>
#include <QTextBrowser>
#include <QTimer>

#include "settings.h"

class radio : public QObject {
  Q_OBJECT
  Q_PROPERTY(int volume_ READ volume_ WRITE setVolume NOTIFY volumeChanged)
public:
  explicit radio(QObject *parent = 0, int volumeValue = 0,
                 bool saveTracksAfterBuffer = false);
  QString radioState;
  int playerPosition;
  int playerDuration;
  int volume, tempVol;
  bool saveTracksAfterBuffer;
  QTimer *radioPlaybackTimer = nullptr;
  QString used_fifo_file_path;
  bool crossFadeEnabled = false;
  bool fading = false;
  bool fadequick = false;

  int volume_() const;
  void setVolume(int val);

signals:
  void volumeChanged(const int);
  void radioStatus(QString radioState);
  void radioPosition(int);
  void radioDuration(int);
  //    void radioEOF(QString);
  void demuxer_cache_duration_changed(double, double);
  void saveTrack(QString format);
  void icy_cover_changed(QPixmap pix);
  void icy_title_changed(QString title);
  void radioProcessReady();

  void getTrackInfo();
  void showToast(QString msg);
  void fadeOutVolume();
  void fadeInVolume();

public slots:
  void playRadio(bool saveTracksAfterBuffer, QUrl url);
  void pauseRadio();
  void resumeRadio();
  void changeVolume(int volume);
  void radioSeek(int pos);
  void loadMedia(QUrl url);
  void quitRadio();
  void deleteProcess(int code);
  void killRadioProcess();
  void stop();
  void startRadioProcess(bool saveTracksAfterBufferMode, QString urlString,
                         bool calledByCloseEvent);
  void quick_seek(bool positive);
private slots:
  void radioReadyRead();
  void radioFinished(int code);

  void LoadAvatar(const QUrl &avatarUrl) {
    QNetworkAccessManager manager;
    QEventLoop loop;
    QNetworkReply *reply = manager.get(QNetworkRequest(avatarUrl));
    connect(reply, &QNetworkReply::finished, &loop, [&reply, this, &loop]() {
      if (reply->error() == QNetworkReply::NoError) {
        QByteArray jpegData = reply->readAll();
        QPixmap pixmap(jpegData);
        if (!pixmap.isNull()) {
          emit icy_cover_changed(pixmap);
        }
      } else {
        qDebug() << reply->errorString();
      }
      loop.quit();
    });
    loop.exec();
  }

  void startPlayingRadio(bool saveTracksAfterBufferMode, QUrl url);

private:
  QProcess *radioProcess = nullptr;
  QString tmp_path, setting_path;

  QString streamUrl;
  QString state_line;
  settings *settUtils;
};

#endif // RADIO_H
