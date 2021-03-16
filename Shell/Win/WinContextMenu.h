#ifndef WINCONTEXTMENU_H
#define WINCONTEXTMENU_H

#include <qt_windows.h>
#include <shlobj.h>

#include "Shell/ContextMenu.h"

class WinContextMenu : public ContextMenu
{
public:
    WinContextMenu(QObject *parent = nullptr);

    void show(const WId wId, const QPoint &pos, const QModelIndexList &indexList,
              const ContextMenu::ContextViewAspect viewAspect, QAbstractItemView *view) override;
    bool handleNativeEvent(const QByteArray &eventType, void *message, long *result) override;
    void defaultActionForIndex(const WId wId, const QModelIndex &index) override;

private:
    IContextMenu2 *imenu2   {};
    IContextMenu3 *imenu3   {};

    void customizeMenu(IContextMenu *imenu, const HMENU hmenu, const ContextMenu::ContextViewAspect viewAspect, const QModelIndex &parent);
    void invokeCommand(HWND hwnd, UINT iCmd, IContextMenu *imenu, QPoint pos, const QModelIndex &parent, FileSystemModel *model);
    void paste(const QModelIndex &parent, FileSystemModel *model, bool createLink = false);
};

#endif // WINCONTEXTMENU_H
