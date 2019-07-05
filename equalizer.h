#ifndef EQUALIZER_H
#define EQUALIZER_H

#include <QWidget>
#include <QDebug>
#include <QSlider>
#include <QEvent>

#include <QSettings>
#include <QCloseEvent>

namespace Ui {
class equalizer;
}

class equalizer : public QWidget
{
    Q_OBJECT

public:
    explicit equalizer(QWidget *parent = nullptr);
    ~equalizer();
    QString eq_args,previous_eq_args;


signals:
void disable_eq();

public slots:
    void removeStyle();
    void loadSettings();
    void setRange();

    void triggerEq();
signals:
    void update_eq(QString);

protected slots:
    bool eventFilter(QObject *obj, QEvent *event);

    void closeEvent(QCloseEvent *event);
private slots:

    void updateEqVal();

    QString validBandVal(QSlider *band);

    void on_reset_master_clicked();

    void on_reset_super_clicked();

    void on_fake_valueChanged(int value);

    void setBandLabel(QSlider *slider);
private:
    Ui::equalizer *ui;
    QSettings *settingsObj = nullptr;
    QString setting_path;



};


#endif // EQUALIZER_H
