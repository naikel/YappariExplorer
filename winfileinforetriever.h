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

#include "fileinforetriever.h"
#include "filesystemitem.h"

class WinFileInfoRetriever : public FileInfoRetriever
{
public:
    WinFileInfoRetriever(QObject *parent = nullptr);
    QIcon getIcon(FileSystemItem *item) const override;

protected:
    void getChildrenBackground(FileSystemItem *parent) override;
    void getParentInfo(FileSystemItem *parent) override;
    void getExtendedInfo(FileSystemItem *parent) override;

private:
    QIcon getIconFromFileInfo(SHFILEINFOW sfi, bool isHidden) const;
    QIcon getIconFromPath(QString path, bool isHidden) const;
    QIcon getIconFromPIDL(LPITEMIDLIST pidl, bool isHidden) const;
    QPixmap getPixmapFromIndex(int index) const;
    SHFILEINFOW getSystemImageListIndexFromPIDL(LPITEMIDLIST pidl) const;
    SHFILEINFOW getSystemImageListIndexFromPath(QString path) const;
};

#endif // WINFILEINFORETRIEVER_H
