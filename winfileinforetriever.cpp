#include "winfileinforetriever.h"

#include <QThread>
#include <QPainter>
#include <QDebug>

#include "filesystemitem.h"

// Qt exports a non-documented function to convert a native Windows HICON to a QPixmap
// This function is found in Qt5Gui.dll
extern QPixmap qt_pixmapFromWinHICON(HICON icon);

WinFileInfoRetriever::WinFileInfoRetriever(QObject *parent) : FileInfoRetriever(parent)
{
}

void WinFileInfoRetriever::getChildrenBackground(FileSystemItem *parent)
{
    qDebug() << "getChildrenBackground()";

    LPITEMIDLIST pidl;

    HRESULT hr;
    hr = (parent->getPath() == "/")
                ? ::SHGetKnownFolderIDList(FOLDERID_ComputerFolder /* FOLDERID_Desktop */, KF_FLAG_DEFAULT, nullptr, &pidl)
                : ::SHParseDisplayName(parent->getPath().toStdWString().c_str(), nullptr, &pidl, 0, nullptr);


    // Get the PIDL for the parent
    if (SUCCEEDED(hr)) {

        // If this is the root we need to retrieve the display name of it and its icon
        if (parent->getPath() == "/") {
            qDebug() << "Getting name of root";
            PWSTR pwstrName;
            hr = ::SHGetNameFromIDList(pidl, SIGDN_NORMALDISPLAY, &pwstrName);
            QString rootName = QString::fromWCharArray(pwstrName);
            parent->setDisplayName(rootName);
            parent->setHasSubFolders(true);
            parent->setIcon(getIconFromPIDL(pidl, false));
            ::CoTaskMemFree(pwstrName);
        }

        IShellFolder *psf {};

        // Get the IShellFolder interface for it
        if (SUCCEEDED(::SHBindToObject(nullptr, pidl, nullptr, IID_IShellFolder, reinterpret_cast<void**>(&psf)))) {

            // Get the enumeration interface
            IEnumIDList *ppenumIDList {};

            // Proof of Concept: only folders
            //SHCONTF flags = SHCONTF_FOLDERS | SHCONTF_FASTITEMS | SHCONTF_NONFOLDERS | SHCONTF_ENABLE_ASYNC;
            SHCONTF flags = SHCONTF_FOLDERS | SHCONTF_FASTITEMS | SHCONTF_INCLUDEHIDDEN | SHCONTF_INCLUDESUPERHIDDEN; // | SHCONTF_ENABLE_ASYNC;

            if (SUCCEEDED(psf->EnumObjects(nullptr, flags, reinterpret_cast<IEnumIDList**>(&ppenumIDList)))) {

                LPITEMIDLIST pidlChildren;

                while (ppenumIDList->Next(1, &pidlChildren, nullptr) == S_OK) {

                    SFGAOF attributes {SFGAO_STREAM | SFGAO_FOLDER | SFGAO_HASSUBFOLDER | SFGAO_HIDDEN };
                    psf->GetAttributesOf(1, const_cast<LPCITEMIDLIST *>(&pidlChildren), &attributes);

                    // Compressed files will have SFGAO_FOLDER and SFGAO_STREAM attributes
                    // We want to skip those
                    if (!(attributes & SFGAO_STREAM)) {

                        STRRET strRet;

                        // Get the absolute path and create a FileSystemItem with it
                        psf->GetDisplayNameOf(pidlChildren, SHGDN_FORPARSING, &strRet);
                        qDebug() << "Path " << QString::fromWCharArray(strRet.pOleStr);
                        FileSystemItem *child = new FileSystemItem(QString::fromWCharArray(strRet.pOleStr));

                        // Get the display name
                        psf->GetDisplayNameOf(pidlChildren, SHGDN_NORMAL, &strRet);
                        child->setDisplayName(QString::fromWCharArray(strRet.pOleStr));

                        // Set basic attributes
                        child->setHasSubFolders(attributes & SFGAO_HASSUBFOLDER);

                        LPITEMIDLIST absolutePidl = ILCombine(pidl, pidlChildren);
                        child->setIcon(getIconFromPIDL(absolutePidl, (attributes & SFGAO_HIDDEN)));
                        ILFree(absolutePidl);

                        qDebug() << "New child " << child->getDisplayName() << " path " << child->getPath();

                        parent->addChild(child);
                    }

                    ILFree(pidlChildren);
                }

                ppenumIDList->Release();
            }
            psf->Release();
        }

        ILFree(pidl);

        parent->setAllChildrenFetched(true);
        emit parentUpdated(parent);
    }
}

QIcon WinFileInfoRetriever::getIconFromPath(QString path, bool isHidden)
{
    int index = getSystemImageListIndexFromPath(path);
    return getIconFromIndex(index, isHidden);
}

QIcon WinFileInfoRetriever::getIconFromPIDL(LPITEMIDLIST pidl, bool isHidden)
{
    int index = getSystemImageListIndexFromPIDL(pidl);
    return getIconFromIndex(index, isHidden);
}


QIcon WinFileInfoRetriever::getIconFromIndex(int index, bool isHidden)
{
    IImageList *spiml {};

    if (SUCCEEDED(::SHGetImageList(SHIL_SMALL, IID_IImageList2, reinterpret_cast<void**>(&spiml)))) {

        HICON hIcon {};
        spiml->GetIcon(index, ILD_TRANSPARENT | ILD_IMAGE, &hIcon);

        QPixmap pixmap = qt_pixmapFromWinHICON(hIcon);

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

        DestroyIcon(hIcon);
        return QIcon(pixmap);

    }

    return QIcon();
}

int WinFileInfoRetriever::getSystemImageListIndexFromPIDL(LPITEMIDLIST pidl)
{
    SHFILEINFOW sfi;
    if (SUCCEEDED(::SHGetFileInfoW(reinterpret_cast<LPCWSTR>(pidl), FILE_ATTRIBUTE_NORMAL, &sfi, sizeof(sfi), SHGFI_PIDL | SHGFI_SYSICONINDEX | SHGFI_SMALLICON | SHGFI_OPENICON))) {
        DestroyIcon(sfi.hIcon);
        return sfi.iIcon;
    }
    return -1;
}

int WinFileInfoRetriever::getSystemImageListIndexFromPath(QString path)
{
    SHFILEINFOW sfi;
    if (SUCCEEDED(::SHGetFileInfoW(path.toStdWString().c_str(), FILE_ATTRIBUTE_NORMAL, &sfi, sizeof(sfi), SHGFI_SYSICONINDEX | SHGFI_SMALLICON | SHGFI_OPENICON))) {
        DestroyIcon(sfi.hIcon);
        return sfi.iIcon;
    }
    return -1;
}


