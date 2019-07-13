#ifndef UTILS_H
#define UTILS_H

#include <QObject>
#include <QDebug>
#include <QFileInfo>
#include <QStandardPaths>
#include <QDir>

class utils : public QObject
{
    Q_OBJECT

public:
    utils(QObject* parent=0);
    virtual ~utils();
public slots:
    QString refreshCacheSize(const QString cache_dir);
    bool delete_cache(const QString cache_dir);
    QString toCamelCase(const QString &s);
private slots:
    quint64 dir_size(const QString &directory);

};

#endif // UTILS_H
