/*
 * Copyright (C) 2019, 2020, 2021 Naikel Aparicio. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ''AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation
 * are those of the author and should not be interpreted as representing
 * official policies, either expressed or implied, of the copyright holder.
 */

#include <QSortFilterProxyModel>
#include <QHeaderView>
#include <QMessageBox>
#include <QSplitter>
#include <QLayout>
#include <QDebug>

#include "CustomExplorer.h"
#include "once.h"

#include <qt_windows.h>

CustomExplorer::CustomExplorer(int nExplorer, QWidget *parent) : QFrame(parent)
{
    setupGui(nExplorer);

    FileSystemModel *fileSystemModel = new FileSystemModel(FileInfoRetriever::Tree, this);
    fileSystemModel->setDefaultRoot();

    treeModel = new TreeModel(this);
    treeModel->setSourceModel(fileSystemModel);
    treeModel->setObjectName("FilterModel");

    model = fileSystemModel;
    Once::connect(fileSystemModel, &FileSystemModel::modelReset, this, &CustomExplorer::initialize);
    treeView->setModel(treeModel);
}

void CustomExplorer::initialize()
{
    qDebug() << "CustomExplorer::initialize";

    AppWindow *mainWindow = AppWindow::instance();

    tabWidget->initialize(model);

    // Handling of single selections
    connect(treeView, &CustomTreeView::clicked, this, &CustomExplorer::setViewRootIndex);
    connect(treeView, &CustomTreeView::collapsed, this, &CustomExplorer::collapseAndSelect);

    // Selections from Views
    connect(tabWidget, &CustomTabWidget::viewIndexChanged, this, &CustomExplorer::viewIndexChanged);
    connect(tabWidget, &CustomTabWidget::defaultActionRequestedForIndex, mainWindow, &AppWindow::defaultActionForIndex);

    // Context Menu
    connect(tabWidget, &CustomTabWidget::contextMenuRequestedForItems, mainWindow, &AppWindow::showContextMenu);
    connect(treeView, &CustomTreeView::contextMenuRequestedForItems, mainWindow, &AppWindow::showContextMenu);

    // Focus change (application title update)
    connect(treeView->selectionModel(), &QItemSelectionModel::currentChanged, [=](const QModelIndex &current, const QModelIndex &) { mainWindow->updateTitle(current); });
    connect(tabWidget, &CustomTabWidget::folderFocusIndex, mainWindow, &AppWindow::updateTitle);

    // Tab handling
    connect(tabWidget, &CustomTabWidget::currentChanged, this, &CustomExplorer::tabChanged);

    // Path Bar
    connect(pathBar, &PathBar::rootIndexChangeRequested, [=](const QModelIndex &sourceIndex) { tabWidget->setViewRootIndex(sourceIndex); });

    // Errors
    connect(treeModel, &QAbstractItemModel::dataChanged, [=](const QModelIndex &topLeft, const QModelIndex &, const QVector<int> &roles) {
        if (roles.contains(FileSystemModel::ErrorCodeRole))
            showError(topLeft);
    });

    // Select current path in TreeView
    BaseTreeView *view = reinterpret_cast<BaseTreeView *>(tabWidget->currentWidget());
    QSortFilterProxyModel *proxyModel = reinterpret_cast<QSortFilterProxyModel *>(view->model());

    if (view->rootIndex().isValid()) {
        QModelIndex sourceIndex = proxyModel->mapToSource(view->rootIndex());
        viewIndexChanged(sourceIndex);

        if (!sourceIndex.parent().isValid())
            treeView->expand(treeModel->mapFromSource(sourceIndex));
    } else {
        expandAndSelectAbsolute(view->getPath());
    }

    /*
     * TODO: DELETE
     */

    /*
    FileSystemModel *fileSystemModel = reinterpret_cast<FileSystemModel *>(model);
    pathBar->setTreeModel(fileSystemModel);
    */

    // Handling of single selections
    //connect(treeView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &CustomExplorer::treeViewSelectionChanged);
    //connect(treeView, &CustomTreeView::clicked, this, &CustomExplorer::setViewRootIndex);

    /*
    // Refresh in the Tree
    connect(treeView, &CustomTreeView::refreshed, tabWidget, &CustomTabWidget::refreshCurrentTab);

    // Focus change (application title update)
    connect(tabWidget, &CustomTabWidget::folderFocus, mainWindow, &AppWindow::updateTitle);
    connect(treeView, &CustomTreeView::viewFocus, mainWindow, &AppWindow::updateTitle);

    // Auto expand & select on view select / Auto tree select and view change on collapse
    connect(tabWidget, &CustomTabWidget::rootRelativeChange, this, &CustomExplorer::expandAndSelectRelative);
    connect(tabWidget, &CustomTabWidget::rootAbsoluteChange, this, &CustomExplorer::expandAndSelectAbsolute);
    connect(treeView, &CustomTreeView::collapsed, this, &CustomExplorer::collapseAndSelect);

    // Tab handling
    connect(tabWidget, &CustomTabWidget::newTabRequested, this, &CustomExplorer::newTabRequested);
    connect(tabWidget, &CustomTabWidget::currentChanged, this, &CustomExplorer::tabChanged);
    connect(fileSystemModel, &FileSystemModel::displayNameChanged, tabWidget, &CustomTabWidget::displayNameChanged);

    // Context Menu
    connect(tabWidget, &CustomTabWidget::contextMenuRequestedForItems, mainWindow, &AppWindow::showContextMenu);
    connect(treeView, &CustomTreeView::contextMenuRequestedForItems, mainWindow, &AppWindow::showContextMenu);

    // Actions
    connect(tabWidget, &CustomTabWidget::defaultActionRequestedForItem, mainWindow, &AppWindow::defaultAction);

    // Errors
    connect(tabWidget, &CustomTabWidget::rootChangeFailed, this, &CustomExplorer::rootChangeFailed);

    // Path Bar
    connect(tabWidget, &CustomTabWidget::viewModelChanged, pathBar, &PathBar::setTabModel);
    connect(pathBar, &PathBar::rootChange, this, &CustomExplorer::expandAndSelectAbsolute);

    */
}

void CustomExplorer::saveSettings() const
{
    tabWidget->saveSettings(nExplorer);

    Settings::settings->savePaneSetting(nExplorer, SETTINGS_PANES_TREEWIDTH, treeView->size().width());
}

bool CustomExplorer::treeViewSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    Q_UNUSED(deselected)

    qDebug() << "CustomExplorer::treeViewSelectionChanged";

    QModelIndexList indexes = selected.indexes();
    if (indexes.size() > 0) {

        const QModelIndex &treeIndex = indexes[0];

        const QModelIndex& source = treeModel->mapToSource(treeIndex);

        // TODO: PathBar would definitely prefer a FilterModel
        pathBar->selectTreeIndex(treeIndex);

        return tabWidget->setViewRootIndex(source);
    } else {
        // We need to select something, the parent

        indexes = deselected.indexes();
        if (indexes.size() > 0) {
            const QModelIndex& source = treeModel->mapToSource(indexes[0]).parent();
            treeView->setCurrentIndex(source);
        } else {
            // Last resort: root
            treeView->setCurrentIndex(treeModel->index(0, 0, QModelIndex()));
        }

        return false;
    }
}

/*
 * TODO: DELETE
 */

/*!
 * \brief Expands current tree item and selects the subfolder item with the new path.
 * \param path a new path that is a subfolder of the current path.
 *
 * Expands the current tree item and looks for the item with the new path inside of it and selects it.
 *
 * For this to work the new path has to be a subfolder of the current path, in other words
 * it has to be a relative of the current path.
 *
 * If the subfolder items have not been fetched yet, this function triggers a new fetch in
 * the background and it's called again after it finishes.
 *
 *  \sa CustomExplorer::expandAndSelectAbsolute
 */
void CustomExplorer::expandAndSelectRelative(QString path)
{
    qDebug() << "CustomExplorer::expandAndSelectRelative" << path;
    FileSystemModel *treeViewModel = reinterpret_cast<FileSystemModel *>(treeView->model());
    QModelIndex parent = treeView->selectedItem();
    if (parent.isValid()) {
        FileSystemItem *parentItem = static_cast<FileSystemItem*>(parent.internalPointer());
        qDebug() << "CustomExplorer::expandAndSelectRelative parent" << parentItem->getPath();
        if (!parentItem->areAllChildrenFetched()) {

            // Call this function again after all items are fetched
            qDebug() << "CustomExplorer::expandAndSelectRelative will be called again after fetch";
            Once::connect(treeViewModel, &FileSystemModel::fetchFinished, this, [this, path]() { this->expandAndSelectRelative(path); });

            // This will trigger the background fetching
            treeView->expand(parent);

        } else {

            qDebug() << "CustomExplorer::expandAndSelectRelative all children of" << parentItem->getPath() << "are fetched";

            if (!treeView->isExpanded(parent)) {
                treeView->expand(parent);
            }
            QModelIndex index = treeViewModel->relativeIndex(path, parent);

            if (index.isValid())
                treeView->setCurrentIndex(index);
            else {
                qDebug() << "CustomExplorer::expandAndSelectRelative can't find the index: the Tree View is not synchronized";

                // Try to add it?
                // treeViewModel->addPath(path);

                /*

                // TODO: What can we do here?

                // Remove all children from the Tree View
                treeViewModel->removeAllRows(parent);

                // Fetch again the children
                parentItem->setHasSubFolders(true);
                treeViewModel->fetchMore(parent);
                */

                // Call this function again when finished
                // TODO This might create an infinite cycle
                Once::connect(treeViewModel, &FileSystemModel::fetchFinished, this, [this, path]() { this->expandAndSelectRelative(path); });

            }
        }
    }
}

/*!
 * \brief Expands the current tree and selects the item with the new path.
 * \param path a new path that can be anywhere.
 *
 * Expands the tree until the item with the new path is visible and selects it.
 *
 * The path can be anywhere.
 *
 * If the subfolder items have not been fetched yet, this function triggers a new fetch in
 * the background and it's called again after it finishes.  This means this function can be
 * called several times until all children that are needed have been fetched.
 *
 * This function is only used when the user selects a tab that hasn't been loaded yet.
 *
 * \sa CustomExplorer::tabChanged
 */
void CustomExplorer::expandAndSelectAbsolute(QString path)
{
    if (path.isEmpty())
        return;

    qDebug() << "CustomExplorer::expandAndSelectAbsolute" << path;

    FileSystemModel *fileSystemModel = reinterpret_cast<FileSystemModel *>(model);

    QModelIndex parentIndex = fileSystemModel->index(0, 0, QModelIndex());
    QModelIndex childIndex {};

    // If path is root then just finish setting up the current tab and selecting root in the tree
    if (path == parentIndex.data(FileSystemModel::PathRole).toString()) {
        tabWidget->setUpTab(tabWidget->currentIndex(), parentIndex);
        QModelIndex parentTree = treeModel->mapFromSource(parentIndex);

        tabChanged(tabWidget->currentIndex());

        // Always expand root
        treeView->expand(parentTree);
        return;
    }

    QString absolutePath = QString();

#ifdef Q_OS_WIN
    if (path.left(3) == "::{") {
        absolutePath = parentIndex.data(FileSystemModel::PathRole).toString();

        if (path.size() > absolutePath.size() && path.left(absolutePath.size()) == absolutePath)
            path = path.mid(path.indexOf(fileSystemModel->separator()) + 1);
    }
#endif

    QStringList pathList = path.split(fileSystemModel->separator());
    for (auto folder : pathList) {
        if (!folder.isEmpty()) {

            // We need to check that the parent has all children fetched and it's expanded
            if (!parentIndex.data(FileSystemModel::AllChildrenFetchedRole).toBool()) {

                qDebug() << "CustomExplorer::expandAndSelectAbsolute will be called again after fetch";
                //Once::connect(fileSystemModel, &FileSystemModel::fetchFinished, this, [this, path]() { this->expandAndSelectAbsolute(path); });

                QObject *context = new QObject(this);
                connect(fileSystemModel, &FileSystemModel::dataChanged, context, [context, this, parentIndex, path](const QModelIndex &topLeft, const QModelIndex &, const QVector<int> &roles) {

                    if (topLeft == parentIndex && (roles.contains(FileSystemModel::AllChildrenFetchedRole) || roles.contains(FileSystemModel::ErrorCodeRole))) {

                        bool fetchFinished = topLeft.data(FileSystemModel::AllChildrenFetchedRole).toBool();

                        if (roles.contains(FileSystemModel::ErrorCodeRole) || fetchFinished)
                            delete context;

                        if (fetchFinished)
                            this->expandAndSelectAbsolute(path);
                    }
                });

                // This will trigger the background fetching
                treeView->expand(treeModel->mapFromSource(parentIndex));
                return;

            } else {
                if (!treeView->isExpanded(parentIndex)) {
                    treeView->expand(treeModel->mapFromSource(parentIndex));
                }

                // Parent has all children fetched and expanded at this point

                if (absolutePath.isEmpty())
                    absolutePath = folder + fileSystemModel->separator();
                else if (absolutePath.at(absolutePath.length() - 1) != fileSystemModel->separator())
                    absolutePath += fileSystemModel->separator() + folder;
                else
                    absolutePath += folder;

                childIndex = fileSystemModel->index(absolutePath);

                // This might happen if the path does not exist anymore
                // For example a tab loaded from settings and the path it points to does not exist anymore
                if (!childIndex.isValid()) {
                    qDebug() <<  "CustomExplorer::expandAndSelectAbsolute couldn't find the path" << absolutePath;

                    // TODO: Show some error to the user

                    return;
                }

               // Prepare for next iteration
                parentIndex = childIndex;
            }
        }
    }
    tabWidget->setUpTab(tabWidget->currentIndex(), childIndex);
    tabChanged(tabWidget->currentIndex());
}

/*!
 * \brief Collapse the tree and optionally selects the collapsed item.
 * \param index item that is going to be collapsed.
 *
 * Collapse the tree and selects the collapsed item.  The collapsed item is only selected if the current
 * selected item is not visible anymore.
 */
void CustomExplorer::collapseAndSelect(QModelIndex index)
{
    qDebug() << "CustomExplorer::collapseAndSelect";

    // TODO: freeChildren called from here (instead of the view) AFTER? selecting the collapsed index is better

    // Only change the selection if we're hiding an item
    if (index.isValid()) {
        if (treeView->selectedItem().isValid()) {
            for (QModelIndex i = treeView->selectedItem().parent(); i.isValid() ; i = i.parent()) {
                if (i == index) {
                    setViewRootIndex(index);
                    return;
                }
            }
        }
    }
}

/*
 * TO DELETE
 */

/*!
 * \brief Creates a new tab after a user's request.
 *
 * Creates a new tab after a request was received usually because the user clicked on the add new tab icon.
 */
void CustomExplorer::newTabRequested()
{
    QModelIndex index = treeView->selectedItem();
    if (index.isValid())
        tabWidget->addNewTab(treeModel->mapToSource(index));
}

/*!
 * \brief Selects the new folder after the user changed to a different tab
 * \param index index of the newly tab selected
 *
 * Tells the Tree View to select the folder of the tab selected.  This function will call
 * CustomExplorer::expandAndSelectAbsolute to look for the folder in the tree and select it.
 *
 * \sa CustomExplorer::expandAndSelectAbsolute
 */
void CustomExplorer::tabChanged(int index)
{
    if (index >= (tabWidget->count() - 1))
        return;

    qDebug() << "CustomExplorer::tabChanged to index" << index;
    DetailedView *detailedView = static_cast<DetailedView *>(tabWidget->currentWidget());

    if (detailedView->rootIndex().isValid()) {

        QSortFilterProxyModel *viewModel = static_cast<QSortFilterProxyModel *>(detailedView->model());
        QModelIndex sourceIndex = viewModel->mapToSource(detailedView->rootIndex());

        QModelIndex treeIndex = treeModel->mapFromSource(sourceIndex);
        treeView->setCurrentIndex(treeIndex);
        treeView->scrollTo(treeIndex);

        pathBar->setHistory(detailedView->getHistory());
        pathBar->selectTreeIndex(treeIndex);
    } else
        expandAndSelectAbsolute(detailedView->getPath());
}

void CustomExplorer::rootChangeFailed(QString path)
{
    qDebug() << "CustomExplorer::rootChangeFailed" << path;

    // Change back current tab to last selected index in Tree View
    QModelIndex index = treeView->selectedItem();
    if (index.isValid()) {
        FileSystemModel *treeViewModel = reinterpret_cast<FileSystemModel *>(treeView->model());
        FileSystemItem *indexItem = static_cast<FileSystemItem*>(index.internalPointer());

        if (indexItem != nullptr && indexItem->getPath() == path) {
            index = index.parent();
            treeView->setCurrentIndex(index);
            indexItem = static_cast<FileSystemItem*>(index.internalPointer());
            qDebug() << index.row() << index.column() << indexItem->getPath();
        }

        tabWidget->setViewRootIndex(index);

        // Refresh

        treeViewModel->removeAllRows(index);

        // Fetch again the children
        indexItem->setHasSubFolders(true);
        treeViewModel->fetchMore(index);

    } else {
        FileSystemModel *treeViewModel = reinterpret_cast<FileSystemModel *>(treeView->model());
        index = treeViewModel->index(0, 0);
        treeView->setCurrentIndex(index);
        tabWidget->setViewRootIndex(index);
    }
}

void CustomExplorer::showError(const QModelIndex &index)
{
    QMessageBox::critical(this, tr("Error"), index.data(FileSystemModel::ErrorMessageRole).toString() + "\n\n" + index.data(FileSystemModel::PathRole).toString());

    // Go back
    DetailedView *detailedView = static_cast<DetailedView *>(tabWidget->currentWidget());
    QModelIndex lastIndex = detailedView->getHistory()->getLastItem();

    if (lastIndex.isValid()) {
        treeView->setCurrentIndex(lastIndex);
        tabWidget->setViewRootIndex(lastIndex);
    }
}

CustomTabWidget *CustomExplorer::getTabWidget() const
{
    return tabWidget;
}

bool CustomExplorer::setViewRootIndex(const QModelIndex &index)
{
    const QModelIndex& source = treeModel->mapToSource(index);
    return tabWidget->setViewRootIndex(source);
}

CustomTreeView *CustomExplorer::getTreeView() const
{
    return treeView;
}

QSplitter *CustomExplorer::getSplitter() const
{
    return splitter;
}

void CustomExplorer::setupGui(int nExplorer)
{
    this->nExplorer = nExplorer;

    splitter = new QSplitter(this);
    splitter->setObjectName(QString::fromUtf8("explorerSplitter") + QString::number(nExplorer));
    splitter->setHandleWidth(1);
    splitter->setChildrenCollapsible(false);

    splitter->setStyleSheet("QSplitter::handle { background: #9fcdb3; }");

    treeView = new CustomTreeView(splitter);
    treeView->setObjectName(QString::fromUtf8("treeView") + QString::number(nExplorer));

    QSizePolicy sizePolicy1(QSizePolicy::MinimumExpanding, QSizePolicy::Expanding);
    sizePolicy1.setHorizontalStretch(0);
    sizePolicy1.setVerticalStretch(0);
    sizePolicy1.setHeightForWidth(treeView->sizePolicy().hasHeightForWidth());

    int baseSize = AppWindow::instance()->getPhysicalPixels(100);
    treeView->setSizePolicy(sizePolicy1);
    treeView->setMinimumSize(QSize(baseSize, 0));
    treeView->setBaseSize(QSize(baseSize, 0));
    treeView->setSizeAdjustPolicy(QAbstractScrollArea::AdjustIgnored);

    splitter->addWidget(treeView);

    treeView->header()->setStretchLastSection(false);

    QWidget *expWidget = new QWidget(this);
    QVBoxLayout *expLayout = new QVBoxLayout(expWidget);
    expLayout->setContentsMargins(0, 0, 0, 0);
    expLayout->setSpacing(0);

    pathBar = new PathBar(expWidget);
    expLayout->addWidget(pathBar);

    tabWidget = new CustomTabWidget(nExplorer, expWidget);
    tabWidget->setObjectName(QString::fromUtf8("tabWidget") + QString::number(nExplorer));
    tabWidget->setStyleSheet("QTabWidget::pane { border: 0px; }");

    QSizePolicy sizePolicy2(QSizePolicy::Expanding, QSizePolicy::Expanding);
    sizePolicy2.setHorizontalStretch(1);
    sizePolicy2.setVerticalStretch(0);
    sizePolicy2.setHeightForWidth(tabWidget->sizePolicy().hasHeightForWidth());

    expWidget->setSizePolicy(sizePolicy2);
    tabWidget->setSizePolicy(sizePolicy2);
    tabWidget->setMinimumSize(QSize(0, 0));

    expLayout->addWidget(tabWidget);

    splitter->addWidget(expWidget);

    setLayout(new QVBoxLayout());
    layout()->setContentsMargins(0, 0, 0, 0);
    layout()->addWidget(splitter);

    int width = Settings::settings->readPaneSetting(nExplorer, SETTINGS_PANES_TREEWIDTH).toInt();
    treeView->resize(width, 0);
}

void CustomExplorer::viewIndexChanged(const QModelIndex &sourceIndex)
{
    const QModelIndex &treeIndex = treeModel->mapFromSource(sourceIndex);
    treeView->setCurrentIndex(treeIndex);

    DetailedView *detailedView = static_cast<DetailedView *>(tabWidget->currentWidget());
    pathBar->setHistory(detailedView->getHistory());
    pathBar->selectTreeIndex(treeIndex);
}
