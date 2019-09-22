#include <QMutexLocker>
#include <QDebug>

#include "fileinforetriever.h"

FileInfoRetriever::FileInfoRetriever(QObject *parent) : QThread(parent)
{

}

FileInfoRetriever::~FileInfoRetriever()
{
    qDebug() << "About to destroy";
    abort.store(true);
    QMutexLocker locker(&mutex);
    condition.wakeAll();
    locker.unlock();
    wait();

    qDebug() << "Destroyed";
}

void FileInfoRetriever::run()
{
    forever {
        QMutexLocker locker(&mutex);
        while (!abort.load() && parents.isEmpty()) {
            qDebug() << "Waiting";
            condition.wait(&mutex);
        }
        qDebug() << "Finished waiting";

        if (abort.load())
            return;


        FileSystemItem *parent = parents.takeFirst();
        locker.unlock();

        qDebug() << "New task " << parent->getDisplayName();

        if (!parent->areAllChildrenFetched())
            getChildrenBackground(parent);
    }
}

FileSystemItem *FileInfoRetriever::getRoot()
{
    // This PC
    QString path { "/" };
    FileSystemItem *root = new FileSystemItem(path);

    getChildrenBackground(root);
    return root;
}

void FileInfoRetriever::getChildren(FileSystemItem *parent) {

    QMutexLocker locker(&mutex);

    qDebug() << "Getting children of " << parent->getPath();

    parents.push(parent);
    condition.wakeAll();
}

void FileInfoRetriever::getChildrenBackground(FileSystemItem *parent) {
    emit parentUpdated(parent);
}
