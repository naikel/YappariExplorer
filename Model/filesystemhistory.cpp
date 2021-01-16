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

void FileSystemHistory::insert(FileSystemItem *item)
{
    // Check if we're just trying to reinsert the same path the cursor is at
    if (cursor >= 0 && cursor < pathList.size() && (item->getPath() == pathList.at(cursor)->path))
        return;

    // If the path is already in history, remove it, so it gets added at the end with a new icon and displayName (they could have changed)
    int row     {};
    bool found  {};
    while (row < pathList.size()) {
        HistoryEntry *entry = pathList.at(row);
        if (entry->path == item->getPath()) {
            found = true;
            break;
        }
        row++;
    }

    if (found) {
        pathList.removeAt(row);
        cursor--;
    }

    // If the cursor is not at the end we discard all forward history from the cursor
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
    newEntry->path =        item->getPath();
    newEntry->icon =        item->getIcon();
    newEntry->displayName = item->getDisplayName();

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

QString FileSystemHistory::getItemAtPos(int pos)
{
    if (pos >= 0 && pos < pathList.size()) {
        cursor = pos;
        return pathList.at(cursor)->path;
    }

    return QString();
}

int FileSystemHistory::getCursor() const
{
    return cursor;
}

QList<HistoryEntry *>& FileSystemHistory::getPathList()
{
    return pathList;
}
