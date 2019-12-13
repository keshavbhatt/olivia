#ifndef ANALYTICS_H
#define ANALYTICS_H

#include <QObject>
#include <QtNetwork>

class analytics : public QObject
{
    Q_OBJECT
public:
    explicit analytics(QObject *parent = nullptr);
    QString data;
    QString uid;

private:
    qint64 startTime;
    QString setting_path;

public slots:
    QString getData();
    void addData(QString arg1);
    void headLessPush();
private slots:
    void processData();
    void push(QString type, QStringList data_list);
    void preparePush(QStringList data_list);
    QString getUidFromOldLogFile();
};

#endif // ANALYTICS_H
