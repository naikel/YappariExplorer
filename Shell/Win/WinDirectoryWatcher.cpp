#include <QDebug>

#include <shlobj.h>

#include "WinDirectoryWatcher.h"

#include "Window/AppWindow.h"

#define COMPUTER_FOLDER_GUID          "::{20D04FE0-3AEA-1069-A2D8-08002B30309D}"

WinDirectoryWatcher::WinDirectoryWatcher(QObject *parent) : DirectoryWatcher(parent)
{
    id = AppWindow::instance()->registerWatcher(this);

    // Use ReadDirectoryChanges to get faster renames and updates but they don't work in every file system
    dirChangeNotifier = new WinDirChangeNotifier(this);
    connect(dirChangeNotifier, &WinDirChangeNotifier::fileRename, [=](QString oldPath, QString newPath) { this->emit fileRename(oldPath, newPath); });
    connect(dirChangeNotifier, &WinDirChangeNotifier::fileModified, [=](QString path ) { this->emit fileModified(path); });
}

WinDirectoryWatcher::~WinDirectoryWatcher()
{
    qDebug() << "WinDirectoryWatcher::~WinDirectoryWatcher About to destroy" << id;

    QStringList paths = watchedPaths.keys();
    for (QString path : paths)
        removePath(path);

    delete dirChangeNotifier;

    qDebug() << "WinDirectoryWatcher::~WinDirectoryWatcher Destroyed" << id;
}

void WinDirectoryWatcher::addPath(QString path)
{
    if (!watchedPaths.contains(path)) {

        LPITEMIDLIST pidl;
        HRESULT hr = ::SHParseDisplayName(path.toStdWString().c_str(), nullptr, &pidl, 0, nullptr);

        if (SUCCEEDED(hr)) {

            HWND hwnd = reinterpret_cast<HWND>(AppWindow::instance()->getWindowId());

            SHChangeNotifyEntry const entries[] = { { pidl, false } };

            int nSources = SHCNRF_ShellLevel | SHCNRF_InterruptLevel | SHCNRF_NewDelivery;

            ULONG regId;
            if (SUCCEEDED(regId = SHChangeNotifyRegister(hwnd, nSources, SHCNE_ALLEVENTS, id, ARRAYSIZE(entries), entries))) {

                watchedPaths.insert(path, regId);

                qDebug() << "WinDirectoryWatcher::addPath watching path " << id << path << watchedPaths;
            }

            ::CoTaskMemFree(pidl);
        }

        dirChangeNotifier->addPath(path);
    }

}

void WinDirectoryWatcher::removePath(QString path)
{
    qDebug() << "WinDirectoryWatcher::removePath" << id << path;
    if (watchedPaths.contains(path)) {
        int regId = watchedPaths.value(path);
        if (regId >= 0)
            SHChangeNotifyDeregister(regId);
        watchedPaths.remove(path);

        dirChangeNotifier->removePath(path);

        qDebug() << "WinDirectoryWatcher::removePath removed successfully" << id << path << watchedPaths;
    }
}

bool WinDirectoryWatcher::handleNativeEvent(const QByteArray &eventType, void *message, long *result)
{
    Q_UNUSED(eventType)
    bool r {};
    MSG *msg = static_cast< MSG * >(message);
    long lEvent;
    PIDLIST_ABSOLUTE *rgpidl;
    HANDLE hNotifyLock = SHChangeNotification_Lock((HANDLE)msg->wParam, (DWORD)msg->lParam, &rgpidl, &lEvent);
    if (hNotifyLock) {
        if (!(lEvent & (SHCNE_UPDATEIMAGE | SHCNE_ASSOCCHANGED | SHCNE_EXTENDED_EVENT | SHCNE_DRIVEADDGUI | SHCNE_SERVERDISCONNECT))) {

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

            if (!strPath1.isEmpty()) {

                QString strEvent;
                switch (lEvent) {
                    case SHCNE_CREATE:
                        strEvent = "SHCNE_CREATE";
                        break;
                    case SHCNE_MKDIR:
                        strEvent = "SHCNE_MKDIR";
                        break;
                    case SHCNE_DRIVEADD:
                        strEvent = "SHCNE_DRIVEADD";
                        break;
                    case SHCNE_RENAMEFOLDER:
                        strEvent = "SHCNE_RENAMEFOLDER";
                        break;
                    case SHCNE_RENAMEITEM:
                        strEvent = "SHCNE_RENAMEITEM";
                        break;
                    case SHCNE_DELETE:
                        strEvent = "SHCNE_DELETE";
                        break;
                    case SHCNE_RMDIR:
                        strEvent = "SHCNE_RMDIR";
                        break;
                    case SHCNE_DRIVEREMOVED:
                        strEvent = "SHCNE_DRIVEREMOVED";
                        break;
                    case SHCNE_UPDATEITEM:
                        strEvent = "SHCNE_UPDATEITEM";
                        break;
                    case SHCNE_UPDATEDIR:
                        strEvent = "SHCNE_UPDATEDIR";
                        break;
                    default:
                        strEvent = "UNKNOWN";

                }

                int index = strPath1.lastIndexOf('\\');
                QString parentPath = strPath1.left(index);

                if (parentPath.length() == 2 && parentPath[1] == ':')
                    parentPath += '\\';


                if (watchedPaths.contains(parentPath)) {

                    qDebug() << "WinDirectoryWatcher::handleNativeEvent notification received " << id << strEvent << strPath1 << strPath2 << watchedPaths;

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
                            emit folderUpdated(strPath1);
                            break;
                    }

                    r = true;
                    *result = 0;
                }
            } else {
                r = false;
            }
        }
        SHChangeNotification_Unlock(hNotifyLock);
    }

    return r;
}

