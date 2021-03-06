#include <QtConcurrent/QtConcurrent>
#include <QDebug>

#include "ShellActions.h"

ShellActions::ShellActions(QObject *parent) : QObject(parent)
{

}

ShellActions::~ShellActions()
{
     qDebug() << "ShellActions::~ShellActions About to destroy";

    if (running.load()) {
        // Abort all the pending threads
        running.store(false);
        pool.clear();
        pool.waitForDone();
    }

    qDebug() << "ShellActions::~ShellActions Destroyed";
}

void ShellActions::renameItem(QUrl srcPath, QString newName)
{
    qDebug() << "ShellActions::renameItem";

    if (running.load()) {
        pool.waitForDone();
    }

    running.store(true);
    QtConcurrent::run(const_cast<QThreadPool *>(&pool), const_cast<ShellActions *>(this), &ShellActions::renameItemBackground, srcPath, newName);
}

void ShellActions::copyItems(QList<QUrl> srcPaths, QString dstPath)
{
    qDebug() << "ShellActions::copyItems";

    if (running.load()) {
        pool.waitForDone();
    }

    running.store(true);
    QtConcurrent::run(const_cast<QThreadPool *>(&pool), const_cast<ShellActions *>(this), &ShellActions::copyItemsBackground, srcPaths, dstPath);
}

void ShellActions::moveItems(QList<QUrl> srcPaths, QString dstPath)
{
    qDebug() << "ShellActions::moveItems";

    if (running.load()) {
        pool.waitForDone();
    }

    running.store(true);
    QtConcurrent::run(const_cast<QThreadPool *>(&pool), const_cast<ShellActions *>(this), &ShellActions::moveItemsBackground, srcPaths, dstPath);
}

void ShellActions::linkItems(QList<QUrl> srcPaths, QString dstPath)
{
    qDebug() << "ShellActions::linkItems";

    if (running.load()) {
        pool.waitForDone();
    }

    running.store(true);
    QtConcurrent::run(const_cast<QThreadPool *>(&pool), const_cast<ShellActions *>(this), &ShellActions::linkItemsBackground, srcPaths, dstPath);
}

void ShellActions::removeItems(QList<QUrl> srcPaths, bool permanent)
{
    qDebug() << "ShellActions::removeItems";

    if (running.load()) {
        pool.waitForDone();
    }

    running.store(true);
    QtConcurrent::run(const_cast<QThreadPool *>(&pool), const_cast<ShellActions *>(this), &ShellActions::removeItemsBackground, srcPaths, permanent);
}

void ShellActions::renameItemBackground(QUrl srcUrl, QString newName)
{
    Q_UNUSED(srcUrl)
    Q_UNUSED(newName)
    running.store(false);
}

void ShellActions::copyItemsBackground(QList<QUrl> srcPaths, QString dstPath)
{
    Q_UNUSED(srcPaths)
    Q_UNUSED(dstPath)
    running.store(false);
}

void ShellActions::moveItemsBackground(QList<QUrl> srcPaths, QString dstPath)
{
    Q_UNUSED(srcPaths)
    Q_UNUSED(dstPath)
    running.store(false);
}

void ShellActions::linkItemsBackground(QList<QUrl> srcPaths, QString dstPath)
{
    Q_UNUSED(srcPaths)
    Q_UNUSED(dstPath)
    running.store(false);
}

void ShellActions::removeItemsBackground(QList<QUrl> srcPaths, bool permanent)
{
    Q_UNUSED(srcPaths)
    Q_UNUSED(permanent)
    running.store(false);
}



