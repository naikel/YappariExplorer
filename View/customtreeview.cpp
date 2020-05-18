#include <QApplication>
#include <QHeaderView>
#include <QKeyEvent>
#include <QDebug>

#include "once.h"
#include "customtreeview.h"
#include "Model/filesystemmodel.h"

CustomTreeView::CustomTreeView(QWidget *parent) : BaseTreeView(parent)
{
    // Custom initialization
    setAutoExpandDelay(500);
    setHeaderHidden(true);
    setSelectionBehavior(QAbstractItemView::SelectItems);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setFrameShape(QFrame::NoFrame);

    header()->setSectionResizeMode(QHeaderView::ResizeToContents);

    // Animations don't really work when rows are inserted dynamically
    setAnimated(false);

    // Disconnect the default customContextMenuRequested to disable background context menu requests
    // Context menu on items is done by mousePressEvent directly
    disconnect(this, &QTreeView::customContextMenuRequested, 0, 0);
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

    FileSystemModel *fileSystemModel = getFileSystemModel();

    QModelIndex root = fileSystemModel->index(0, 0, QModelIndex());

    // Expand the root and select it
    expand(root);
    selectionModel()->select(root, QItemSelectionModel::SelectCurrent);
}

void CustomTreeView::selectIndex(QModelIndex index)
{
    selectionModel()->select(index, QItemSelectionModel::ClearAndSelect);
    scrollTo(index);
}

QModelIndex CustomTreeView::moveCursor(QAbstractItemView::CursorAction cursorAction, Qt::KeyboardModifiers modifiers)
{
    qDebug() << "CustomTreeView::moveCursor";
    QModelIndex newIndex = QTreeView::moveCursor(cursorAction, modifiers);
    emit clicked(newIndex);
    return newIndex;
}

void CustomTreeView::resizeEvent(QResizeEvent *event)
{
    emit resized();

    qDebug() << "CustomTreeView::resizeEvent resize to " << event->size();

    QTreeView::resizeEvent(event);
}

void CustomTreeView::selectEvent()
{
    qDebug() << "CustomTreeView::selectEvent";
    if (currentIndex().isValid()) {
        if (!isExpanded(currentIndex()))
            expand(currentIndex());
        else
            collapse(currentIndex());
    }
}

void CustomTreeView::mousePressEvent(QMouseEvent *event)
{
    switch (event->button()) {
        case Qt::RightButton:
            {
                QPoint pos = event->pos();
                QModelIndex index = indexAt(pos);

                // Only allow context menu on an item, and ignore background context menu requests
                if (index.isValid() && index.internalPointer() != nullptr) {

                    // Save current selection
                    QModelIndex lastSelection = selectedItem();

                    // Select temporarily the index and request context menu
                    selectIndex(index);
                    contextMenuRequested(event->pos());

                    // Restore previous selection
                    selectIndex(lastSelection);
                    event->accept();
                }
            }
            break;
        default:
            BaseTreeView::mousePressEvent(event);
    }
}
