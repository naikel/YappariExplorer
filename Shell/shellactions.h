#ifndef SHELLACTIONS_H
#define SHELLACTIONS_H

#include <QThreadPool>
#include <QAtomicInt>
#include <QUrl>

class ShellActions : public QObject
{
    Q_OBJECT

public:
    ShellActions(QObject *parent = nullptr);
    ~ShellActions();

    void renameItem(QUrl srcPath, QString newName);
    void copyItems(QList<QUrl> srcPaths, QString dstPath);
    void moveItems(QList<QUrl> srcPaths, QString dstPath);
    void linkItems(QList<QUrl> srcPaths, QString dstPath);
    void removeItems(QList<QUrl> srcPaths, bool permanent);

protected:
    QAtomicInt running;

    virtual void renameItemBackground(QUrl srcPath, QString newName);
    virtual void copyItemsBackground(QList<QUrl> srcPaths, QString dstPath);
    virtual void moveItemsBackground(QList<QUrl> srcUrls, QString dstPath);
    virtual void linkItemsBackground(QList<QUrl> srcPaths, QString dstPath);
    virtual void removeItemsBackground(QList<QUrl> srcPaths, bool permanent);

private:
    QThreadPool pool;
};

#endif // SHELLACTIONS_H
