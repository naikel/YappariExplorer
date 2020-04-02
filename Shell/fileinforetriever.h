#ifndef FILEINFORETRIEVER_H
#define FILEINFORETRIEVER_H

#include <QThreadPool>
#include <QAtomicInt>

#include "filesystemitem.h"

class FileInfoRetriever : public QObject
{
    Q_OBJECT

public:

    enum Scope {
        Tree,
        List
    };

    FileInfoRetriever(QObject *parent = nullptr);
    ~FileInfoRetriever() override;

    FileSystemItem *getRoot(QString path);

    Scope getScope() const;
    void setScope(const Scope &value);

    void getChildren(FileSystemItem *parent);
    virtual QIcon getIcon(FileSystemItem *fileSystemItem) const;

signals:
    void parentUpdated(FileSystemItem *parent);
    void itemUpdated(FileSystemItem *item);
    void extendedInfoUpdated(FileSystemItem *parent);

protected:
    QAtomicInt running;

    virtual void getChildrenBackground(FileSystemItem *parent);
    virtual void getParentInfo(FileSystemItem *parent);
    virtual void getExtendedInfo(FileSystemItem *parent);

private:
    QThreadPool pool;
    Scope scope;
};

#endif // FILEINFORETRIEVER_H
