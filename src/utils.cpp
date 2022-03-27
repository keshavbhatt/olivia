#include "utils.h"

#include <QDateTime>
#include <QDesktopServices>
#include <QProcess>
#include <QRandomGenerator>
#include <QUrl>

utils::utils(QObject *parent) : QObject(parent) { setParent(parent); }

utils::~utils() { this->deleteLater(); }

// calculate dir size
quint64 utils::dir_size(const QString &directory) {
  quint64 sizex = 0;
  QFileInfo str_info(directory);
  if (str_info.isDir()) {
    QDir dir(directory);
    QFileInfoList list =
        dir.entryInfoList(QDir::Files | QDir::Dirs | QDir::Hidden |
                          QDir::NoSymLinks | QDir::NoDotAndDotDot);
    for (int i = 0; i < list.size(); ++i) {
      QFileInfo fileInfo = list.at(i);
      if (fileInfo.isDir()) {
        sizex += dir_size(fileInfo.absoluteFilePath());
      } else {
        sizex += fileInfo.size();
      }
    }
  }
  return sizex;
}

// get the size of cache folder in human readble format
QString utils::refreshCacheSize(const QString cache_dir) {
  qint64 cache_size = dir_size(cache_dir);
  QString cache_unit;
  if (cache_size > 1024 * 1024 * 1024) {
    cache_size = cache_size / (1024 * 1024 * 1024);
    cache_unit = " GB";
  }
  if (cache_size > 1024 * 1024) {
    cache_size = cache_size / (1024 * 1024);
    cache_unit = " MB";
  } else if (cache_size > 1024) {
    cache_size = cache_size / (1024);
    cache_unit = " kB";
  } else {
    cache_unit = " B";
  }
  return QString::number(cache_size) + cache_unit;
}

bool utils::delete_cache(const QString cache_dir) {
  bool deleted = QDir(cache_dir).removeRecursively();
  QDir(cache_dir).mkpath(cache_dir);
  return deleted;
}

// returns string with first letter capitalized
QString utils::toCamelCase(const QString &s) {
  QStringList parts = s.split(' ', QString::SkipEmptyParts);
  for (int i = 0; i < parts.size(); ++i)
    parts[i].replace(0, 1, parts[i][0].toUpper());
  return parts.join(" ");
}

QString utils::generateRandomId(int length) {

  QDateTime cd = QDateTime::currentDateTime();
  const QString possibleCharacters(
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789" +
      QString::number(cd.currentMSecsSinceEpoch())
          .remove(QRegExp("[^a-zA-Z\\d\\s]")));
  const int randomStringLength = length;
  QString randomString;
  for (int i = 0; i < randomStringLength; ++i) {
    int index =
        QRandomGenerator::global()->generate() % possibleCharacters.length();
    QChar nextChar = possibleCharacters.at(index);
    randomString.append(nextChar);
  }
  return randomString.trimmed().simplified().remove(" ");
}

void utils::desktopOpenUrl(const QString str) {
  QProcess *xdg_open = new QProcess(0);
  xdg_open->start("xdg-open", QStringList() << str);
  if (xdg_open->waitForStarted(1000) == false) {
    // try using QdesktopServices
    bool opened = QDesktopServices::openUrl(QUrl(str));
    if (opened == false) {
      qWarning() << "failed to open url" << str;
    }
  }
  connect(xdg_open,
          static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(
              &QProcess::finished),
          [xdg_open](int exitCode, QProcess::ExitStatus exitStatus) {
            Q_UNUSED(exitCode);
            Q_UNUSED(exitStatus);
            xdg_open->close();
            xdg_open->deleteLater();
          });
}

QString utils::EncodeXML(const QString &encodeMe) {

  QString temp;
  int length = encodeMe.size();
  for (int index = 0; index < length; index++) {
    QChar character(encodeMe.at(index));

    switch (character.unicode()) {
    case '&':
      temp += "&amp;";
      break;

    case '\'':
      temp += "&apos;";
      break;

    case '"':
      temp += "&quot;";
      break;

    case '<':
      temp += "&lt;";
      break;

    case '>':
      temp += "&gt;";
      break;

    default:
      temp += character;
      break;
    }
  }

  return temp;
}

QString utils::DecodeXML(const QString &decodeMe) {

  QString temp(decodeMe);

  temp.replace("&amp;", "&");
  temp.replace("&apos;", "'");
  temp.replace("&quot;", "\"");
  temp.replace("&lt;", "<");
  temp.replace("&gt;", ">");

  return temp;
}
