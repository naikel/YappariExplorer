#include <QApplication>
#include <QHeaderView>
#include <QKeyEvent>
#include <QDebug>

#include "once.h"
#include "customtreeview.h"
#include "Model/filesystemmodel.h"
#include "Base/baseitemdelegate.h"

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

void CustomTreeView::selectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    viewFocusChanged();
    BaseTreeView::selectionChanged(selected, deselected);
}

void CustomTreeView::viewFocusChanged()
{
    QModelIndex index = selectedItem();
    if (index.isValid() && index.internalPointer() != nullptr) {

        FileSystemItem *item = static_cast<FileSystemItem *>(index.internalPointer());
        emit viewFocus(item);
    }
}


QModelIndex CustomTreeView::moveCursor(QAbstractItemView::CursorAction cursorAction, Qt::KeyboardModifiers modifiers)
{
    qDebug() << "CustomTreeView::moveCursor" << cursorAction;
    QModelIndex newIndex = QTreeView::moveCursor(cursorAction, modifiers);

    if (cursorAction < QAbstractItemView::MoveNext)
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
                    return;
                }
            }
            break;
        case Qt::LeftButton:
            viewFocusChanged();
            BaseTreeView::mousePressEvent(event);
            break;
        default:
            BaseTreeView::mousePressEvent(event);
    }
}

/*!
 * \brief Handles a viewport event
 * \param event the viewport event
 *
 * This function fixes a Qt bug where expand/collapse icons do not return to normal colors after
 * hovering the mouse out of them.  See QTBUG-86852.
 *
 * Basically this function updates the old row.
 */
bool CustomTreeView::viewportEvent(QEvent *event)
{
    switch (event->type()) {
        case QEvent::HoverEnter:
        case QEvent::HoverLeave:
        case QEvent::HoverMove: {

            QHoverEvent *he = static_cast<QHoverEvent*>(event);
            QModelIndex index = indexAt(he->pos());
            if (index.isValid() && index != hoverIndex) {

                // Workaround for QTBUG-86852
                QRect rect = visualRect(hoverIndex);
                rect.setX(0);
                rect.setWidth(viewport()->width());
                viewport()->update(rect);
            }
            hoverIndex = index;
            break;
        }
        default:
            break;
    }

    return QTreeView::viewportEvent(event);
}
