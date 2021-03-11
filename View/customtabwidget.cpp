#include <QApplication>
#include <QHeaderView>
#include <QBuffer>
#include <QDebug>

#include "customtabbar.h"
#include "detailedview.h"

#include "Model/SortModel.h"

#include "Settings/Settings.h"

#include "customtabwidget.h"

CustomTabWidget::CustomTabWidget(int pane, QWidget *parent) : QTabWidget(parent)
{
    this->pane = pane;
    setAcceptDrops(true);
    CustomTabBar *customTabBar = new CustomTabBar(this);
    setTabBar(customTabBar);

    connect(customTabBar, &CustomTabBar::newTabClicked, this, &CustomTabWidget::copyCurrentTab);
    connect(customTabBar, &CustomTabBar::tabBarDoubleClicked, this, &CustomTabWidget::closeTab);
}

void CustomTabWidget::initialize(QAbstractItemModel *model)
{
    this->model = reinterpret_cast<FileSystemModel *>(model);

    connect(this->model, &FileSystemModel::dataChanged, this, &CustomTabWidget::modelDataChanged);
    connect(this->model, &FileSystemModel::fetchFinished, this, &CustomTabWidget::updateTab);


    int tabsCount = Settings::settings->readPaneSetting(pane, SETTINGS_PANES_TABSCOUNT).toInt();
    if (tabsCount > 0) {
        for (int tab = 0; tab < tabsCount; tab++) {
            QString path = Settings::settings->readTabSetting(pane, tab, SETTINGS_TABS_PATH).toString();
            QString displayName = Settings::settings->readTabSetting(pane, tab, SETTINGS_TABS_DISPLAYNAME).toString();
            QIcon icon = base64ToIcon(Settings::settings->readTabSetting(pane, tab, SETTINGS_TABS_ICON).toString());
            addNewTab(path, displayName, icon);
            int pos = count() - 2;
            DetailedView *detailedView = reinterpret_cast<DetailedView *>(widget(pos));

            detailedView->setSortingColumn(Settings::settings->readTabSetting(pane, tab, SETTINGS_TABS_SORTCOLUMN).toInt());
            detailedView->setSortOrder(Settings::settings->readTabSetting(pane, tab, SETTINGS_TABS_SORTASCENDING).toBool() ? Qt::AscendingOrder : Qt::DescendingOrder);

            QJsonArray columns = Settings::settings->readTabSetting(pane, tab, SETTINGS_COLUMNS).toArray();
            QList<int> columnsWidth;
            QList<int> visualIndexes;

            for (int section = 0; section < columns.size() ; section++) {
                columnsWidth.append(Settings::settings->readColumnSetting(pane, tab, section, SETTINGS_COLUMNS_WIDTH).toInt());
                visualIndexes.append(Settings::settings->readColumnSetting(pane, tab, section, SETTINGS_COLUMNS_VISUALINDEX).toInt());
            }

            detailedView->setColumnsWidth(columnsWidth);
            detailedView->setVisualIndexes(visualIndexes);
        }

        int currentTab = Settings::settings->readPaneSetting(pane, SETTINGS_PANES_CURRENTTAB).toInt();
        setCurrentIndex(currentTab);
    } else
        addNewTab(model->index(0, 0, QModelIndex()));

}

/*
 * TO DELETE
 */
void CustomTabWidget::addNewTab(const QString path)
{
    DetailedView *detailedView = new DetailedView(this);
    FileSystemModel *fileSystemModel = new FileSystemModel(FileInfoRetriever::List, detailedView);
    if (path.isNull())
        fileSystemModel->setDefaultRoot();
    else
        fileSystemModel->setRoot(path);

    //connect(detailedView, &DetailedView::viewFocus, this, &CustomTabWidget::tabFocus);
    connect(detailedView, &DetailedView::doubleClicked, this, &CustomTabWidget::doubleClicked);
    connect(detailedView, &DetailedView::rootChanged, this, &CustomTabWidget::rootChanged);
    connect(fileSystemModel, &FileSystemModel::fetchFinished, this, &CustomTabWidget::updateTab);
    connect(fileSystemModel, &FileSystemModel::fetchFailed, this, &CustomTabWidget::tabFailed);

    // Context menu
    connect(detailedView, &DetailedView::contextMenuRequestedForItems, this, &CustomTabWidget::emitContextMenu);

    detailedView->setModel(fileSystemModel);

    int pos = count() - 1;
    insertTab(pos, detailedView, fileSystemModel->getRoot()->getDisplayName());
    setTabIcon(pos, fileSystemModel->getRoot()->getIcon());
    setCurrentIndex(pos);
}

/*
 * TO DELETE
 */
void CustomTabWidget::addNewTab(const QModelIndex &sourceIndex)
{
    DetailedView *detailedView = new DetailedView(this);

    SortModel *sortModel = new SortModel(detailedView);
    sortModel->setSourceModel(model);
    sortModel->setObjectName("SortModel");

    connect(detailedView, &DetailedView::doubleClicked, this, &CustomTabWidget::viewIndexSelected);
    connect(detailedView, &DetailedView::rootIndexChanged, [=](const QModelIndex &index) { setTabName(currentIndex()); emit this->viewIndexChanged(index); emit folderFocusIndex(index); });
    connect(detailedView, &DetailedView::viewFocusIndex, [=](const QModelIndex &index) { emit this->folderFocusIndex(index); });

    // Context menu
    connect(detailedView, &DetailedView::contextMenuRequestedForItems, this, &CustomTabWidget::emitContextMenu);

    detailedView->setModel(sortModel);
    detailedView->setRootIndex(sortModel->mapFromSource(sourceIndex));

    int pos = count() - 1;
    insertTab(pos, detailedView, sourceIndex.data(Qt::DisplayRole).toString());
    setTabIcon(pos, sourceIndex.data(Qt::DecorationRole).value<QIcon>());
    setCurrentIndex(pos);
}

void CustomTabWidget::addNewTab(const QString path, const QString displayName, const QIcon icon)
{
    qDebug() << "CustomTabWidget::addNewTab" << path << displayName;

    DetailedView *detailedView = new DetailedView(this);

    detailedView->setPath(path);

    int pos = count() - 1;
    insertTab(pos, detailedView, icon, displayName);
}

void CustomTabWidget::copyCurrentTab()
{
    qDebug() << "CustomTabWidget::copyCurrentTab";

    DetailedView *sourceView = static_cast<DetailedView *>(currentWidget());
    DetailedView *newView = new DetailedView(this);

    sourceView->storeCurrentSettings();

    newView->setSortOrder(sourceView->getSortOrder());
    newView->setSortingColumn(sourceView->getSortingColumn());
    newView->setVisualIndexes(sourceView->getVisualIndexes());
    newView->setColumnsWidth(sourceView->getColumnsWidth());

    QSortFilterProxyModel *viewModel = reinterpret_cast<QSortFilterProxyModel *>(sourceView->model());
    QModelIndex sourceIndex = viewModel->mapToSource(sourceView->rootIndex());

    if (sourceIndex.isValid()) {

        int pos = count() - 1;
        insertTab(pos, newView, sourceIndex.data(Qt::DisplayRole).toString());
        setUpTab(pos, sourceIndex);
        setCurrentIndex(pos);

    } else {
        qDebug() << "CustomTabWidget::copyCurrentTab failed to obtain current tab index";
        newView->deleteLater();
    }
}

void CustomTabWidget::setUpTab(int tab, const QModelIndex &sourceIndex)
{
    qDebug() << "CustomTabWidget::setUpCurrentTab" << sourceIndex;

    DetailedView *detailedView = static_cast<DetailedView *>(widget(tab));

    SortModel *sortModel = new SortModel(detailedView);
    sortModel->setSourceModel(model);
    sortModel->setObjectName("SortModel");

    connect(detailedView, &DetailedView::doubleClicked, this, &CustomTabWidget::viewIndexSelected);
    connect(detailedView, &DetailedView::rootIndexChanged, [=](const QModelIndex &index) { setTabName(currentIndex()); emit this->viewIndexChanged(index); emit folderFocusIndex(index); });
    connect(detailedView, &DetailedView::viewFocusIndex, [=](const QModelIndex &index) { emit this->folderFocusIndex(index); });

    // Context menu
    connect(detailedView, &DetailedView::contextMenuRequestedForItems, this, &CustomTabWidget::emitContextMenu);

    detailedView->setModel(sortModel);
    detailedView->setRootIndex(sortModel->mapFromSource(sourceIndex));

    QHeaderView *header = detailedView->header();

    header->setSortIndicator(detailedView->getSortingColumn(), detailedView->getSortOrder());

    QList<int> columnsWidth = detailedView->getColumnsWidth();
    QList<int> visualIndexes = detailedView->getVisualIndexes();

    for (int section = 0; section < columnsWidth.size() ; section++) {
        header->resizeSection(section, columnsWidth[section]);
        header->moveSection(section, visualIndexes[section]);
    }

    setTabName(tab);
}

void CustomTabWidget::saveSettings(int pane)
{
    Settings::settings->newTabSetting(pane);
    int tabsCount = count() - 1;
    Settings::settings->savePaneSetting(pane, SETTINGS_PANES_TABSCOUNT, tabsCount);
    Settings::settings->savePaneSetting(pane, SETTINGS_PANES_CURRENTTAB, currentIndex());
    for (int tab = 0; tab < tabsCount; tab++) {
        const BaseTreeView *view = static_cast<BaseTreeView *>(widget(tab));
        QModelIndex index = view->rootIndex();

        if (index.isValid()) {
            Settings::settings->saveTabSetting(pane, tab, SETTINGS_TABS_PATH, index.data(FileSystemModel::PathRole).toString());
            Settings::settings->saveTabSetting(pane, tab, SETTINGS_TABS_DISPLAYNAME, index.data(Qt::DisplayRole).toString());
            Settings::settings->saveTabSetting(pane, tab, SETTINGS_TABS_ICON, iconToBase64(index.data(Qt::DecorationRole).value<QIcon>()));
        } else {
            Settings::settings->saveTabSetting(pane, tab, SETTINGS_TABS_PATH, view->getPath());
            Settings::settings->saveTabSetting(pane, tab, SETTINGS_TABS_DISPLAYNAME, tabText(tab));
            Settings::settings->saveTabSetting(pane, tab, SETTINGS_TABS_ICON, iconToBase64(tabIcon(tab)));
        }

        Settings::settings->saveTabSetting(pane, tab, SETTINGS_TABS_SORTCOLUMN, view->header()->sortIndicatorSection());
        Settings::settings->saveTabSetting(pane, tab, SETTINGS_TABS_SORTASCENDING, (view->header()->sortIndicatorOrder() == Qt::AscendingOrder));

        QHeaderView *header = view->header();
        for (int section = 0; section< header->count() ; section++) {
            Settings::settings->saveColumnSetting(pane, tab, section, SETTINGS_COLUMNS_WIDTH, header->sectionSize(section));
            Settings::settings->saveColumnSetting(pane, tab, section, SETTINGS_COLUMNS_VISUALINDEX, header->visualIndex(section));
        }
    }
}

bool CustomTabWidget::setViewRootIndex(const QModelIndex &index)
{
    if (index.isValid()) {
        DetailedView *detailedView = static_cast<DetailedView *>(currentWidget());
        if (detailedView->rootIndex() != index) {
            qDebug() << "CustomTabWidget::setViewRootIndex";

            QModelIndex proxyIndex = reinterpret_cast<SortModel*>(detailedView->model())->mapFromSource(index);

            // Clear selection
            detailedView->clearSelection();
            detailedView->setRootIndex(proxyIndex);

            emit viewIndexChanged(index);

            setTabName(currentIndex());

            emit folderFocusIndex(proxyIndex);
            return true;
        }
    }

    return false;
}

/*
 * TO DELETE
 */
bool CustomTabWidget::setViewRootPath(const QString &path)
{
    qDebug() << "CustomTabWidget::setViewRootPath new path " << path;
    DetailedView *detailedView = static_cast<DetailedView *>(currentWidget());
    return detailedView->setRoot(path);
}

/*
 * TO DELETE
 */
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

void CustomTabWidget::viewIndexSelected(const QModelIndex &index)
{
    qDebug() << "CustomTabWidget::viewIndexSelected";
    if (index.isValid()) {
        const SortModel *model = reinterpret_cast<const SortModel *>(index.model());
        QModelIndex sourceIndex = model->mapToSource(index);
        if (!sourceIndex.data(FileSystemModel::FileRole).toBool()) {
            if (setViewRootIndex(sourceIndex)) {
            //    emit viewIndexChanged(sourceIndex);
            }
        } else {
            emit defaultActionRequestedForIndex(sourceIndex);
        }
    }
}

void CustomTabWidget::updateTab()
{
    qDebug() << "CustomTabWidget::updateTab";
    QAbstractItemView *view = static_cast<QAbstractItemView *>(currentWidget());
    view->scrollToTop();
    qDebug() << "CustomTabWidget::updateTab tab updated";
}

void CustomTabWidget::setTabName(int index)
{
    QAbstractItemView *view = static_cast<QAbstractItemView *>(widget(index));
    setTabText(index, view->rootIndex().data(Qt::DisplayRole).toString());
    setTabIcon(index, view->rootIndex().data(Qt::DecorationRole).value<QIcon>());
}

/*
 * TODO: DELETE
 */
void CustomTabWidget::tabFailed(qint32 err, QString errMessage)
{
    Q_UNUSED(err)
    Q_UNUSED(errMessage)
    qDebug() << "CustomTabWidget::tabFailed";

    DetailedView *detailedView = static_cast<DetailedView *>(currentWidget());
    FileSystemModel *fileSystemModel = static_cast<FileSystemModel *>(detailedView->model());
    emit rootChangeFailed(fileSystemModel->getRoot()->getPath());
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
        // TODO: Implement active tab history
        if (tabBar()->currentIndex() == count() - 1)
            tabBar()->setCurrentIndex(count() - 2);
    }
}

void CustomTabWidget::modelDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles)
{
    if (topLeft.isValid() && topLeft == bottomRight && roles.contains(Qt::DisplayRole)) {
        qDebug() << "CustomTabWidget::modelDataChanged";
        for (int tab = 0; tab < count() - 1; tab++) {
            QString path = topLeft.data(FileSystemModel::PathRole).toString();
            QAbstractItemView *view = static_cast<QAbstractItemView *>(widget(tab));
            if (view != nullptr && view->rootIndex().data(FileSystemModel::PathRole).toString() == path)
                setTabName(tab);
        }
    }
}

/*
 * TODO: DELETE
 */
void CustomTabWidget::tabFocus(FileSystemItem *item)
{
    emit folderFocus(item);
}

void CustomTabWidget::refreshCurrentTab()
{
    QAbstractItemView *view= static_cast<QAbstractItemView *>(currentWidget());
    view->model()->setData(view->rootIndex(), false, FileSystemModel::AllChildrenFetchedRole);
}

void CustomTabWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    Q_UNUSED(event)

    qDebug() << "CustomTabWidget::mouseDoubleClickEvent open new tab";
    emit newTabRequested();
}

void CustomTabWidget::emitContextMenu(const QPoint &pos, const QModelIndexList &indexList,
                                      const ContextMenu::ContextViewAspect viewAspect, QAbstractItemView *view)
{
    emit contextMenuRequestedForItems(pos, indexList, viewAspect, view);
}

/*
 * TODO: DELETE
 */
void CustomTabWidget::rootChanged(QString path)
{
    // This is a root change through history navigation so it is not relative to the parent
    // it can be anywhere in the file system
    // so we call the absolute change instead of the relative one
    emit rootAbsoluteChange(path);
}

QString CustomTabWidget::iconToBase64(QIcon icon)
{
   int iconSize = QApplication::style()->pixelMetric(QStyle::PM_SmallIconSize);
   QPixmap pixmap = icon.pixmap(iconSize, iconSize);

   QByteArray bytes;
   QBuffer buffer(&bytes);
   buffer.open(QIODevice::WriteOnly);
   pixmap.save(&buffer, "PNG");

   return bytes.toBase64();
}

QIcon CustomTabWidget::base64ToIcon(QString base64)
{
    QPixmap pixmap;
    if (pixmap.loadFromData(QByteArray::fromBase64(base64.toLatin1()))) {

        QIcon icon;
        icon.addPixmap(pixmap);

        return icon;

    } else {
        return model->getFolderIcon();
    }
}
