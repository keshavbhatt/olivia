#-------------------------------------------------
#
# Project created by QtCreator 2018-12-24T16:12:36
#
#-------------------------------------------------

QT       += core gui webkit webkitwidgets sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = olivia
TEMPLATE = app
INSTALLS += target
target.path = /usr/bin

desktop.path = /usr/share/applications/
desktop.files = olivia.desktop

INSTALLS += desktop

CONFIG += c++11


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
        mainwindow.cpp \
    DarkStyle.cpp \
    cookiejar.cpp \
    elidedlabel.cpp \
    nowplaying.cpp \
    store.cpp \
    radio.cpp \
    onlinesearchsuggestion.cpp \
    seekslider.cpp \
    volumeslider.cpp

HEADERS  += mainwindow.h \
    DarkStyle.h \
    cookiejar.h \
    elidedlabel.h \
    nowplaying.h \
    store.h \
    radio.h \
    onlinesearchsuggestion.h \
    seekslider.h \
    volumeslider.h

FORMS    += mainwindow.ui \
    track.ui \
    settings.ui \
    minimode.ui

RESOURCES += \
    darkstyle.qrc \
    icons.qrc \
    web.qrc
