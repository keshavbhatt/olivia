#ifndef SOUNDCLOUD_H
#define SOUNDCLOUD_H

#include <QObject>
#include <QtNetwork>

class SoundCloud : public QObject
{
    Q_OBJECT

public:
    explicit SoundCloud(QObject *parent = nullptr);
    QString cid;
private:
    QString home = "https://soundcloud.com/charts";
    QStringList jsList;

    QString cidjs;
    int jsId = 0;
signals:

public slots:
    bool cidEmpty();
    void getHome();
private slots:
    void getJs(const QString res);
    void getCid();


    void extract_cid(QString rep);
};

#endif // SOUNDCLOUD_H
