#ifndef FILEINFORETRIEVER_H
#define FILEINFORETRIEVER_H

#include <QWaitCondition>
#include <QAtomicInt>
#include <QThread>
#include <QMutex>

#include "FileSystemItem.h"

class FileInfoRetriever : public QThread
{
    Q_OBJECT

public:

    FileInfoRetriever(QObject *parent = nullptr);
    ~FileInfoRetriever();

    virtual QString getRootPath() const;

    // These functions are executed in a separate thread
    void getInfo(FileSystemItem *parent);
    void getChildren(FileSystemItem *parent);
    void getIcon(FileSystemItem *parent, bool background = true);

    // These functions are not executed in a separated thread
    virtual bool refreshItem(FileSystemItem *fileSystemItem) = 0;
    virtual bool willRecycle(FileSystemItem *fileSystemItem) = 0;

signals:
    void parentUpdated(FileSystemItem *parent);
    void itemUpdated(FileSystemItem *item);

    // New ones
    void parentInfoUpdated(FileSystemItem *parent);
    void parentChildrenUpdated(FileSystemItem *parent);
    void iconUpdated(FileSystemItem *item);


public slots:
    void quit();

protected:
    QAtomicInt running;

    void run() override;

    virtual void getChildrenBackground(FileSystemItem *parent) = 0;
    virtual bool getParentBackground(FileSystemItem *parent) = 0;
    virtual void getIconBackground(FileSystemItem *parent, bool background = true) = 0;

private:

    QAtomicInt threadRunning;
    QMutex jobMutex;

    // In Windows if you have several threads calling shell function the functionality is impaired.
    // So all the threads are initialized as COINIT_APARTMENTTHREAD and we have to serialize all threads
    // accessing the filesystem.
    // We do this using a static QMutex
    // https://docs.microsoft.com/en-us/troubleshoot/windows/win32/shell-functions-multithreaded-apartment

#ifdef Q_OS_WIN
    static QMutex threadMutex;
#endif

    enum JobType {

        Parent,
        Children,
        Icon
    };

    typedef struct _job {

        FileSystemItem *item;
        JobType type;

    } Job;

    QList<Job> jobsQueue;
    QWaitCondition jobAvailable;
    Job currentJob;

    void addJob(FileSystemItem *item, FileInfoRetriever::JobType type, bool insertAtFront = false);

};

#endif // FILEINFORETRIEVER_H
