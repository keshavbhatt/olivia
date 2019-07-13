#include "utils.h"

utils::utils(QObject *parent) : QObject(parent)
{
    setParent(parent);
}

utils::~utils()
{
    this->deleteLater();
}

//calculate dir size
quint64 utils::dir_size(const QString & directory)
{
    quint64 sizex = 0;
    QFileInfo str_info(directory);
    if (str_info.isDir())
    {
        QDir dir(directory);
        QFileInfoList list = dir.entryInfoList(QDir::Files | QDir::Dirs |  QDir::Hidden | QDir::NoSymLinks | QDir::NoDotAndDotDot);
        for (int i = 0; i < list.size(); ++i)
        {
            QFileInfo fileInfo = list.at(i);
            if(fileInfo.isDir())
            {
                sizex += dir_size(fileInfo.absoluteFilePath());
            }
            else{
                sizex += fileInfo.size();
            }
        }
    }
    return sizex;
}

//get the size of cache folder in human readble format
QString utils::refreshCacheSize(const QString cache_dir)
{
    qint64 cache_size = dir_size(cache_dir);
    QString cache_unit;
    if(cache_size > 1024*1024*1024)
    {
        cache_size = cache_size/(1024*1024*1024);
        cache_unit = " GB";
    }
    if(cache_size > 1024*1024)
    {
        cache_size = cache_size/(1024*1024);
        cache_unit = " MB";
    }
    else if(cache_size > 1024)
    {
        cache_size = cache_size/(1024);
        cache_unit = " kB";
    }
    else
    {
        cache_unit = " B";
    }
    return QString::number(cache_size) + cache_unit;
}

bool utils::delete_cache(const QString cache_dir){
    bool deleted = QDir(cache_dir).removeRecursively();
    QDir(cache_dir).mkpath(cache_dir);
    return deleted;
}

//returns string with first letter capitalized
QString utils::toCamelCase(const QString& s)
{
    QStringList parts = s.split(' ', QString::SkipEmptyParts);
    for (int i = 0; i < parts.size(); ++i)
        parts[i].replace(0, 1, parts[i][0].toUpper());
    return parts.join(" ");
}

