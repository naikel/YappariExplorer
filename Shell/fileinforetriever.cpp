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

bool FileInfoRetriever::getInfo(FileSystemItem *root)
{
    if (root != nullptr && !root->getPath().isEmpty()) {

        // TODO Use a QMutex here

        // If this thread is still doing a previous task we have to abort it and wait until it finishes
        if (running.load()) {
            // Abort all the pending threads
            qDebug() << "FileInfoRetriever::getInfo" << getScope() << "Stopping all threads";
            running.store(false);
            pool.clear();
            qDebug() << "FileInfoRetriever::getInfo" << getScope() << "Waiting for all threads to finish";
            pool.waitForDone();
            qDebug() << "FileInfoRetriever::getInfo" << getScope() << "All threads finished";
        }

       if (getParentInfo(root)) {
            getChildren(root);
            return true;
       }
   }

   return false;
}

bool FileInfoRetriever::getParentInfo(FileSystemItem *parent)
{
    Q_UNUSED(parent)

    return true;
}

void FileInfoRetriever::getExtendedInfo(FileSystemItem *parent)
{
    Q_UNUSED(parent)
}

void FileInfoRetriever::refreshItem(FileSystemItem *fileSystemItem)
{
    Q_UNUSED(fileSystemItem)
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
        qDebug() << "FileInfoRetriever::getChildren waiting for previous threads to finish";
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
    emit parentUpdated(parent, 0, QString());

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

