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

private:
    qint64 startTime;

public slots:
    QString getData();
    void addData(QString arg1);
};

#endif // ANALYTICS_H
