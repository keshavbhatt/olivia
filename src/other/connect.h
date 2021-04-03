#ifndef CONNECT_H
#define CONNECT_H

#include <QWidget>

namespace Ui {
class Connect;
}

class Connect : public QWidget
{
    Q_OBJECT

public:
    explicit Connect(QWidget *parent = nullptr);
    ~Connect();
    bool hasMails = false;

public slots:
    void check_mail();
    void gotoInbox();
signals:
    void newMail();
private slots:
    void on_message_textChanged();

    void on_send_clicked();

    void on_email_textChanged(const QString &arg1);

    bool isValid();
    void on_inbox_clicked();

    void on_backToMessages_clicked();

    void on_send_anonymously_clicked();

    void sendMessage(bool anonymous);
    void reset();
    void getMessages();
    void processReply(const QByteArray data);
    void on_messageIds_currentTextChanged(const QString &currentText);

    void setMessage(const QByteArray data);
    void on_deleteCurrentMessage_clicked();

private:
    Ui::Connect *ui;
    QString uid;
    int messageMaxLength,messageMinLength;
    QString postUrl,getMessagesUrl,getMessageUrl,deleteMessageUrl,checkMailUrl;
};

#endif // CONNECT_H
