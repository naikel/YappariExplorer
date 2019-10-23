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

    enum Scope {
        Tree,
        List
    };

    FileInfoRetriever(QObject *parent = nullptr);
    ~FileInfoRetriever() override;

    void run() override;
    FileSystemItem *getRoot(QString path);
    void getChildren(FileSystemItem *fileSystemItem);

    Scope getScope() const;
    void setScope(const Scope &value);

    virtual QIcon getIcon(FileSystemItem *fileSystemItem) const;

signals:
    void parentUpdated(FileSystemItem *parent);

protected:
    QAtomicInt running;

    virtual void getChildrenBackground(FileSystemItem *parent);
    virtual void getParentInfo(FileSystemItem *parent);

private:
    mutable QMutex mutex;
    QWaitCondition condition;
    QStack<FileSystemItem *> parents;
    Scope scope;

    QAtomicInt abort;
};

#endif // FILEINFORETRIEVER_H
