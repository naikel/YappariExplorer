#ifndef DIRECTORYWATCHER_H
#define DIRECTORYWATCHER_H

#include <QObject>

#include "Shell/FileSystemItem.h"

class DirectoryWatcher : public QObject
{
    Q_OBJECT
public:
    DirectoryWatcher(QObject *parent = nullptr);

    virtual void addItem(FileSystemItem *item) = 0;
    virtual void removeItem(FileSystemItem *item) = 0;
    virtual bool isWatching(FileSystemItem *item) = 0;
    virtual void refresh() = 0;
    virtual bool handleNativeEvent(const QByteArray &eventType, void *message, long *result);

signals:
    void fileRename(FileSystemItem *item, QString newFileName);
    void fileModified(FileSystemItem *item);
    void fileAdded(FileSystemItem *parent, QString fileName);
    void fileRemoved(FileSystemItem *item);
    void folderUpdated(FileSystemItem *item);
};

#endif // DIRECTORYWATCHER_H
