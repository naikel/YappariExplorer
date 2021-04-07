#include <QtConcurrent/QtConcurrentRun>
#include <QApplication>
#include <QModelIndex>
#include <QTimer>
#include <QDebug>

#include "FileInfoRetriever.h"

#ifdef Q_OS_WIN
QMutex FileInfoRetriever::threadMutex;
#endif

FileInfoRetriever::FileInfoRetriever(QObject *parent) : QThread(parent)
{
}

FileInfoRetriever::~FileInfoRetriever()
{
    qDebug() << "FileInfoRetriever::~FileInfoRetriever About to destroy";

    if (running.load()) {

        // Abort all the pending threads
        running.store(false);
    }

    qDebug() << "FileInfoRetriever::~FileInfoRetriever Destroyed";
}

QString FileInfoRetriever::getRootPath() const
{
    return "/";
}

void FileInfoRetriever::getInfo(FileSystemItem *parent)
{
    addJob(parent, Parent);
}

void FileInfoRetriever::addJob(FileSystemItem *item, FileInfoRetriever::JobType type, bool insertAtFront)
{
    Job job;
    job.item = item;
    job.type = type;
    if (!insertAtFront)
        jobsQueue.append(job);
    else
        jobsQueue.insert(0, job);

    jobAvailable.wakeAll();
}

void FileInfoRetriever::run()
{
    QMutex mutex;

    threadRunning.store(true);

    forever {

        if (!threadRunning.load())
            break;

        mutex.lock();

        if (jobsQueue.isEmpty())
            jobAvailable.wait(&mutex);

        if (!threadRunning.load()) {
            mutex.unlock();
            break;
        }

        if (!jobsQueue.isEmpty()) {

            running.store(true);
            jobMutex.lock();
            currentJob = jobsQueue.takeFirst();
            jobMutex.unlock();

            if (currentJob.item != nullptr) {

#ifdef Q_OS_WIN
                threadMutex.lock();
#endif

                switch(currentJob.type) {

                    case Parent:
                        getParentBackground(currentJob.item);
                        break;
                    case Children:
                        if (!currentJob.item->areAllChildrenFetched())
                            getChildrenBackground(currentJob.item);
                        break;
                    case Icon:
                        getIconBackground(currentJob.item);
                        break;
                }

#ifdef Q_OS_WIN
                threadMutex.unlock();
#endif

                currentJob.item = nullptr;
            }

            running.store(false);
        }

        mutex.unlock();
    }
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
    if (parent != nullptr) {

        qDebug() << "FileInfoRetriever::getChildren" << parent->getPath();

        // Cancel everything for this model
        jobMutex.lock();

        if (currentJob.item != nullptr && currentJob.type == Children && currentJob.item->getPath() != parent->getPath()) {
            qDebug() << "FileInfoRetriever::getChildren cancelling current fetch of" << currentJob.item->getPath();
            running.store(false);
        }

        // Let's double check the children haven't been fetched
        if (!parent->areAllChildrenFetched()) {
            addJob(parent, Children, true);
        } else
            qDebug() << "FileInfoRetriever::getChildren" << "all children were already fetched";

        jobMutex.unlock();
    }
}

void FileInfoRetriever::getIcon(FileSystemItem *parent, bool background)
{
    if (background)
        addJob(parent, Icon);
    else
        getIconBackground(parent, background);
}

void FileInfoRetriever::quit()
{
    threadRunning.store(false);
    jobAvailable.wakeAll();
}

