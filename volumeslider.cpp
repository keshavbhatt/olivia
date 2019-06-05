#include "volumeslider.h"
#include <QStyleOptionSlider>
#include <QPainter>
#include <QDebug>

volumeSlider::volumeSlider(QWidget *parent)
    : QSlider(parent)
{
    setMouseTracking(true);
}

void volumeSlider::paintEvent(QPaintEvent *ev) {
  QStyleOptionSlider opt;
  initStyleOption(&opt);

  opt.subControls = QStyle::SC_SliderGroove | QStyle::SC_SliderHandle;
  if (tickPosition() != NoTicks) {
    opt.subControls |= QStyle::SC_SliderTickmarks;
  }
  QRect groove_rect = style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderGroove, this);
  QSlider::paintEvent(ev);
  QRect rect(groove_rect.right()-static_cast<int>(groove_rect.width()*0.23), groove_rect.top(),static_cast<int>(groove_rect.width()*0.24), groove_rect.height());//0.6 * groove_rect.width()
  QPainter painter(this);
  painter.fillRect(rect, QBrush(QColor(42,130,218,80)));
}

void volumeSlider::mouseMoveEvent(QMouseEvent *ev){
    emit showToolTip(ev->localPos().toPoint());
    QSlider::mouseMoveEvent(ev);
}

void volumeSlider::mousePressEvent(QMouseEvent *ev){
    emit setPosition(ev->localPos().toPoint());
    QSlider::mousePressEvent(ev);
}

void volumeSlider::wheelEvent(QWheelEvent *e){
    //emit showToolTip(QPoint(0,0));
    QSlider::wheelEvent(e);
}


