#ifndef UTILS_H
#define UTILS_H

#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QObject>
#include <QStandardPaths>

class utils : public QObject {
  Q_OBJECT

public:
  utils(QObject *parent = 0);
  virtual ~utils();
public slots:
  QString refreshCacheSize(const QString cache_dir);
  bool delete_cache(const QString cache_dir);
  static QString toCamelCase(const QString &s);
  static QString generateRandomId(int length);
  static void desktopOpenUrl(const QString str);
  static QString EncodeXML(const QString &encodeMe);
  static QString DecodeXML(const QString &decodeMe);
private slots:
  quint64 dir_size(const QString &directory);
};

#endif // UTILS_H
