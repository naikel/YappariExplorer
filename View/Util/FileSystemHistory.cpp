#include <QDebug>

#include "FileSystemHistory.h"

FileSystemHistory::FileSystemHistory(QObject *parent) : QObject(parent)
{

}

FileSystemHistory::~FileSystemHistory()
{
}

bool FileSystemHistory::isCursorAtTheEnd() const
{
    return (cursor == (indexList.size() - 1));
}

void FileSystemHistory::insert(const QModelIndex &index)
{
    // Check if we're just trying to reinsert the same index the cursor is at
    if (!index.isValid() || (cursor >= 0 && cursor < indexList.size() && (index == indexList.at(cursor))))
        return;

    // If the path is already in history, remove it, so it gets added at the end with a new icon and displayName (they could have changed)
    int row     {};
    bool found  {};
    while (row < indexList.size()) {
        QModelIndex entry = indexList.at(row);
        if (entry.data(FileSystemModel::PathRole).toString() == index.data(FileSystemModel::PathRole).toString()) {
            found = true;
            break;
        }
        row++;
    }

    if (found) {
        indexList.removeAt(row);
        cursor--;
    }

    // If the cursor is not at the end we discard all forward history from the cursor
    if (!isCursorAtTheEnd()) {

        int itemsToRemove = indexList.size() - cursor - 1;
        while (itemsToRemove-- > 0)
            indexList.takeLast();
    }

    if (indexList.size() >= 10) {
        --cursor;
        indexList.removeFirst();
    }

    indexList.append(index);
    cursor++;

    qDebug() << "FileSystemHistory::insert done" << index.data(FileSystemModel::PathRole).toString() << "pos" << cursor;
}

bool FileSystemHistory::canGoForward() const
{
    return (indexList.size() > 0 && !isCursorAtTheEnd());
}

bool FileSystemHistory::canGoBack() const
{
    return (indexList.size() > 0 && cursor > 0);
}

bool FileSystemHistory::canGoUp() const
{
    return (indexList.at(cursor).parent().isValid());
}

QModelIndex FileSystemHistory::getLastItem()
{
    qDebug() << "FileSystemHistory::getLastItem" << cursor;
    if (canGoBack()) {
        return indexList.at(--cursor);
    }

    return QModelIndex();
}

QModelIndex FileSystemHistory::getNextItem()
{
    qDebug() << "FileSystemHistory::getNextItem" << cursor;
    if (canGoForward()) {
        return indexList.at(++cursor);
    }

    return QModelIndex();
}

QModelIndex FileSystemHistory::getParentItem()
{
    if (canGoUp()) {
        return indexList.at(cursor).parent();
    }
    return QModelIndex();
}

QModelIndex FileSystemHistory::getItemAtPos(int pos)
{
    if (pos >= 0 && pos < indexList.size()) {
        cursor = pos;
        return indexList.at(cursor);
    }

    return QModelIndex();
}

int FileSystemHistory::getCursor() const
{
    return cursor;
}

QModelIndexList& FileSystemHistory::getIndexList()
{
    return indexList;
}
