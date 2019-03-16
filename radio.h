#ifndef RADIO_H
#define RADIO_H

#include <QObject>
#include <QDebug>
#include <QProcess>
#include <QTextBrowser>
#include <QStandardPaths>
#include <QTimer>

class radio : public QObject
{
    Q_OBJECT
public:
    explicit radio(QObject *parent = 0, int volumeValue = 0, bool saveTracksAfterBuffer= false);
    QString radioState;
    int volume ;
    bool saveTracksAfterBuffer;


signals:
    void radioStatus(QString radioState);
    void radioPosition(int);
    void radioDuration(int);
//    void radioEOF(QString);
    void demuxer_cache_duration_changed(double,double);
    void saveTrack(QString format);

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
private slots:
    void radioReadyRead();
    void radioFinished(int code);

    void startRadioProcess(bool saveTracksAfterBufferMode, QString);


private:
    QProcess *radioProcess = nullptr;
    QString setting_path;
    QTimer *radioPlaybackTimer = nullptr;
    QString streamUrl;


};

#endif // RADIO_H
