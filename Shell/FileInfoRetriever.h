#ifndef FILEINFORETRIEVER_H
#define FILEINFORETRIEVER_H

#include <QThreadPool>
#include <QAtomicInt>

#include "FileSystemItem.h"

class FileInfoRetriever : public QObject
{
    Q_OBJECT

public:

    FileInfoRetriever(QObject *parent = nullptr);
    ~FileInfoRetriever();

    virtual QString getRootPath() const;

    bool getInfo(FileSystemItem *root);

    void getChildren(FileSystemItem *parent);
    virtual QIcon getIcon(FileSystemItem *fileSystemItem) const;
    virtual void setDisplayNameOf(FileSystemItem *fileSystemItem);
    virtual bool refreshItem(FileSystemItem *fileSystemItem) = 0;

    virtual bool willRecycle(FileSystemItem *fileSystemItem);

signals:
    void parentUpdated(FileSystemItem *parent);
    void itemUpdated(FileSystemItem *item);
    void extendedInfoUpdated(FileSystemItem *parent);


protected:
    QAtomicInt running;

    virtual void getChildrenBackground(FileSystemItem *parent);
    virtual bool getParentInfo(FileSystemItem *parent);
    virtual void getExtendedInfo(FileSystemItem *parent);

private:
    QThreadPool pool;

    FileSystemItem *currentParent {};
};

#endif // FILEINFORETRIEVER_H
