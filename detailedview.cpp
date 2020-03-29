#include <QHeaderView>
#include <QDebug>

#include "detailedview.h"
#include "dateitemdelegate.h"

DetailedView::DetailedView(QWidget *parent) : QTreeView(parent)
{
    setRootIsDecorated(false);
    setFrameShape(QFrame::NoFrame);
    setSortingEnabled(true);
    setSelectionMode(QAbstractItemView::ExtendedSelection);

    // This should be configurable: row selection or item selection
    setSelectionBehavior(QAbstractItemView::SelectItems);

    DateItemDelegate *dateDelegate = new DateItemDelegate(this);
    setItemDelegateForColumn(FileSystemModel::Columns::LastChangeTime, dateDelegate);

    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &QTreeView::customContextMenuRequested, this, &DetailedView::contextMenuRequested);
}

void DetailedView::setModel(QAbstractItemModel *model)
{
    FileSystemModel *fileSystemModel = reinterpret_cast<FileSystemModel *>(model);

    connect(fileSystemModel, &FileSystemModel::fetchStarted, this, &DetailedView::setBusyCursor);
    connect(fileSystemModel, &FileSystemModel::fetchFinished, this, &DetailedView::setNormalCursor);

    QTreeView::setModel(model);

    // Test default settings
    this->header()->setMinimumSectionSize(100);
    this->header()->resizeSection(FileSystemModel::Columns::Name, 600);
    this->header()->resizeSection(FileSystemModel::Columns::Extension, 100);
    this->header()->resizeSection(FileSystemModel::Columns::LastChangeTime, 230);
    this->header()->setStretchLastSection(false);
    this->header()->setSortIndicator(FileSystemModel::Columns::Extension, Qt::SortOrder::AscendingOrder);
}

void DetailedView::setRoot(QString root)
{
    if (!root.isEmpty()) {

        // This item is from the tree view model, not from the model of this view
        FileSystemModel *fileSystemModel = static_cast<FileSystemModel *>(model());
        if (!(fileSystemModel->getRoot()->getPath() == root)) {
            qDebug() << "DetailedView::setRoot" << root;
            fileSystemModel->setRoot(root);
        } else
            qDebug() << "DetailedView::setRoot this view's root is already" << root;
        }
}

void DetailedView::setNormalCursor()
{
    setCursor(Qt::ArrowCursor);
}

void DetailedView::setBusyCursor()
{
    setCursor(Qt::BusyCursor);
}

void DetailedView::contextMenuRequested(const QPoint &pos)
{
    QList<FileSystemItem *> fileSystemItems;
    ContextMenu::ContextViewAspect viewAspect;

    // Get current selected item if any
    QModelIndex index = indexAt(pos);
    if (index.isValid() && index.internalPointer() != nullptr) {
        viewAspect = ContextMenu::Selection;
        for (QModelIndex selectedIndex : selectedIndexes()) {
            if (selectedIndex.column() == 0 && selectedIndex.internalPointer() != nullptr) {
                FileSystemItem *fileSystemItem = static_cast<FileSystemItem *>(selectedIndex.internalPointer());
                fileSystemItems.append(fileSystemItem);
                qDebug() << "DetailedView::contextMenuRequested selected for " << fileSystemItem->getPath();
            }
        }
    } else {
        viewAspect = ContextMenu::Background;
        FileSystemModel *fileSystemModel = static_cast<FileSystemModel *>(model());
        FileSystemItem *fileSystemItem = fileSystemModel->getRoot();
        fileSystemItems.append(fileSystemItem);
        qDebug() << "DetailedView::contextMenuRequested selected for the background on " << fileSystemItem->getPath();
    }

    emit contextMenuRequestedForItems(mapToGlobal(pos), fileSystemItems, viewAspect);
}
