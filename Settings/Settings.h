#ifndef SETTINGS_H
#define SETTINGS_H

#include <QJsonDocument>
#include <QJsonObject>
#include <QString>

// Settings

#define SETTINGS_PANES                      "panes"

// Global Settings
#define SETTINGS_GLOBAL                     "global"
#define SETTINGS_GLOBAL_X                   "x"
#define SETTINGS_GLOBAL_Y                   "y"
#define SETTINGS_GLOBAL_WIDTH               "width"
#define SETTINGS_GLOBAL_HEIGHT              "height"
#define SETTINGS_GLOBAL_MAXIMIZED           "maximized"
#define SETTINGS_GLOBAL_SCREEN              "screen"

// Columns settings
#define SETTINGS_COLUMNS                    "columns"
#define SETTINGS_COLUMNS_WIDTH              "width"

class Settings
{
public:
    Settings();

    void saveGlobalSetting(QString setting, QJsonValue value);
    QJsonValue readGlobalSetting(QString setting);
    int readIntGlobalSetting(QString setting);
    bool readBoolGlobalSetting(QString setting);
    void save();

    static Settings *settings;

private:
    QJsonDocument settingsJson;

    QString getSettingsFilePath();
    void createDefaultSettings();
};

#endif // SETTINGS_H
