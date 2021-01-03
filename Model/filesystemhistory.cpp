#include <QDebug>

#include "filesystemhistory.h"

FileSystemHistory::FileSystemHistory(QObject *parent) : QObject(parent)
{

}

FileSystemHistory::~FileSystemHistory()
{
    for (HistoryEntry *entry : qAsConst(pathList))
        delete entry;
}

bool FileSystemHistory::isCursorAtTheEnd()
{
    qDebug() << "FileSystemHistory::isCursorAtTheEnd" << cursor << pathList.size();
    return (cursor == (pathList.size() - 1));
}

void FileSystemHistory::insert(QString path, QIcon icon)
{
    // Check if we're just trying to reinsert the same path the cursor is at
    if (cursor >= 0 && cursor < pathList.size() && (path == pathList.at(cursor)->path))
        return;

    if (!isCursorAtTheEnd()) {

        int itemsToRemove = pathList.size() - cursor - 1;
        while (itemsToRemove-- > 0) {
            HistoryEntry *oldEntry = pathList.takeLast();
            delete oldEntry;
        }
    }

    if (pathList.size() >= 10) {
        --cursor;
        pathList.removeFirst();
    }

    HistoryEntry *newEntry = new HistoryEntry;
    newEntry->path = path;
    newEntry->icon = icon;

    pathList.append(newEntry);
    cursor++;
    qDebug() << "FileSystemHistory::insert done" << newEntry->path << "pos" << cursor;
}

bool FileSystemHistory::canGoForward()
{
    return (pathList.size() > 0 && !isCursorAtTheEnd());
}

bool FileSystemHistory::canGoBack()
{
    return (pathList.size() > 0 && cursor > 0);
}

QString FileSystemHistory::getLastItem()
{
    qDebug() << "FileSystemHistory::getLastItem" << cursor;
    if (canGoBack()) {
        return pathList.at(--cursor)->path;
    }

    return QString();
}

QString FileSystemHistory::getNextItem()
{
    qDebug() << "FileSystemHistory::getNextItem" << cursor;
    if (canGoForward()) {
        return pathList.at(++cursor)->path;
    }

    return QString();
}
