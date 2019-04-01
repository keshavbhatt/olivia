#ifndef PAGINATOR_H
#define PAGINATOR_H

#include <QObject>
#include <QStandardPaths>
#include <QDir>
#include <QDebug>
#include <QFile>

class paginator: public QObject
{
    Q_OBJECT

public:
   explicit paginator(QObject *parent = 0);
    Q_INVOKABLE void save(QString pageType, QString dataType, QString query, QString data);
    Q_INVOKABLE bool isOffline(QString pageType, QString dataType, QString query);
    Q_INVOKABLE QString load(QString pageType, QString dataType, QString query);
    Q_INVOKABLE void deleteCache(QString pageType, QString dataType, QString query);
    Q_INVOKABLE QString getList(QString page, QString dataType);
    Q_INVOKABLE void clearRecentSearches();


signals:
    void reloadRequested(QString dataType,QString query);

private slots:
    QString createDir(QString name);
    QString getPath(QString pageType, QString dataType, QString query);
private:
    QString paginator_path;
};

#endif // PAGINATOR_H
