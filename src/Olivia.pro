#-------------------------------------------------
#
# Project created by QtCreator 2018-12-24T16:12:36
#
#-------------------------------------------------

QT       += core gui webkit webkitwidgets sql xml

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TEMPLATE = app

# Set program name
TARGET = olivia

# Set program version
VERSION = 0.0.1
DEFINES += VERSIONSTR=\\\"$${VERSION}\\\"


CONFIG(release, debug|release):DEFINES += QT_NO_DEBUG_OUTPUT

CONFIG += c++11

LIBS += -ltag


# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.

DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


SOURCES += main.cpp\
    analytics.cpp \
    backup.cpp \
    download_widget.cpp \
    equalizer.cpp \
        mainwindow.cpp \
    DarkStyle.cpp \
    cookiejar.cpp \
    elidedlabel.cpp \
    nowplaying.cpp \
    other/connect.cpp \
    request.cpp \
    scrolltext.cpp \
    similartracks.cpp \
    soundcloud.cpp \
    store.cpp \
    radio.cpp \
    onlinesearchsuggestion.cpp \
    seekslider.cpp \
    trackproperties.cpp \
    videooption.cpp \
    volumeslider.cpp \
    settings.cpp \
    paginator.cpp \
    waitingspinnerwidget.cpp \
    youtube.cpp \
    lyrics.cpp \
    controlbutton.cpp \
    utils.cpp

HEADERS  += mainwindow.h \
    DarkStyle.h \
    analytics.h \
    backup.h \
    cookiejar.h \
    download_widget.h \
    elidedlabel.h \
    equalizer.h \
    notificationpopup.h \
    nowplaying.h \
    other/connect.h \
    request.h \
    scrolltext.h \
    similartracks.h \
    soundcloud.h \
    store.h \
    radio.h \
    onlinesearchsuggestion.h \
    seekslider.h \
    stringchangewatcher.h \
    trackproperties.h \
    videooption.h \
    volumeslider.h \
    settings.h \
    paginator.h \
    waitingspinnerwidget.h \
    youtube.h \
    lyrics.h \
    controlbutton.h \
    manifest_resolver.h \
    utils.h

FORMS    += mainwindow.ui \
    backup.ui \
    download_widget.ui \
    downloaditem.ui \
    equalizer.ui \
    other/connect.ui \
    smart_mode.ui \
    toast.ui \
    track.ui \
    settings.ui \
    minimode.ui \
    lyrics.ui \
    lyricitem.ui \
    trackproperties.ui \
    videooption.ui

RESOURCES += \
    app-res.qrc \
    darkstyle.qrc \
    icons.qrc \
    web.qrc


!win32:!macx {
    QT += dbus

    SOURCES +=  plugins/mpris/mprisplayeradapter.cpp \
                plugins/mpris/mprisadapter.cpp \
                plugins/mpris/mprisplugin.cpp

    HEADERS +=  plugins/mpris/mprisplayeradapter.h \
                plugins/mpris/mprisadapter.h \
                plugins/mpris/mprisplugin.h

    OTHER_FILES += \
        plugins/org.mpris.MediaPlayer2.xml \
        plugins/org.mpris.MediaPlayer2.Player.xml \
        plugins/org.freedesktop.DBus.Properties.xml
}
    
# Deployment
isEmpty(PREFIX){
 PREFIX = /usr
}

BINDIR  = $$PREFIX/bin
DATADIR = $$PREFIX/share

target.path = $$BINDIR

icon.files = icons/olivia.png
icon.path = $$DATADIR/icons/hicolor/512x512/apps/

desktop.files = olivia.desktop
desktop.path = $$DATADIR/applications/

INSTALLS += target icon desktop
