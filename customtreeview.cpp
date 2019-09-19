#include "customtreeview.h"

CustomTreeView::CustomTreeView(QWidget *parent) : QTreeView(parent)
{
    // Custom initialization

    setHeaderHidden(true);
    setSelectionBehavior(QAbstractItemView::SelectItems);
}
