#include <QtConcurrent/QtConcurrent>
#include <QDebug>

#include "shellactions.h"

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



