#include "backup.h"
#include "ui_backup.h"

#include <QProcess>
#include <QDir>
#include <QMessageBox>
#include <QDateTime>
#include <QFileDialog>
#include <QDebug>
#include <QStandardPaths>

QString debugger_name = "BACKUP_RESTORE:";

Backup::Backup(QWidget *parent, QString settings_path, QSettings *settingsObj) :
    QWidget(parent),
    ui(new Ui::Backup)
{
    ui->setupUi(this);

    ui->status->setElideMode(Qt::ElideLeft);
    ui->status_restore->setElideMode(Qt::ElideLeft);

    this->settings_path = settings_path;
    this->settingsObj = settingsObj;

    //pattern for exclusion in tar backup process
    audio_path = "**downloadedTracks/*";
    video_path = "**downloadedVideos/*";


    if(settingsObj->value("last_backup").isValid()){
        lastBackTime = settingsObj->value("last_backup").toString();
    }

    //setup initial backup path
    if(settingsObj->value("backup_path").isValid()){
        ui->backup_path->setText(settingsObj->value("backup_path").toString());
    }else{
        QString path= QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
        if(!path.endsWith('/')){
            path = path+"/";
        }
        ui->backup_path->setText(path);
    }

    //check last backup
    check_last_backup();
}

void Backup::fixTheme(){
    foreach(QLabel *label,this->findChildren<QLabel*>()){
        label->setStyleSheet("");
    }
}

void Backup::check_last_backup(){
    QString backupFilePath = ui->backup_path->text()+"Olivia_backup_"+lastBackTime+".tar";
    if(QFileInfo(backupFilePath).exists()){
        ui->last->setText("Last backup: "+lastBackTime);
    }else {
        ui->last->setText("Last backup: ");
    }
}

Backup::~Backup()
{
    delete ui;
}

void Backup::on_backup_clicked()
{
    if(backup_path_valid()){
        bool audio = ui->audio_checkBox->isChecked();
        bool video = ui->video_checkBox->isChecked();

        QStringList args;

        lastBackTime = QDateTime::currentDateTime().toLocalTime().toString();
        args<<"-cvf"<<ui->backup_path->text().trimmed()+"Olivia_backup_"+lastBackTime+".tar";
        if(!audio){
            args<<"--exclude="+audio_path;
        }
        if(!video){
            args<<"--exclude="+video_path;
        }

        args<<"-C"<<settings_path<<".";

        qDebug()<<debugger_name<<args;

        QProcess *tar = new QProcess(this);
        tar->setObjectName("tar");
        tar->setProgram("tar");
        tar->setArguments(args);
        connect(tar,SIGNAL(finished(int)),this,SLOT(tarFinished(int)));
        connect(tar,SIGNAL(readyRead()),this,SLOT(tarReadyRead()));
        tar->start();
        ui->backup->setEnabled(false);
    }else{
        QMessageBox msgBox;
        msgBox.setText("Please provide a valid backup path.");
              msgBox.setIconPixmap(QPixmap(":/icons/sidebar/info.png").scaled(42,42,Qt::KeepAspectRatio,Qt::SmoothTransformation));
        msgBox.setStandardButtons(QMessageBox::Ok );
        msgBox.setDefaultButton(QMessageBox::Ok);
        msgBox.exec();
        return;
    }
}

void Backup::tarFinished(int exitCode){
    ui->backup->setEnabled(true);
    qDebug()<<debugger_name<<"tar exitied with"<<exitCode;
    if(exitCode==0){
        ui->status->setText("Done.");
        ui->last->setText("Last backup: "+lastBackTime);
        settingsObj->setValue("last_backup",lastBackTime);
    }else{
        lastBackTime = "";
         ui->status->setText("Error occured.");
        QMessageBox msgBox;
        msgBox.setText("An error occured while backing up data. Please report this to developer.");
              msgBox.setIconPixmap(QPixmap(":/icons/sidebar/info.png").scaled(42,42,Qt::KeepAspectRatio,Qt::SmoothTransformation));
        msgBox.setStandardButtons(QMessageBox::Ok );
        msgBox.setDefaultButton(QMessageBox::Ok);
        msgBox.exec();
        return;
    }
}

void Backup::tarReadyRead(){
    QProcess *senderProcess = static_cast<QProcess*>(sender());
    QString output = senderProcess->readLine(0);
    ui->status->setText(output);
}

bool Backup::backup_path_valid(){
    QString given_path = ui->backup_path->text();
    QDir dir(given_path);
    bool valid = dir.exists();
    valid = !given_path.trimmed().isEmpty();
    return valid;
}

void Backup::on_change_path_clicked()
{
    QString path = QFileDialog::getExistingDirectory(this,"Select path to store backup file",QDir::homePath(),QFileDialog::ShowDirsOnly|QFileDialog::DontUseNativeDialog);

    if(QDir(path).exists() && QFileInfo(path).isDir()){
        if(!path.endsWith('/')){
            path = path+"/";
        }
        ui->backup_path->setText(path);
        settingsObj->setValue("backup_path",path);
        check_last_backup();
    }else{
        path = ui->backup_path->text();
        if(path.isEmpty()||(!QDir(path).exists() && !QFileInfo(path).isDir())){
            QMessageBox msgBox;
            msgBox.setText("Invalid path selected, Please try again.");
                  msgBox.setIconPixmap(QPixmap(":/icons/sidebar/info.png").scaled(42,42,Qt::KeepAspectRatio,Qt::SmoothTransformation));
            msgBox.setStandardButtons(QMessageBox::Ok );
            msgBox.setDefaultButton(QMessageBox::Ok);
            msgBox.exec();
        }
    }
}

void Backup::on_restore_clicked()
{
    //show warning
    QMessageBox msgBox;
    msgBox.setText("This will delete current application data of Olivia and restore selected backup.\nPress cancel to Cancel process.");
          msgBox.setIconPixmap(QPixmap(":/icons/sidebar/info.png").scaled(42,42,Qt::KeepAspectRatio,Qt::SmoothTransformation));
    msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Cancel);
    int ret = msgBox.exec();
    switch (ret) {
      case QMessageBox::Ok:
            start_restore();
        break;
      case  QMessageBox::Cancel:
            return;
        break;
    }
}

void Backup::start_restore(){

    //get backup restore path from existing dialog
    QString back_dir_path = ui->backup_path->text().trimmed();

    //get backup file
    QString backup_file_path = QFileDialog::getOpenFileName(this,tr("Select backup file to be restored."),back_dir_path,tr("Archive (*.tar)"),nullptr,QFileDialog::DontUseNativeDialog);

    //prepare arguments
    QStringList args;
    args<<"-xvf"<<backup_file_path;
    args<<"-C"<<settings_path<<".";

    //prepare untar process
    QProcess *untar = new QProcess(this);
    untar->setObjectName("untar");
    untar->setProgram("tar");
    untar->setArguments(args);
    connect(untar,SIGNAL(finished(int)),this,SLOT(untarFinished(int)));
    connect(untar,SIGNAL(readyRead()),this,SLOT(untarReadyRead()));


    //empty existing directory first
    ui->status_restore->setText("Deleting old files");
    if(QDir(settings_path).removeRecursively()){
        QDir d;
        d.mkpath(settings_path);
        ui->status_restore->setText("Deleted old files");
        ui->status_restore->setText("Starting restore process");
        untar->start();
        ui->restore->setEnabled(false);
    }
}

void Backup::untarFinished(int exitCode){
    ui->restore->setEnabled(true);
    qDebug()<<debugger_name<<"untar exited with code "<<exitCode;
    if(exitCode==0){
        ui->status_restore->setText("Done.");
        emit app_restart_required();
    }else{
        ui->status_restore->setText("Error occured.");
        QMessageBox msgBox;
        msgBox.setText("An error occured while restoring data. Please report this to developer.");
              msgBox.setIconPixmap(QPixmap(":/icons/sidebar/info.png").scaled(42,42,Qt::KeepAspectRatio,Qt::SmoothTransformation));
        msgBox.setStandardButtons(QMessageBox::Ok );
        msgBox.setDefaultButton(QMessageBox::Ok);
        msgBox.exec();
        return;
    }
}

void Backup::untarReadyRead(){
    QProcess *senderProcess = static_cast<QProcess*>(sender());
    QString output = senderProcess->readLine(0);
    ui->status_restore->setText(output);
}
