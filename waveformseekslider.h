#ifndef WAVEFORMSEEKSLIDER_H
#define WAVEFORMSEEKSLIDER_H

#include <QLabel>
#include <QPixmap>
#include <QDebug>
#include <QResizeEvent>

class waveformseekslider : public QLabel
{
    Q_OBJECT
public:
    explicit waveformseekslider(QWidget *parent = 0);
    virtual int heightForWidth( int width ) const;
    virtual QSize sizeHint() const;
    QPixmap scaledPixmap() const;
    double value;
    bool isDragging;
signals:
    void setPosition(QPoint localPos);
    void showToolTip(QPoint localPos);
public slots:
    void setPixmap ( const QPixmap & );
    void resizeEvent(QResizeEvent *);
    void loadPixmap(QString localFileUrl);
protected :
    void paintEvent(QPaintEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event) ;
    void wheelEvent(QWheelEvent *event);
    void mouseReleaseEvent(QMouseEvent *ev);
private:
    QPixmap pix;
    QRect lastRect;

    const QRgb CROP_COLOR = QColor(Qt::transparent).rgba();

    QImage crop(const QImage& image)
    {
        QRect croppedRegion(0, 0, image.width(), image.height());

        // Top
        for (int row = 0; row < image.height(); row++) {
            for (int col = 0; col < image.width(); col++) {
                if (image.pixel(col, row) != CROP_COLOR) {
                    croppedRegion.setTop(row);
                    row = image.height();
                    break;
                }
            }
        }

        // Bottom
        for (int row = image.height() - 1; row >= 0; row--) {
            for (int col = 0; col < image.width(); col++) {
                if (image.pixel(col, row) != CROP_COLOR) {
                    croppedRegion.setBottom(row);
                    row = -1;
                    break;
                }
            }
        }

        // Left
        for (int col = 0; col < image.copy(croppedRegion).width(); col++) {
            for (int row = 0; row < image.copy(croppedRegion).height(); row++) {
                if (image.copy(croppedRegion).pixel(col, row) != CROP_COLOR) {
                    croppedRegion.setLeft(col);
                    col = image.copy(croppedRegion).width();
                    break;
                }
            }
        }


        // Right
        for (int col = image.copy(croppedRegion).width(); col >= 0; col--) {
            for (int row = 0; row < image.copy(croppedRegion).height(); row++) {
                if (image.pixel(col, row) != CROP_COLOR) {
                    croppedRegion.setRight(col);
                    col = -1;
                    break;
                }
            }
        }
        return image.copy(croppedRegion);
    }
};



#endif // WAVEFORMSEEKSLIDER_H
