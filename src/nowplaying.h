#ifndef NOWPLAYING_H
#define NOWPLAYING_H

#include <QWidget>

class nowPlaying : public QWidget
{
    Q_OBJECT
public:
    explicit nowPlaying(QWidget *parent = 0);

signals:

public slots:
    void setImage(QPixmap p);
    void setColor(QColor c);
protected:
    void paintEvent(QPaintEvent *e);
private:
    QPixmap m_image;
    QSize m_size;
    QColor m_color;
};

#endif // NOWPLAYING_H
