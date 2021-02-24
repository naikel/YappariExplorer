#include "customtabwidget.h"

#include <QHeaderView>
#include <QDebug>

#include "customtabbar.h"
#include "detailedview.h"

#include "Settings/Settings.h"

CustomTabWidget::CustomTabWidget(int pane, QWidget *parent) : QTabWidget(parent)
{
    this->pane = pane;
    setAcceptDrops(true);
    CustomTabBar *customTabBar = new CustomTabBar(this);
    setTabBar(customTabBar);

    connect(customTabBar, &CustomTabBar::newTabClicked, this, &CustomTabWidget::newTabClicked);
    connect(customTabBar, &CustomTabBar::tabBarDoubleClicked, this, &CustomTabWidget::closeTab);
}

void CustomTabWidget::initialize()
{
    int tabsCount = Settings::settings->readPaneSetting(pane, SETTINGS_PANES_TABSCOUNT).toInt();
    if (tabsCount > 0) {
        for (int tab = 0; tab < tabsCount; tab++) {
            QString path = Settings::settings->readTabSetting(pane, tab, SETTINGS_TABS_PATH).toString();
            addNewTab(path);
            int pos = count() - 2;
            DetailedView *detailedView = reinterpret_cast<DetailedView *>(widget(pos));
            QHeaderView *header = detailedView->header();

            int indicatorSection = Settings::settings->readTabSetting(pane, tab, SETTINGS_TABS_SORTCOLUMN).toInt();
            bool isAscending = Settings::settings->readTabSetting(pane, tab, SETTINGS_TABS_SORTASCENDING).toBool();
            header->setSortIndicator(indicatorSection, isAscending ? Qt::AscendingOrder : Qt::DescendingOrder);

            QJsonArray columns = Settings::settings->readTabSetting(pane, tab, SETTINGS_COLUMNS).toArray();
            for (int section = 0; section < columns.size() ; section++) {
                int sectionSize = Settings::settings->readColumnSetting(pane, tab, section, SETTINGS_COLUMNS_WIDTH).toInt();
                header->resizeSection(section, sectionSize);

                int visualIndex = Settings::settings->readColumnSetting(pane, tab, section, SETTINGS_COLUMNS_VISUALINDEX).toInt();
                header->moveSection(section, visualIndex);
            }
        }

        int currentTab = Settings::settings->readPaneSetting(pane, SETTINGS_PANES_CURRENTTAB).toInt();
        setCurrentIndex(currentTab);
    } else
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

    connect(detailedView, &DetailedView::viewFocus, this, &CustomTabWidget::tabFocus);
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

void CustomTabWidget::saveSettings(int pane)
{
    Settings::settings->newTabSetting(pane);
    int tabsCount = count() - 1;
    Settings::settings->savePaneSetting(pane, SETTINGS_PANES_TABSCOUNT, tabsCount);
    Settings::settings->savePaneSetting(pane, SETTINGS_PANES_CURRENTTAB, currentIndex());
    for (int tab = 0; tab < tabsCount; tab++) {
        const DetailedView *detailedView = static_cast<DetailedView *>(widget(tab));

        FileSystemModel *fileSystemModel = static_cast<FileSystemModel *>(detailedView->model());
        if (fileSystemModel != nullptr) {
            Settings::settings->saveTabSetting(pane, tab, SETTINGS_TABS_PATH, fileSystemModel->getRoot()->getPath());
            Settings::settings->saveTabSetting(pane, tab, SETTINGS_TABS_SORTCOLUMN, detailedView->header()->sortIndicatorSection());
            Settings::settings->saveTabSetting(pane, tab, SETTINGS_TABS_SORTASCENDING, (detailedView->header()->sortIndicatorOrder() == Qt::AscendingOrder));

            QHeaderView *header = detailedView->header();
            for (int section = 0; section< header->count() ; section++) {
                Settings::settings->saveColumnSetting(pane, tab, section, SETTINGS_COLUMNS_WIDTH, header->sectionSize(section));
                Settings::settings->saveColumnSetting(pane, tab, section, SETTINGS_COLUMNS_VISUALINDEX, header->visualIndex(section));
            }
        }
    }
}

bool CustomTabWidget::setViewRootIndex(const QModelIndex &index)
{
    qDebug() << "CustomTabWidget::setViewRootIndex";
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
                FileSystemModel *fileSystemModel = detailedView->getFileSystemModel();
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

void CustomTabWidget::refreshCurrentTab()
{
    DetailedView *detailedView = static_cast<DetailedView *>(currentWidget());
    FileSystemModel *fileSystemModel = detailedView->getFileSystemModel();
    fileSystemModel->refresh(fileSystemModel->getRoot()->getPath());
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
