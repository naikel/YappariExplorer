#include <QDebug>

#include <wchar.h>

#include "windirectorywatcher.h"

WinDirectoryWatcher::WinDirectoryWatcher(QString path, QObject *parent) : QThread(parent)
{
    this->path = path;
}

WinDirectoryWatcher::~WinDirectoryWatcher()
{
    qDebug() << "WinDirectoryWatcher::~WinDirectoryWatcher About to destroy";
    stop();
    wait();
    qDebug() << "WinDirectoryWatcher::~WinDirectoryWatcher Destroyed";
}

void WinDirectoryWatcher::run()
{
    // Create the handle

    std::wstring wstrPath = path.toStdWString();
    handle = CreateFileW(wstrPath.c_str(), GENERIC_READ, FILE_SHARE_WRITE | FILE_SHARE_READ | FILE_SHARE_DELETE,
                         nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, nullptr);

    if (handle == INVALID_HANDLE_VALUE) {
        qDebug() << "WinDirectoryWatcher::run couldn't watch the path " << path;
        return;
    }

    event = CreateEventW(nullptr, true, false, nullptr);

    forever {

        qDebug() << "WinDirectoryWatcher::run watching" << path;

        FILE_NOTIFY_INFORMATION info[32];
        DWORD dwBytesReturned = 0;
        OVERLAPPED overlapped;
        overlapped.hEvent = event;

        if (!ReadDirectoryChangesW(handle, static_cast<LPVOID>(&info), sizeof(info),
                                   false,
                                   FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME |
                                   FILE_NOTIFY_CHANGE_ATTRIBUTES | FILE_NOTIFY_CHANGE_SIZE,
                                   &dwBytesReturned, &overlapped, nullptr)) {

            qDebug() << "WinDirectoryWatcher::run couldn't read directory changes";
            if (handle) {
                CloseHandle(handle);
                handle = 0;
            }
            if (event) {
                CloseHandle(event);
                event = 0;
            }
            return;
        }

        WaitForSingleObject(event, INFINITE);

        if (stopRequested) {
            qDebug() << "WinDirectoryWatcher::run this thread is stopping";
            if (handle) {
                CloseHandle(handle);
                handle = 0;
            }
            if (event) {
                CloseHandle(event);
                event = 0;
            }
            return;
        }

        FILE_NOTIFY_INFORMATION *n = info;
        QList<QString> strList;
        while (n) {

            QString fileName = QString::fromWCharArray(n->FileName, (n->FileNameLength / sizeof(WCHAR)));
            strList.append(path + (path.right(1) != "\\" ? "\\" : "") + fileName);
            qDebug() << "WinDirectoryWatcher::run change detected from" << fileName << "action" << n->Action;

            if (n->NextEntryOffset)
                n = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(reinterpret_cast<BYTE*>(n) + n->NextEntryOffset);
            else
                n = nullptr;
        };

        // If this is a rename action we should have two filenames in the string list, old and new.
        if (info->Action == FILE_ACTION_RENAMED_OLD_NAME && strList.count() == 2)
            emit fileRename(strList[0], strList[1]);

        if (event)
            ResetEvent(event);
    }
}

void WinDirectoryWatcher::stop()
{
    qDebug() << "WinDirectoryWatcher::stop";
    stopRequested = true;

    if (event)
        SetEvent(event);
}
