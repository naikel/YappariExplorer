#include <QHeaderView>
#include <QDebug>

#include "detailedview.h"
#include "filesystemmodel.h"
#include "dateitemdelegate.h"

DetailedView::DetailedView(QWidget *parent) : QTreeView(parent)
{
    setRootIsDecorated(false);
    setFrameShape(QFrame::NoFrame);
    setSortingEnabled(true);

    DateItemDelegate *dateDelegate = new DateItemDelegate(this);
    setItemDelegateForColumn(FileSystemModel::Columns::LastChangeTime, dateDelegate);
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

void DetailedView::setNormalCursor()
{
    setCursor(Qt::ArrowCursor);
}

void DetailedView::setBusyCursor()
{
    setCursor(Qt::BusyCursor);
}
