#ifndef FILESYSTEMHISTORY_H
#define FILESYSTEMHISTORY_H

#include <QList>
#include <QIcon>

class FileSystemHistory : QObject
{
public:
    typedef struct _HistoryEntry {
        QString path;
        QIcon icon;
    } HistoryEntry;

    FileSystemHistory(QObject *parent = nullptr);
    ~FileSystemHistory();

    bool isCursorAtTheEnd();
    void insert(QString path, QIcon icon);

    bool canGoForward();
    bool canGoBack();

    QString getLastItem();
    QString getNextItem();

private:
    QList<HistoryEntry *> pathList;
    int cursor { -1 };

};

#endif // FILESYSTEMHISTORY_H
