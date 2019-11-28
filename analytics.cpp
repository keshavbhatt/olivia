#include "analytics.h"
#include <QDateTime>
#include <QSysInfo>

analytics::analytics(QObject *parent) : QObject(parent)
{
   startTime = QDateTime::currentMSecsSinceEpoch();
   qDebug()<<QSysInfo::currentCpuArchitecture()<<"\n"<<QSysInfo::kernelType()<<QSysInfo::kernelVersion()<<"\n"<<QSysInfo::prettyProductName();
}

QString analytics::getData(){
    qint64 appDuration = QDateTime::currentMSecsSinceEpoch() - startTime;
    return QString::number(appDuration);
}
