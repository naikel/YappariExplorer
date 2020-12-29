#include <QDebug>

#include <shlobj.h>

#include "windirectorywatcherv2.h"

#include "Window/AppWindow.h"

WinDirectoryWatcherv2::WinDirectoryWatcherv2(QObject *parent) : DirectoryWatcher(parent)
{
    id = AppWindow::registerWatcher(this);
}

WinDirectoryWatcherv2::~WinDirectoryWatcherv2()
{
    qDebug() << "WinDirectoryWatcher::~WinDirectoryWatcher About to destroy";

    QStringList paths = watchedPaths.keys();
    for (QString path : paths)
        removePath(path);

    qDebug() << "WinDirectoryWatcher::~WinDirectoryWatcher Destroyed";
}

void WinDirectoryWatcherv2::addPath(QString path)
{
    LPITEMIDLIST pidl;
    HRESULT hr;

    hr = ::SHParseDisplayName(path.toStdWString().c_str(), nullptr, &pidl, 0, nullptr);

    if (SUCCEEDED(hr)) {

        HWND hwnd = reinterpret_cast<HWND>(AppWindow::getWinId());

        SHChangeNotifyEntry const entries[] = { { pidl, false } };

        int nSources = SHCNRF_ShellLevel | SHCNRF_InterruptLevel | SHCNRF_NewDelivery;

        ULONG regId;
        if (SUCCEEDED(regId = SHChangeNotifyRegister(hwnd, nSources, SHCNE_ALLEVENTS, id, ARRAYSIZE(entries), entries))) {

            watchedPaths.insert(path, regId);

            qDebug() << "WinDirectoryWatcher::run watching path " << path;
        }

        ::CoTaskMemFree(pidl);
    }
}

void WinDirectoryWatcherv2::removePath(QString path)
{
    qDebug() << "WinDirectoryWatcher::removePath" << path;
    if (watchedPaths.contains(path)) {
        SHChangeNotifyDeregister(watchedPaths.value(path));
        qDebug() << "WinDirectoryWatcher::removePath removed successfully" << path;
    }
}

bool WinDirectoryWatcherv2::handleNativeEvent(const QByteArray &eventType, void *message, long *result)
{
    Q_UNUSED(eventType)
    bool r {};
    MSG *msg = static_cast< MSG * >(message);
    long lEvent;
    PIDLIST_ABSOLUTE *rgpidl;
    HANDLE hNotifyLock = SHChangeNotification_Lock((HANDLE)msg->wParam, (DWORD)msg->lParam, &rgpidl, &lEvent);
    if (hNotifyLock) {
        if (!(lEvent & (SHCNE_UPDATEIMAGE | SHCNE_ASSOCCHANGED | SHCNE_EXTENDED_EVENT | SHCNE_FREESPACE | SHCNE_DRIVEADDGUI | SHCNE_SERVERDISCONNECT))) {

            WCHAR path1[MAX_PATH], path2[MAX_PATH];

            QString strPath1, strPath2;

            if (rgpidl[0]) {
                SHGetPathFromIDListW(rgpidl[0], path1);
                strPath1 = QString::fromWCharArray(path1);
            }

            if (rgpidl[1]) {
                SHGetPathFromIDListW(rgpidl[1], path2);
                strPath2 = QString::fromWCharArray(path2);
            }

            qDebug() << "WinDirectoryWatcher::handleNativeEvent notification received " << lEvent << strPath1 << strPath2;

            switch (lEvent) {
                case SHCNE_CREATE:
                case SHCNE_MKDIR:
                case SHCNE_DRIVEADD:
                    emit fileAdded(strPath1);
                    break;
                case SHCNE_RENAMEFOLDER:
                case SHCNE_RENAMEITEM:
                    emit fileRename(strPath1, strPath2);
                    break;
                case SHCNE_DELETE:
                case SHCNE_RMDIR:
                case SHCNE_DRIVEREMOVED:
                    emit fileRemoved(strPath1);
                    break;
                case SHCNE_UPDATEITEM:
                    emit fileModified(strPath1);
                    break;
                case SHCNE_UPDATEDIR:
                    // TODO: Need to update the whole dir
                    break;
            }

            r = true;
            *result = 0;

        }
        SHChangeNotification_Unlock(hNotifyLock);
    }

    return r;
}

