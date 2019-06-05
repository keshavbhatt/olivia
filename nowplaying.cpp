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
    fgBrush = QBrush(QColor(m_color.red(),m_color.green(),m_color.blue(),8));

    painter.setPen(QColor(m_color.red(),m_color.green(),m_color.blue(),0)); //transparent borders
    painter.fillRect(rect(), bgBrush); //image
    painter.fillRect(rect(), fgBrush); //tint color
    painter.drawRect(rect());
    painter.end();

    QWidget::paintEvent(e);
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
