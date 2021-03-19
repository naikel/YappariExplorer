#include <QSortFilterProxyModel>
#include <QDebug>

#include "FileSystemHistory.h"

FileSystemHistory::FileSystemHistory(QAbstractItemModel *model, QObject *parent) : QObject(parent)
{
    QSortFilterProxyModel *proxyModel = reinterpret_cast<QSortFilterProxyModel *>(model);

    this->model = reinterpret_cast<FileSystemModel *>(proxyModel->sourceModel());
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

void FileSystemHistory::insert(QString displayName, QIcon icon, QString path)
{
    // Check if we're just trying to reinsert the same path the cursor is at
    if (cursor >= 0 && cursor < pathList.size() && (path == pathList.at(cursor)->path))
        return;

    // If the path is already in history, remove it, so it gets added at the end with a new icon and displayName (they could have changed)
    int row     {};
    bool found  {};
    while (row < pathList.size()) {
        HistoryEntry *entry = pathList.at(row);
        if (entry->path == path) {
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
    newEntry->path =        path;
    newEntry->icon =        icon;
    newEntry->displayName = displayName;

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

bool FileSystemHistory::canGoUp()
{
    QString path = pathList.at(cursor)->path;

    QModelIndex index = model->index(path);

    if (!index.isValid())
        return false;

    return (index.parent().isValid());
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

QString FileSystemHistory::getParentItem()
{
    if (canGoUp()) {
        return model->index(pathList.at(cursor)->path).parent().data(FileSystemModel::PathRole).toString();
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
