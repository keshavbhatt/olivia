#ifndef VOLUMESLIDER_H
#define VOLUMESLIDER_H

#include <QObject>
#include <QSlider>
#include <QWidget>
#include <QMouseEvent>
#include <QWheelEvent>

class volumeSlider : public QSlider
{
    Q_OBJECT
public:
    volumeSlider(QWidget* parent=0);
signals:
    void setPosition(QPoint localPos);
    void showToolTip(QPoint localPos);
protected:
    void paintEvent(QPaintEvent *ev);
protected slots:
    void mouseMoveEvent(QMouseEvent *ev);
    void mousePressEvent(QMouseEvent *ev);
    void wheelEvent(QWheelEvent *e);
};

#endif // VOLUMESLIDER_H
