#include <QApplication>
#include <QFileIconProvider>
#include <QPainter>
#include <QDebug>
#include <QTime>

#include "winfileinforetriever.h"
#include "Window/AppWindow.h"

#define COMPUTER_FOLDER_GUID          "::{20D04FE0-3AEA-1069-A2D8-08002B30309D}"
#define CONTROL_PANEL_GUID            "::{26EE0668-A00A-44D7-9371-BEB064C98683}"

// Known icon indexes from the Windows System List
#define DEFAULT_FILE_ICON_INDEX       0
#define DEFAULT_FOLDER_ICON_INDEX     3

#define GUID_SIZE                     38
#define BITBUCKET_VOLUME_KEY          "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\BitBucket\\Volume\\"

// Qt exports a non-documented function to convert a native Windows HICON to a QPixmap
// This function is found in Qt5Gui.dll
extern QPixmap qt_pixmapFromWinHICON(HICON icon);

WinFileInfoRetriever::WinFileInfoRetriever(QObject *parent) : FileInfoRetriever(parent)
{
}

QString WinFileInfoRetriever::getRootPath() const
{
    return COMPUTER_FOLDER_GUID;
}

bool WinFileInfoRetriever::getParentInfo(FileSystemItem *parent)
{
    LPITEMIDLIST pidl;
    HRESULT hr;
    qDebug() << "WinFileInfoRetriever::getParentInfo " << getScope() << " Parent path " << parent->getPath();

    hr = ::SHParseDisplayName(parent->getPath().toStdWString().c_str(), nullptr, &pidl, 0, nullptr);

    // Get the PIDL for the parent
    if (SUCCEEDED(hr)) {

        qDebug() << "WinFileInfoRetriever::getParentInfo " << getScope() << " Getting name of root in getParentInfo()";
        PWSTR pwstrName;
        hr = ::SHGetNameFromIDList(pidl, SIGDN_NORMALDISPLAY, &pwstrName);
        QString rootName = QString::fromWCharArray(pwstrName);
        qDebug() << "WinFileInfoRetriever::getParentInfo " << getScope() << " Root name is " << rootName;

        QTime start;
        start.start();
        parent->setDisplayName(rootName);
        qDebug() << "WinFileInfoRetriever::getParentInfo " << getScope() << " setDisplayName" << start.elapsed();
        QIcon icon = getIconFromPIDL(pidl, false, true);
        qDebug() << "WinFileInfoRetriever::getParentInfo " << getScope() << " getIconFromPIDL" << start.elapsed();
        parent->setIcon(icon);
        qDebug() << "WinFileInfoRetriever::getParentInfo " << getScope() << " setIcon" << start.elapsed();

        // The Windows root (My Computer) shouldn't have any capability
        if (parent->getPath() != getRootPath()) {
            SFGAOF attributes { SFGAO_CAPABILITYMASK | SFGAO_READONLY };
            IShellFolder *psf {};

            // Get the IShellFolder interface for the parent to get the attributes
            LPCITEMIDLIST child;
            if (SUCCEEDED(::SHBindToParent(pidl, IID_IShellFolder, reinterpret_cast<void**>(&psf), &child))) {
                psf->GetAttributesOf(1, &child, &attributes);
                setCapabilities(parent, attributes);
                psf->Release();
            }
        }

        ::CoTaskMemFree(pwstrName);
        ::ILFree(pidl);

        qDebug() << "WinFileInfoRetriever::getParentInfo " << getScope() << " CoTaskFreeMem" << start.elapsed();

        return true;
    } else {
        WCHAR buffer[256];
        FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, hr, 0, buffer, sizeof(buffer), nullptr);

        QString errMessage = QString::fromWCharArray(buffer);

        qDebug() << "WinFileInfoRetriever::getParentInfo " << getScope() << "Couldn't access" << parent->getPath() << "HRESULT" << hr << "(" << errMessage << ")";
        emit parentUpdated(parent, hr, errMessage);

        return false;
    }
}

void WinFileInfoRetriever::getChildrenBackground(FileSystemItem *parent)
{
    ::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    // Sometimes the last folder is deleted between getParentInfo and this function
    // We need to double check it
    bool subFolders         {};
    LPITEMIDLIST pidl;
    HRESULT hr;
    qDebug() << "WinFileInfoRetriever::getChildrenBackground " << getScope() << "Parent path " << parent->getPath();
    hr = ::SHParseDisplayName(parent->getPath().toStdWString().c_str(), nullptr, &pidl, 0, nullptr);

    // Get the PIDL for the parent
    if (SUCCEEDED(hr)) {

        IShellFolder *psf {};

        // Get the IShellFolder interface for it
        if (SUCCEEDED(::SHBindToObject(nullptr, pidl, nullptr, IID_IShellFolder, reinterpret_cast<void**>(&psf)))) {

            qDebug() << "WinFileInfoRetriever::getChildrenBackground " << getScope() << "Bound to object";

            // Get the enumeration interface
            IEnumIDList *ppenumIDList {};

            SHCONTF flags = SHCONTF_FOLDERS | SHCONTF_INCLUDEHIDDEN | SHCONTF_INCLUDESUPERHIDDEN; // | SHCONTF_ENABLE_ASYNC | SHCONTF_FASTITEMS;

            if (getScope() == FileInfoRetriever::List)
                flags |= SHCONTF_NONFOLDERS;

            if (SUCCEEDED(psf->EnumObjects(reinterpret_cast<HWND>(AppWindow::getWinId()), flags, reinterpret_cast<IEnumIDList**>(&ppenumIDList)))) {

                qDebug() << "WinFileInfoRetriever::getChildrenBackground " << getScope() << "Children enumerated";

                LPITEMIDLIST pidlChildren {};

                while (running.load() && ppenumIDList->Next(1, &pidlChildren, nullptr) == S_OK) {

                    // qDebug() << "WinFileInfoRetriever::getChildrenBackground " << getScope() << QTime::currentTime() << "Before getting attributes";
                    SFGAOF attributes { SFGAO_STREAM | SFGAO_FOLDER | SFGAO_HIDDEN | SFGAO_CAPABILITYMASK | SFGAO_READONLY };

                    psf->GetAttributesOf(1, const_cast<LPCITEMIDLIST *>(&pidlChildren), &attributes);
                    // qDebug() << "WinFileInfoRetriever::getChildrenBackground " << getScope() << QTime::currentTime() << "Got attributes";

                    // Compressed files will have SFGAO_FOLDER and SFGAO_STREAM attributes
                    // We want to skip those for the Tree scope
                    if (getScope() == FileInfoRetriever::List || !(attributes & SFGAO_STREAM)) {

                        STRRET strRet;

                        // Get the absolute path and create a FileSystemItem with it
                        psf->GetDisplayNameOf(pidlChildren, SHGDN_FORPARSING, &strRet);
                        QString displayName = QString::fromWCharArray(strRet.pOleStr);

                        if (displayName == CONTROL_PANEL_GUID)
                            continue;

                        FileSystemItem *child = new FileSystemItem(displayName);

                        // Set basic attributes
                        child->setFolder((attributes & SFGAO_FOLDER) && !(attributes & SFGAO_STREAM) && !child->isDrive());

                        // Get the display name
                        flags = (child->isFolder() || child->isDrive()) ? SHGDN_NORMAL : SHGDN_FORPARSING | SHGDN_INFOLDER;
                        psf->GetDisplayNameOf(pidlChildren, flags, &strRet);
                        child->setDisplayName(QString::fromWCharArray(strRet.pOleStr));

                        // qDebug() << "WinFileInfoRetriever::getChildrenBackground " << getScope() << child->getPath() << "isDrive" << child->isDrive();

                        if (child->isFolder()) {

                            // Check if it has subfolders only if it's a folder and it's the Tree scope
                            if (getScope() == FileInfoRetriever::Tree) {
                                SFGAOF subfolders { SFGAO_HASSUBFOLDER };
                                psf->GetAttributesOf(1, const_cast<LPCITEMIDLIST *>(&pidlChildren), &subfolders);
                                child->setHasSubFolders((getScope() == FileInfoRetriever::Tree) ? (subfolders & SFGAO_HASSUBFOLDER) : false);
                            }
                            if (!subFolders)
                                subFolders = true;

                            child->setType(QApplication::translate("QFileDialog", "File Folder", "Match Windows Explorer"));
                        } else
                            child->setHasSubFolders(child->isDrive() ? true : false);

                        child->setHidden(attributes & SFGAO_HIDDEN);

                        if (getScope() == FileInfoRetriever::List) {

                            // File Size - Method 3 - FASTEST
                            if (!child->isDrive()) {
                                WIN32_FIND_DATAW fileAttributeData;
                                if (SUCCEEDED(::SHGetDataFromIDListW(psf, pidlChildren, SHGDFIL_FINDDATA, &fileAttributeData, sizeof(fileAttributeData)))) {
                                    quint64 size = fileAttributeData.nFileSizeHigh;
                                    size = (size << 32) + fileAttributeData.nFileSizeLow;
                                    child->setSize(size);

                                    child->setCreationTime(fileTimeToQDateTime(&(fileAttributeData.ftCreationTime)));
                                    child->setLastChangeTime(fileTimeToQDateTime(&(fileAttributeData.ftLastWriteTime)));
                                    child->setLastAccessTime(fileTimeToQDateTime(&(fileAttributeData.ftLastAccessTime)));
                                }
                            }

                            // File Type
                            if (!child->isFolder()) {
                                SFGAOF noAttributes {};
                                UINT flags = SHGFI_USEFILEATTRIBUTES | SHGFI_TYPENAME;
                                SHFILEINFO info;
                                ::SHGetFileInfo(child->getPath().toStdWString().c_str(), noAttributes, &info, sizeof(SHFILEINFO), flags);
                                child->setType(QString::fromStdWString(info.szTypeName));
                            }
                        }

                        // Capabilites
                        setCapabilities(child, attributes);

                        parent->addChild(child);
                    }

                    ::ILFree(pidlChildren);
                }
                ppenumIDList->Release();
            }
            psf->Release();
        }

        ::ILFree(pidl);

        if (!running.load()) {
            qDebug() << "WinFileInfoRetriever::getChildrenBackground " << getScope() << "Parent path " << parent->getPath() << " aborted!";
            ::CoUninitialize();
            return;
        }

        parent->setHasSubFolders(subFolders);
        parent->setAllChildrenFetched(true);
        emit parentUpdated(parent, 0, QString());

        //if (getScope() == FileInfoRetriever::List)
        //     getExtendedInfo(parent);

        running.store(false);
    }

    ::CoUninitialize();
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
            if (!item->isFolder()) {
                /*
                if (!item->isDrive()) {
                    // File Size - Method 1 - VERY SLOW
                    WIN32_FILE_ATTRIBUTE_DATA fileAttributeData;
                    if (GetFileAttributesExW(item->getPath().toStdWString().c_str(), GetFileExInfoStandard, &fileAttributeData)) {
                        quint64 size = fileAttributeData.nFileSizeHigh;
                        size = (size << 32) + fileAttributeData.nFileSizeLow;
                        item->setSize(size);
                    }

                    // File Size - Method 2 - FASTER
                    WIN32_FIND_DATAW fileAttributeData;
                    HANDLE h = FindFirstFileW(item->getPath().toStdWString().c_str(), &fileAttributeData);
                    if (h != INVALID_HANDLE_VALUE) {
                        FindClose(h);
                        quint64 size = fileAttributeData.nFileSizeHigh;
                        size = (size << 32) + fileAttributeData.nFileSizeLow;
                        item->setSize(size);
                    }

                }
                */

                // File Type
                SHFILEINFO info;
                ::SHGetFileInfo(item->getPath().toStdWString().c_str(), attributes, &info, sizeof(SHFILEINFO), flags);
                strType = QString::fromStdWString(info.szTypeName);
                item->setType(strType);
                emit itemUpdated(item);
            }
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

void WinFileInfoRetriever::setDisplayNameOf(FileSystemItem *fileSystemItem)
{
    SFGAOF attributes {};
    UINT flags = SHGFI_USEFILEATTRIBUTES | SHGFI_TYPENAME | SHGFI_DISPLAYNAME;
    SHFILEINFOW info;
    if (SUCCEEDED(::SHGetFileInfoW(fileSystemItem->getPath().toStdWString().c_str(), attributes, &info, sizeof(SHFILEINFO), flags))) {
        fileSystemItem->setDisplayName(QString::fromStdWString(info.szDisplayName));

        QString type = QString::fromStdWString(info.szTypeName);
        if (type != fileSystemItem->getType()) {
            // We need to get a new icon since the file changed types
            fileSystemItem->setIcon(getIconFromPath(fileSystemItem->getPath(), fileSystemItem->isHidden(), true));
        }

        fileSystemItem->setType(fileSystemItem->isFolder() ? QApplication::translate("QFileDialog", "File Folder", "Match Windows Explorer") : QString::fromStdWString(info.szTypeName));
    }
}

bool WinFileInfoRetriever::refreshItem(FileSystemItem *fileSystemItem)
{
    WIN32_FIND_DATAW fileAttributeData;
    HANDLE h = FindFirstFileW(fileSystemItem->getPath().toStdWString().c_str(), &fileAttributeData);
    if (h != INVALID_HANDLE_VALUE) {
        FindClose(h);
        quint64 size = fileAttributeData.nFileSizeHigh;
        size = (size << 32) + fileAttributeData.nFileSizeLow;
        fileSystemItem->setSize(size);
        fileSystemItem->setCreationTime(fileTimeToQDateTime(&(fileAttributeData.ftCreationTime)));
        fileSystemItem->setLastChangeTime(fileTimeToQDateTime(&(fileAttributeData.ftLastWriteTime)));
        fileSystemItem->setLastAccessTime(fileTimeToQDateTime(&(fileAttributeData.ftLastAccessTime)));

        if (fileAttributeData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            fileSystemItem->setFolder(true);
        }

        fileSystemItem->setHidden(fileAttributeData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN);

        SFGAOF attributes {};
        ULONG flags = SFGAO_CAPABILITYMASK;
        if (getScope() == FileInfoRetriever::Tree && fileSystemItem->isFolder())
            flags |= SFGAO_HASSUBFOLDER;

        LPITEMIDLIST pidl;
        if (SUCCEEDED(SHParseDisplayName(fileSystemItem->getPath().toStdWString().c_str(), nullptr, &pidl, flags, &attributes))) {

            if (getScope() == FileInfoRetriever::Tree)
                fileSystemItem->setHasSubFolders(attributes & SFGAO_HASSUBFOLDER);

            setCapabilities(fileSystemItem, attributes);

            // Refresh icon
            fileSystemItem->setIcon(getIconFromPIDL(pidl, fileSystemItem->isHidden(), true));

            ::ILFree(pidl);
        }

        // File Type
        //if (!fileSystemItem->isFolder()) {
            SFGAOF noAttributes {};
            UINT sFlags = SHGFI_USEFILEATTRIBUTES | SHGFI_TYPENAME;
            SHFILEINFO info;
            ::SHGetFileInfo(fileSystemItem->getPath().toStdWString().c_str(), noAttributes, &info, sizeof(SHFILEINFO), sFlags);
            fileSystemItem->setType(QString::fromStdWString(info.szTypeName));
        //} else {
        //    fileSystemItem->setType(QApplication::translate("QFileDialog", "File Folder", "Match Windows Explorer"));
        //}

        emit itemUpdated(fileSystemItem);

        return true;

    } else {

        qDebug() << "WinFileInfoRetriever::refreshItem item" << fileSystemItem->getPath() << "seems that it doesn't exist anymore";
        return false;

    }
}

bool WinFileInfoRetriever::willRecycle(FileSystemItem *fileSystemItem)
{
    qDebug() << "WinFileInfoRetriever::willRecycle";

    // Get the drive
    if (fileSystemItem->getPath().length() < 3)
        return false;

    QString drive = fileSystemItem->getPath().left(3);
    if (!drive.at(0).isLetter() || drive.at(1) != ':' || drive.at(2) != '\\')
        return false;

    // Get the volume GUID
    WCHAR buffer[MAX_PATH];
    GetVolumeNameForVolumeMountPointW(drive.toStdWString().c_str(), buffer, MAX_PATH);

    QString volumeName = QString::fromWCharArray(buffer);
    qDebug() << "WinFileInfoRetriever::willRecycle path" << fileSystemItem->getPath() << "volumeName" << volumeName;

    // If volumeName is empty this means it is not a fixed drive
    if (volumeName.isEmpty())
        return false;

    int index = volumeName.indexOf('{');
    if (index  < 0)
        return false;

    QString guid = volumeName.mid(index, GUID_SIZE);

    QString subKey = BITBUCKET_VOLUME_KEY + guid;
    qDebug() << "WinFileInfoRetriever::willRecycle subkey" << subKey;

    // Check if the Recycle Bin is enabled for this drive
    HKEY phkResult;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, subKey.toStdWString().c_str(), 0, KEY_QUERY_VALUE, &phkResult) == ERROR_SUCCESS) {

        DWORD nukeOnDelete {};
        ULONG dwSize = sizeof(nukeOnDelete);
        RegQueryValueExW(phkResult, TEXT("NukeOnDelete"), nullptr, nullptr, (LPBYTE)&nukeOnDelete, &dwSize);
        RegCloseKey(phkResult);

        if (!nukeOnDelete)
            return true;
    }

    return false;
}

QIcon WinFileInfoRetriever::getIcon(FileSystemItem *item) const
{
    ::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    // qDebug() << "WinFileInfoRetriever::getIcon " << getScope() << item->getPath();
    LPITEMIDLIST pidl;
    HRESULT hr;
    if (SUCCEEDED(hr = ::SHParseDisplayName(item->getPath().toStdWString().c_str(), nullptr, &pidl, 0, nullptr))) {
        QIcon icon = getIconFromPIDL(pidl, item->isHidden());
        ::ILFree(pidl);
        ::CoUninitialize();
        return icon;
    }
    else
        qDebug() << "WinFileInfoRetriever::getIcon failed " << QString::number(hr, 16) << getScope() << item->getPath();

    ::CoUninitialize();
    return QIcon();
}

QIcon WinFileInfoRetriever::getIconFromPath(QString path, bool isHidden, bool ignoreDefault) const
{
    SHFILEINFOW sfi = getSystemImageListIndexFromPath(path);
    return getIconFromFileInfo(sfi, isHidden, ignoreDefault);
}

QIcon WinFileInfoRetriever::getIconFromPIDL(LPITEMIDLIST pidl, bool isHidden, bool ignoreDefault) const
{
    SHFILEINFOW sfi = getSystemImageListIndexFromPIDL(pidl);
    return getIconFromFileInfo(sfi, isHidden, ignoreDefault);
}

/*!
 * \brief Returns a QIcon with the icon stored in a SHFILEINFOW from an specific cile
 * \param sfi a SHFILEINFOW structure with an index or a handle to the icon
 * \param isHidden true if the file has the hidden attribute
 * \param ignoreDefault true if this function should return a valid QIcon even if it's a default one
 * \return a QIcon with the file icon
 *
 * This function returns a QIcon constructed from a Windows HICON or from an index of the Windows System Image List.
 * If the file has the hidden attribute the icon will be alpha blended to look ghosted.
 * By default this function will return an null QIcon if the icon is the default Windows folder icon or the default
 * file icon, unless ignoreDefault is true; in that case the QIcon is returned anyway.
 */
QIcon WinFileInfoRetriever::getIconFromFileInfo(SHFILEINFOW sfi, bool isHidden, bool ignoreDefault) const
{
    if (sfi.iIcon > -1) {

        // Just a bit of optimization: don't update default icons if the caller doesn't want it. They are already there
        if (!ignoreDefault && !isHidden && (sfi.iIcon == DEFAULT_FOLDER_ICON_INDEX || sfi.iIcon == DEFAULT_FILE_ICON_INDEX)) {
            ::DestroyIcon(sfi.hIcon);
            return QIcon();
        }

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
        ::DestroyIcon(sfi.hIcon);
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
            ::DestroyIcon(hIcon);
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

QDateTime WinFileInfoRetriever::fileTimeToQDateTime(LPFILETIME fileTime)
{
    SYSTEMTIME systemTime;
    ::FileTimeToSystemTime(fileTime, &systemTime);
    QDate date = QDate(systemTime.wYear, systemTime.wMonth, systemTime.wDay);
    QTime time = QTime(systemTime.wHour, systemTime.wMinute, systemTime.wSecond, systemTime.wMilliseconds);
    return(QDateTime(date, time, Qt::UTC));
}

void WinFileInfoRetriever::setCapabilities(FileSystemItem *item, const SFGAOF &attributes)
{
    quint16 capabilities {};
    if (attributes & SFGAO_CANCOPY)
        capabilities |= FSI_CAN_COPY;
    if (attributes & SFGAO_CANMOVE)
        capabilities |= FSI_CAN_MOVE;
    if (attributes & SFGAO_CANLINK)
        capabilities |= FSI_CAN_LINK;
    if (attributes & SFGAO_CANRENAME)
        capabilities |= FSI_CAN_RENAME;
    if (attributes & SFGAO_CANDELETE)
        capabilities |= FSI_CAN_DELETE;
    if (!(attributes & SFGAO_READONLY))
        capabilities |= FSI_DROP_TARGET;

    item->setCapabilities(capabilities);
}

