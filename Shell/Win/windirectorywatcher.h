#ifndef WINDIRECTORYWATCHER_H
#define WINDIRECTORYWATCHER_H

#include <qt_windows.h>
#include <fileapi.h>

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
        HANDLE handle                       {};
        OVERLAPPED overlapped               {};
        FILE_NOTIFY_INFORMATION info[32]    {};
        DWORD dwBytesReturned               {};
    } DirectoryWatch;

    QMap<QString, DirectoryWatch *> watchedPaths;

    bool readDirectory(DirectoryWatch *watch);
};

#endif // WINDIRECTORYWATCHER_H
