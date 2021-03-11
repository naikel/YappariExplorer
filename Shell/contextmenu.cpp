#include "contextmenu.h"

ContextMenu::ContextMenu(QObject *parent) : QObject(parent)
{

}

void ContextMenu::show(const WId wId, const QPoint &pos, const QModelIndexList &indexList,
                       const ContextMenu::ContextViewAspect viewAspect, QAbstractItemView *view)
{
    Q_UNUSED(wId)
    Q_UNUSED(pos)
    Q_UNUSED(indexList)
    Q_UNUSED(viewAspect)
    Q_UNUSED(view)
}

void ContextMenu::defaultActionForIndex(const WId wId, const QModelIndex &index)
{
    Q_UNUSED(wId)
    Q_UNUSED(index)
}

bool ContextMenu::handleNativeEvent(const QByteArray &eventType, void *message, long *result)
{
    Q_UNUSED(eventType)
    Q_UNUSED(message)
    Q_UNUSED(result)

    return false;
}
