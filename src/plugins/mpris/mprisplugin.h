#ifndef MPRISPLUGIN_H
#define MPRISPLUGIN_H

#include <QObject>
#include <QDBusMessage>
#include <memory>
#include "radio.h"
#include <QDBusObjectPath>

class MprisPlugin : public QObject
{
    Q_OBJECT
public:
    explicit MprisPlugin(QObject *parent = 0);

    Q_PROPERTY(bool CanControl READ CanControl)
    bool CanControl() const;
    Q_PROPERTY(bool CanGoNext READ CanGoNext)
    bool CanGoNext() const;
    Q_PROPERTY(bool CanGoPrevious READ CanGoPrevious)
    bool CanGoPrevious() const;
    Q_PROPERTY(bool CanPause READ CanPause)
    bool CanPause() const;
    Q_PROPERTY(bool CanSeek READ CanSeek)
    bool CanSeek() const;
//    Q_PROPERTY(qlonglong Position READ Position)
//    qlonglong Position() const;

    Q_PROPERTY(QVariantMap Metadata READ Metadata)
    QVariantMap Metadata() const;

    Q_PROPERTY(QString PlaybackStatus READ PlaybackStatus)
    QString PlaybackStatus() const;

    Q_PROPERTY(bool CanQuit READ CanQuit)
    bool CanQuit() const;
    Q_PROPERTY(bool CanRaise READ CanRaise)
    bool CanRaise() const;
    Q_PROPERTY(QString DesktopEntry READ DesktopEntry)
    QString DesktopEntry() const;
    Q_PROPERTY(QString Identity READ Identity)
    QString Identity() const;

    Q_PROPERTY(double Volume READ volume WRITE setVolume)

    double volume() const;
    void setVolume(double value);

    qlonglong playerPosition;
    QString playerStatus;
    QVariantMap currentSongMeta;

    Q_PROPERTY(qlonglong Position READ position)
    qlonglong position() const;


signals:
   void PlaybackStatusChanged(const QString state);
   void currentSongChanged(const QVariantMap song);

   void Next();
   void Pause();
   void Play();
   void PlayPause();
   void Stop();
   void Raise();


public slots:
   void Quit();
   void SetPosition(const QDBusObjectPath &TrackId, qlonglong Position);
   void Seek(qlonglong Offset);

private:
   radio *radio_manager = nullptr;
};

#endif // MPRISPLUGIN_H
