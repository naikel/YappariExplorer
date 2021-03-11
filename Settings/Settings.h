#ifndef SETTINGS_H
#define SETTINGS_H

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QString>

// Settings

// Global Settings
#define SETTINGS_GLOBAL                     "global"
#define SETTINGS_GLOBAL_X                   "x"
#define SETTINGS_GLOBAL_Y                   "y"
#define SETTINGS_GLOBAL_WIDTH               "width"
#define SETTINGS_GLOBAL_HEIGHT              "height"
#define SETTINGS_GLOBAL_MAXIMIZED           "maximized"
#define SETTINGS_GLOBAL_SCREEN              "screen"
#define SETTINGS_GLOBAL_SPLITTER_SIZES      "splittersizes"
#define SETTINGS_GLOBAL_EXPLORERS           "explorers"

// Panes settings
#define SETTINGS_PANES                      "panes"
#define SETTINGS_PANES_TREEWIDTH            "treewidth"
#define SETTINGS_PANES_TABSCOUNT            "tabscount"
#define SETTINGS_PANES_CURRENTTAB           "currenttab"

// Tab Settings
#define SETTINGS_TABS                       "tabs"
#define SETTINGS_TABS_PATH                  "path"
#define SETTINGS_TABS_SORTCOLUMN            "sortcolumn"
#define SETTINGS_TABS_SORTASCENDING         "sortascending"
#define SETTINGS_TABS_DISPLAYNAME           "displayname"
#define SETTINGS_TABS_ICON                  "icon"

// Columns settings
#define SETTINGS_COLUMNS                    "columns"
#define SETTINGS_COLUMNS_WIDTH              "width"
#define SETTINGS_COLUMNS_VISUALINDEX        "visualindex"

class Settings
{
public:
    Settings();

    void saveGlobalSetting(QString setting, QJsonValue value);
    QJsonValue readGlobalSetting(QString setting);

    void savePaneSetting(int pane, QString setting, QJsonValue value);
    QJsonValue readPaneSetting(int pane, QString setting);

    void newTabSetting(int pane);
    void saveTabSetting(int pane, int tab, QString setting, QJsonValue value);
    QJsonValue readTabSetting(int pane, int tab, QString setting);

    void saveColumnSetting(int pane, int tab, int column, QString setting, QJsonValue value);
    QJsonValue readColumnSetting(int pane, int tab, int column, QString setting);

    void save();

    static Settings *settings;

private:
    // Sections
    QJsonObject global;
    QJsonArray panes;

    QString getSettingsFilePath();
    void readSettings(QJsonDocument settingsJson);
    QByteArray getJsonDocument();

    void createDefaultSettings();
    void createGlobalDefaultSettings();
    void createPanesDefaultSettings();
};

#endif // SETTINGS_H
