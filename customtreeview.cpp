#include <QApplication>
#include <QDebug>

#include "once.h"
#include "customtreeview.h"
#include "filesystemmodel.h"

CustomTreeView::CustomTreeView(QWidget *parent) : QTreeView(parent)
{
    // Custom initialization
    setHeaderHidden(true);
    setSelectionBehavior(QAbstractItemView::SelectItems);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setFrameShape(QFrame::NoFrame);

    // Animations don't really work when rows are inserted dynamically
    setAnimated(false);
}

void CustomTreeView::setModel(QAbstractItemModel *model)
{
    FileSystemModel *fileSystemModel = reinterpret_cast<FileSystemModel *>(model);
    Once::connect(fileSystemModel, &FileSystemModel::fetchFinished, this, &CustomTreeView::initialize);

    connect(fileSystemModel, &FileSystemModel::fetchStarted, this, &CustomTreeView::setBusyCursor);
    connect(fileSystemModel, &FileSystemModel::fetchFinished, this, &CustomTreeView::setNormalCursor);
    connect(this, &CustomTreeView::activated, this, &CustomTreeView::expandOrCollapse);

    QTreeView::setModel(model);
}

void CustomTreeView::setRootIndex(const QModelIndex &index)
{
    QTreeView::setRootIndex(index);
}

QModelIndex CustomTreeView::selectedItem()
{
    if (selectionModel()->selectedIndexes().size() > 0) {
        QModelIndex index = selectionModel()->selectedIndexes().at(0);
        if (index.isValid())
            return index;
    }
    return QModelIndex();
}

void CustomTreeView::initialize()
{
    qDebug() << "CustomTreeView::initialize";

    FileSystemModel *fileSystemModel = reinterpret_cast<FileSystemModel *>(model());

    QModelIndex root = fileSystemModel->index(0, 0, QModelIndex());

    // Expand the root and select it
    expand(root);
    selectionModel()->select(root, QItemSelectionModel::SelectCurrent);
}

void CustomTreeView::setNormalCursor()
{
    setCursor(Qt::ArrowCursor);
}

void CustomTreeView::setBusyCursor()
{
    setCursor(Qt::BusyCursor);
}

void CustomTreeView::expandOrCollapse(const QModelIndex &index)
{
    if (model()->hasChildren(index)) {
        if (!isExpanded(index))
            expand(index);
        else
            collapse(index);
    }
}

QModelIndex CustomTreeView::moveCursor(QAbstractItemView::CursorAction cursorAction, Qt::KeyboardModifiers modifiers)
{
    QModelIndex newIndex = QTreeView::moveCursor(cursorAction, modifiers);
    emit clicked(newIndex);
    return newIndex;
}
