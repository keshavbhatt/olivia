#include "mprisplayeradapter.h"
#include <QMetaObject>
#include <QByteArray>
#include <QList>
#include <QMap>
#include <QString>
#include <QStringList>
#include <QVariant>

/*
 * Implementation of adaptor class MprisPlayerAdapter
 */

MprisPlayerAdapter::MprisPlayerAdapter(QObject *parent)
    : QDBusAbstractAdaptor(parent)
{
    // constructor
    setAutoRelaySignals(true);
}

MprisPlayerAdapter::~MprisPlayerAdapter()
{
    // destructor
}

bool MprisPlayerAdapter::canControl() const
{
    // get the value of property CanControl
    return qvariant_cast< bool >(parent()->property("CanControl"));
}

bool MprisPlayerAdapter::canGoNext() const
{
    // get the value of property CanGoNext
    return qvariant_cast< bool >(parent()->property("CanGoNext"));
}

bool MprisPlayerAdapter::canGoPrevious() const
{
    // get the value of property CanGoPrevious
    return qvariant_cast< bool >(parent()->property("CanGoPrevious"));
}

bool MprisPlayerAdapter::canPause() const
{
    // get the value of property CanPause
    return qvariant_cast< bool >(parent()->property("CanPause"));
}

bool MprisPlayerAdapter::canSeek() const
{
    // get the value of property CanSeek
    return qvariant_cast< bool >(parent()->property("CanSeek"));
}

QVariantMap MprisPlayerAdapter::metadata() const
{
    // get the value of property Metadata
    return qvariant_cast< QVariantMap >(parent()->property("Metadata"));
}

QString MprisPlayerAdapter::playbackStatus() const
{
    // get the value of property PlaybackStatus
    return qvariant_cast< QString >(parent()->property("PlaybackStatus"));
}

qlonglong MprisPlayerAdapter::position() const
{
    // get the value of property Position
    return qvariant_cast< qlonglong >(parent()->property("Position"));
}

void MprisPlayerAdapter::Next()
{
    // handle method call org.mpris.MediaPlayer2.Player.Next
    QMetaObject::invokeMethod(parent(), "Next");
}

void MprisPlayerAdapter::Pause()
{
    // handle method call org.mpris.MediaPlayer2.Player.Pause
    QMetaObject::invokeMethod(parent(), "Pause");
}

void MprisPlayerAdapter::Play()
{
    // handle method call org.mpris.MediaPlayer2.Player.Play
    QMetaObject::invokeMethod(parent(), "Play");
}

void MprisPlayerAdapter::PlayPause()
{
    // handle method call org.mpris.MediaPlayer2.Player.PlayPause
    QMetaObject::invokeMethod(parent(), "PlayPause");
}

void MprisPlayerAdapter::Stop()
{
    // handle method call org.mpris.MediaPlayer2.Player.Stop
    QMetaObject::invokeMethod(parent(), "Stop");
}

