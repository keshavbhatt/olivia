#ifndef ONLINESEARCHSUGGESTION_H
#define ONLINESEARCHSUGGESTION_H

#include <QObject>
#include <QLineEdit>
#include <QTreeWidget>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QByteArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
#include <QHeaderView>
#include <QEvent>
#include <QJsonArray>
#include <QKeyEvent>
#include <QSettings>


class onlineSearchSuggestion : public QObject
{
    Q_OBJECT

public:
    explicit onlineSearchSuggestion(QLineEdit *parent = nullptr);
    ~onlineSearchSuggestion();
    bool eventFilter(QObject *obj, QEvent *ev) override;
    void showCompletion(const QVector<QString> &choices);

public slots:

    void doneCompletion();
    void preventSuggest();
    void autoSuggest();
    void handleNetworkData(QNetworkReply *networkReply);

private slots:
    bool checkBlackList(QString word);
private:
    QLineEdit *editor = nullptr;
    QTreeWidget *popup = nullptr;
    QTimer timer;
    QNetworkAccessManager networkManager;
    QStringList blacklist;
    QSettings settingsObj;

};

#endif // ONLINESEARCHSUGGESTION_H
