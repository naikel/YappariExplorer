#ifndef FILEINFORETRIEVER_H
#define FILEINFORETRIEVER_H

#include <QWaitCondition>
#include <QAtomicInt>
#include <QThread>
#include <QStack>
#include <QMutex>

#include "filesystemitem.h"

class FileInfoRetriever : public QThread
{
    Q_OBJECT

public:
    FileInfoRetriever(QObject *parent = nullptr);
    ~FileInfoRetriever() override;

    void run() override;
    FileSystemItem *getRoot();
    void getChildren(FileSystemItem *fileSystemItem);

Q_SIGNALS:
    void parentUpdated(FileSystemItem *parent);

protected:
    virtual void getChildrenBackground(FileSystemItem *fileSystemItem);

private:
    mutable QMutex mutex;
    QWaitCondition condition;
    QStack<FileSystemItem *> parents;

    QAtomicInt abort;
    QAtomicInt cursor;

    void overrideCursor();
    void resetCursor();
};

#endif // FILEINFORETRIEVER_H
