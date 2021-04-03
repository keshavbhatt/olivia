#include "mprisadapter.h"
#include <QtCore/QMetaObject>
#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVariant>

/*
 * Implementation of adaptor class MprisAdapter
 */

MprisAdapter::MprisAdapter(QObject *parent)
    : QDBusAbstractAdaptor(parent)
{
    // constructor
    setAutoRelaySignals(true);
}

MprisAdapter::~MprisAdapter()
{
    // destructor
}

bool MprisAdapter::canQuit() const
{
    // get the value of property CanQuit
    return qvariant_cast< bool >(parent()->property("CanQuit"));
}

bool MprisAdapter::canRaise() const
{
    // get the value of property CanRaise
    return qvariant_cast< bool >(parent()->property("CanRaise"));
}

QString MprisAdapter::desktopEntry() const
{
    // get the value of property DesktopEntry
    return qvariant_cast< QString >(parent()->property("DesktopEntry"));
}

QString MprisAdapter::identity() const
{
    // get the value of property Identity
    return qvariant_cast< QString >(parent()->property("Identity"));
}

void MprisAdapter::Quit()
{
    // handle method call org.mpris.MediaPlayer2.Quit
    QMetaObject::invokeMethod(parent(), "Quit");
}

void MprisAdapter::Raise()
{
    // handle method call org.mpris.MediaPlayer2.Raise
    QMetaObject::invokeMethod(parent(), "Raise");
}

