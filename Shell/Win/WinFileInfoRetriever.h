#ifndef WINFILEINFORETRIEVER_H
#define WINFILEINFORETRIEVER_H

#include <QObject>

#include <qt_windows.h>
#include <initguid.h>
#include <shlobj.h>
#include <shobjidl.h>
#include <winnt.h>
#include <knownfolders.h>
#include <commoncontrols.h>

#include "Shell/FileInfoRetriever.h"
#include "Shell/FileSystemItem.h"

class WinFileInfoRetriever : public FileInfoRetriever
{
public:
    WinFileInfoRetriever(QObject *parent = nullptr);

    QString getRootPath() const override;
    QIcon getIcon(FileSystemItem *item) const override;
    void setDisplayNameOf(FileSystemItem *fileSystemItem) override;
    bool refreshItem(FileSystemItem *fileSystemItem) override;

    bool willRecycle(FileSystemItem *fileSystemItem) override;

protected:
    void getChildrenBackground(FileSystemItem *parent) override;
    bool getParentInfo(FileSystemItem *parent) override;
    void getExtendedInfo(FileSystemItem *parent) override;

private:
    void getChildInfo(IShellFolder *psf, LPITEMIDLIST pidlChild, FileSystemItem *child);
    QIcon getIconFromFileInfo(SHFILEINFOW sfi, bool isHidden, bool ignoreDefault = false) const;
    QIcon getIconFromPath(QString path, bool isHidden, bool ignoreDefault = false) const;
    QIcon getIconFromPIDL(LPITEMIDLIST pidl, bool isHidden, bool ignoreDefault = false) const;
    QPixmap getPixmapFromIndex(int index) const;
    SHFILEINFOW getSystemImageListIndexFromPIDL(LPITEMIDLIST pidl) const;
    SHFILEINFOW getSystemImageListIndexFromPath(QString path) const;
    QDateTime fileTimeToQDateTime(LPFILETIME fileTime);
    void setCapabilities(FileSystemItem *item, const SFGAOF& attributes);
};

#endif // WINFILEINFORETRIEVER_H
