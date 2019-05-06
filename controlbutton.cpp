#include "controlbutton.h"
#include <QToolTip>
#include <QEvent>

controlButton::controlButton(QWidget *parent)
    : QPushButton(parent)
{
    setMouseTracking(true);
}

void controlButton::mouseMoveEvent(QMouseEvent *e){
    QToolTip::showText(this->mapToGlobal(e->localPos().toPoint()),this->toolTip());
    QPushButton::mouseMoveEvent(e);
}


