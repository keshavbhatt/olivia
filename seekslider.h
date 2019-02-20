#ifndef SEEKSLIDER_H
#define SEEKSLIDER_H

#include <QObject>
#include <QSlider>
#include <QWidget>

class seekSlider : public QSlider
{
    Q_OBJECT
public:
    seekSlider(QWidget* parent=0);
    double subControlWidth;
protected:
    void paintEvent(QPaintEvent *ev);

};

#endif // SEEKSLIDER_H
