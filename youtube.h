#ifndef YOUTUBE_H
#define YOUTUBE_H

#include <QObject>
#include <QDebug>
#include <QSettings>
#include <QDir>
#include <QFile>
#include <QStandardPaths>

#include <QWebView>
#include <QWebPage>
#include <QWebFrame>

#include "paginator.h"

class Youtube : public QObject
{
    Q_OBJECT
public:
    explicit Youtube(QObject *parent = 0,QWebView *webview = nullptr, paginator *pagination_manager = nullptr);
    Q_INVOKABLE QString getCurrentCountry();
    Q_INVOKABLE void saveGeo(QString country);
    Q_INVOKABLE void flat_playlist(QVariant playlist_id);


signals:
    void setCountry(QString country);
private:
    QSettings settings;
    QString   setting_path;
    QWebView *view = nullptr;
    paginator *pagination_manager = nullptr;

public slots:

private slots:
    void flatterFinished(int exitCode);
    void processPlaylistData(QString html);
};

#endif // YOUTUBE_H
