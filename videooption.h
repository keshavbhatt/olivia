#ifndef VIDEOOPTION_H
#define VIDEOOPTION_H

#include <QWidget>
#include <QTextDocument>
#include <QProcess>
#include <QStandardPaths>
#include <QTextCodec>
#include <QDebug>

#include "store.h"

namespace Ui {
class VideoOption;
}

class VideoOption : public QWidget
{
    Q_OBJECT

public:
    explicit VideoOption(QWidget *parent = nullptr,store *store = nullptr);
    ~VideoOption();

signals:
    bool checkEngine();

public slots:
    void setMeta(QString ids);
    void removeStyle();

protected slots:
    void closeEvent(QCloseEvent *event);

private slots:

    void getVideoStream(QString ytIds, QString songId);
    void processYtdlQueue();
    void ytdlFinished(int code);
    void update_audio_video_code(bool checked);
    void getUrlForFormatsAndPLay(QString audioFormat, QString videoFormat);
    void getUrlProcessFinished(int code);
    void mergeAndPlay(QString videoUrlStr, QString audioUrlStr);
    void playerReadyRead();
    void playerFinished(int code);
private:
    Ui::VideoOption *ui;
    store *store_manager = nullptr;
    QProcess * ytdlProcess = nullptr;
    QList<QStringList> ytdlQueue ;
    QString currentUrl;
    QStringList resolution_List;
    QString audioCode,videoCode;
    QString videoUrl,audioUrl;



};

#endif // VIDEOOPTION_H
