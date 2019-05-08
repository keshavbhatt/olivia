#ifndef LYRICS_H
#define LYRICS_H

#include <QWidget>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QNetworkDiskCache>
#include <QStandardPaths>
#include <QTextDocument>

#include "ui_lyricitem.h"

namespace Ui {
class Lyrics;
}

class Lyrics : public QWidget
{
    Q_OBJECT

public:
    explicit Lyrics(QWidget *parent = 0);
    ~Lyrics();

public slots:
   void setCustomStyle(QString searchStyle , QString listStyle , QString windowStyle);

   void setQueryString(QString query);
protected slots:
   void resizeEvent(QResizeEvent *event);

   void hideEvent(QHideEvent *event);
   void showEvent(QShowEvent *event);
private slots:

    void _networkManagerFinished(QNetworkReply *reply);

    void LoadCover(const QUrl &avatarUrl,QLabel &lable)
    {
       QString setting_path =  QStandardPaths::writableLocation(QStandardPaths::DataLocation);

       QNetworkAccessManager manager;
       manager.setParent(this);
       QNetworkDiskCache* diskCache = new QNetworkDiskCache(this);
       diskCache->setCacheDirectory(setting_path);
       manager.setCache(diskCache);

       QEventLoop loop;
       loop.setParent(this);
       QNetworkReply *reply = manager.get(QNetworkRequest(avatarUrl));
       QObject::connect(reply, &QNetworkReply::finished, &loop, [&reply, this,&loop ,&lable](){
        if (reply->error() == QNetworkReply::NoError)
        {
            QByteArray jpegData = reply->readAll();
            QPixmap pixmap;
            pixmap.loadFromData(jpegData);
            if (!pixmap.isNull())
            {
                lable.setPixmap(pixmap);
            }
        }else{
           // qDebug()<<reply->errorString();
        }
        reply->deleteLater();
        loop.quit();
      });
        diskCache->deleteLater();
        manager.deleteLater();
      loop.exec();
    }
    void on_query_returnPressed();

    void showLyrics();
    void showLyricsWidget(QString data);
    void hideLyrics();
    void on_close_clicked();

    void hideResult();
    void setLyricsTitle(QString title);

    void on_lyrics_textChanged();

    void on_copy_clicked();

    //return pain text version of text which contains html symbolic notations
    QString htmlToPlainText(QString html){
        QTextDocument text;
        text.setHtml(html);
        return text.toPlainText();
    }

private:
    Ui::Lyrics *ui;
    Ui::lyricItem lyricItem_Ui;
    QNetworkAccessManager *_networkManager;



};

#endif // LYRICS_H
