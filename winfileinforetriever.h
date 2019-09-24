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

    QIcon getIconFromPath(QString path, bool isHidden);
    QIcon getIconFromPIDL(LPITEMIDLIST pidl, bool isHidden);

protected:
    void getChildrenBackground(FileSystemItem *fileSystemItem) override;

private:
    int getSystemImageListIndexFromPIDL(LPITEMIDLIST pidl);
    int getSystemImageListIndexFromPath(QString path);
    QIcon getIconFromIndex(int index, bool isHidden);
};

#endif // WINFILEINFORETRIEVER_H
