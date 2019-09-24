#include <QApplication>
#include <QMutexLocker>
#include <QTimer>
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

        if (!parent->areAllChildrenFetched()) {

            overrideCursor();
            getChildrenBackground(parent);
            resetCursor();
        }
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

void FileInfoRetriever::getChildren(FileSystemItem *parent)
{

    QMutexLocker locker(&mutex);

    qDebug() << "Getting children of " << parent->getPath();

    parents.push(parent);
    condition.wakeAll();
}

void FileInfoRetriever::getChildrenBackground(FileSystemItem *parent)
{
    emit parentUpdated(parent);
}

void FileInfoRetriever::overrideCursor()
{
    if (!cursor.load()) {
        cursor.store(true);

        // Qt bug: restoreOverrideCursor() does nothing in Windows.  Cursor seems to only change when using setOverrideCursor()
        QApplication::restoreOverrideCursor();
        QApplication::setOverrideCursor(Qt::BusyCursor);
    }
}

void FileInfoRetriever::resetCursor()
{
    if (cursor.load()) {

        // Qt bug: restoreOverrideCursor() does nothing in Windows.  Cursor seems to only change when using setOverrideCursor()
        QApplication::restoreOverrideCursor();
        QApplication::setOverrideCursor(Qt::ArrowCursor);
        cursor.store(false);
    }
}
