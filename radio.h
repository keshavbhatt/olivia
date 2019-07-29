#ifndef RADIO_H
#define RADIO_H

#include <QObject>
#include <QDebug>
#include <QProcess>
#include <QTextBrowser>
#include <QStandardPaths>
#include <QTimer>
#include <QLabel>
#include <QEventLoop>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

#include "settings.h"


class radio : public QObject
{
    Q_OBJECT
public:
    explicit radio(QObject *parent = 0, int volumeValue = 0, bool saveTracksAfterBuffer= false);
    QString radioState;
    int volume ;
    bool saveTracksAfterBuffer;
    QTimer *radioPlaybackTimer = nullptr;
    QString used_fifo_file_path;

signals:
    void radioStatus(QString radioState);
    void radioPosition(int);
    void radioDuration(int);
//    void radioEOF(QString);
    void demuxer_cache_duration_changed(double,double);
    void saveTrack(QString format);
    void icy_cover_changed(QPixmap pix);
    void radioProcessReady();

public slots:
    void playRadio(bool saveTracksAfterBuffer, QUrl url );
    void pauseRadio();
    void resumeRadio();
    void changeVolume(int volume);
    void radioSeek(int pos);
    void loadMedia(QUrl url);
    void quitRadio();
    void deleteProcess(int code);
    void killRadioProcess();
    void stop();
    void startRadioProcess(bool saveTracksAfterBufferMode, QString urlString, bool calledByCloseEvent);

private slots:
    void radioReadyRead();
    void radioFinished(int code);


    void LoadAvatar(const QUrl &avatarUrl)
    {

       QNetworkAccessManager manager;
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
                emit icy_cover_changed(pixmap);
//                lable.setPixmap(pixmap);
            }
        }else{
            qDebug()<<reply->errorString();
        }
        loop.quit();
      });

      loop.exec();
    }


private:
    QProcess *radioProcess = nullptr;
    QString tmp_path,setting_path;

    QString streamUrl;
    QString state_line;
    settings *settUtils;



};

#endif // RADIO_H
