#include "baseitemdelegate.h"

BaseItemDelegate::BaseItemDelegate(QObject *parent) : QStyledItemDelegate(parent)
{

}

void BaseItemDelegate::initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const
{
    QStyledItemDelegate::initStyleOption(option, index);

    // Turn off that really ugly gray dotted border when an item has focus
    if(option->state & QStyle::State_HasFocus)
        option->state = option->state & ~QStyle::State_HasFocus;

    // No mouse over for columns > 0
    if(index.column() > 0 && (option->state & QStyle::State_MouseOver))
        option->state = option->state & ~QStyle::State_MouseOver;
}
