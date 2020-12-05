#ifndef NOTIFICATIONPOPUP_H
#define NOTIFICATIONPOPUP_H

#pragma once

#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPushButton>
#include <QSpacerItem>
#include <QTimer>
#include <QVBoxLayout>
#include <QPropertyAnimation>
#include <QApplication>
#include <QDesktopWidget>
#include <QDebug>


class NotificationPopup : public QWidget
{
    Q_OBJECT

    QLabel m_icon, m_title, m_message;
    QTimer *timer = nullptr;

public:
    NotificationPopup(QWidget *parent) : QWidget(parent)
    {
        setWindowFlags(Qt::ToolTip);
        auto rootLayout = new QHBoxLayout(this);

        rootLayout->addWidget(&m_icon);

        auto bodyLayout = new QVBoxLayout;
        rootLayout->addLayout(bodyLayout);

        auto titleLayout = new QHBoxLayout;
        bodyLayout->addLayout(titleLayout);

        titleLayout->addWidget(&m_title);
        titleLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding));

        auto close = new QPushButton(tr("Close"));
        titleLayout->addWidget(close);
        connect(close, &QPushButton::clicked, this, &NotificationPopup::onClosed);

        bodyLayout->addWidget(&m_message);
        adjustSize();
    }

    void present(QString title,QString message, const QPixmap image)
    {

        m_title.setText("<b>" + title + "</b>");
        m_message.setText(message);
        m_icon.setPixmap(image.scaledToHeight(m_icon.height(),Qt::SmoothTransformation));

        if(timer == nullptr){
            timer = new QTimer(this);
            timer->setInterval(10000);
        }
        connect(timer,&QTimer::timeout,[=](){
           onClosed();
        });
        timer->start();
        this->adjustSize();
        qApp->processEvents();
        int x = QApplication::desktop()->geometry().width()-(this->sizeHint().width()+10);
        int y = 40;

        QPropertyAnimation *a = new QPropertyAnimation(this,"pos");
        a->setDuration(200);
        a->setStartValue(QApplication::desktop()->mapToGlobal(QPoint(x+this->width(),y)));
        a->setEndValue(QApplication::desktop()->mapToGlobal(QPoint(x,y)));
        a->setEasingCurve(QEasingCurve::Linear);
        a->start(QPropertyAnimation::DeleteWhenStopped);
        this->show();
    }

protected slots:
    void onClosed()
    {
        hide();
        timer->disconnect();
        timer->stop();
    }

protected:
    void mouseReleaseEvent(QMouseEvent *event) override
    {
        QWidget::mouseReleaseEvent(event);
        if (event->button() == Qt::LeftButton) {
            emit notification_clicked();
            onClosed();
        }
    }
signals:
    void notification_clicked();
};

#endif // NOTIFICATIONPOPUP_H
