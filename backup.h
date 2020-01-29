#ifndef BACKUP_H
#define BACKUP_H

#include <QWidget>
#include <QSettings>

namespace Ui {
class Backup;
}

class Backup : public QWidget
{
    Q_OBJECT

public:
    explicit Backup(QWidget *parent = nullptr,QString settings_path="",QSettings *settingsObj=nullptr);
    ~Backup();

signals:
    void app_restart_required();

public slots:
    void check_last_backup();
    void fixTheme();

private slots:
    void on_backup_clicked();
    void on_restore_clicked();
    bool backup_path_valid();
    void on_change_path_clicked();

    void tarReadyRead();
    void tarFinished(int exitCode);

    void untarFinished(int exitCode);
    void untarReadyRead();
    void start_restore();
private:
    Ui::Backup *ui;
    QString settings_path,audio_path,video_path;
    QString lastBackTime;
    QSettings *settingsObj = nullptr;
};

#endif // BACKUP_H
