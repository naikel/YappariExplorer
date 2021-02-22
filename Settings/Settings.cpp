#include "Settings.h"

#define SETTINGS_FILE   "settings.json"

#include <QStandardPaths>
#include <QFile>
#include <QDebug>
#include <QDir>

Settings *Settings::settings {};

Settings::Settings()
{
    qDebug() << "Settings::Settings initializing";

    QString settingsPath = getSettingsFilePath();

    QFile file(settingsPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Settings::Settings couldn't open JSON settings (maybe it doesn't exist)";
        createDefaultSettings();
        return;
    }

    QByteArray contents = file.readAll();

    file.close();

    QJsonParseError error;
    QJsonDocument settingsJson = QJsonDocument::fromJson(contents, &error);

    if (error.error != QJsonParseError::NoError) {
        qDebug() << "Settings::Settings Error parsing JSON settings file: " + QString::number(error.error);
        createDefaultSettings();
        return;
    }

    readSettings(settingsJson);
    qDebug() << "Settings::Settings JSON settings file read successfully";

}

void Settings::saveGlobalSetting(QString setting, QJsonValue value)
{
    global.insert(setting, value);
}

QJsonValue Settings::readGlobalSetting(QString setting)
{
    return global.value(setting);
}

void Settings::savePaneSetting(int pane, QString setting, QJsonValue value)
{
    QJsonObject paneObject = panes[pane].toObject();
    paneObject.insert(setting, value);
    panes[pane] = paneObject;
}

QJsonValue Settings::readPaneSetting(int pane, QString setting)
{
    return panes[pane].toObject().value(setting);
}

void Settings::newTabSetting(int pane)
{
    QJsonArray tabsArray;
    savePaneSetting(pane, SETTINGS_TABS, tabsArray);
}

void Settings::saveTabSetting(int pane, int tab, QString setting, QJsonValue value)
{
    QJsonArray tabsArray = readPaneSetting(pane, SETTINGS_TABS).toArray();

    while (tabsArray.size() <= tab)
        tabsArray.append(QString());

    QJsonObject tabObject = tabsArray[tab].toObject();
    tabObject.insert(setting, value);
    tabsArray[tab] = tabObject;
    savePaneSetting(pane, SETTINGS_TABS, tabsArray);
}

QJsonValue Settings::readTabSetting(int pane, int tab, QString setting)
{
    QJsonArray tabsArray = readPaneSetting(pane, SETTINGS_TABS).toArray();

    return tabsArray[tab].toObject().value(setting);
}

void Settings::saveColumnSetting(int pane, int tab, int column, QString setting, QJsonValue value)
{
    QJsonArray columnsArray = readTabSetting(pane, tab, SETTINGS_COLUMNS).toArray();

    while (columnsArray.size() <= column)
        columnsArray.append(QString());

    QJsonObject columnObject = columnsArray[column].toObject();
    columnObject.insert(setting, value);
    columnsArray[column] = columnObject;
    saveTabSetting(pane, tab, SETTINGS_COLUMNS, columnsArray);
}

QJsonValue Settings::readColumnSetting(int pane, int tab, int column, QString setting)
{
    QJsonArray columnsArray = readTabSetting(pane, tab, SETTINGS_COLUMNS).toArray();

    return columnsArray[column].toObject().value(setting);
}

void Settings::save()
{
    QString settingsPath = getSettingsFilePath();

    QFile file(settingsPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug() << "Settings::Settings couldn't save JSON settings" << settingsPath;
        return;
    }

    file.write(getJsonDocument());
    file.close();

    qDebug() << "Settings::Settings JSON settings saved successfully" << settingsPath;
}

QString Settings::getSettingsFilePath()
{
    QString path;

    path = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);

    QDir settingsDir(path);

    if (!settingsDir.exists()) {
        if (!settingsDir.mkpath(path)) {
            qDebug() << "Settings::getSettingsFilePath couldn't create settings directory" << path;
        }
    }

    path = path + "/" + SETTINGS_FILE;

    return path;
}

void Settings::readSettings(QJsonDocument settingsJson)
{
    QJsonObject mainDocument = settingsJson.object();

    global = mainDocument.value(SETTINGS_GLOBAL).toObject();

    if (global.isEmpty())
        createGlobalDefaultSettings();

    panes = mainDocument.value(SETTINGS_PANES).toArray();

    if (panes.isEmpty())
        createPanesDefaultSettings();
}

QByteArray Settings::getJsonDocument()
{
    QJsonObject mainDocument;
    mainDocument.insert(SETTINGS_GLOBAL, global);
    mainDocument.insert(SETTINGS_PANES, panes);

    QJsonDocument settingsJson;
    settingsJson.setObject(mainDocument);

    return settingsJson.toJson(QJsonDocument::Indented);
}

void  Settings::createDefaultSettings()
{
    qDebug() << "Settings::createDefaultSettings";

    createGlobalDefaultSettings();
    createPanesDefaultSettings();

    qDebug() << getJsonDocument().toStdString().c_str();
}

void Settings::createGlobalDefaultSettings()
{
    global.insert(SETTINGS_GLOBAL_EXPLORERS, 2);
    global.insert(SETTINGS_GLOBAL_X, -1);
    global.insert(SETTINGS_GLOBAL_Y, -1);
    global.insert(SETTINGS_GLOBAL_WIDTH, -1);
    global.insert(SETTINGS_GLOBAL_HEIGHT, -1);

}

void Settings::createPanesDefaultSettings()
{
    QJsonObject defaultPane;
    defaultPane.insert(SETTINGS_PANES_TREEWIDTH, 400);

    QJsonObject defaultTab;
    QJsonArray columns;
    QJsonObject column;

    column.insert(SETTINGS_COLUMNS_VISUALINDEX, 0);
    column.insert(SETTINGS_COLUMNS_WIDTH, 600);
    columns.append(column);
    column.insert(SETTINGS_COLUMNS_VISUALINDEX, 1);
    column.insert(SETTINGS_COLUMNS_WIDTH, 100);
    columns.append(column);
    column.insert(SETTINGS_COLUMNS_VISUALINDEX, 2);
    column.insert(SETTINGS_COLUMNS_WIDTH, 230);
    columns.append(column);

    defaultTab.insert(SETTINGS_COLUMNS, columns);
    defaultTab.insert(SETTINGS_TABS_SORTCOLUMN, 1);
    defaultTab.insert(SETTINGS_TABS_SORTASCENDING, true);

    QJsonArray tabs;
    tabs.append(defaultTab);

    defaultPane.insert(SETTINGS_PANES_TABSCOUNT, 1);
    defaultPane.insert(SETTINGS_TABS, tabs);

    panes.append(defaultPane);
    panes.append(defaultPane);
}
