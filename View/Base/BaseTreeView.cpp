#include <QScrollBar>
#include <QHeaderView>
#include <QApplication>
#include <QMessageBox>
#include <QMimeData>
#include <QKeyEvent>
#include <QTimer>
#include <QDebug>
#include <QDrag>

#include "BaseItemDelegate.h"
#include "BaseTreeView.h"

#include "once.h"
#include "Model/FileSystemModel.h"
#include "Model/TreeModel.h"

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
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    connect(this, &QTreeView::customContextMenuRequested, this, &BaseTreeView::contextMenuRequested);

    connect(this, &QTreeView::expanded, [=](const QModelIndex &index) { this->updateRefCounter(index, true); } );
    connect(this, &QTreeView::collapsed, [=](const QModelIndex &index) { this->updateRefCounter(index, false); } );

    // This is the timer to process queued dataChanged signals (of icons updates only) and send them as a few signals as possible
    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &BaseTreeView::processQueuedSignals);
    timer->start(10);
}

BaseTreeView::~BaseTreeView()
{
    QModelIndex index = rootIndex();
    if (index.isValid()) {
        model()->setData(index, QVariant(), FileSystemModel::DecreaseRefCounterRole);
        qDebug() << "REF COUNTERS" << index.data(FileSystemModel::PathRole) << index.data(FileSystemModel::RefCounterRole).toInt();
    }
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

        case Qt::Key_F5:
            model()->setData(rootIndex(), false, FileSystemModel::AllChildrenFetchedRole);
            break;

        // Disable expand all
        case Qt::Key_Asterisk:
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

        case Qt::ForwardButton:
            forwardEvent();
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

#ifdef Q_OS_WIN
    Qt::DropActions dropActions = Qt::ActionMask;

    QModelIndexList selection = selectedIndexes();
    for (const QModelIndex &index : selection) {
        if (index.isValid() && (index.flags() & Qt::ItemIsDragEnabled) && index.data(FileSystemModel::DriveRole).toBool()) {
            dropActions = Qt::LinkAction;
            break;
        }
    }

    if (dropActions == Qt::ActionMask)
        dropActions = model()->supportedDragActions();
#else
    Qt::DropActions dropActions = model()->supportedDragActions();
#endif

    QTreeView::startDrag(dropActions);
}

void BaseTreeView::dragEnterEvent(QDragEnterEvent *event)
{
    qDebug() << "BaseTreeView::dragEnterEvent";

    // This will set the QAbstractItemView state to DraggingState and allows autoexpand of trees
    QTreeView::dragEnterEvent(event);

    // TODO: Support for FileGroupDescriptorW in Windows
    // See: https://docs.microsoft.com/en-us/windows/win32/shell/clipboard
    // qDebug() << event->mimeData()->formats();
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
        SortModel *sortModel = reinterpret_cast<SortModel *>(model());
        event->setDropAction(sortModel->defaultDropActionForIndex(indexAt(event->pos()), event->mimeData(), event->possibleActions()));
    }
}

void BaseTreeView::dropEvent(QDropEvent *event)
{
    qDebug() << "BaseTreeView::dropEvent" << event->dropAction();

    // If no keyboard modifiers are being pressed, select the default action for the drop target
    if (!event->keyboardModifiers()) {
        SortModel *sortModel = reinterpret_cast<SortModel *>(model());
        event->setDropAction(sortModel->defaultDropActionForIndex(indexAt(event->pos()), event->mimeData(), event->possibleActions()));
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
    qDebug() << "BaseTreeView::shouldEdit";
    setCurrentIndex(index);
    QAbstractItemView::edit(index);
}

void BaseTreeView::updateRefCounter(QModelIndex index, bool increase)
{
    QModelIndex i = index;
    while (i.isValid()) {

        model()->setData(i, QVariant(), increase ? FileSystemModel::IncreaseRefCounterRole : FileSystemModel::DecreaseRefCounterRole);

        qDebug() << "REF COUNTERS" << i.data(FileSystemModel::PathRole) << i.data(FileSystemModel::RefCounterRole).toInt();

        i = i.parent();
    }
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

/*!
 * \brief Handles a Forward event.
 *
 * This function does nothing. Subclasses must implement this function.
 */
void BaseTreeView::forwardEvent()
{
    qDebug() << "BaseTreeView::forwardEvent";
}

QList<int> BaseTreeView::getVisualIndexes() const
{
    return visualIndexes;
}

void BaseTreeView::setVisualIndexes(const QList<int> &value)
{
    visualIndexes = value;
}

Qt::SortOrder BaseTreeView::getSortOrder() const
{
    return sortOrder;
}

void BaseTreeView::setSortOrder(const Qt::SortOrder &value)
{
    sortOrder = value;
}

int BaseTreeView::getSortingColumn() const
{
    return sortingColumn;
}

void BaseTreeView::setSortingColumn(int value)
{
    sortingColumn = value;
}

QList<int> BaseTreeView::getColumnsWidth() const
{
    return columnsWidth;
}

void BaseTreeView::setColumnsWidth(const QList<int> &value)
{
    columnsWidth = value;
}

QString BaseTreeView::getPath() const
{
    return path;
}

void BaseTreeView::setPath(const QString &value)
{
    path = value;
}

bool BaseTreeView::isEditingIndex(const QModelIndex &index) const
{
    return ((state() & QAbstractItemView::EditingState) && index == editIndex);
}

void BaseTreeView::deleteSelectedItems()
{
    QModelIndexList list = selectedIndexes();

    QString dest;

    if (list.size() > 0) {

        QModelIndex selectedIndex = list.at(0);
        if (selectedIndex.isValid()) {

            if (list.size() == 1) {
                dest = "\"" + selectedIndex.data(Qt::DisplayRole).toString() + "\"";
            } else {
                dest = QString::number(list.size()) + " "+ tr("items");
            }

            // TODO: It's called Trash in Unix and it's in $XDG_DATA_HOME/Trash or .local/share/Trash

            QString action;
            bool perm;
            SortModel *sortModel = reinterpret_cast<SortModel *>(model());

            // Recycle bin is at least per folder so we only need to check one item to see if the Recycle Bin is enabled
            if (!sortModel->willRecycle(selectedIndex) || QApplication::keyboardModifiers() & Qt::ShiftModifier) {
                perm = true;
                action = tr("permanently delete") + " " + dest;
            } else {
                perm = false;
                action = tr("send") + " " + dest + " " + tr("to the Recycle bin");
            }

            QString text = tr("Do you really want to") + " " + action + "?";

            // TODO: Better icon for this
            if (QMessageBox::question(this, tr("Confirm File Delete"), text) == QMessageBox::Yes) {
                sortModel->removeIndexes(list, perm);
            }
        }
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
 * This function also tracks when the model starts a new fetch and changes the cursor accordingly.
 *
 * \see processQueuedSignals()
 */
void BaseTreeView::dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles)
{
    if (topLeft == bottomRight && topLeft.isValid()) {

        // If the editor is open these signals break the selection
        if (isPersistentEditorOpen(topLeft))
            return;

        if (roles.contains(FileSystemModel::ErrorCodeRole)) {
            setNormalCursor();
            return;
        }

        // TODO: Should we wait 1 second to show the busy cursor?
        if (roles.contains(FileSystemModel::AllChildrenFetchedRole)) {

            if (!topLeft.data(FileSystemModel::AllChildrenFetchedRole).toBool()) {
                setBusyCursor();
            } else {
                setNormalCursor();
            }
        }

        if (roles.contains(FileSystemModel::ShouldEditRole)) {
            shouldEdit(topLeft);
            return;
        }
    }


    if (roles.contains(Qt::DecorationRole)) {

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
        return;
    }

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

        bool isInRect = (point.x() >= rect.x()) && (point.x() <= rect.x() + rect.width());
        if (!isInRect)
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
        int x2 = fm.size(0, " " + index.data().toString() + " ").width();
        int iconSize = QApplication::style()->pixelMetric(QStyle::PM_SmallIconSize);

        x2 += iconSize;

        if (rootIsDecorated()) {
            QModelIndex parent = index.parent();
            int level = 1;
            while (parent.isValid()) {
                level++;
                parent = parent.parent();
            }
            int padding = level * indentation();
            x1 += padding;
            x2 += padding;
        }

        int width = qMin(x2 - x1, columnWidth(0) - 1);

        return QRect(x1, rect.y(), width, rect.height());
    }
    return rect;
}

void BaseTreeView::storeCurrentSettings()
{
    QHeaderView *viewHeader = header();
    setSortingColumn(viewHeader->sortIndicatorSection());
    setSortOrder(viewHeader->sortIndicatorOrder());

    columnsWidth.clear();
    visualIndexes.clear();

    int count = viewHeader->count();
    for (int section = 0; section < count ; section++) {
        columnsWidth.append(viewHeader->sectionSize(section));
        visualIndexes.append(viewHeader->visualIndex(section));
    }
}

QPoint BaseTreeView::mapToViewport(QPoint pos)
{
    QPoint viewportPos;

    // Vertical bar is forced to scroll per pixel
    viewportPos.setY(pos.y() + verticalScrollBar()->value());

    // Horizontal bar is always scroll per pixel
    viewportPos.setX(pos.x() + horizontalScrollBar()->value());

    return viewportPos;

}

QPoint BaseTreeView::mapFromViewport(QPoint pos)
{
    QPoint treeViewPos;

    // Vertical bar is forced to scroll per pixel
    treeViewPos.setY(pos.y() - verticalScrollBar()->value());

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

    QModelIndexList sourceIndexList;
    ContextMenu::ContextViewAspect viewAspect;

    QSortFilterProxyModel *proxyModel = reinterpret_cast<QSortFilterProxyModel *>(model());

    // Get current selected item if any
    QModelIndex index = indexAt(pos);
    if (index.isValid() && index.column() == 0) {

        // If the index is not selected, we should select it
        // and clear the previous selection

        QModelIndexList indexes = selectedIndexes();
        if (!indexes.contains(index)) {
           selectionModel()->select(index, QItemSelectionModel::ClearAndSelect);
        }

        viewAspect = ContextMenu::Selection;
        QModelIndexList list = selectedIndexes();
        for (const QModelIndex &selectedIndex : list) {
            if (selectedIndex.column() == 0) {
                sourceIndexList.append(proxyModel->mapToSource(selectedIndex));
                qDebug() << "BaseTreeView::contextMenuRequested for item" << selectedIndex.data(FileSystemModel::PathRole).toString();
            }
        }
    } else {
        viewAspect = ContextMenu::Background;
        sourceIndexList.append(proxyModel->mapToSource(rootIndex()));
        qDebug() << "BaseTreeView::contextMenuRequested selected for the background on " << rootIndex().data(FileSystemModel::PathRole).toString();
    }

    // If there's a header in this view, position would include it. We have to skip it.
    QPoint destination = pos;
    destination.setY(destination.y() + header()->height());
    emit contextMenuRequestedForItems(mapToGlobal(destination), sourceIndexList, viewAspect, this);
}

void BaseTreeView::setRootIndex(const QModelIndex &index)
{
    QModelIndex oldIndex = rootIndex();
    if (oldIndex.isValid()) {
        model()->setData(oldIndex, QVariant(), FileSystemModel::DecreaseRefCounterRole);
        qDebug() << "REF COUNTERS" << oldIndex.data(FileSystemModel::PathRole) << oldIndex.data(FileSystemModel::RefCounterRole).toInt();
    }

    model()->setData(index, QVariant(), FileSystemModel::IncreaseRefCounterRole);
    qDebug() << "REF COUNTERS" << index.data(FileSystemModel::PathRole) << index.data(FileSystemModel::RefCounterRole).toInt();
    QTreeView::setRootIndex(index);
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
    if (!signalsQueue.size())
        return;

    mutex.lock();
    QList<QModelIndex> indexList = signalsQueue.keys();
    for (const QModelIndex &parent : indexList) {
        QMap<int, QModelIndex> *queue = signalsQueue.value(parent);

        QModelIndex topIndex     {};
        QModelIndex bottomIndex  {};

        int topRow               { -1 };
        int bottomRow            { -1 };

        QVector<int> roles;
        roles.append(Qt::DecorationRole);

        QList<int> keys = queue->keys();
        for (int row : keys) {

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

                topRow = row;
                topIndex = queue->value(row);

                bottomRow = row;
                bottomIndex = queue->value(row);
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

