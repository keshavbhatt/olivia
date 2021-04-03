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

void settings::changeCrossFadeSetting(bool checked){
    settingsObj.setValue("crossfade",checked);
}

void settings::changeVisualizerSetting(bool checked){
    settingsObj.setValue("visualizer",checked);
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

void settings::changeMpris(bool checked){
    settingsObj.setValue("mpris",checked);
}

void settings::changeSmartPlaylist(bool checked){
    settingsObj.setValue("smart_playlist",checked);
}

void settings::changeMarqueeSetting(bool checked)
{
    settingsObj.setValue("marquee",checked);
}


