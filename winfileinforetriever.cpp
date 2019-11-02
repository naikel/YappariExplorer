#include <QApplication>
#include <QPainter>
#include <QDebug>
#include <QTime>

#include "winfileinforetriever.h"
#include "filesystemitem.h"

#define CONTROL_PANEL_GUID "::{26EE0668-A00A-44D7-9371-BEB064C98683}"

// Qt exports a non-documented function to convert a native Windows HICON to a QPixmap
// This function is found in Qt5Gui.dll
extern QPixmap qt_pixmapFromWinHICON(HICON icon);

WinFileInfoRetriever::WinFileInfoRetriever(QObject *parent) : FileInfoRetriever(parent)
{
}

void WinFileInfoRetriever::getParentInfo(FileSystemItem *parent)
{
    LPITEMIDLIST pidl;
    HRESULT hr;
    qDebug() << "WinFileInfoRetriever::getParentInfo " << getScope() << " Parent path " << parent->getPath();
    hr = (parent->getPath() == "/")
                ? ::SHGetKnownFolderIDList(FOLDERID_ComputerFolder /* FOLDERID_Desktop */, KF_FLAG_DEFAULT, nullptr, &pidl)
                : ::SHParseDisplayName(parent->getPath().toStdWString().c_str(), nullptr, &pidl, 0, nullptr);

    // Get the PIDL for the parent
    if (SUCCEEDED(hr)) {

        qDebug() << "WinFileInfoRetriever::getParentInfo " << getScope() << " Getting name of root in getParentInfo()";
        PWSTR pwstrName;
        hr = ::SHGetNameFromIDList(pidl, SIGDN_NORMALDISPLAY, &pwstrName);
        QString rootName = QString::fromWCharArray(pwstrName);
        qDebug() << "WinFileInfoRetriever::getParentInfo " << getScope() << " Root name is " << rootName;
        parent->setDisplayName(rootName);
        parent->setIcon(getIconFromPIDL(pidl, false));
        ::CoTaskMemFree(pwstrName);
    }
}

void WinFileInfoRetriever::getChildrenBackground(FileSystemItem *parent)
{
    ::CoInitializeEx(nullptr, COINIT_MULTITHREADED);

    LPITEMIDLIST pidl;
    HRESULT hr;
    qDebug() << "WinFileInfoRetriever::getChildrenBackground " << getScope() << "Parent path " << parent->getPath();
    hr = (parent->getPath() == "/")
                ? ::SHGetKnownFolderIDList(FOLDERID_ComputerFolder /* FOLDERID_Desktop */, KF_FLAG_DEFAULT, nullptr, &pidl)
                : ::SHParseDisplayName(parent->getPath().toStdWString().c_str(), nullptr, &pidl, 0, nullptr);

    // Get the PIDL for the parent
    if (SUCCEEDED(hr)) {

        IShellFolder *psf {};

        // Get the IShellFolder interface for it
        if (SUCCEEDED(::SHBindToObject(nullptr, pidl, nullptr, IID_IShellFolder, reinterpret_cast<void**>(&psf)))) {

            qDebug() << "WinFileInfoRetriever::getChildrenBackground " << getScope() << "Bound to object";

            // Get the enumeration interface
            IEnumIDList *ppenumIDList {};

            SHCONTF flags = SHCONTF_FOLDERS | SHCONTF_FASTITEMS | SHCONTF_INCLUDEHIDDEN | SHCONTF_INCLUDESUPERHIDDEN | SHCONTF_ENABLE_ASYNC;

            if (getScope() == FileInfoRetriever::List)
                flags |= SHCONTF_NONFOLDERS;

            if (SUCCEEDED(psf->EnumObjects(nullptr, flags, reinterpret_cast<IEnumIDList**>(&ppenumIDList)))) {

                qDebug() << "WinFileInfoRetriever::getChildrenBackground " << getScope() << "Children enumerated";

                LPITEMIDLIST pidlChildren;

                while (running.load() && ppenumIDList->Next(1, &pidlChildren, nullptr) == S_OK) {

                    // qDebug() << "WinFileInfoRetriever::getChildrenBackground " << getScope() << QTime::currentTime() << "Before getting attributes";
                    SFGAOF attributes { SFGAO_STREAM | SFGAO_FOLDER | SFGAO_HASSUBFOLDER | SFGAO_HIDDEN };
                    psf->GetAttributesOf(1, const_cast<LPCITEMIDLIST *>(&pidlChildren), &attributes);
                    // qDebug() << "WinFileInfoRetriever::getChildrenBackground " << getScope() << QTime::currentTime() << "Got attributes";

                    // Compressed files will have SFGAO_FOLDER and SFGAO_STREAM attributes
                    // We want to skip those
                    if (getScope() == FileInfoRetriever::List || !(attributes & SFGAO_STREAM)) {

                        STRRET strRet;

                        // Get the absolute path and create a FileSystemItem with it
                        psf->GetDisplayNameOf(pidlChildren, SHGDN_FORPARSING, &strRet);
                        QString displayName = QString::fromWCharArray(strRet.pOleStr);

                        if (displayName == CONTROL_PANEL_GUID)
                            continue;

                        FileSystemItem *child = new FileSystemItem(displayName);

                        // Get the display name
                        psf->GetDisplayNameOf(pidlChildren, SHGDN_NORMAL, &strRet);
                        child->setDisplayName(QString::fromWCharArray(strRet.pOleStr));
                        qDebug() << "WinFileInfoRetriever::getChildrenBackground " << getScope() << child->getPath() << "isDrive" << child->isDrive();

                        // Set basic attributes
                        child->setFolder((attributes & SFGAO_FOLDER) && !(attributes & SFGAO_STREAM) && !child->isDrive());
                        child->setHasSubFolders((getScope() == FileInfoRetriever::Tree) ? (attributes & SFGAO_HASSUBFOLDER) : false);
                        child->setHidden(attributes & SFGAO_HIDDEN);
                        if (child->isFolder())
                            child->setType(QApplication::translate("QFileDialog", "File Folder", "Match Windows Explorer"));

                        // Set extended attributes if the scope is a detailed list
                        /*
                        if (getScope() == FileInfoRetriever::List) {
                            QString strType;
                            if (child->isFolder()) {
                                strType = QApplication::translate("QFileDialog", "File Folder", "Match Windows Explorer");
                            } else {
                                if (!child->isDrive()) {
                                    // File Size
                                    // Method 2 - VERY FAST
                                    WIN32_FIND_DATAW fileAttributeData;
                                    HANDLE h = FindFirstFileW(child->getPath().toStdWString().c_str(), &fileAttributeData);
                                    if (h != INVALID_HANDLE_VALUE) {
                                        FindClose(h);
                                        qint64 size = fileAttributeData.nFileSizeHigh;
                                        size = (size << 32) + fileAttributeData.nFileSizeLow;
                                        child->setSize(size);
                                    }
                                }

                                // File Type
                                SHFILEINFO info;
                                ::SHGetFileInfo(child->getPath().toStdWString().c_str(), attributes, &info, sizeof(SHFILEINFO), flags);
                                strType = QString::fromStdWString(info.szTypeName);
                            }
                            child->setType(strType);
                        }
                        */
                        parent->addChild(child);
                    }

                    ILFree(pidlChildren);
                }

                ppenumIDList->Release();
            }
            psf->Release();
        }

        ILFree(pidl);

        if (!running.load()) {
            qDebug() << "WinFileInfoRetriever::getChildrenBackground " << getScope() << "Parent path " << parent->getPath() << " aborted!";
            return;
        }

        parent->setHasSubFolders(true);
        parent->setAllChildrenFetched(true);
        emit parentUpdated(parent);

        if (getScope() == FileInfoRetriever::List)
             getExtendedInfo(parent);

        running.store(false);
    }
}

void WinFileInfoRetriever::getExtendedInfo(FileSystemItem *parent)
{
    qDebug() << "WinFileInfoRetriever::getExtendedInfo " << getScope() << " Parent path " << parent->getPath();

    QTime start;
    start.start();

    if (parent != nullptr && parent->areAllChildrenFetched()) {

        SFGAOF attributes {};
        UINT flags = SHGFI_USEFILEATTRIBUTES | SHGFI_TYPENAME;

        for (auto item : parent->getChildren()) {

            if (!running.load())
                break;

            QString strType;
            if (item->isFolder()) {
                strType = QApplication::translate("QFileDialog", "File Folder", "Match Windows Explorer");
            } else {

                if (!item->isDrive()) {
                    // File Size
                    /*
                    // Method 1 - VERY SLOW
                    WIN32_FILE_ATTRIBUTE_DATA fileAttributeData;
                    if (GetFileAttributesExW(item->getPath().toStdWString().c_str(), GetFileExInfoStandard, &fileAttributeData)) {
                        qint64 size = fileAttributeData.nFileSizeHigh;
                        size = (size << 32) + fileAttributeData.nFileSizeLow;
                        item->setSize(size);
                    }
                    */
                    // Method 2 - VERY FAST
                    WIN32_FIND_DATAW fileAttributeData;
                    HANDLE h = FindFirstFileW(item->getPath().toStdWString().c_str(), &fileAttributeData);
                    if (h != INVALID_HANDLE_VALUE) {
                        FindClose(h);
                        qint64 size = fileAttributeData.nFileSizeHigh;
                        size = (size << 32) + fileAttributeData.nFileSizeLow;
                        item->setSize(size);
                    }
                }

                // File Type
                SHFILEINFO info;
                ::SHGetFileInfo(item->getPath().toStdWString().c_str(), attributes, &info, sizeof(SHFILEINFO), flags);
                strType = QString::fromStdWString(info.szTypeName);
            }
            item->setType(strType);
            emit itemUpdated(item);
        }

        if (!running.load()) {
            qDebug() << "WinFileInfoRetriever::getExtendedInfo " << getScope() << "Parent path " << parent->getPath() << " aborted!";
            return;
        }

        // We need to signal the model that all the children need to be re-sorted
        emit (extendedInfoUpdated(parent));
    }

    qDebug() << "WinFileInfoRetriever::getExtendedInfo Finished in" << start.elapsed() << "milliseconds";
}

QIcon WinFileInfoRetriever::getIcon(FileSystemItem *item) const
{
    qDebug() << "WinFileInfoRetriever::getIcon " << getScope() << item->getPath();
    LPITEMIDLIST pidl;
    if (SUCCEEDED(::SHParseDisplayName(item->getPath().toStdWString().c_str(), nullptr, &pidl, 0, nullptr))) {
        QIcon icon = getIconFromPIDL(pidl, item->isHidden());
        ILFree(pidl);
        return icon;
    }
    return QIcon();
}

QIcon WinFileInfoRetriever::getIconFromPath(QString path, bool isHidden) const
{
    SHFILEINFOW sfi = getSystemImageListIndexFromPath(path);
    return getIconFromFileInfo(sfi, isHidden);
}

QIcon WinFileInfoRetriever::getIconFromPIDL(LPITEMIDLIST pidl, bool isHidden) const
{
    SHFILEINFOW sfi = getSystemImageListIndexFromPIDL(pidl);
    return getIconFromFileInfo(sfi, isHidden);
}

QIcon WinFileInfoRetriever::getIconFromFileInfo(SHFILEINFOW sfi, bool isHidden) const
{
    if (sfi.iIcon > -1) {
        QPixmap pixmap = getPixmapFromIndex(sfi.iIcon);
        if (pixmap.isNull())
            pixmap = qt_pixmapFromWinHICON(sfi.hIcon);

        if (isHidden) {

            // The ghosted icon for hidden files is not provided by the OS
            // All implementations I have seen use the Windows native AlphaBlend() function.
            // Here's an alpha blend implementation using Qt objects only

            QImage image = pixmap.toImage();
            QPainter paint(&pixmap);
            paint.setCompositionMode(QPainter::CompositionMode_Clear);
            paint.setOpacity(0.5);
            paint.drawImage(0, 0, image);
        }
        DestroyIcon(sfi.hIcon);
        return QIcon(pixmap);
    }
    return QIcon();
}

QPixmap WinFileInfoRetriever::getPixmapFromIndex(int index) const
{
    IImageList *spiml {};

    if (SUCCEEDED(::SHGetImageList(SHIL_SMALL, IID_IImageList2, reinterpret_cast<void**>(&spiml)))) {

        HICON hIcon {};

        if (SUCCEEDED(spiml->GetIcon(index, ILD_TRANSPARENT | ILD_IMAGE, &hIcon))) {
            QPixmap pixmap = qt_pixmapFromWinHICON(hIcon);
            DestroyIcon(hIcon);
            return pixmap;
        }
    }
    return QPixmap();
}

SHFILEINFOW WinFileInfoRetriever::getSystemImageListIndexFromPIDL(LPITEMIDLIST pidl) const
{
    SHFILEINFOW sfi;
    if (SUCCEEDED(::SHGetFileInfoW(reinterpret_cast<LPCWSTR>(pidl), FILE_ATTRIBUTE_NORMAL, &sfi, sizeof(sfi), SHGFI_PIDL | SHGFI_SYSICONINDEX | SHGFI_SMALLICON | SHGFI_ICON | SHGFI_OVERLAYINDEX))) {
        return sfi;
    }
    sfi.iIcon = -1;
    return sfi;
}

SHFILEINFOW WinFileInfoRetriever::getSystemImageListIndexFromPath(QString path) const
{
    SHFILEINFOW sfi;
    if (SUCCEEDED(::SHGetFileInfoW(path.toStdWString().c_str(), FILE_ATTRIBUTE_NORMAL, &sfi, sizeof(sfi), SHGFI_SYSICONINDEX | SHGFI_SMALLICON | SHGFI_ICON | SHGFI_OVERLAYINDEX))) {
        return sfi;
    }
    sfi.iIcon = -1;
    return sfi;
}

