#include "lyrics.h"
#include "ui_lyrics.h"
#include "cookiejar.h"
#include "elidedlabel.h"

#include <QStandardPaths>
#include <QNetworkDiskCache>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QListWidgetItem>


Lyrics::Lyrics(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Lyrics)
{
    ui->setupUi(this);
    this->setWindowTitle(qApp->applicationName()+" | Lyrics Repository");
    ui->progressBar->setMaximum(100);
    ui->progressBar->hide();

    ui->resultCount->hide();


    //sets search icon in label
    ui->label_5->resize(ui->label_5->width(),ui->query->height());
    ui->label_5->setPixmap(QPixmap(":/icons/sidebar/search.png").scaled(18,18,Qt::KeepAspectRatio,Qt::SmoothTransformation));

    ui->lyricsWidget->hide();

    _networkManager = new QNetworkAccessManager(nullptr);
    connect(_networkManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(_networkManagerFinished(QNetworkReply*)));

    //make it fast
    QString setting_path =  QStandardPaths::writableLocation(QStandardPaths::DataLocation);
    QString cookieJarPath ;
    if(setting_path.split("/").last().isEmpty()){
       cookieJarPath  =  setting_path+"/cookiejar_olivia.dat";
    }else{
       cookieJarPath  =  setting_path+"cookiejar_olivia.dat";
    }
    _networkManager->setCookieJar(new CookieJar(cookieJarPath,_networkManager));
    QNetworkDiskCache* diskCache = new QNetworkDiskCache(this);
    diskCache->setCacheDirectory(setting_path);
    _networkManager->setCache(diskCache);


    //adds lyrics title widget in lyrics header
    ElidedLabel *titleLabel = new ElidedLabel("title",nullptr);
    titleLabel->setObjectName("lyricsTitleWidget");
    titleLabel->setAlignment(Qt::AlignHCenter);
    ui->lyricsTitleLayout->addWidget(titleLabel);

}

void Lyrics::_networkManagerFinished(QNetworkReply *reply){

    ui->progressBar->setMaximum(100);
    ui->progressBar->hide();

    //process JSON

    QString strReply = static_cast<QString>(reply->readAll());
    QJsonDocument jsonResponse = QJsonDocument::fromJson(strReply.toUtf8());
    QJsonArray jsonArray = jsonResponse.array();

//    jsonArray.count()>0 ? ui->noResult->hide() : ui->noResult->show();

    if(jsonArray.count()>0){
        ui->noResult->hide();
        ui->resultCount->show();
        ui->resultCount->setText("Returned "+QString::number(jsonArray.count())+" results for your query.");
    }else{
        ui->noResult->show();
        ui->resultCount->hide();
    }

    foreach (const QJsonValue & value, jsonArray) {
        if(this->isVisible()){
            QJsonObject obj = value.toObject();
            QString title = obj.value("song_title").toString();
            QString link  = obj.value("link").toString();
            QString cover =obj.value("cover").toString();

            QWidget *lyric_item_widget = new QWidget(ui->result);
            lyricItem_Ui.setupUi(lyric_item_widget);

            QFont font("Ubuntu");
            font.setPixelSize(12);
            setFont(font);

            ElidedLabel *titleLabel = new ElidedLabel(title,nullptr);
            titleLabel->setObjectName("lyric_title");
            titleLabel->setFont(font);
            lyricItem_Ui.verticalLayout->addWidget(titleLabel);

            lyricItem_Ui.url->setText(link);
            lyricItem_Ui.url->hide();


            LoadCover( QUrl(cover),*lyricItem_Ui.cover);

            connect(lyricItem_Ui.showLyrics,SIGNAL(clicked(bool)),this,SLOT(showLyrics()));

            //set up an item
            QListWidgetItem* item;
            item = new QListWidgetItem(ui->result);

            QGraphicsOpacityEffect *eff = new QGraphicsOpacityEffect(this);

            item->setSizeHint(lyric_item_widget->minimumSizeHint());

            ui->result->setItemWidget(item, lyric_item_widget);
            ui->result->itemWidget(item)->setGraphicsEffect(eff);

            QPropertyAnimation *a = new QPropertyAnimation(eff,"opacity");
            a->setDuration(500);
            a->setStartValue(0);
            a->setEndValue(1);
            a->setEasingCurve(QEasingCurve::InCirc);
            a->start(QPropertyAnimation::DeleteWhenStopped);
            ui->result->addItem(item);

        }else{
            return;
        }
    }
    reply->deleteLater();
}

void Lyrics::setLyricsTitle(QString title){
    ui->lyricsWidget->findChild<ElidedLabel*>("lyricsTitleWidget")->setText(title);
}

void Lyrics::showLyrics(){

    QPushButton* senderButton = qobject_cast<QPushButton*>(sender());
    QString url = senderButton->parent()->findChild<QLineEdit*>("url")->text();
    QString setting_path =  QStandardPaths::writableLocation(QStandardPaths::DataLocation);

    setLyricsTitle(senderButton->parent()->findChild<ElidedLabel*>("lyric_title")->text());

    QNetworkAccessManager manager;
    manager.setParent(this);
    QNetworkDiskCache* diskCache = new QNetworkDiskCache(this);
    diskCache->setCacheDirectory(setting_path);
    manager.setCache(diskCache);

    QEventLoop loop;
    loop.setParent(this);
    QNetworkReply *reply = manager.get(QNetworkRequest(url));
    ui->progressBar->setMaximum(0);
    ui->progressBar->show();

    QObject::connect(reply, &QNetworkReply::finished, &loop, [&reply, this,&loop ](){
     if (reply->error() == QNetworkReply::NoError)
     {
         QByteArray data = reply->readAll();
         if(!data.isEmpty()){
             showLyricsWidget("<center>"+data+"</center>");
         }else{
             showLyricsWidget("<center>"+data+"</center>");
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

void Lyrics::showLyricsWidget(QString data){
//    data = htmlToPlainText(data);
    ui->lyrics->setHtml(data.split("Song Discussions is protected").first());
//    ui->lyrics->append(QString("<img src=\"https://tpc.googlesyndication.com/simgad/16094122936629946910\" />"));
    QPropertyAnimation *animation = new QPropertyAnimation(ui->lyricsWidget, "maximumHeight");
    animation->setDuration(500);
    animation->setStartValue(0);
    animation->setEndValue(this->height());
    animation->setEasingCurve(QEasingCurve::Linear);
    animation->start(QPropertyAnimation::DeleteWhenStopped);
    hideResult();

    ui->progressBar->setMaximum(100);
    ui->progressBar->hide();
}

void Lyrics::hideResult(){
    QPropertyAnimation *animation = new QPropertyAnimation(ui->resultWidget, "maximumHeight");
    animation->setDuration(500);
    animation->setStartValue(this->height());
    animation->setEndValue(0);
    animation->setEasingCurve(QEasingCurve::Linear);
    ui->lyricsWidget->setVisible(true);
    animation->start(QPropertyAnimation::DeleteWhenStopped);
}

void Lyrics::hideLyrics(){
    QPropertyAnimation *animation = new QPropertyAnimation(ui->resultWidget, "maximumHeight");
    animation->setDuration(500);
    animation->setStartValue(0);
    animation->setEndValue(this->height());
    animation->setEasingCurve(QEasingCurve::Linear);
    ui->resultWidget->setVisible(true);
    connect(animation,&QPropertyAnimation::valueChanged, [=](QVariant var){
         ui->lyricsWidget->setMaximumHeight(-var.toRect().height());
    });
    connect(animation,&QPropertyAnimation::finished, [=](){
         ui->lyricsWidget->setVisible(false);
    });
    animation->start(QPropertyAnimation::DeleteWhenStopped);
}

void Lyrics::resizeEvent(QResizeEvent *event){
    int height = event->size().height();
    if(ui->lyricsWidget->height()>0){
        ui->lyricsWidget->setMaximumHeight(height);
    }
    if(ui->resultWidget->height()>0){
        ui->resultWidget->setMaximumHeight(height);
    }
    QWidget::resizeEvent(event);
}

Lyrics::~Lyrics()
{
    delete ui;
}

void Lyrics::setCustomStyle(QString searchStyle, QString listStyle, QString windowStyle)
{
    ui->query->setStyleSheet(searchStyle);
    ui->result->setStyleSheet(listStyle+"border:none;border-radius:0px;");
    ui->label_5->setStyleSheet(searchStyle+"border:none;border-radius:0px;");
    this->setStyleSheet(windowStyle.replace("QMainWindow","QWidget#Lyrics"));
}

void Lyrics::on_query_returnPressed()
{
    QNetworkRequest req;
    req.setUrl(QUrl("http://ktechpit.com/USS/Olivia/lyrics/search.php?query="+ui->query->text().trimmed()));
    _networkManager->get(req);

    ui->result->clear();
    ui->lyrics->clear();
    ui->progressBar->setMaximum(0);
    ui->progressBar->show();
}

void Lyrics::on_close_clicked()
{
    hideLyrics();
}

//for external calls with automatic search enabled
void Lyrics::setQueryString(QString query){
    query = query.split("(").first();
    this->show();
    ui->result->clear();
    ui->noResult->hide();
    ui->resultCount->hide();
    if(ui->resultWidget->height()<1){
        hideLyrics();
    }
    ui->query->setText(query);
    if(query.length()>1)
    emit ui->query->returnPressed();
}

void Lyrics::showEvent(QShowEvent *event){
    ui->result->clear();
    QWidget::showEvent(event);
}


void Lyrics::hideEvent(QHideEvent *event){
    ui->result->clear();
    QWidget::hideEvent(event);
}



void Lyrics::on_lyrics_textChanged()
{
    if(ui->lyrics->toPlainText().contains("Submit Now")){
        ui->lyrics->setHtml("<center><br><br><br>Lyrics for this song is not available! "
                            "<br><br>"
                            "<b>Lyrics submission feature will be availabe in future.</b>"
                            "</center>");
        ui->copy->setEnabled(false);
    }else{
        ui->copy->setEnabled(true);
    }
}

void Lyrics::on_copy_clicked()
{
    ui->lyrics->selectAll();
    ui->lyrics->copy();
}
