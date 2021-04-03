#include "mprisplugin.h"
#include <QDBusConnection>
#include "mprisadapter.h"
#include "mprisplayeradapter.h"
#include <QApplication>
#include <QWidget>

static const QString PLAYER_SERVICE_NAME = "org.mpris.MediaPlayer2.olivia";
static const QString PLAYER_OBJECT_PATH = "/org/mpris/MediaPlayer2";

static bool DBUS_NOTIFY_PROPERTIES_CHANGED(QString iface, QVariantMap changed, QStringList invalidated = QStringList()) {
    auto dbus_msg = QDBusMessage::createSignal(PLAYER_OBJECT_PATH,
                                               "org.freedesktop.DBus.Properties",
                                               "PropertiesChanged");
    dbus_msg << iface << changed << invalidated;
    return QDBusConnection::sessionBus().send(dbus_msg);
}

MprisPlugin::MprisPlugin(QObject *parent) :
    QObject(parent)
{

    radio_manager = parent->findChild<radio*>("radio_manager");
    QDBusConnection con = QDBusConnection::sessionBus();
    con.registerService(PLAYER_SERVICE_NAME);
    new MprisAdapter(this);
    new MprisPlayerAdapter(this);
    con.registerObject(PLAYER_OBJECT_PATH, this);

    connect(this,&MprisPlugin::PlaybackStatusChanged, [=] (const QString state) {
        QVariantMap changedMap;
        changedMap.insert("PlaybackStatus", state.trimmed());
        DBUS_NOTIFY_PROPERTIES_CHANGED("org.mpris.MediaPlayer2.Player", changedMap);
    });

    connect(this, &MprisPlugin::currentSongChanged, [=] (const QVariantMap song) {
        QVariantMap changedMap;
        currentSongMeta = song;
        changedMap.insert("Metadata", song);
        DBUS_NOTIFY_PROPERTIES_CHANGED("org.mpris.MediaPlayer2.Player", changedMap);
    });
}

bool MprisPlugin::CanControl() const {
    return true;
}
bool MprisPlugin::CanGoNext() const {
    return true;
}
bool MprisPlugin::CanGoPrevious() const {
    return true;
}
bool MprisPlugin::CanPause() const {
    return true;
}
bool MprisPlugin::CanSeek() const {
    return true;
}
//qlonglong MprisPlugin::Position() const {
//    return this->playerPosition;
//}

bool MprisPlugin::CanQuit() const {
    return true;
}
bool MprisPlugin::CanRaise() const {
    return true;
}
QString MprisPlugin::DesktopEntry() const {
    return "olivia";
}
QString MprisPlugin::Identity() const {
    return "olivia";
}

QVariantMap MprisPlugin::Metadata() const {
    return currentSongMeta;
}

QString MprisPlugin::PlaybackStatus() const {
    if(playerStatus.isEmpty())
        return "Stopped";
    else
        return playerStatus;
}

void MprisPlugin::Quit() {
    qApp->quit();
}


double MprisPlugin::volume() const {
    // dummy method - can't get the volume
    return 0.5;
}

void MprisPlugin::setVolume(double value) {
    qDebug()<<value;
   radio_manager->changeVolume(value * 100);
}


qlonglong MprisPlugin::position() const {
//    const PTrack& track = player->getTrack();
//    return track.isStream ? 0 : track.currSec * 1000000;
    return this->playerPosition;
}

void MprisPlugin::SetPosition(
        const QDBusObjectPath &TrackId, qlonglong Position) {
   // if(trackID != TrackId) return;
    Q_UNUSED(TrackId);
    radio_manager->radioSeek(static_cast<int>(Position / 1000000));
}

void MprisPlugin::Seek(qlonglong Offset) {
   radio_manager->radioSeek(static_cast<int>(Offset / 1000000));
}
