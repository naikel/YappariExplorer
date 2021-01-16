#include "customtabwidget.h"

#include <QHeaderView>
#include <QDebug>

#include "customtabbar.h"
#include "detailedview.h"

CustomTabWidget::CustomTabWidget(QWidget *parent) : QTabWidget(parent)
{
    setAcceptDrops(true);
    CustomTabBar *customTabBar = new CustomTabBar(this);
    setTabBar(customTabBar);

    connect(customTabBar, &CustomTabBar::newTabClicked, this, &CustomTabWidget::newTabClicked);
    connect(customTabBar, &CustomTabBar::tabBarDoubleClicked, this, &CustomTabWidget::closeTab);

    addNewTab(QString());
}

void CustomTabWidget::addNewTab(const QString path)
{
    DetailedView *detailedView = new DetailedView(this);
    FileSystemModel *fileSystemModel = new FileSystemModel(FileInfoRetriever::List, detailedView);
    if (path.isNull())
        fileSystemModel->setDefaultRoot();
    else
        fileSystemModel->setRoot(path);

    detailedView->setModel(fileSystemModel);

    connect(detailedView, &DetailedView::viewFocus, this, &CustomTabWidget::tabFocus);
    connect(detailedView, &DetailedView::doubleClicked, this, &CustomTabWidget::doubleClicked);
    connect(detailedView, &DetailedView::rootChanged, this, &CustomTabWidget::rootChanged);
    connect(fileSystemModel, &FileSystemModel::fetchFinished, this, &CustomTabWidget::updateTab);
    connect(fileSystemModel, &FileSystemModel::fetchFailed, this, &CustomTabWidget::tabFailed);

    // Context menu
    connect(detailedView, &DetailedView::contextMenuRequestedForItems, this, &CustomTabWidget::emitContextMenu);

    int pos = tabBar()->count() - 1;
    insertTab(pos, detailedView, fileSystemModel->getRoot()->getDisplayName());
    setTabIcon(pos, fileSystemModel->getRoot()->getIcon());
    setCurrentIndex(pos);
}

bool CustomTabWidget::setViewRootIndex(const QModelIndex &index)
{
    qDebug() << "CustomTabWidget::setViewIndex";
    if (index.isValid() && index.internalPointer() != nullptr) {
        FileSystemItem *fileSystemItem = static_cast<FileSystemItem*>(index.internalPointer());
        return setViewRootPath(fileSystemItem->getPath());
    }

    return false;
}

bool CustomTabWidget::setViewRootPath(const QString &path)
{
    qDebug() << "CustomTabWidget::setViewRootPath new path " << path;
    DetailedView *detailedView = static_cast<DetailedView *>(currentWidget());
    return detailedView->setRoot(path);
}

void CustomTabWidget::doubleClicked(const QModelIndex &index)
{
    qDebug() << "CustomTabWidget::doubleClicked";
    if (index.isValid() && index.column() == 0 && index.internalPointer() != nullptr) {
        FileSystemItem *fileSystemItem = static_cast<FileSystemItem*>(index.internalPointer());
        if (fileSystemItem->isDrive() || fileSystemItem->isFolder()) {
            QString path = fileSystemItem->getPath();

            // This will delete (free) the fileSystemItem of index and will be no longer valid
            if (setViewRootIndex(index))
                emit rootRelativeChange(path);
        } else {
            emit defaultActionRequestedForItem(fileSystemItem);
        }
    }
}

void CustomTabWidget::updateTab()
{
    qDebug() << "CustomTabWidget::updateTab";
    DetailedView *detailedView = static_cast<DetailedView *>(currentWidget());
    FileSystemModel *fileSystemModel = static_cast<FileSystemModel *>(detailedView->model());
    nameTab(currentIndex(), fileSystemModel->getRoot());
    detailedView->scrollToTop();
    qDebug() << "CustomTabWidget::updateTab tab updated. " << fileSystemModel->getRoot()->childrenCount() << " items";
    qDebug() << "---------------------------------------------------------------------------------------------------";

    emit viewModelChanged(fileSystemModel);
}

void CustomTabWidget::nameTab(int index, FileSystemItem *item)
{
    setTabText(index, item->getDisplayName());
    setTabIcon(index, item->getIcon());
}


void CustomTabWidget::tabFailed(qint32 err, QString errMessage)
{
    Q_UNUSED(err)
    Q_UNUSED(errMessage)
    qDebug() << "CustomTabWidget::tabFailed";

    DetailedView *detailedView = static_cast<DetailedView *>(currentWidget());
    FileSystemModel *fileSystemModel = static_cast<FileSystemModel *>(detailedView->model());
    emit rootChangeFailed(fileSystemModel->getRoot()->getPath());
}

void CustomTabWidget::newTabClicked()
{
    emit newTabRequested();
}

void CustomTabWidget::closeTab(int index)
{
    int tabCount = count();
    if (tabCount > 2 && index != (tabCount - 1)) {
        DetailedView *detailedView = static_cast<DetailedView *>(widget(index));
        removeTab(index);
        detailedView->deleteLater();

        // This will make the current tab the last one created
        // Eventually this should be changed to the last one active, when history is implemented
        if (tabBar()->currentIndex() == count() - 1)
            tabBar()->setCurrentIndex(count() - 2);
    }
}

void CustomTabWidget::displayNameChanged(QString oldPath, FileSystemItem *item)
{
    if (item != nullptr) {
        for (int tab = 0; tab < count() - 1; tab++) {
            DetailedView *detailedView = static_cast<DetailedView *>(widget(tab));
            if (detailedView != nullptr) {
                FileSystemModel *fileSystemModel = static_cast<FileSystemModel *>(detailedView->model());
                FileSystemItem *root = fileSystemModel->getRoot();
                if (root->getPath() == oldPath) {

                    root->setPath(item->getPath());
                    root->setDisplayName(item->getDisplayName());
                    root->setIcon(item->getIcon());

                    nameTab(tab, item);
                }
            }
        }
    }
}

void CustomTabWidget::tabFocus(FileSystemItem *item)
{
    emit folderFocus(item);
}

void CustomTabWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    Q_UNUSED(event)

    qDebug() << "CustomTabWidget::mouseDoubleClickEvent open new tab";
    emit newTabRequested();
}

void CustomTabWidget::emitContextMenu(const QPoint &pos, const QList<FileSystemItem *> fileSystemItems,
                                      const ContextMenu::ContextViewAspect viewAspect, QAbstractItemView *view)
{
    emit contextMenuRequestedForItems(pos, fileSystemItems, viewAspect, view);
}

void CustomTabWidget::rootChanged(QString path)
{
    // This is a root change through history navigation so it is not relative to the parent
    // it can be anywhere in the file system
    // so we call the absolute change instead of the relative one
    emit rootAbsoluteChange(path);
}
