#ifndef REQUEST_H
#define REQUEST_H

#include <QtNetwork>
#include <QObject>

class Request : public QObject
{
    Q_OBJECT

public:
    Request(QObject* parent=0);
    virtual ~Request();
public slots:
    void get(const QUrl url, QString uid);
signals:
   void requestFinished(QString uid,QString rep);
};

#endif // REQUEST_H
