#include "equalizer.h"
#include "ui_equalizer.h"
#include <QDir>
#include <QStandardPaths>



equalizer::equalizer(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::equalizer)
{
    ui->setupUi(this);
    ui->fake->hide();

    setting_path=  QStandardPaths::writableLocation(QStandardPaths::DataLocation);

    if(!QDir(setting_path).exists()){
        QDir d(setting_path);
        d.mkpath(setting_path);
    }
    settingsObj = new QSettings(setting_path+"/eq.conf",QSettings::NativeFormat,nullptr);
}

//remove widget style
void equalizer::removeStyle(){
    ui->ig1->setStyleSheet("background:transparent;");
    ui->ig2->setStyleSheet("background:transparent;");
    ui->ig3->setStyleSheet("background:transparent;");
    ui->ig4->setStyleSheet("background:transparent;");
}


//set initial range for all bands + some other stuffs
void equalizer::setRange(){

    //set up tempo slider
    ui->tempo->setRange(0,20);
    connect(ui->tempo,SIGNAL(sliderReleased()),this,SLOT(updateEqVal()));


    //set up balance slider
    ui->balance->setRange(0,20);
    connect(ui->balance,SIGNAL(sliderReleased()),this,SLOT(updateEqVal()));

    //set range for masterEQ bands
    QList<QSlider*> sliders_list;
    sliders_list = ui->groupBox->findChildren<QSlider*>();
    foreach(QSlider *slider , sliders_list){
        if(slider->objectName()!="balance"&&slider->objectName()!="tempo" &&slider->objectName()!="fake"){
            slider->setTickPosition(QSlider::TicksRight);
            slider->setRange(-10,10);
            connect(slider,SIGNAL(sliderReleased()),this,SLOT(updateEqVal()));
        }
        slider->installEventFilter(this);
    }

    //set range for superEQ bands
    QList<QSlider*> sliders_list2;
    sliders_list2 = ui->groupBox_2->findChildren<QSlider*>();
    foreach(QSlider *slider , sliders_list2){
        slider->setTickPosition(QSlider::TicksRight);
        slider->setRange(0,20);
        connect(slider,SIGNAL(sliderReleased()),this,SLOT(updateEqVal()));
        slider->installEventFilter(this);
    }




    //set label width mEQ
    QList<QLabel*> labels_list;
    labels_list = ui->groupBox->findChildren<QLabel*>();
    foreach(QLabel *label , labels_list){
        if(!label->objectName().contains("ig")){
            QString labelText = label->text();
            int fSize = label->fontInfo().pointSize();
            label->setFont(QFont(label->font().family(),10));
            label->setText("0000000");
            int totalWidth = label->fontMetrics().boundingRect(label->text()).width();
            int oneCharWidth = totalWidth/7;
            if(labelText.length()<7){
                label->setMinimumWidth(oneCharWidth*label->text().length()+oneCharWidth*2);
            }
            label->setFont(QFont(label->font().family(),fSize));
            label->setText("<p align=\"center\" ><span style=\"font-size:10pt;\">"+labelText+"</span></p>");
        }
    }

    //set label width sEQ
    QList<QLabel*> labels_list2;
    labels_list2 = ui->groupBox_2->findChildren<QLabel*>();
    foreach(QLabel *label , labels_list2){
        QString labelText = label->text();
        int fSize = label->fontInfo().pointSize();
        label->setFont(QFont(label->font().family(),10));
        label->setText("0000");
        int totalWidth = label->fontMetrics().boundingRect(label->text()).width();
        int oneCharWidth = totalWidth/4;
        if(labelText.length()<4){
            label->setMinimumWidth(oneCharWidth*label->text().length()+oneCharWidth*2);
        }
        label->setFont(QFont(label->font().family(),fSize));
        label->setText("<p align=\"center\" ><span style=\"font-size:10pt;\">"+labelText+"</span></p>");
    }

    connect(ui->eq_checkBox,&QCheckBox::toggled,[=](bool checked){
        settingsObj->setValue("equalizer_enabled",checked);

        if(!checked){
            ui->groupBox->setEnabled(false);
            ui->groupBox_2->setEnabled(false);
            emit disable_eq();
        }else{
           ui->groupBox->setEnabled(true);
           ui->groupBox_2->setEnabled(true);
           triggerEq();
        }
    });
}

bool equalizer::eventFilter(QObject *obj, QEvent *event)
{
    if(event->type() == QEvent::Wheel && (obj->objectName().contains("m")||obj->objectName().contains("b")))
    {
        //block mouse wheel event
        return true;
    }
    return false;
}


void equalizer::closeEvent(QCloseEvent *event){
    QList<QSlider*> sliders_list;
    sliders_list = this->findChildren<QSlider*>();
    foreach(QSlider *slider , sliders_list){
        settingsObj->setValue(slider->objectName(),slider->value());
    }
    event->accept();
}

//prepare the eq args to be sent to radio(player)
void equalizer::updateEqVal(){

    QSlider *slider = qobject_cast<QSlider*>(sender());
    setBandLabel(slider);
    eq_args.clear();

    //tempo
    if(ui->tempo->value()>0){
        double constant = 10;
        double d_val = double(static_cast<double>(ui->tempo->value())/constant);
        eq_args += "scaletempo=scale="+QString::number(d_val)+",";
    }

    //balance
    if(ui->balance->value()>10 || ui->balance->value()<10){
        double constant = 10;
        double constant2 = 100;
        int val = ui->balance->value();
        double c1v = 0.0,c2v = 0.0;
        QString c2vStr,c1vStr;
        if(val<10){
             val = 10 - val;
             c1v = double(static_cast<double>(val)/constant);
             if(val==0){
                 c1vStr = "1";
             }else{
                 c1vStr = QString::number(c1v);
             }
             double c2vCores = double(1.0) - c1v ;
             if(val==0){
                 c2vStr = "0";
             }else{
                 c2vStr = QString::number(c2vCores);
             }
        }else{
             c2v = double(static_cast<double>(val)/constant2);
             if(val==20){
                 c2vStr = "1";
             }else if(val!=0){
                 c2vStr = "0."+QString::number(c2v).split(".1").last();
             }
             double c1vCores = double(0.9) - c2v ;
             if(val == 20 ){
                 c1vStr = "0";
             }else{
                 c1vStr = "0."+QString::number(c1vCores).split(".7").last();
             }
        }
        eq_args += "lavfi=[pan=stereo|c0="+c1vStr+"*c0|c1="+c2vStr+"*c1]"+",";
    }


    //prepare master EQ bands args
    // QString bandName = slider->objectName().split("b").last();
    eq_args +="equalizer=f=64:width_type=o:w=3.3:g="+QString::number(ui->m1->value())+","+
              "equalizer=f=400:width_type=o:w=2.0:g="+QString::number(ui->m2->value())+","+
              "equalizer=f=1250:width_type=o:w=1.3:g="+QString::number(ui->m3->value())+","+
              "equalizer=f=2830:width_type=o:w=1.0:g="+QString::number(ui->m4->value())+","+
              "equalizer=f=5600:width_type=o:w=1.0:g="+QString::number(ui->m5->value())+","+
              "equalizer=f=12500:width_type=o:w=1.3:g="+QString::number(ui->m6->value());

    //prepare super EQ bands args
    QString superEq =
            validBandVal(ui->b1)+
            validBandVal(ui->b2)+
            validBandVal(ui->b3)+
            validBandVal(ui->b4)+
            validBandVal(ui->b5)+
            validBandVal(ui->b6)+
            validBandVal(ui->b7)+
            validBandVal(ui->b8)+
            validBandVal(ui->b9)+
            validBandVal(ui->b10)+
            validBandVal(ui->b11)+
            validBandVal(ui->b12)+
            validBandVal(ui->b13)+
            validBandVal(ui->b14)+
            validBandVal(ui->b15)+
            validBandVal(ui->b16)+
            validBandVal(ui->b17)+
            validBandVal(ui->b18);

    //append superEq args to masterEq args if superEq have valid args
    if(!superEq.isEmpty()){
        eq_args += ",superequalizer="+superEq;
    }

    //check superEQ enabled ?
    if(eq_args.contains("superequalizer")){
        ui->groupBox_2->setTitle("Super Audio Filters (Enabled)");
    }else{
        ui->groupBox_2->setTitle("Super Audio Filters (Disabled)");
    }

    //check masterEQ enabled ?
    QList<QSlider*> sliders_list;
    sliders_list = ui->groupBox->findChildren<QSlider*>();
    bool mEqEnabled = false;
    foreach(QSlider *slider , sliders_list){
        if(slider->objectName()!="tempo" && slider->objectName()!="balance"){
            if(slider->value() != 0 ){
                mEqEnabled = true;
                break;
            }
        }
    }
    if(mEqEnabled){
        ui->groupBox->setTitle("Master Equalizers (Enabled)");
    }else{
        ui->groupBox->setTitle("Master Equalizers (Disabled)");
    }

    //remove ':' from the eq_args in superEq args if present at last
    if(eq_args.at(eq_args.length()-1)==':'){
        eq_args.chop(1);
    }
    if(settingsObj->value("equalizer_enabled",false).toBool()){
        emit update_eq(eq_args);
    }
}

//disable arg for band which is set to zero in superEQ only
QString equalizer::validBandVal(QSlider *band){
    QString args;
    QString bandName = band->objectName().split("b").last()+"b";
    if(band->value()!=0){
        if(bandName.contains("18")){
            args += bandName+"="+QString::number(band->value());
        }else {
            args += bandName+"="+QString::number(band->value())+":";
        }
    }
    else
        args += "";
    return args;
}

equalizer::~equalizer()
{
    delete ui;
}


void equalizer::on_reset_master_clicked()
{
    //set range for masterEQ bands
    QList<QSlider*> sliders_list;
    sliders_list = ui->groupBox->findChildren<QSlider*>();
    foreach(QSlider *slider , sliders_list){
        if(slider->objectName()!="balance"&&slider->objectName()!="tempo"){
           slider->setValue(0);
           setBandLabel(slider);
           settingsObj->setValue(slider->objectName(),slider->value());
        }
    }
    ui->tempo->setValue(10);
    setBandLabel(ui->tempo);

    ui->balance->setValue(10);
    setBandLabel(ui->balance);

    settingsObj->setValue(ui->tempo->objectName(),ui->tempo->value());
    settingsObj->setValue(ui->balance->objectName(),ui->balance->value());
    triggerEq();
}



void equalizer::on_reset_super_clicked()
{
    //set range for masterEQ bands
    QList<QSlider*> sliders_list;
    sliders_list = ui->groupBox_2->findChildren<QSlider*>();
    foreach(QSlider *slider , sliders_list){
           slider->setValue(0);
           setBandLabel(slider);
           settingsObj->setValue(slider->objectName(),slider->value());
    }
    triggerEq();
}

void equalizer::loadSettings(){

    //load master eq
    QList<QSlider*> sliders_list;
    sliders_list = ui->groupBox->findChildren<QSlider*>();
    foreach(QSlider *slider , sliders_list){
        if(slider->objectName()!="balance"&&slider->objectName()!="tempo"){
             slider->blockSignals(true);//block slider value changed cause it will cause eq_trigger
             slider->setValue(settingsObj->value(slider->objectName(),0).toInt());
             setBandLabel(slider);
             slider->blockSignals(false);
         }
    }

    //load super eq
    QList<QSlider*> sliders_list2;
    sliders_list2 = ui->groupBox_2->findChildren<QSlider*>();
    foreach(QSlider *slider , sliders_list2){
             slider->blockSignals(true);//block slider value changed cause it will cause eq_trigger
             slider->setValue(settingsObj->value(slider->objectName(),0).toInt());
             setBandLabel(slider);
             slider->blockSignals(false);

     }

    ui->balance->blockSignals(true);
    ui->tempo->blockSignals(true);

    ui->tempo->setValue(settingsObj->value(ui->tempo->objectName(),10).toInt());
    setBandLabel(ui->tempo);
    ui->balance->setValue(settingsObj->value(ui->balance->objectName(),10).toInt());
    setBandLabel(ui->balance);

    ui->tempo->blockSignals(false);
    ui->balance->blockSignals(false);



    ui->groupBox->setEnabled(settingsObj->value("equalizer_enabled",false).toBool());
    ui->groupBox_2->setEnabled(settingsObj->value("equalizer_enabled",false).toBool());

    ui->eq_checkBox->blockSignals(true);
    ui->eq_checkBox->setChecked(settingsObj->value("equalizer_enabled",false).toBool());
    ui->eq_checkBox->blockSignals(false);

    qDebug()<<"loaded eq settings";
}

void equalizer::on_fake_valueChanged(int value)
{
    Q_UNUSED(value);
    updateEqVal();
}

void equalizer::triggerEq(){
    qDebug()<<"trigger eq called, eq enabled = "<<settingsObj->value("equalizer_enabled",false).toBool();
    if(settingsObj->value("equalizer_enabled",false).toBool()){
        int val = rand() % ((999 - 0) + 1) + 0;
        ui->fake->setValue(val);
    }
}

void equalizer::setBandLabel(QSlider* slider){
    int val = slider->value();

    if(slider->objectName().contains("m") && !slider->objectName().contains("tempo")){
     QString bandLabelName = "ml"+slider->objectName().split("m").last();
     if(ui->groupBox->findChild<QLabel*>(bandLabelName)!=nullptr){
           ui->groupBox->findChild<QLabel*>(bandLabelName)->setText("<span style=\"font-size:10pt;\">"+QString::number(val)+"dB</span>");
     }
    } else{
     QString bandLabelName = "bl"+slider->objectName().split("b").last();
     if(ui->groupBox_2->findChild<QLabel*>(bandLabelName)!=nullptr){
           ui->groupBox_2->findChild<QLabel*>(bandLabelName)->setText("<span style=\"font-size:10pt;\">"+QString::number(val)+"dB</span>");
     }
    }

    //tempo
    if(slider->objectName()=="tempo" && slider->value()>0){
        double constant = 10;
        double d_val = double(static_cast<double>(ui->tempo->value())/constant);
        if(QString::number(d_val)=="1"){
            ui->ig2->setText("<p align=\"center\" ><span style=\"font-size:10pt;\">Tempo(Normal)</span></p>");
        }else{
            ui->ig2->setText("<p align=\"center\" ><span style=\"font-size:10pt;\">Tempo("+QString::number(d_val)+")</span></p>");
        }
    }

    //balance
    if(slider->objectName()=="balance"){
        if(ui->balance->value()>10 || ui->balance->value()<10){
            double constant = 10;
            double constant2 = 100;
            int val = ui->balance->value();
            double c1v = 0.0,c2v = 0.0;
            QString c2vStr,c1vStr;


            if(val<10){
                 val = 10 - val;
                 c1v = double(static_cast<double>(val)/constant);
                 if(val==0){
                     c1vStr = "1";
                 }else{
                     c1vStr = QString::number(c1v);
                 }
                 double c2vCores = double(1.0) - c1v ;
                 if(val==0){
                     c2vStr = "0";
                 }else{
                     c2vStr = QString::number(c2vCores);
                 }
            }else{
                 c2v = double(static_cast<double>(val)/constant2);
                 if(val==20){
                     c2vStr = "1";
                 }else if(val!=0){
                     c2vStr = "0."+QString::number(c2v).split(".1").last();
                 }
                 double c1vCores = double(0.9) - c2v ;
                 if(val == 20 ){
                     c1vStr = "0";
                 }else{
                     c1vStr = "0."+QString::number(c1vCores).split(".7").last();
                 }
            }
            ui->balanceL->setText("<p align=\"center\" ><span style=\"font-size:10pt;\">L("+c1vStr+")</span></p>");
            ui->balanceR->setText("<p align=\"center\" ><span style=\"font-size:10pt;\">R("+c2vStr+")</span></p>");
        }else{
            //center
            ui->balanceL->setText("<p align=\"center\" ><span style=\"font-size:10pt;\">L(1)</span></p>");
            ui->balanceR->setText("<p align=\"center\" ><span style=\"font-size:10pt;\">R(1)</span></p>");
        }
    }
}
