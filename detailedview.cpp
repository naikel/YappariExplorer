#include <QHeaderView>

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
}

void DetailedView::setNormalCursor()
{
    setCursor(Qt::ArrowCursor);
}

void DetailedView::setBusyCursor()
{
    setCursor(Qt::BusyCursor);
}
