#include "youtube.h"

Youtube::Youtube(QObject *parent) : QObject(parent)
{
    setParent(parent);
    setting_path =  QStandardPaths::writableLocation(QStandardPaths::DataLocation);
}

QString Youtube::getCurrentCountry(){
    return settings.value("country").toString().toUpper();
}

void Youtube::saveGeo(QString country){
    settings.setValue("country",country.toLower());
    emit setCountry(settings.value("country").toString().toUpper());
}

