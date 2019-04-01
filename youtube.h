#ifndef YOUTUBE_H
#define YOUTUBE_H

#include <QObject>
#include <QDebug>
#include <QSettings>
#include <QDir>
#include <QFile>
#include <QStandardPaths>


class Youtube : public QObject
{
    Q_OBJECT
public:
    explicit Youtube(QObject *parent = 0);
    Q_INVOKABLE QString getCurrentCountry();
    Q_INVOKABLE void saveGeo(QString country);

signals:
    void setCountry(QString country);
private:
    QSettings settings;
    QString   setting_path;

public slots:
private slots:

};

#endif // YOUTUBE_H
