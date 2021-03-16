#include <QtConcurrent/QtConcurrentRun>
#include <QApplication>
#include <QTimer>
#include <QDebug>

#include "FileInfoRetriever.h"

FileInfoRetriever::FileInfoRetriever(QObject *parent) : QObject(parent)
{
    pool.setMaxThreadCount(1);
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

QString FileInfoRetriever::getRootPath() const
{
    return "/";
}

bool FileInfoRetriever::getInfo(FileSystemItem *root)
{
    if (root != nullptr && !root->getPath().isEmpty()) {

        // TODO Use a QMutex here

        // If this thread is still doing a previous task we have to abort it and wait until it finishes
        if (running.load()) {
            // Abort all the pending threads
            qDebug() << "FileInfoRetriever::getInfo" << "Stopping all threads";
            running.store(false);
            pool.clear();
            qDebug() << "FileInfoRetriever::getInfo" << "Waiting for all threads to finish";
            pool.waitForDone();
            qDebug() << "FileInfoRetriever::getInfo" << "All threads finished";
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

bool FileInfoRetriever::willRecycle(FileSystemItem *fileSystemItem)
{
    Q_UNUSED(fileSystemItem)

    return false;
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
    qDebug() << "FileInfoRetriever::getChildren" << parent->getPath();

    // This function might get called several times for the same parent
    // Why? Because QTreeView somehow calls fetchMore twice
    // So let's try to serialize it

    // Ignore multiple fetches to the same parent
    if (currentParent != nullptr && parent != nullptr && currentParent->getPath() == parent->getPath())
        return;

    // TODO: This check should be in a different thread to not block the GUI
    if (running.load()) {

        // Abort current fetch
        qDebug() << "FileInfoRetriever::getChildren" << "aborting current fetch!";
        running.store(false);

        qDebug() << "FileInfoRetriever::getChildren" << "waiting for previous threads to finish" << pool.activeThreadCount();
        pool.waitForDone();
        qDebug() << "FileInfoRetriever::getChildren" << "all threads finished";

        // Clean current fetch
        if (currentParent != nullptr) {
            qDebug() << "FileInfoRetriever::getChildren" << "cleaning aborted parent";
            currentParent->removeChildren();
            currentParent = nullptr;
        }
    }

    // Let's double check the children haven't been fetched
    if (!parent->areAllChildrenFetched()) {
        qDebug() << "FileInfoRetriever::getChildren" << "about to go on the background to fetch the children";
        running.store(true);
        currentParent = parent;
        QtConcurrent::run(const_cast<QThreadPool *>(&pool), const_cast<FileInfoRetriever *>(this), &FileInfoRetriever::getChildrenBackground, parent);
    } else
        qDebug() << "FileInfoRetriever::getChildren" << "all children were already fetched";
}

void FileInfoRetriever::getChildrenBackground(FileSystemItem *parent)
{
    emit parentUpdated(parent);

    // getExtendedInfo(parent);

    currentParent = nullptr;
    running.store(false);
}

// TODO: Everything here should have a different thread
QIcon FileInfoRetriever::getIcon(FileSystemItem *item) const
{
    Q_UNUSED(item)
    return QIcon();
}

void FileInfoRetriever::setDisplayNameOf(FileSystemItem *item)
{
    Q_UNUSED(item)
}

