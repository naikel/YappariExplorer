#ifndef FILESYSTEMHISTORY_H
#define FILESYSTEMHISTORY_H

#include <QList>
#include <QIcon>

#include "Model/FileSystemModel.h"

struct _HistoryEntry {
    QString displayName;
    QString path;
    QIcon icon;
};

typedef struct _HistoryEntry HistoryEntry;

class FileSystemHistory : QObject
{
public:
    FileSystemHistory(QAbstractItemModel *model, QObject *parent = nullptr);
    ~FileSystemHistory();

    bool isCursorAtTheEnd();
    void insert(QString displayName, QIcon icon, QString path);

    bool canGoForward();
    bool canGoBack();
    bool canGoUp();

    QString getLastItem();
    QString getNextItem();
    QString getParentItem();
    QString getItemAtPos(int pos);

    int getCursor() const;
    QList<HistoryEntry *> &getPathList();

private:
    FileSystemModel *model;
    QList<HistoryEntry *> pathList;
    int cursor { -1 };

};

#endif // FILESYSTEMHISTORY_H
