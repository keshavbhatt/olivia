#include "settings.h"


settings::settings(QObject *parent) :QObject(parent)
{
    settingsObj.setObjectName("settings");
    setting_path =  QStandardPaths::writableLocation(QStandardPaths::DataLocation);

    settingsObj.setValue("AppActivation-Code","A_scNo.2335");
}

void settings::changeSaveAfterSetting(bool checked){
    settingsObj.setValue("saveAfterBuffer",checked);
}

void settings::changeShowSearchSuggestion(bool checked){
    settingsObj.setValue("showSearchSuggestion",checked);
}

void settings::changeEqualizerSetting(bool checked){
    settingsObj.setValue("equalizer",checked);
}

void settings::changeDynamicTheme(bool checked){
    settingsObj.setValue("dynamicTheme",checked);
     emit dynamicTheme(checked);
}

void settings::changeMiniModeStayOnTop(bool checked){
    settingsObj.setValue("miniModeStayOnTop",checked);
}

void settings::changeZoom(QString zoom){
    settingsObj.setValue("zoom",zoom);
}

void settings::changeAppTransperancy(int val){
    settingsObj.setValue("appTransperancy",val);
}

void settings::changeMiniModeTransperancy(int val){
    settingsObj.setValue("miniModeTransperancy",val);
}

void settings::changeSystemTitlebar(bool checked){
    settingsObj.setValue("systemTitlebar",checked);
}


