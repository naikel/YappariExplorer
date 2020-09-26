#include <QHeaderView>
#include <QMouseEvent>
#include <QDebug>

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
    // a FileSystemItem pointer, might be no longer valid since the model changed and all the FileSystemItem pointers
    // will be freed.

    // So basically this is the minimum reimplementation that does what we need it to do

    qDebug() << "DetailedView::mouseDoubleClickEvent";
    const QPersistentModelIndex persistent = indexAt(event->pos());
    if (persistent.isValid())
        emit doubleClicked(persistent);
}

void DetailedView::mousePressEvent(QMouseEvent *event)
{
    QModelIndex index = indexAt(event->pos());
    if (event->button() == Qt::LeftButton && (!index.isValid() || index.column() > 0)) {

        // Clear current selection
        clearSelection();

        // Position includes the header we have to skip it
        origin = event->pos();
        origin.setY(origin.y() + header()->height());

        if (!rubberBand)
            rubberBand = new QRubberBand(QRubberBand::Rectangle, this);
        rubberBand->setGeometry(QRect(origin, QSize()));
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
        destination.setY(destination.y() + header()->height());

        rubberBand->setGeometry(QRect(origin, destination).normalized());
        event->accept();
    } else
        BaseTreeView::mouseMoveEvent(event);
}

void DetailedView::mouseReleaseEvent(QMouseEvent *event)
{
    if (rubberBand != nullptr) {
        rubberBand->hide();
        // determine selection, for example using QRect::intersects()
        // and QRect::contains().
        rubberBand->deleteLater();
        rubberBand = nullptr;
        event->accept();
    } else
        BaseTreeView::mouseReleaseEvent(event);

}

