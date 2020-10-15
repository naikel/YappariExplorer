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
