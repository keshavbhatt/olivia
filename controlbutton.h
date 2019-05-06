#ifndef CONTROLBUTTON_H
#define CONTROLBUTTON_H

#include <QObject>
#include <QPushButton>

class controlButton : public QPushButton
{
    Q_OBJECT
public:
    controlButton(QWidget *parent = 0);

signals:

public slots:

protected slots:
    void mouseMoveEvent(QMouseEvent *e);
    bool eventFilter(QObject *obj, QEvent *event);
};

#endif // CONTROLBUTTON_H
