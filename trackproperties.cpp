#include "trackproperties.h"
#include "ui_trackproperties.h"

#include <QFileDialog>
#include <QPushButton>
#include <QFrame>

#include "taglib/taglib.h"
#include <taglib/fileref.h>
#include <taglib/tfile.h>
#include <taglib/tag.h>
#include <taglib/id3v2tag.h>
#include <taglib/mpegfile.h>
#include <taglib/attachedpictureframe.h>
#include <taglib/mp4coverart.h>


QString btn_style ="QPushButton{color: silver; background-color: #45443F; border:1px solid #272727; padding-top: 3px; padding-bottom: 3px; padding-left: 3px; padding-right: 3px; border-radius: 2px; outline: none;}"
"QPushButton:disabled { background-color: #45443F; border:1px solid #272727; padding-top: 3px; padding-bottom: 3px; padding-left: 5px; padding-right: 5px; /*border-radius: 2px;*/ color: #636363;}"
"QPushButton:hover{border: 1px solid #272727;background-color:#5A584F; color:silver ;}"
"QPushButton:pressed {background-color: #45443F;color: silver;padding-bottom:1px;}";

TrackProperties::TrackProperties(QWidget *parent,QString setting_path) :
    QWidget(parent),
    ui(new Ui::TrackProperties)
{
    ui->setupUi(this);
    this->setWindowModality(Qt::WindowModal); // block input to parent window
    this->setting_path = setting_path; // set settings path from mainwindow
    ui->progressBar->setValue(0);
    ui->progressBar->hide();
}

void TrackProperties::setValues(QStringList meta){
    this->meta = meta;
    // set different values form meta of track
    QString title,albumId,album,artistId,artist,thumbnail,offlineUrl,ytids,dominantColor;
    if(meta.count()==10){
        this->trackId = meta.at(0);
        title = meta.at(1);
        albumId = meta.at(2);
        album = meta.at(3);
        artistId = meta.at(4);
        artist = meta.at(5);
        thumbnail = meta.at(6);
        offlineUrl = meta.at(7);
        ytids = meta.at(8);
        dominantColor = meta.at(9);

        QString base64 = thumbnail.split("base64,").last();
        QByteArray ba = base64.toUtf8();
        QPixmap image;
        image.loadFromData(QByteArray::fromBase64(ba));

        if(albumId.contains("undefined-")){
            ui->cover->setFixedSize(290,176);
        }else{
            ui->cover->setFixedSize(170,170);
        }

        if(!image.isNull()){
            ui->cover->setPixmap(image.scaled(ui->cover->size(),Qt::KeepAspectRatio,Qt::SmoothTransformation));
            image.save(setting_path+"/converted_temp/"+"cover.jpg","JPEG");
        }
        //set values
        ui->title_val->setText(title);
        ui->title_val->setToolTip(title);
        ui->album_val->setText(album);
        ui->artist_val->setText(artist);
        setWidgetTheme(dominantColor);
    }
}

void TrackProperties::setWidgetTheme(QString dominantColor){
    foreach (QPushButton *btn, this->findChildren<QPushButton*>()) {
        btn->setStyleSheet(btn_style);
    }
    //sets theme for properties widget according to dominant color of track
    QString r,g,b;
    QStringList colors = dominantColor.split(",");
    if(colors.count()==3){
        r=colors.at(0);
        g=colors.at(1);
        b=colors.at(2);
    }else{
        r="98";
        g="164";
        b="205";
    }
    QString widgetStyle= "background-color:"
                         "qlineargradient(spread:pad, x1:0, y1:0, x2:0, y2:1,"
                         "stop:0.129213 rgba("+r+", "+g+", "+b+", 30),"
                         "stop:0.38764 rgba("+r+", "+g+", "+b+", 120),"
                         "stop:0.679775 rgba("+r+", "+g+", "+b+", 84),"
                         "stop:1 rgba("+r+", "+g+", "+b+", 30));";
    setStyleSheet("QWidget#TrackProperties{"+widgetStyle+";border:none;}");
    ui->line->setStyleSheet("background-color:rgba("+r+", "+g+", "+b+", 50)");
    ui->line_2->setStyleSheet("background-color:rgba("+r+", "+g+", "+b+", 50)");
}

TrackProperties::~TrackProperties()
{
    delete ui;
}

void TrackProperties::on_showInFiles_clicked()
{
    // call desktop file manager to show song cache folder
    QDesktopServices::openUrl(QUrl(setting_path+"/downloadedTracks/"));
}

void TrackProperties::on_copy_clicked()
{
    QString filename = QFileDialog::getSaveFileName(this, tr("Save File"),
                              setting_path+"/downloadedTracks/"+this->trackId,
                              tr("Audio raw (*.webm)"));
    QFile file(setting_path+"/downloadedTracks/"+this->trackId);
    file.copy(filename);
}

void TrackProperties::on_deleteTrack_clicked()
{
    // call mainwindow to delete cache
    emit delete_song_cache(this->trackId);
    this->close();
}

void TrackProperties::on_copy_with_tags_clicked()
{
    // Write tags to file
    convertToMpeg(setting_path+"/downloadedTracks/"+this->trackId);
}

void TrackProperties::convertToMpeg(QString path){
    ui->copy_with_tags->setEnabled(false);
    ui->copy_with_tags->setText("Converting...");
    ui->progressBar->show();

    QProcess *process = new QProcess(this);
    process->setProgram("bash");
    process->setArguments(QStringList()<<"-c"<<QString(" pv -n \""+path+"\""" | ffmpeg -v warning -i pipe:0 -vn -ar 44100 -ac 2 -b:a 192k \""+setting_path+"/converted_temp/"+"output.mp3\""" -y"));
    process->setReadChannelMode(QProcess::MergedChannels);
    connect(process,&QProcess::readyRead,[=](){
       int value = QString(process->readAll().trimmed()).toInt();
       ui->progressBar->setValue(value);
       ui->copy_with_tags->setText("Converting...("+QString::number(value)+"%)");
    });
    connect(process,SIGNAL(finished(int,QProcess::ExitStatus)),this,SLOT(conversion_Finished(int,QProcess::ExitStatus)));
    process->start();
}

void TrackProperties::conversion_Finished(int exitCode,QProcess::ExitStatus status){
    Q_UNUSED(status);
    if(exitCode==0){
        QString newNameWithTags = writeTags();

       if(newNameWithTags.isEmpty())
           return;


       QString filename = QFileDialog::getSaveFileName(this, tr("Save File"),
                                    newNameWithTags,
                                    tr("Audio (*.mp3)"));
       QFile file(newNameWithTags);
       file.copy(filename);

       ui->copy_with_tags->setEnabled(false);
       ui->copy_with_tags->setText("File ready to export");
       ui->progressBar->hide();
    }
    sender()->deleteLater();
}

QString TrackProperties::writeTags(){

            QString newNameWithTags = "";

            QFile f(setting_path+"/converted_temp/output.mp3");
            QProcess *process=new QProcess(0);
            process->setProgram("bash");
            //ffmpeg -i Laredo.mp3 -i cover.jpg -map 0:0 -map 1:0 -c copy -id3v2_version 3 -metadata:s:v title="Album cover" -metadata:s:v comment="Cover (front)" out.mp3 -v error
            process->setArguments(QStringList()<<"-c"<<"ffmpeg -i "+f.fileName()+" -i "+setting_path+"/converted_temp/"+"cover.jpg -map 0:0 -map 1:0 -c copy -id3v2_version 3"+
                                  " -metadata:s:v title=\"Album cover\" -metadata:s:v comment=\"Cover (front)\" "+setting_path+"/converted_temp/outputWithCover.mp3"+" -v error");

            process->start();
            process->waitForFinished();
            f.remove();

            QByteArray fileName = QFile::encodeName( setting_path+"/converted_temp/"+"outputWithCover.mp3" );
            QFile file(fileName);
            const char * encodedName = fileName.constData();
            TagLib::FileRef fileref = TagLib::FileRef( encodedName );
            if (fileref.isNull())
            {
                qDebug()<<"is Null";
                return "";
            }
            else
            {
                fileref.tag()->setTitle(TagLib::String(QString(meta.at(1)).toStdString()));
                fileref.tag()->setArtist(TagLib::String(QString(meta.at(5)).toStdString()));
                fileref.tag()->setAlbum(TagLib::String(QString(meta.at(3)).toStdString()));

                if(fileref.save()){
                    file.rename(setting_path+"/converted_temp/"+this->meta.at(1)+".mp3");
                    newNameWithTags = file.fileName();
                }
            }

        return newNameWithTags;
}

void TrackProperties::closeEvent(QCloseEvent *event){
    this->meta.clear();
    this->trackId.clear();
    ui->progressBar->hide();
    ui->copy_with_tags->setEnabled(true);
    ui->copy_with_tags->setText("Convert to mp3 and copy media file");
    event->accept();
}

