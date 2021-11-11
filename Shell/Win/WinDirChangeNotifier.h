#ifndef WINDIRCHANGENOTIFIER_H
#define WINDIRCHANGENOTIFIER_H

#include <qt_windows.h>
#include <fileapi.h>

#include <QMutex>
#include <QMultiMap>

#include "Shell/DirectoryWatcher.h"

class WinDirChangeNotifier : public DirectoryWatcher
{
    Q_OBJECT

public:
    WinDirChangeNotifier(QObject *parent = nullptr);
    ~WinDirChangeNotifier();

    void addItem(FileSystemItem *item) override;
    void removeItem(FileSystemItem *item) override;
    bool isWatching(FileSystemItem *item) override;
    void refresh() override;
    void directoryChanged(LPOVERLAPPED lpOverLapped);


private:

    typedef struct _DirectoryWatch {
        FileSystemItem *item;
        HANDLE handle                           {};
        OVERLAPPED overlapped                   {};
        FILE_NOTIFY_INFORMATION info[10240]     {};
        DWORD dwBytesReturned                   {};
    } DirectoryWatch;

    QMutex mutex;
    QMultiMap<QString, DirectoryWatch *> watchedPaths;

    static int id;
    int thisId;

    bool readDirectory(DirectoryWatch *watch);
    void removeWatch(DirectoryWatch *watch);

};

#endif // WINDIRCHANGENOTIFIER_H
