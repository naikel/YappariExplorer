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

    bool getInfo(FileSystemItem *root);

    Scope getScope() const;
    void setScope(const Scope &value);

    void getChildren(FileSystemItem *parent);
    virtual QIcon getIcon(FileSystemItem *fileSystemItem) const;
    virtual void setDisplayNameOf(FileSystemItem *fileSystemItem);

signals:
    void parentUpdated(FileSystemItem *parent, qint32 err, QString errMessage);
    void itemUpdated(FileSystemItem *item);
    void extendedInfoUpdated(FileSystemItem *parent);


protected:
    QAtomicInt running;

    virtual void getChildrenBackground(FileSystemItem *parent);
    virtual bool getParentInfo(FileSystemItem *parent);
    virtual void getExtendedInfo(FileSystemItem *parent);

private:
    QThreadPool pool;
    Scope scope;
};

#endif // FILEINFORETRIEVER_H
