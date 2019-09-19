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

#include "filesystemitem.h"

class WinFileInfoRetriever : public QObject
{
    Q_OBJECT

public:
    WinFileInfoRetriever(QObject *parent = nullptr);

    FileSystemItem *getRoot();
    void getChildren(FileSystemItem *fileSystemItem);
    QIcon getIconFromPath(QString path, bool isHidden);
    QIcon getIconFromPIDL(LPITEMIDLIST pidl, bool isHidden);

private:
    int getSystemImageListIndexFromPIDL(LPITEMIDLIST pidl);
    int getSystemImageListIndexFromPath(QString path);
    QIcon getIconFromIndex(int index, bool isHidden);
};

#endif // WINFILEINFORETRIEVER_H
