#ifndef WINDIRECTORYWATCHER_H
#define WINDIRECTORYWATCHER_H

#include <qt_windows.h>
#include <fileapi.h>

#include <QMutex>
#include <QHash>
#include <QMap>

#include "Shell/directorywatcher.h"

class WinDirectoryWatcher : public DirectoryWatcher
{
    Q_OBJECT

public:
    WinDirectoryWatcher(QObject *parent = nullptr);
    ~WinDirectoryWatcher();

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

#endif // WINDIRECTORYWATCHER_H
