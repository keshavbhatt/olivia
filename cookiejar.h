

#ifndef COOKIEJAR_H
#define COOKIEJAR_H

#include <QBasicTimer>
#include <QNetworkCookieJar>

class CookieJar : public QNetworkCookieJar
{
    Q_OBJECT
public:
    CookieJar(const QString& cookieFileName, QObject* parent = 0);
    virtual ~CookieJar();

    void save();
    void load();

    virtual bool setCookiesFromUrl(const QList<QNetworkCookie> &cookieList, const QUrl &url);

public Q_SLOTS:
    void clearCookieJar();

protected:
    virtual void timerEvent(QTimerEvent* ev);

private:
    void expireCookies();
    QBasicTimer m_cookieSavingTimer;
    QString m_cookieFileName;
};

#endif // COOKIEJAR_H
