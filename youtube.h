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

class Youtube : public QObject
{
    Q_OBJECT
public:
    explicit Youtube(QObject *parent = 0,QWebView *webview = nullptr);
    Q_INVOKABLE QString getCurrentCountry();
    Q_INVOKABLE void saveGeo(QString country);
    Q_INVOKABLE void flat_playlist(QVariant playlist_id);


signals:
    void setCountry(QString country);
private:
    QSettings settings;
    QString   setting_path;
    QWebView *view = nullptr;

public slots:

private slots:
    void flatterFinished(int exitCode);
};

#endif // YOUTUBE_H
