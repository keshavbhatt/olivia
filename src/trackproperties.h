#ifndef TRACKPROPERTIES_H
#define TRACKPROPERTIES_H

#include <QWidget>
#include <QDebug>
#include <QDir>
#include <QDesktopServices>
#include <QProcess>

namespace Ui {
class TrackProperties;
}

class TrackProperties : public QWidget
{
    Q_OBJECT

public:
    explicit TrackProperties(QWidget *parent = nullptr, QString setting_path = "");
    ~TrackProperties();

public slots:
    void setValues(QStringList meta);
private slots:
    void setWidgetTheme(QString dominantColor);
    void on_showInFiles_clicked();

    void on_copy_clicked();

    void on_deleteTrack_clicked();
    void on_copy_with_tags_clicked();

    QString writeTags();
    void convertToMpeg(QString path);
    void conversion_Finished(int exitCode, QProcess::ExitStatus status);
signals:
    void delete_song_cache(QString trackId);

protected slots:
    void closeEvent(QCloseEvent *event);
private:
    Ui::TrackProperties *ui;
    QString setting_path,trackId;
    QStringList meta;
};

#endif // TRACKPROPERTIES_H
