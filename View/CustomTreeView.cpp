#include <QApplication>
#include <QHeaderView>
#include <QKeyEvent>
#include <QPainter>
#include <QMimeData>
#include <QDebug>
#include <QDir>

#include "once.h"
#include "CustomTreeView.h"
#include "Model/SortModel.h"
#include "Base/BaseItemDelegate.h"

#include <QProxyStyle>

/*!
 * \brief A ProxyStyle class.
 *
 * This proxy style class fixes a bug where the base style not always clear the decoration after the cursor
 * has left hovering it.
 *
 */
class ProxyStyle : public QProxyStyle {

    void drawPrimitive(QStyle::PrimitiveElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget = nullptr) const override {

        QStyleOption *opt = const_cast<QStyleOption *>(option);

        if (element == QStyle::PE_IndicatorBranch) {

            const CustomTreeView *treeView = dynamic_cast<const CustomTreeView *>(widget);
            QPoint pos = treeView->mapFromGlobal(QCursor::pos());
            QPoint pos2 = pos;
            pos2 += { treeView->indentation(), 0 };

            // Cursor is not hovering this item
            if (!treeView->indexAt(pos).isValid() && !treeView->indexAt(pos2).isValid())
                opt->state &= ~QStyle::State_MouseOver;
        }

        QProxyStyle::drawPrimitive(element, opt, painter, widget);
    }

};

CustomTreeView::CustomTreeView(QWidget *parent) : BaseTreeView(parent)
{
    // Custom initialization
    setAutoExpandDelay(500);
    setHeaderHidden(true);
    setSelectionBehavior(QAbstractItemView::SelectItems);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setFrameShape(QFrame::NoFrame);
    setSortingEnabled(true);

    header()->setSortIndicator(0, Qt::AscendingOrder);

    header()->setSectionResizeMode(QHeaderView::ResizeToContents);

    // Animations don't really work when rows are inserted dynamically
    setAnimated(false);

    // Disconnect the default customContextMenuRequested to disable background context menu requests
    // Context menu on items is done by mousePressEvent directly
    disconnect(this, &QTreeView::customContextMenuRequested, 0, 0);

    BaseItemDelegate *baseDelegate = new BaseItemDelegate(this);
    setItemDelegateForColumn(FileSystemModel::Columns::Name, baseDelegate);
    setStyle(new ProxyStyle);
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

void CustomTreeView::shouldEdit(QModelIndex sourceIndex)
{
    Q_UNUSED(sourceIndex)

    // A CustomTreeView should never edit on demand
}

void CustomTreeView::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {

        case Qt::Key_F5: {
                QModelIndexList list = selectedIndexes();
                if (list.size() > 0)
                    model()->setData(list[0], false, FileSystemModel::AllChildrenFetchedRole);
            }
            break;

        default:
            BaseTreeView::keyPressEvent(event);
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

void CustomTreeView::setDropAction(QDropEvent *event)
{
    QModelIndex index = indexAt(event->pos());

    // If the index is invalid don't allow a background drop, it makes no sense in the TreeView
    if (!index.isValid()) {
        event->setDropAction(Qt::IgnoreAction);
        return;
    }

    // Ignore the action if the drop is coming from another view and is moving objects to the same
    if (event->possibleActions() & Qt::CopyAction && event->mimeData()->urls().size() > 0) {
        QUrl url = event->mimeData()->urls().at(0);
        QString srcPath = url.toLocalFile();

#ifdef Q_OS_WIN
        srcPath = srcPath.replace('/', QDir::separator());
#endif

        QString dstPath = index.data(FileSystemModel::PathRole).toString();

        int i = srcPath.lastIndexOf(QDir::separator());
        if (i > 0 && srcPath.left(i) == dstPath)
            event->setDropAction(Qt::CopyAction);
    }
}

void CustomTreeView::dragMoveEvent(QDragMoveEvent *event)
{
    BaseTreeView::dragMoveEvent(event);

    if (event->isAccepted())
        setDropAction(event);
}

void CustomTreeView::dropEvent(QDropEvent *event)
{
    // If no keyboard modifiers are being pressed, select the default action for the drop target
    if (!event->keyboardModifiers()) {
        SortModel *sortModel = reinterpret_cast<SortModel *>(model());
        event->setDropAction(sortModel->defaultDropActionForIndex(indexAt(event->pos()), event->mimeData(), event->possibleActions()));
    }

    setDropAction(event);

    QTreeView::dropEvent(event);
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

/*!
 * \brief A mouse button was clicked.
 * \param event the QMouseEvent that represents the mouse button that was sent to the widget.
 *
 * This function handles the following situations:
 *
 * - Ignores background context menu requests for the TreeView
 * - Avoids dragging "the background" making the TreeView change selections very quickly
 */
void CustomTreeView::mousePressEvent(QMouseEvent *event)
{
    switch (event->button()) {
        case Qt::RightButton:
            {
                QPoint pos = event->pos();
                QModelIndex index = indexAt(pos);

                // Only allow context menu on an item, and ignore background context menu requests
                if (index.isValid()) {

                    // Save current selection
                    QModelIndex lastSelection = selectedItem();

                    // This will select the index
                    contextMenuRequested(event->pos());

                    // Restore previous selection
                    if (index.isValid())
                        setCurrentIndex(lastSelection);

                    event->accept();
                    return;
                }
            }
            break;
        case Qt::LeftButton:
            {
                QPoint pos = event->pos();
                QModelIndex index = indexAt(pos);

                if (!index.isValid())
                    draggingNothing = true;

                BaseTreeView::mousePressEvent(event);

                break;
            }
        default:
            BaseTreeView::mousePressEvent(event);
    }
}

void CustomTreeView::mouseReleaseEvent(QMouseEvent *event)
{
    draggingNothing = false;
    BaseTreeView::mouseReleaseEvent(event);
}

void CustomTreeView::mouseMoveEvent(QMouseEvent *event)
{
    if (!draggingNothing)
        BaseTreeView::mouseMoveEvent(event);
}

void CustomTreeView::mouseDoubleClickEvent(QMouseEvent *event)
{
    // Double click at a decoration must be treated as a single click

    if (!indexAt(event->pos()).isValid() && indexAt(event->pos() + QPoint(indentation(), 0)).isValid())
        mousePressEvent(event);
    else
        BaseTreeView::mouseDoubleClickEvent(event);
}

/*!
 * \brief Handles a viewport event
 * \param event the viewport event
 *
 * This function fixes a Qt bug where expand/collapse icons do not return to normal colors after
 * hovering the mouse out of them.  See QTBUG-86852.
 *
 * It also has a better detection of the decoration than the base implementation because of the
 * new implementation of indexAt() and visualRect() functions.
 *
 * Basically this function updates the old row.
 *
 * \see CustomTreeView::indexAt
 * \see CustomTreeView::visualRect
 */
bool CustomTreeView::viewportEvent(QEvent *event)
{
    switch (event->type()) {
        case QEvent::HoverEnter:
        case QEvent::HoverLeave:
        case QEvent::HoverMove: {

            QHoverEvent *he = static_cast<QHoverEvent*>(event);
            QPoint pos = he->pos();
            QModelIndex index = QModelIndex();

            // If the cursor is over the text then it is not over the decoration
            // so it is invalid for the purpose of this function
            // Otherwise we try to get the index of the decoration only if the cursor
            // is over a decoration
            if (!indexAt(he->pos()).isValid()) {
                pos += { indentation(), 0 };
                index = indexAt(pos);
            }

            if (hoverIndex.isValid() && index != hoverIndex) {

                // Workaround for QTBUG-86852
                // Clear last hovered index
                QRect rect = visualRect(hoverIndex);
                rect.setX(0);
                rect.setWidth(viewport()->width());
                viewport()->update(rect);
            }
            hoverIndex = index;

            if (index.isValid()) {
                // Update the hovered row
                QRect rect = visualRect(index);
                rect.setX(0);
                rect.setWidth(viewport()->width());
                viewport()->update(rect);
            }
            break;
        }
        default:
            break;
    }

    return QTreeView::viewportEvent(event);
}
