#ifndef PAGINATOR_H
#define PAGINATOR_H

#include <QObject>

class paginator: public QObject
{
    Q_OBJECT

public:
   explicit paginator(QObject *parent = 0);
};

#endif // PAGINATOR_H
