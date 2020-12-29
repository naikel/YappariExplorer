#include <QDebug>

#include "filesystemhistory.h"

FileSystemHistory::FileSystemHistory(QObject *parent) : QObject(parent)
{

}

bool FileSystemHistory::isCursorAtTheEnd()
{
    qDebug() << "FileSystemHistory::isCursorAtTheEnd" << cursor << pathList.size() << pathList;
    return (cursor == (pathList.size() - 1));
}

void FileSystemHistory::insert(QString path)
{
    // Check if we're just trying to reinsert the same path the cursor is at
    if (cursor >= 0 && cursor < pathList.size() && (path == pathList.at(cursor)))
        return;

    if (!isCursorAtTheEnd()) {

        int itemsToRemove = pathList.size() - cursor - 1;
        while (itemsToRemove-- > 0)
            pathList.removeLast();
    }

    pathList.append(path);
    cursor++;
    qDebug() << "FileSystemHistory::insert done" << path << cursor << pathList;
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
    qDebug() << "FileSystemHistory::getLastItem" << cursor << pathList;
    if (canGoBack()) {
        return pathList.at(--cursor);
    }

    return QString();
}

QString FileSystemHistory::getNextItem()
{
    qDebug() << "FileSystemHistory::getNextItem" << cursor << pathList;
    if (canGoForward()) {
        return pathList.at(++cursor);
    }

    return QString();
}
