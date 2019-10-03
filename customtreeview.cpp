#include <QApplication>
#include <QDebug>

#include "customtreeview.h"

#include "filesystemmodel.h"

CustomTreeView::CustomTreeView(QWidget *parent) : QTreeView(parent)
{
    // Custom initialization
    setHeaderHidden(true);
    setSelectionBehavior(QAbstractItemView::SelectItems);

    // Animations don't really work when rows are inserted dynamically
    setAnimated(false);
}

void CustomTreeView::setModel(QAbstractItemModel *model)
{
    FileSystemModel *fileSystemModel = reinterpret_cast<FileSystemModel *>(model);

    connect(fileSystemModel, SIGNAL(fetchStarted()), this, SLOT(setBusyCursor()));
    connect(fileSystemModel, SIGNAL(fetchFinished()), this, SLOT(setNormalCursor()));

    QTreeView::setModel(model);
}

void CustomTreeView::setNormalCursor()
{
    setCursor(Qt::ArrowCursor);
}

void CustomTreeView::setBusyCursor()
{
    setCursor(Qt::BusyCursor);
}
