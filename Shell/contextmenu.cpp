#include "contextmenu.h"

ContextMenu::ContextMenu(QObject *parent) : QObject(parent)
{

}

void ContextMenu::show(const WId wId, const QPoint &pos, const QList<FileSystemItem *> fileSystemItems, const ContextMenu::ContextViewAspect viewAspect)
{
    Q_UNUSED(wId)
    Q_UNUSED(pos)
    Q_UNUSED(fileSystemItems)
    Q_UNUSED(viewAspect)
}

bool ContextMenu::handleNativeEvent(const QByteArray &eventType, void *message, long *result)
{
    Q_UNUSED(eventType)
    Q_UNUSED(message)
    Q_UNUSED(result)

    return false;
}
