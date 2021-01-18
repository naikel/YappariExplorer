#include <QDebug>

#include <wchar.h>
#include <ioapiset.h>

#include "windirectorywatcher.h"

int WinDirectoryWatcher::id {};
QList<LPOVERLAPPED> validOverlapped {};

WinDirectoryWatcher::WinDirectoryWatcher(QObject *parent) : DirectoryWatcher(parent)
{
    thisId = id++;
    qDebug() << "WinDirectoryWatcher::WinDirectoryWatcher registered with id" << id;
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
    qDebug() << "WinDirectoryWatcher::run addPath " << thisId << path;

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
            qDebug() << "WinDirectoryWatcher::run couldn't watch the path " << thisId << path;
            delete watch;
            return;
        }

        // Read Directory contents
        if (readDirectory(watch) == false) {
            qDebug() << "WinDirectoryWatcher::run couldn't watch the path " << thisId << path;
            delete watch;
            return;
        }

        mutex.lock();
        watchedPaths.insert(path, watch);
        validOverlapped.append(&watch->overlapped);
        mutex.unlock();
        qDebug() << "WinDirectoryWatcher::run watching path " << thisId << path;
    }
}

void WinDirectoryWatcher::removePath(QString path)
{
    qDebug() << "WinDirectoryWatcher::removePath" << path;
    if (watchedPaths.contains(path)) {
        DirectoryWatch *watch = watchedPaths.value(path);
        CancelIoEx(watch->handle, &(watch->overlapped));
        CloseHandle(watch->handle);

        mutex.lock();
        watchedPaths.remove(path);
        validOverlapped.removeAll(&watch->overlapped);
        delete watch;
        mutex.unlock();

        qDebug() << "WinDirectoryWatcher::removePath removed successfully" << thisId << path;

    }
}

void WinDirectoryWatcher::directoryChanged(LPOVERLAPPED lpOverLapped)
{
    qDebug() << "WinDirectoryWatcher::directoryChanged starting" << thisId ;

    // Find the object
    mutex.lock();
    qDebug() << "WinDirectoryWatcher::directoryChanged locked sucessfully" << thisId ;
    for (DirectoryWatch *watch : watchedPaths.values()) {
        if (&(watch->overlapped) == lpOverLapped && watch->info->Action > 0) {

            FILE_NOTIFY_INFORMATION *n = watch->info;
            QList<QString> strList;
            while (n) {

                QString fileName = QString::fromWCharArray(n->FileName, (n->FileNameLength / sizeof(WCHAR)));
                strList.append(watch->path + (watch->path.right(1) != "\\" ? "\\" : "") + fileName);

                QString action;
                switch (n->Action) {
                    case FILE_ACTION_MODIFIED:
                        action = "FILE_ACTION_MODIFIED";
                        break;
                    case FILE_ACTION_ADDED:
                        action = "FILE_ACTION_ADDED";
                        break;
                    case FILE_ACTION_REMOVED:
                        action = "FILE_ACTION_REMOVED";
                        break;
                    case FILE_ACTION_RENAMED_OLD_NAME:
                        action = "FILE_ACTION_RENAMED_OLD_NAME";
                        break;
                    case FILE_ACTION_RENAMED_NEW_NAME:
                        action = "FILE_ACTION_RENAMED_NEW_NAME";
                        break;
                    default:
                        action = "UNKNOWN_ACTION";
                        break;
                }

                qDebug() << "WinDirectoryWatcher::directoryChanged change detected from" << fileName << "action" << action << n->Action;

                if (n->NextEntryOffset)
                    n = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(reinterpret_cast<BYTE*>(n) + n->NextEntryOffset);
                else
                    n = nullptr;
            };

            qDebug() << "WinDirectoryWatcher::directoryChanged processing" << strList.count() << "events";

            for (int i = 0; i < strList.count(); i++) {

                if (watch->info->Action == FILE_ACTION_RENAMED_OLD_NAME && ((i + 1) < strList.count())) {

                    // If this is a rename action we should have two filenames in the string list, old and new.
                    qDebug() << "RENAME" << strList[i] << strList[i+1];
                    emit fileRename(strList[i], strList[i + 1]);
                    i += 1;
                    continue;
                }

                switch (watch->info->Action) {
                    case FILE_ACTION_MODIFIED:
                        emit fileModified(strList[i]);
                        break;
                    case FILE_ACTION_ADDED:
                        emit fileAdded(strList[i]);
                        break;
                    case FILE_ACTION_REMOVED:
                        emit fileRemoved(strList[i]);
                        break;
                    default:
                        break;
                }
            }

            // Read contents again
            readDirectory(watch);

            break;
        }
    }

    qDebug() << "WinDirectoryWatcher::directoryChanged attempting to unlock" << thisId;
    mutex.unlock();
    qDebug() << "WinDirectoryWatcher::directoryChanged unlocked sucessfully" << thisId;
}

void completionRoutine(DWORD dwErrorCode, DWORD dwBytesTransferred, LPOVERLAPPED lpOverLapped)
{
    Q_UNUSED(dwBytesTransferred)

    if (SUCCEEDED(dwErrorCode) && validOverlapped.contains(lpOverLapped)) {
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

