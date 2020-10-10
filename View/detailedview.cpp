#include <QHeaderView>
#include <QMouseEvent>
#include <QDebug>
#include <QScrollBar>

#include "detailedview.h"
#include "dateitemdelegate.h"

DetailedView::DetailedView(QWidget *parent) : BaseTreeView(parent)
{
    setRootIsDecorated(false);
    setFrameShape(QFrame::NoFrame);
    setSortingEnabled(true);
    setSelectionMode(QAbstractItemView::ExtendedSelection);

    // This should be configurable: row selection or item selection
    setSelectionBehavior(QAbstractItemView::SelectItems);

    DateItemDelegate *dateDelegate = new DateItemDelegate(this);
    setItemDelegateForColumn(FileSystemModel::Columns::LastChangeTime, dateDelegate);

    BaseItemDelegate *baseDelegate = new BaseItemDelegate(this);
    setItemDelegateForColumn(FileSystemModel::Columns::Name, baseDelegate);
    setItemDelegateForColumn(FileSystemModel::Columns::Extension, baseDelegate);
    setItemDelegateForColumn(FileSystemModel::Columns::Size, baseDelegate);
    setItemDelegateForColumn(FileSystemModel::Columns::Type, baseDelegate);
}

void DetailedView::initialize()
{
    this->header()->setMinimumSectionSize(100);
    this->header()->resizeSection(FileSystemModel::Columns::Name, 600);
    this->header()->resizeSection(FileSystemModel::Columns::Extension, 100);
    this->header()->resizeSection(FileSystemModel::Columns::LastChangeTime, 230);
    this->header()->setStretchLastSection(false);
    this->header()->setSortIndicator(FileSystemModel::Columns::Extension, Qt::SortOrder::AscendingOrder);
    BaseTreeView::initialize();
}

bool DetailedView::setRoot(QString root)
{
    if (!root.isEmpty()) {

        // This item is from the tree view model, not from the model of this view
        FileSystemModel *fileSystemModel = getFileSystemModel();
        if (!(fileSystemModel->getRoot()->getPath() == root)) {
            qDebug() << "DetailedView::setRoot" << root;
            return fileSystemModel->setRoot(root);
        } else {
            qDebug() << "DetailedView::setRoot this view's root is already" << root;
        }
    }

    return false;
}

void DetailedView::selectEvent()
{
    qDebug() << "DetailedView::selectEvent";
    for (QModelIndex selectedIndex : selectedIndexes()) {
        if (selectedIndex.column() == 0 && selectedIndex.internalPointer() != nullptr) {
            FileSystemItem *fileSystemItem = getFileSystemModel()->getFileSystemItem(selectedIndex);
            qDebug() << "DetailedView::selectEvent selected for " << fileSystemItem->getPath();
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
        if (persistent.isValid()) {
            event->accept();
            emit doubleClicked(persistent);
        }
    }
}

void DetailedView::mousePressEvent(QMouseEvent *event)
{
    QModelIndex index = indexAt(event->pos());
    if (event->button() == Qt::LeftButton && (!index.isValid() || index.column() > 0)) {

        if (event->modifiers() & Qt::ControlModifier) {
            command = QItemSelectionModel::Toggle;
        } else {
            command = QItemSelectionModel::Select;
            clearSelection();
        }
        currentSelection = selectionModel()->selection();

        // Position includes the header we have to skip it
        QPoint originPos = event->pos();
        origin = mapToViewport(originPos);

        originPos.setY(origin.y() + header()->height());

        if (!rubberBand)
            rubberBand = new QRubberBand(QRubberBand::Rectangle, this);
        rubberBand->setGeometry(QRect(originPos, QSize()));
        rubberBand->show();
        event->accept();
    } else
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

void DetailedView::setSelectionFromViewportRect(const QRect &rect, QItemSelection &currentSelection, QItemSelectionModel::SelectionFlags command)
{
    // The following lines are from the QTreeView implementation
    if (!selectionModel() || rect.isNull())
           return;

    QPoint tl(isRightToLeft() ? qMax(rect.left(), rect.right())
              : qMin(rect.left(), rect.right()), qMin(rect.top(), rect.bottom()));
    QPoint br(isRightToLeft() ? qMin(rect.left(), rect.right()) :
              qMax(rect.left(), rect.right()), qMax(rect.top(), rect.bottom()));

    int topRow = tl.y() / getDefaultRowHeight();
    int bottomRow =  br.y() / getDefaultRowHeight();

    if (topRow < 0)
        topRow = 0;

    int maxRow = model()->rowCount(QModelIndex()) - 1;
    if (bottomRow > maxRow)
        bottomRow = maxRow;

    QModelIndexList selectedIndexes;
    for (int row = topRow; row <= bottomRow; row++) {
        QModelIndex index = model()->index(row, 0, QModelIndex());
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
    for (QModelIndex index : selectedIndexes)
        selectionModel()->select(index, command);

}
