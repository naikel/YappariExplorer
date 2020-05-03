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

    // Disconnect the default customContextMenuRequested and connect our own
    disconnect(this, &QTreeView::customContextMenuRequested, this, &BaseTreeView::contextMenuRequested);
    //connect(this, &QTreeView::customContextMenuRequested, this, &CustomTreeView::contextMenuRequested);
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

                // Save current selection
                QModelIndex lastSelection = selectedItem();

                // Change the selection temporary to the item at the event position
                QModelIndex index = indexAt(pos);
                selectIndex(index);
                contextMenuRequested(event->pos());

                // Restore previous selection
                selectIndex(lastSelection);
                event->accept();
            }
            break;
        default:
            BaseTreeView::mousePressEvent(event);
    }
}

/*!
 * \brief A context menu was requested.
 * \param pos the QPoint position where the context menu was requested, relative to the widget.
 *
 * This function verifies if the context menu was requested on an item, to avoid background requests.
 */
void CustomTreeView::contextMenuRequested(const QPoint &pos)
{
    // Only allow context menu on an item, and ignore background context menu requests
    QModelIndex index = indexAt(pos);
    if (index.isValid() && index.internalPointer() != nullptr) {
        BaseTreeView::contextMenuRequested(pos);
    }
}
