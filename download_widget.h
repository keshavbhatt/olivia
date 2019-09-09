#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QProcess>
#include <QFile>
#include <QDir>
#include <QUrl>
#include <QListWidgetItem>
#include <QSettings>
#include "ui_downloaditem.h"
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QFileDialog>

namespace Ui {
class Widget;
}

class Widget : public QWidget
{
    Q_OBJECT

public:
    explicit Widget(QWidget *parent = 0);
    ~Widget();

public slots:

    void startWget(QString url, QString downloadLocation, QStringList formats);

    void insertDownloadWidget(QString, QString , QString ,bool , QStringList);

    void downloadProcessReadyRead();

    void downloadProcessFinished(int);

    void downloadProcessStateChanged(QProcess::ProcessState);

    void on_removeSelected_clicked();

    void on_downloadList_currentRowChanged(int currentRow);

    void on_startSelected_clicked();

    void on_pauseSelected_clicked();

    void update_ui_btns(int);


private:
    Ui::Widget *ui;

    Ui::downloadItem downloadItemUi;

    QList<QProcess*> downloadList;

    QString downloadArgs;

    QStringList color;

    QFile dbFile;

    QFile historyFile;

    QString setting_path;

 //   QSettings settings;



public:
    QString downloadLocation;
    QString trackId,trackTitle;

private slots:
    void on_startAll_clicked();
    void on_pauseAll_clicked();
    void on_removeAll_clicked();
    void udpate_stats();
    void save_download_item(int);
    void remove_download_item(QString);
    QString returnPath(QString);
    QString make_database_file(QUrl);
    bool check_if_url_already_exist(QString);
    void update_db_file(QStringList);
    void read_history();
    void add_history_items_to_ui(QStringList);
    void start_process(QStringList);
    void hide_already_inList();
    void on_downloadList_itemDoubleClicked(QListWidgetItem *item);
};

#endif // WIDGET_H
