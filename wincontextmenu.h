#ifndef WINCONTEXTMENU_H
#define WINCONTEXTMENU_H

#include <qt_windows.h>
#include <shlobj.h>

#include "contextmenu.h"

class WinContextMenu : public ContextMenu
{
public:
    WinContextMenu(QObject *parent = nullptr);

    void show(const WId wId, const QPoint &pos, const QList<FileSystemItem *> fileSystemItems, const ContextMenu::ContextViewAspect viewAspect) override;
    bool handleNativeEvent(const QByteArray &eventType, void *message, long *result) override;

private:
    IContextMenu2 *imenu2;
    IContextMenu3 *imenu3;
};

#endif // WINCONTEXTMENU_H
