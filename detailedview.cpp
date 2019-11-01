#include <QHeaderView>
#include <QDebug>

#include "detailedview.h"
#include "filesystemmodel.h"

DetailedView::DetailedView(QWidget *parent) : QTreeView(parent)
{
    setRootIsDecorated(false);
    setFrameShape(QFrame::NoFrame);
    setSortingEnabled(true);
    this->header()->setSortIndicator(0, Qt::AscendingOrder);

}

void DetailedView::setModel(QAbstractItemModel *model)
{
    FileSystemModel *fileSystemModel = reinterpret_cast<FileSystemModel *>(model);

    connect(fileSystemModel, &FileSystemModel::fetchStarted, this, &DetailedView::setBusyCursor);
    connect(fileSystemModel, &FileSystemModel::fetchFinished, this, &DetailedView::setNormalCursor);

    QTreeView::setModel(model);

    // Test default settings
    this->header()->resizeSection(FileSystemModel::Columns::Name,600);
    this->header()->resizeSection(FileSystemModel::Columns::Extension,30);
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
