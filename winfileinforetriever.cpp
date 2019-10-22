#include <QApplication>
#include <QThread>
#include <QPainter>
#include <QDebug>
#include <QTimer>

#include "winfileinforetriever.h"
#include "filesystemitem.h"

// Qt exports a non-documented function to convert a native Windows HICON to a QPixmap
// This function is found in Qt5Gui.dll
extern QPixmap qt_pixmapFromWinHICON(HICON icon);

WinFileInfoRetriever::WinFileInfoRetriever(QObject *parent) : FileInfoRetriever(parent)
{
}

void WinFileInfoRetriever::getParentInfo(FileSystemItem *parent)
{
    qDebug() << "WinFileInfoRetriever::getParentInfo " << getScope();

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
    qDebug() << "WinFileInfoRetriever::getChildrenBackground " << getScope();

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

            // Get the enumeration interface
            IEnumIDList *ppenumIDList {};

            // Proof of Concept: only folders
            //SHCONTF flags = SHCONTF_FOLDERS | SHCONTF_FASTITEMS | SHCONTF_NONFOLDERS | SHCONTF_ENABLE_ASYNC;
            SHCONTF flags = SHCONTF_FOLDERS | SHCONTF_FASTITEMS | SHCONTF_INCLUDEHIDDEN | SHCONTF_INCLUDESUPERHIDDEN; // | SHCONTF_ENABLE_ASYNC;

            if (getScope() == FileInfoRetriever::List)
                flags |= SHCONTF_NONFOLDERS;

            if (SUCCEEDED(psf->EnumObjects(nullptr, flags, reinterpret_cast<IEnumIDList**>(&ppenumIDList)))) {

                LPITEMIDLIST pidlChildren;

                while (running.load() && ppenumIDList->Next(1, &pidlChildren, nullptr) == S_OK) {

                    SFGAOF attributes { SFGAO_STREAM | SFGAO_FOLDER | SFGAO_HASSUBFOLDER | SFGAO_HIDDEN };
                    psf->GetAttributesOf(1, const_cast<LPCITEMIDLIST *>(&pidlChildren), &attributes);

                    // Compressed files will have SFGAO_FOLDER and SFGAO_STREAM attributes
                    // We want to skip those
                    if (getScope() == FileInfoRetriever::List || !(attributes & SFGAO_STREAM)) {

                        STRRET strRet;

                        // Get the absolute path and create a FileSystemItem with it
                        psf->GetDisplayNameOf(pidlChildren, SHGDN_FORPARSING, &strRet);
                        FileSystemItem *child = new FileSystemItem(QString::fromWCharArray(strRet.pOleStr));

                        // Get the display name
                        psf->GetDisplayNameOf(pidlChildren, SHGDN_NORMAL, &strRet);
                        child->setDisplayName(QString::fromWCharArray(strRet.pOleStr));
                        qDebug() << "WinFileInfoRetriever::getChildrenBackground " << getScope() << "Path " << child->getPath() << " isDrive " << child->isDrive();

                        // Set basic attributes
                        child->setFolder((attributes & SFGAO_FOLDER) && !(attributes & SFGAO_STREAM) && !child->isDrive());
                        child->setHasSubFolders((getScope() == FileInfoRetriever::Tree) ? (attributes & SFGAO_HASSUBFOLDER) : false);

                        LPITEMIDLIST absolutePidl = ILCombine(pidl, pidlChildren);
                        child->setIcon(getIconFromPIDL(absolutePidl, (attributes & SFGAO_HIDDEN)));



                        ILFree(absolutePidl);

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

        parent->sortChildren(Qt::AscendingOrder);
        parent->setHasSubFolders(true);
        parent->setAllChildrenFetched(true);
        emit parentUpdated(parent);
    }
}

QIcon WinFileInfoRetriever::getIconFromPIDL(LPITEMIDLIST pidl, bool isHidden)
{
    SHFILEINFOW sfi;
    if (SUCCEEDED(::SHGetFileInfoW(reinterpret_cast<LPCWSTR>(pidl), FILE_ATTRIBUTE_NORMAL, &sfi, sizeof(sfi),
                                   SHGFI_PIDL | SHGFI_SYSICONINDEX | SHGFI_SMALLICON | SHGFI_ICON | SHGFI_OVERLAYINDEX))) {

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

QPixmap WinFileInfoRetriever::getPixmapFromIndex(int index)
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
