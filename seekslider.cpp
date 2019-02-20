#include "seekslider.h"
#include <QStyleOptionSlider>
#include <QPainter>
#include <QDebug>

seekSlider::seekSlider(QWidget *parent)
    : QSlider(parent)
{
}

void seekSlider::paintEvent(QPaintEvent *ev) {
  QStyleOptionSlider opt;
  initStyleOption(&opt);

  opt.subControls = QStyle::SC_SliderGroove | QStyle::SC_SliderHandle;
  if (tickPosition() != NoTicks) {
    opt.subControls |= QStyle::SC_SliderTickmarks;
  }
  QRect groove_rect = style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderGroove, this);
  QSlider::paintEvent(ev);
  QRect rect(groove_rect.left(), groove_rect.top(),(subControlWidth/100)*groove_rect.width() , groove_rect.height());//0.6 * groove_rect.width()
  QPainter painter(this);
  painter.fillRect(rect, QBrush(QColor(42,130,218,80)));
}
