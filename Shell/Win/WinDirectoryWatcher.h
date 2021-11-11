#ifndef WINDIRECTORYWATCHER_H
#define WINDIRECTORYWATCHER_H

#include <qt_windows.h>
#include <fileapi.h>

#include <QMultiMap>

#include "Shell/DirectoryWatcher.h"
#include "Shell/Win/WinDirChangeNotifier.h"

class WinDirectoryWatcher : public DirectoryWatcher
{
    Q_OBJECT

    typedef struct _Watcher {
        FileSystemItem *item;
        QString path;
        ULONG regId;
    } Watcher;

public:
    WinDirectoryWatcher(QObject *parent = nullptr);
    ~WinDirectoryWatcher();

    void addItem(FileSystemItem *item) override;
    void removeItem(FileSystemItem *item) override;
    bool isWatching(FileSystemItem *item) override;
    void refresh() override;
    bool handleNativeEvent(const QByteArray &eventType, void *message, long *result) override;

private:
    QMultiMap<QString, Watcher *> watchedPaths;
    quint32 id;
    WinDirChangeNotifier *dirChangeNotifier;

    void removeWatch(Watcher *watcher);
    void directoryChange(long lEvent, QString strPath1, QString strPath2);
};

#endif // WINDIRECTORYWATCHER_H
