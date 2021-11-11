#include <QtConcurrent/QtConcurrent>
#include <QDebug>

#include <shlobj.h>

#include "WinDirectoryWatcher.h"

#include "Window/AppWindow.h"

#define COMPUTER_FOLDER_GUID          "::{20D04FE0-3AEA-1069-A2D8-08002B30309D}"

WinDirectoryWatcher::WinDirectoryWatcher(QObject *parent) : DirectoryWatcher(parent)
{
    id = AppWindow::instance()->registerWatcher(this);
}

WinDirectoryWatcher::~WinDirectoryWatcher()
{
    qDebug() << "WinDirectoryWatcher::~WinDirectoryWatcher About to destroy" << id;

    QStringList paths = watchedPaths.keys();
    for (QString path : paths) {

        QList<Watcher *> watchers = watchedPaths.values(path);
        for (Watcher *value : watchers)
            removeWatch(value);
    }

    qDebug() << "WinDirectoryWatcher::~WinDirectoryWatcher Destroyed" << id;
}

void WinDirectoryWatcher::addItem(FileSystemItem *item)
{
    if (item == nullptr)
        return;

    QString path = item->getPath();

    if (!watchedPaths.contains(path)) {

        LPITEMIDLIST pidl;
        HRESULT hr = ::SHParseDisplayName(path.toStdWString().c_str(), nullptr, &pidl, 0, nullptr);

        if (SUCCEEDED(hr)) {

            HWND hwnd = reinterpret_cast<HWND>(AppWindow::instance()->getWindowId());

            SHChangeNotifyEntry const entries[] = { { pidl, false } };

            int nSources = SHCNRF_ShellLevel | SHCNRF_InterruptLevel | SHCNRF_NewDelivery;

            ULONG regId;
            if (SUCCEEDED(regId = SHChangeNotifyRegister(hwnd, nSources, SHCNE_ALLEVENTS, id, ARRAYSIZE(entries), entries))) {

                Watcher *watcher = new Watcher({ item, path, regId });
                watchedPaths.insert(path, watcher);

                qDebug() << "WinDirectoryWatcher::addPath watching path " << id << path << watchedPaths.keys();
            }

            ::CoTaskMemFree(pidl);
        }
    }

}

void WinDirectoryWatcher::removeItem(FileSystemItem *item)
{
    if (item == nullptr)
        return;

    QString path = item->getPath() + '\\';

    qDebug() << "WinDirectoryWatcher::removeItem" << id << path;

    QList<Watcher *> watchers = watchedPaths.values();
    for (Watcher *watcher : watchers) {

        if (watcher->item == item || watcher->path.left(path.length()) == path)
            removeWatch(watcher);
    }
}

void WinDirectoryWatcher::removeWatch(Watcher *watcher)
{
    QString path = watcher->path;
    ULONG regId = watcher->regId;
    SHChangeNotifyDeregister(regId);

    QString pathKey = watchedPaths.key(watcher);
    if (!pathKey.isEmpty())
        watchedPaths.remove(pathKey, watcher);

    delete watcher;

    qDebug() << "WinDirectoryWatcher::removeItem removed successfully" << id << path << watchedPaths.keys();
}

bool WinDirectoryWatcher::isWatching(FileSystemItem *item)
{
    QList<Watcher *> watchers = watchedPaths.values();
    for (Watcher *watcher : watchers)
        if (watcher->item == item)
            return true;

    return false;
}

void WinDirectoryWatcher::refresh()
{
    QList<Watcher *> watchers = watchedPaths.values();
    for (Watcher *watcher : watchers) {
        if (watcher->item->getPath() != watcher->path) {

            FileSystemItem *item = watcher->item;

            qDebug() << "WinDirectoryWatcher::refresh refreshing" << watcher->path << "to" << item->getPath();

            removeWatch(watcher);
            addItem(item);
        }
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

            QtConcurrent::run(this, &WinDirectoryWatcher::directoryChange, lEvent, strPath1, strPath2);

            r = true;
            *result = 0;
        }
    } else {
        r = false;
    }
    SHChangeNotification_Unlock(hNotifyLock);
    return r;
}


void WinDirectoryWatcher::directoryChange(long lEvent, QString strPath1, QString strPath2)
{

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

            QList<Watcher *> watchers = watchedPaths.values(parentPath);
            for (Watcher *watcher : watchers) {

                FileSystemItem *item = (parentPath == strPath1) ? watcher->item : watcher->item->getChild(strPath1);

                qDebug() << "WinDirectoryWatcher::directoryChange notification received " << id << strEvent << strPath1 << strPath2 << watchedPaths.keys();

                switch (lEvent) {
                    case SHCNE_CREATE:
                    case SHCNE_MKDIR:
                    case SHCNE_DRIVEADD:
                        if (item == nullptr)
                            emit fileAdded(watcher->item, strPath1);
                        break;
                    case SHCNE_RENAMEFOLDER:
                    case SHCNE_RENAMEITEM:
                        if (item != nullptr)
                            emit fileRename(item, strPath2);
                        break;
                    case SHCNE_DELETE:
                    case SHCNE_RMDIR:
                    case SHCNE_DRIVEREMOVED:
                        if (item != nullptr)
                            emit fileRemoved(item);
                        break;
                    case SHCNE_UPDATEITEM:
                        if (item != nullptr)
                            emit fileModified(item);
                        break;
                    case SHCNE_UPDATEDIR:
                        if (item != nullptr)
                            emit folderUpdated(item);
                        break;
                }
            }
        }
    }
}

