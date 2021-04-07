#ifndef WINDIRCHANGENOTIFIER_H
#define WINDIRCHANGENOTIFIER_H

#include <qt_windows.h>
#include <fileapi.h>

#include <QMutex>
#include <QHash>
#include <QMap>

#include "Shell/DirectoryWatcher.h"

class WinDirChangeNotifier : public DirectoryWatcher
{
    Q_OBJECT

public:
    WinDirChangeNotifier(QObject *parent = nullptr);
    ~WinDirChangeNotifier();

    void addPath(QString path) override;
    void removePath(QString path) override;
    void directoryChanged(LPOVERLAPPED lpOverLapped);


private:

    typedef struct _DirectoryWatch {
        QString path;
        HANDLE handle                           {};
        OVERLAPPED overlapped                   {};
        FILE_NOTIFY_INFORMATION info[10240]     {};
        DWORD dwBytesReturned                   {};
    } DirectoryWatch;

    QMutex mutex;
    QHash<QString, DirectoryWatch *> watchedPaths;

    bool readDirectory(DirectoryWatch *watch);

    static int id;
    int thisId;
};

#endif // WINDIRCHANGENOTIFIER_H
