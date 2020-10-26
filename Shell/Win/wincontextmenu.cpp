#include <QDebug>
#include <QTime>

#include "wincontextmenu.h"

#define SCRATCH_QCM_FIRST      1
#define SCRATCH_QCM_LAST  0x7FFF

// Menu verbs
#define VERB_VIEW           "view"
#define VERB_SORTBY         "arrange"
#define VERB_GROUPBY        "groupby"
#define VERB_SHARE          "Windows.Share"
#define VERB_MODERNSHARE    "Windows.ModernShare"

WinContextMenu::WinContextMenu(QObject *parent) : ContextMenu(parent)
{

}

void WinContextMenu::show(const WId wId, const QPoint &pos, const QList<FileSystemItem *> fileSystemItems,
                          const ContextMenu::ContextViewAspect viewAspect, QAbstractItemView *view)
{
    qDebug() << "WinContextMenu::show";

    if (!fileSystemItems.size())
        return;

    HRESULT hr;
    LPITEMIDLIST pidl;
    QString mPath = fileSystemItems.at(0)->getPath();

    hr = (mPath == "/")
                ? ::SHGetKnownFolderIDList(FOLDERID_ComputerFolder /* FOLDERID_Desktop */, KF_FLAG_DEFAULT, nullptr, &pidl)
                : ::SHParseDisplayName(mPath.toStdWString().c_str(), nullptr, &pidl, 0, nullptr);

    if (SUCCEEDED(hr)) {
        IShellFolder *psf;
        LPCITEMIDLIST pidlChild {};
        PCUITEMID_CHILD_ARRAY pidlList {};

        // All items have the same parent we just need one to bind to the parent
        if (SUCCEEDED(hr = ::SHBindToParent(pidl, IID_IShellFolder, reinterpret_cast<void**>(&psf), &pidlChild))) {

            HWND hwnd = reinterpret_cast<HWND>(wId);
            IContextMenu *imenu;

            if (viewAspect == ContextMenu::Background) {
                // Context menu was requested on the background of the view

                hr = ::SHBindToObject(nullptr, pidl, nullptr, IID_IShellFolder, reinterpret_cast<void**>(&psf));
                IShellView* pShellView = nullptr;
                psf->CreateViewObject(hwnd, IID_PPV_ARGS(&pShellView));

                hr = pShellView->GetItemObject(SVGIO_BACKGROUND, IID_IContextMenu, reinterpret_cast<void**>(&imenu));

            } else {
                // Context menu was requested on one or more selected items

                pidlList = new LPCITEMIDLIST[static_cast<unsigned long long>(fileSystemItems.size())];
                UINT i = 0;
                for (FileSystemItem *fileSystemItem : fileSystemItems) {

                    // We already have the first one
                    // Saving a couple of nanoseconds here by not calling ParseDisplayName on it :)
                    if (!i) {
                        pidlList[i++] = pidlChild;
                        continue;
                    }

                    LPITEMIDLIST pidlItem;

                    // Path should be relative to the folder, not absolute.  We'll use getDisplayName() for this
                    std::wstring wstrDisplayName = fileSystemItem->getDisplayName().toStdWString();
                    LPWSTR path = const_cast<LPWSTR>(wstrDisplayName.c_str());

                    if (SUCCEEDED(psf->ParseDisplayName(nullptr, nullptr, path, nullptr, &pidlItem, nullptr))) {
                        pidlList[i++] = pidlItem;
                    }
                }

                hr = psf->GetUIObjectOf(hwnd, i, pidlList, IID_IContextMenu, nullptr, reinterpret_cast<void**>(&imenu));
            }

            if (SUCCEEDED(hr)) {
                HMENU hmenu = ::CreatePopupMenu();

                // Populate the menu
                // TODO: CMF_EXTENDEDVERBS should be added if the context menu was right clicked
                qDebug() << "WinContextMenu::show " << QTime::currentTime() << "Before populating the menu";
                if (SUCCEEDED(imenu->QueryContextMenu(hmenu, 0, SCRATCH_QCM_FIRST, SCRATCH_QCM_LAST, CMF_CANRENAME | CMF_EXPLORE))) {

                    // Delete/Add custom menu items
                    qDebug() << "WinContextMenu::show " << QTime::currentTime() << "Before customizing the menu";
                    customizeMenu(imenu, hmenu, viewAspect);

                    imenu->QueryInterface(IID_IContextMenu2, reinterpret_cast<void**>(&imenu2));
                    imenu->QueryInterface(IID_IContextMenu3, reinterpret_cast<void**>(&imenu3));

                    qDebug() << "WinContextMenu::show " << QTime::currentTime() << "Before calling the menu";
                    UINT iCmd = static_cast<UINT>(::TrackPopupMenuEx(hmenu, TPM_RETURNCMD, pos.x(), pos.y(), hwnd, nullptr));
                    if (imenu2) {
                      imenu2->Release();
                      imenu2 = nullptr;
                    }
                    if (imenu3) {
                      imenu3->Release();
                      imenu3 = nullptr;
                    }

                    if (iCmd > 0) {

                        // All entries have the same root and at least there's one
                        QString workingDirectory;
                        FileSystemItem *parent = fileSystemItems[0]->getParent();
                        if (parent)
                            workingDirectory = parent->getPath();

                        CHAR cBuffer[256] {};

                        // Get language-independent verb
                        imenu->GetCommandString(iCmd - SCRATCH_QCM_FIRST, GCS_VERBW, nullptr, cBuffer, 255);

                        QString strVerb = QString::fromWCharArray(reinterpret_cast<wchar_t *>(cBuffer));

                        if (strVerb == "rename" && view != nullptr && fileSystemItems.size() == 1) {

                            FileSystemModel *model = static_cast<FileSystemModel *>(view->model());
                            view->edit(model->index(fileSystemItems[0]));
                        } else
                            invokeCommand(hwnd, iCmd, imenu, pos, workingDirectory, view);
                    }
                }
                ::DestroyMenu(hmenu);
                imenu->Release();
            }
            psf->Release();
            if (pidlList) {
                // We can't free the first one.  Read ::SHBindToParent documentation for more information.
                for (int i = 1; i < fileSystemItems.size(); i++)
                    ::CoTaskMemFree(const_cast<LPITEMIDLIST>(pidlList[i]));

                delete pidlList;
            }
        }

        ::CoTaskMemFree(pidl);
    }
}

void WinContextMenu::defaultAction(const WId wId, const FileSystemItem *fileSystemItem)
{
    qDebug() << "WinContextMenu::defaultAction";

    HRESULT hr;
    LPITEMIDLIST pidl;
    QString mPath = fileSystemItem->getPath();
    if (SUCCEEDED(hr = ::SHParseDisplayName(mPath.toStdWString().c_str(), nullptr, &pidl, 0, nullptr))) {
      IShellFolder *psf;
      LPCITEMIDLIST pidlChild;
      if (SUCCEEDED(hr = ::SHBindToParent(pidl, IID_IShellFolder,  reinterpret_cast<void**>(&psf), &pidlChild))) {

        HWND hwnd = reinterpret_cast<HWND>(wId);
        IContextMenu *imenu;
        hr = psf->GetUIObjectOf(hwnd, 1, &pidlChild, IID_IContextMenu, nullptr, reinterpret_cast<void**>(&imenu));
        if (SUCCEEDED(hr)) {
            HMENU hmenu = ::CreatePopupMenu();
            if (SUCCEEDED(imenu->QueryContextMenu(hmenu, 0, SCRATCH_QCM_FIRST, SCRATCH_QCM_LAST, CMF_DEFAULTONLY))) {

                UINT iCmd = ::GetMenuDefaultItem(hmenu, FALSE, 0);

                if (iCmd != static_cast<UINT>(-1)) {
                    QString workingDirectory;
                    if (fileSystemItem->getParent())
                        workingDirectory = fileSystemItem->getParent()->getPath();

                    invokeCommand(hwnd, iCmd, imenu, QPoint(), workingDirectory, nullptr);
                }
            }
            ::DestroyMenu(hmenu);
            imenu->Release();
        }
        psf->Release();
      }
      ::CoTaskMemFree(pidl);
    }
}

void WinContextMenu::invokeCommand(HWND hwnd, UINT iCmd, IContextMenu *imenu, QPoint pos, QString workingDirectory, QAbstractItemView *view)
{
    UINT command = iCmd - SCRATCH_QCM_FIRST;

    CHAR cBuffer[256] {};

    // Get language-independent verb
    imenu->GetCommandString(command, GCS_VERBW, nullptr, cBuffer, 255);

    QString strVerb = QString::fromWCharArray(reinterpret_cast<wchar_t *>(cBuffer));

    qDebug() << "WinContextMenu::invokeCommand command selected " << command << strVerb;

    CMINVOKECOMMANDINFOEX info = {};
    info.cbSize = sizeof(info);
    info.fMask = CMIC_MASK_ASYNCOK | CMIC_MASK_UNICODE; // | CMIC_MASK_FLAG_NO_UI;
    if (!pos.isNull()) {
        info.fMask |= CMIC_MASK_PTINVOKE;
        info.ptInvoke.x = pos.x();
        info.ptInvoke.y = pos.y();
    }
    info.hwnd = hwnd;
    info.lpVerb  = MAKEINTRESOURCEA(command);
    info.lpVerbW  = MAKEINTRESOURCEW(command);
    info.nShow = SW_SHOWNORMAL;

    if (workingDirectory != nullptr && !workingDirectory.isEmpty()) {
        std::wstring wrkDir = workingDirectory.toStdWString();
        info.lpDirectoryW = wrkDir.c_str();
        qDebug() << workingDirectory;
    }

    HRESULT hr = imenu->InvokeCommand(reinterpret_cast<LPCMINVOKECOMMANDINFO>(&info));
    qDebug() << "WinContextMenu::invokeCommand result code " << QString("%1").arg(hr, 0, 16) << "HWND" << hwnd;
}

bool WinContextMenu::handleNativeEvent(const QByteArray &eventType, void *message, long *result)
{
    Q_UNUSED(eventType)
    Q_UNUSED(result)

    MSG *msg = static_cast< MSG * >( message );
    if (msg->message == WM_INITMENUPOPUP) {
        if (imenu3) {
            LRESULT lres;
            if (SUCCEEDED(imenu3->HandleMenuMsg2(msg->message, msg->wParam, msg->lParam, &lres))) {
              return true;
            }
        } else if (imenu2) {
            if (SUCCEEDED(imenu2->HandleMenuMsg(msg->message, msg->wParam, msg->lParam))) {
              return true;
            }
        }
    }
    return false;
}

void WinContextMenu::customizeMenu(IContextMenu *imenu, const HMENU hmenu, const ContextMenu::ContextViewAspect viewAspect)
{
    // Browse the explorer menu
    int count = GetMenuItemCount(hmenu);
    qDebug() << "Menu items" << count;
    wchar_t buffer[1024];
    QString verb;
    QList<UINT> verbsToDelete;
    for (int i = 0 ; i < count; i++) {
        MENUITEMINFO info {};
        info.fMask = MIIM_ID | MIIM_FTYPE | MIIM_STRING;
        info.fType = MFT_STRING;
        info.cbSize = sizeof(MENUITEMINFO);
        info.cch = 1024;
        info.dwTypeData = buffer;
        if (!GetMenuItemInfoW(hmenu, static_cast<UINT>(i), TRUE, &info))
            qDebug() << "failed";
        CHAR cBuffer[256] {};

        if (!(info.fType & MFT_SEPARATOR) && info.wID > SCRATCH_QCM_FIRST) {

            // Get language-independent verb
            imenu->GetCommandString(info.wID - SCRATCH_QCM_FIRST, GCS_VERBW, nullptr, cBuffer, 255);
            verb = QString::fromWCharArray(reinterpret_cast<wchar_t *>(cBuffer));
            if (viewAspect == ContextMenu::Background) {

                // Delete default explorer entries for the background view
                // We will add our own implementation later
                if (verb == VERB_VIEW || verb == VERB_SORTBY || verb == VERB_GROUPBY || verb == VERB_SHARE)
                    verbsToDelete.append(info.wID);

            } else {

                // Delete UWP calls. They will get an INVALID_WINDOW_HANDLE (0x80070578) error anyway
                if (verb == VERB_MODERNSHARE)
                    verbsToDelete.append(info.wID);
            }
        }

        qDebug() << info.wID << QString::fromWCharArray(info.dwTypeData) << "VERB: " << verb;
        verb.clear();

        // Delete separators at the end
        if (i == (count - 1) && (info.fType & MFT_SEPARATOR))
            ::DeleteMenu(hmenu, static_cast<UINT>(i), MF_BYPOSITION);
    }

    // Delete verbs we marked that we do not want
    for (UINT id : verbsToDelete)
        ::DeleteMenu(hmenu, id, MF_BYCOMMAND);

}
