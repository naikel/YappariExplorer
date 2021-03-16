#ifndef FILESYSTEMHISTORY_H
#define FILESYSTEMHISTORY_H

#include <QList>
#include <QIcon>

#include "Model/FileSystemModel.h"

class FileSystemHistory : QObject
{
public:
    FileSystemHistory(QObject *parent = nullptr);
    ~FileSystemHistory();

    bool isCursorAtTheEnd() const;
    void insert(const QModelIndex &index);

    bool canGoForward() const;
    bool canGoBack() const;
    bool canGoUp() const;

    QModelIndex getLastItem();
    QModelIndex getNextItem();
    QModelIndex getParentItem();
    QModelIndex getItemAtPos(int pos);

    int getCursor() const;
    QModelIndexList &getIndexList();

private:
    QModelIndexList indexList;
    int cursor { -1 };

};

#endif // FILESYSTEMHISTORY_H
