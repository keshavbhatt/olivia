#ifndef VOLUMESLIDER_H
#define VOLUMESLIDER_H

#include <QObject>
#include <QSlider>
#include <QWidget>

class volumeSlider : public QSlider
{
    Q_OBJECT
public:
    volumeSlider(QWidget* parent=0);
protected:
    void paintEvent(QPaintEvent *ev);
};

#endif // VOLUMESLIDER_H
