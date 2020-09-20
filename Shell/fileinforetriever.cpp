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
    qDebug() << "FileInfoRetriever::~FileInfoRetriever About to destroy";

    if (running.load()) {
        // Abort all the pending threads
        running.store(false);
        pool.clear();
        pool.waitForDone();
    }

    qDebug() << "FileInfoRetriever::~FileInfoRetriever Destroyed";
}

FileSystemItem *FileInfoRetriever::getRoot(QString path)
{
    // If this thread is still doing a previous task we have to abort it and wait until it finishes
    if (running.load()) {
        // Abort all the pending threads
        qDebug() << "FileInfoRetriever::getRoot: " << getScope() << "Stopping all threads";
        running.store(false);
        pool.clear();
        qDebug() << "FileInfoRetriever::getRoot: " << getScope() << "Waiting for all threads to finish";
        pool.waitForDone();
        qDebug() << "FileInfoRetriever::getRoot: " << getScope() << "All threads finished";
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

void FileInfoRetriever::getExtendedInfo(FileSystemItem *parent)
{
    Q_UNUSED(parent)
}

/*!
 * \brief Gets all the children of a FileSystemItem parent.
 * \param parent a FileSystemItem
 *
 * Gets all the children of a FileSystemItem parent (a folder or a drive) concurrently.
 *
 * The children will be retrieved on the background in a new thread.
 */
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

    if (getScope() == FileInfoRetriever::List)
         getExtendedInfo(parent);

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

void FileInfoRetriever::setDisplayNameOf(FileSystemItem *item)
{
    Q_UNUSED(item)
}

