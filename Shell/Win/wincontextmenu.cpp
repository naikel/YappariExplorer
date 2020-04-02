#include <QDebug>

#include "wincontextmenu.h"

#define SCRATCH_QCM_FIRST      1
#define SCRATCH_QCM_LAST  0x7FFF

// Menu verbs
#define VERB_VIEW     "view"
#define VERB_SORTBY   "arrange"
#define VERB_GROUPBY  "groupby"

WinContextMenu::WinContextMenu(QObject *parent) : ContextMenu(parent)
{

}

void WinContextMenu::show(const WId wId, const QPoint &pos, const QList<FileSystemItem *> fileSystemItems, const ContextMenu::ContextViewAspect viewAspect)
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
                    LPWSTR path = const_cast<LPWSTR>(fileSystemItem->getDisplayName().toStdWString().c_str());

                    if (SUCCEEDED(psf->ParseDisplayName(nullptr, nullptr, path, nullptr, &pidlItem, nullptr))) {
                        pidlList[i++] = pidlItem;
                    }
                }

                hr = psf->GetUIObjectOf(hwnd, i, pidlList, IID_IContextMenu, nullptr, reinterpret_cast<void**>(&imenu));
            }

            if (SUCCEEDED(hr)) {
                HMENU hmenu = ::CreatePopupMenu();

                // Populate the menu
                // ToDo: CMF_EXTENDEDVERBS should be added if the context menu was right clicked
                if (SUCCEEDED(imenu->QueryContextMenu(hmenu, 0, SCRATCH_QCM_FIRST, SCRATCH_QCM_LAST, CMF_CANRENAME | CMF_EXPLORE))) {

                    // Delete/Add custom menu items
                    customizeMenu(imenu, hmenu, viewAspect);

                    imenu->QueryInterface(IID_IContextMenu2, reinterpret_cast<void**>(&imenu2));
                    imenu->QueryInterface(IID_IContextMenu3, reinterpret_cast<void**>(&imenu3));

                    int iCmd = ::TrackPopupMenuEx(hmenu, TPM_RETURNCMD, pos.x(), pos.y(), hwnd, nullptr);
                    if (imenu2) {
                      imenu2->Release();
                      imenu2 = nullptr;
                    }
                    if (imenu3) {
                      imenu3->Release();
                      imenu3 = nullptr;
                    }

                    if (iCmd > 0) {
                        qDebug() << "WinContextMenu::show command selected " << iCmd;
                        CMINVOKECOMMANDINFOEX info = {};
                        info.cbSize = sizeof(info);
                        info.fMask = CMIC_MASK_ASYNCOK | CMIC_MASK_UNICODE | CMIC_MASK_PTINVOKE; // | CMIC_MASK_FLAG_NO_UI;
                        info.hwnd = hwnd;
                        info.lpVerb  = MAKEINTRESOURCEA(iCmd - SCRATCH_QCM_FIRST);
                        info.lpVerbW  = MAKEINTRESOURCEW(iCmd - SCRATCH_QCM_FIRST);
                        info.nShow = SW_SHOWNORMAL;
                        info.ptInvoke.x = pos.x();
                        info.ptInvoke.y = pos.y();
                        HRESULT hr = imenu->InvokeCommand(reinterpret_cast<LPCMINVOKECOMMANDINFO>(&info));
                        qDebug() << "WinContextMenu::show result code " << hr;
                    }
                }
                DestroyMenu(hmenu);
                imenu->Release();
            }
            psf->Release();
            if (pidlList) {
                // We can't free the first one.  Read ::SHBindToParent documentation for more information.
                for (int i = 1; i < fileSystemItems.size(); i++)
                    CoTaskMemFree(const_cast<LPITEMIDLIST>(pidlList[i]));

                delete pidlList;
            }
        }

        CoTaskMemFree(pidl);
    }
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

        qDebug() << info.wID << QString::fromWCharArray(info.dwTypeData);

        if (!(info.fType & MFT_SEPARATOR) && info.wID > SCRATCH_QCM_FIRST) {

            // Get language-independent verb
            imenu->GetCommandString(info.wID - SCRATCH_QCM_FIRST, GCS_VERBW, nullptr, cBuffer, 255);
            verb = QString::fromWCharArray(reinterpret_cast<wchar_t *>(cBuffer));
            if (viewAspect == ContextMenu::Background) {

                if (verb == VERB_VIEW || verb == VERB_SORTBY || verb == VERB_GROUPBY)
                    verbsToDelete.append(info.wID);
            }
        }

        qDebug() << "VERB: " << verb;

        // Delete separators at the end
        if (i == (count - 1) && (info.fType & MFT_SEPARATOR))
            DeleteMenu(hmenu, static_cast<UINT>(i), MF_BYPOSITION);
    }

    // Delete verbs we do not want
    for (UINT id : verbsToDelete)
        DeleteMenu(hmenu, id, MF_BYCOMMAND);

}
