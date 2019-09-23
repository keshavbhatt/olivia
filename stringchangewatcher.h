#ifndef STRINGCHANGEWATCHER_H
#define STRINGCHANGEWATCHER_H

#include <QObject>

class stringChangeWatcher : public QObject
{
    Q_OBJECT
    Q_PROPERTY( QString value READ getValue WRITE setValue NOTIFY valueChanged )
public:
    explicit stringChangeWatcher( QObject* parent = nullptr ) :
        QObject{ parent }, str{ "" } {}
    virtual ~stringChangeWatcher() {}

    QString getValue() const { return str; }
public slots:
    void setValue( QString value )
    {
        if ( value != str ) {
            str = value;
            emit valueChanged( str );
        }
    }
signals:
    void valueChanged( QString value );
private:
    QString str;

};
#endif // STRINGCHANGEWATCHER_H
