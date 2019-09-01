#ifndef VIDEOOPTION_H
#define VIDEOOPTION_H

#include <QWidget>
#include <QTextDocument>
#include <QProcess>
#include <QStandardPaths>
#include <QTextCodec>
#include <QDebug>
#include <QRadioButton>
#include <QScrollBar>
#include <QKeyEvent>
#include <QTimer>
#include <QEvent>

#include "store.h"

namespace Ui {
class VideoOption;
}

class VideoOption : public QWidget
{
    Q_OBJECT

public:
    explicit VideoOption(QWidget *parent = nullptr,store *store = nullptr,QString fifo = "");
    ~VideoOption();


signals:
    bool checkEngine();
    void downloadRequested(const QStringList currentTrackMeta,const QStringList downloadFormats);

public slots:
    void removeStyle();
    void setMeta(QString ids);
    void setMetaFromWeb(QVariant data);

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
    void resetVars();
    void LoadAvatar(const QUrl &avatarUrl);
    void toggleFullscreen();
    void deleteProcess(int code);

    void getUrlForFormatsAndDownload(QString audioFormat, QString videoFormat);
    void addToDownload(QString videoFormat, QString audioFormat);
private:
    Ui::VideoOption *ui;
    store *store_manager = nullptr;
    QProcess * ytdlProcess = nullptr;
    QList<QStringList> ytdlQueue ;
    QString currentUrl,currentTitle;
    QStringList resolution_List;
    QString audioCode,videoCode;
    QString videoUrl,audioUrl;
    QString used_fifo_file_path;
    QTimer *playerTimer = nullptr;
    QString task = "";
    QStringList currentTrackMeta;



};

#endif // VIDEOOPTION_H
