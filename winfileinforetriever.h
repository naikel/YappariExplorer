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

    QIcon getIconFromPIDL(LPITEMIDLIST pidl, bool isHidden);

protected:
    void getChildrenBackground(FileSystemItem *parent) override;
    void getParentInfo(FileSystemItem *parent) override;

private:
    QPixmap getPixmapFromIndex(int index);
};

#endif // WINFILEINFORETRIEVER_H
