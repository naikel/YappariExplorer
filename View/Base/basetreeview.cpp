#include <QScrollBar>
#include <QHeaderView>
#include <QApplication>
#include <QMessageBox>
#include <QMimeData>
#include <QKeyEvent>
#include <QTimer>
#include <QDebug>
#include <QDrag>

#include "baseitemdelegate.h"
#include "basetreeview.h"

#include "once.h"
#include "Model/filesystemmodel.h"

/*!
 * \brief The constructor.
 * \param parent The QWidget parent.
 *
 * The constructor only enables the context menu.
 */
BaseTreeView::BaseTreeView(QWidget *parent) : QTreeView(parent)
{
    setAcceptDrops(true);
    setDragEnabled(true);
    setDragDropMode(QAbstractItemView::DragDrop);
    setDropIndicatorShown(false);
    setEditTriggers(QAbstractItemView::SelectedClicked | QAbstractItemView::EditKeyPressed);
    setContextMenuPolicy(Qt::CustomContextMenu);
    setUniformRowHeights(true);
    connect(this, &QTreeView::customContextMenuRequested, this, &BaseTreeView::contextMenuRequested);

    // This is the timer to process queued dataChanged signals (of icons updates only) and send them as a few signals as possible
    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &BaseTreeView::processQueuedSignals);
    timer->start(100);
}

/*!
 * \brief Sets the model for the view.
 * \param model Model for the view.
 *
 * Sets the model for the view to present.  The model is a FileSystemModel.
 *
 * After the model has finished fetching the initial items the function BaseTreeView::initialize will be called
 * to finish initialization of the view.
 *
 * This function also configures the mouse cursor to show a busy icon when the model is fetching new items.
 *
 * \sa FileSystemModel::initialize
 */
void BaseTreeView::setModel(QAbstractItemModel *model)
{
    FileSystemModel *oldModel = getFileSystemModel();
    if (oldModel == model)
        return;

    if (oldModel != nullptr) {
        disconnect(oldModel, &FileSystemModel::fetchStarted, this, &BaseTreeView::setBusyCursor);
        disconnect(oldModel, &FileSystemModel::fetchFinished, this, &BaseTreeView::setNormalCursor);
        disconnect(oldModel, &FileSystemModel::fetchFailed, this, &BaseTreeView::showError);
        disconnect(oldModel, &FileSystemModel::shouldEdit, this, &BaseTreeView::shouldEdit);
    }

    FileSystemModel *fileSystemModel = static_cast<FileSystemModel *>(model);
    Once::connect(fileSystemModel, &FileSystemModel::fetchFinished, this, &BaseTreeView::initialize);

    connect(fileSystemModel, &FileSystemModel::fetchStarted, this, &BaseTreeView::setBusyCursor);
    connect(fileSystemModel, &FileSystemModel::fetchFinished, this, &BaseTreeView::setNormalCursor);
    connect(fileSystemModel, &FileSystemModel::fetchFailed, this, &BaseTreeView::showError);
    connect(fileSystemModel, &FileSystemModel::shouldEdit, this, &BaseTreeView::shouldEdit);

    QTreeView::setModel(fileSystemModel);
}

/*!
 * \brief Initializes the view.
 *
 * Initializes the view. This function is called after the model has fetched the initial items.
 *
 * This function does nothing in this class.  Subclasses should reimplement this function and put here the
 * custom configuration of their view.
 */
void BaseTreeView::initialize()
{
    qDebug() << "BaseTreeView::initialize";

    QModelIndex root = model()->index(0 , 0, QModelIndex());
    defaultRowHeight = (root.isValid()) ? rowHeight(root) : 0;
}

/*!
 * \brief Sets the cursor icon to normal.
 */
void BaseTreeView::setNormalCursor()
{
    setCursor(Qt::ArrowCursor);
}

/*!
 * \brief Sets the cursor icon to busy.
 */
void BaseTreeView::setBusyCursor()
{
    setCursor(Qt::BusyCursor);
}

/*!
 * \brief A key press event occurred.
 * \param event the QKeyEvent that represents the key event that was sent to the widget.
 *
 * This function handles the default key actions like Enter, Backspace, etc.
 */
void BaseTreeView::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {

        case Qt::Key_Select:
        case Qt::Key_Enter:
        case Qt::Key_Return:
            if (state() != QAbstractItemView::EditingState) {
                selectEvent();
                event->accept();
            } else {
                QTreeView::keyPressEvent(event);
            }
            break;

        case Qt::Key_Back:
        case Qt::Key_Backspace:
            backEvent();
            event->accept();
            break;

        case Qt::Key_Delete:
            deleteSelectedItems();
            break;

        default:
            QTreeView::keyPressEvent(event);
    }
}

/*!
 * \brief A mouse button was clicked.
 * \param event the QMouseEvent that represents the mouse button that was sent to the widget.
 *
 * This function handles the default mouse button actions like Back, Forward, etc.
 */
void BaseTreeView::mousePressEvent(QMouseEvent *event)
{
    switch (event->button()) {
        case Qt::BackButton:
            backEvent();
            event->accept();
            break;
        default:
            QTreeView::mousePressEvent(event);
    }
}

void BaseTreeView::startDrag(Qt::DropActions supportedActions)
{
    Q_UNUSED(supportedActions)

    qDebug() << "BaseTreeView:startDrag";

    Qt::DropActions dropActions = getFileSystemModel()->supportedDragActionsForIndexes(selectedIndexes());
    QTreeView::startDrag(dropActions);
}

void BaseTreeView::dragEnterEvent(QDragEnterEvent *event)
{
    qDebug() << "BaseTreeView::dragEnterEvent";

    // This will set the QAbstractItemView state to DraggingState and allows autoexpand of trees
    QTreeView::dragEnterEvent(event);

    // ToDo: Support for FileGroupDescriptorW in Windows
    // qDebug() << event->mimeData()->formats()
    if (event->mimeData()->hasFormat("text/uri-list")) {
        event->acceptProposedAction();
    } else
        event->ignore();
}

void BaseTreeView::dragMoveEvent(QDragMoveEvent *event)
{
    // Process default events (autoexpand tree on drag, keyboard modifiers and highlight of destination)
    QTreeView::dragMoveEvent(event);

    // If no keyboard modifiers are being pressed, select the default action for the drop target
    if (event->isAccepted() && !event->keyboardModifiers()) {
        event->setDropAction(getFileSystemModel()->defaultDropActionForIndex(indexAt(event->pos()), event->mimeData(), event->possibleActions()));
    }
}

void BaseTreeView::dropEvent(QDropEvent *event)
{
    qDebug() << "BaseTreeView::dropEvent" << event->dropAction();

    // If no keyboard modifiers are being pressed, select the default action for the drop target
    if (!event->keyboardModifiers()) {
        event->setDropAction(getFileSystemModel()->defaultDropActionForIndex(indexAt(event->pos()), event->mimeData(), event->possibleActions()));
    }
    QTreeView::dropEvent(event);
}

/*!
 * \brief Returns the rectangle from the viewport of the items in the given selection.
 * \param selection the selected items.
 *
 * This is almost a copy of the QTreeView implementation except for one added line.
 *
 * Since we do not want highlighting of the selected items to be of column width, the visualRect()
 * function will return the exact rectangle of the item and not the column width.
 *
 * This function only calls visualRect() on the first and last element of the range.  If a bigger
 * element is in between, the region will ignore that width and the selection range would look
 * like a total disaster.
 *
 * To avoid the problem we force the last element of the range to be of the width of the column.
 *
 * \see visualRect()
 */
QRegion BaseTreeView::visualRegionForSelection(const QItemSelection &selection) const
{
    if (selection.isEmpty())
        return QRegion();
    QRegion selectionRegion;
    const QRect &viewportRect = viewport()->rect();
    for (const auto &range : selection) {
        if (!range.isValid())
            continue;
        QModelIndex parent = range.parent();
        QModelIndex leftIndex = range.topLeft();
        int columnCount = model()->columnCount(parent);
        while (leftIndex.isValid() && isIndexHidden(leftIndex)) {
            if (leftIndex.column() + 1 < columnCount)
                leftIndex = model()->index(leftIndex.row(), leftIndex.column() + 1, parent);
            else
                leftIndex = QModelIndex();
        }
        if (!leftIndex.isValid())
            continue;
        const QRect leftRect = visualRect(leftIndex);
        int top = leftRect.top();
        QModelIndex rightIndex = range.bottomRight();
        while (rightIndex.isValid() && isIndexHidden(rightIndex)) {
            if (rightIndex.column() - 1 >= 0)
                rightIndex = model()->index(rightIndex.row(), rightIndex.column() - 1, parent);
            else
                rightIndex = QModelIndex();
        }
        if (!rightIndex.isValid())
            continue;
        QRect rightRect = visualRect(rightIndex);

        // Increase the rectangle to the whole column
        // This is the only line different to the original implementation
        rightRect.setWidth(columnWidth(rightIndex.column()));

        int bottom = rightRect.bottom();
        if (top > bottom)
            qSwap<int>(top, bottom);
        int height = bottom - top + 1;
        if (header()->sectionsMoved()) {
            for (int c = range.left(); c <= range.right(); ++c) {
                const QRect rangeRect(columnViewportPosition(c), top, columnWidth(c), height);
                if (viewportRect.intersects(rangeRect))
                    selectionRegion += rangeRect;
            }
        } else {
            QRect combined = leftRect|rightRect;
            combined.setX(columnViewportPosition(isRightToLeft() ? range.right() : range.left()));
            if (viewportRect.intersects(combined))
                selectionRegion += combined;
        }
    }
    return selectionRegion;
}

bool BaseTreeView::edit(const QModelIndex &index, QAbstractItemView::EditTrigger trigger, QEvent *event)
{
    bool result = QTreeView::edit(index, trigger, event);

    if (result) {

        QAbstractItemDelegate *del = itemDelegate(index);
        connect(del, &BaseItemDelegate::closeEditor, this, &BaseTreeView::editorClosed);
        editIndex = index;
    }

    return result;
}

void BaseTreeView::editorClosed()
{
    qDebug() << "BaseTreeView::editClosed";
    QAbstractItemDelegate *del = itemDelegate(editIndex);
    disconnect(del, &BaseItemDelegate::closeEditor, this, &BaseTreeView::editorClosed);

    editIndex = QModelIndex();
}

void BaseTreeView::shouldEdit(QModelIndex index)
{
    selectionModel()->select(index, QItemSelectionModel::ClearAndSelect);
    QAbstractItemView::edit(index);
}

/*!
 * \brief Handles a Select/Enter/Return event.
 *
 * This function does nothing. Subclasses must implement this function.
 */
void BaseTreeView::selectEvent()
{
    qDebug() << "BaseTreeView::selectEvent";
}

/*!
 * \brief Handles a Back/Backspace event.
 *
 * This function does nothing. Subclasses must implement this function.
 */
void BaseTreeView::backEvent()
{
    qDebug() << "BaseTreeView::backEvent";
}

int BaseTreeView::getDefaultRowHeight() const
{
    return defaultRowHeight;
}

bool BaseTreeView::isEditingIndex(const QModelIndex &index) const
{
    return ((state() & QAbstractItemView::EditingState) && index == editIndex);
}

void BaseTreeView::deleteSelectedItems()
{
    QModelIndexList list = selectedIndexes();

    QString dest;
    if (list.size() == 1) {
        QModelIndex selectedIndex = list.at(0);
        if (selectedIndex.isValid()) {
            FileSystemItem *item = getFileSystemModel()->getFileSystemItem(selectedIndex);
            dest = "\"" + item->getDisplayName() + "\"";
        }
    } else {
        dest = QString::number(list.size()) + " "+ tr("files");
    }

    // TODO: Recycle bin might be disabled
    // TODO: It's called Trash in Unix and it' s in $XDG_DATA_HOME/Trash or .local/share/Trash

    QString action = tr("send") + " " + dest + " "  + tr("to the Recycle bin");
    QString text = tr("Do you really want to") + " " + action + "?";

    // TODO: Better icon for this
    if (QMessageBox::question(this, tr("Confirm File Delete"), text) == QMessageBox::Yes) {
        getFileSystemModel()->removeIndexes(list);
    }
}

bool BaseTreeView::isDragging() const
{
    return state() == DraggingState;
}

/*!
 * \brief Process items that have been changed in the model.
 * \param topLeft QModelIndex at the top left of the model changes rectangle
 * \param bottomRight QModelIndex at the bottom right of the model changes rectangle
 * \param roles Roles that the model has changed
 *
 * This function will queue signals with role Qt:DecorationRole (icons) for delayed processing as a bundle.
 * For all other signals, they are sent to the QTreeView base implementation.
 *
 * \see processQueuedSignals()
 */
void BaseTreeView::dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles)
{
    if (topLeft == bottomRight && topLeft.isValid() && roles.contains(Qt::DecorationRole)) {

        QModelIndex parent = topLeft.parent();

        // We need exlusive access to the signalsQueue object
        mutex.lock();
        if (signalsQueue.contains(parent)) {
            QMap<int, QModelIndex> *queue = signalsQueue.value(parent);
            queue->insert(topLeft.row(), topLeft);
        } else {
            QMap<int, QModelIndex> *queue = new QMap<int, QModelIndex>();
            queue->insert(topLeft.row(), topLeft);
            signalsQueue.insert(parent, queue);
        }
        mutex.unlock();
    } else
        QTreeView::dataChanged(topLeft, bottomRight, roles);
}


/*!
 * \brief Returns the model index of the item at the viewport coordinates point.
 * \param point a QPoint object
 * \return a valid QModelIndex object if there's a valid index at the point or an invalid one if not.
 *
 * If the point is inside the first column this function will return a valid index only if the point
 * is above the exact space occupied by the index, and not from the empty space in the row.
 *
 * \see visualRect()
 */
QModelIndex BaseTreeView::indexAt(const QPoint &point) const
{
    QModelIndex index = QTreeView::indexAt(point);

    if (index.isValid() && index.column() == 0) {

        QRect rect = visualRect(index);
        if (rect.x() > point.x() || (rect.width() + rect.x()) < point.x())
            return QModelIndex();
    }

    return index;
}

/*!
 * \brief Returns the rectangle on the viewport occupied by the item at index.
 * \param index a QModelIndex.
 * \return a QRect object with the rectangle occupied by the item.
 *
 * The base implementation always return rectangles of the column width.
 * This implementation will return the exact rectangle the item occupies if the item
 * is in the first column.
 */
QRect BaseTreeView::visualRect(const QModelIndex &index) const
{
    QRect rect = QTreeView::visualRect(index);

    if (index.isValid() && index.column() == 0) {

        // Fix visual rectangle
        QFontMetrics fm = QFontMetrics(font());
        int x1 = 1;
        int x2 = fm.size(0, index.data().toString()).width();
        int iconSize = QApplication::style()->pixelMetric(QStyle::PM_SmallIconSize);

        // TODO Where is this magic number from??
        x2 += iconSize + 11;

        FileSystemItem *parent = getFileSystemModel()->getFileSystemItem(index)->getParent();
        int level = (rootIsDecorated()) ? 0 : -1;
        while (parent != nullptr) {
            level++;
            parent = parent->getParent();
        }
        int padding = level * indentation();
        x1 += padding;
        x2 += padding;

        if (rootIsDecorated()) {
            x1 += iconSize + 7;
            x2 += indentation();
        }

        int width = qMin(x2 - x1 + 1, columnWidth(0) - 1);

        return QRect(x1, rect.y(), width, rect.height());

    }
    return rect;
}

QPoint BaseTreeView::mapToViewport(QPoint pos)
{
    QPoint viewportPos;

    // Y
    if (verticalScrollMode() == QAbstractItemView::ScrollPerPixel) {
        viewportPos.setY(pos.y() + verticalScrollBar()->value());
    } else {
        viewportPos.setY(pos.y() + (verticalScrollBar()->value() * defaultRowHeight));
    }

    // Horizontal bar is always scroll per pixel
    viewportPos.setX(pos.x() + horizontalScrollBar()->value());

    return viewportPos;

}

QPoint BaseTreeView::mapFromViewport(QPoint pos)
{
    QPoint treeViewPos;

    // Y
    if (verticalScrollMode() == QAbstractItemView::ScrollPerPixel) {
        treeViewPos.setY(pos.y() - verticalScrollBar()->value());
    } else {
        treeViewPos.setY(pos.y() - (verticalScrollBar()->value() * defaultRowHeight));
    }

    // Horizontal bar is always scroll per pixel
    treeViewPos.setX(pos.x() - horizontalScrollBar()->value());

    return treeViewPos;
}

/*!
 * \brief A context menu was requested.
 * \param pos the QPoint position where the context menu was requested, relative to the widget.
 *
 * This function determines if the context menu was requested on an item or in the background, and then
 * sends the BaseTreeView::contextMenuRequestedForItems signal.
 */
void BaseTreeView::contextMenuRequested(const QPoint &pos)
{
    qDebug() << "BaseTreeView::contextMenuRequested";

    QList<FileSystemItem *> fileSystemItems;
    ContextMenu::ContextViewAspect viewAspect;

    // Get current selected item if any
    QModelIndex index = indexAt(pos);
    if (index.isValid() && index.column() == 0 && index.internalPointer() != nullptr) {
        viewAspect = ContextMenu::Selection;
        for (QModelIndex selectedIndex : selectedIndexes()) {
            if (selectedIndex.column() == 0 && selectedIndex.internalPointer() != nullptr) {
                FileSystemItem *fileSystemItem = getFileSystemModel()->getFileSystemItem(selectedIndex);
                fileSystemItems.append(fileSystemItem);
                qDebug() << "BaseTreeView::contextMenuRequested selected for " << fileSystemItem->getPath();
            }
        }
    } else {
        viewAspect = ContextMenu::Background;
        FileSystemItem *fileSystemItem = getFileSystemModel()->getRoot();
        fileSystemItems.append(fileSystemItem);
        qDebug() << "BaseTreeView::contextMenuRequested selected for the background on " << fileSystemItem->getPath();
    }

    // If there's a header in this view, position would include it. We have to skip it.
    QPoint destination = pos;
    destination.setY(destination.y() + header()->height());
    emit contextMenuRequestedForItems(mapToGlobal(destination), fileSystemItems, viewAspect, this);
}


void BaseTreeView::showError(qint32 err, QString errMessage)
{
    Q_UNUSED(err)

    setNormalCursor();

    FileSystemModel *fileSystemModel = getFileSystemModel();
    QString path = fileSystemModel->getRoot()->getPath();
    QMessageBox::critical(this, tr("Error"), errMessage + "\n" + path);
}

bool BaseTreeView::setRoot(QString path)
{
    Q_UNUSED(path)

    mutex.lock();
    signalsQueue.clear();
    mutex.unlock();

    return true;
}

/*!
 * \brief Process queued dataChanged signals
 *
 * This function will try to bundle all the queued signals in as few signals as possible
 * to achieve better performance.
 *
 * Only Qt:DecorationRole (icons) signals are queued.
 *
 * \see BaseTreeView::dataChanged
 */
void BaseTreeView::processQueuedSignals()
{
    mutex.lock();
    for (QModelIndex parent : signalsQueue.keys()) {
        QMap<int, QModelIndex> *queue = signalsQueue.value(parent);

        QModelIndex topIndex     {};
        QModelIndex bottomIndex  {};

        int topRow               { -1 };
        int bottomRow            { -1 };

        QVector<int> roles;
        roles.append(Qt::DecorationRole);

        for (int row : queue->keys()) {

            if (topRow < 0) {
                topRow = row;
                topIndex = queue->value(row);
            }

            if (bottomRow < 0 || (bottomRow + 1) == row) {
                bottomRow = row;
                bottomIndex = queue->value(row);

            } else {

                qDebug() << "BaseTreeView::processQueuedSignals from" << topRow << "to" << bottomRow;
                QTreeView::dataChanged(topIndex, bottomIndex, roles);

                topRow = -1;
                bottomRow = -1;
            }
        }

        if (topRow >= 0 && bottomRow >= 0) {
            qDebug() << "BaseTreeView::processQueuedSignals from" << topRow << "to" << bottomRow;
            QTreeView::dataChanged(topIndex, bottomIndex, roles);
        }

        delete queue;
    }
    signalsQueue.clear();
    mutex.unlock();
}

