#include <QApplication>
#include <QMutexLocker>
#include <QTimer>
#include <QDebug>

#include "fileinforetriever.h"

FileInfoRetriever::FileInfoRetriever(QObject *parent) : QThread(parent)
{
    // Default scope is Tree
    scope = Scope::Tree;
}

FileInfoRetriever::~FileInfoRetriever()
{
    qDebug() << "~FileInfoRetriever About to destroy";
    abort.store(true);
    QMutexLocker locker(&mutex);
    condition.wakeAll();
    locker.unlock();
    wait();

    qDebug() << "~FileInfoRetriever Destroyed";
}

void FileInfoRetriever::run()
{
    forever {
        QMutexLocker locker(&mutex);
        while (!abort.load() && parents.isEmpty()) {
            qDebug() << "FileInfoRetriever::run " << scope << " Waiting";
            condition.wait(&mutex);
        }
        qDebug() << "FileInfoRetriever::run " << scope << " Finished waiting";

        if (abort.load())
            return;

        FileSystemItem *parent = parents.takeFirst();
        locker.unlock();

        qDebug() << "FileInfoRetriever::run " << scope << " New task " << parent->getPath();

        if (!parent->areAllChildrenFetched()) {
            running.store(true);
            getChildrenBackground(parent);
            running.store(false);
            condition.wakeAll();
        }
    }
}

FileSystemItem *FileInfoRetriever::getRoot(QString path)
{
    // If this thread is still doing a previous task we have to abort it and wait until it finishes
    if (running.load()) {
        QMutexLocker locker(&mutex);
        running.store(false);
        condition.wait(&mutex);
    }

    FileSystemItem *root = new FileSystemItem(path);

    getParentInfo(root);
    getChildren(root);
    return root;
}

void FileInfoRetriever::getChildren(FileSystemItem *parent)
{

    QMutexLocker locker(&mutex);

    qDebug() << "FileInfoRetriever:getChildren " << scope << " Getting children of " << parent->getPath();

    parents.push(parent);
    condition.wakeAll();
}

void FileInfoRetriever::getParentInfo(FileSystemItem *parent)
{
    Q_UNUSED(parent)
}

void FileInfoRetriever::getChildrenBackground(FileSystemItem *parent)
{
    emit parentUpdated(parent);
}

FileInfoRetriever::Scope FileInfoRetriever::getScope() const
{
    return scope;
}

void FileInfoRetriever::setScope(const Scope &value)
{
    scope = value;
}

