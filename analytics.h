#ifndef ANALYTICS_H
#define ANALYTICS_H

#include <QObject>
#include <QtNetwork>

class analytics : public QObject
{
    Q_OBJECT
public:
    explicit analytics(QObject *parent = nullptr);

signals:

private:
    qint64 startTime;

public slots:
    QString getData();
};

#endif // ANALYTICS_H
