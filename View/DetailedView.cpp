#include <QSortFilterProxyModel>
#include <QHeaderView>
#include <QMouseEvent>
#include <QDebug>
#include <QScrollBar>

#include "DetailedView.h"
#include "DateItemDelegate.h"

DetailedView::DetailedView(QWidget *parent) : BaseTreeView(parent)
{
    setRootIsDecorated(false);
    setFrameShape(QFrame::NoFrame);
    setSortingEnabled(true);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setExpandsOnDoubleClick(false);
    setItemsExpandable(false);

    // This should be configurable: row selection or item selection
    setSelectionBehavior(QAbstractItemView::SelectItems);

    DateItemDelegate *dateDelegate = new DateItemDelegate(this);
    setItemDelegateForColumn(FileSystemModel::Columns::LastChangeTime, dateDelegate);

    BaseItemDelegate *baseDelegate = new BaseItemDelegate(this);
    setItemDelegateForColumn(FileSystemModel::Columns::Name, baseDelegate);
    setItemDelegateForColumn(FileSystemModel::Columns::Extension, baseDelegate);
    setItemDelegateForColumn(FileSystemModel::Columns::Size, baseDelegate);
    setItemDelegateForColumn(FileSystemModel::Columns::Type, baseDelegate);

    this->header()->setMinimumSectionSize(100);
    this->header()->setStretchLastSection(false);
}

void DetailedView::setModel(QAbstractItemModel *model)
{
    qDebug() << "DetailedView::setModel";

    BaseTreeView::setModel(model);

    if (history)
        delete history;

    history = new FileSystemHistory(model, this);

    this->header()->resizeSection(FileSystemModel::Columns::Name, 600);
    this->header()->resizeSection(FileSystemModel::Columns::Extension, 100);
    this->header()->resizeSection(FileSystemModel::Columns::LastChangeTime, 230);
    this->header()->setSortIndicator(FileSystemModel::Columns::Extension, Qt::SortOrder::AscendingOrder);
}

FileSystemHistory *DetailedView::getHistory()
{
    return history;
}

void DetailedView::setRootIndex(const QModelIndex &index)
{
    qDebug() << "DetailedView::setRootIndex";

    BaseTreeView::setRootIndex(index);

    if (index.isValid())
        history->insert(index.data(Qt::DisplayRole).toString(),
                        index.data(Qt::DecorationRole).value<QIcon>(),
                        index.data(FileSystemModel::PathRole).toString());
}

void DetailedView::selectEvent()
{
    qDebug() << "DetailedView::selectEvent";
    for (QModelIndex& selectedIndex : selectedIndexes()) {
        if (selectedIndex.column() == 0 && selectedIndex.internalPointer() != nullptr) {
            qDebug() << "DetailedView::selectEvent selected for " << selectedIndex.data(FileSystemModel::PathRole).toString();
            emit doubleClicked(selectedIndex);
            return;
        }
    }
}

void DetailedView::mouseDoubleClickEvent(QMouseEvent *event)
{
    // QTreeView implementation of this function calls QAbstractViewItem::edit on an index which the internal pointer,
    // a FileSystemItem pointer, might be no longer valid since after a double click the model is changed
    // (double-clicking on a folder to open it for example) and all the FileSystemItem pointers will be freed.

    // So basically this is the minimum reimplementation that does what we need it to do (not editing on double-click)

    if (event->button() == Qt::LeftButton) {
        qDebug() << "DetailedView::mouseDoubleClickEvent";
        const QPersistentModelIndex persistent = indexAt(event->pos());
        if (persistent.isValid() && persistent.column() == 0) {
            event->accept();
            emit doubleClicked(persistent);
        }
    }
}

void DetailedView::mousePressEvent(QMouseEvent *event)
{
    QModelIndex index = indexAt(event->pos());

    if (event->button() == Qt::LeftButton) {

        // If there was a click in this view tell AppWindow to update the titleBar
        emit viewFocusIndex(rootIndex());

        if (!index.isValid() || index.column() > 0) {

            if (event->modifiers() & Qt::ControlModifier) {
                command = QItemSelectionModel::Toggle;
            } else {
                command = QItemSelectionModel::Select;
                clearSelection();
            }
            currentSelection = selectionModel() != nullptr ? selectionModel()->selection() : QItemSelection();

            // Position includes the header we have to skip it
            QPoint originPos = event->pos();
            origin = mapToViewport(originPos);

            originPos.setY(origin.y() + header()->height());

            if (!rubberBand)
                rubberBand = new QRubberBand(QRubberBand::Rectangle, this);
            rubberBand->setGeometry(QRect(originPos, QSize()));
            rubberBand->show();
            event->accept();
            return;
        }
    }

    BaseTreeView::mousePressEvent(event);
}

void DetailedView::mouseMoveEvent(QMouseEvent *event)
{
    if (rubberBand != nullptr) {
        // Position includes the header we have to skip it
        QPoint destination = event->pos();

        int headerHeight = header()->height();
        destination.setY(destination.y() + headerHeight);

        if (destination.x() < 0) {
            // TODO: Left scroll
            destination.setX(0);
        }

        if (destination.y() < headerHeight) {
            destination.setY(headerHeight);
            QScrollBar *verticalBar = verticalScrollBar();
            verticalBar->setValue(verticalBar->value() - 1);
        }

        QRect viewPortRect = geometry();

        if (destination.y() > viewPortRect.height()) {
            destination.setY(viewPortRect.height());

            QScrollBar *verticalBar = verticalScrollBar();
            verticalBar->setValue(verticalBar->value() + 1);
        }

        if (destination.x() > viewPortRect.width()) {
            // TODO: Left scroll
            destination.setX(viewPortRect.width());
        }

        QPoint originPos = mapFromViewport(origin);

        if (originPos.y() < 0)
            originPos.setY(0);

        if (originPos.x() < 0)
            originPos.setX(0);

        if (originPos.y() > viewPortRect.height()) {
            originPos.setY(viewPortRect.height());
        }

        if (originPos.x() > viewPortRect.width()) {
            originPos.setX(viewPortRect.width());
        }
        originPos.setY(originPos.y() + header()->height());

        rubberBand->setGeometry(QRect(originPos, destination).normalized());
        setSelectionFromViewportRect(QRect(origin, mapToViewport(event->pos())).normalized(), currentSelection, command);
        event->accept();
    } else
        BaseTreeView::mouseMoveEvent(event);
}

void DetailedView::mouseReleaseEvent(QMouseEvent *event)
{
    if (rubberBand != nullptr) {
        rubberBand->hide();
        rubberBand->deleteLater();
        rubberBand = nullptr;
        event->accept();
    } else
        BaseTreeView::mouseReleaseEvent(event);

}

void DetailedView::backEvent()
{
    if (history->canGoBack()) {
        QString path = history->getLastItem();
        if (!path.isEmpty())
            emit selectPathInTree(path);
    }
}

void DetailedView::forwardEvent()
{
    if (history->canGoForward()) {
        QString path = history->getNextItem();
        if (!path.isEmpty())
            emit selectPathInTree(path);
    }
}

/*!
 * \brief Applies the selection \a flags to the items in or touched by the rectangle \a rect.
 * \param rect a QRect
 * \param command the selection flags
 *
 * The QAbstractItemView implementation assumes the items occupy the whole row, and then just looks for
 * the top left corner and the bottom right corner items.
 *
 * Since the items don't occupy the whole row in this view, most of the times the corners would point
 * to an invalid index.
 *
 * To solve this we modify the rectangle to be 1px at the left and 1px wide.  That ensures the rectangle
 * touches all the items.
 */
void DetailedView::setSelection(const QRect &rect, QItemSelectionModel::SelectionFlags command)
{
    QRect newRect = rect.normalized();

    newRect.adjust(1, 0, -newRect.width() + 1, 0);

    BaseTreeView::setSelection(newRect, command);
}

void DetailedView::setSelectionFromViewportRect(const QRect &rect, QItemSelection &currentSelection, QItemSelectionModel::SelectionFlags command)
{
    // The following lines are from the QTreeView implementation

    // There must be at least one child in this view otherwise return
    if (!selectionModel() || rect.isNull() || model()->rowCount(rootIndex()) <= 0)
        return;

    QPoint tl(isRightToLeft() ? qMax(rect.left(), rect.right())
              : qMin(rect.left(), rect.right()), qMin(rect.top(), rect.bottom()));
    QPoint br(isRightToLeft() ? qMin(rect.left(), rect.right()) :
              qMax(rect.left(), rect.right()), qMax(rect.top(), rect.bottom()));

    // This is why there must be at least one child
    int rh = rowHeight(model()->index(0, 0, rootIndex()));

    int topRow = tl.y() / rh;
    int bottomRow =  br.y() / rh;

    if (topRow < 0)
        topRow = 0;

    int maxRow = model()->rowCount(rootIndex()) - 1;
    if (bottomRow > maxRow)
        bottomRow = maxRow;

    QVector<QModelIndex> selectedIndexes;
    for (int row = topRow; row <= bottomRow; row++) {
        const QModelIndex& index = model()->index(row, 0, rootIndex());
        QRect indexRect = visualRect(index);
        QPoint topLeft = (isRightToLeft()) ? indexRect.topRight() : indexRect.topLeft();
        QPoint pos = mapToViewport(topLeft);
        QRect newRect = QRect(pos, QSize(indexRect.width(), indexRect.height()));
        if (newRect.intersects(rect)) {
            selectedIndexes.append(index);
        }
    }

    selectionModel()->clear();
    selectionModel()->select(currentSelection, command);
    for (QModelIndex& index : selectedIndexes)
        selectionModel()->select(index, command);
}
