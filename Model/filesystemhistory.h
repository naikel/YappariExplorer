#ifndef FILESYSTEMHISTORY_H
#define FILESYSTEMHISTORY_H

#include <QList>
#include <QIcon>

#include "Shell/filesystemitem.h"

struct _HistoryEntry {
    QString displayName;
    QString path;
    QIcon icon;
};

typedef struct _HistoryEntry HistoryEntry;

class FileSystemHistory : QObject
{
public:
    FileSystemHistory(QObject *parent = nullptr);
    ~FileSystemHistory();

    bool isCursorAtTheEnd();
    void insert(FileSystemItem *item);

    bool canGoForward();
    bool canGoBack();

    QString getLastItem();
    QString getNextItem();
    QString getItemAtPos(int pos);

    int getCursor() const;
    QList<HistoryEntry *> &getPathList();

private:
    QList<HistoryEntry *> pathList;
    int cursor { -1 };

};

#endif // FILESYSTEMHISTORY_H
