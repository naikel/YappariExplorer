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

    void copyItems(QList<QUrl> srcPaths, QString dstPath);
    void moveItems(QList<QUrl> srcPaths, QString dstPath);
    void linkItems(QList<QUrl> srcPaths, QString dstPath);

protected:
    QAtomicInt running;

    virtual void copyItemsBackground(QList<QUrl> srcPaths, QString dstPath);
    virtual void moveItemsBackground(QList<QUrl> srcUrls, QString dstPath);
    virtual void linkItemsBackground(QList<QUrl> srcPaths, QString dstPath);

private:
    QThreadPool pool;
};

#endif // SHELLACTIONS_H
