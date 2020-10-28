#include <QDebug>

#include <wchar.h>
#include <ioapiset.h>

#include "windirectorywatcher.h"

WinDirectoryWatcher::WinDirectoryWatcher(QObject *parent) : DirectoryWatcher(parent)
{
}

WinDirectoryWatcher::~WinDirectoryWatcher()
{
    qDebug() << "WinDirectoryWatcher::~WinDirectoryWatcher About to destroy";

    QStringList paths = watchedPaths.keys();
    for (QString path : paths)
        removePath(path);

    qDebug() << "WinDirectoryWatcher::~WinDirectoryWatcher Destroyed";
}

void WinDirectoryWatcher::addPath(QString path)
{
    qDebug() << "WinDirectoryWatcher::run addPath " << path;

    if (!path.isNull() && path.length() >= 3 && path.at(0).isLetter() && path.at(1) == ':' && path.at(2) == '\\' && !watchedPaths.contains(path)) {

        DirectoryWatch *watch = new DirectoryWatch;

        watch->path = path;

        // When using a completion routine the hEvent member of the OVERLAPPED structure is not
        // used by the system, so we can use it ourselves.
        watch->overlapped.hEvent = reinterpret_cast<HANDLE>(this);

        // Create file

        std::wstring wstrPath = path.toStdWString();
        watch->handle = CreateFileW(wstrPath.c_str(), GENERIC_READ, FILE_SHARE_WRITE | FILE_SHARE_READ | FILE_SHARE_DELETE,
                                    nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, nullptr);

        if (watch->handle == INVALID_HANDLE_VALUE) {
            qDebug() << "WinDirectoryWatcher::run couldn't watch the path " << path;
            delete watch;
            return;
        }

        // Read Directory contents
        if (readDirectory(watch) == false) {
            qDebug() << "WinDirectoryWatcher::run couldn't watch the path " << path;
            delete watch;
            return;
        }

        qDebug() << "WinDirectoryWatcher::run watching path " << path;
        watchedPaths.insert(path, watch);

    }
}

void WinDirectoryWatcher::removePath(QString path)
{
    qDebug() << "WinDirectoryWatcher::removePath" << path;
    if (watchedPaths.contains(path)) {
        DirectoryWatch *watch = watchedPaths.value(path);
        CancelIoEx(watch->handle, &(watch->overlapped));
        CloseHandle(watch->handle);

        watchedPaths.remove(path);
        delete watch;

        qDebug() << "WinDirectoryWatcher::removePath removed successfully" << path;

    }
}

void WinDirectoryWatcher::directoryChanged(LPOVERLAPPED lpOverLapped)
{
    qDebug() << "WinDirectoryWatcher::directoryChanged";

    // Find the object
    for (DirectoryWatch *watch : watchedPaths.values()) {
        if (&(watch->overlapped) == lpOverLapped && watch->info->Action > 0) {

            FILE_NOTIFY_INFORMATION *n = watch->info;
            QList<QString> strList;
            while (n) {

                QString fileName = QString::fromWCharArray(n->FileName, (n->FileNameLength / sizeof(WCHAR)));
                strList.append(watch->path + (watch->path.right(1) != "\\" ? "\\" : "") + fileName);
                qDebug() << "WinDirectoryWatcher::directoryChaged change detected from" << fileName << "action" << n->Action;

                if (n->NextEntryOffset)
                    n = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(reinterpret_cast<BYTE*>(n) + n->NextEntryOffset);
                else
                    n = nullptr;
            };

            if (watch->info->Action == FILE_ACTION_RENAMED_OLD_NAME && strList.count() == 2) {

                // If this is a rename action we should have two filenames in the string list, old and new.
                emit fileRename(strList[0], strList[1]);

            } else if (strList.count() == 1) {

                // For everything else we should have only one filename in the string list
                switch (watch->info->Action) {
                    case FILE_ACTION_MODIFIED:
                        emit fileModified(strList[0]);
                        break;
                    case FILE_ACTION_ADDED:
                        emit fileAdded(strList[0]);
                        break;
                    case FILE_ACTION_REMOVED:
                        emit fileRemoved(strList[0]);
                        break;
                    default:
                        break;
                }
            }

            // Read contents again
            readDirectory(watch);

            return;
        }
    }
}

void completionRoutine(DWORD dwErrorCode, DWORD dwBytesTransferred, LPOVERLAPPED lpOverLapped)
{
    Q_UNUSED(dwBytesTransferred)

    if (SUCCEEDED(dwErrorCode)) {
        WinDirectoryWatcher *watcher = reinterpret_cast<WinDirectoryWatcher *>(lpOverLapped->hEvent);
        watcher->directoryChanged(lpOverLapped);
    }
}

bool WinDirectoryWatcher::readDirectory(DirectoryWatch *watch)
{

    return ReadDirectoryChangesW(watch->handle, static_cast<LPVOID>(&(watch->info)), sizeof(watch->info),
                                 false,
                                 FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME |
                                 FILE_NOTIFY_CHANGE_ATTRIBUTES | FILE_NOTIFY_CHANGE_SIZE,
                                 &(watch->dwBytesReturned), &(watch->overlapped), &completionRoutine);
}

