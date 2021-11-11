#include <QDebug>

#include "DirectoryWatcher.h"

DirectoryWatcher::DirectoryWatcher(QObject *parent) : QObject(parent)
{
}

bool DirectoryWatcher::handleNativeEvent(const QByteArray &eventType, void *message, long *result)
{
    Q_UNUSED(eventType)
    Q_UNUSED(message)
    Q_UNUSED(result)

    return false;
}
