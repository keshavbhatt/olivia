#ifndef SEEKSLIDER_H
#define SEEKSLIDER_H

#include <QObject>
#include <QSlider>
#include <QWidget>
#include <QMouseEvent>

class seekSlider : public QSlider
{
    Q_OBJECT
public:
    seekSlider(QWidget* parent=nullptr);
    double subControlWidth;
signals:
    void setPosition(QPoint localPos);
    void showToolTip(QPoint localPos);
protected:
    void paintEvent(QPaintEvent *ev);
    void mousePressEvent(QMouseEvent *ev);
    void mouseMoveEvent(QMouseEvent *ev) ;
    void wheelEvent(QWheelEvent *e);
};

#endif // SEEKSLIDER_H
