#include "baseitemdelegate.h"
#include "basetreeview.h"

#include <QDebug>

BaseItemDelegate::BaseItemDelegate(QObject *parent) : QStyledItemDelegate(parent)
{

}

void BaseItemDelegate::initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const
{
    QStyledItemDelegate::initStyleOption(option, index);

    // Turn off that really ugly gray dotted border when an item has focus
    if (option->state & QStyle::State_HasFocus)
        option->state &= ~QStyle::State_HasFocus;

    // No mouse over for columns > 0
    if (index.column() > 0 && (option->state & QStyle::State_MouseOver))
        option->state &= ~QStyle::State_MouseOver;

    // No mouse over when dragging over items that are not droppable
    // Funny that widget is an undocumented member of the class QStyleOptionViewTime
    const BaseTreeView *treeView = static_cast<const BaseTreeView *>(option->widget);
    if (treeView->isDragging() && index.column() == 0 && !(index.flags() & Qt::ItemIsDropEnabled))
        option->state &= ~QStyle::State_MouseOver;
}
