#ifndef SETTINGS_H
#define SETTINGS_H

#include <QDebug>
#include <QStandardPaths>
#include <QMessageBox>
#include <QTextCodec>
#include <QApplication>
#include <QSettings>
#include <QObject>


class settings : public QObject
{
    Q_OBJECT
public:
    explicit settings(QObject *parent = nullptr);
    QSettings settingsObj;
    QString setting_path;
signals:
    void dynamicTheme(bool);

public slots:
    void changeSaveAfterSetting(bool);

    void changeShowSearchSuggestion(bool checked);
    void changeDynamicTheme(bool checked);
    void changeMiniModeStayOnTop(bool checked);
    void changeZoom(QString zoom);
    void changeMiniModeTransperancy(int val);
    void changeSystemTitlebar(bool checked);
    void changeAppTransperancy(int val);
    void changeCrossFadeSetting(bool checked);
    void changeVisualizerSetting(bool checked);
    void changeMpris(bool checked);
    void changeSmartPlaylist(bool checked);
};

#endif // SETTINGS_H
