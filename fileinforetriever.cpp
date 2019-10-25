#include <QtConcurrent/QtConcurrentRun>
#include <QApplication>
#include <QTimer>
#include <QDebug>

#include "fileinforetriever.h"

FileInfoRetriever::FileInfoRetriever(QObject *parent) : QObject(parent)
{
    // Default scope is Tree
    scope = Scope::Tree;
}

FileInfoRetriever::~FileInfoRetriever()
{
    qDebug() << "~FileInfoRetriever About to destroy";

    if (running.load()) {
        // Abort all the pending threads
        running.store(false);
        pool.clear();
        pool.waitForDone();
    }

    qDebug() << "~FileInfoRetriever Destroyed";
}

FileSystemItem *FileInfoRetriever::getRoot(QString path)
{
    // If this thread is still doing a previous task we have to abort it and wait until it finishes
    if (running.load()) {
        // Abort all the pending threads
        running.store(false);
        pool.clear();
        pool.waitForDone();
    }

    FileSystemItem *root = new FileSystemItem(path);

    getParentInfo(root);
    getChildren(root);
    return root;
}

void FileInfoRetriever::getParentInfo(FileSystemItem *parent)
{
    Q_UNUSED(parent)
}

void FileInfoRetriever::getChildren(FileSystemItem *parent)
{
    // This function might get called several times for the same parent
    // So let's try to serialize it
    if (running.load()) {
        pool.waitForDone();
    }

    // Let's double check the children hasn't been fetched
    if (!parent->areAllChildrenFetched()) {
        running.store(true);
        QtConcurrent::run(const_cast<QThreadPool *>(&pool), const_cast<FileInfoRetriever *>(this), &FileInfoRetriever::getChildrenBackground, parent);
    }
}

void FileInfoRetriever::getChildrenBackground(FileSystemItem *parent)
{
    emit parentUpdated(parent);
    running.store(false);
}

FileInfoRetriever::Scope FileInfoRetriever::getScope() const
{
    return scope;
}

void FileInfoRetriever::setScope(const Scope &value)
{
    scope = value;
}

QIcon FileInfoRetriever::getIcon(FileSystemItem *item) const
{
    Q_UNUSED(item)
    return QIcon();
}

