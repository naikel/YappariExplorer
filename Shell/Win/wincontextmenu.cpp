#include <QApplication>
#include <QDebug>
#include <QTime>

#include "View/Base/basetreeview.h"
#include "wincontextmenu.h"

#define SCRATCH_QCM_FIRST      1
#define SCRATCH_QCM_LAST  0x7FFF

// Menu verbs
#define VERB_VIEW               "view"
#define VERB_SORTBY             "arrange"
#define VERB_GROUPBY            "groupby"
#define VERB_UNDO               "undo"
#define VERB_DELETE             "delete"
#define VERB_RENAME             "rename"
#define VERB_REFRESH            "refresh"
#define VERB_SHARE              "Windows.Share"
#define VERB_MODERNSHARE        "Windows.ModernShare"
#define VERB_VIEWCUSTOMWIZARD   "viewcustomwizard"

WinContextMenu::WinContextMenu(QObject *parent) : ContextMenu(parent)
{

}

void WinContextMenu::show(const WId wId, const QPoint &pos, const QList<FileSystemItem *> fileSystemItems,
                          const ContextMenu::ContextViewAspect viewAspect, QAbstractItemView *view)
{
    qDebug() << "WinContextMenu::show";

    ::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
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
            FileSystemItem *parent {};

            if (viewAspect == ContextMenu::Background) {
                // Context menu was requested on the background of the view

                psf->Release();
                hr = ::SHBindToObject(nullptr, pidl, nullptr, IID_IShellFolder, reinterpret_cast<void**>(&psf));
                IShellView* pShellView = nullptr;
                psf->CreateViewObject(hwnd, IID_PPV_ARGS(&pShellView));

                parent = fileSystemItems[0];

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

                parent = fileSystemItems[0]->getParent();

                hr = psf->GetUIObjectOf(hwnd, i, pidlList, IID_IContextMenu, nullptr, reinterpret_cast<void**>(&imenu));


            }

            if (SUCCEEDED(hr)) {
                HMENU hmenu = ::CreatePopupMenu();

                // Populate the menu

                UINT uFlags = CMF_CANRENAME | CMF_EXPLORE;
                if (QApplication::keyboardModifiers() & Qt::ShiftModifier)
                    uFlags |= CMF_EXTENDEDVERBS;

                qDebug() << "WinContextMenu::show " << QTime::currentTime() << "Before populating the menu";
                if (SUCCEEDED(imenu->QueryContextMenu(hmenu, 0, SCRATCH_QCM_FIRST, SCRATCH_QCM_LAST, uFlags))) {

                    // Delete/Add custom menu items
                    qDebug() << "WinContextMenu::show " << QTime::currentTime() << "Before customizing the menu";
                    customizeMenu(imenu, hmenu, viewAspect);

                    if (FAILED(imenu->QueryInterface(IID_IContextMenu2, reinterpret_cast<void**>(&imenu2))))
                        qDebug() << "imenu2 failed";

                    if (FAILED(imenu->QueryInterface(IID_IContextMenu3, reinterpret_cast<void**>(&imenu3))))
                        qDebug() << "imenu3 failed";

                    qDebug() << "WinContextMenu::show " << QTime::currentTime() << "Before calling the menu";
                    UINT iCmd = static_cast<UINT>(::TrackPopupMenuEx(hmenu, TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_LEFTALIGN, pos.x(), pos.y(), hwnd, nullptr));
                    if (imenu2) {
                      imenu2->Release();
                      imenu2 = nullptr;
                    }
                    if (imenu3) {
                      imenu3->Release();
                      imenu3 = nullptr;
                    }

                    if (iCmd > 0) {

                        // Get language-independent verb
                        CHAR cBuffer[256] {};
                        imenu->GetCommandString(iCmd - SCRATCH_QCM_FIRST, GCS_VERBW, nullptr, cBuffer, 255);

                        QString strVerb = QString::fromWCharArray(reinterpret_cast<wchar_t *>(cBuffer));

                        if (strVerb == VERB_RENAME && view != nullptr && fileSystemItems.size() == 1) {

                            FileSystemModel *model = static_cast<FileSystemModel *>(view->model());
                            view->edit(model->index(fileSystemItems[0]));

                        } else if (strVerb == VERB_DELETE && view != nullptr) {

                            // TODO: We need a QAbstractItemView implementation of this
                            // like get selected items and then call model()->removeIndexes

                            BaseTreeView *baseView = static_cast<BaseTreeView*>(view);
                            baseView->deleteSelectedItems();

                        } else if (strVerb == VERB_REFRESH && view != nullptr) {

                            FileSystemModel *model = static_cast<FileSystemModel *>(view->model());
                            model->refresh();

                        } else {

                            // All entries have the same root and at least there's one
                            invokeCommand(hwnd, iCmd, imenu, pos, parent, view);
                        }
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

        ::CoUninitialize();
    }
}

void WinContextMenu::defaultAction(const WId wId, const FileSystemItem *fileSystemItem)
{
    qDebug() << "WinContextMenu::defaultAction";

    ::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

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

                    invokeCommand(hwnd, iCmd, imenu, QPoint(), fileSystemItem->getParent(), nullptr);
                }
            }
            ::DestroyMenu(hmenu);
            imenu->Release();
        }
        psf->Release();
      }
      ::CoTaskMemFree(pidl);
    }

    ::CoUninitialize();
}

void WinContextMenu::invokeCommand(HWND hwnd, UINT iCmd, IContextMenu *imenu, QPoint pos, FileSystemItem *parent, QAbstractItemView *view)
{
    UINT command = iCmd - SCRATCH_QCM_FIRST;

    CHAR cBuffer[256] {};

    // This holds the working directory data it has to be valid until the InvokeCommand call
    std::wstring wstrWrkDir;
    std::string strWrkDir;

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

    if (parent != nullptr) {
        wstrWrkDir = parent->getPath().toStdWString();
        strWrkDir = parent->getPath().toStdString();

        // Pointers to wstrWrkDir (std::wstring) and strWrkDir (std::string) have to be valid until the InvokeCommand call

        info.lpDirectoryW = wstrWrkDir.c_str();
        info.lpDirectory = strWrkDir.c_str();
    }

    // Get language-independent verb
    imenu->GetCommandString(command, GCS_VERBW, nullptr, cBuffer, 255);

    QString strVerb = QString::fromWCharArray(reinterpret_cast<wchar_t *>(cBuffer));

    // Tell the view's model to watch for new objects so the view can ask the user to rename them
    if (strVerb.size() > 1 && (strVerb == CMDSTR_NEWFOLDERA || strVerb.left(1) == ".")) {
        FileSystemModel *model = reinterpret_cast<FileSystemModel *>(view->model());
        model->startWatch(parent, strVerb);
    }

    qDebug() << "WinContextMenu::invokeCommand command selected " << command << strVerb << "working directory" << wstrWrkDir;

    HRESULT hr = imenu->InvokeCommand(reinterpret_cast<LPCMINVOKECOMMANDINFO>(&info));
    qDebug() << "WinContextMenu::invokeCommand result code " << QString("%1").arg(hr, 0, 16) << "HWND" << hwnd;
}

bool WinContextMenu::handleNativeEvent(const QByteArray &eventType, void *message, long *result)
{
    Q_UNUSED(eventType)
    Q_UNUSED(result)

    MSG *msg = static_cast<MSG *>(message);
    if (msg->message == WM_INITMENUPOPUP || msg->message == WM_DRAWITEM || msg->message == WM_MENUCHAR || msg->message == WM_MEASUREITEM) {

        if (imenu3) {
            qDebug() << "imenu3";
            LRESULT lres;
            HRESULT hr;
            if (SUCCEEDED(hr = imenu3->HandleMenuMsg2(msg->message, msg->wParam, msg->lParam, &lres))) {
                return true;
            } else
                qDebug() << "imenu3 failed" << QString::number(hr, 16);

        } else if (imenu2) {
            qDebug() << "imenu2";
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
    bool lastSeparator {};
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

        if (!(info.fType & MFT_SEPARATOR)) {

            if (info.wID > SCRATCH_QCM_FIRST) {

                // Get language-independent verb
                imenu->GetCommandString(info.wID - SCRATCH_QCM_FIRST, GCS_VERBW, nullptr, cBuffer, 255);
                verb = QString::fromWCharArray(reinterpret_cast<wchar_t *>(cBuffer));

                if (viewAspect == ContextMenu::Background) {

                    // Delete default explorer entries for the background view
                    // We will add our own implementation later
                    if (verb == VERB_UNDO || verb == VERB_VIEW || verb == VERB_SORTBY || verb == VERB_GROUPBY || verb == VERB_VIEWCUSTOMWIZARD || verb == VERB_SHARE) {
                        verbsToDelete.append(i);
                        continue;
                    }

                } else {

                    // Delete UWP calls. They will get an INVALID_WINDOW_HANDLE (0x80070578) error anyway
                    if (verb == VERB_MODERNSHARE) {
                        verbsToDelete.append(i);
                        continue;
                    }
                }
            }

            lastSeparator = false;

        } else {

            // Delete separators that are consecutive or at the end
            if (i == (count - 1) || lastSeparator)
                verbsToDelete.append(i);

            lastSeparator = true;
        }

        qDebug() << info.wID << QString::fromWCharArray(info.dwTypeData) << "VERB: " << verb << "Is Separator?" << (info.fType & MFT_SEPARATOR);

        verb.clear();
    }

    // Delete verbs we marked that we do not want
    int offset = 0;
    for (UINT id : verbsToDelete)
        DeleteMenu(hmenu, (id - offset++), MF_BYPOSITION);

}
