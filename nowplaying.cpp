#include "nowplaying.h"
#include <QPainter>
#include <QDebug>
#include <QPaintEvent>
#include <QLabel>

nowPlaying::nowPlaying(QWidget *parent) : QWidget(parent)
{

}

void nowPlaying::paintEvent( QPaintEvent* e )
{
    QPainter painter(this);
    QBrush bgBrush,fgBrush;
    if(m_image.isNull()){
        QPixmap pix(":/icons/cover_wrapper.png");
        m_image = pix.scaled(QSize(this->size()),Qt::KeepAspectRatio,Qt::SmoothTransformation);
    }

    if(!m_color.isValid()){
        m_color =QColor(36,136,173,95);
    }

    bgBrush = QBrush(m_image);
    fgBrush = QBrush(QColor(m_color.red(),m_color.green(),m_color.blue(),0.8));

    painter.setPen(QColor(m_color.red(),m_color.green(),m_color.blue(),0)); //transparent borders
    painter.fillRect(rect(), bgBrush); //image
    painter.fillRect(rect(), fgBrush); //tint color
    painter.drawRect(rect());

    QWidget::paintEvent(e);
}

void nowPlaying::wheelEvent(QWheelEvent *event){
//    QLabel *cover = this->findChild<QLabel *>("cover");
//    ((QLabel*)(cover))->resize(278,100);
//   ((QLabel*)(cover))->setPixmap(QPixmap(*cover->pixmap()).scaled(278,100,Qt::KeepAspectRatio,Qt::SmoothTransformation));
//    event->accept();
    const int degrees = event->delta()  / 8;
      qDebug() << degrees;
      int steps = degrees / 15;
      qDebug() << steps;
      double scaleFactor = 1.0; //How fast we zoom
      const qreal minFactor = 1.0;
      const qreal maxFactor = 10.0;
      if(steps > 0)
      {
          qDebug()<<"zoomin";
         //h11 = (h11 >= maxFactor) ? h11 : (h11 + scaleFactor);
         //h22 = (h22 >= maxFactor) ? h22 : (h22 + scaleFactor);
      }
      else
     {
          qDebug()<<"zoomout";
         //h11 = (h11 <= minFactor) ? minFactor : (h11 - scaleFactor);
         //h22 = (h22 <= minFactor) ? minFactor : (h22 - scaleFactor);
     }
}

void nowPlaying::setImage(QPixmap p){
    //m_image = p;
    Q_UNUSED(p);
    update();
}

void nowPlaying::setColor(QColor c){
    m_color = c;
    update();
}
