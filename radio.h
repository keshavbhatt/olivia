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
    explicit radio(QObject *parent = 0);
    QString radioState;

signals:
    void radioStatus(QString radioState);
    void radioPosition(int);
    void radioDuration(int);

public slots:
    void playRadio(QUrl url,int volume);
    void pauseRadio();
    void resumeRadio();
    void killRadio();
    void quitRadio();
    void changeVolume(int volume);
    void radioSeek(int pos);
private slots:
    void radioReadyRead();

    void radioFinished(int code);
private:
    QProcess *radioProcess = nullptr;
    QString setting_path;
    QTimer *radioPlaybackTimer = nullptr;
};

#endif // RADIO_H
