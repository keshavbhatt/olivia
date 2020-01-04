#include "connect.h"
#include "ui_connect.h"
#include "analytics.h"
#include "utils.h"

#include <QUrlQuery>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QDebug>
#include <QGroupBox>


Connect::Connect(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Connect)
{
    ui->setupUi(this);

    analytics * analytic = this->parent()->findChild<analytics*>("analytics");
    setEnabled(false);
    connect(analytic,&analytics::analytics_ready,[=](){
        setEnabled(true);
        uid = analytic->uid.split("_").last();
        ui->uid->setText(uid);
        check_mail();
    });
    ui->uidWidget->hide();

    messageMaxLength = 250;
    messageMinLength = 20;
    postUrl = "http://ktechpit.com/USS/Olivia/services/connect/message.php";
    getMessagesUrl = "http://ktechpit.com/USS/Olivia/services/connect/list.php";
    getMessageUrl = "http://ktechpit.com/USS/Olivia/services/connect/read.php";
    deleteMessageUrl= "http://ktechpit.com/USS/Olivia/services/connect/delete.php";
    checkMailUrl = "http://ktechpit.com/USS/Olivia/services/connect/check.php";

    ui->message->setPlaceholderText("Write your suggestions, request or about any issues you facing."
                                    "\n\nMessage text must be > "+QString::number(messageMinLength)+" chars and < "+QString::number(messageMaxLength)+" chars.");

    ui->inboxWidget->hide();
    ui->deleteCurrentMessage->setEnabled(false);
    //init validator
    isValid();

    //style stuffs
    foreach (QTextEdit *edit, this->findChildren<QTextEdit*>()) {
        edit->setStyleSheet("QTextEdit{border:none;"
                            "background-image: url(:/icons/others/grid.png);"
                            "background-attachment: fixed;}"
                            "QScrollBar:vertical {"
                            "   background-color: transparent;"
                            "   border:none;"
                            "   width: 10px;"
                            "   margin: 22px 0 22px 0;"
                            "}"
                            "QScrollBar::handle:vertical {"
                            "   background: grey;"
                            "   min-height: 20px;"
                            "}");
    }
    QString btn_style ="QPushButton{color: silver; background-color: #45443F; border:1px solid #272727; padding-top: 3px; padding-bottom: 3px; padding-left: 3px; padding-right: 3px; border-radius: 2px; outline: none;}"
    "QPushButton:disabled { background-color: #45443F; border:1px solid #272727; padding-top: 3px; padding-bottom: 3px; padding-left: 5px; padding-right: 5px; /*border-radius: 2px;*/ color: #636363;}"
    "QPushButton:hover{border: 1px solid #272727;background-color:#5A584F; color:silver ;}"
    "QPushButton:pressed {background-color: #45443F;color: silver;padding-bottom:1px;}";
    foreach (QPushButton *btn, this->findChildren<QPushButton*>()) {
        if(btn->objectName()!="icon")
        btn->setStyleSheet(btn_style);
    }
    ui->loader->setRoundness(70.0);
    ui->loader->setMinimumTrailOpacity(15.0);
    ui->loader->setTrailFadePercentage(70.0);
    ui->loader->setNumberOfLines(10);
    ui->loader->setLineLength(5);
    ui->loader->setLineWidth(2);
    ui->loader->setInnerRadius(2);
    ui->loader->setRevolutionsPerSecond(1);
    ui->loader->setColor(QColor(227, 222, 222));
}

Connect::~Connect()
{
    delete ui;
}

void Connect::on_message_textChanged()
{
  isValid();

  if(ui->message->toPlainText().length() > messageMaxLength)
  {
      int diff = ui->message->toPlainText().length() - messageMaxLength;
      QString newStr = ui->message->toPlainText();
      newStr.chop(diff);
      ui->message->setText(newStr);
      QTextCursor cursor(ui->message->textCursor());
      cursor.movePosition(QTextCursor::End, QTextCursor::MoveAnchor);
      ui->message->setTextCursor(cursor);
  }
}

void Connect::on_send_clicked()
{
    if(isValid()){
       sendMessage(false);
    }
}
void Connect::on_send_anonymously_clicked()
{
       sendMessage(true);
}

void Connect::sendMessage(bool anonymous){

    QUrl serviceUrl = QUrl(postUrl);
    QByteArray postData;
    QUrlQuery query;
    QString mid = utils::generateRandomId(10);
    query.addQueryItem("mid",mid);
    query.addQueryItem("uid",uid.split("_").last());
    if(anonymous){
        query.addQueryItem("email","anon");
    }else{
        query.addQueryItem("email",ui->email->text().trimmed());
    }
    query.addQueryItem("message",ui->message->toPlainText().trimmed());

    //save data to text file
    postData = query.toString(QUrl::FullyEncoded).toUtf8();
    QNetworkAccessManager *networkManager = new QNetworkAccessManager(this);
    connect(networkManager,&QNetworkAccessManager::finished,[=](QNetworkReply*rep){
        if(rep->error()==QNetworkReply::NoError){
             reset();
             ui->loader->stop();
        }else{
             ui->loader->stop();
//            showStatusMessage(rep->errorString(),3000);
        }
    });
    QNetworkRequest networkRequest(serviceUrl);
    networkRequest.setHeader(QNetworkRequest::ContentTypeHeader,"application/x-www-form-urlencoded");
    networkManager->post(networkRequest,postData);
    ui->loader->start();
    ui->send->setEnabled(false);
    ui->send_anonymously->setEnabled(false);
}



void Connect::reset(){
    ui->message->clear();
    ui->email->clear();
    isValid();
}

void Connect::on_email_textChanged(const QString &arg1)
{   
    Q_UNUSED(arg1);
    isValid();
}

bool Connect::isValid(){
    //set email validator is not set
    if(ui->email->validator()==nullptr){
        QRegularExpression rx("\\b[A-Z0-9._%+-]+@[A-Z0-9.-]+\\.[A-Z]{2,4}\\b",
                                  QRegularExpression::CaseInsensitiveOption);
        ui->email->setValidator(new QRegularExpressionValidator(rx, this));
    }
    //check message body and
    QString message = ui->message->toPlainText();
    ui->send->setEnabled((message.trimmed().count()>messageMinLength && !message.trimmed().isEmpty()) && ui->email->hasAcceptableInput());
    ui->send_anonymously->setEnabled((message.trimmed().count()>messageMinLength && !message.trimmed().isEmpty()));
    return ui->send->isEnabled();
}

void Connect::on_inbox_clicked()
{
    ui->inboxWidget->show();
    ui->messageWidget->hide();
    getMessages();
}

void Connect::on_backToMessages_clicked()
{
    ui->inboxWidget->hide();
    ui->messageWidget->show();
    ui->messageIds->clear();
    ui->messageText->clear();
    ui->replyText->clear();
    ui->messageIds->setEnabled(true);
    ui->deleteCurrentMessage->setEnabled(false);
}

void Connect::getMessages(){
    QUrl serviceUrl = QUrl(getMessagesUrl+"?uid="+ui->uid->text().trimmed());
    QNetworkAccessManager *networkManager = new QNetworkAccessManager(this);
    connect(networkManager,&QNetworkAccessManager::finished,[=](QNetworkReply*rep){
        if(rep->error()==QNetworkReply::NoError){
             ui->loader->stop();
             processReply(rep->readAll());
        }else{
             ui->loader->stop();
        }
    });
    QNetworkRequest networkRequest(serviceUrl);
    networkManager->get(networkRequest);
    ui->loader->start();
}

void Connect::processReply(const QByteArray data){
    if(data.trimmed().isEmpty()){
        QListWidgetItem *item = new QListWidgetItem("No messages");
        item->setTextAlignment(Qt::AlignHCenter);
        item->setFlags(Qt::NoItemFlags);
        ui->messageIds->addItem(item);
        return;
    }
    QStringList messageIds = QString(data).split("<br>");
    foreach (QString id, messageIds) {
        if(!id.trimmed().isEmpty()){
            QListWidgetItem *item ;
            if(id.split("[readed]").last()=="0"){
                item = new QListWidgetItem(QIcon(":/icons/micro/email-closed.png"),id.split("[readed]").first().trimmed());
            }else{
                item = new QListWidgetItem(QIcon(":/icons/micro/email-opened.png"),id.split("[readed]").first().trimmed());
            }
            ui->messageIds->addItem(item);
        }
    }
}



void Connect::on_messageIds_currentTextChanged(const QString &currentText)
{
    ui->deleteCurrentMessage->setEnabled(false);
    ui->messageText->clear();
    ui->replyText->clear();
    if(currentText.trimmed().isEmpty())
        return;

    QUrl serviceUrl = QUrl(getMessageUrl+"?mid="+currentText+"&uid="+ui->uid->text().trimmed());
    QNetworkAccessManager *networkManager = new QNetworkAccessManager(this);
    connect(networkManager,&QNetworkAccessManager::finished,[=](QNetworkReply*rep){
        if(rep->error()==QNetworkReply::NoError){
             ui->loader->stop();
             setMessage(rep->readAll());
             ui->deleteCurrentMessage->setEnabled(true);
        }else{
             ui->loader->stop();
             ui->deleteCurrentMessage->setEnabled(false);
        }
    });
    QNetworkRequest networkRequest(serviceUrl);
    networkManager->get(networkRequest);
    ui->loader->start();
}

void Connect::setMessage(const QByteArray data){
    QStringList responseList = QString(data).split("----reply----");
    if(responseList.count()==2){
        ui->messageText->setText(responseList.at(0));
        ui->replyText->setText(responseList.at(1));
    }else{
        ui->messageText->setText("<b>Invalid message</b>");
    }
}

void Connect::on_deleteCurrentMessage_clicked()
{
    if(ui->messageIds->count()==0)
        return;
    int currentRow = ui->messageIds->currentRow();
    QString mid = ui->messageIds->currentItem()->text().trimmed();
    if(mid.isEmpty()){
        ui->deleteCurrentMessage->setEnabled(false);
        return;
    }
    QUrl serviceUrl = QUrl(deleteMessageUrl+"?mid="+mid+"&uid="+ui->uid->text().trimmed());
    QNetworkAccessManager *networkManager = new QNetworkAccessManager(this);
    connect(networkManager,&QNetworkAccessManager::finished,[=](QNetworkReply*rep){
        if(rep->error()==QNetworkReply::NoError){
             ui->loader->stop();
             QListWidgetItem *item = ui->messageIds->takeItem(currentRow);
             delete item;
             ui->messageText->clear();
             ui->replyText->clear();
             ui->messageIds->setEnabled(true);
             ui->deleteCurrentMessage->setEnabled(false);
        }else{
             ui->messageIds->setEnabled(true);
             ui->loader->stop();
        }
    });
    ui->messageIds->setEnabled(false);
    QNetworkRequest networkRequest(serviceUrl);
    networkManager->get(networkRequest);
    ui->loader->start();
}

void Connect::check_mail(){
    QUrl serviceUrl = QUrl(checkMailUrl+"?uid="+ui->uid->text().trimmed());
    QNetworkAccessManager *networkManager = new QNetworkAccessManager(this);
    connect(networkManager,&QNetworkAccessManager::finished,[=](QNetworkReply*rep){
        if(rep->error()==QNetworkReply::NoError){
            if(rep->readAll().trimmed()=="1")
             emit newMail();
        }
    });
    QNetworkRequest networkRequest(serviceUrl);
    networkManager->get(networkRequest);
}

void Connect::gotoInbox()
{
    on_inbox_clicked();
}
