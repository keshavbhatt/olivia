#include "waveformseekslider.h"
#include <QPaintDevice>
#include <QProcess>
#include <QDebug>
#include <QPainter>

waveformseekslider::waveformseekslider(QWidget *parent) :
    QLabel(parent)
{
    this->setMouseTracking(true);
    this->setMinimumSize(1,1);
    setScaledContents(false);
    setPixmap(QPixmap(":/icons/others/blankWave.png"));
    this->setMaximumHeight(60);
    this->setMinimumHeight(60);

    value = 0;
    isDragging = false;
}


void waveformseekslider::loadPixmap(QString localFileUrl)
{
    QProcess *ffmpeg = new QProcess(this);
    ffmpeg->start("bash",QStringList()<<"-c"<<"yes | ffmpeg -i "+localFileUrl+" -filter_complex '[0:a]showwavespic=s=650x40:colors=#11B9E6' /tmp/output2.png");
    ffmpeg->waitForStarted();
    connect(ffmpeg, static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
    [=]  (int exitCode, QProcess::ExitStatus exitStatus)
    {
        Q_UNUSED(exitStatus);
        if(exitCode==0){
            QPixmap waveMap = QPixmap::fromImage(crop(QImage("/tmp/output2.png")));
            if(!waveMap.isNull()){
                setPixmap(waveMap);
                //TODO save image for later use // img.save("/tmp/out2.png");
            }else{
                setPixmap(QPixmap(":/icons/others/blankWave.png"));
            }
        }else{
             setPixmap(QPixmap(":/icons/others/blankWave.png"));
        }
        ffmpeg->deleteLater();
    });
}



void waveformseekslider::setPixmap ( const QPixmap & p)
{
    pix = p;
    QLabel::setPixmap(scaledPixmap());
}

int waveformseekslider::heightForWidth( int width ) const
{
    return pix.isNull() ? this->height() : ((qreal)pix.height()*width)/pix.width();
}

QSize waveformseekslider::sizeHint() const
{
    int w = this->width();
    return QSize( w, heightForWidth(w) );
}

QPixmap waveformseekslider::scaledPixmap() const
{
    return pix.scaled(this->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

void waveformseekslider::resizeEvent(QResizeEvent * e)
{
    if(!pix.isNull())
        QLabel::setPixmap(scaledPixmap());
    QLabel::resizeEvent(e);
}


void waveformseekslider::paintEvent(QPaintEvent *event){
    int left = this->rect().left();
    int top = this->rect().top();
    int height = this->rect().height();
    int width = this->rect().width();
    if(isDragging){
        QRect progressRect(left, top,(value/100)*width , height);
        QPainter painter(this);
        painter.fillRect(progressRect, QBrush(QColor(42,130,218,30)));
        painter.fillRect(lastRect, QBrush(QColor(42,130,218,60)));
        QLabel::paintEvent(event);
    }else{
        QRect progressRect(left, top,(value/100)*width , height);
        lastRect = progressRect;
        QPainter painter(this);
        painter.fillRect(progressRect, QBrush(QColor(42,130,218,60)));
        QLabel::paintEvent(event);
    }
}

void waveformseekslider::mousePressEvent(QMouseEvent *event){
    emit setPosition(event->localPos().toPoint());
    QLabel::mousePressEvent(event);
}

void waveformseekslider::mouseMoveEvent(QMouseEvent *event){
   emit showToolTip(event->localPos().toPoint());
    if (event->buttons() & Qt::LeftButton) {
        isDragging = true;
        blockSignals(true);
        double width =  (double)event->pos().x() / (double)this->width();
        value = (double)width*100;
        repaint();
        releaseMouse();
        blockSignals(false);
        event->accept();
    }
    QLabel::mouseMoveEvent(event);
}

void waveformseekslider::mouseReleaseEvent(QMouseEvent *ev){
    emit setPosition(ev->localPos().toPoint());
    isDragging = false;
    QLabel::mouseReleaseEvent(ev);
}


void waveformseekslider::wheelEvent(QWheelEvent *event){
    event->ignore();
}
