#include <QDebug>

#include "directorywatcher.h"

DirectoryWatcher::DirectoryWatcher(QObject *parent) : QObject(parent)
{
}

void DirectoryWatcher::addPath(QString path)
{
    Q_UNUSED(path)
}

void DirectoryWatcher::removePath(QString path)
{
    Q_UNUSED(path)
}

bool DirectoryWatcher::handleNativeEvent(const QByteArray &eventType, void *message, long *result)
{
    Q_UNUSED(eventType)
    Q_UNUSED(message)
    Q_UNUSED(result)

    return false;
}
