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
    settingsJson = QJsonDocument::fromJson(contents, &error);

    if (error.error != QJsonParseError::NoError) {
        qDebug() << "Settings::Settings Error parsing JSON settings file: " + QString::number(error.error);
        createDefaultSettings();
        return;
    }

    qDebug() << "Settings::Settings JSON settings file read successfully";

}

void Settings::saveGlobalSetting(QString setting, QJsonValue value)
{
    QJsonObject mainDocument = settingsJson.object();
    QJsonObject global = mainDocument.value(SETTINGS_GLOBAL).toObject();

    global.insert(setting, value);
    mainDocument.insert(SETTINGS_GLOBAL, global);

    settingsJson.setObject(mainDocument);
}

QJsonValue Settings::readGlobalSetting(QString setting)
{
    QJsonObject mainDocument = settingsJson.object();
    QJsonObject global = mainDocument.value(SETTINGS_GLOBAL).toObject();

    return global.value(setting);
}

int Settings::readIntGlobalSetting(QString setting)
{
    return readGlobalSetting(setting).toInt();
}

bool Settings::readBoolGlobalSetting(QString setting)
{
    return readGlobalSetting(setting).toBool();
}

void Settings::save()
{
    QString settingsPath = getSettingsFilePath();

    QFile file(settingsPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug() << "Settings::Settings couldn't save JSON settings" << settingsPath;
        return;
    }

    QByteArray document = settingsJson.toJson(QJsonDocument::Indented);
    file.write(document);
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

void  Settings::createDefaultSettings()
{
    qDebug() << "Settings::createDefaultSettings";

    QJsonObject mainDocument;
    QJsonObject global;

    global.insert(SETTINGS_GLOBAL_X, -1);
    global.insert(SETTINGS_GLOBAL_Y, -1);
    global.insert(SETTINGS_GLOBAL_WIDTH, -1);
    global.insert(SETTINGS_GLOBAL_HEIGHT, -1);

    mainDocument.insert(SETTINGS_GLOBAL, global);

    settingsJson.setObject(mainDocument);

}
